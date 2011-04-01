#include <qpainter.h>
#include <qpixmap.h>
#include <qimage.h>
#include <qwidget.h>
#include <qscrollview.h>
#include <qlabel.h>
#include <qpoint.h>
#include <qarray.h>
#include <qtimer.h>
#include <qthread.h>
#include <qapplication.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <qgfx_qws.h>
#include <stdio.h>

#include "pdfwidget.h"

#include <archos/aimage.h>
#include <archos/alog.h>
#include <archos/dmalloc.h>

#define MAX_SCROLL_DELAY 8

#define RESIZER_CONSTRAINT 256

#define NO_SCROLLBAR

#define LINKS_SUPPORT


static Poppler::Page *s_page = 0;
static QWidget *s_eventReceiver;

static bool abortCheck(void*);
static int memoryAllocator(unsigned char *buffer[], int *scan_line, int w, int h, int fmt, void *ctx);

// must be accessable by the pdfWidget so it can abort the poppler library
static volatile bool s_abortedFastMove = false;
static struct timeval s_preview_time;

struct RenderJob
{
	Poppler::Document *doc;
	int pageNum;
	int scale;
};

struct RenderImage
{
	unsigned char *buffer[4];
	int width;
	int height;
	int linestep;
	int numBytes;
};

class RenderThread : public QThread 
{
	public :
		RenderThread(QObject* m) : QThread()
			, master(m)
			, currentJobPending(false)
			, jobBufferPending(false)
		{}

		virtual void run(void)
		{
			RenderImage *img = NULL;

			while (currentJobPending || jobBufferPending) {
				lock.lock();
				if (jobBufferPending) {
					if (img) {
						ALOG_DEBUG("free current render buffer");
						DMALLOC_free(img->buffer[0]);
						delete img;
						img = NULL;
					}
					ALOG_DEBUG("picking up job from Buffer");
					currentJob = jobBuffer;
					jobBufferPending = false;
					s_abortedFastMove = false;
				}
				lock.unlock();
				delete s_page;
				s_page = 0;
				s_page = currentJob.doc->getPage(currentJob.pageNum);
ALOG_DEBUG("starting to work on page %i, scale %i", currentJob.pageNum, currentJob.scale);
				gettimeofday(&s_preview_time, NULL);
				img = new RenderImage;

				s_page->renderToBufferScaled(img->buffer, &img->numBytes, &img->linestep, &img->width, &img->height, Poppler::Page::BufferFormatInterleavedRgb, currentJob.scale, currentJob.scale, memoryAllocator, (void*)this /* ctx */, &abortCheck, 0);
ALOG_DEBUG("finished Page");
				currentJobPending = false;
			}
			QApplication::postEvent(master, new QCustomEvent((QEvent::Type)PageReady, img));
			img = NULL;
		}

		void setJob(RenderJob& j) 
		{
			lock.lock();
			if (!running()) {
ALOG_DEBUG("starting m_renderThread");
				currentJob = j;
				currentJobPending = true;
				s_abortedFastMove = false;
				start();
			} else {
ALOG_DEBUG("adding job for running thread");
				jobBuffer = j;
				jobBufferPending = true;
				s_abortedFastMove = true;
			}
			lock.unlock();
		}
	private:
		QObject* master;
		QMutex lock;
		RenderJob currentJob;
		bool currentJobPending;
		RenderJob jobBuffer;
		bool jobBufferPending;
};

static int memoryAllocator(unsigned char *buffer[], int *linestep, int w, int h, int fmt, void *)
{
ALOG_DEBUG("memoryAllocator, w: %d, h: %d, fmt: %d", w, h, fmt);

	//RenderThread *thread = (RenderThread *)ctx;

	int numBytes = 0;

	switch (fmt) {
		case Poppler::Page::BufferFormatInterleavedRgb:
		{
			int lstep = (w + 31) & ~0x1f; /* make it 32 byte aligned */
			//int h = (h + 31) & ~0x1f; /* make it 32 byte aligned */
			int buffer_size = lstep*h;

			numBytes = buffer_size*3;

			buffer[0] = (unsigned char *)DMALLOC_alloc(numBytes);
			if (!buffer[0] || ((unsigned long)buffer[0] & 0x0000001f)) {
ALOG_DEBUG("null or not 32 byte aligned (0x%08x)", buffer[0]);
				return -1;
			}
			buffer[1] = buffer[0] + buffer_size;
			buffer[2] = buffer[1] + buffer_size;

			if (linestep)
				*linestep = lstep;

			break;
		}

		default:
		{
			numBytes = w*h*4;
			unsigned char *tmp = (unsigned char *)DMALLOC_alloc(numBytes);
			buffer[0] = tmp;
			if (!buffer[0] || ((unsigned long)buffer[0] & 0x0000001f)
			  ) {
				ALOG_WARNING("null or not 32 byte aligned");
				return -1;
			}

			if (linestep)
				*linestep = w;
			break;
		}
	}
ALOG_DEBUG("memoryAllocator, w: %d, h: %d, fmt: %d, numBytes: %d, %08x", w, h, fmt, numBytes, buffer[0]);
	return numBytes; /* real size */
}

bool preview_time_elapsed(struct timeval *preview, struct timeval *now)
{
	// previews every 3/4 seconds
	return ((now->tv_sec * 1000000 + now->tv_usec) -
			(preview->tv_sec * 1000000 + preview->tv_usec))	>= 7500000;
}

