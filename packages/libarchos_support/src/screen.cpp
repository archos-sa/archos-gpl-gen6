#include "screen.h"
#include <qapplication.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <qgfx_qws.h>

using namespace archos;

ScreenMode Screen::mode()
{
	int dw = qt_screen->width();
	int dh = qt_screen->height();

	if ( dw == 800 && dh == 480 ) {
		return wvga;
	} else if (dw == 1280 && dh == 720 ) {
		return hdmi_720p;
	} else {
		return tv;
	}
}

QString Screen::prefix()
{
	switch (Screen::mode()) {
	case wvga:
		return QString("wvga/");
	case hdmi_720p:
		return QString("hdmi_720p/");
	case tv:
		return QString("tv/");
	}
	// This should not happen
	ASSERT(0);
	return QString();
}
