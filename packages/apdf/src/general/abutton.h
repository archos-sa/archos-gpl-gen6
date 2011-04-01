#ifndef ABUTTON_H
#define ABUTTON_H

#include "style.h"

#include <qpushbutton.h>

class QPainter;

class AButton : public QPushButton
{
public:
	AButton(QWidget *parent=0, const char * name=0 );

	void setButtonType(AStyle::ButtonType);

	virtual QSize sizeHint() const;

protected:
	virtual void drawButton(QPainter *);
	virtual void drawButtonLabel(QPainter *);

private:
	AStyle::ButtonType m_buttonType;
};


#endif /* ABUTTON_H */