static bool abortCheck(void*)
{
	bool ret = false;

	if (s_abortedFastMove) {
		ALOG_WARNING("s_abortedFastMove");
		ret = true;
		s_abortedFastMove = false;
	}
	return ret;
}

PDFWidget::PDFWidget(Poppler::Document *d, QWidget *parent, const char* name, int zoomLevelMax) :
	QScrollView(parent, name, WRepaintNoErase ),
	m_currentPage(0),
	m_aimage_orig(0),
	m_doc(d),
	m_page(0),
	m_panningPos(0),
	m_zoomMode(ZoomModeFitToWidth),
	m_zoomLevel(-1),
	m_zoomLevelMax(zoomLevelMax),
	m_scheduledContentsPosX(0),
	m_scheduledContentsPosY(0),
	m_selectedLink(-1),
	m_linkSelected(false),
	m_LinkPos(0),
	m_x_origin(0),
	m_y_origin(0),
	m_direction(Forward),
	m_pageEndOffs(0),
	m_delayCounter(0)
{
	m_renderThread = new RenderThread(this);

	setHScrollBarMode(AlwaysOff);
	setVScrollBarMode(AlwaysOff);
	setFrameStyle(NoFrame);

	s_eventReceiver = this;
	connect(&m_delayCounterTimer, SIGNAL(timeout()), this, SLOT(resetDelayCounter())); // keypad
	render();
}

void PDFWidget::freeSourceImage()
{
	if (m_aimage_orig != NULL) {
		unsigned char *buffer = m_aimage_orig->pixels();
		//int num_bytes = m_aimage_orig->numBytes();
		delete m_aimage_orig;
		DMALLOC_free(buffer);
		m_aimage_orig = 0;
	}
}


PDFWidget::~PDFWidget(void)
{
	freeSourceImage();
	delete m_renderThread;
}

void PDFWidget::setDocument( Poppler::Document *d, int level) 
{
	m_doc = d;
	m_currentPage = 0;
	delete m_page;
	m_page = m_doc->getPage(m_currentPage);
	m_pixmap = QPixmap();
	freeSourceImage();
	emitNavSignals();
	setZoomLevel(level);
	render();

}

void PDFWidget::setDocument( Poppler::Document *d, ZoomMode mode) 
{
	m_doc = d;
	m_currentPage = 0;
	delete m_page;
	m_page = m_doc->getPage(m_currentPage);
	m_pixmap = QPixmap();
	freeSourceImage();
	emitNavSignals();
	setZoomMode(mode);
	emit zoomChanged();
	render();
}

void PDFWidget::drawContents( QPainter *painter, int, int, int, int)
{
	int sv_width = viewport()->width();
	int sv_height = viewport()->height();

	if (!m_pixmap.isNull()) {

		m_x_origin = m_pageOffs.x();
		m_y_origin = m_pageOffs.y();

		painter->drawPixmap(m_x_origin, m_y_origin, m_pixmap);
		painter->translate(m_x_origin, m_y_origin);
		drawLinks(painter);
		painter->translate(-m_x_origin, -m_y_origin);
		// paint the visible content area not used by the pixmap
		// structured so we avoid painting overlapping areas twice
		painter->fillRect(0, 0, m_x_origin, contentsHeight(),
				QColor(123, 121, 123));
		// right of the pixmap
		painter->fillRect(m_x_origin + m_pixmap.width(), 0,
				contentsWidth(), contentsHeight(), QColor(123, 121, 123));
		// over the pixmap
		painter->fillRect(m_x_origin, 0,
				m_x_origin + m_pixmap.width(),
				m_y_origin, QColor(123, 121, 123));
		// under the pixmap
		painter->fillRect(m_x_origin,
				m_y_origin + m_pixmap.height(),
				m_x_origin + m_pixmap.width(),
				contentsHeight(), QColor(123, 121, 123));
	} else {
		ALOG_WARNING("No valid pixmap to display");
		painter->fillRect(0, 0, sv_width, sv_height, QColor(123, 121, 123));
	}
}

int PDFWidget::fitToRenderBuffer(int bw, int bh) const
{
	float pWidth;
	float pHeight;

	Poppler::Page::Orientation orientation = m_page->orientation();
	if (orientation == Poppler::Page::Landscape ||
		orientation == Poppler::Page::Seascape) {
		pWidth = (float)m_page->pageSize().height();
		pHeight = (float)m_page->pageSize().width();
	} else {
		pWidth = (float)m_page->pageSize().width();
		pHeight = (float)m_page->pageSize().height();
	}

	return (int)(QMIN(((float)bw * 72.0) / pWidth, ((float)bh * 72.0) / pHeight));
}

int PDFWidget::getFitToPageLevelValue() const
{
	if (!m_aimage_orig || m_aimage_orig->isNull())
		return 100;

	int dw = qt_screen->width();
	int dh = qt_screen->height();

	return (int)(QMIN(dw * 100 / m_aimage_orig->width(),dh * 100 / m_aimage_orig->height()));
}

int PDFWidget::getFitToWidthLevelValue() const
{
	if (!m_aimage_orig || m_aimage_orig->isNull())
		return 100;

	int dw = qt_screen->width();

	return dw * 100 / m_aimage_orig->width();
}

int PDFWidget::getFitToWidthEnlargedLevelValue() const
{
	return 100;
}

void PDFWidget::zoomOut()
{
	int level = m_zoomLevel - 1;
	setZoomLevel(level);
	m_selectedLink = selectLink();
}

void PDFWidget::zoomIn()
{
	int level = m_zoomLevel + 1;
	setZoomLevel(level);
	m_selectedLink = selectLink();
}

