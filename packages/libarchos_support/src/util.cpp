#include "util.h"
#include <qgfx_qws.h>
#include <qobject.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/ioctl.h>


#define LCD_width_qvga       320
#define LCD_height_qvga      240
#define LCD_width_wide_qvga  480
#define LCD_height_wide_qvga 272
#define LCD_width_wide_vga   800
#define LCD_height_wide_vga  480


int archos::getResolution()
{
#	ifndef SIM
	int fd = ::open( "/dev/fb0", O_RDWR );
	if( fd < 0 ) {
		qWarning("Can't open framebuffer device /dev/fb0");
		return -1;
	}

	fb_var_screeninfo vinfo;
	memset( &vinfo, 0, sizeof(fb_var_screeninfo) );

	if( ioctl( fd, FBIOGET_VSCREENINFO, &vinfo ) ) {
		qFatal("Error reading variable information in card init");
		::close(fd);
		return -1;
	}
	::close(fd);

	int w = vinfo.xres;
	int h = vinfo.yres;

#	else
	int w = qt_screen->width();
	int h = qt_screen->height();
#	endif

	if(w == LCD_width_qvga && h == LCD_height_qvga ) {
		return RESOLUTION_QVGA;

	} else if(w == LCD_width_wide_qvga && h == LCD_height_wide_qvga ) {
		return RESOLUTION_WIDE_QVGA;

	} else if( w == LCD_width_wide_vga  && h == LCD_height_wide_vga  ) {
		return RESOLUTION_WIDE_VGA;

	} else {
		return RESOLUTION_TV;
	} 
}

int archos::getVideoMode()
{
	if( getResolution() == RESOLUTION_TV ) {
		return TvMode;
	} else {
		return LcdMode;
	}
}
