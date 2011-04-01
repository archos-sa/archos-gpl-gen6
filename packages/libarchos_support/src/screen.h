#ifndef SCREEN_H
#define SCREEN_H

#include <qstring.h>

namespace archos {

enum ScreenMode {
	wvga,
	hdmi_720p,
	tv
};

class Screen {
	public:
		static ScreenMode mode();
		static QString prefix();
};

};

#endif // SCREEN_H