bool PDFWidget::zoomIsDefault() const
{
	return getZoomLevel() == getFitToWidthLevelValue();
}

void PDFWidget::setFitToPage()
{
	setZoomMode(ZoomModeFitToPage);
}

void PDFWidget::setFitToWidth()
{
	setZoomMode(ZoomModeFitToWidth);
}

void PDFWidget::setFitToWidthEnlarged()
{
	setZoomMode(ZoomModeFitToWidthEnlarged);
}

int PDFWidget::getZoomLevelMax() const
{
	return (int)QMAX(m_zoomLevelMax, getFitToWidthLevelValue());
}

int PDFWidget::getZoomLevelMin() const
{
	return (int)QMIN(m_zoomLevelMax, getFitToPageLevelValue());
}

int PDFWidget::getZoomLevel() const
{
	if (getZoomMode() == ZoomModeFitToPage)
		return getFitToPageLevelValue();
	else if (getZoomMode() == ZoomModeFitToWidth)
		return getFitToWidthLevelValue();
	else
		return m_zoomLevel; // custom mode
}

ZoomMode PDFWidget::getZoomMode() const
{
	return m_zoomMode;
}

void PDFWidget::setZoomMode(ZoomMode mode)
{
	if (mode == m_zoomMode)
		return;

	m_zoomMode = mode;

	if (mode == ZoomModeCustomValue )
		return;

	int level;

	if (mode == ZoomModeFitToPage)
		level = getFitToPageLevelValue();
	else if (mode == ZoomModeFitToWidthEnlarged)
		level = getFitToWidthEnlargedLevelValue(); // default
	else
		level = getFitToWidthLevelValue(); // default

	setZoom(level);
}

void PDFWidget::setZoomLevel(int level)
{
	if (level > getZoomLevelMax())
		level = getZoomLevelMax();
	else if (level < getZoomLevelMin())
		level = getZoomLevelMin();

	m_zoomMode = ZoomModeCustomValue;

	if (level == m_zoomLevel)
		return;

	setZoom(level);
}

void PDFWidget::setZoom(int level)
{
	m_zoomLevel = level;

	emit zoomChanged();

	if (m_doc == 0) {
		return;
	}
	scale();
}


class Scaler : public QThread {
public:
	Scaler(archos::AImage* image, int zoomLevel)
	: m_srcImage(image)
	, m_zoomLevel(zoomLevel)
	{}

	const archos::AImage *result() const
	{
		ASSERT(!running());
		return &m_dstImage;
	}

protected:
	virtual void run()
	{
		archos::AImage scaled = m_srcImage->scale(m_zoomLevel, 0);
		m_dstImage = scaled.convert(32, archos::AImage::FormatNonInterleaved);
	}

private:
	archos::AImage *m_srcImage;
	int m_zoomLevel;
	archos::AImage m_dstImage;
};


void PDFWidget::scale()
{
	if (!m_aimage_orig) {
		ALOG_WARNING("no buffer to scale");
		return;
	}

	emit scaling(true);

	// Thread the scaling so that the GUI is still updated
	Scaler scaler(m_aimage_orig, getZoomLevel());
	scaler.start();
	while (scaler.running()) {
		qApp->processEvents();
	}
	
	// NOTE: dstQImage won't own the pixels in dstAImage, thus dstAImage must
	// not be deleted before dstQImage
	const archos::AImage *dstAImage = scaler.result();
	QImage dstQImage(dstAImage->pixels(), dstAImage->width(), dstAImage->height(), dstAImage->depth(), dstAImage->lineStep(), 0, 0, QImage::IgnoreEndian);

	m_pixmap.convertFromImage(dstQImage, Qt::AutoColor);

	pageResize();
	//scheduleContentsPosUpdate(getContentsPos(m_center.x())-viewport()->width()/2, getContentsPos(m_center.y())-viewport()->height()/2);
	//center(getContentsPos(m_center.x()), getContentsPos(m_center.y()));

	m_selectedLink = selectLink();
ALOG_DEBUG("selectedLink = %i", m_selectedLink);

	updateContents(0, 0, contentsWidth(), contentsHeight());

	emit scaling(false);
}

void PDFWidget::updateViewRect()
{
	int x1, y1, x2, y2;

	if (m_pixmap.isNull())
		return;

	if (m_pixmap.width() < viewport()->width()) {
		x1 = 0;
		x2 = m_pixmap.width();
	} else {
		x1 = contentsX() - m_x_origin > 0 ? contentsX() - m_x_origin : 0;
		x2 = x1 + visibleWidth() - m_x_origin;
		// FIXME if there is no border visible it's off by borderwidth pixels.
	}

	if (m_pixmap.height() < viewport()->height()) {
		ALOG_DEBUG("doc inside vp");
		y1 = 0;
		y2 = m_pixmap.height();
	} else {
		if (contentsY() <= m_y_origin) {
			// border visibl. at the top
			ALOG_DEBUG("border at top");
			y1 = 0;
			y2 = visibleHeight() - (m_y_origin - contentsY());
		} else if ((contentsY() - m_y_origin) + visibleHeight() >= m_pixmap.height()) {
			// border at the bottom
			ALOG_DEBUG("border at bottom");
			y1 = contentsY() - m_y_origin;
			y2 = y1 + (visibleHeight() - m_y_origin);
		} else {
			// no border visible
			ALOG_DEBUG("no border whatsoever");
			y1 = contentsY() - m_y_origin;
			y2 = y1 + visibleHeight();
		}
	}
	ALOG_DEBUG("y1 = %i, y2 = %i (diff %i)", y1, y2, y2 - y1);
	m_viewRect = QRect(QPoint(x1, y1), QPoint(x2, y2));
}

