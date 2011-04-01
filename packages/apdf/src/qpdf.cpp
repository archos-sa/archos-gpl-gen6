#include <qwindowsystem_qws.h>
#include <qgfx_qws.h>

#include <qapplication.h>
#include <qwidgetstack.h>
#include <qtimer.h>
#include <qfileinfo.h>
#include <qstring.h>
#include <qthread.h>
#include <qlayout.h>
#include <qlabel.h>
#include <stdlib.h>
#include <qtextcodec.h>
#include <poppler/poppler-qt.h>

#include <linux/fb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>

#include <archos/screen.h>
#include <archos/atr.h>
#include <archos/astring.h>
#include <archos/alog.h>
#include <archos/dmalloc.h>

#include "qpdf.h"
#include "gotopage.h"
#include "general/style.h"
#include "general/fontchooser.h"
#include "general/statusbar.h"
#include "general/batterywidget.h"
#include "general/clockwidget.h"
#include "general/volumewidget.h"
#include "general/abutton.h"
#include "general/amenu.h"

#define MENU_HIDE_TIME 5000

#define MENU_RIGHT_MARGIN 6
#define MENU_TOP_MARGIN 3

using namespace Poppler;
using namespace archos;

static const int STATUSBAR_MARGIN = 4;
static const int MENU_BUTTON_RIGHT_MARGIN = 1;

static const char *tr_path = "/tmp/pdf.trex";

enum {
	ArchosRemote_PageUp   = 4103,
	ArchosRemote_PageDown = 4102,
	ArchosRemote_Home     = 4112,
};

static void initAppSettings()
{
	QPalette palette = qApp->palette();
	palette.setColor(QColorGroup::Text, Qt::white);
	palette.setColor(QColorGroup::Button, QColor(38, 38, 38));
	palette.setColor(QColorGroup::ButtonText, Qt::white);
	palette.setColor(QColorGroup::Background, QColor(38, 38, 38));
	palette.setColor(QColorGroup::Foreground, Qt::white);
	qApp->setPalette(palette);
	qApp->setFont(FontChooser::getFont());
}

int main ( int argc, char **argv )	
{
	QWSServer::setDesktopBackground(QColor(123, 123, 123));  // gray
	QApplication app(argc, argv);

	if (atr_load(tr_path) ) {
		ALOG_WARNING("Failed to load translations from '%s'\n", tr_path);
	}
	initAppSettings();

	QStringList args;
	for (int i = 1; i < argc; i++) {
		args.append(QString::fromUtf8(argv[i]));
	}

	QStringList file_arg = args.grep("-file=", 1);
	if (file_arg.count() != 1) {
		ALOG_ERROR("Missing -file option\n");
		return -1;
	}
	QString file(file_arg[0].remove(0, 6));

	int time12 = 0;
	if (args.contains("-time=12h") ) {
		time12 = 1;
	}

	if (DMALLOC_init() < 0) {
		ALOG_ERROR("DMALLOC_init failed\n");
		return -1;
	}
	
	QPdfDlg *dlg = new QPdfDlg(time12);

	app.setMainWidget(dlg);
	dlg->showMaximized();
	dlg->toggleVideo();  // adjust fb4 to the current setting of fb0

	dlg->openFile(file);
	int ret = app.exec();

	DMALLOC_exit();
	ALOG_DEBUG("Ending");
	
	return ret;

}

QPdfDlg::QPdfDlg(int time12)
		: QWidget(NULL, "mainwidget", QWidget::WStyle_Customize | QWidget::WStyle_NoBorder)
	, m_docLoaded(false)
	, m_pages(0)
	, m_doc(0)
	, m_menuButton(0)
	, m_menu(0)
	, m_time12Flag(time12)
	, m_videoSwitching(false)
{
	m_avossocket = archos::AvosSocket::getInstance();
	if ( m_avossocket->start() ) {
		ALOG_ERROR("Failed to setup the avossocket");
		qApp->quit();
	}
	connect(m_avossocket, SIGNAL(packetReady(extapp_msg_t*)), this, SLOT(handleMsg(extapp_msg_t*)));

	QTimer::singleShot(2000, this, SLOT(sendStartupReadyMessage()) );

	m_screen_width = qt_screen->width();
	m_screen_height = qt_screen->height();

	QLayout *l = new QVBoxLayout(this, 0, 0);
	m_pdfWidget = new PDFWidget(0, this, "pdfWidget", MAX_ZOOM_PERCENT_VALUE);
	l->add(m_pdfWidget);

	createGui();

	createAutoHideTimer();

	connect(m_pdfWidget, SIGNAL(rendererRunning(bool)), this, SLOT(setWorking(bool)));
	connect(m_pdfWidget, SIGNAL(scaling(bool)), this, SLOT(setWorking(bool)));
	connect(m_pdfWidget, SIGNAL(currentPageChanged()), this, SLOT(onCurrentPageChanged()));
	connect(m_pdfWidget, SIGNAL(zoomChanged()), this, SLOT(zoomChanged()));
	connect(m_pdfWidget, SIGNAL(pagesInHistory(bool)), this, SLOT(pagesInHistory(bool)));
}


