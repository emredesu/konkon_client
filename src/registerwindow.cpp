#include "registerwindow.h"

RegisterWindow::RegisterWindow(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	ui.password_textbox->setEchoMode(QLineEdit::Password);
	ui.password_repeat_textbox->setEchoMode(QLineEdit::Password);
}

RegisterWindow::~RegisterWindow()
{
}

void RegisterWindow::on_RegisterButtonClicked() {
	using Globals::UI::show_popup_window;

	if (ui.password_textbox->text() != ui.password_repeat_textbox->text())
		show_popup_window("Entered passwords do not match!");
	else if (ui.password_textbox->text().length() < 6 || ui.password_textbox->text().length() > 32)
		show_popup_window("Passwords must be longer than 6 characters and shorter than 32 characters.");
	else if (ui.username_textbox->text().length() < 3 || ui.username_textbox->text().length() > 32)
		show_popup_window("Usernames must be longer than 6 characters and shorter than 32 characters.");
	else {
		emit register_requested(ui.username_textbox->text(), ui.password_textbox->text());
	}
}

void RegisterWindow::on_BackToLoginButtonClicked() {
	emit swap_to_login_window();
}