void PDFWidget::resetDelayCounter(void)
{
	m_delayCounter = 0;
}

void PDFWidget::handleNextPageDelay(int direction)
{
	if ( ( verticalScrollBar()->value() == verticalScrollBar()->minValue() ||
		verticalScrollBar()->value() == verticalScrollBar()->maxValue() ) && !m_renderThread->running()) {
		m_delayCounter++;
		if ( m_delayCounter > MAX_SCROLL_DELAY ) {
			if ( direction == Back ) {
				prevPage();
			}
			else {
				nextPage();
			}
			m_delayCounter = 0;
		}
		if ( !m_delayCounterTimer.isActive() ) {
			m_delayCounterTimer.start(750, true);
		}
	}
	else {
		m_delayCounterTimer.stop();
		m_delayCounter = 0;
	}
}

void PDFWidget::scrollUp(int factor)
{
	int tmp = selectNextLink(Up, m_selectedLink);
	if (tmp != m_selectedLink && tmp != -1) {
		m_selectedLink = tmp;
		QRect m = m_viewRect; m.moveBy(0, -32);
#ifdef LINKS_SUPPORT
		Poppler::Links links = m_page->getLinks();
		Poppler::Link l = links.getLink(m_selectedLink);
		// will the link be visible if we scroll?
		if (m.contains(l.getScreenCoords().center(), true)) {
			#ifdef ALOG_ENABLE_DEBUG
			int x1, y1, x2, y2;
			m.coords(&x1, &y1, &x2, &y2);
			ALOG_DEBUG("x1 = %i, y1 = %i x2 = %i y2 = %i", x1, y1, x2, y2);
			ALOG_DEBUG("xc = %i, yc = %i", l.getScreenCoords().center().x(), l.getScreenCoords().center().y());
			#endif
			scrollBy(0, -8 * factor);
		}
#endif
		updateContents(0, 0, contentsWidth(), contentsHeight());
	}
	else {
		if ( tooSmalltoScroll() && !m_renderThread->running()) {
			prevPage();
		}
		else {
			scrollBy(0, -8 * factor);
			handleNextPageDelay(Back);
		}
	}
	updateViewRect();
}

void PDFWidget::scrollDown(int factor)
{
	int tmp = selectNextLink(Down, m_selectedLink);
	if (tmp != m_selectedLink && tmp != -1) {
		m_selectedLink = tmp;
		QRect m = m_viewRect; m.moveBy(0, 32);
#ifdef LINKS_SUPPORT
		Poppler::Links links = m_page->getLinks();
		Poppler::Link l = links.getLink(m_selectedLink);
		// will the link be visible if we scroll?
		if (m.contains(l.getScreenCoords().center(), true)) {
			#ifdef ALOG_ENABLE_DEBUG
			int x1, y1, x2, y2;
			m.coords(&x1, &y1, &x2, &y2);
			ALOG_DEBUG("x1 = %i, y1 = %i x2 = %i y2 = %i", x1, y1, x2, y2);
			ALOG_DEBUG("xc = %i, yc = %i", l.getScreenCoords().center().x(), l.getScreenCoords().center().y());
			#endif
			scrollBy(0, 8 * factor);
		}
#endif
		updateContents(0, 0, contentsWidth(), contentsHeight());
	}
	else {
		if ( tooSmalltoScroll() && !m_renderThread->running()) {
			nextPage();
		}
		else {
			scrollBy(0, 8 * factor);
			handleNextPageDelay(Forward);
		}
	}
	updateViewRect();
}

void PDFWidget::scrollLeft(int factor)
{
#ifdef LINKS_SUPPORT
	int tmp = selectNextLink(Left, m_selectedLink);
	if (tmp != m_selectedLink) {
		m_selectedLink = tmp;
		scrollBy(-8 * factor, 0);
		updateContents(0, 0, contentsWidth(), contentsHeight());
	}
	else 
#endif
	{
		scrollBy(-8 * factor, 0);
	}
	updateViewRect();
}

void PDFWidget::scrollRight(int factor)
{
#ifdef LINKS_SUPPORT
	int tmp = selectNextLink(Right, m_selectedLink);
	if (tmp != m_selectedLink) {
		m_selectedLink = tmp;
		scrollBy(8 * factor, 0);
		updateContents(0, 0, contentsWidth(), contentsHeight());
	}
	else 
#endif
	{
		scrollBy(8 * factor, 0);
	}
	updateViewRect();
}

void PDFWidget::render()
{
	if (m_doc) {
		emit rendererRunning(true);
		m_firstPreview = true;

		RenderJob j = { m_doc, m_currentPage, fitToRenderBuffer(PDF_SOURCE_RES_WIDTH, PDF_SOURCE_RES_HEIGHT/*794*2,1123*2*/) };
		m_renderThread->setJob(j);
	}
	else {
		ALOG_WARNING("No document was set");
	}
}

void PDFWidget::customEvent(QCustomEvent *e) 
{
	if ((int)e->type() == PageReady) {
		pageReady(e->data());
	} else {
		QScrollView::customEvent(e);
	}
}

void PDFWidget::pageReady(void* args)
{
ALOG_DEBUG("pageReady:-)\n");
	freeSourceImage();
	RenderImage *img = (RenderImage*)args;
	m_aimage_orig = new archos::AImage (img->buffer, img->numBytes, img->width, img->height, 24, img->linestep, archos::AImage::FormatInterleaved);
	delete img;

	emit rendererRunning(false);

	scale();

	setPagePos(m_scheduledContentsPosX, m_scheduledContentsPosY);
}

