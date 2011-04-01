#include "texteditor_internal.h"
#include "keyboardwidget.h"
#include <qapplication.h>
#include <qlayout.h>
#include <qmultilineedit.h>
#include <stdio.h>


archos::TextEditorInternal::TextEditorInternal( QWidget *parent )
	: QWidget( parent )
	, kbdw( NULL )
	, editw( NULL )
{
	QLayout *l = new QVBoxLayout( this );
	l->setAutoAdd( true );

	editw = new QMultiLineEdit( this );
	editw->setFrameStyle( QFrame::NoFrame );

	QPalette pal = editw->palette();
	pal.setColor( QColorGroup::Background, QColor( 0, 86, 194 ) );
	editw->setPalette( pal );

	kbdw = new KeyboardWidget( this );
	kbdw->setPalette( pal );
	kbdw->setReceiver( editw );

	qApp->installEventFilter( kbdw );
	editw->setFocus();

	connect(
		kbdw, SIGNAL( enterPressed() ),
		this, SIGNAL( editingFinished() )
	);
}

void archos::TextEditorInternal::setText( const QString &txt )
{
	editw->setText( txt );
}

QString archos::TextEditorInternal::text() const
{
	return editw->text();
}
