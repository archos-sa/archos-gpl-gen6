#include "abutton.h"
#include "apixmaphorizontalbar.h"
#include "slider.h"
#include "style.h"

#include <qpainter.h>
#include <qapplication.h>
#include <qsize.h>
#include <assert.h>
#include <qgfx_qws.h>

using namespace archos;

static const int SPACING = 10;

static const int START_INTERVAL = 150 * 4;
static const int FINAL_INTERVAL = 150;

SpeedUpTimer::SpeedUpTimer(QObject *parent)
	: QTimer(parent)
	, m_startInterval(0)
	, m_finalInterval(0)
	, m_currentInterval(0)
{
	connect(this, SIGNAL(timeout()), this, SLOT(decreaseInterval()));
};

void SpeedUpTimer::start()
{
	QTimer::start(m_startInterval);
}

void SpeedUpTimer::stop()
{
	QTimer::stop();
	m_currentInterval = m_startInterval;
}

// start interval must be higher than the finalInterval
void SpeedUpTimer::setIntervals(int startInterval, int finalInterval)
{
	QTimer::stop();
	m_startInterval = startInterval;
	m_finalInterval = finalInterval;
}

void SpeedUpTimer::decreaseInterval()
{
	if (m_currentInterval == m_finalInterval) {
		return;
	}

	m_currentInterval /= 2;
	if (m_currentInterval < m_finalInterval) {
		m_currentInterval = m_finalInterval;
	}
	changeInterval(m_currentInterval);
}


//// Slider ////
Slider::Slider(QWidget *parent, const char *name)
	: QWidget(parent, name, WRepaintNoErase)
	, m_steps(-1)
	, m_cursorSnapped(false)
	, m_stepSize(1)
	, m_sliderLength(0)
{
	if (s_emptyBar == 0) {
		setupBitmaps();
	}
	else if (s_mode != Screen::mode()) {
		delete s_emptyBar;
		delete s_cursorBar;
		setupBitmaps();
	}

	m_decButton = new AButton(this);
	m_incButton = new AButton(this);

	m_decButton->setPixmap(AStyle::loadIcon("general/key_left.png"));
	m_incButton->setPixmap(AStyle::loadIcon("general/key_right.png"));

	connect(m_decButton, SIGNAL(pressed()), this, SLOT(onButtonPressed()));
	connect(m_decButton, SIGNAL(released()), this, SLOT(onButtonReleased()));
	connect(m_incButton, SIGNAL(pressed()), this, SLOT(onButtonPressed()));
	connect(m_incButton, SIGNAL(released()), this, SLOT(onButtonReleased()));

	QFont appFont = qApp->font();
	if ( Screen::mode() == wvga ) {
		appFont.setPointSize(22);
	}
	else {  // tv
		appFont.setPointSize(18);
	}
	setFont(appFont);

	setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));

	connect(&kbd_timer, SIGNAL(timeout()), this, SLOT(updateValue()));
	kbd_timer.setIntervals(START_INTERVAL, FINAL_INTERVAL);
	connect(&mouse_timer, SIGNAL(timeout()), this, SLOT(updateValue()));
	mouse_timer.setIntervals(START_INTERVAL, FINAL_INTERVAL);
}

Slider::~Slider()
{
}

void Slider::setupBitmaps()
{
	s_mode = Screen::mode();

	s_emptyBar = new APixmapHorizontalBar;
	s_emptyBar->loadPixmaps("general/hbar_empty%1.png");

	s_cursorBar = new APixmapHorizontalBar;
	s_cursorBar->loadPixmaps("general/hbar_cursor%1.png");

	s_cursorWidth = s_mode == wvga ? 42 : 60;
}

APixmapHorizontalBar *Slider::s_emptyBar = 0;
APixmapHorizontalBar *Slider::s_cursorBar = 0;
ScreenMode Slider::s_mode = wvga;
int Slider::s_cursorWidth = 0;

void Slider::resizeEvent(QResizeEvent *)
{
	updateLayout();
}

void Slider::showEvent(QShowEvent *)
{
	updateLayout();
}

void Slider::updateLayout()
{
	m_decButton->adjustSize();
	m_incButton->adjustSize();

	QSize buttonSize = m_decButton->sizeHint();
	int buttonY = (height() - buttonSize.height()) / 2;

	m_decButton->move(0, buttonY);

	m_sliderStartPos = QPoint(m_decButton->x() + buttonSize.width() + SPACING, sliderMiddlePos());

	m_incButton->move(width() - m_incButton->sizeHint().width(), buttonY);
	m_sliderLength = (m_incButton->x() - SPACING) - m_sliderStartPos.x();
}

void Slider::paintEvent(QPaintEvent *evt)
{
	QRect area(evt->rect());
	QPixmap buffer(area.size());
	QPainter p(&buffer, this);
	
	p.eraseRect(rect());

	AStyle *aStyle = AStyle::get();
	// draw slider
	aStyle->backgroundBar()->draw(&p, m_sliderStartPos.x() - SPACING, m_decButton->y(),
		m_sliderLength + 2 * SPACING, APixmapHorizontalBar::LookMiddle);	
	s_emptyBar->draw(&p, m_sliderStartPos.x(), m_sliderStartPos.y(), m_sliderLength);

	// draw the slider cursor
	int slider_x_pos = m_sliderStartPos.x() + sliderValueToPixelPos(m_value);
	s_cursorBar->draw(&p, slider_x_pos, sliderMiddlePos(), s_cursorWidth);

	QPainter sp(this);
	sp.drawPixmap(area.topLeft(), buffer);
}

QSize Slider::sizeHint() const
{
	int dw = qt_screen->width();
	return QSize(dw * 2 / 3, m_decButton->sizeHint().height() + 20);
}