QPoint PDFWidget::getLinkPointScaled(QPoint &p)
{
	QPoint sp;

	sp.setX((int)((float)p.x() * (float)getZoomLevel() / 100.0));
	sp.setY((int)((float)p.y() * (float)getZoomLevel() / 100.0));

	return sp;
}

QPoint PDFWidget::getLinkPointScaled(QPoint p)
{
	QPoint sp;

	sp.setX((int)((float)p.x() * (float)getZoomLevel() / 100.0));
	sp.setY((int)((float)p.y() * (float)getZoomLevel() / 100.0));

	return sp;
}

QRect PDFWidget::getLinkRectScaled(QRect &r)
{	int x, y, w, h;
	r.rect(&x, &y, &w, &h);
	const float zoom = getZoomLevel() / 100.;

	x = int(x * zoom);
	y = int(y * zoom);
	w = int(w * zoom);
	h = int(h * zoom);

	return QRect(x,y,w,h);
}

int PDFWidget::getSourcePos(int v)
{
	return (int)((float)v * 100.0 / (float)getZoomLevel());
}

int PDFWidget::getContentsPos(int v)
{
	return (int)((float)v * (float)getZoomLevel() / 100.0);
}

QPoint PDFWidget::ContentsToSource(QPoint p)
{
	QPoint sp;

	sp.setX((int)((float)p.x() * 100.0 / (float)getZoomLevel()));
	sp.setY((int)((float)p.y() * 100.0 / (float)getZoomLevel()));

	return sp;
}

void PDFWidget::adjustCoords()
{
	int x, y;
	viewportToContents(viewport()->width()>>1, viewport()->height()>>1, x, y);
	m_center = QPoint(getSourcePos(x), getSourcePos(y));
}

int PDFWidget::selectNextLink(int direction, int currentLink)
{
#ifdef LINKS_SUPPORT
	Poppler::Links links = m_page->getLinks();
	int linkCnt = links.getNumLinks();

	if (linkCnt <= 0 || m_pixmap.isNull()) {
		ALOG_WARNING("no links on this page");
		return -1;
	}

	if (currentLink == -1 || currentLink >= linkCnt) {
		ALOG_WARNING("no valid link. trying to select a new one");
		return selectLink();
	}

	if (currentLink == -1) {
		ALOG_WARNING("still no valid link...");
		return -1;
	}

	ALOG_DEBUG("ok the selected link is %i", currentLink);

	Poppler::Link selected = links.getLink(currentLink);
	Poppler::Link tmp;
	Poppler::Link tmpSameRow;
	QPoint tmpPoint;

	QPoint selectedScaled = getLinkPointScaled(selected.getScreenCoords().center());

	int selected_y = selectedScaled.y();
	int selected_x = selectedScaled.x();

	for (int i = 0; i < linkCnt; i++) {
		Poppler::Link l = links.getLink(i);
		// inside the visible screen?

		QPoint lcoord = getLinkPointScaled(l.getScreenCoords().center());
		if (m_viewRect.contains(lcoord, true)) {
			if (direction == Up) {
				// handle links in the same row further left
				if (selected.getId() != l.getId() && lcoord.y() ==
						selected_y && lcoord.x() < selected_x) {
					if (tmpSameRow.getId() == -1) {
						tmpSameRow = l;
					}
					else if (abs(lcoord.x() - selected_x) < abs(getLinkPointScaled(tmpSameRow.getScreenCoords().center()).x() - selected_x)) {
						tmpSameRow = l;
					}
				}



				// handle links further up
				else if (lcoord.y() < selected_y) {
					if (tmp.getId() != -1) {
						if (abs(selected_y - lcoord.y()) <= abs(selected_y - tmpPoint.y())) {
							// if there are several links further up on the same row take the one most rigth
							if (lcoord.y() == tmpPoint.y()) {
								if (lcoord.x() > tmpPoint.x()) {
									tmp = l;
								}
							}
							else {
								tmp = l;
								tmpPoint = getLinkPointScaled(tmp.getScreenCoords().center());
							}
						}
					}
					else {
						tmp = l;
						tmpPoint = getLinkPointScaled(tmp.getScreenCoords().center());
					}
				}
			}
			else if (direction == Down) {
				// handle links in the same row f::displayurther right
				if (selected.getId() != l.getId() && lcoord.y() ==
						selected_y && lcoord.x() > selected_x) {
					if (tmpSameRow.getId() == -1) {
						tmpSameRow = l;
					}
					else if (abs(lcoord.x() - selected_x) < abs(getLinkPointScaled(tmpSameRow.getScreenCoords().center()).x() - selected_x)) {
						tmpSameRow = l;
					}
				}
				// handle links further down
				else if (lcoord.y() > selected_y) {
					if (tmp.getId() != -1) {
						if (abs(selected_y - lcoord.y()) <= abs(selected_y - tmpPoint.y())) {
							// if there are several links further down on the same row take the one most left
							if (lcoord.y() == tmpPoint.y()) {
								if (lcoord.x() < tmpPoint.x()) {
									tmp = l;
								}
							}
							else {
								tmp = l;
								tmpPoint = getLinkPointScaled(tmp.getScreenCoords().center());
							}
						}
					}
					else {
						tmp = l;
						tmpPoint = getLinkPointScaled(tmp.getScreenCoords().center());
					}
				}
			}
			else if (direction == Left) {
				if (selected.getId() != l.getId() && lcoord.x() < selected_x) {
					if (tmp.getId() == -1) {
						tmp = l;
						tmpPoint = getLinkPointScaled(tmp.getScreenCoords().center());
					}
					else if (abs(lcoord.x() - selected_x) < abs(tmpPoint.x() - selected_x)) {
						tmp = l;
						tmpPoint = getLinkPointScaled(tmp.getScreenCoords().center());
					}
				}
			}
			else if (direction == Right) {
				if (selected.getId() != l.getId() && lcoord.x() > selected_x) {
					if (tmp.getId() == -1) {
						tmp = l;
						tmpPoint = getLinkPointScaled(tmp.getScreenCoords().center());
					}
					else if (abs(lcoord.x() - selected_x) < abs(tmpPoint.x() - selected_x)) {
						tmp = l;
						tmpPoint = getLinkPointScaled(tmp.getScreenCoords().center());
					}
				}
			}
		}
	}

	// links on the same row have precedence
	if (tmpSameRow.getId() != -1) {
		selected = tmpSameRow;
ALOG_DEBUG("next selected link (sameRow): %i", selected.getId());
	} else if (tmp.getId() != -1) {
		selected = tmp;
ALOG_DEBUG("next selected link: %i", selected.getId());
	} else {
		if (!m_viewRect.contains(selected.getScreenCoords().center(), true)) {
			ALOG_WARNING("no new selected link");
			return -1;
		}
	}

ALOG_DEBUG("the returned m_selectedLink is %i", selected.getId());

	return selected.getId();
#else
	return 0;
#endif
}

