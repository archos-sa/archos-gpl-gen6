#ifndef _AIMAGE_H
#define _AIMAGE_H

enum {
	RSZ_ASYNC = 0,
	RSZ_SYNC
};

enum {
	RSZ_ASAP = 0,
	RSZ_VBL
};

enum {
	RSZ_NORMAL = 0,
	RSZ_BLUR,
	RSZ_NEAREST_NEIGHBOUR
};

namespace archos {

struct window
{
	int x;
	int y;
	int width;
	int height;
};

class AImageData {

public:
	AImageData() : count(1) { };
	virtual ~AImageData() { };

 	void ref() { count++; }
	bool deref() { count--; return count==0; }

	unsigned char *buffer[4];
	int w;
	int h;
	int d;
	int linestep;
	int num_bytes;
	int fmt;
	unsigned int flags;
	struct window window;
private:
	int count;
};

class AImage {

public:
	enum { MemDefault=0, MemDma=1, MemCached=2, MemOwn=4, LineStep32Aligned=8 };
	enum { FormatNonInterleaved=0, FormatInterleaved};
	AImage();
	AImage( const AImage &image );
	AImage(int width, int height, int depth, unsigned int flags);
	AImage(int width, int height, int depth, int fmt, unsigned int flags);
	AImage(int width, int height, int depth, int fmt, int win_x, int win_y, int win_width, int win_height, unsigned int flags);
	//AImage(unsigned char *buf, int numBytes, int width, int height, int depth, unsigned int flags);
	AImage(unsigned char *buf[], int numBytes, int width, int height, int depth, int scanline, int fmt);
	AImage(unsigned char *buf, int width, int height, int depth, int scanline, int fmt);
	
	virtual ~AImage();
	
	bool create(int width, int height, int depth, unsigned int flags);
	bool create(int width, int height, int depth, int fmt, unsigned int flags);
	bool create(int width, int height, int depth, int fmt, int win_x, int win_y, int win_width, int win_height, unsigned int flags);
	bool create(unsigned char *buf[], int bytes, int width, int height, int depth, int l, int format);

	AImage copy() const;
	AImage convert(int depth, int fmt) const;
	AImage scale(int width, int height, int win_x, int win_y, int win_width, int win_height, unsigned int flags) const;
	AImage scale(int percent, unsigned int flags) const;

	int width() const { return data->w; } 
	int height() const { return data->h; } 
	int depth() const { return data->d; } 
	int windowX() const { return data->window.x; }
	int windowY() const { return data->window.y; }
	int windowWidth() const { return data->window.width; }
	int windowHeight() const { return data->window.height; }
	unsigned char * scanline(int ib, int i) const { return data->buffer[ib] + i * data->linestep; }	
	unsigned char *scanline(int i) const { return scanline(0, i); }	

	int format() const { return data->fmt; } 	
	int flags() const { return data->flags; } 	
	int lineStep() const { return data->linestep; }
	unsigned char* pixels(int i) const
	{
//printf("pixels: %d\n", i);
		if (isNull() || i>3)
			return NULL;
		
		return data->buffer[i];
	}

	unsigned char* pixels() const
	{
		return pixels(0);
	}
	
	bool isNull(int i) const
	{
		if (data->buffer[i] == NULL || data->w == 0 || data->h == 0 || data->d == 0)
			return true;
			
		return false;
	}

	bool isNull() const
	{
		return isNull(0);
	}
	
	AImage &operator=(const AImage &image)
	{
		image.data->ref();			
		if ( data->deref() ) {
			reset();
			delete data;
		}
		data = image.data;
		return *this;
	}
	
	int numBytes() const
	{
		if (data->num_bytes != -1)
			return data->num_bytes;
		else
			return data->w * data->h * (data->d>>3);
	}
	
private:
	void init();
	void reinit();
	void reset();
	void freeBuffer();
	int image_resize_accel( AImage &dst, int i, int *cmd_tag, int vblsync, int mode, float coef ) const;
	int image_resize_grey256_accel( AImage &dst, int i ) const;
	int image_resize_grey256_software( AImage &dst, int i) const;
	int image_resize_wait(int tag) const;

	
private:
	AImageData *data;
};


};


#endif /* _AIMAGE_H */
