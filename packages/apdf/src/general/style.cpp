#include "style.h"

#include <qbitmap.h>
#include <qpainter.h>

#include "apixmaphorizontalbar.h"

#include <archos/alog.h>

using namespace archos;

#ifndef PREFIX
#error "PREFIX not defined"
#endif
static const char* ICON_PATH = PREFIX "/share/apdf/icons/";

static const QColor STATUSBAR_BACKGROUND_COLOR = Qt::black;

static QBitmap loadBitmap(const QString& relativePath)
{
	QBitmap bitmap;
	QString path = ICON_PATH + Screen::prefix() + relativePath;
	if (!bitmap.load(path)) {
		ALOG_WARNING("Could not load bitmap %s", path.ascii());
	}
	return bitmap;
}

QPixmap AStyle::loadIcon(const QString& relativePath)
{
	QPixmap pixmap;
	QString path = ICON_PATH + Screen::prefix() + relativePath;
	if (!pixmap.load(path)) {
		ALOG_WARNING("Could not load pixmap %s", path.ascii());
	}
	return pixmap;
}

AStyle *AStyle::get()
{
	if(s_instance == NULL)
		s_instance = new AStyle(Screen::mode());
	else if (s_instance->m_mode != Screen::mode()) {
		delete s_instance;
		s_instance = new AStyle(Screen::mode());
	}
	return s_instance;
}

AStyle::AStyle(ScreenMode mode)
	: QWindowsStyle()
	, m_mode(mode)
{
	m_selection_left = loadIcon("general/selection_left.png");
	m_selection_right = loadIcon("general/selection_right.png");
	m_selection_mid = loadIcon("general/selection_mid.png");

	m_panel_tl = loadIcon("general/box_top_left.png");
	m_panel_tr = loadIcon("general/box_top_right.png");
	m_panel_bl = loadIcon("general/box_bottom_left.png");
	m_panel_br = loadIcon("general/box_bottom_right.png");
	m_panel_top = loadIcon("general/box_top.png");
	m_panel_bottom = loadIcon("general/box_bottom.png");
	m_panel_left = loadIcon("general/box_left.png");
	m_panel_right = loadIcon("general/box_right.png");

	m_panel_tl_mask = loadIcon("general/box_top_left_mask.png");
	m_panel_tr_mask = loadIcon("general/box_top_right_mask.png");
	m_panel_bl_mask = loadIcon("general/box_bottom_left_mask.png");
	m_panel_br_mask = loadIcon("general/box_bottom_right_mask.png");
	m_panel_top_mask = loadIcon("general/box_top_mask.png");
	m_panel_bottom_mask = loadIcon("general/box_bottom_mask.png");
	m_panel_left_mask = loadIcon("general/box_left_mask.png");
	m_panel_right_mask = loadIcon("general/box_right_mask.png");

	m_backgroundBar.reset(new APixmapHorizontalBar);
	m_backgroundBar->loadPixmaps("general/hbar_backgnd%1.png");

	m_unselectedButtonBackgroundBar.reset(new APixmapHorizontalBar);
	m_unselectedButtonBackgroundBar->loadPixmaps("general/button_generic_unselected%1.png");

	m_selectedButtonBackgroundBar.reset(new APixmapHorizontalBar);
	m_selectedButtonBackgroundBar->loadPixmaps("general/button_generic_selected%1.png");

	m_unselectedStandardButtonBackground = loadIcon("general/button_standard_unselected.png");
	m_selectedStandardButtonBackground = loadIcon("general/button_standard_selected.png");

	m_unselectedStatusbarButtonBackground = loadIcon("general/button_statusbar_unselected.png");
	m_selectedStatusbarButtonBackground = loadIcon("general/button_statusbar_selected.png");

	m_menuItemBackgroundBar.reset(new APixmapHorizontalBar);
	m_menuItemBackgroundBar->loadPixmaps("general/menu%1.png", "_mid");
}

void AStyle::drawSelection(QPainter *p, const QRect &r) const
{
	int x, y, w, h;
	r.rect(&x, &y, &w, &h);
	int tw= m_selection_left.width();

	p->drawPixmap(x, y, m_selection_left);
	p->drawPixmap(x + w - tw, y, m_selection_right);
	p->drawTiledPixmap(QRect(x + tw, y, w - 2* tw, m_selection_left.height()), m_selection_mid);
}

void AStyle::drawButtonBackground(QPainter *painter, const QRect& rect, AStyle::ButtonType buttonType, bool selected) const
{
	if (buttonType != ButtonTypeGeneric) {
		QPixmap pix;
		if (buttonType == ButtonTypeStandard) {
			pix = selected ? m_selectedStandardButtonBackground : m_unselectedStandardButtonBackground;
		} else {
			// Statusbar
			pix = selected ? m_selectedStatusbarButtonBackground : m_unselectedStatusbarButtonBackground;
			painter->fillRect(rect, STATUSBAR_BACKGROUND_COLOR);
		}
		painter->drawPixmap(rect.topLeft(), pix);
		return;
	}

	APixmapHorizontalBar *bar = selected ? m_selectedButtonBackgroundBar.get() : m_unselectedButtonBackgroundBar.get();
	bar->draw(painter, rect.x(), rect.y(), rect.width());
}

