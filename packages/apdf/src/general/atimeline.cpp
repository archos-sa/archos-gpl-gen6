#include "atimeline.h"

ATimeLine::ATimeLine(QObject *parent)
: QObject(parent)
, m_duration(2000)
, m_updateInterval(50)
, m_timer(new QTimer(this))
{
	connect(m_timer, SIGNAL(timeout()),
		this, SLOT(onTimerUpdated()) );
}


void ATimeLine::setDuration(int duration)
{
	m_duration =duration;
}


void ATimeLine::setUpdateInterval(int interval)
{
	m_updateInterval = interval;
}


void ATimeLine::start()
{
	m_startTime.start();
	m_timer->start(m_updateInterval);
	emit valueChanged(0);
}


void ATimeLine::onTimerUpdated()
{
	double value = m_startTime.elapsed() / double(m_duration);

	if (value > 1.) {
		value = 1.;
		m_timer->stop();
	}

	emit valueChanged(value);

	if (!m_timer->isActive()) {
		emit finished();
	}
}
