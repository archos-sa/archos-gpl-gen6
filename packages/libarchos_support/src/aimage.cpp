#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "dmalloc.h"
#include "aimage.h"

#define CONFIG_RESIZE_ACCEL

#if !defined(SIM) && defined(CONFIG_RESIZE_ACCEL)

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

extern "C" {

#include <linux/davinci-resz.h>
#include "davinci-resz-coef.h"

}

#define  NUM_PHASES 8
#define  NUM_TAPS   4
#define  NUM_D2PH   4  /* for downsampling 2+x ~ 4x, number of phases */
#define  NUM_D2TAPS 7  /* for downsampling 2+x ~ 4x, number of taps */
#define  NUM_COEFS (NUM_PHASES * NUM_TAPS) 

/* filter bit depth = 10-bit = sign + 1.8 */ 
#define  FIR_RND_ADD 128
#define  FIR_RND_SHIFT 8
#define  FIR_RND_SCALE 256

#endif

#define PIXEL_WIDTH_OFF		2
#define PIXEL_HEIGHT_OFF	2

#if !defined(SIM) && defined(CONFIG_RESIZE_ACCEL)
static int resz_fd = -1;

extern "C" {
// ****************************************************
//
//	_compute_filter_nearest_neighbour
//
// ****************************************************
static void _compute_filter_nearest_neighbour(short coef_arr[], int rsz, float coef)
{
	int   i;

	for( i = 0 ; i < 32 ; i++ )
		coef_arr[i] = 0;

	if( rsz > 512 ){
		for( i = 0 ; i < 4 ; i++ )
			coef_arr[8*i] = (short)(((float)FIR_RND_SCALE) * coef);
	} else {
		for( i = 0 ; i < 8 ; i++ )
			coef_arr[4*i] = (short)((float)FIR_RND_SCALE * coef);
	}
}

// ****************************************************
//
//	_compute_filter_blur
//	FIXME: we could find a simpler way to compute these...
//
// ****************************************************
static void _compute_filter_blur(short coef_arr[], int rsz)
{
	int   i, j;
	int   u, d, m, ntaps;
	int   nphases, w;
	double win, t;
	double interlv_coef[NUM_COEFS+1];
	double deinterlv_coef[NUM_COEFS+1];
	double acc;
	double scale;
	int   ofst_per_phase;
	int   max_idx, max_coef;
	long  acc_long;

	u = 256;  
	d = rsz;
	m = (u > d) ? u : d;
	ntaps = (rsz > 512) ? NUM_D2TAPS : NUM_TAPS;  /* < 1/2x -> 7-tap */
	nphases = (rsz > 512) ? NUM_D2PH : NUM_PHASES; 
	ofst_per_phase = (rsz > 512) ? NUM_TAPS*2 : NUM_TAPS; /* 4 or 8, never 7 */
	w = nphases * ntaps;

	for (i=0; i<=w; i++) { /* note that we need one extra here */

		win = (i < w/2) ? i : (w-i);

		t = ((double) i - w/2) * u / nphases;
		interlv_coef[i] = win;
	}

	for (j=0; j<NUM_COEFS; j++)   
		coef_arr[j] = 0;          /* prefill 0 so that 7-tap mode gets 8th tap=0 */

	for (j=0; j<nphases; j++)   /* j=phase */
		  for (i=0; i<ntaps; i++)   /* i=tap */
			  deinterlv_coef[ofst_per_phase*j + i] = interlv_coef[nphases*i + nphases - j];

	/* normalize for each phase */
	for (j=0; j<nphases; j++) {  /* j=phase */
		acc = 0.0;
		for (i=0; i<ntaps; i++) /* i=tap */
			acc += deinterlv_coef[ofst_per_phase*j + i];
		scale = 1.0 / acc;  /* normalize sum of weights to 1.0 */
		for (i=0; i<ntaps; i++) /* taps */
			deinterlv_coef[ofst_per_phase*j + i] = deinterlv_coef[ofst_per_phase*j + i] * scale;
	}

	/* convert to S10Q8 format */
	for (j=0; j<nphases; j++) { /* j=phase */
		for (i=0; i<ntaps; i++) { /* taps */
			double d_coef = deinterlv_coef[ofst_per_phase*j + i] * FIR_RND_SCALE + 0.5;
			if (d_coef > 511) /* don't forget to saturate when rounding up! */
				d_coef = 511;
			coef_arr[ofst_per_phase*j + i] = (short) (d_coef);
		}
	}

	/* sum up each phase and adjust largest coef to comprehend rounding errors 
	 * in previous pass 
	 */
	for (j=0; j<nphases; j++) {  /* j=phase */
		acc_long = 0;
		max_coef = 0;
		max_idx = 0;
		for (i=0; i<ntaps; i++) { /* taps */
			acc_long += coef_arr[ofst_per_phase*j + i];
			if (coef_arr[ofst_per_phase*j + i] > max_coef) {
				max_coef = coef_arr[ofst_per_phase*j + i];
				max_idx = ofst_per_phase*j + i;
			}
		}
		acc_long = FIR_RND_SCALE - acc_long;
		coef_arr[max_idx] += acc_long;
	}
}

// ****************************************************
//
//	_compute_filter
//
// ****************************************************
static int _compute_filter(short coef_arr[], int rsz, int mode, float coef)
{
	switch( mode ){
		case RSZ_NEAREST_NEIGHBOUR:
			_compute_filter_nearest_neighbour( coef_arr, rsz, coef );
			return 0;
		case RSZ_BLUR:
			_compute_filter_blur( coef_arr, rsz );
			return 0;
		case RSZ_NORMAL:
			memcpy( coef_arr, &davinci_resz_coef[rsz - 64], sizeof(davinci_resz_coef[0]));
			return 0;
	}
	return 1;
}

}