void Slider::setValues(int value, int bottom, int top)
{
	assert(value >= bottom && value <= top);
	m_bottom = bottom;
	m_top = top;
	m_value = value;
	m_steps = -1;
}

void Slider::setValues(int value, int bottom, int top, int steps)
{
	assert(value >= bottom && value <= top);
	m_bottom = bottom;
	m_top = top;
	m_value = value;
	m_steps = steps;
}

int Slider::getValue()
{
	return m_value;
}

int Slider::top() const
{
	return m_top;
}

int Slider::bottom() const
{
	return m_bottom;
}

void Slider::incValue()
{
	if (m_value == m_top)
		return;
	
	if (m_steps == -1)
		m_value++;
	else
		m_value += (m_top-m_bottom) / m_steps;

	if (m_value > m_top)
		m_value = m_top;
	
	emit valueChanged(m_value);
	update();
}

void Slider::decValue()
{
	if (m_value == m_bottom)
		return;
	
	if (m_steps == -1)
		m_value--;
	else
		m_value -= (m_top-m_bottom) / m_steps;
	
	if (m_value < m_bottom)
		m_value = m_bottom;
	
	emit valueChanged(m_value);
	update();
}

void Slider::incValueBy10()
{
	if (m_value == m_top)
		return;
	
	if (m_steps == -1)
		m_value += 10;
	else
		m_value += 10*(m_top-m_bottom) / m_steps;

	if (m_value > m_top)
		m_value = m_top;
	
	emit valueChanged(m_value);
	update();
}

void Slider::decValueBy10()
{
	if (m_value == m_bottom)
		return;
	
	if (m_steps == -1)
		m_value -= 10;
	else
		m_value -= 10*(m_top-m_bottom) / m_steps;
	
	if (m_value < m_bottom)
		m_value = m_bottom;
	
	emit valueChanged(m_value);
	update();
}

void Slider::updateValue()
{
	if (m_stepSize == 1) {
		if ( m_direction == left) {
			decValue();
		}
		else {
			incValue();
		}
	}
	else if (m_stepSize == 10) {
		if ( m_direction == left) {
			decValueBy10();
		}
		else {
			incValueBy10();
		}
	}
}

void Slider::keyPressEvent(QKeyEvent *e)
{
	switch(e->key()) {
		case Key_Up:
		case Key_Left:
			m_direction = left;
			m_stepSize = 1;
			kbd_timer.start();
			break;
		case Key_Down:
		case Key_Right:
			m_direction = right;
			m_stepSize = 1;
			kbd_timer.start();
			break;
		case Key_PageUp:
			m_direction = left;
			m_stepSize = 10;
			kbd_timer.start();
			break;
		case Key_PageDown:
			m_direction = right;
			m_stepSize = 10;
			kbd_timer.start();
			break;
		default:
			e->ignore();
			break;
	}
}

void Slider::keyReleaseEvent(QKeyEvent *e)
{
	switch(e->key()) {
		case Key_Up:
		case Key_Left:
			kbd_timer.stop();
			decValue();
			break;
		case Key_Right:
		case Key_Down:
			kbd_timer.stop();
			incValue();
			break;
		case Key_PageUp:
			kbd_timer.stop();
			decValueBy10();
			break;
		case Key_PageDown:
			kbd_timer.stop();
			incValueBy10();
			break;
		default:
			e->ignore();
			break;
	}
}

QRect Slider::sliderRect()
{
	return QRect(m_sliderStartPos, QSize(m_sliderLength, s_emptyBar->height()));
}

void Slider::mousePressEvent(QMouseEvent *e)
{
	const int ex = e->pos().x();
	const int sx = m_sliderStartPos.x(); 
	if ( ex > sx && ex < ( sx + m_sliderLength ) ) {
		m_cursorSnapped = true;
		m_value = sliderPosToValue(e->pos().x());
		emit valueChanged(m_value);
		update();
	}
}

void Slider::mouseMoveEvent(QMouseEvent *e)
{
	if (m_cursorSnapped) {
		int tmp = sliderPosToValue(e->pos().x());
		if ( tmp != m_value ) {
			m_value = tmp;
			emit valueChanged(m_value);
			update();
		}
	}
}

int Slider::sliderValueToPixelPos(int value)
{
	int tmp = value - m_bottom;
	int steps = m_top - m_bottom;
	
	if (value == m_top)
		return m_sliderLength - s_cursorWidth;
	else
		return tmp * (m_sliderLength - s_cursorWidth) / steps;
}

int Slider::sliderPosToValue(int x)
{
	int tmp = x - s_cursorWidth/2 - sliderRect().x();
	int steps = m_top - m_bottom;

	if (m_sliderLength == 0)
		return 0;
	
	int value = (tmp * steps / (m_sliderLength - s_cursorWidth)) + m_bottom;
	
	if (value < m_bottom) {
		value = m_bottom;
	}
	else if (value > m_top) {
		value = m_top;
	}
	return value;
}

int Slider::sliderMiddlePos(void)
{
	return (height() - s_emptyBar->height()) / 2;
}

QPoint Slider::getCenter(void)
{
	return QPoint(m_sliderStartPos.x() + m_sliderLength / 2, sliderMiddlePos());
}

void Slider::mouseReleaseEvent(QMouseEvent *)
{
	m_cursorSnapped = false;
}

void Slider::onButtonPressed()
{
	if (sender() == m_decButton) {
		m_direction = left;
	} else {
		m_direction = right;
	}
	m_valueOnPressed = m_value;
	mouse_timer.start();
}

void Slider::onButtonReleased()
{
	mouse_timer.stop();
	if (m_value == m_valueOnPressed) {
		// Move at least once
		if (m_direction == left) {
			decValue();
		} else {
			incValue();
		}
	}
}
