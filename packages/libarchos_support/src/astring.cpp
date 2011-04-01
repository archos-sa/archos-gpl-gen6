#include <qstring.h>
#include <wchar.h>
#include <stdio.h>
#include <qtextcodec.h>

#include "astring.h"
#include "awchar.h"
#include "harfbuzz.h"

using namespace archos;

archos::AString::AString(const QString& n) : QString(n) {}

static bool _is_arabic( awchar *translation )
{
	int i = 0;

	for ( i = 0; i < awstrlen(translation); i++ ) {
		if ( ( ( translation[i] >= 0x0600 ) && ( translation[i] <= 0x06FF ) )
			|| ( ( translation[i] >= 0x0750 ) && ( translation[i] <= 0x077F ) )
			|| ( ( translation[i] >= 0xFB50 ) && ( translation[i] <= 0xFDFF ) )
			|| ( ( translation[i] >= 0xFE70 ) && ( translation[i] <= 0xFEFF ) ) ) {
			return TRUE;
		}
	}

	return FALSE;
}

static void _fixRtl( awchar *translation, awchar *out )
{
	memset( out, 0, 256 );

	if (QString(QTextCodec::locale()).startsWith("ar_AR")) {
		harfbuzz_arabic( (awchar *)translation, awstrlen(translation) );

		// Check if the translation is actually containing arabic characters
		// Could be a missing translation -> western characters
		if ( _is_arabic( translation ) ) {
			int i;
			for ( i = 0; i < awstrlen(translation); i++ ) {
				out[i] = translation[awstrlen(translation) - 1 - i];
			}
		} else {
			awstrncpy( out, translation, awstrlen(translation) );
		}
	} else {
		// Hebrew
		const awchar *tmp = bidi( (const awchar*)translation );
		awstrncpy( out, (awchar*)tmp, awstrlen(tmp) );
	}
}

int utf8_to_utf16(awchar *utf16, const unsigned char *utf8, int utf16_max )
{
	unsigned short *utf16_0;
	unsigned char 	c;

	utf16_0 = utf16;
	while ( (c = *utf8++) && utf16_max-- > 0 ) {
		unsigned short w = 0;
		if ( (c & 0x80) == 0 ) {
			w = c;
		} else if ( (c & 0xE0) == 0xC0) {
			w = (c & 0x1F);
			w <<= 6;
			c = *utf8++;
			w |= (c & 0x3F);
		} else if ( (c & 0xF0) == 0xE0) {
			w = (c & 0x1F);
			w <<= 6;
			c = *utf8++;
			w |= (c & 0x3F);
			w <<= 6;
			c = *utf8++;
			w |= (c & 0x3F);
		} else {
			w = '?';
		}
		*utf16++ = w;
	}
	
	*utf16 = 0x0000;
	return ( utf16 - utf16_0 );
}

AString archos::AString::import(const char * utf8)
{
	if ((QString(QTextCodec::locale()).startsWith("he_HE")) || (QString(QTextCodec::locale()).startsWith("ar_AR"))) {
		awchar buf[1024];
		awchar buf2[1024];
		int len = utf8_to_utf16((awchar*)buf, (const unsigned char*)utf8, 256);

		_fixRtl( buf, buf2 );

		QChar qchar_tab[1024];
		int i = 0;
		for ( i = 0; i < awstrlen( buf2 ); i++ ) {
			qchar_tab[i] = QChar( (unsigned int)buf2[i] );
		}

		AString ret(QString( (QChar *)qchar_tab, awstrlen( buf2 ) ));

		return ret;
	}
	else {
		return AString::fromUtf8(utf8);
	}
}

size_t awstrlen(const awchar *string)
{
	size_t n = 0;
	while ( *string != (awchar)0x0000 ) {
		string++;
		n++;
	}
	return n;
}

awchar *awstrncpy(register awchar *dest, register const awchar *src, register size_t n)
{
	if (n)
	{
		register awchar       *d = dest - 1;
		register const awchar *s = src - 1;
		while ( (*++d = *++s) != 0x0000 && --n);
		if ( n-- > 1 ) {
			do {
				*++d = 0x0000;
			}
			while (--n);
		}
	}
	return dest;
}

const char *w2c( const awchar *wide )
{
	static char out[256 + 1];
	char *c = out;
	int len = 0;
	
	while( *wide && len++ < 256 )
		*c++ = *wide++;
	*c = 0;
	return out;
}

#if 0

int main(int, char**)
{
	AString in(AString::import("te הסתר כיתוב he"));
}

#endif
