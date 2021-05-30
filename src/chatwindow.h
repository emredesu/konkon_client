#pragma once

#include <QWidget>
#include <QInputDialog>
#include <QMessageBox>
#include <QDir>
#include <QMenu>
#include <QList>
#include <QVariant>
#include <QtMultimedia/QSoundEffect>
#include <QUrl>
#include <QIcon>
#include <QBrush>
#include <QClipboard>
#include <vector>
#include <iostream> // todo - temp
#include <boost/range/adaptor/reversed.hpp>
#include "globals.h"
#include "customqtextedit.h"
#include "ui_chatwindow.h"

class ChatWindow : public QWidget
{
	Q_OBJECT

public:
	ChatWindow(QWidget *parent = Q_NULLPTR);
	~ChatWindow();
	virtual void keyPressEvent(QKeyEvent* event);
	void setup(std::vector<std::string> friends, std::vector<std::string> friend_requests);
	void reset();
	void add_new_friend_request(QString sender);
	void add_new_friend(QString username);
	void remove_from_friends_list(QString username);
	void update_chatbox(bool is_for_new_message);
	QListWidgetItem* get_currently_selected_friend();
	QListWidget* get_friends_list_object();
	void set_friend_data(QString friend_username, QVariant data);
	void update_friend_icon(QString friend_username, bool is_online);
	void change_friend_colour(QString friend_username, QBrush qtcolour);
	QListWidgetItem* get_friend_qlistwidgetitem_object_by_name(QString friend_username);

	QString username;
	QString last_selected_friend;

	QSoundEffect new_message_sfx;
	QSoundEffect new_friend_request_sfx;
	QSoundEffect friend_deleted_sfx;
	QSoundEffect login_successful_sfx;
	
	QIcon online_icon;
	QIcon offline_icon;
	QIcon new_message_icon;

signals:
	void friendship_request_sent(QString to_whom);
	void message_sent(QString message_content, QString to_whom);
	void logout_requested();

	void friendship_request_responded(bool accepted, QString friend_request_sender);
	void friend_removal_requested(QString deleted_username);

	/* Only 100 messages can be requested at once. If the parameter "last_n_messages" is 200, messages between the last 100th and 200th messages will be 
	returned by the server. */
	void messages_requested(QString user, int last_n_messages);

public slots:
	void friend_selected(QListWidgetItem* item);
	void friend_request_selected(QListWidgetItem* item);

	void add_friend_button_clicked();
	void logout_button_clicked();
	void message_send_confirmed_slot();

	void display_context_menu_on_friends_list();
	void display_context_menu_on_friend_requests_list();

	void friend_removal_requested_slot();
	void message_double_clicked(QListWidgetItem* item);

private:
	Ui::ChatWindow ui;
};
