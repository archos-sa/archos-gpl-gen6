#ifndef SEPARATOR_H
#define SEPARATOR_H

#include <qwidget.h>

class Separator : public QWidget {
	Q_OBJECT
public:
	Separator(QWidget* parent, const char* name = "separator");
	~Separator();

	virtual QSize sizeHint() const;

protected:
	void paintEvent(QPaintEvent* ev);
};

#endif

