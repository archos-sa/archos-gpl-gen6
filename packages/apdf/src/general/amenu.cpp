#include "amenu.h"

#include <qpainter.h>
#include <qapplication.h>

// Local
#include "atimeline.h"
#include "style.h"

// LAS
#include <archos/screen.h>
#define ALOG_ENABLE_DEBUG
#include <archos/alog.h>

using namespace archos;

static const QColor MENU_OUTER_BORDER_COLOR = QColor(48, 48, 49);
static const int MENU_MARGIN = 2;
static const int MENU_FONT_SIZE = 22;
static const int MENU_TO_SUBMENU_MARGIN = 4;

static const int MENU_TEXT_TOP_MARGIN = 2;
static const int MENU_TEXT_HMARGIN = 7;

static const int SUBMENU_SEPARATOR_VMARGIN = 12;
static const QColor SUBMENU_SEPARATOR_COLOR = QColor(191, 191, 191);

static const int ANIMATION_DELTA = 32; // From AVOS: MenuWidget_onAnimTimeout
static const int ANIMATION_INTERVAL = 30; // From AVOS: MenuWidget_showNextTo

MenuButton::MenuButton(QWidget *parent, int id, const QString& text)
	: QWidget(parent)
	, m_look(APixmapHorizontalBar::LookWhole)
	, m_showIcon(true)
	, m_isFocused(false)
	, m_id(id)
	{
		setText(text);

		switch (Screen::mode()) {
		case hdmi_720p:
			m_menuHeight = 72;
			m_menuIconWidth = 60;
			break;
		case wvga:
		case tv:
		default:
			m_menuIconWidth = 40;
			m_menuHeight = 54;
			break;
		}
	}

QSize MenuButton::sizeHint() const
{
	QSize sh;
	QFontMetrics fm(font());
	sh.setWidth(fm.width(text()) + MENU_TEXT_HMARGIN * 2);
	if (m_showIcon) {
		// Reserve space for the icon
		sh.rwidth() += MENU_TEXT_HMARGIN + m_menuIconWidth;
	}
	sh.setHeight(m_menuHeight);
	return sh;
}

bool MenuButton::event(QEvent* e)
{
	if ( e->type() == QEvent::MouseButtonPress ||
	     e->type() == QEvent::MouseMove  ||
	     e->type() == QEvent::MouseButtonRelease ) {
		QMouseEvent* me = new QMouseEvent((*(QMouseEvent*)e));
		QApplication::postEvent(parentWidget(), (QEvent*)me);
		return false;
	}
	return QWidget::event(e);
}

void MenuButton::paintEvent(QPaintEvent *evt)
{
	QRect area(evt->rect());
	QPixmap buffer(area.size());
	QPainter p(&buffer, this);
	p.eraseRect(rect());

	drawButton(&p, colorsInverted());

	QPainter sp(this);
	sp.drawPixmap(area.topLeft(), buffer);
}

void MenuButton::drawButton(QPainter* painter, bool inverted)
{
	AStyle::get()->drawMenuItemBackground(painter, rect(), m_look, inverted);

	drawButtonLabel(painter, inverted);

	if (m_look == APixmapHorizontalBar::LookMiddle || m_look == APixmapHorizontalBar::LookRight) {
		// Draw separator
		painter->setPen(SUBMENU_SEPARATOR_COLOR);
		painter->drawLine(
			0, SUBMENU_SEPARATOR_VMARGIN,
			0, height() - SUBMENU_SEPARATOR_VMARGIN
			);
	}
}

void MenuButton::drawButtonLabel(QPainter *painter, bool inverted)
{
	if (m_showIcon && !m_icon.isNull()) {
		int posY = (height() - m_icon.height())  / 2;
		painter->drawPixmap(MENU_TEXT_HMARGIN, posY, m_icon);
	}

	if (isEnabled()) {
		if (inverted) {
			painter->setPen(Qt::black);
		}
		else {
			painter->setPen(Qt::white);
		}
	}
	else {
		painter->setPen(Qt::gray);
	}

	QRect textRect = rect();
	textRect.moveBy(
		MENU_TEXT_HMARGIN,
		MENU_TEXT_TOP_MARGIN);

	if (m_showIcon) {
		textRect.moveBy(
			m_menuIconWidth + MENU_TEXT_HMARGIN,
			0);
	}
	painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text());
}


