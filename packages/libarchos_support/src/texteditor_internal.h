#ifndef TEXTEDITOR_INTERNAL_H
#define TEXTEDITOR_INTERNAL_H

#include "keyboardwidget.h"
#include <qwidget.h>

class QMultiLineEdit;


namespace archos {

class TextEditorInternal : public QWidget {
	Q_OBJECT

 public:
	TextEditorInternal( QWidget *parent );

	QString text() const;
	void    setText( const QString &txt );

 signals:
	void    editingFinished();

 private:
	KeyboardWidget *kbdw;
	QMultiLineEdit *editw;
};

}


#endif // TEXTEDITOR_INTERNAL_H
