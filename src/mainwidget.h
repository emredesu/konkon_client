#pragma once

#include <QCoreApplication>
#include <QApplication>
#include <QStackedWidget>
#include <QList>
#include <QtMultimedia/QSoundEffect>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <mutex>
#include <ctime>
#include <iomanip>
#include <chrono> 
#include <nlohmann/json.hpp>
#include "ui_mainwidget.h"
#include "loginwindow.h"
#include "registerwindow.h"
#include "globals.h"
#include "connectionmanager.h"
#include "chatwindow.h"

enum WindowEnum {
	LOGIN_WINDOW = 0,
	REGISTER_WINDOW = 1,
	CHAT_WINDOW = 2
};

enum class ReceivedMessageType {
	ERROR_MESSAGE,
	LOGIN_AUTHENTICATION,
	REGISTRATION_CONFIRMATION,
	FRIEND_REQUEST_RESULT,
	SEND_MESSAGE_RESULT,
	NEW_MESSAGE,
	NEW_FRIENDSHIP_REQUEST,
	FRIENDSHIP_REQUEST_UPDATE,
	FRIEND_DELETION_UPDATE,
	DELETED_BY_FRIEND,
	FORCED_LOGOUT,
	FETCH_MESSAGES_REQUEST_RESPONSE,
	DUMMY_MESSAGE,
	GET_STATUS_RESPONSE,
	GET_STATUSES_RESPONSE
};

enum class SoundEffect {
	NEW_MESSAGE,
	NEW_FRIEND_REQUEST,
	FRIEND_DELETED,
	LOGIN_SUCCESSFUL
};

class MainWidget : public QStackedWidget
{
	Q_OBJECT

public:
	MainWidget(QWidget *parent = Q_NULLPTR);
	~MainWidget();

	// Overriding these methods to be able to properly resize the QStackedWidget when we change the current widget.
	void setCurrentIndex(int index);
	QSize sizeHint() const override;
	QSize minimumSizeHint() const override;

	void request_friend_statuses();

signals:
	void login_successful_signal(std::vector<std::string> friends, std::vector<std::string> friend_requests);
	void login_failed_signal(QString reason);
	void registration_successful_signal();
	void registration_failed_signal(QString reason);
	void disconnected_signal();
	void sig_show_popup_message(QString message, QString title = "Warning!", QMessageBox::Icon icon = QMessageBox::Warning, QString custom_button_text = "OK");
	void forced_logout_signal(QString reason);
	void update_chatbox_signal(bool is_for_new_message);
	void update_friend_icons_signal(nlohmann::json friend_statuses);
	void play_sfx_signal(SoundEffect which_sfx);
	void alert_signal(int duration_in_milliseconds);

public slots:
	void swap_to_register_window();
	void swap_to_login_window();

	void send_login_info(QString username, QString password);
	void send_registration_info(QString username, QString password);

	void login_successful_handler(std::vector<std::string> friends, std::vector<std::string> friend_requests);
	void login_failed_handler(QString reason);
	void registration_successful_handler();
	void registration_failed_handler(QString reason);

	void exit_handler();
	void disconnection_handler();

	void friend_request_handler(QString request_target);
	void sent_message_handler(QString message_content, QString message_target);
	void logout_handler();

	void request_messages(QString friend_username, int max_index);

	void friend_request_response_handler(bool is_accepted, QString request_sender);
	void friend_deletion_handler(QString username);

	void slot_show_popup_message(QString message, QString title, QMessageBox::Icon icon, QString custom_button_text);

	void forced_logout_handler(QString reason);

	void update_chatbox_slot(bool is_for_new_message);

	void update_friend_icons(nlohmann::json friend_statuses);

	void play_sfx(SoundEffect which_sfx);
	void create_alert(int duration_in_milliseconds);

private:
	Ui::MainWidget ui;
	LoginWindow login_window;
	RegisterWindow register_window;
	ChatWindow chat_window;
	ConnectionManager connection_manager;

	std::mutex mutex;
	std::vector<nlohmann::json> received_messages;
	std::thread network_thread;
	std::thread message_processor_thread;
	std::thread friend_status_checker_thread;

	// These functions will run in a while(true) loop on seperate threads.
	void receive_and_parse_forever();
	void process_received_forever();
	void check_friend_statuses_forever();

	std::map<std::string, ReceivedMessageType> received_string_to_enum {
		{"unexpected-error", ReceivedMessageType::ERROR_MESSAGE},
		{"login-authentication", ReceivedMessageType::LOGIN_AUTHENTICATION},
		{"registration-confirmation", ReceivedMessageType::REGISTRATION_CONFIRMATION},
		{"friend-request-result", ReceivedMessageType::FRIEND_REQUEST_RESULT},
		{"send-message-result", ReceivedMessageType::SEND_MESSAGE_RESULT},
		{"new-message", ReceivedMessageType::NEW_MESSAGE},
		{"new-friendship-request", ReceivedMessageType::NEW_FRIENDSHIP_REQUEST},
		{"friend-request-update", ReceivedMessageType::FRIENDSHIP_REQUEST_UPDATE},
		{"friend-deletion-update", ReceivedMessageType::FRIEND_DELETION_UPDATE},
		{"deleted-by-friend", ReceivedMessageType::DELETED_BY_FRIEND},
		{"forced-logout", ReceivedMessageType::FORCED_LOGOUT},
		{"fetch-messages-request-response", ReceivedMessageType::FETCH_MESSAGES_REQUEST_RESPONSE},
		{"dummy-message", ReceivedMessageType::DUMMY_MESSAGE},
		{"get-status-response", ReceivedMessageType::GET_STATUS_RESPONSE},
		{"get-statuses-response", ReceivedMessageType::GET_STATUSES_RESPONSE}
	};
};