AMenuBase::AMenuBase(QWidget *parent, QBoxLayout::Direction direction)
	: QWidget(parent, 0, WType_TopLevel | WStyle_Customize | WStyle_NoBorder)
	, m_focusedButton(0)
{
	setBackgroundColor(MENU_OUTER_BORDER_COLOR);
	m_layout = new QBoxLayout(this, direction);

	// We want to hide when an entry is clicked
	connect(this, SIGNAL(entryActivated(int)),
		this, SLOT(hide()) );
}


MenuButton *AMenuBase::findButtonForId(int id) const
{
	QValueList<MenuButton*>::ConstIterator
		it = m_buttonForIdMap.begin(),
		end = m_buttonForIdMap.end();
	for (; it != end; ++it) {
		if ((*it)->id() == id) {
			return *it;
		}
	}

	QValueList<ASubMenu*>::ConstIterator
		sub_it = m_subMenuList.begin(),
		sub_end = m_subMenuList.end();
	for (; sub_it != sub_end; ++sub_it) {
		MenuButton* button = (*sub_it)->findButtonForId(id);
		if (button) {
			return button;
		}
	}
	return 0;
}

void AMenu::hideSubMenus()
{
	QValueList<ASubMenu*>::ConstIterator
		sub_it = m_subMenuList.begin(),
		sub_end = m_subMenuList.end();
	for (; sub_it != sub_end; ++sub_it) {
		(*sub_it)->hide();
	}
}

MenuButton* AMenuBase::addEntry(int id, const QString& text)
{
	MenuButton *button = createEntryButton(text, id);

	m_buttonForIdMap.append(button);
	
	return button;
}

MenuButton* AMenu::addEntry(int id, const QString& text)
{
	MenuButton *button = AMenuBase::addEntry(id, text);
	connect(button, SIGNAL(focused()),
		this, SLOT(hideSubMenus()));
	return button;
}

void AMenuBase::setEntryVisible(int id, bool visible)
{
	MenuButton *button = findButtonForId(id);
	ASSERT(button);
	if (visible) {
		button->show();
	} else {
		button->hide();
	}
	m_layout->activate();
	resize(m_layout->minimumSize());
}


void AMenuBase::setEntryIcon(int id, const QString& iconName)
{
	MenuButton *button = findButtonForId(id);
	ASSERT(button);
	QPixmap icon = AStyle::loadIcon(iconName);
	button->setIcon(icon);
}


void AMenuBase::setEntryEnabled(int id, bool enabled)
{
	MenuButton *button = findButtonForId(id);
	ASSERT(button);
	button->setEnabled(enabled);
}

void AMenuBase::setEntryFocused(int id, bool focused)
{
	MenuButton *button = findButtonForId(id);
	ASSERT(button);
	button->setFocused(focused);
}

MenuButton *AMenuBase::mapPointToButton(const QPoint &p)
{
	ButtonForIdMap::ConstIterator it = m_buttonForIdMap.begin();
	MenuButton *ret = 0;
	while ( it != m_buttonForIdMap.end() ) {
		MenuButton* tmp = *it;

		QRect r(tmp->rect());
		QPoint mp(tmp->mapToGlobal(QPoint(r.x(), r.y())));
		QRect mr(mp.x(), mp.y(), r.width(), r.height());

		if ( mr.contains(p) ) {
			ret = *it;
			break;
		}
		++it;
	}
	return ret;
}

void AMenuBase::setupButton(MenuButton* button)
{
	QFont font = button->font();
	font.setPixelSize(MENU_FONT_SIZE);
	button->setFont(font);
	m_layout->addWidget(button);
}

MenuButton *AMenuBase::createEntryButton(const QString& text, int id)
{
	MenuButton *button = new MenuButton(this, id, text);
	setupButton(button);	
	return button;
}

MenuButton *AMenuBase::createSubEntryButton(const QString& text, int id)
{
	MenuButton *button = new SubMenuButton(this, id, text);
	setupButton(button);	
	return button;
}

ASubMenu *AMenuBase::addSubMenu(int id, const QString& text)
{
	MenuButton *button = createSubEntryButton(text, id);
	
	ASubMenu *subMenu = new ASubMenu(this, button);
	subMenu->hide();
	connect(button, SIGNAL(focused()),
		subMenu, SLOT(show()) );

	m_subMenuList.append(subMenu);
	m_buttonForIdMap.append(button);
	
	return subMenu;
}


