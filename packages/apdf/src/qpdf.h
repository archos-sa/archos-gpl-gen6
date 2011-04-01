#ifndef __QPDF_H__
#define __QPDF_H__

#include <qstring.h>
#include <poppler/poppler-qt.h>
#include <archos/extapp_msg.h>
#include <archos/avossocket.h>
#include <archos/screen.h>

#include "pdfwidget.h"

class AButton;
class AMenu;
class Statusbar;
class GotoPage;
class QSocket;
class AvosSocket;
class DocumentLoader;
class CloseButton;
class HotSpot;

enum { DocLoaded = 1200, DocFailed };

class QPdfDlg : public QWidget {
	Q_OBJECT

public:
	QPdfDlg(int time12=0);
	virtual ~QPdfDlg();
	void toggleVideo(void);

signals:
	void signalBatteryLevel(int);
	void signalWifiLevel(int);
	void updateTime12(int);

public slots:
	void openFile(const QString &);

private slots:
	void setWorking(bool working);
	void onCurrentPageChanged();
	void zoomChanged();
	void onMenuEntryActivated(int id);
	void pagesInHistory(bool page);
	void sendStartupReadyMessage();
	void handleMsg(extapp_msg_t* msg);
	void gotoPageProxy(int);
	void setZoomLevelProxy(int level);
	void restartGui(void);
	void autoHideGui();
	void onMenuButtonClicked();
	void sendExitToHomeMessage();

protected:
	bool event(QEvent* e);
	void customEvent(QCustomEvent *e);
	bool eventFilter(QObject *target, QEvent *event);

private:
	void createGui();
	void sendSimpleMessageToAvos(int request);

	void createAutoHideTimer();
	void createMenuButton();
	void createMenu(void);
	void createGotoPageDlg();
	void createZoomDlg();
	void createStatusbar(void);

	void docLoaded(Poppler::Document* d);
	void reloadLanguage(char* lang);

	void hideGui(void);

	void startAutoHideTimer();

	void updateVolumeWidget(extapp_msg_sound_t* soundMessage);
	void updateStatusbarTitle();

	bool m_docLoaded;
	int m_pages;
	Poppler::Document *m_doc;
	QString m_docTitle;

	PDFWidget* m_pdfWidget;
	GotoPage *m_gotoPageDlg;
	GotoPage *m_zoomDlg;  // FIXME: should be renamed to sliderdlg or something like this
	Statusbar *m_statusbar;
	DocumentLoader *m_loader;
	AButton *m_menuButton;
	AMenu *m_menu;
	QTimer *m_autoHideTimer;

	enum menuids { hide = 0, zoommenu, zoomfth, zoomftw, zoomftwe, zoomset, zoom, prevpage, nextpage, linkback, gotopage };

	int m_screen_width, m_screen_height;
	void setupMenuItems();

	archos::AvosSocket *m_avossocket;

	int m_time12Flag;

	bool m_videoSwitching;
};

#endif