QPdfDlg::~QPdfDlg()
{
	delete m_doc;
}

void QPdfDlg::reloadLanguage(char* lang)
{
	if ( lang && strlen(lang) > 2 ) {
		setenv("LANG", lang, 1);
		setenv("LC_ALL", lang, 1);
		atr_unload();
		if (atr_load(tr_path) ) {
			ALOG_WARNING("Failed to load translations!");
		}
		qApp->setFont(FontChooser::getFont(lang));
		restartGui();
	}
}


void QPdfDlg::sendSimpleMessageToAvos(int request)
{
	if (m_avossocket) {
		extapp_msg_t msg = { { EXTAPP_MAGIC, request, 0 }, NULL };
		m_avossocket->sendMsg( &msg );
	} else {
		ALOG_WARNING("Can't send message (%d) to Avos, no Avos socket", request);
	}
}


void QPdfDlg::sendExitToHomeMessage()
{
	ALOG_WARNING("");
	sendSimpleMessageToAvos(EXTAPP_PACKET_EXIT_TO_HOME);
}


void QPdfDlg::updateVolumeWidget(extapp_msg_sound_t* soundMessage)
{
	VolumeWidget* widget = m_statusbar->volumeWidget();
	widget->setSpeakerActivated(soundMessage->speaker_activated != 0);
	widget->setMuted(soundMessage->mute != 0);
	widget->setLevel(soundMessage->volume);
}


void QPdfDlg::handleMsg(extapp_msg_t* msg)
{
	extapp_msg_timeformat_t t;
	ALOG_DEBUG("type=%d", msg->header.packet_type);

	switch ( msg->header.packet_type ) {
		case EXTAPP_PACKET_LANG_CHANGED:
			reloadLanguage((char*)msg->data);
			break;
		case EXTAPP_PACKET_BATTERY_STATUS:
			emit signalBatteryLevel( *((int*) msg->data) );
			break;
		case EXTAPP_PACKET_TIMEFORMAT_CHANGED:
			t = *(extapp_msg_timeformat_t*)msg->data;
			emit updateTime12(t == time24 ? 0 : 1 );
			break;
		case EXTAPP_PACKET_TERMINATE:
			ALOG_WARNING("Shutting down...");
			qApp->quit();
			break;
		case EXTAPP_PACKET_SOUND_STATUS:
			updateVolumeWidget(static_cast<extapp_msg_sound_t*>(msg->data));
			break;
		case EXTAPP_PACKET_NETWORK_STATUS:
			m_statusbar->setNetworkStatus(* static_cast<extapp_msg_network_t*>(msg->data));
			break;
		default:
			ALOG_WARNING("Unknown message (%x).", msg->header.packet_type);
			break;
	}
	m_avossocket->freeMsg(msg);
}

void QPdfDlg::gotoPageProxy(int page_value)
{
	m_pdfWidget->jumpToPage(page_value - 1);
}

void QPdfDlg::setZoomLevelProxy(int level)
{
	m_pdfWidget->setZoomLevel(level);
}

bool QPdfDlg::eventFilter(QObject *target, QEvent *event)
{
	if (event->type() == QEvent::MouseButtonPress && target == m_pdfWidget->viewport()) {
		m_gotoPageDlg->hide();
		m_zoomDlg->hide();
		m_menu->hide();

	} else if ((event->type() == QEvent::KeyRelease ||
		( event->type() == QEvent::KeyPress && ((QKeyEvent*)event)->isAutoRepeat())) &&
		((QKeyEvent*)event)->key() == Key_F6)
	{
		toggleVideo();
		return true;
	}
	return QWidget::eventFilter(target, event);
}

