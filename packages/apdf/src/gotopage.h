#ifndef GOTO_PAGE_H
#define GOTO_PAGE_H

#include <qwidget.h>

class QLabel;
class Slider;
class DlgMenu;

class GotoPage : public QWidget {
	Q_OBJECT
  public:
	GotoPage(QWidget *parent = 0, const char *name = 0);
	~GotoPage();
	virtual void paintEvent(QPaintEvent*);
	void setValues(int value, int bottom, int top);
	void setValues(int value, int bottom, int top, int steps);
	int getValue(void);
	void updateMask();

  signals:
	void selectedValue(int v);
	void valueChanged(int v);
	void closed();

  protected:
	virtual void showEvent(QShowEvent *);
	virtual void keyReleaseEvent(QKeyEvent *e);
	virtual void keyPressEvent(QKeyEvent *e);

  private slots:
	void acked(void);
	void changed(int v);

  private:
	Slider *m_slider;
	QLabel *m_sliderLabel;
	void updateSliderLabel();
};

#endif // GOTO_PAGE_H
