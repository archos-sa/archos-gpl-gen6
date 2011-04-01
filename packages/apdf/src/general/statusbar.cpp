#include <qpainter.h>
#include <qbitmap.h>
#include <qapplication.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qgfx_qws.h>
#include <qpushbutton.h>
#include <qtimer.h>

#include <archos/screen.h>
#include <archos/alog.h>

#include "style.h"
#include "statusbar.h"
#include "fontchooser.h"
#include "batterywidget.h"
#include "clockwidget.h"
#include "volumewidget.h"
#include "abutton.h"

#include <stdio.h>

using namespace archos;

static const int ANIMATION_INTERVAL = 100;

/**
 * Truncates _text if it is too wide to fit width, according to the font
 * metrics fm
 */
static QString truncateText(const QString &_text, const QFontMetrics &fm, int width)
{
	if (fm.width(_text) <= width) {
		return _text;
	}

	const QString ellipsis = "...";
	width -= fm.width(ellipsis);

	QString text = _text;
	while (fm.width(text) > width && text.length() > 0) {
		text.truncate(text.length() - 1);
	}
	return text + ellipsis;
}


/**
 * A widget which can show two texts, one on the left, the other on the right.
 * If the left one is too wide it gets truncated.
 */
class TitleLabel : public QWidget {
public:
	TitleLabel(QWidget *parent)
	: QWidget(parent)
	{}

	void setTexts(const QString &mainText, const QString &rightText)
	{
		m_mainText = mainText;
		m_rightText = rightText;
		update();
	}

protected:
	virtual void paintEvent(QPaintEvent *event)
	{
		QPainter painter(this);
		const QFontMetrics &fm = painter.fontMetrics();
		const int rightTextWidth = fm.width(m_rightText);

		const QString mainText = truncateText(m_mainText + ' ', fm, width() - rightTextWidth);

		const int posY = (height() - fm.height()) / 2 + fm.ascent();
		painter.drawText(0, posY, mainText);
		painter.drawText(width() - rightTextWidth, posY, m_rightText);
	}

private:
	QString m_mainText, m_rightText;
};


Statusbar::Statusbar(QWidget *parent)
: QObject(parent, "status_bar")
, m_working(false)
{
	QPixmap pix;

	// Home button
	m_homeButton = new AButton(parent);
	m_homeButton->setButtonType(AStyle::ButtonTypeStatusbar);
	m_homeButton->resize(m_homeButton->sizeHint());
	pix = AStyle::loadIcon("general/statusbar_home_icon.png");
	m_homeButton->setPixmap(pix);

	// Title widget
	createTitleWidget(parent);

	// Back button
	m_backButton = new AButton(parent);
	m_backButton->setButtonType(AStyle::ButtonTypeStatusbar);
	m_backButton->resize(m_backButton->sizeHint());
	pix = AStyle::loadIcon("general/statusbar_back_icon.png");
	m_backButton->setPixmap(pix);

	// Info widgets
	createInfoWidgets(parent);

	m_titleWidget->setBackgroundPixmap(*m_infoWidgetsContainer->backgroundPixmap());
	initWidgetBackground(m_spinLabel);
	initWidgetBackground(m_titleLabel);
}

Statusbar::~Statusbar()
{
	// Delete widgets ourself because we are not their parent widget
	// (Statusbar inherits from QObject)
	delete m_homeButton;
	delete m_backButton;
	delete m_titleWidget;
	delete m_infoWidgetsContainer;
}

void Statusbar::createTitleWidget(QWidget *parent)
{
	m_titleWidget = new QWidget(parent);

	// Spin label
	QString tmpl = "general/anim_progress_%1.png";
	for (int x=1; x<=8; ++x) {
		QPixmap pix = AStyle::loadIcon(tmpl.arg(x));
		m_spinningPixmaps << pix;
	}
	m_spinningIt = m_spinningPixmaps.begin();

	m_pdfIcon = AStyle::loadIcon("general/filetype_pdf.png");

	m_spinLabel = new QLabel(m_titleWidget);
	m_spinLabel->setPixmap(m_pdfIcon);
	m_spinLabel->setFixedWidth(m_pdfIcon.width());
	m_spinLabel->setAlignment(Qt::AlignCenter);

	m_spinTimer = new QTimer(m_titleWidget);
	connect(m_spinTimer, SIGNAL(timeout()), this, SLOT(doSpin()));

	// title label
	m_titleLabel = new TitleLabel(m_titleWidget);
	QFont font = qApp->font();
	if ( Screen::mode() == wvga ) {
		font.setPointSize(22);
	} else {  // tv
		font.setPointSize(18);
	}
	m_titleLabel->setFont(font);

	// Layouting
	QHBoxLayout *layout = new QHBoxLayout(m_titleWidget);
	layout->setMargin(2);
	layout->addWidget(m_spinLabel);
	layout->addWidget(m_titleLabel);
}