void QPdfDlg::sendStartupReadyMessage(void)
{
#ifndef SIM
	sendSimpleMessageToAvos(EXTAPP_PACKET_STARTUP_READY);
#endif
	sendSimpleMessageToAvos(EXTAPP_PACKET_NO_POWEROFF);
}

bool QPdfDlg::event(QEvent* e)
{
	static int last_key = -1;
	static int press_rep_factor = 1;

	bool handled = false;
	if (m_pdfWidget && m_pdfWidget->isVisible()) {
		if (e->type() == QEvent::KeyRelease ||
				(e->type() == QEvent::KeyPress && ((QKeyEvent*)e)->isAutoRepeat()) ) {
			QKeyEvent* keyEvent = (QKeyEvent*)e;
			// till the docLoader hasn't finished ESC is the only input
			if ( !m_docLoaded || m_videoSwitching ) {
				if (keyEvent->key() == Key_F2) {
					close();
					handled = true;
				}
			}
			else {
				// if the user keeps one of the direction keys pressed
				// increase the scrolling speed.
				int rep_factor = 1;
				if ( e->type() == QEvent::KeyPress ) {
					if (  last_key == -1 ) {
						last_key = keyEvent->key();
					}
					else {
						if ( last_key == keyEvent->key() ) {
							press_rep_factor = press_rep_factor + 1 <= 3 ? press_rep_factor + 1 : 3;
						}
						else {
							last_key = -1;
							press_rep_factor = 1;
						}
					}
					rep_factor = press_rep_factor;
				}
				else {
					last_key = -1;
					press_rep_factor = 1;
				}

				switch (keyEvent->key()) {
					case Key_Up:
						m_pdfWidget->scrollUp(rep_factor);
						handled = true;
						break;
					case Key_Down:
						m_pdfWidget->scrollDown(rep_factor);
						handled = true;
						break;
					case Key_Left:
						m_pdfWidget->scrollLeft(rep_factor);
						handled = true;
						break;
					case Key_Right:
						m_pdfWidget->scrollRight(rep_factor);
						handled = true;
						break;
					case Key_Return:
						m_pdfWidget->followLink();
						handled = true;
						break;
					case Key_PageUp:
					case ArchosRemote_PageUp:
						m_pdfWidget->prevPage();
						//m_pdfWidget->zoomIn();
						handled = true;
						break;
					case Key_PageDown:
					case ArchosRemote_PageDown:
						m_pdfWidget->nextPage();
						//m_pdfWidget->zoomOut();
						handled = true;
						break;
					case Key_F1:	// having an autorepeat on the menu key is cumbersome.
						if (!keyEvent->isAutoRepeat()) {
							onMenuButtonClicked();
							handled = true;
						}
						break;
					case Key_F2:
						if (m_pdfWidget->zoomIsDefault()) {
							close();
						}
						else {
							m_pdfWidget->setFitToWidth();
						}
						handled = true;
						break;

					case ArchosRemote_Home:
						sendExitToHomeMessage();
						handled = true;
						break;

					default:
						ALOG_WARNING("Unknown key %x", keyEvent->key());
						break;
				}
			}
		}
	}
	if (!handled)
		return QWidget::event(e);
	return true;
}

void QPdfDlg::toggleVideo(void)
{
#ifndef SIM
	static int old_w = 0, old_h = 0;

	int fd = ::open("/dev/fb0", O_RDWR);
	if (fd < 0) {
		ALOG_WARNING("Can't open framebuffer device /dev/fb0");
		return;
	}

	fb_var_screeninfo vinfo;
	memset( &vinfo, 0, sizeof(fb_var_screeninfo) );

	if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo)) {
		ALOG_ERROR("Error reading variable information in card init");
		::close(fd);
		qApp->quit();
		return;
	}
	::close(fd);

	int w = vinfo.xres;
	int h = vinfo.yres;
	int d = vinfo.bits_per_pixel;

	if (w == old_w && h == old_h) {
		return;
	}
	old_w = w;
	old_h = h;

	qt_screen->setMode(w, h, d);
	QWSServer::beginDisplayReconfigure();
	QWSServer::endDisplayReconfigure();

	if (m_screen_width != w || m_screen_height != h) {
		m_videoSwitching = true;
		m_screen_width = qt_screen->width();
		m_screen_height = qt_screen->height();
		hideGui();
		QTimer::singleShot(500, this, SLOT(restartGui()));
	}
