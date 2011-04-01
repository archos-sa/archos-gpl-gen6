/****************************************************************************
** Copyright (C) 2006 basysKom GmbH.  All rights reserved.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** Contact: info@basyskom.de
**
**********************************************************************/
#ifndef BATTERYWIDGET_H
#define BATTERYWIDGET_H

#include <qwidget.h>
#include <qpixmap.h>

class QTimer;

class BatteryWidget : public QWidget {
	Q_OBJECT
public:
	BatteryWidget(QWidget* parent, const char* name = "battery_widget");
	~BatteryWidget();

	QSize sizeHint() const;

public slots:
        void slotBatteryStat( int level );
        
protected:
	void paintEvent(QPaintEvent* ev);

private:
	QTimer* m_pUpdateTimer;
        int m_stat;
        bool m_blink;
	QPixmap m_batt_a, 
		m_batt_b, 
		m_batt_0, 
		m_batt_1, 
		m_batt_2, 
		m_batt_3;
};

#endif

