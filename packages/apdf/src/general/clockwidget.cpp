/****************************************************************************
** Copyright (C) 2006 basysKom GmbH.  All rights reserved.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** Contact: info@basyskom.de
**
**********************************************************************/
#include "clockwidget.h"

#include <qpainter.h>
#include <qimage.h>
#include <qapplication.h>
#include <qtimer.h>
#include <qdatetime.h>
#include <qfontmetrics.h>

#include <archos/screen.h>
using namespace archos;

ClockWidget::ClockWidget(QWidget* parent, const char* name) : QWidget(parent, name)
{
	m_time12 = 0;
	m_pUpdateTimer = new QTimer(this);
	connect(m_pUpdateTimer, SIGNAL(timeout()), this, SLOT(update()));
	m_pUpdateTimer->start(1000);
	m_blink = false;
	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

	int fontSize;
	if ( Screen::mode() == wvga ) {
		fontSize = 16;
	}
	else {  // tv
		fontSize = 12;
	}

	setFont(QFont("archos", fontSize));
}

ClockWidget::~ClockWidget()
{
}

void ClockWidget::setTimeFormat12(int n)
{
	m_time12 = n;
}

void ClockWidget::paintEvent(QPaintEvent*)
{	
	QTime tim = QTime::currentTime();
	QPainter p(this);
	p.setPen(palette().active().text());

	int hour = 0;
	if ( m_time12 ) {
		hour = tim.hour() % 12;
		if ( hour == 0) {
			hour = 12;
		}
	} else {
		hour = tim.hour();
	}
	QString text;
	text.sprintf("%02d %02d", hour, tim.minute());

	QRect textRect = rect();
	p.drawText(textRect, Qt::AlignCenter, text);

	if (m_blink) {
		p.drawText(textRect, Qt::AlignCenter, ":");
	}
	m_blink = !m_blink;
}