int PDFWidget::selectLink()
{
#ifdef LINKS_SUPPORT
	Poppler::Links links = m_page->getLinks();
	int linkCnt = links.getNumLinks();
	if (linkCnt < 1 || m_pixmap.isNull()) {
		return -1;
	}

	// is the currently selected link still visible?
	if (m_selectedLink != -1 && m_selectedLink < linkCnt) {
		Poppler::Link l = links.getLink(m_selectedLink);
		if (m_viewRect.contains(getLinkPointScaled(l.getScreenCoords().center()), true)) {
ALOG_DEBUG("old link %i si still visible", m_selectedLink);
			return l.getId();
		}
	}

	Poppler::Link selected(9999, 9999, 9999, 9999, -1);	// invalid
	for (int i = 0; i < linkCnt; i++) {
		Poppler::Link l = links.getLink(i);

		QPoint lcoord = getLinkPointScaled(l.getScreenCoords().center());
		QPoint selectedScaled = getLinkPointScaled(selected.getScreenCoords().center());
ALOG_DEBUG("m_viewRect: %d, %d, %d, %d, lcoord: %d, %d", m_viewRect.x(), m_viewRect.y(), m_viewRect.width(), m_viewRect.height(), lcoord.x(), lcoord.y());

		if (m_viewRect.contains(lcoord, true)) {
ALOG_DEBUG("m_viewRect.contains lcoord");
			// select the link nearest to the top/left
			if (m_direction == Forward) {
				if (lcoord.y() < selectedScaled.y() &&
					lcoord.x() < selectedScaled.x()) {
					selected = l;
				}
			}
			// select the link nearest to the bottom right
			else if (m_direction == Back) {
				if (abs(lcoord.y() - m_pixmap.width()) <
					abs(selectedScaled.y() - m_pixmap.width())) {
					selected = l;
				}
			}
		}
	}
	return selected.getId();
#else
	return 0;
#endif
}

void PDFWidget::drawLinks(QPainter *painter)
{
#ifdef LINKS_SUPPORT
	if (m_pixmap.isNull() || m_renderThread->running())
		return;

	Poppler::Links links = m_page->getLinks();
	int linkCnt = links.getNumLinks();
	if (m_selectedLink >= linkCnt) {
		ALOG_WARNING("m_selectedLink >= linkCnt (%d >= %d)", m_selectedLink, linkCnt);
	}

	QPen selected(QColor(255, 0, 0), 3);	// red, 3 pixel thick
	QPen unselected(QColor(69, 196, 255), 1); // light blue, 1 pixel thick

	for (int i = 0; i < linkCnt; i++) {
		QRect lr = links.getLink(i).getScreenCoords();
		lr = getLinkRectScaled(lr);

		painter->setPen(i == m_selectedLink ? selected : unselected);
		painter->drawRect(lr);
	}
#endif
}

void PDFWidget::followLink()
{
#ifdef LINKS_SUPPORT
	ALOG_DEBUG("follow link");
	if (m_selectedLink == -1) {
		return;
	}

	Poppler::Links links = m_page->getLinks();
	Poppler::Link link = links.getLink(m_selectedLink);
	QPoint center = link.getLinkCenterScreenCoord();
	ALOG_DEBUG("x = %i, y = %i", center.x(), center.y());
	int nxt = m_page->linkDest((double)center.x(), (double)center.y());
	ALOG_DEBUG("nxt = %i", nxt);
	if (nxt != -1 && nxt > 0 && nxt < m_doc->getNumPages()) {
		m_navStack.append(PageNum(m_currentPage));
		ALOG_DEBUG("put %i on the m_navStack", m_currentPage);
		emit pagesInHistory(true);
		jumpToPage(nxt);
	}
#endif
}

bool PDFWidget::rendererRunning(void)
{
	return m_renderThread->running();
}

