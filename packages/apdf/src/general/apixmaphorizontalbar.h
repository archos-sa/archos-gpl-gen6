#ifndef APIXMAPHORIZONTALBAR_H
#define APIXMAPHORIZONTALBAR_H

#include <qpixmap.h>

class QPainter;
class QString;

class APixmapHorizontalBar {
public:
	enum Look {
		LookLeft,    /* < Text   */
		LookMiddle,  /* | Text   */
		LookRight,   /* | Text > */
		LookWhole    /* < Text > */
	};

	/**
	 * load bar pixmaps.
	 * @p tmplName should be of the form "general/prefix%1.png".
	 * @p fill will be used to replace %1 in tmplName to load the fill pixmap.
	 *    It defaults to "".
	 * @p left will be used to replace %1 in tmplName to load the left pixmap.
	 *    It defaults to "_left".
	 * @p right will be used to replace %1 in tmplName to load the right pixmap.
	 *    It defaults to "_right".
	 */
	void loadPixmaps(const QString& tmplName, const QString& fill = QString(), const QString& left = QString(), const QString& right = QString());

	void draw(QPainter*, int barX, int barY, int width, APixmapHorizontalBar::Look look = LookWhole) const;

	int leftMargin() const;

	int rightMargin() const;

	int height() const;

private:
	QPixmap m_leftPix, m_fillPix, m_rightPix;
};


#endif /* APIXMAPHORIZONTALBAR_H */
