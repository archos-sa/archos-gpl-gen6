#ifndef AMENU_H
#define AMENU_H

#include <qlayout.h>
#include <qlist.h>
#include <qwidget.h>

#include "style.h"

class ASubMenu;
class ATimeLine;

/**
 * A button which looks like an AVOS menu entry
 */
class MenuButton : public QWidget {
	Q_OBJECT

public:
	MenuButton(QWidget *parent, int id, const QString& text);
	
	int id() const
	{
		return m_id;
	}

	void setLook(APixmapHorizontalBar::Look look)
	{
		m_look = look;
	}

	void setShowIcon(bool value)
	{
		m_showIcon = value;
	}

	/**
	 * We do not use setPixmap() because it removes the text
	 */
	void setIcon(const QPixmap& icon)
	{
		m_icon = icon;
	}

	virtual QSize minimumSizeHint() const
	{
		return sizeHint();
	}

	virtual void setText(const QString &t)
	{
		m_text = t;
	}

	QString text() const
	{
		return m_text;
	}
	
	virtual void setFocused(bool f)
	{
		emit focused();
		if ( m_isFocused != f ) {
			m_isFocused = f;
			update();
		}
	}

	bool isDown() const
	{
		return m_isFocused;
	}
	
	virtual QSize sizeHint() const;

protected:
	virtual bool event(QEvent* e);
	virtual void paintEvent(QPaintEvent *evt);
	virtual void drawButton(QPainter* painter, bool inverted);
	virtual void drawButtonLabel(QPainter *painter, bool inverted);

	virtual bool colorsInverted()
	{
		return isDown();
	}

signals:
	void focused();
	
private:
	APixmapHorizontalBar::Look m_look;
	bool m_showIcon;
	QPixmap m_icon;
	QString m_text;
	bool m_isFocused;
	int m_id;
	int m_menuHeight;
	int m_menuIconWidth;
};


class SubMenuButton : public MenuButton {
	Q_OBJECT

public:
	SubMenuButton(QWidget *parent, int id, const QString& text)
	: MenuButton(parent, id, text)
	{
	}
	
signals:
	void focused();
	
protected:
	virtual bool colorsInverted()
	{
		return false;
	}

};

typedef QValueList<MenuButton*> ButtonForIdMap;

/**
 * Common menu class, inherited by AMenu and ASubMenu
 */
class AMenuBase : public QWidget {
	Q_OBJECT

	public:
		AMenuBase(QWidget*, QBoxLayout::Direction);

		virtual MenuButton* addEntry(int id, const QString& text);
		ASubMenu *addSubMenu(int id, const QString& text);

		void setEntryVisible(int id, bool visible);
		void setEntryEnabled(int id, bool enabled);
		void setEntryFocused(int id, bool focused);
		void setEntryIcon(int id, const QString& iconName);
	
	public slots:
		virtual void hide();

	signals:
		void entryActivated(int id);
		void aboutToHide();

	protected:
		QBoxLayout *m_layout;
		MenuButton *findButtonForId(int id) const;
		virtual void mouseMoveEvent(QMouseEvent *e);
		virtual void mousePressEvent(QMouseEvent *e);
		virtual void mouseReleaseEvent(QMouseEvent *e);
		virtual MenuButton *createEntryButton(const QString& text, int id);
		void setupButton(MenuButton* button);
		MenuButton *createSubEntryButton(const QString& text, int id);
		virtual MenuButton *mapPointToButton(const QPoint &p);
		
		virtual void focusNext();
		virtual void focusPrev();
		virtual ButtonForIdMap visibleEntries();
		virtual void focusFirstEntry();
		
		MenuButton* m_focusedButton;
		QValueList<ASubMenu*> m_subMenuList;

	private:
		ButtonForIdMap m_buttonForIdMap;
};


/**
 * The toplevel menu
 */
class AMenu : public AMenuBase {
	Q_OBJECT
	public:
		AMenu(QWidget *parent);
		virtual void show();

		void slideIn();

		void setFinalPosition(const QPoint& pos);
		virtual MenuButton* addEntry(int id, const QString& text);

	public slots:
		virtual void hideSubMenus();

	protected:
		virtual void keyReleaseEvent(QKeyEvent *e);

	private slots:
		void onTimeLineChanged(double);

	private:
		ATimeLine* m_timeLine;
		QPoint m_finalPosition;
};


/**
 * The submenu inside a menu
 */
class ASubMenu : public AMenuBase {
	Q_OBJECT
	public:
		ASubMenu(AMenuBase *parent, MenuButton *parentButton);

		virtual MenuButton* addEntry(int id, const QString& text);

		virtual void show();

		void setIcon(const QString& iconName);
		
	protected:
		virtual void keyReleaseEvent(QKeyEvent *e);

	private:
		void updateEntryButtonPositions();
		MenuButton *m_parentButton;
};

#endif /* AMENU_H */