QColor AStyle::statusbarBackgroundColor()
{
	return STATUSBAR_BACKGROUND_COLOR;
}

QSize AStyle::statusbarButtonSize() const
{
	return m_unselectedStatusbarButtonBackground.size();
}

QBitmap AStyle::statusbarButtonMask() const
{
	return loadBitmap("general/button_statusbar_mask.png");
}

QSize AStyle::standardButtonSize() const
{
	return m_unselectedStandardButtonBackground.size();
}

int AStyle::genericButtonHeight() const
{
	return m_unselectedButtonBackgroundBar->height();
}

int AStyle::selectionHeight()
{
        return m_selection_mid.height();
}

void AStyle::drawPanel(QPainter *p, const QRect &area) const
{
	int x, y, w, h;
	area.rect(&x, &y, &w, &h);

	int lw= m_panel_left.width();
	int th= m_panel_top.height();
	int rw= m_panel_right.width();
	int bh= m_panel_bottom.height();

	// fill panel
	p->eraseRect(QRect(x + lw, y, w - lw - rw, h) );
	p->eraseRect(QRect(x, y + th, w, h - th - bh) );

	// draw corners
	p->drawPixmap(x, y, m_panel_tl);
	p->drawPixmap(x + w - rw, y, m_panel_tr);
	p->drawPixmap(x, y + h - bh, m_panel_bl);
	p->drawPixmap(x + w - rw, y + h - bh, m_panel_br);

	// draw edges
	p->drawTiledPixmap(
		x + m_panel_tl.width(), 
		y, 
		w - m_panel_tl.width() -  m_panel_tr.width(), 
		m_panel_top.height(), m_panel_top
	);
	p->drawTiledPixmap(
		x + m_panel_bl.width(), 
		y + h - m_panel_bottom.height(), 
		w - m_panel_bl.width() - m_panel_br.width(),
		m_panel_bottom.height(), m_panel_bottom
	);
	p->drawTiledPixmap(
		x + w - m_panel_tr.width(),
		y + m_panel_tr.height(),
		m_panel_right.width(),
		h - m_panel_tr.height() - m_panel_br.height(), m_panel_right
	);
	p->drawTiledPixmap(
		x,
		y +  + m_panel_tl.height(),
		m_panel_left.width(),
		h - m_panel_tl.height() - m_panel_bl.height(), m_panel_left
	);
}

void AStyle::drawPanelMask(QPainter *p, const QRect &area) const
{
	int x, y, w, h;
	area.rect(&x, &y, &w, &h);

	int lw= m_panel_left_mask.width();
	int th= m_panel_top_mask.height();
	int rw= m_panel_right_mask.width();
	int bh= m_panel_bottom_mask.height();

	// fill panel
	p->fillRect(QRect(x + lw, y, w - lw - rw, h), QColor(255, 255, 255));
	p->fillRect(QRect(x, y + th, w, h - th - bh), QColor(255, 255, 255));

	// draw corners
	p->drawPixmap(x, y, m_panel_tl_mask);
	p->drawPixmap(x + w - rw, y, m_panel_tr_mask);
	p->drawPixmap(x, y + h - bh, m_panel_bl_mask);
	p->drawPixmap(x + w - rw, y + h - bh, m_panel_br_mask);

	// draw edges
	p->drawTiledPixmap(
		x + m_panel_tl_mask.width(), 
		y, 
		w - m_panel_tl_mask.width() -  m_panel_tr_mask.width(), 
		m_panel_top_mask.height(), m_panel_top_mask
	);
	p->drawTiledPixmap(
		x + m_panel_bl_mask.width(), 
		y + h - m_panel_bottom_mask.height(), 
		w - m_panel_bl_mask.width() - m_panel_br_mask.width(),
		m_panel_bottom_mask.height(), m_panel_bottom_mask
	);
	p->drawTiledPixmap(
		x + w - m_panel_tr_mask.width(),
		y + m_panel_tr_mask.height(),
		m_panel_right_mask.width(),
		h - m_panel_tr_mask.height() - m_panel_br_mask.height(), m_panel_right_mask
	);
	p->drawTiledPixmap(
		x,
		y +  + m_panel_tl_mask.height(),
		m_panel_left_mask.width(),
		h - m_panel_tl_mask.height() - m_panel_bl_mask.height(), m_panel_left_mask
	);
}

void AStyle::drawMenuItemBackground(QPainter *painter, const QRect& rect, APixmapHorizontalBar::Look look, bool selected) const
{
	m_menuItemBackgroundBar->draw(painter, rect.x(), rect.y(), rect.width(), look);

	if (selected) {
		int selectRectMargin = look == APixmapHorizontalBar::LookWhole ? 2 : 6;
		QRect selectRect = rect;
		selectRect.rLeft() += selectRectMargin;
		selectRect.rTop() += selectRectMargin;
		selectRect.rRight() -= selectRectMargin;
		selectRect.rBottom() -= selectRectMargin;
		painter->fillRect(selectRect, Qt::white);
	}
}


AStyle *AStyle::s_instance= NULL;
