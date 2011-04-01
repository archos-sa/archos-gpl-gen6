#include "apixmaphorizontalbar.h"

#include "style.h"

#include <archos/alog.h>

#include <qpainter.h>

void APixmapHorizontalBar::loadPixmaps(const QString& tmplName, const QString& _fill, const QString& _left, const QString& _right)
{
	QString fill = _fill.isNull() ? "" : _fill;
	QString left = _left.isNull() ? "_left" : _left;
	QString right = _right.isNull() ? "_right" : _right;

	m_leftPix = AStyle::loadIcon(tmplName.arg(left));
	m_fillPix = AStyle::loadIcon(tmplName.arg(fill));
	m_rightPix = AStyle::loadIcon(tmplName.arg(right));

	int heightL, heightF, heightR;
	heightL = m_leftPix.height();
	heightF = m_fillPix.height();
	heightR = m_rightPix.height();

	if (heightL != heightF || heightL != heightR) {
		ALOG_WARNING("name=%s all heights are not the same (left: %d, fill: %d, right: %d)", tmplName.ascii(), heightL, heightF, heightR);
	}
}


void APixmapHorizontalBar::draw(QPainter *painter, int barX, int barY, int width, APixmapHorizontalBar::Look look) const
{
	bool paintLeft = look == LookLeft || look == LookWhole;
	bool paintRight = look == LookRight || look == LookWhole;

	int leftMargin = paintLeft ? m_leftPix.width() : 0;
	int rightMargin = paintRight ? m_rightPix.width() : 0;
	QRect innerRect(
		barX + leftMargin,
		barY,
		width - leftMargin - rightMargin,
		height());

	if (paintLeft) {
		painter->drawPixmap(barX, barY, m_leftPix);
	}

	painter->drawTiledPixmap(innerRect, m_fillPix);

	if (paintRight) {
		painter->drawPixmap(barX + width - rightMargin, barY, m_rightPix);
	}
}


int APixmapHorizontalBar::height() const
{
	ASSERT(!m_fillPix.isNull());
	return m_fillPix.height();
}


int APixmapHorizontalBar::leftMargin() const
{
	ASSERT(!m_leftPix.isNull());
	return m_leftPix.width();
}


int APixmapHorizontalBar::rightMargin() const
{
	ASSERT(!m_rightPix.isNull());
	return m_rightPix.width();
}