// ****************************************************
//
//	image_resize_accel
//
// 	hardware accelerated stretch-blit
//      Not a static, needed in image_rgb16.c
// ****************************************************
int archos::AImage::image_resize_accel( AImage &dst, int i, int *cmd_tag, int vblsync, int mode, float coef ) const
{
	int h_rsz, v_rsz, pixel_h = PIXEL_HEIGHT_OFF, pixel_w = PIXEL_WIDTH_OFF;
	vpssrsz_param_cmd_t cmd_data;
	
	memset(&cmd_data, 0, sizeof(cmd_data));

	if( data->window.width == 0 || data->window.height == 0 ) {
printf("IMG: invalid src %dx%d\n", data->window.width, data->window.height );
		return -1;
	}

	if( dst.windowWidth() == 0 || dst.windowHeight() == 0 ) {
printf("IMG: invalid dst %dx%d\n", dst.windowWidth(), dst.windowHeight() );
		return -1;
	}

	if( data->window.width <= (2 * PIXEL_WIDTH_OFF) || mode == RSZ_NEAREST_NEIGHBOUR ){
		// if picture is too small or if we turned off filtering
		pixel_w = 0;
	}

	if( data->window.height <= (2 * PIXEL_HEIGHT_OFF) || mode == RSZ_NEAREST_NEIGHBOUR ){
		// if picture is too small or if we turned off filtering
		pixel_h = 0;
	}

	h_rsz = 256 * (data->window.width  - 2 * pixel_w) / dst.windowWidth();
	v_rsz = 256 * (data->window.height - 2 * pixel_h) / dst.windowHeight();

	if( h_rsz < 64 || h_rsz > 1024 ){
printf("IMG: invalid horizontal ratio ( h_rsz = %d )\n", h_rsz );
		return -1;
	}

	if( v_rsz < 64 || v_rsz > 1024 ){
printf("IMG: invalid vertical ratio ( v_rsz = %d )\n", v_rsz );
		return -1;
	}

	cmd_data.size              = sizeof(cmd_data);
	//cmd_data.src_rect.addr     = (unsigned long)PIXELPTR(src, data->window.x + pixel_w, data->window.y + pixel_h);
	cmd_data.src_rect.addr     = (unsigned long)(pixels(i) + (data->window.y+pixel_h) * lineStep() + data->window.x + pixel_w);
	cmd_data.src_rect.linestep = (unsigned int)lineStep();
	cmd_data.src_rect.height   = (unsigned int)data->window.height - 2 * pixel_h;
	cmd_data.src_rect.width    = (unsigned int)data->window.width  - 2 * pixel_w;

	cmd_data.dst_rect.addr     = (unsigned long)(dst.pixels(i) + (dst.windowY()) * dst.lineStep() + dst.windowX());
	cmd_data.dst_rect.linestep = (unsigned int)dst.lineStep();
	cmd_data.dst_rect.height   = (unsigned int)dst.windowHeight();
	cmd_data.dst_rect.width    = (unsigned int)dst.windowWidth() & ~1;
	
	cmd_data.h_rsz = h_rsz;
	cmd_data.v_rsz = v_rsz;
	
	_compute_filter( cmd_data.v_coeff.coeff, v_rsz, mode, coef );
	_compute_filter( cmd_data.h_coeff.coeff, h_rsz, mode, coef );

	/* synchronize to VBL ? */
	cmd_data.vbl_sync = vblsync;	

	/* image format ? */
	cmd_data.format = 1; // grayscale

	if (resz_fd == -1) {
		resz_fd = open("/dev/vpssresz", O_RDWR);
		fcntl(resz_fd, F_SETFD, FD_CLOEXEC);
	}

	if (data->flags & (MemDma | MemCached))
		msync(pixels(i), height() * lineStep(), MS_SYNC);

	int err = ioctl(resz_fd, VPSSRSZ_CMD_PARAMBLT_ASYNC, &cmd_data);
	if (err < 0) {
printf("IMG: resize failed: %s\r\n", strerror(errno));
		return -1;
	}

	*cmd_tag = err;

	return 0;
}
#endif