void PDFWidget::pageResize()
{
	if (m_pixmap.isNull()) {
		resizeContents(0,0);
		return;
	}

	int contentPosX = 0, contentPosY = 0;
	int width, height;
	int posX, posY;

	int visible_width = viewport()->width();

	if (pageWidth() <= visible_width)
		width = viewport()->width();
	else
		width = pageWidth();

	if (pageHeight() <= viewport()->height())
		height = viewport()->height();
	else
		height = pageHeight();

	resizeContents(width, height);

	if (pageWidth() <= visible_width) {
		posX = -(viewport()->width()-pageWidth())/2;
		m_pageOffs.setX(-posX);
	} else {
		posX = getContentsPos(m_center.x())-viewport()->width()/2;
		posX = posX < 0 ? 0 : posX;
		contentPosX = posX;
		m_pageOffs.setX(0);
	}

	if (pageHeight() <= viewport()->height()) {
		posY = -(viewport()->height()-pageHeight())/2;
		m_pageOffs.setY(-posY);
	} else {
		posY = getContentsPos(m_center.y())-viewport()->height()/2;
		posY = posY < 0 ? 0 : posY;
		contentPosY = posY;
		m_pageOffs.setY(0);
	}

	m_page_pos.setX(posX);
	m_page_pos.setY(posY);

	setContentsPos(contentPosX, contentPosY);
}

int PDFWidget::pageWidth() const
{
	return m_pixmap.width();
}

int PDFWidget::pageHeight() const
{
	return m_pixmap.height();
}

void PDFWidget::setPagePos(int x, int y)
{
	if (m_pixmap.isNull() || (x == m_page_pos.x() && y == m_page_pos.y()))
		return;

	m_page_pos.setX(x);
	m_page_pos.setY(y);

	int contentPosX = x, contentPosY = y;

	int visible_width = viewport()->width();

	if (pageWidth() <= visible_width) {
		contentPosX = 0;
	} else {
		contentPosX = x;
		m_pageOffs.setX(0);
		if (contentPosX < 0) {
			contentPosX = 0;
			m_page_pos.setX(0);
		} else if (contentPosX > contentsWidth() - viewport()->width()) {
			contentPosX = contentsWidth() - viewport()->width();
			m_page_pos.setX(contentsWidth() - viewport()->width());
		}
	}

	if (pageHeight() <= viewport()->height()) {
		contentPosY = 0;
	} else {
		contentPosY = y;
		m_pageOffs.setY(0);
		if (contentPosY < 0) {
			contentPosY = 0;
			m_page_pos.setY(0);
		} else if (contentPosY > contentsHeight() - viewport()->height()) {
			contentPosY = contentsHeight() - viewport()->height();
			m_page_pos.setY(contentsHeight() - viewport()->height());
		}
	}

	setContentsPos(contentPosX, contentPosY);

	adjustCoords();

	if (m_pageOffs.x() || m_pageOffs.y()) {
		updateContents(0, 0, contentsWidth(), contentsHeight());
	}
}

QPoint PDFWidget::pagePos()
{
	return m_page_pos;
}

void PDFWidget::scrollBy(int x, int y)
{
	setPagePos(pagePos().x() + x, pagePos().y() + y);
}

// private slot
void PDFWidget::emitNavSignals()
{
	emit currentPageChanged();
}

// public slot
void PDFWidget::nextPage()
{
	if (m_currentPage + 1 < m_doc->getNumPages()) {
		m_selectedLink = -1;
		m_direction = Forward;
		m_currentPage++;
		delete m_page;
		m_page = m_doc->getPage(m_currentPage);
		emitNavSignals();
		render();
		scheduleContentsPosUpdate(contentsX(), 0);
	}
}

// public slot
void PDFWidget::prevPage()
{
	if (m_currentPage > 0) {
		m_selectedLink = -1;
		m_direction = Back;
		m_currentPage--;
		delete m_page;
		m_page = m_doc->getPage(m_currentPage);
		emitNavSignals();
		render();
		scheduleContentsPosUpdate(contentsX(), contentsHeight() - visibleHeight());
	}
}

// public slot
void PDFWidget::lastPage() 
{
	if (m_currentPage != m_doc->getNumPages() - 1) {
		m_selectedLink = -1;
		m_direction = Forward;
		m_currentPage = m_doc->getNumPages() - 1;
		delete m_page;
		m_page = m_doc->getPage(m_currentPage);
		emitNavSignals();
		render();
		scheduleContentsPosUpdate(contentsX(), 0);
	}
}

// public slot
void PDFWidget::firstPage()
{
	if (m_currentPage != 0) {
		m_selectedLink = -1;
		m_direction = Back;
		m_currentPage = 0;
		delete m_page;
		m_page = m_doc->getPage(m_currentPage);
		emitNavSignals();
		render();
		scheduleContentsPosUpdate(contentsX(), 0);
	}
}

// public slot
void PDFWidget::jumpToPage(int pageX)
{
	if ((pageX <= m_doc->getNumPages() - 1) && (pageX != m_currentPage)) {
		m_selectedLink = -1;
		m_direction = Forward;
		m_currentPage = pageX;
		delete m_page;
		m_page = m_doc->getPage(m_currentPage);
		render();
		scheduleContentsPosUpdate(contentsX(), 0);
		emitNavSignals();
	}
}

void PDFWidget::setHotspot(QSize s)
{
	m_hotspot = s;
}

