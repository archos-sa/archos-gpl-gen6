#include "volumewidget.h"

#include <qpainter.h>

#include "style.h"

static const int MAX_LEVEL = 99;

VolumeWidget::VolumeWidget(QWidget *parent)
: QWidget(parent)
, m_speakerActivated(true)
, m_muted(false)
, m_level(50)
{
	m_back = AStyle::loadIcon("general/volume_backgnd.png");
	m_speakerMutedPix = AStyle::loadIcon("general/volume_speaker_muted.png");
	m_speakerPix = AStyle::loadIcon("general/volume_speaker.png");
	m_headphonesMutedPix = AStyle::loadIcon("general/volume_headphones_muted.png");
	m_headphonesPix = AStyle::loadIcon("general/volume_headphones.png");
	m_levelPix = AStyle::loadIcon("general/volume_level.png");

	setFixedSize(m_back.size());
}

void VolumeWidget::setSpeakerActivated(bool activated)
{
	m_speakerActivated = activated;
	update();
}

void VolumeWidget::setMuted(bool muted)
{
	m_muted = muted;
	update();
}

void VolumeWidget::setLevel(int volume)
{
	m_level = volume;
	update();
}

void VolumeWidget::paintEvent(QPaintEvent*)
{
	QPainter painter(this);
	painter.drawPixmap(0, 0, m_back);
	if (!m_muted) {
		int width = m_levelPix.width() * m_level / MAX_LEVEL;
		painter.drawPixmap(0, 0, m_levelPix, 0, 0, width, m_levelPix.height());
	}

	QPixmap symbolPix;
	if (m_muted) {
		symbolPix = m_speakerActivated ? m_speakerMutedPix : m_headphonesMutedPix;
	} else {
		symbolPix = m_speakerActivated ? m_speakerPix : m_headphonesPix;
	}
	painter.drawPixmap(0, 0, symbolPix);
}