#else
	m_videoSwitching = true;
	m_screen_width = qt_screen->width();
	m_screen_height = qt_screen->height();
	hideGui();
	QTimer::singleShot(500, this, SLOT(restartGui()));
#endif

}

void QPdfDlg::setWorking(bool working)
{
	m_statusbar->setWorking(working);
}


class DocumentLoader : public QThread 
{
	public :
		DocumentLoader(QString docPath, QObject *master) : QThread()
			, m_master(master)
			, m_docPath(docPath)
		{}

		virtual void run()
		{
			ALOG_DEBUG("DocumentLoader: Trying to load %s", m_docPath.ascii());
			Poppler::Document *doc = Poppler::Document::load(m_docPath);
			if (doc) {
				QApplication::postEvent(m_master, new QCustomEvent((QEvent::Type)DocLoaded, doc));
			} else {
				QApplication::postEvent(m_master, new QCustomEvent((QEvent::Type)DocLoaded, 0));
			}
		}

	private:
		QObject *m_master;
		QString m_docPath;
};

void QPdfDlg::customEvent(QCustomEvent *e)
{
    if ((int)e->type() == DocLoaded) {
        docLoaded((Poppler::Document*)e->data());
    } else {
        QWidget::customEvent(e);
    }
}

void QPdfDlg::docLoaded(Poppler::Document* d)
{
	if (d) {
		m_doc = d;
		m_pages = m_doc->getNumPages();
		m_menu->setEntryVisible(prevpage, false);
		m_menu->setEntryVisible(nextpage, m_pages > 1);
		m_menu->setEntryVisible(gotopage, m_pages > 1);
		m_pdfWidget->setDocument(m_doc, ZoomModeFitToWidth);
		m_docLoaded = true;
	} else {
		ALOG_ERROR("Failed to load document");
		setWorking(false);
		exit(1);
	}
}

void QPdfDlg::openFile(const QString &f)
{
	QFileInfo fi(f);
	m_docTitle = fi.baseName();
	
	if (fi.exists()) {
		delete m_doc;
		m_doc = 0;
		m_loader = new DocumentLoader(f, this);
		setWorking(true);
		m_loader->start();
	}
	else {
		ALOG_ERROR("Couldn't open file '%s'!", f.ascii());
		exit(1);
	}
}

void QPdfDlg::onCurrentPageChanged()
{
	int currentPage = m_pdfWidget->getPageNumber();
	int lastPage = m_pdfWidget->getNumPages();
	m_menu->setEntryVisible(prevpage, currentPage > 1);
	m_menu->setEntryVisible(nextpage, currentPage < lastPage);

	updateStatusbarTitle();
}

void QPdfDlg::updateStatusbarTitle()
{
	int currentPage = m_pdfWidget->getPageNumber();
	int lastPage = m_pdfWidget->getNumPages();
	m_statusbar->setTitle(m_docTitle, currentPage, lastPage);
}

void QPdfDlg::pagesInHistory(bool page)
{
	m_menu->setEntryVisible(linkback, page);
}

// private slot
void QPdfDlg::zoomChanged()
{
	ASSERT(m_menu);
	ZoomMode mode = m_pdfWidget->getZoomMode();
	m_menu->setEntryEnabled(zoomfth,  mode != ZoomModeFitToPage);
	m_menu->setEntryEnabled(zoomftw,  mode != ZoomModeFitToWidth);
	m_menu->setEntryEnabled(zoomftwe, mode != ZoomModeFitToWidthEnlarged);
}

void QPdfDlg::createMenuButton()
{
	m_menuButton = new AButton(this);
	m_menuButton->setButtonType(AStyle::ButtonTypeStatusbar);
	m_menuButton->resize(m_menuButton->sizeHint());
	m_menuButton->move(qt_screen->width() - m_menuButton->width() - MENU_BUTTON_RIGHT_MARGIN, 0);
	m_menuButton->show();

	QPixmap pixmap = AStyle::loadIcon("general/statusbar_menu_icon.png");
	m_menuButton->setPixmap(pixmap);

	connect(m_menuButton, SIGNAL(clicked()),
		this, SLOT(onMenuButtonClicked()) );
}

