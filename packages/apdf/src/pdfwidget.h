#include <qscrollview.h>
#include <poppler/poppler-qt.h>
#include <qstring.h>
#include <qrect.h>
#include <qtimer.h>

#include <archos/aimage.h>

class QPixmap;
class QImage;
class QPainter;
class QWidget;
class QKeyEvent;
class QMouseEvent;
class QLabel;
class QPoint;
class RenderThread;

#define MIN_ZOOM_PERCENT_VALUE 25
#define MAX_ZOOM_PERCENT_VALUE 100

/** This is the id of the QCustomEvent with which the renderer thread notifies
 *  the pdfWidget that it is ready and a new pixmap is available.
 */

typedef enum { ZoomModeCustomValue = 0, ZoomModeFitToPage, ZoomModeFitToWidth, ZoomModeFitToWidthEnlarged } ZoomMode;
	
enum { PageReady = 1001 };

// used for the navigation stack
class PageNum {
  public:
    PageNum() {};
    PageNum(int p) : _page(p) { };
    int getPageNum() { return _page; };
  private:
    int _page;
};

enum { Up = 0, Down, Left, Right};
enum { Back = 0, Forward };

class PDFWidget : public QScrollView {
	Q_OBJECT

public:
	PDFWidget(Poppler::Document *d = 0, QWidget *parent = 0, const char* name = 0,
		int maxZoomLevel = MAX_ZOOM_PERCENT_VALUE);
	~PDFWidget();
	void setDocument( Poppler::Document *d, int level = 100);
	void setDocument( Poppler::Document *d, ZoomMode zoomMode = ZoomModeCustomValue);
	
	/** return the current page number. It starts with one. */
	int getPageNumber();
	int getNumPages();
	bool arePagesInHistory();
	bool rendererRunning(void);
	void setHotspot(QSize s);
	int getZoomLevelMax() const;
	int getZoomLevelMin() const;
	ZoomMode getZoomMode() const;
	int getZoomLevel() const;

public slots:
	void nextPage();
	void prevPage();
	void lastPage();
	void firstPage();
	void jumpToPage(int page);
	void scrollUp(int factor);
	void scrollDown(int factor);
	void scrollLeft(int factor);
	void scrollRight(int factor);
	void scrollBy(int x, int y);
	void zoomIn();
	void zoomOut();
	bool zoomIsDefault() const;
	void moveBack();
	void followLink();
	void setFitToPage();
	void setFitToWidth();
	void setFitToWidthEnlarged();
	void setZoomLevel(int);
	void setZoomMode(ZoomMode);

signals:
	void rendererRunning(bool running);
	void scaling(bool scaling);

	void currentPageChanged();

	/** Emited when the current page isn't completly rendered because
	the user aborted the renderer. */
	void currentPageIsAborted();
	void zoomChanged();
	void pagesInHistory(bool pages);
	void hotspot(void);

protected:
    void drawContents( QPainter *painter, int, int, int, int );
    void viewportMousePressEvent ( QMouseEvent * e);
    void viewportMouseReleaseEvent ( QMouseEvent * e);
    void viewportMouseMoveEvent ( QMouseEvent * e);
    void customEvent(QCustomEvent *e);
    void resizeEvent(QResizeEvent *);

private slots:
	void pageReady(void*);
	void resetDelayCounter(void);
	void adjustCoords();

private:
	void setPagePos(int, int);
	QPoint pagePos();
	void pageResize();
	int pageWidth() const;
	int pageHeight() const;

	void scale();
	void render();
	void prevPagePanning();
	void viewportMousePressEventPanning(QMouseEvent *e);
	void viewportMouseReleaseEventPanning(QMouseEvent *e);
	void viewportMouseMoveEventPanning(QMouseEvent * e);
	void setZoom(int);
	int getFitToPageLevelValue() const;
	int getFitToWidthLevelValue() const;
	int getFitToWidthEnlargedLevelValue() const;
	int fitToRenderBuffer(int bw, int wh) const;
	void drawLinks(QPainter *p);
	bool tooSmalltoScroll(void);
	void handleNextPageDelay(int);
	void freeSourceImage();
	QRect getLinkRectScaled(QRect &r);
	QPoint getLinkPointScaled(QPoint &p);
	QPoint getLinkPointScaled(QPoint p);
	int getSourcePos(int v);
	int getContentsPos(int v);
	QPoint ContentsToSource(QPoint p);
	void handleMouseAtEndOfPage(int y);
	
    /** When switching to a new page we have to reposition the ScrollViewContents.
        Rendering the page costs time so we do it in another thread and wait till it's
        finished. Based on the method which requested the new page we wan't the content
        positioned differently. So a method can use this to request a new position
        for the new page. The values are used when the page is ready*/
    void scheduleContentsPosUpdate(int x, int y);

	void emitNavSignals();

	int m_currentPage;
	QPixmap m_pixmap;
	archos::AImage *m_aimage_orig;
	
	Poppler::Document *m_doc;
	Poppler::Page *m_page;
	RenderThread *m_renderThread;

	QPoint* m_panningPos;

	ZoomMode m_zoomMode;
	int m_zoomLevel;
	int m_zoomLevelMax;

	int m_scheduledContentsPosX, m_scheduledContentsPosY;
	bool m_firstPreview;

	int m_selectedLink;
	bool m_linkSelected;
	QPoint *m_LinkPos;
	int m_x_origin, m_y_origin;
	QValueList<PageNum> m_navStack;
	void updateViewRect();
	int selectLink();
	int selectNextLink(int direction, int currentLink);
	QRect m_viewRect;
	int m_direction;
	QSize m_hotspot;
	int m_pageEndOffs;
	QPoint m_center;
	QPoint m_pageOffs;
	QPoint m_page_pos;

	int m_delayCounter;
	QTimer m_delayCounterTimer;
};
