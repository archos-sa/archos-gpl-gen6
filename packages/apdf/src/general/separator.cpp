#include "separator.h"

#include <qpainter.h>


Separator::Separator(QWidget* parent, const char* name) : QWidget(parent, name)
{
	setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
}

Separator::~Separator()
{
}

QSize Separator::sizeHint() const
{
	return QSize(32, 3);
}

void Separator::paintEvent(QPaintEvent*)
{
	QPainter p(this);
	p.setPen(Qt::white);
	p.drawLine(0, 1, width(), 1);
}
