#pragma once

#include <QtWidgets/QWidget>
#include <QKeyEvent>
#include <QIcon>
#include <QDesktopServices>
#include <QUrl>
#include "globals.h"
#include "ui_loginwindow.h"

class LoginWindow : public QWidget
{
    Q_OBJECT

public:
    virtual void keyPressEvent(QKeyEvent* event);
    LoginWindow(QWidget *parent = Q_NULLPTR);

signals:
    void swap_to_register_window();
    void login_requested(QString username, QString password);

public slots:
    void on_RegisterButtonClick(); // qt slot naming convention
    void on_LoginButtonClick();
    void github_button_clicked();

private:
    Ui::LoginWindowClass ui;

    QIcon github_icon;
};
