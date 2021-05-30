#include "loginwindow.h"

LoginWindow::LoginWindow(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    ui.password_field->setEchoMode(QLineEdit::Password);

    github_icon = QIcon("sprites/github_logo.png");
    ui.github_button->setIcon(github_icon);
}

void LoginWindow::on_RegisterButtonClick() {
    emit swap_to_register_window();
}

void LoginWindow::on_LoginButtonClick() {
    emit login_requested(ui.username_field->text(), ui.password_field->text());
}

void LoginWindow::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) { 
        case Qt::Key_Enter: // Make "return (enter)" and "numpad enter" keys do the same thing as pressing the "login" button.
        case Qt::Key_Return:
            emit login_requested(ui.username_field->text(), ui.password_field->text());
            break;
    }
}

void LoginWindow::github_button_clicked() {
    bool is_successful_client = QDesktopServices::openUrl(QUrl("https://github.com/emredesu/konkon_client"));
    bool is_successful_server = QDesktopServices::openUrl(QUrl("https://github.com/emredesu/konkon_server"));

    if ((is_successful_client && is_successful_server) != true) {
        Globals::UI::show_popup_window("Can't open one or any of the URLs for unknown reasons.");
    }
}