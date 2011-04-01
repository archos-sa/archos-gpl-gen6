#ifndef TEXTEDITOR_H
#define TEXTEDITOR_H

#include <qwidget.h>


namespace archos {

class TextEditor : public QWidget {
	Q_OBJECT

 public:
	TextEditor( QWidget *parent );

	QString text() const;
	void    setText( const QString &txt );

 signals:
	void    editingFinished();

 private:
	QWidget *m_editw;
};

}


#endif // TEXTEDITOR_H
