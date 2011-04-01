/* poppler-qt.h: qt interface to poppler
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

#ifndef __POPPLER_QT_H__
#define __POPPLER_QT_H__

#include <qcstring.h>
#include <qdatetime.h>
#include <qpixmap.h>
#include <qvaluelist.h>
#include <qrect.h>
#include <qpoint.h>

#include <poppler-page-transition.h>

class SplashOutputDev;
class Links;
class QPixmap;

namespace Poppler {

class Document;
class Page;

/* A rectangle on a page, with coordinates in PDF points. */
class Rectangle
{
  public:
    Rectangle(double x1 = 0, double y1 = 0, double x2 = 0, double y2 = 0) : 
      m_x1(x1), m_y1(y1), m_x2(x2), m_y2(y2) {}
    bool isNull() const { return m_x1 == 0 && m_y1 == 0 && m_x2 == 0 && m_y2 == 0; }
  
    double m_x1;
    double m_y1;
    double m_x2;
    double m_y2;
};

#if 0
class TextBox
{
 public:
    TextBox(const QString& text, const Rectangle &bBox) :
    m_text(text), m_bBox(bBox) {};

    QString getText() const { return m_text; };
    Rectangle getBoundingBox() const { return m_bBox; };

  private:
    QString m_text;
    Rectangle m_bBox;
};
#endif


/**
  Container class for information about a font within a PDF document
*/
class FontInfoData;
class FontInfo {
public:
  enum Type {
    unknown,
    Type1,
    Type1C,
    Type3,
    TrueType,
    CIDType0,
    CIDType0C,
    CIDTrueType
  };

  /**
    Create a new font information container
  */
  FontInfo( const QString &fontName, const bool isEmbedded,
            const bool isSubset, Type type );

  FontInfo();
  
  FontInfo( const FontInfo &fi );

  ~FontInfo();

  /**
    The name of the font. Can be QString::null if the font has no name
  */
  const QString &name() const;

  /**
    Whether the font is embedded in the file, or not

    \return true if the font is embedded
  */
  bool isEmbedded() const;

  /**
    Whether the font provided is only a subset of the full
    font or not. This only has meaning if the font is embedded.

    \return true if the font is only a subset
  */
  bool isSubset() const;

  /**
    The type of font encoding
  */
  Type type() const;

  const QString &typeName() const;

private:
  FontInfoData *data;
};

class Links;
class PageData;
class Page {
  friend class Document;
  public:
    ~Page();
    void renderToPixmap(QPixmap **q, int x, int y, int w, int h, double xres, double yres);
    void renderToImageScaled(QImage **q, int hDpi, int vDpi, bool (*abortCheckCbk)(void *data) = 0,
             void *abortCheckCbkData = 0);

    enum {BufferFormatNormal=0, BufferFormatInterleavedRgb, BufferFormatInterleavedRgba};
    
	void renderToBufferScaled(unsigned char *buffer[], int *num_bytes, int *scan_line, int *w, int *h, int fmt, int hDpi, int vDpi, 
		int (*memoryAllocator)(unsigned char *buffer[], int *scan_line, int w, int h, int fmt, void *ctx),
		void *ctx,
		bool (*abortCheckCbk)(void *data),
		void *abortCheckCbkData);


    void getPagePreview(QImage **q);

    /**
      This is a convenience function that is equivalent to
      renderToPixmap() with xres and yres set to 72.0. We keep it
      only for binary compatibility

      \sa renderToImage()
     */
    void renderToPixmap(QPixmap **q, int x, int y, int w, int h);

    /**
      \brief Render the page to a QImage using the Splash renderer

     This method can be used to render the page to a QImage. It
     uses the "Splash" rendering engine.

     \param xres horizontal resolution of the graphics device,
     in dots per inch (defaults to 72 dpi)

     \param yres vertical resolution of the graphics device, in
     dots per inch (defaults to 72 dpi)

     \returns a QImage of the page.

     \sa renderToPixmap()
    */
    QImage renderToImage(double xres = 72.0, double yres = 72.0);

    /**
     * Returns the size of the page in points
     **/
    QSize pageSize() const;

    /**
    * Returns the text that is inside the Rectangle r
    * If r is a null Rectangle all text of the page is given
    **/
    //QString getText(const Rectangle &r) const;

    //QValueList<TextBox*> textList() const;

    /**
    * Returns the transition of this page
    **/
    PageTransition *getTransition() const;

	enum Orientation {
      Landscape,
      Portrait,
      Seascape,
      UpsideDown
    };

    /**
    *  The orientation of the page
    **/
    Orientation orientation() const;

    bool isLink(int x, int y);
    int linkDest(double x, double y) const;
    Poppler::Links getLinks();
    int linkId(double x, double y) const;
    QPoint translatePoint(int x1, int y1) const;

  private:
    SplashOutputDev *output_dev;
    Page(const Document *doc, int index);
    PageData *data;

    ::Links *links;
    void setupLinks();
};

class DocumentData;

class Document {
  friend class Page;
  
public:
  enum PageMode {
    UseNone,
    UseOutlines,
    UseThumbs,
    FullScreen,
    UseOC
  };
  
  static Document *load(const QString & filePath);
  
  Page *getPage(int index) const{ return new Page(this, index); }
  
  int getNumPages() const;
  
  PageMode getPageMode() const;
  
  bool unlock(const QCString &password);
  
  bool isLocked() const; 
  
  QDateTime getDate( const QString & data ) const;
  QString getInfo( const QString & data ) const;
  bool isEncrypted() const;
  bool isLinearized() const;
  bool okToPrint() const;
  bool okToChange() const;
  bool okToCopy() const;
  bool okToAddNotes() const;
  double getPDFVersion() const;

  /**
    The fonts within the PDF document.

    \note this can take a very long time to run with a large
    document. You may wish to use the call below if you have more
    than say 20 pages
  */
  QValueList<FontInfo> fonts() const;

  /**
    \overload

    \param numPages the number of pages to scan
    \param fontList pointer to the list where the font information
    should be placed

    \return false if the end of the document has been reached
  */
  bool scanForFonts( int numPages, QValueList<FontInfo> *fontList ) const;

  ~Document();
  
private:
  DocumentData *data;
  Document(DocumentData *dataA);
};

class Link {

  public:
    Link() : _id(-1) {};
    Link(int x1, int y1, int x2, int y2, int id) :
      _x1(x1), _y1(y1), _x2(x2), _y2(y2), _id(id) {};

    QRect getScreenCoords() const
    {
      return QRect(QPoint(_x1, _y1), QPoint( _x2, _y2)).normalize();
    }

    int getId() { return _id; };

    QPoint getLinkCenterScreenCoord() const
    {
      return QPoint(static_cast<int>(_x1 + (_x2 - _x1) / (float)2),
                    static_cast<int>(_y1 + (_y2 - _y1) / (float)2));
    }

  private:
    int _x1, _y1, _x2, _y2;
    int _id;
};

class Links {

  public:

    void addLink(int x1, int y1, int x2, int y2, int id) {
      _links.append(Link(x1, y1, x2, y2, id));
    }

    int getNumLinks() { return _links.count(); }

    Link getLink(int i) {
      if (i >= 0 && i < getNumLinks()) {
        return _links[i];
      }
      else {
        return Link();
      }
    }

  private:
    QValueList<Poppler::Link> _links;
};

}
#endif
