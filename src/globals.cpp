#include "globals.h"

int Globals::UI::show_popup_window(QString string, QString window_name, QMessageBox::Icon icon, QString custom_button_text) {
	QMessageBox message_box;
	message_box.setWindowTitle(window_name);
	message_box.setIcon(icon);
	message_box.setText(string);
	message_box.setButtonText(QMessageBox::Ok, custom_button_text);
	QApplication::beep(); // Play the default notification sound of the current OS.
	return message_box.exec();
}

int Globals::UI::show_popup_question_window(QWidget* parent, QString window_title, QString window_text, bool do_beep) {
	int reply;

	if (do_beep)
		QApplication::beep();

	reply = QMessageBox::question(parent, window_title, window_text, QMessageBox::Yes, QMessageBox::No);
	return reply;
}

std::string Globals::time::unix_time_to_readable_string(unsigned long long unix_time) {
	std::time_t time = unix_time;
	std::tm* time_tm_structure = std::localtime(&time);
	char buffer[100];
	std::strftime(buffer, sizeof(buffer), "%d/%m/%Y %T", time_tm_structure);
	return std::string(buffer);
}

QString Globals::generate_message(QString sent_at, QString sent_by, QString message_content) {
	return sent_at + " " + sent_by + ": " + message_content;
}

std::string Globals::time::current_time_as_readable_string() {
	std::time_t time_now = std::time(nullptr);
	std::tm* time_tm_structure = std::localtime(&time_now);
	char buffer[100];
	std::strftime(buffer, sizeof(buffer), "%d/%m/%Y %T", time_tm_structure);
	return std::string(buffer);
}