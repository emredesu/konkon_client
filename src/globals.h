#pragma once

#include <iostream> // todo temp
#include <string>
#include <ctime>
#include <QMessageBox>
#include <QApplication>
#include <QString> 

// Functions that will be used across the entire app will be placed in this namespace.
namespace Globals {
	QString generate_message(QString sent_at, QString sent_by, QString message_content);

	namespace UI {
		int show_popup_window(QString string, QString window_name = "Warning!", QMessageBox::Icon icon = QMessageBox::Warning, QString custom_button_text = "OK"); // Returns the button clicked.
		int show_popup_question_window(QWidget* parent, QString window_title, QString window_text, bool do_beep);
	}

	namespace time {
		std::string unix_time_to_readable_string(unsigned long long unix_time);
		std::string current_time_as_readable_string();
	}
}
