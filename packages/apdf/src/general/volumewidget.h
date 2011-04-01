#ifndef VOLUMEWIDGET_H
#define VOLUMEWIDGET_H

#include <qpixmap.h>
#include <qwidget.h>

class VolumeWidget : public QWidget
{
	public:
		VolumeWidget(QWidget *parent);

		void setSpeakerActivated(bool);

		void setMuted(bool);

		void setLevel(int);

	protected:
		virtual void paintEvent(QPaintEvent*);

	private:
		bool m_speakerActivated;
		bool m_muted;
		int m_level;
	
		QPixmap m_back;
		QPixmap m_speakerMutedPix;
		QPixmap m_speakerPix;
		QPixmap m_headphonesMutedPix;
		QPixmap m_headphonesPix;
		QPixmap m_levelPix;
};

#endif /* VOLUMEWIDGET_H */
