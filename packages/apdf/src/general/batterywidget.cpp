/****************************************************************************
** Copyright (C) 2006 basysKom GmbH.  All rights reserved.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** Contact: info@basyskom.de
**
**********************************************************************/
#include "batterywidget.h"
#include "style.h"

#include <archos/extapp_msg.h>
#include <qtimer.h>
#include <qpixmap.h>
#include <qpainter.h>

using namespace archos;

BatteryWidget::BatteryWidget(QWidget* parent, const char* name) : QWidget(parent, name)
{
	m_blink = false;
	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
	m_pUpdateTimer = new QTimer(this);
	connect (m_pUpdateTimer, SIGNAL(timeout()), this, SLOT(update()));
	m_pUpdateTimer->start(1000);
        m_stat = batt_loading;

	m_batt_a = AStyle::loadIcon("general/batt_a.png");
	m_batt_b = AStyle::loadIcon("general/batt_b.png");
	m_batt_0 = AStyle::loadIcon("general/batt_0.png");
	m_batt_1 = AStyle::loadIcon("general/batt_1.png");
	m_batt_2 = AStyle::loadIcon("general/batt_2.png");
	m_batt_3 = AStyle::loadIcon("general/batt_3.png");
}

BatteryWidget::~BatteryWidget()
{
}

void BatteryWidget::paintEvent(QPaintEvent*)
{
	QPainter p(this);
	switch (m_stat) {
	case batt_loading:
		p.drawPixmap(0,0, m_blink ? m_batt_a : m_batt_b);
		break;
	case batt_lvl0:
		p.drawPixmap(0,0, m_batt_0);
		break;
	case batt_lvl1:
		p.drawPixmap(0,0, m_batt_1);
		break;
	case batt_lvl2:
		p.drawPixmap(0,0, m_batt_2);
		break;
	case batt_lvl3:
		p.drawPixmap(0,0, m_batt_3);
		break;
	default:
		qDebug("BatteryWidget got message from AVOS that is interesting because it is not documented: %i", m_stat);
		break;

	}

	m_blink = !m_blink;
}

QSize BatteryWidget::sizeHint() const {
	return m_batt_a.size();
}

void BatteryWidget::slotBatteryStat(int i) {
        m_stat = i;
}