void AMenuBase::hide()
{
	emit aboutToHide();
	QWidget::hide();
}


void AMenuBase::mousePressEvent(QMouseEvent *e)
{
	MenuButton* m = mapPointToButton(e->globalPos());
	if ( m ) {
		if ( m_focusedButton != 0 ) {
			m_focusedButton->setFocused(false);
		}
		m->setFocused(true);
		m_focusedButton = m;
	}
}

void AMenuBase::mouseMoveEvent(QMouseEvent *e)
{
	MenuButton* m = mapPointToButton(e->globalPos());
	if ( m && m != m_focusedButton ) {
		if ( m_focusedButton != 0 ) {
			m_focusedButton->setFocused(false);
		}
		m->setFocused(true);
		m_focusedButton = m;
	}
}

void AMenuBase::mouseReleaseEvent(QMouseEvent *e)
{
	MenuButton* m = mapPointToButton(e->globalPos());
	if ( m ) {
		emit entryActivated(m->id());
	}
}

ButtonForIdMap AMenuBase::visibleEntries()
{
	ButtonForIdMap result;
	ButtonForIdMap::ConstIterator
		it = m_buttonForIdMap.begin(),
		end = m_buttonForIdMap.end();
	
	while ( it != end ) {
		if ( (*it)->isVisible() ) {
			result.append(*it);
		}
		++it;
	}

	return result;
}

void AMenuBase::focusNext()
{
	ButtonForIdMap vEntries = visibleEntries();

	if ( m_focusedButton == 0 ) {
		m_focusedButton = *vEntries.begin();
		m_focusedButton->setFocused(true);
		return;
	}

	ButtonForIdMap::ConstIterator
		it = vEntries.begin(),
		end = vEntries.end();
	
	while ( it != end ) {
		MenuButton* tmp = *it;
		++it;
		if ( tmp == m_focusedButton ) {
			m_focusedButton->setFocused(false);
			if ( it != end ) {
				m_focusedButton = *it;
			}
			else {
				m_focusedButton = *(vEntries.begin());
			}
			m_focusedButton->setFocused(true);
			break;
		}
	}
}


void AMenuBase::focusPrev()
{
	ButtonForIdMap vEntries = visibleEntries();

	if ( m_focusedButton == 0 ) {
		m_focusedButton = *vEntries.begin();
		m_focusedButton->setFocused(true);
		return;
	}
	
	int cnt = vEntries.count();
	while ( cnt > 0 ) {
		MenuButton* tmp = vEntries[cnt - 1];
		--cnt;
		if ( tmp == m_focusedButton ) {
			m_focusedButton->setFocused(false);
			if ( cnt != 0 ) {
				m_focusedButton = vEntries[cnt -1];
			}
			else {
				m_focusedButton = vEntries[vEntries.count() -1];
			}
			m_focusedButton->setFocused(true);
			break;
		}
	}
}

void AMenuBase::focusFirstEntry()
{
	ASSERT(!m_focusedButton);
	ButtonForIdMap vEntries = visibleEntries();
	if ( vEntries.begin() != vEntries.end() ) {
		m_focusedButton = *vEntries.begin();
		if ( m_focusedButton ) {
			m_focusedButton->setFocused(true);
		}
	}
}


//// AMenu ////
AMenu::AMenu(QWidget *parent)
: AMenuBase(parent, QBoxLayout::TopToBottom)
, m_timeLine(new ATimeLine(this))
{
	//m_timeLine->setUpdateInterval(ANIMATION_INTERVAL);
	//connect(m_timeLine, SIGNAL(valueChanged(double)),
	//	SLOT(onTimeLineChanged(double)) );
	// FIXME: animation

	m_layout->setMargin(MENU_MARGIN);
	m_layout->setSpacing(MENU_MARGIN);
}


void AMenu::setFinalPosition(const QPoint& position)
{
	m_finalPosition = position;
}

void AMenu::slideIn()
{
	// AVOS Menu uses a fixed pixel delta...
	//m_timeLine->setDuration(ANIMATION_INTERVAL * width() / ANIMATION_DELTA);
	//m_timeLine->start();
	// FIXME: animation
	show();
	if ( !m_focusedButton ) {
		focusFirstEntry();
	}
}