// ****************************************************
//
//	image_resize_grey256_software
//
// ****************************************************
int archos::AImage::image_resize_grey256_software( AImage &dst, int i) const
{
	unsigned char *out_ptr, *in_ptr;

	int out_x, out_y;
	int in_x, in_y;
	int fine_in_x, fine_in_y;

	if( data->window.width == 0 || data->window.height == 0 ) {
printf("IMG: invalid src %dx%d\n", data->window.width, data->window.height );
		return -1;
	}

	if( dst.windowWidth() == 0 || dst.windowHeight() == 0 ) {
printf("IMG: invalid dst %dx%d\n", dst.windowWidth(), dst.windowHeight() );
		return -1;
	}

	int in_w = data->window.width;
	int in_h = data->window.height;
	int out_w = dst.windowWidth();
	int out_h = dst.windowHeight();

	//if( out_h == 0 || out_w == 0 )
		//return 0;

//printf( "%s %dx%d --> %dx%d \r\n", __FUNCTION__, in_w , in_h, out_w, out_h );

	int h_rsz = ( in_w << 10 ) / out_w;
	int v_rsz = ( in_h << 10 ) / out_h;
	fine_in_y = 0;
	for( out_y = 0 ; out_y < out_h ; out_y++ ) {

		fine_in_x = 0;
		out_ptr = dst.pixels(i) + (dst.windowY()+out_y)*dst.lineStep() + dst.windowX();
		//out_ptr = (UCHAR*)( PIXELPTR( dst_img, dst_img->window.x, dst_img->window.y + out_y ));
//printf("linestep: %d\n", dst.lineStep());
		for (out_x = 0 ; out_x < out_w ; out_x++ ) { 
			
			in_x = fine_in_x >> 10;
			in_y = fine_in_y >> 10;

			in_ptr = pixels(i) + (data->window.y+in_y)*lineStep() + data->window.x+in_x;
			//in_ptr = (UCHAR*)( PIXELPTR( src_img, src_img->window.x + in_x, src_img->window.y + in_y ));
			*out_ptr++ = *in_ptr;

			fine_in_x += h_rsz;

		}

		fine_in_y += v_rsz;
	}

	return 0;
}

#if !defined(SIM) && defined(CONFIG_RESIZE_ACCEL)
// ****************************************************
//
//	_image_resize_wait
//
// ****************************************************
int archos::AImage::image_resize_wait(int tag) const
{
	if (resz_fd < 0)
		return -1;

	if (ioctl(resz_fd, VPSSRSZ_CMD_WAIT_SYNC, &tag) < 0) {
printf("IMG: resize failed: %s\r\n", strerror(errno));
		return -1;
	}

	return 0;
}

