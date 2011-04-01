#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <qpixmap.h>
#include <qwidget.h>

#include <archos/extapp_msg.h>

class QLabel;
class QTimer;

class AButton;
class BatteryWidget;
class ClockWidget;
class VolumeWidget;
class TitleLabel;

/**
 * The statusbar.
 * Does not inherit from QWidget because inheriting from QWidget would force us
 * to generate complicated masks to be able to "see through" the bar.
 */
class Statusbar : public QObject {
	Q_OBJECT

	public:
		Statusbar(QWidget *parent);
		~Statusbar();

		AButton* homeButton() const {
			return m_homeButton;
		}

		AButton* backButton() const {
			return m_backButton;
		}

		QWidget* infoWidgetsContainer() const {
			return m_infoWidgetsContainer;
		}

		VolumeWidget* volumeWidget() const {
			return m_volumeWidget;
		}

		ClockWidget* clockWidget() const {
			return m_clockWidget;
		}

		BatteryWidget* batteryWidget() const {
			return m_batteryWidget;
		}

		QWidget* titleWidget() const {
			return m_titleWidget;
		}

		void setNetworkStatus(extapp_msg_network_t networkStatus);

		void setTitle(const QString& title, int currentPage, int lastPage);

		bool isVisible() const;

		void updateWidgetsGeometry(int width, int spacing);
	
	public slots:
		void setWorking(bool working);
		void show();
		void hide();

	private slots:
		void doSpin();

	private:
		void initWidgetBackground(QWidget *w);
		void createTitleWidget(QWidget* parent);
		void createInfoWidgets(QWidget* parent);

		AButton* m_homeButton;
		AButton* m_backButton;
		QWidget* m_infoWidgetsContainer;
		ClockWidget *m_clockWidget;
		BatteryWidget *m_batteryWidget;
		QLabel *m_networkWidget;
		VolumeWidget *m_volumeWidget;
		QWidget *m_titleWidget;
		QLabel *m_spinLabel;
		TitleLabel *m_titleLabel;
		QTimer *m_spinTimer;

		QRect m_titleWidgetFullRect;
		QRect m_titleWidgetReducedRect;

		typedef QValueList<QPixmap> PixmapList;
		PixmapList m_spinningPixmaps;
		QPixmap m_pdfIcon;

		bool m_working;
		PixmapList::ConstIterator m_spinningIt;
};


#endif // STATUSBAR_H