void AMenu::show()
{
	move(m_finalPosition);
	AMenuBase::show();
	// make sure the focused signal gets emited again
	if ( m_focusedButton ) {
		m_focusedButton->setFocused(true);
	}
}

void AMenu::keyReleaseEvent(QKeyEvent *e)
{
	switch(e->key()) {
		case Key_Up:
			focusPrev();
			break;
		case Key_Down:
			focusNext();
			break;
		case Key_Left:
		case Key_Right:
		case Key_PageDown:
		case Key_PageUp:
			e->ignore();
			break;
		case Key_Enter:
		case Key_Return:
			emit entryActivated(m_focusedButton->id());
			break;
		case Key_F1:
			if ( isVisible() ) {
				hide();
			}
			else {
				slideIn();
			}
			break;
		case Key_F2:
			if ( isVisible() ) {
				hide();
			}
			break;
		default:
			break;
	}
}

void AMenu::onTimeLineChanged(double value)
{
	QPoint pos = m_finalPosition;
	pos.rx() += int( width() * (1. - value) );
	move(pos);

	if (!isVisible()) {
		AMenuBase::show();
	}
}

//// ASubMenu ////
ASubMenu::ASubMenu(AMenuBase* parent, MenuButton* parentButton)
: AMenuBase(parent, QBoxLayout::LeftToRight)
, m_parentButton(parentButton)
{
	m_layout->setMargin(MENU_MARGIN);

	// Forward entryActivated() signal to toplevel menu
	connect(this, SIGNAL(entryActivated(int)),
		parent, SIGNAL(entryActivated(int)) );
	
	// Hide when our parent hide itself
	connect(parent, SIGNAL(aboutToHide()),
		this, SLOT(hide()) );
}

void ASubMenu::keyReleaseEvent(QKeyEvent *e)
{
	switch(e->key()) {
		case Key_Left:
			focusPrev();
			break;
		case Key_Right:
			focusNext();
			break;
		case Key_Up:
		case Key_Down: {
			// workaround. event wasn't propagated to the parent on ignore()
			QKeyEvent* ke = new QKeyEvent((*e));
			QApplication::postEvent(parentWidget(), (QEvent*)ke);
			hide();
			break;
		}
		case Key_Enter:
		case Key_Return:
			emit entryActivated(m_focusedButton->id());
			break;
		case Key_F1:
			if ( isVisible() ) {
				parentWidget()->hide();
			}
			break;
		default:
			break;
	}
}


void ASubMenu::show()
{
	resize(sizeHint());
	QPoint pos = m_parentButton->mapToGlobal(QPoint(0, 0));
	pos.rx() -= MENU_MARGIN;
	pos.ry() -= MENU_MARGIN;
	pos.rx() -= width() + MENU_TO_SUBMENU_MARGIN;
	move(pos);

	if ( m_focusedButton ) {
		m_focusedButton->setFocused(true);
	}
	else {
		focusFirstEntry();
	}
	AMenuBase::show();
}


MenuButton* ASubMenu::addEntry(int id, const QString& text)
{
	MenuButton *button = AMenuBase::addEntry(id, text);
	ASSERT(button);
	button->setShowIcon(false);
	updateEntryButtonPositions();
	return button;
}

void ASubMenu::updateEntryButtonPositions()
{
	QLayoutIterator it = m_layout->iterator();
	MenuButton *firstButton = static_cast<MenuButton*>(it.current()->widget());
	MenuButton *lastButton = 0;
	for (; it.current(); ++it) {
		QLayoutItem *item = it.current();
		MenuButton *button = static_cast<MenuButton*>(item->widget());
		if (button) {
			button->setLook(APixmapHorizontalBar::LookMiddle);
			lastButton = button;
		}
	}

	ASSERT(firstButton);
	ASSERT(lastButton);
	if (firstButton == lastButton) {
		firstButton->setLook(APixmapHorizontalBar::LookWhole);
	} else {
		firstButton->setLook(APixmapHorizontalBar::LookLeft);
		lastButton->setLook(APixmapHorizontalBar::LookRight);
	}
}


void ASubMenu::setIcon(const QString& iconName)
{
	QPixmap icon = AStyle::loadIcon(iconName);
	m_parentButton->setIcon(icon);
}
