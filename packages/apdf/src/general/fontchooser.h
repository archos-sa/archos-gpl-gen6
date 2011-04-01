#ifndef FONT_CHOOSER_H
#define FONT_CHOOSER_H

#include <qfont.h>


class FontChooser {
 public:
	static const QFont getFont();
	static const QFont getFont(const QString&);

 private:
	FontChooser();
};


#endif // FONT_CHOOSER_H