// *****************************************************************************
//
//	image_resize_grey256_accel
//	
// *****************************************************************************
int archos::AImage::image_resize_grey256_accel( AImage &dst, int i ) const
{
	int tag = 0; 
//printf("image_resize_grey256_accel\n");

	if (format() != FormatInterleaved || depth() != 24)
	{
printf("dst format not supported, %d, depth: %d\n", format(), depth());
		return -1;
	}

	if (dst.format() != FormatInterleaved || dst.depth() != 24)
	{
printf("dst format not supported, %d, depth: %d\n", dst.format(), dst.depth());
		return -1;
	}
	
	int err = image_resize_accel( dst, i, &tag, RSZ_ASAP, RSZ_NORMAL, 1 );
	if ( err ) {
		/* we could not do it by hardware, do it in software */
		if (image_resize_grey256_software( dst, i ) < 0)
			return -1;
	}

	return tag;
}

#define IMAGE_RESIZE_GREY	image_resize_grey256_accel
#define RESIZE_WAIT(tag)	image_resize_wait(tag)
#else
#define IMAGE_RESIZE_GREY	image_resize_grey256_software
#define RESIZE_WAIT(tag)	do {} while (0)
#endif

/*****************************************************/

void archos::AImage::init()
{
	data = new AImageData;
	reinit();
}

void archos::AImage::reinit()
{
	data->w = data->h = data->d = 0;
	data->num_bytes = 0;
	data->linestep = 0;
	data->buffer[0] = 0;
	data->buffer[1] = 0;
	data->buffer[2] = 0;
	data->buffer[3] = 0;
	data->fmt = FormatNonInterleaved;
	data->flags = 0;
	data->window.x = 0;
	data->window.y = 0;
	data->window.width = 0;
	data->window.height = 0;
}

void archos::AImage::reset()
{
	freeBuffer();
	reinit();
}

/*****************************************************/

void archos::AImage::freeBuffer()
{
	if (data->buffer[0] && !(data->flags & MemOwn))
	{
		if (data->flags & MemDma)
		{
//printf("DMALLOC_free: %08x, %d\n", data->buffer[0], data->num_bytes);
			DMALLOC_free(data->buffer[0]);
		}
		else
		{
//printf("free: %08x, %d\n", data->buffer[0], data->num_bytes);
			free(data->buffer[0]);
		}	
		data->buffer[0] = 0;	
	}
}

archos::AImage::AImage() 
{
	init();
};

archos::AImage::AImage( const AImage &image )
{
    data = image.data;
    data->ref();
}

archos::AImage::AImage(int width, int height, int depth, int fmt, int win_x, int win_y, int win_width, int win_height, unsigned int _flags)
{
	init();
	create(width, height, depth, fmt, win_x, win_y, win_width, win_height, _flags);
}

archos::AImage::AImage(int width, int height, int depth, int fmt, unsigned int _flags) 
{
	init();
	create(width, height, depth, fmt, _flags);
}

archos::AImage::AImage(int width, int height, int depth, unsigned int _flags) {

	init();
	create(width, height, depth, _flags);
}

archos::AImage::AImage(unsigned char *buf, int width, int height, int depth, int scanline, int fmt)
{
	init();
	create(&buf, scanline*height, width, height, depth, scanline, fmt);
}

archos::AImage::AImage(unsigned char *buf[], int bytes, int width, int height, int depth, int l, int format = FormatInterleaved)
{
	init();
	create(buf, bytes, width, height, depth, l, format);
}

archos::AImage::~AImage()
{
	if ( data && data->deref() ) {
		reset();
		delete data;
	}
}

