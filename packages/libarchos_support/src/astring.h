#ifndef ASTRING_H
#define ASTRING_H

#include <qstring.h>

namespace archos {

class AString : public QString {

public:
	AString(const QString&);
	void fixRtl(void); // bidi conversion
	static AString import(const char *utf8);
};

};

#endif
