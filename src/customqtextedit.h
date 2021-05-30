#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QKeyEvent> 

// Custom QTextBox class for capturing "Return" and "Numpad Enter" key events.

class CustomQTextEdit : public QTextEdit {
	Q_OBJECT

	signals:
		void enter_pressed();

	public:
		CustomQTextEdit(QWidget*) {}; // Constructor that accepts a parent widget.

		void CustomQTextEdit::keyPressEvent(QKeyEvent* event) {
			if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
				emit enter_pressed();
			}
			else {
				QTextEdit::keyPressEvent(event);
			}
		}
};