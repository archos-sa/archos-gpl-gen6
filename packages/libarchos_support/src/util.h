#ifndef UTIL_H
#define UTIL_H

namespace archos {

enum DisplayType {
	LcdMode, 
	TvMode
};

enum Resolution {
	RESOLUTION_QVGA,
	RESOLUTION_WIDE_QVGA, 
	RESOLUTION_WIDE_VGA,
	RESOLUTION_TV
};

int getVideoMode();
int getResolution();

}


#endif // UTIL_H