void QPdfDlg::createStatusbar(void)
{
	// Status bar
	m_statusbar = new Statusbar(this);
	m_statusbar->clockWidget()->setTimeFormat12(m_time12Flag);

	connect(m_statusbar->backButton(), SIGNAL(clicked()),
		qApp, SLOT(quit()));

	connect(m_statusbar->homeButton(), SIGNAL(clicked()),
		this, SLOT(sendExitToHomeMessage()));

	connect(this, SIGNAL(updateTime12(int)),
		m_statusbar->clockWidget(), SLOT(setTimeFormat12(int)));

	connect(this, SIGNAL(signalBatteryLevel(int)),
		m_statusbar->batteryWidget(), SLOT(slotBatteryStat(int)));

	// Reposition status bar widgets
	ASSERT(m_menuButton);
	m_statusbar->updateWidgetsGeometry(m_menuButton->x() - STATUSBAR_MARGIN, STATUSBAR_MARGIN);
	m_statusbar->show();

	// Dialogs
	connect(m_gotoPageDlg, SIGNAL(closed()), m_statusbar, SLOT(show()));
	connect(m_zoomDlg, SIGNAL(closed()), m_statusbar, SLOT(show()));

	updateStatusbarTitle();

	sendSimpleMessageToAvos(EXTAPP_PACKET_REQUEST_SOUND_STATUS);
	sendSimpleMessageToAvos(EXTAPP_PACKET_REQUEST_NETWORK_STATUS);
	sendSimpleMessageToAvos(EXTAPP_PACKET_REQUEST_BATTERY_STATUS);
}

void QPdfDlg::createAutoHideTimer()
{
	m_autoHideTimer = new QTimer(this);
	connect(m_autoHideTimer, SIGNAL(timeout()),
		this, SLOT(autoHideGui()) );
	startAutoHideTimer();
}

void QPdfDlg::startAutoHideTimer()
{
	m_autoHideTimer->start(MENU_HIDE_TIME, true /* singleshot */);
}

void QPdfDlg::autoHideGui()
{
	if (m_menu->isVisible() || m_gotoPageDlg->isVisible() || m_zoomDlg->isVisible()) {
		// Do not autohide if menu is visible
		startAutoHideTimer();
	} else {
		hideGui();
	}
}

void QPdfDlg::onMenuButtonClicked()
{
	if (!m_statusbar->isVisible()) {
		m_statusbar->show();
		startAutoHideTimer();
	}

	if ( m_menu->isVisible() ) {
		m_menu->hide();
	}
	else {
		m_menu->slideIn();
	}
}

void QPdfDlg::createMenu()
{
	m_menu = new AMenu(this);

	setupMenuItems();

	ASSERT(m_menuButton);
	QPoint pos(
		qt_screen->width() - m_menu->width() - MENU_RIGHT_MARGIN,
		m_menuButton->height() + MENU_TOP_MARGIN);
	m_menu->setFinalPosition(pos);

	connect(m_menu, SIGNAL(entryActivated(int)), this, SLOT(onMenuEntryActivated(int)));
}

void QPdfDlg::setupMenuItems()
{
	m_menu->addEntry(hide, AString::import(atr("pdf_hide_overlay")));
	m_menu->setEntryIcon(hide, "general/label_hide_osd.png");

	ASubMenu* subMenu = m_menu->addSubMenu(zoommenu, AString::import(atr("pdf_zoom")));
	subMenu->setIcon("general/label_zoom.png");

	subMenu->addEntry(zoomfth, AString::import(atr("pdf_zoom_fth")));
	subMenu->addEntry(zoomftw, AString::import(atr("pdf_zoom_ftw")));
	subMenu->addEntry(zoomftwe, AString::import(atr("pdf_zoom_ftwe")));
	subMenu->addEntry(zoomset, AString::import(atr("pdf_zoom_set")));

	bool enabled = m_pdfWidget->getPageNumber() > 1;
	m_menu->addEntry(prevpage, AString::import(atr("pdf_prev_page")));
	m_menu->setEntryIcon(prevpage, "general/label_previous_page.png");
	m_menu->setEntryVisible(prevpage, enabled);

	enabled = m_pdfWidget->getPageNumber() < m_pdfWidget->getNumPages();
	m_menu->addEntry(nextpage, AString::import(atr("pdf_next_page")));
	m_menu->setEntryIcon(nextpage, "general/label_next_page.png");
	m_menu->setEntryVisible(nextpage, enabled);

	enabled = m_pdfWidget->arePagesInHistory();
	m_menu->addEntry(linkback, AString::import(atr("pdf_link_back")));
	m_menu->setEntryVisible(linkback, enabled);

	enabled = m_pdfWidget->getNumPages() > 1;
	m_menu->addEntry(gotopage, AString::import(atr("pdf_goto_page")));
	m_menu->setEntryIcon(gotopage, "general/label_goto_page.png");
	m_menu->setEntryVisible(gotopage, enabled);
}

