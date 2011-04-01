#ifndef SLIDER_H
#define SLIDER_H

#include <qpixmap.h>
#include <qwidget.h>
#include <qrect.h>
#include <qsize.h>
#include <qtimer.h>
#include <qpoint.h>

#include <archos/screen.h>

class AButton;
class APixmapHorizontalBar;

class SpeedUpTimer : public QTimer {
	Q_OBJECT
  public:
	SpeedUpTimer(QObject *parent = 0);
	void setIntervals(int startInterval, int finalInterval);
	void start();
	void stop();

  private slots:
	void decreaseInterval();

  private:
	int m_startInterval, m_finalInterval, m_currentInterval;
};

class Slider : public QWidget {
	Q_OBJECT
  public:
	Slider(QWidget *parent = 0, const char *name = 0);
	~Slider();
	int getValue();
	void setValues(int value, int bottom, int top);
	void setValues(int value, int bottom, int top, int steps);
	void reset();
	virtual QSize sizeHint() const;
	QPoint getCenter(void);

	int bottom() const;
	int top() const;

  signals:
	void valueChanged(int);

  protected:
	virtual void showEvent(QShowEvent *evt);
	virtual void paintEvent(QPaintEvent *evt);
	virtual void keyPressEvent(QKeyEvent *e);
	virtual void keyReleaseEvent(QKeyEvent *e);
	virtual void mouseMoveEvent(QMouseEvent *e);
	virtual void mousePressEvent(QMouseEvent *e);
	virtual void mouseReleaseEvent(QMouseEvent *e);
	virtual void resizeEvent(QResizeEvent *e);

  private slots:
	void updateValue();
	void onButtonPressed();
	void onButtonReleased();

  private:
	void updateLayout();
	void incValue();
	void decValue();
	void incValueBy10();
	void decValueBy10();
	QRect sliderRect();
	int sliderPosToValue(int x);
	int sliderValueToPixelPos(int value);
	void setupBitmaps();
	int sliderMiddlePos(void);

	int m_value, m_bottom, m_top;
	int m_steps;

	AButton *m_decButton;
	AButton *m_incButton;
	
	static APixmapHorizontalBar *s_emptyBar;
	static APixmapHorizontalBar *s_cursorBar;
	QPoint m_sliderStartPos;
	bool m_cursorSnapped;
	SpeedUpTimer kbd_timer, mouse_timer;
	enum { left = 0, right } m_direction;
	int m_stepSize;
	int m_sliderLength;
	int m_valueOnPressed;

	static archos::ScreenMode s_mode;
	static int s_cursorWidth;
};

#endif // SLIDER_H
