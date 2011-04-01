#ifndef STYLE_H
#define STYLE_H

#include <memory>

#include <qpixmap.h>
#include <qwindowsstyle.h>
#include <archos/screen.h>

#include "apixmaphorizontalbar.h"

class QPainter;

class APixmapHorizontalBar;

class AStyle : public QWindowsStyle {
 public:
	static QPixmap loadIcon(const QString& relativePath);

	enum ButtonType {
		ButtonTypeStatusbar, /** A button in the status bar */
		ButtonTypeStandard,  /** A button using the standard button size */
		ButtonTypeGeneric    /** A button with a custom width */
	};

	static QColor statusbarBackgroundColor();

	static AStyle *get();

	APixmapHorizontalBar *backgroundBar() const { return m_backgroundBar.get(); }

	void drawSelection(QPainter *p, const QRect &area) const;
	void drawPanel(QPainter *p, const QRect &area) const;
	void drawPanelMask(QPainter *p, const QRect &area) const;
	int selectionHeight();

	void drawMenuItemBackground(QPainter *painter, const QRect&, APixmapHorizontalBar::Look, bool selected) const;

	void drawButtonBackground(QPainter *painter, const QRect&, ButtonType, bool selected) const;

	QBitmap statusbarButtonMask() const;

	QSize statusbarButtonSize() const;

	QSize standardButtonSize() const;

	int genericButtonHeight() const;

 private:
	static AStyle *s_instance;

	QPixmap m_selection_left,
		m_selection_right,
		m_selection_mid,
		m_panel_tl,
		m_panel_tr,
		m_panel_bl,
		m_panel_br,
		m_panel_top,
		m_panel_bottom,
		m_panel_left,
		m_panel_right,
		m_panel_tl_mask,
		m_panel_tr_mask,
		m_panel_bl_mask,
		m_panel_br_mask,
		m_panel_top_mask,
		m_panel_bottom_mask,
		m_panel_left_mask,
		m_panel_right_mask,
		m_selectedStandardButtonBackground,
		m_unselectedStandardButtonBackground,
		m_selectedStatusbarButtonBackground,
		m_unselectedStatusbarButtonBackground;

	std::auto_ptr<APixmapHorizontalBar> m_backgroundBar;
	std::auto_ptr<APixmapHorizontalBar> m_unselectedButtonBackgroundBar;
	std::auto_ptr<APixmapHorizontalBar> m_selectedButtonBackgroundBar;
	std::auto_ptr<APixmapHorizontalBar> m_menuItemBackgroundBar;

	archos::ScreenMode m_mode;
	AStyle(archos::ScreenMode mode);
};


#endif // STYLE_H