bool archos::AImage::create(int w, int h, int d, int f, int win_x, int win_y, int win_width, int win_height, unsigned int _flags)
{
		reset();
	if (w == 0 || h == 0 || d == 0)
		goto failed;
			
	data->w = w;
	data->h = h;
	data->d = d;
	data->fmt = f;
	data->flags = _flags;
	data->window.x = win_x;
	data->window.y = win_y;
	data->window.width = win_width;
	data->window.height = win_height;
		
	if (data->fmt == FormatInterleaved && data->d == 24)
	{
		int lstep;
		
		if (data->flags & LineStep32Aligned)
			lstep = (w + 31) & ~0x1f; // 32 byte aligned
		else
			lstep = w;
//printf("lstep: %d, w: %d\n", lstep, w);
			
		data->num_bytes = lstep*h*3;
		
		data->linestep = lstep;
	}
	else
	{
		int lstep = w * (d >> 3);

		if (data->flags & LineStep32Aligned)
			lstep = ((lstep + 31) & ~0x1f); // 32 byte aligned
		
//printf("lstep: %d, w: %d\n", lstep, w);
		data->num_bytes = lstep * h;
		
		data->linestep = lstep;
	}
	
	
	if (data->flags & MemDma)
		data->buffer[0] = (unsigned char*)DMALLOC_alloc(data->num_bytes);	
	else
		data->buffer[0] = (unsigned char*)malloc(data->num_bytes);

	if (data->buffer[0] == NULL)
		goto failed;
	
	if (data->fmt == FormatInterleaved && data->d == 24)
	{
		data->buffer[1] = data->buffer[0] + data->linestep * h;
		data->buffer[2] = data->buffer[1] + data->linestep * h;
		data->buffer[3] = NULL;
	}
	else
	{
		data->buffer[1] = NULL;
		data->buffer[2] = NULL;
		data->buffer[3] = NULL;
	}
	
	return true;
failed:
printf("create failed\n");
	data->w = 0;
	data->h = 0;
	data->d = 0;
	data->linestep = 0;
	data->flags = 0;
	data->buffer[0] = NULL;
	data->buffer[1] = NULL;
	data->buffer[2] = NULL;
	data->buffer[3] = NULL;
	return false;
}

bool archos::AImage::create(int w, int h, int d, int f, unsigned int _flags)
{
	return create(w, h, d, f, 0, 0, 0, 0, _flags);
}

bool archos::AImage::create(int w, int h, int d, unsigned int _flags)
{
	return create(w, h, d, FormatNonInterleaved, 0, 0, 0, 0, _flags);
}


bool archos::AImage::create(unsigned char *buf[], int bytes, int w, int h, int d, int l, int f = FormatInterleaved)
{
	reset();
	
	if ((d == 24 || d == 32) && f == FormatInterleaved)
	{	
		data->buffer[0] = buf[0];
		data->buffer[1] = buf[1];
		data->buffer[2] = buf[2];
		if (d == 32)
			data->buffer[3] = buf[3];
	}
	else if (f == FormatNonInterleaved)
	{
		data->buffer[0] = buf[0];
		if (data->buffer[0] == NULL)
			goto failed;
	
	}
	
	data->w = w;
	data->h = h;
	data->d = d;
	data->linestep = l;
	data->num_bytes = bytes;
	data->flags = MemOwn;
	if (!(l & 0x1f))
		data->flags |= LineStep32Aligned;
	data->fmt = f;
	return true;
	
failed:
	data->w = 0;
	data->h = 0;
	data->d = 0;
	data->flags = 0;
	data->buffer[0] = NULL;
	data->buffer[1] = NULL;
	data->buffer[2] = NULL;
	data->buffer[3] = NULL;
	return false;
}

archos::AImage archos::AImage::copy() const
{
	if (isNull())
		return AImage();
	
	AImage dst(width(), height(), depth(), data->flags);
	
	if (dst.isNull())
		return AImage();
		
	memcpy(dst.pixels(), pixels(), numBytes());
	
	return dst; 	
}

