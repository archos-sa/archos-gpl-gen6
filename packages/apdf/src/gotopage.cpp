#include <stdio.h>
#include "gotopage.h"
#include "general/abutton.h"
#include "general/slider.h"
#include "general/style.h"
#include "general/separator.h"

#include <qapplication.h>
#include <qlabel.h>
#include <qpainter.h>
#include <qbitmap.h>
#include <qgfx_qws.h>
#include <qlayout.h>
#include <archos/atr.h>
#include <archos/screen.h>
#include <archos/astring.h>

#define ALOG_ENABLE_DEBUG
#include <archos/alog.h>

#define OK_ID		0
#define CANCEL_ID	1

using namespace archos;

GotoPage::GotoPage(QWidget *parent, const char *name) :
	QWidget( parent, name,
		QWidget::WStyle_Customize | QWidget::WStyle_NoBorder | WRepaintNoErase | QWidget::WStyle_StaysOnTop)
{
	int dw = qt_screen->width();
	int dh = qt_screen->height();
	int x;
	int margin;

	QFont font = qApp->font();
	if ( Screen::mode() == wvga ) {
		x = 130;
		margin = 24;
		font.setPointSize(22);
	}
	else {  // tv
		x = 100;
		margin = 22;
		font.setPointSize(18);
	}
	setFont(font);

	// Slider
	m_slider = new Slider(this);

	// Slider label
	m_sliderLabel = new QLabel(this);
	m_sliderLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	{
		QFontMetrics fm(m_sliderLabel->font());
		m_sliderLabel->setFixedWidth(fm.width("  0000/0000"));
	}

	QVBoxLayout *mainLayout = new QVBoxLayout(this);
	mainLayout->setMargin(margin);

	QHBoxLayout *sliderLayout = new QHBoxLayout();

	mainLayout->addLayout(sliderLayout);
	sliderLayout->addWidget(m_slider);
	sliderLayout->addWidget(m_sliderLabel);

	// Separator
	Separator *sep = new Separator(this);
	mainLayout->addWidget(sep);
	mainLayout->addItem(new QSpacerItem( 1, 7, QSizePolicy::Maximum, QSizePolicy::Fixed));

	// Ok button
	AButton *okButton = new AButton(this);
	okButton->setButtonType(AStyle::ButtonTypeStandard);
	okButton->setText(AString::import(atr("pdf_ok")));
	connect(okButton, SIGNAL(clicked()), this, SLOT(acked()) );
	mainLayout->addWidget(okButton);

	// Geometry
	setMaximumSize(QSize(dw-40, dh-40));
	adjustSize();
	move((dw - width()) / 2, (dh - height()) / 2);
	connect(m_slider, SIGNAL(valueChanged(int)), this, SLOT(changed(int)));
}

GotoPage::~GotoPage()
{
}

void GotoPage::setValues(int value, int bottom, int top)
{
	m_slider->setValues(value, bottom, top);
	updateSliderLabel();
}

void GotoPage::setValues(int value, int bottom, int top, int steps)
{
	m_slider->setValues(value, bottom, top, steps);
	updateSliderLabel();
}

void GotoPage::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	AStyle::get()->drawPanel(&p, rect());
}

void GotoPage::updateSliderLabel()
{
	QString text = QString("%1/%2").arg(m_slider->getValue()).arg(m_slider->top());
	m_sliderLabel->setText(text);
}

void GotoPage::acked(void)
{
	hide();
	emit selectedValue(m_slider->getValue());
	emit closed();
}

void GotoPage::changed(int v)
{
	updateSliderLabel();
	emit valueChanged(v);
}

void GotoPage::keyPressEvent(QKeyEvent *e)
{
	switch(e->key()) {
		case Key_Left:
		case Key_Right:
		case Key_PageDown:
		case Key_PageUp:
			// as long as a key is presssed a timer will move the slider
			// we dont care for auto repeats.
			if ( !e->isAutoRepeat() ) {
				QApplication::sendEvent(m_slider, e);
			}
			break;
		default:
			break;
	}
}

void GotoPage::keyReleaseEvent(QKeyEvent *e)
{
	switch(e->key()) {
		case Key_Return:
			acked();
			break;
		case Key_Left:
		case Key_Right:
		case Key_PageDown:
		case Key_PageUp:
			// as long as a key is presssed a timer will move the slider
			// we dont care for auto repeats.
			if ( !e->isAutoRepeat() ) {
				QApplication::sendEvent(m_slider, e);
			}
			break;
		case Key_F2:
			hide();
			break;
		default:
			break;
	}
}

void GotoPage::showEvent(QShowEvent*)
{
	updateMask();
	setFocus();
}

void GotoPage::updateMask()
{
	QBitmap mask(size());
	{	QPainter p(&mask);
		AStyle::get()->drawPanelMask(&p, rect());
	}
	setMask(mask);	
}


int GotoPage::getValue(void)
{
	return m_slider->getValue();
}
