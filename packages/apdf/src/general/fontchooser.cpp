#include "fontchooser.h"
#include <qtextcodec.h>
#include <qfont.h>
#include <qstring.h>

#include <archos/alog.h>

const QFont FontChooser::getFont(const QString& locale)
{
	ALOG_DEBUG("locale used: '%s'", locale.ascii());
	if (locale.startsWith("zh_TW")) {	// traditional and simplified chinese need different fontfiles.
		return QFont("tc");
	} else if (locale.startsWith("zh_CN")) {
		return QFont("sc");
	} else if (locale.startsWith("he_HE") || locale.startsWith("ru_RU") || locale.startsWith("ar_AR")) {
		return QFont("archos_world");
	} else {
		return QFont("archos");
	}
}

const QFont FontChooser::getFont()
{
	return getFont(QTextCodec::locale());
}
