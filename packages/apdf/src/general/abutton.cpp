#include "abutton.h"

// Qt
#include <qapplication.h>
#include <qbitmap.h>
#include <qpainter.h>

// Local
#include "style.h"

using namespace archos;

static const int GENERIC_BUTTON_INNER_MARGIN = 10;

AButton::AButton(QWidget *parent, const char * name)
: QPushButton(parent, name)
{
	setWFlags(getWFlags()|Qt::WRepaintNoErase);
	setFocusPolicy(QWidget::NoFocus);
	setButtonType(AStyle::ButtonTypeGeneric);
}

void AButton::setButtonType(AStyle::ButtonType type)
{
	m_buttonType = type;

	AStyle *aStyle = AStyle::get();
	switch (m_buttonType) {
	case AStyle::ButtonTypeStatusbar:
		setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
		setMask(aStyle->statusbarButtonMask());
		break;
	
	case AStyle::ButtonTypeStandard:
		setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
		clearMask();
		break;

	case AStyle::ButtonTypeGeneric:
		setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
		clearMask();
		break;
	}
}

QSize AButton::sizeHint() const
{
	AStyle *aStyle = AStyle::get();
	QSize sh;
	switch (m_buttonType) {
	case AStyle::ButtonTypeStatusbar:
		sh = aStyle->statusbarButtonSize();
		break;

	case AStyle::ButtonTypeStandard:
		sh = aStyle->standardButtonSize();
		break;

	case AStyle::ButtonTypeGeneric:
		QSize sh = QPushButton::sizeHint();
		sh.setWidth(sh.width() + 2 * GENERIC_BUTTON_INNER_MARGIN);
		sh.setHeight(aStyle->genericButtonHeight());
		break;
	}
	sh = sh.expandedTo(QApplication::globalStrut());
	return sh;
}

void AButton::drawButton(QPainter *painter)
{
	AStyle::get()->drawButtonBackground(painter, rect(), m_buttonType, isDown());

	drawButtonLabel(painter);
}

void AButton::drawButtonLabel(QPainter *painter)
{
	const QPixmap *fg = pixmap();
	if (fg) {
		painter->drawPixmap(
			(width() - fg->width()) / 2,
			(height() - fg->height()) / 2,
			*fg);
	} else {
		if (isDown()) {
			painter->setPen(Qt::black);
		} else {
			painter->setPen(Qt::white);
		}
		QRect textRect = rect();
		textRect.moveBy(0, 1); // Match Avos offset
		painter->drawText(textRect, Qt::AlignCenter, text());
	}
}
