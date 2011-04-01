/****************************************************************************
** Copyright (C) 2006 basysKom GmbH.  All rights reserved.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** Contact: info@basyskom.de
**
**********************************************************************/
#ifndef CLOCKWIDGET_H
#define CLOCKWIDGET_H

#include <qwidget.h>
#include <qpixmap.h>

class QTimer;

class ClockWidget : public QWidget {
	Q_OBJECT
public:
	ClockWidget(QWidget* parent, const char* name = "clock_widget");
	~ClockWidget();

public slots:
	// FIXME: using an int for a flag is ugly
	void setTimeFormat12(int n);

protected:
	void paintEvent(QPaintEvent* ev);

private:
	QTimer* m_pUpdateTimer;
	bool m_blink;
	int m_time12;
};

#endif