void PDFWidget::viewportMousePressEvent(QMouseEvent *e)
{
	if ( !m_doc ) {
		return;
	}

	int x = e->pos().x(), y = e->pos().y();

	if ( (x > width() - m_hotspot.width()) && (y > height() - m_hotspot.height()) ) {
		emit hotspot();
	}
	else if (e->button() == QMouseEvent::LeftButton) {
		m_panningPos = new QPoint(x, y);
#ifdef LINKS_SUPPORT
		// did the user touch a link?
		viewportToContents(x, y, x, y);
		x = getSourcePos(x -m_x_origin);
		y = getSourcePos(y - m_y_origin);
		m_linkSelected = m_page->isLink(x, y);
		if (m_linkSelected) {
			m_LinkPos = new QPoint(getSourcePos(e->pos().x()), getSourcePos(e->pos().y()));
			int tmpLink = m_page->linkId(x, y);
			if (tmpLink != -1) {
				m_selectedLink = tmpLink;
				ALOG_DEBUG("clickedLink = %i", m_selectedLink);
				updateContents(0, 0, contentsWidth(), contentsHeight());
			}
		}
#endif
	}
}

void PDFWidget::viewportMouseReleaseEvent(QMouseEvent * e)
{
	if ( !m_doc ) {
		return;
	}

	if (e->button() == QMouseEvent::LeftButton) {
		delete m_panningPos;
		m_panningPos = 0;
		m_pageEndOffs = 0;

#ifdef LINKS_SUPPORT
		if (m_linkSelected) {
			int x, y;
			viewportToContents(e->pos().x(), e->pos().y(), x, y);
			x = getSourcePos(x - m_x_origin);
			y = getSourcePos(y - m_y_origin);
			int nxt = m_page->linkDest(x, y);
			if (nxt != -1 && nxt > 0 && nxt < m_doc->getNumPages()) {
				m_navStack.append(PageNum(m_currentPage));
				ALOG_DEBUG("put %i on the m_navStack", m_currentPage);
				emit pagesInHistory(true);
				jumpToPage(nxt);
			}
			m_linkSelected = false;
			delete m_LinkPos;
		}
		else 
#endif
		{
			m_direction = Forward;
#ifdef LINKS_SUPPORT
			m_selectedLink = selectLink();
			ALOG_DEBUG("selectedLink = %i", m_selectedLink);
#endif
			updateContents(0, 0, contentsWidth(), contentsHeight());
		}
	}
}


void PDFWidget::moveBack()
{
	if (m_navStack.fromLast() != m_navStack.end()) {
		int pageNum = m_navStack.last().getPageNum();
		m_navStack.remove(m_navStack.fromLast());
		ALOG_DEBUG("took %i from the m_navStack", pageNum);
		emit pagesInHistory(m_navStack.count());
		jumpToPage(pageNum);
	}
}

bool PDFWidget::arePagesInHistory(void)
{
	return m_navStack.count() > 0;
}

bool PDFWidget::tooSmalltoScroll(void)
{
	return verticalScrollBar()->maxValue() == 0;
}

void PDFWidget::handleMouseAtEndOfPage(int y)
{
	if (!m_panningPos)
		return;

	int diff = m_panningPos->y() - y;

	if(diff < 0 && contentsY() == 0) {
		m_pageEndOffs += diff;
	} else if (diff > 0 && contentsY()+visibleHeight() >= pageHeight()) {
		m_pageEndOffs += diff;
	} else {
		m_pageEndOffs = 0;
		return;
	}

	if (!m_renderThread->running()) {
		if (m_pageEndOffs > 50) {
			m_pageEndOffs = 0;
			nextPage();
		} else if (m_pageEndOffs < -50) {
			m_pageEndOffs = 0;
			prevPage();
		}
	}
}

void PDFWidget::viewportMouseMoveEvent(QMouseEvent * e)
{
	if ( !m_doc ) {
		return;
	}

	if ( e->state() & QMouseEvent::LeftButton && m_panningPos ) {
		// to much moving around, probably clicking the link wasn't the
		// users intention

#ifdef LINKS_SUPPORT
		if (m_linkSelected) {
			int x, y;
			viewportToContents(e->pos().x(), e->pos().y(), x, y);
			x = getSourcePos(x-m_x_origin);
			y = getSourcePos(y-m_y_origin);

			// lets see if we're above a link
			int tmpLink = m_page->linkId(x,  y);
			if (tmpLink != m_selectedLink) {
				m_selectedLink = tmpLink;
				ALOG_DEBUG("movedToLink = %i", m_selectedLink);
				updateContents(0, 0, contentsWidth(), contentsHeight());
			}

			return;

		}
#endif
		handleMouseAtEndOfPage(e->y());

		scrollBy(m_panningPos->x() - e->x(), m_panningPos->y() - e->y());

		updateViewRect();

		m_panningPos->setX(e->x());
		m_panningPos->setY(e->y());
	}
}

void PDFWidget::scheduleContentsPosUpdate( int x, int y )
{
	m_scheduledContentsPosX = x;
	m_scheduledContentsPosY = y;
}

void PDFWidget::resizeEvent(QResizeEvent *)
{
	m_pixmap = QPixmap();
	resizeContents(viewport()->width(), viewport()->height());
	ALOG_DEBUG("viewport()->width() = %i, viewport()->height() = %i", viewport()->width(), viewport()->height());
//setContentsPos(0, 0);
//updateContents(0, 0, contentsWidth(), contentsHeight());
	scale();
}

int PDFWidget::getPageNumber()
{
	return m_currentPage + 1;
}

int PDFWidget::getNumPages()
{
	int ret = 0;
	if (m_doc)
		ret = m_doc->getNumPages();
	return ret;
}
