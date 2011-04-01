/* poppler-page.cc: qt interface to poppler
 * Copyright (C) 2005, Net Integration Technologies, Inc.
 * Copyright (C) 2005, Tobias Koening
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <poppler-qt.h>
#include <qfile.h>
#include <qimage.h>
#include <qpainter.h>
#include <GlobalParams.h>
#include <PDFDoc.h>
#include <UGooString.h>
#include <Link.h>
#include <Catalog.h>
#include <Object.h>
#include <ErrorCodes.h>
#include <SplashOutputDev.h>
#if 0
#include <TextOutputDev.h>
#endif
#include <splash/SplashBitmap.h>
#include "poppler-private.h"
#include "poppler-page-transition-private.h"

#include <stdio.h>

namespace Poppler {

class PageData {
  public:
  const Document *doc;
  int index;
  PageTransition *transition;
};

Page::Page(const Document *doc, int index) : links(0) {
  data = new PageData();
  data->index = index;
  data->doc = doc;
  data->transition = 0;

  setupLinks();
}

Page::~Page()
{
  delete data->transition;
  delete data;
  delete links;
}

void Page::renderToPixmap(QPixmap **q, int x, int y, int w, int h)
{
  renderToPixmap(q, x, y, w, h, 72.0, 72.0);
}

void Page::renderToPixmap(QPixmap **q, int x, int y, int w, int h, double xres, double yres)
{
  QImage img = renderToImage(xres, yres);
  *q = new QPixmap();
  (*q)->convertFromImage(img, Qt::AutoColor);
}

QImage Page::renderToImage(double xres, double yres)
{
  SplashBitmap *bitmap;
  SplashColorPtr color_ptr;
  output_dev = data->doc->data->getOutputDev();

  data->doc->data->doc.displayPageSlice(output_dev, data->index + 1, xres, yres,
      0, false, false, false, -1, -1, -1, -1);
  bitmap = output_dev->getBitmap ();
  color_ptr = bitmap->getDataPtr ();
  int bw = bitmap->getWidth();
  int bh = bitmap->getHeight();

  QImage img;

  img.create(bw, bh, 32, 0, QImage::IgnoreEndian);
  int rowSize = output_dev->getBitmap()->getRowSize();

  for (int i = 0; i < bh; i++) {
      memcpy(img.scanLine(i), color_ptr + i * rowSize, bw * 4);
  }
  return img;
}

// preview is not thread save. may only be used be one thread
// at a time.
void Page::getPagePreview(QImage **q)
{
    // FIXME: the output_dev must be protected by a lock?
    SplashBitmap *bitmap;
    SplashColorPtr color_ptr;
    output_dev = data->doc->data->getOutputDev();
    if (output_dev != 0) {
        bitmap = output_dev->getBitmap ();
        int bw = bitmap->getWidth();
        int bh = bitmap->getHeight();
        color_ptr = bitmap->getDataPtr ();
        *q = new QImage();
        (*q)->create(bw, bh, 32, 0, QImage::IgnoreEndian);
	int rowSize = output_dev->getBitmap()->getRowSize();
	for (int i = 0; i < bh; i++) {
		memcpy((*q)->scanLine(i), color_ptr + i * rowSize, bw * 4);
  	}
   }
}

void Page::renderToImageScaled(QImage **q, int hDpi, int vDpi,
             bool (*abortCheckCbk)(void *data),
             void *abortCheckCbkData)
{
  SplashBitmap *bitmap;
  SplashColorPtr color_ptr;
  output_dev = data->doc->data->getOutputDev();

  data->doc->data->doc.displayPageSlice(output_dev, data->index + 1, hDpi, vDpi,
      0, false, false, false, -1, -1, -1, -1, (GBool (*)(void*))abortCheckCbk,
                                          abortCheckCbkData);

  bitmap = output_dev->getBitmap ();
  color_ptr = bitmap->getDataPtr ();
  int bw = bitmap->getWidth();
  int bh = bitmap->getHeight();

  *q = new QImage();
  (*q)->create(bw, bh, 32, 0, QImage::IgnoreEndian);
  int rowSize = output_dev->getBitmap()->getRowSize();

  for (int i = 0; i < bh; i++) {
     memcpy((*q)->scanLine(i), color_ptr + i * rowSize, bw * 4);
  }
}

void Page::renderToBufferScaled(unsigned char *buffer[], int *num_bytes, int *dst_linestep, int *w, int *h, int fmt, int hDpi, int vDpi, 
		int (*memoryAllocator)(unsigned char *buffer[], int *dst_linestep, int w, int h, int fmt, void *ctx),
		void *ctx,
		bool (*abortCheckCbk)(void *data),
		void *abortCheckCbkData)
{
	if (!buffer || !dst_linestep)
		return;
		
	if (!memoryAllocator)
		return;
	
	SplashBitmap *bitmap;
	SplashColorPtr color_ptr;
	output_dev = data->doc->data->getOutputDev();

	data->doc->data->doc.displayPageSlice(output_dev, data->index + 1, hDpi, vDpi,
	    0, false, false, false, -1, -1, -1, -1, (GBool (*)(void*))abortCheckCbk,
						abortCheckCbkData);

	bitmap = output_dev->getBitmap ();
	color_ptr = bitmap->getDataPtr ();
	int bw = bitmap->getWidth();
	int bh = bitmap->getHeight();

	int src_lstep = output_dev->getBitmap()->getRowSize();
	
	int size;
	
	if ((size = memoryAllocator(buffer, dst_linestep, bw, bh, fmt, ctx)) < 0) {
	      	*buffer = NULL;
		return;
	}
	else if (num_bytes)
		*num_bytes = size;		

	if (w) *w = bw;
	if (h) *h = bh;
	
	switch (fmt)
	{
		case BufferFormatInterleavedRgb:
		{
			unsigned char *ptrR = buffer[0];
			unsigned char *ptrG = buffer[1];
			unsigned char *ptrB = buffer[2];
			
			for (int i = 0; i < bh; i++)
			{
				unsigned char *src = color_ptr + i * src_lstep;
				for (int j = 0; j < bw; j++) {
					ptrR[j] = *src++;
					ptrG[j] = *src++;
					ptrB[j] = *src++;
					
					src++; // alpha;
				}
				ptrR += *dst_linestep;
				ptrG += *dst_linestep;
				ptrB += *dst_linestep;
			}

			break;
		}
		case BufferFormatInterleavedRgba:
		{
			unsigned char *ptrR = buffer[0];
			unsigned char *ptrG = buffer[1];
			unsigned char *ptrB = buffer[2];
			unsigned char *ptrA = buffer[3];
			
			for (int i = 0; i < bh; i++)
			{
				unsigned char *src = color_ptr + i * src_lstep;
				for (int j = 0; j < bw; j++) {
					ptrR[j] = *src++;
					ptrG[j] = *src++;
					ptrB[j] = *src++;
					ptrA[j] = *src++;
				}
				ptrR += *dst_linestep;
				ptrG += *dst_linestep;
				ptrB += *dst_linestep;
				ptrA += *dst_linestep;
			}
			break;
		}
		
		default:
		{
			unsigned char *ptr = *buffer;
	
			for (int i = 0; i < bh; i++) {
				memcpy(ptr, color_ptr + i * src_lstep, bw * 4);
				ptr += *dst_linestep;
			}
			break;
		}
		
	}
}

void Page::setupLinks()
{
  Catalog* catalog = data->doc->data->doc.getCatalog();
  //qWarning("PageSize h = %i; w = %i", pageSize().width(), pageSize().height());

  if (links == 0) {
    Object obj;
    ::Page *p = catalog->getPage(data->index + 1);
    links = new ::Links(p->getAnnots(&obj), catalog->getBaseURI());
    obj.free();
  }
}

Poppler::Links Page::getLinks()
{
  Poppler::Links ret;
  int linkCnt = links->getNumLinks();
  for (int i = 0; i < linkCnt; i++) {
    ::Link *l = links->getLink(i);
    double x1, y1, x2, y2;
    int x1t, y1t, x2t, y2t;
    l->getRect(&x1, &y1, &x2, &y2);
    //qWarning("x1 = %f y1 = %f x2 = %f y2 = %f", x1, y1, x2, y2);
    SplashOutputDev *output = data->doc->data->getOutputDev();
    output->cvtUserToDev( x1, y1, &x1t, &y1t );
    output->cvtUserToDev( x2, y2, &x2t, &y2t );
    //qWarning("x1t = %i  y1t = %i x2t = %i  y2t = %i", x1t, y1t, x2t, y2t);
    ret.addLink(x1t, y1t, x2t, y2t, i);
  }
  return ret;
}

int Page::linkId(double x, double y) const
{
  int ret = -1;
  int linkCnt = links->getNumLinks();
  if (linkCnt <= 0)
    return ret;

  QPoint pdfCoords = translatePoint(x, y);
  for (int i = 0; i < linkCnt; i++) {
    ::Link *l = links->getLink(i);
    if (l->inRect(pdfCoords.x(), pdfCoords.y())) {
      //qWarning("clicked on link %i", i);
      ret = i;
      break;
    }
  }
  return ret;
}

int Page::linkDest(double x, double y) const
{
    QPoint pdfCoords = translatePoint(static_cast<int>(x), static_cast<int>(y));
    //qWarning("pdfCoords x = %i y = %i", pdfCoords.x(), pdfCoords.y());
    LinkAction *action = links->find(pdfCoords.x(), pdfCoords.y());
    int dstPage = -1;
    Catalog *catalog = data->doc->data->doc.getCatalog();

    if (action && action->getKind() == actionGoTo) {
        LinkGoTo* lgt = (LinkGoTo*)action;
        LinkDest *dst = lgt->getDest();
        if (!dst) {
            UGooString *namedDst = lgt->getNamedDest();
            if (namedDst) {
                dst = catalog->findDest(lgt->getNamedDest());
            }
        }

        if (dst) {
          if (dst->isPageRef()) {
            Ref pageref = dst->getPageRef();
            dstPage = catalog->findPage(pageref.num,pageref.gen);
          }
          else {
            dstPage = dst->getPageNum();
          }
        }
    }

    // HACK sometimes the pdf page count start at 0 and sometimes 1
    if (dstPage != -1) {
        dstPage--;
    }
    return dstPage;
}

// x/y are coordinates within the pixmap
bool Page::isLink(int x, int y)
{
    //qWarning("we check if %i %i is a link", x, y);
    QPoint pdfCoords = translatePoint(x, y);
    //qWarning("(translated)we check if %i %i is a link %i",
    //  pdfCoords.x(), pdfCoords.y(), links->onLink(pdfCoords.x(), pdfCoords.y()));
    return links->onLink(pdfCoords.x(), pdfCoords.y());
}

QPoint Page::translatePoint(int x1, int y1) const
{
  double xt, yt;
  SplashOutputDev *output = data->doc->data->getOutputDev();
  output->cvtDevToUser( (double)x1, (double)y1, &xt, &yt );
  //qWarning("xt = %f  yt = %f", xt, yt);
  double nl = (double)xt,
         nt = (double)yt;
  //qWarning("nl = %f  nt = %f", nl, nt);
  return QPoint(static_cast<int>(nl), static_cast<int>(nt));
}

#if 0
QString Page::getText(const Rectangle &r) const
{
  TextOutputDev *output_dev;
  GooString *s;
  PDFRectangle *rect;
  QString result;
  ::Page *p;

  output_dev = new TextOutputDev(0, gFalse, gFalse, gFalse);
  data->doc->data->doc.displayPageSlice(output_dev, data->index + 1, 72, 72,
      0, false, false, false, -1, -1, -1, -1);
  p = data->doc->data->doc.getCatalog()->getPage(data->index + 1);
  if (r.isNull())
  {
    rect = p->getCropBox();
    s = output_dev->getText(rect->x1, rect->y1, rect->x2, rect->y2);
  }
  else
  {
    double height, y1, y2;
    height = p->getCropHeight();
    y1 = height - r.m_y2;
    y2 = height - r.m_y1;
    s = output_dev->getText(r.m_x1, y1, r.m_x2, y2);
  }

  result = QString::fromUtf8(s->getCString());

  delete output_dev;
  delete s;
  return result;
}

QValueList<TextBox*> Page::textList() const
{
  TextOutputDev *output_dev;
  
  QValueList<TextBox*> output_list;
  
  output_dev = new TextOutputDev(0, gFalse, gFalse, gFalse);

  data->doc->data->doc.displayPageSlice(output_dev, data->index + 1, 72, 72,
      0, false, false, false, -1, -1, -1, -1);

  TextWordList *word_list = output_dev->makeWordList();
  
  if (!word_list) {
    delete output_dev;
    return output_list;
  }
  
  for (int i = 0; i < word_list->getLength(); i++) {
    TextWord *word = word_list->get(i);
    QString string = QString::fromUtf8(word->getText()->getCString());
    double xMin, yMin, xMax, yMax;
    word->getBBox(&xMin, &yMin, &xMax, &yMax);
    
    TextBox* text_box = new TextBox(string, Rectangle(xMin, yMin, xMax, yMax));
    
    output_list.append(text_box);
  }
  
  delete word_list;
  delete output_dev;
  
  return output_list;
}
#endif

PageTransition *Page::getTransition() const
{
  if (!data->transition) 
  {
    Object o;
    PageTransitionParams params;
    params.dictObj = data->doc->data->doc.getCatalog()->getPage(data->index + 1)->getTrans(&o);
    data->transition = new PageTransition(params);
    o.free();
  }
  return data->transition;
}

QSize Page::pageSize() const
{
  ::Page *p = data->doc->data->doc.getCatalog()->getPage(data->index + 1);
  return QSize( (int)p->getMediaWidth(), (int)p->getMediaHeight() );
}

Page::Orientation Page::orientation() const
{
  ::Page *p = data->doc->data->doc.getCatalog()->getPage(data->index + 1);

  int rotation = p->getRotate();
  switch (rotation) {
  case 90:
    return Page::Landscape;
    break;
  case 180:
    return Page::UpsideDown;
    break;
  case 270:
    return Page::Seascape;
    break;
  default:
    return Page::Portrait;
  }
}


}
