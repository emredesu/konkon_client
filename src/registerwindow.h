#pragma once

#include <QWidget>
#include "globals.h"
#include "ui_registerwindow.h"

class RegisterWindow : public QWidget
{
	Q_OBJECT

public:
	RegisterWindow(QWidget *parent = Q_NULLPTR);
	~RegisterWindow();

signals:
	void swap_to_login_window();
	void register_requested(QString username, QString password);

public slots:
	void on_RegisterButtonClicked();
	void on_BackToLoginButtonClicked();

private:
	Ui::RegisterWindow ui;
};