void QPdfDlg::createGotoPageDlg()
{
	m_gotoPageDlg = new GotoPage(this, "gotoPageDlg");
	connect(m_gotoPageDlg, SIGNAL(selectedValue(int)), this, SLOT(gotoPageProxy(int)));
	// Prevent the dialog from appearing when the application is started.
	// Note: this does not happen on sim, only on target.
	m_gotoPageDlg->hide();
}

void QPdfDlg::createZoomDlg()
{
	m_zoomDlg = new GotoPage(this, "createZoomDlg");
	connect(m_zoomDlg, SIGNAL(selectedValue(int)), this, SLOT(setZoomLevelProxy(int)));
	// Prevent the dialog from appearing when the application is started.
	// Note: this does not happen on sim, only on target.
	m_zoomDlg->hide();
}

void QPdfDlg::hideGui(void)
{
	if ( m_gotoPageDlg ) {
		m_gotoPageDlg->hide();
	}

	if ( m_zoomDlg ) {
		m_zoomDlg->hide();
	}

	if ( m_menu ) {
		m_menu->hide();
	}

	m_statusbar->hide();
}

void QPdfDlg::restartGui(void)
{
	qApp->removeEventFilter(this);

	delete m_gotoPageDlg;
	delete m_zoomDlg;
	delete m_menu;
	delete m_menuButton;
	delete m_statusbar;

	createGui();

	if (m_doc) {
		setWorking(m_pdfWidget->rendererRunning());
	} else {
		setWorking(true);
	}

	int dw = qt_screen->width();
	int dh = qt_screen->height();
	m_pdfWidget->resize(dw, dh);

	m_videoSwitching = false;
}

void QPdfDlg::createGui()
{
	QSize strut = QApplication::globalStrut();
	// Values come from avos/Include/layout.h
	switch (Screen::mode()) {
		case tv:
			strut = QSize(64, 34);
			break;
		case wvga:
			strut = QSize(80, 42);
			break;
		case hdmi_720p:
			strut = QSize(120, 60);
			break;
	}

	QApplication::setGlobalStrut(strut);

	createGotoPageDlg();

	createZoomDlg();

	createMenuButton();

	createMenu();

	createStatusbar();

	qApp->installEventFilter(this);
}

void QPdfDlg::onMenuEntryActivated(int id) 
{
	switch(id) {
		case zoomfth :
			m_pdfWidget->setFitToPage();
			break;
		case zoomftw :
			m_pdfWidget->setFitToWidth();
			break;
		case zoomftwe :
			m_pdfWidget->setFitToWidthEnlarged();
			break;
		case zoomset :
			m_menu->hide();
			m_zoomDlg->setValues(m_pdfWidget->getZoomLevel(), m_pdfWidget->getZoomLevelMin(), m_pdfWidget->getZoomLevelMax(), 10);
			m_zoomDlg->show();
			break;
		case prevpage :
			m_pdfWidget->prevPage();
			break;
		case nextpage :
			m_pdfWidget->nextPage();
			break;
		case linkback :
			m_pdfWidget->moveBack();
			break;
		case gotopage :
			m_menu->hide();
			// there is no page 0
			m_gotoPageDlg->setValues(m_pdfWidget->getPageNumber(), 1, m_pdfWidget->getNumPages());
			m_gotoPageDlg->show();
			break;
		case hide:
			hideGui();
			break;
	}
}