void Statusbar::createInfoWidgets(QWidget* parent)
{
	m_infoWidgetsContainer = new QWidget(parent);

	QFont font = qApp->font();
	if ( Screen::mode() == wvga ) {
		font.setPointSize(22);
	} else {  // tv
		font.setPointSize(18);
	}
	m_infoWidgetsContainer->setFont(font);

	// Define background
	QPixmap pixmap = AStyle::loadIcon("general/statusbar_backgnd.png");
	QPixmap bg(pixmap.size());
	{
		QPainter painter(&bg);
		painter.fillRect(bg.rect(), AStyle::statusbarBackgroundColor());
		painter.drawPixmap(0, 0, pixmap);
	}
	m_infoWidgetsContainer->setBackgroundPixmap(bg);

	// Volume
	m_volumeWidget = new VolumeWidget(m_infoWidgetsContainer);
	initWidgetBackground(m_volumeWidget);

	// Clock
	m_clockWidget = new ClockWidget(m_infoWidgetsContainer);
	initWidgetBackground(m_clockWidget);

	// Network
	m_networkWidget = new QLabel(m_infoWidgetsContainer);
	m_networkWidget->setAlignment(Qt::AlignCenter);
	initWidgetBackground(m_networkWidget);

	// Battery
	m_batteryWidget = new BatteryWidget(m_infoWidgetsContainer);
	initWidgetBackground(m_batteryWidget);

	// Layout
	// We do not use a QGridLayout because we need to fine-tune each widget
	// position to match AVOS statusbar
	int infoWidth;
	int infoHeight = AStyle::get()->statusbarButtonSize().height();
	QPoint volumeOffset, clockOffset, networkOffset, batteryOffset;
	int deltaY;
	switch (Screen::mode()) {
	case wvga:
		infoWidth = 78;
		break;
	case hdmi_720p:
		infoWidth = 78;
		break;
	case tv:
		infoWidth = 76;
		volumeOffset  = QPoint(-1, 0);
		clockOffset   = QPoint(-4, 1);
		networkOffset = QPoint(-1, -4);
		batteryOffset = QPoint(-1, -4);
		break;
	}
	int cellWidth = infoWidth / 2;
	int cellHeight = infoHeight / 2;
	m_infoWidgetsContainer->setFixedSize(infoWidth, infoHeight);

	m_volumeWidget->setGeometry (0,         0,          cellWidth, cellHeight);
	m_clockWidget->setGeometry  (cellWidth, 0,          cellWidth, cellHeight);
	m_networkWidget->setGeometry(0,         cellHeight, cellWidth, cellHeight);
	m_batteryWidget->setGeometry(cellWidth, cellHeight, cellWidth, cellHeight);

	#define applyOffset(name) m_ ## name ## Widget->move(m_ ## name ## Widget->pos() + name ## Offset)
	applyOffset(volume);
	applyOffset(clock);
	applyOffset(network);
	applyOffset(battery);
	#undef applyOffset

	m_infoWidgetsContainer->resize(m_infoWidgetsContainer->sizeHint());
}

void Statusbar::initWidgetBackground(QWidget *widget)
{
	widget->setBackgroundPixmap(*m_infoWidgetsContainer->backgroundPixmap());
	widget->setBackgroundOrigin(QWidget::ParentOrigin);
}

void Statusbar::updateWidgetsGeometry(int width, int spacing)
{
	m_homeButton->move(0, 0);
	m_homeButton->hide();
	m_infoWidgetsContainer->move(width - m_infoWidgetsContainer->width() - spacing, 0);
	m_backButton->move(m_infoWidgetsContainer->x() - m_backButton->width() - spacing, 0);

	// Place title widget between home and back buttons when statusbar is
	// visible
	m_titleWidgetFullRect.setLeft(m_homeButton->rect().right() + spacing);
	m_titleWidgetFullRect.setTop(0);
	m_titleWidgetFullRect.setWidth(m_backButton->x() - m_titleWidgetFullRect.left() - spacing);
	m_titleWidgetFullRect.setHeight(m_backButton->height());

	m_titleWidgetReducedRect = QRect(0, 0, m_spinLabel->width(), m_backButton->height());

	m_titleWidget->setGeometry( m_titleWidgetFullRect );
}


void Statusbar::setTitle(const QString& title, int currentPage, int lastPage)
{
	QString rightText = QString("%1/%2").arg(currentPage).arg(lastPage);
	m_titleLabel->setTexts(title, rightText);
}

void Statusbar::setWorking(bool working)
{
	m_working = working;
	if (m_working) {
		m_spinTimer->start(ANIMATION_INTERVAL);
		m_titleWidget->show();
		doSpin();
	} else {
		m_spinTimer->stop();
		m_spinLabel->setPixmap(m_pdfIcon);
		if (!isVisible()) {
			m_titleWidget->hide();
		}
	}
}

void Statusbar::show()
{
	m_homeButton->show();
	m_backButton->show();
	m_infoWidgetsContainer->show();
	m_titleWidget->setGeometry( m_titleWidgetFullRect );
	m_titleWidget->show();
	m_titleLabel->show();
}

void Statusbar::hide()
{
	m_homeButton->hide();
	m_backButton->hide();
	m_infoWidgetsContainer->hide();

	m_titleWidget->setGeometry( m_titleWidgetReducedRect );
	m_titleLabel->hide();
	if (m_working) {
		m_titleWidget->show();
	} else {
		m_titleWidget->hide();
	}
}

bool Statusbar::isVisible() const
{
	return m_backButton->isVisible();
}

void Statusbar::doSpin()
{
	++m_spinningIt;
	if (m_spinningIt == m_spinningPixmaps.end()) {
		m_spinningIt = m_spinningPixmaps.begin();
	}
	m_spinLabel->setPixmap(*m_spinningIt);
}

void Statusbar::setNetworkStatus(extapp_msg_network_t networkStatus)
{
	ALOG_WARNING("m_networkStatus=%d", networkStatus);

	QString name;
	switch (networkStatus) {
	case net_none:
		m_networkWidget->setText(QString());
		return;

	case net_wifi_disconnected:
		name = "wifi_offline";
		break;

	case net_wifi_connected:
		name = "wifi_online";
		break;

	case net_ethernet_disconnected:
		name = "ethernet_offline";
		break;

	case net_ethernet_connected:
		name = "ethernet_online";
		break;
	}

	ALOG_WARNING("name=%s", name.ascii());
	name = QString("general/statusbar_%1.png").arg(name);
	ALOG_WARNING("name=%s", name.ascii());
	QPixmap pix = AStyle::loadIcon(name);
	m_networkWidget->setPixmap(pix);
}