archos::AImage archos::AImage::convert(int d, int f) const
{
	if (isNull())
		return AImage();

	AImage	dst;

	if (d == 16)
	{
		if (depth() == 16)
		{
			if (format() == f)
				return copy();
		}
		else if (depth() == 24)
		{
			if (f == FormatNonInterleaved && format() == FormatInterleaved)
			{
				dst.create(width(), height(), d, 0, (flags() & LineStep32Aligned));
			
				//if (dst.isNull())
					//return null();

//printf("lineStep: %d, lineStep: %d, %d, %08x!\n", lineStep(), dst.lineStep(), lineStep() & 3, flags() & LineStep32Aligned);
				if (lineStep() & 1 || lineStep()*2 != dst.lineStep())
				{
					for (int i=0; i<height(); i++)
					{
						unsigned char *ptrR = scanline(0, i);
						unsigned char *ptrG = scanline(1, i);
						unsigned char *ptrB = scanline(2, i);
				
						unsigned short *dptr = (unsigned short*)dst.scanline(i);
				
						for (int j=0; j<width(); j++)
						{
							*dptr++ = (((unsigned short)(*ptrB++)) & 0xf8) << 8
								| (((unsigned short)(*ptrG++)) & 0xfc) << 3
								| (((unsigned short)(*ptrR++)) & 0xf8) >> 3;
							
						}
					}
				}
				else
				{
				 	if (lineStep() & 3)
					{
						unsigned long *dptr = (unsigned long*)dst.pixels();
						unsigned short *ptrR = (unsigned short*)pixels(0);
						unsigned short *ptrG = (unsigned short*)pixels(1);
						unsigned short *ptrB = (unsigned short*)pixels(2);
				
						for (int i=0; i<height()*lineStep()>>1; i++)
						{		
							*dptr++ = (((unsigned long)(*ptrB)) <<  8) & 0x0000f800
								| (((unsigned long)(*ptrG)) <<  3) & 0x000007e0
								| (((unsigned long)(*ptrR)) >>  3) & 0x0000001f
								| (((unsigned long)(*ptrB)) << 16) & 0xf8000000
								| (((unsigned long)(*ptrG)) << 11) & 0x07e00000
								| (((unsigned long)(*ptrR)) <<  5) & 0x001f0000;
						
							ptrR++;
							ptrG++;
							ptrB++;
						}
					}
					else
					{
						unsigned long *dptr = (unsigned long*)dst.pixels();
						unsigned long *ptrR = (unsigned long*)pixels(0);
						unsigned long *ptrG = (unsigned long*)pixels(1);
						unsigned long *ptrB = (unsigned long*)pixels(2);
				
						for (int i=0; i<height()*lineStep()>>2; i++)
						{		
							*dptr++ = (((unsigned long)(*ptrB)) <<  8) & 0x0000f800
								| (((unsigned long)(*ptrG)) <<  3) & 0x000007e0
								| (((unsigned long)(*ptrR)) >>  3) & 0x0000001f
								| (((unsigned long)(*ptrB)) << 16) & 0xf8000000
								| (((unsigned long)(*ptrG)) << 11) & 0x07e00000
								| (((unsigned long)(*ptrR)) <<  5) & 0x001f0000;
							
							*dptr++ = (((unsigned long)(*ptrB)) >>  8) & 0x0000f800
								| (((unsigned long)(*ptrG)) >> 13) & 0x000007e0
								| (((unsigned long)(*ptrR)) >> 19) & 0x0000001f
								| (((unsigned long)(*ptrB)) <<  0) & 0xf8000000
								| (((unsigned long)(*ptrG)) >>  5) & 0x07e00000
								| (((unsigned long)(*ptrR)) >> 11) & 0x001f0000;
						
							ptrR++;
							ptrG++;
							ptrB++;
						}
					}
				}
				
				
				return dst;
			}
		}
	}
	else if (d == 24)
	{
		if (depth() == 24)
		{
			if (format() == f)
			{
				return copy();
			}
			else if (f == FormatNonInterleaved && format() == FormatInterleaved)
			{
				dst.create(width(), height(), d, 0, (flags() & LineStep32Aligned));
			
				if (dst.isNull())
					return AImage();
			
				for (int i=0; i<height(); i++)
				{
					unsigned char *ptrR = scanline(0, i);
					unsigned char *ptrG = scanline(1, i);
					unsigned char *ptrB = scanline(2, i);
				
					unsigned short *dptr = (unsigned short*)dst.scanline(i);
				
					for (int j=0; j<width(); j++)
					{
						*dptr++ = *ptrR++;
						*dptr++ = *ptrG++;
						*dptr++ = *ptrB++;		
					}
				}
				
				return dst;
			}
		}
	}
	else if (d == 32)
	{
		if (depth() == 24)
		{
			if (f == FormatNonInterleaved && format() == FormatInterleaved)
			{
				dst.create(width(), height(), d, 0, (flags() & LineStep32Aligned));
			
				//if (dst.isNull())
					//return null();

				for (int i=0; i<height(); i++)
				{
					unsigned char *ptrR = scanline(0, i);
					unsigned char *ptrG = scanline(1, i);
					unsigned char *ptrB = scanline(2, i);
				
					unsigned long *dptr = (unsigned long*)dst.scanline(i);
				
					for (int j=0; j<width(); j++)
					{
						*dptr++ =
							((unsigned long)(*ptrR++))
							| ((unsigned long)(*ptrG++)) << 8
							| ((unsigned long)(*ptrB++)) << 16
							| 0xff000000;
					}
				}
				return dst;
			}
		}
		else if (depth() == 32)
		{
			if (f == FormatNonInterleaved && format() == FormatInterleaved)
			{
				dst.create(width(), height(), d, 0);
			
				if (dst.isNull())
					return AImage();

				for (int i=0; i<height(); i++)
				{
					unsigned char *ptrR = scanline(0, i);
					unsigned char *ptrG = scanline(1, i);
					unsigned char *ptrB = scanline(2, i);
					unsigned char *ptrA = scanline(3, i);
				
					unsigned char *dptr = dst.scanline(i);
				
					for (int j=0; j<width(); j++)
					{
						*dptr++ = *ptrR++;
						*dptr++ = *ptrG++;
						*dptr++ = *ptrB++;
						*dptr++ = *ptrA++;		
					}
				}
				
				return dst;
			}
		}
	}

printf("format not supported: dst depth: %d, dst format: %d, src depth: %d, src format: %d\n",
		d, f, depth(), format());
	return AImage();
}

