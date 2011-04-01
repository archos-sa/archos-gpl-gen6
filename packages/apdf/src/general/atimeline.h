#ifndef ATIMELINE_H
#define ATIMELINE_H

// Qt
#include <qdatetime.h>
#include <qtimer.h>

/**
 * A timeline object, useful for animations.
 */
class ATimeLine : public QObject {
	Q_OBJECT

	public:
		ATimeLine(QObject *parent = 0);

		void setDuration(int duration);

		void setUpdateInterval(int interval);

		void start();

	signals:
		void valueChanged(double);
		void finished();

	private slots:
		void onTimerUpdated();

	private:
		int m_duration;
		int m_updateInterval;
		QTime m_startTime;
		QTimer *m_timer;
};

#endif /* ATIMELINE_H */
