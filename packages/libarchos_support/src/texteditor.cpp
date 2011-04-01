#include "texteditor.h"
#include "texteditor_internal.h"
#include <qlayout.h>


archos::TextEditor::TextEditor(QWidget *parent)
	: QWidget( parent )
	, m_editw( new TextEditorInternal( this ) )
{

	QVBoxLayout *l = new QVBoxLayout( this );
	l->addWidget( m_editw );

	connect(
		m_editw, SIGNAL( editingFinished() ),
		this, SIGNAL( editingFinished() )
	);
}

void archos::TextEditor::setText( const QString &txt )
{
	( (TextEditorInternal*) m_editw )->setText( txt );
}

QString archos::TextEditor::text() const
{
	return ( (TextEditorInternal*) m_editw )->text();
}