archos::AImage archos::AImage::scale(int w, int h, int win_x, int win_y, int win_width, int win_height, unsigned int dst_flags) const
{
	if (isNull())
		return AImage();
	
	data->window.x = 0;
	data->window.y = 0;
	data->window.width = width();
	data->window.height = height();
	
	AImage dst(w, h, depth(), FormatInterleaved, win_x, win_y, win_width, win_height, dst_flags | MemDma | (flags() & LineStep32Aligned));
	
	if (dst.isNull())
		return AImage();
		
	int tag;
	
	if ((tag = IMAGE_RESIZE_GREY(dst, 0)) < 0)
		return AImage();
	if ((tag = IMAGE_RESIZE_GREY(dst, 1)) < 0)
		return AImage();
	if ((tag = IMAGE_RESIZE_GREY(dst, 2)) < 0)
		return AImage();
	
	RESIZE_WAIT(tag);
	
	return dst; 	
}

#define RESIZER_CONSTRAINT 256

static inline int calcZfRounded(int s, int d) {
	return ((s * RESIZER_CONSTRAINT * 2 / d) + 1) / 2;
}

archos::AImage archos::AImage::scale(int percent, unsigned int dst_flags) const
{
	if (isNull())
		return AImage();
	
	int dst_width = width() * percent / 100;
	int zf = calcZfRounded(width(), dst_width);
	
//printf("zf: %d, src w: %d, lstep: %d, dst w: %d, percent: %d\n", zf, width(), lineStep(), dst_width, percent);	 	
	
	data->window.x = 0;
	data->window.y = 0;
	data->window.width = width() & ~0x1;
	data->window.height = height();
	
	AImage dst(width() * RESIZER_CONSTRAINT / zf, height() * RESIZER_CONSTRAINT / zf, depth(), FormatInterleaved, 0, 0, width() * RESIZER_CONSTRAINT / zf, height() * RESIZER_CONSTRAINT / zf, dst_flags | MemDma | (flags() & LineStep32Aligned));

//printf("real dst w: %d, dst lstep: %d\n", width() * RESIZER_CONSTRAINT / zf, dst.lineStep());	 	
	
	if (dst.isNull())
		return AImage();
		
	int tag;
	
	if ((tag = IMAGE_RESIZE_GREY(dst, 0)) < 0)
		return AImage();
	if ((tag = IMAGE_RESIZE_GREY(dst, 1)) < 0)
		return AImage();
	if ((tag = IMAGE_RESIZE_GREY(dst, 2)) < 0)
		return AImage();
	
	RESIZE_WAIT(tag);

//printf("resizing done\n");
	return dst; 	
}

