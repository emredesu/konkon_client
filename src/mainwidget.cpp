#include "mainwidget.h"

MainWidget::MainWidget(QWidget *parent) : QStackedWidget(parent), 
										  network_thread(&MainWidget::receive_and_parse_forever, this), 
										  message_processor_thread(&MainWidget::process_received_forever, this),
										  friend_status_checker_thread(&MainWidget::check_friend_statuses_forever, this) {
	// Need to do this to be able to pass these object types through Qt signals.
	qRegisterMetaType<std::vector<std::string>>("std::vector<std::string>");
	qRegisterMetaType<QMessageBox::Icon>("QMessageBox::Icon");
	qRegisterMetaType<QVector<int>>("QVector<int>");
	qRegisterMetaType<nlohmann::json>("nlohmann::json");
	qRegisterMetaType<SoundEffect>("SoundEffect");

	ui.setupUi(this);
	addWidget(&login_window);
	addWidget(&register_window);
	addWidget(&chat_window);

	setCurrentIndex(0); // Launch to the login screen.
	setWindowTitle("Konkon - Login");

	// Set up screen switching signals.
	connect(&login_window, &LoginWindow::swap_to_register_window, this, &MainWidget::swap_to_register_window);
	connect(&register_window, &RegisterWindow::swap_to_login_window, this, &MainWidget::swap_to_login_window);

	// Set up popup message signals that come from other threads.
	connect(this, &MainWidget::sig_show_popup_message, this, &MainWidget::slot_show_popup_message);
	connect(this, &MainWidget::forced_logout_signal, this, &MainWidget::forced_logout_handler);
	connect(this, &MainWidget::update_chatbox_signal, this, &MainWidget::update_chatbox_slot);
	connect(this, &MainWidget::update_friend_icons_signal, this, &MainWidget::update_friend_icons);
	connect(this, &MainWidget::play_sfx_signal, this, &MainWidget::play_sfx);
	connect(this, &MainWidget::alert_signal, this, &MainWidget::create_alert);

	// Set up signals for login/registration messages.
	connect(&login_window, &LoginWindow::login_requested, this, &MainWidget::send_login_info);
	connect(&register_window, &RegisterWindow::register_requested, this, &MainWidget::send_registration_info);

	// Set up signals for login/registration success/failure messages.
	connect(this, &MainWidget::login_successful_signal, this, &MainWidget::login_successful_handler);
	connect(this, &MainWidget::login_failed_signal, this, &MainWidget::login_failed_handler);
	connect(this, &MainWidget::registration_successful_signal, this, &MainWidget::registration_successful_handler);
	connect(this, &MainWidget::registration_failed_signal, this, &MainWidget::registration_failed_handler);

	// Set up the signal for disconnection.
	connect(this, &MainWidget::disconnected_signal, this, &MainWidget::disconnection_handler);

	// Set up signals that come from the chat window.
	connect(&chat_window, &ChatWindow::friendship_request_sent, this, &MainWidget::friend_request_handler);
	connect(&chat_window, &ChatWindow::message_sent, this, &MainWidget::sent_message_handler);
	connect(&chat_window, &ChatWindow::logout_requested, this, &MainWidget::logout_handler);
	connect(&chat_window, &ChatWindow::friendship_request_responded, this, &MainWidget::friend_request_response_handler);
	connect(&chat_window, &ChatWindow::friend_removal_requested, this, &MainWidget::friend_deletion_handler);
	connect(&chat_window, &ChatWindow::messages_requested, this, &MainWidget::request_messages);

	// Attempt connecting to the server.
	try {
		connection_manager.connect();
	}
	catch (const std::exception& e) {
		int pressed_button = Globals::UI::show_popup_window("Cannot connect to the server: " + QString(e.what()));

		if (pressed_button == QMessageBox::Ok) // Exit the app after the "OK" button is clicked.
			exit(EXIT_FAILURE);
	}

	network_thread.detach();
	message_processor_thread.detach();
	friend_status_checker_thread.detach();
}

MainWidget::~MainWidget()
{
}

QSize MainWidget::sizeHint() const {
	return currentWidget()->sizeHint(); // Override to return the sizeHint of the current widget instead of the largest widget available.
}

QSize MainWidget::minimumSizeHint() const {
	return currentWidget()->minimumSizeHint(); // Override to return the minimumSizeHint of the current widget instead of the largest widget available.
}

void MainWidget::setCurrentIndex(int index) {
	if (this->currentWidget() != 0) {
		this->currentWidget()->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	}

	QStackedWidget::setCurrentIndex(index);
	this->currentWidget()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); // Set the sizePolicy of the current widget to be expanding so that it makes the screen scale to its maximum size.
	this->adjustSize(); // Resize the QStackedWidget based on the current widget.
}

void MainWidget::receive_and_parse_forever() {
	while (true) {
		using json = nlohmann::json;

		std::string received = connection_manager.receive(); // This blocks until something can be read.

		if (received == "DISCONNECTED") {
			emit disconnected_signal();
			break;
		}
		else if (received == "NOT_CONNECTED")
			continue;

		std::cout << received << "\n"; // TODO - just for testing, remove later
		json parsed;

		try {
			parsed = json::parse(received);
		}
		catch (json::exception& e) { // In the unlikely event of an unparsable message, we will just log it and ignore it.
			std::cerr << "--- RECEIVED UNPARSABLE MESSAGE: " << received << " ---";
			continue;
		}

		mutex.lock();
		received_messages.emplace_back(parsed);
		mutex.unlock();
	}
}

void MainWidget::process_received_forever() {
	while (true) {
		if (!received_messages.empty()) {
			mutex.lock();

			nlohmann::json received_json = received_messages[0];

			ReceivedMessageType received_message_type = received_string_to_enum[received_json["message-type"]];

			switch (received_message_type) {
				case ReceivedMessageType::ERROR_MESSAGE: {
					std::string server_received_type = received_json["received-type"];
					std::cerr << "------- RECEIVED UNKNOWN ERROR, SERVER RECEIVED: " << server_received_type << " --------" << "\n";
					break;
				}

				case ReceivedMessageType::LOGIN_AUTHENTICATION: {
					bool is_successful = received_json["success"];

					if (is_successful) {
						connection_manager.has_logged_in = true;
						connection_manager.session_cookie = received_json["cookie"];
						std::vector<std::string> friends = received_json["friends"].get<std::vector<std::string>>();
						std::vector<std::string> friend_requests = received_json["friend-requests"].get<std::vector<std::string>>();

						emit play_sfx_signal(SoundEffect::LOGIN_SUCCESSFUL);
						emit login_successful_signal(friends, friend_requests);
					}
					else {
						QString failure_reason = QString::fromStdString(received_json["failure-reason"]);

						emit login_failed_signal(failure_reason);
					}

					break;
				}

				case ReceivedMessageType::REGISTRATION_CONFIRMATION: {
					bool is_successful = received_json["success"];

					if (is_successful) {
						emit registration_successful_signal();
					}
					else {
						QString failure_reason = QString::fromStdString(received_json["failure-reason"]);

						emit registration_failed_signal(failure_reason);
					}

					break;
				}

				case ReceivedMessageType::FRIEND_REQUEST_RESULT: {
					bool is_successful = received_json["success"];

					if (is_successful) {
						emit sig_show_popup_message(QString("Friendship request sent successfully."), QString("Success!"), QMessageBox::Information);
					}
					else {
						QString failure_reason = QString::fromStdString(received_json["reason"]);
						emit sig_show_popup_message(failure_reason, QString("Error!"));
					}

					break;
				}

				case ReceivedMessageType::SEND_MESSAGE_RESULT: {
					bool is_successful = received_json["success"];

					if (!is_successful) {
						QString failure_reason = QString::fromStdString(received_json["reason"]);
						emit sig_show_popup_message(failure_reason);
					}

					break;
				}

				case ReceivedMessageType::NEW_MESSAGE: {
					unsigned long long sent_at_ulonglong = received_json["sent-at"];

					QString sent_by = QString::fromStdString(received_json["sent-by"]);
					QString sent_at = QString::fromStdString(Globals::time::unix_time_to_readable_string(sent_at_ulonglong));
					QString message_content = QString::fromStdString(received_json["message-content"]);

					QString finalized_str = sent_at + " " + sent_by + ": " + message_content;

					QListWidgetItem* friend_item = chat_window.get_friend_qlistwidgetitem_object_by_name(sent_by);

					if (friend_item != nullptr) {
						QVariant friend_data = friend_item->data(Qt::UserRole);

						if (!friend_data.isNull()) { // Check if friend data is loaded from the server.
							QList<QVariant> friend_data_qlist = friend_data.toList();

							friend_data_qlist.insert(friend_data_qlist.begin(), finalized_str); // Newest messages are at the start of the vector.
							friend_item->setData(Qt::UserRole, friend_data_qlist);

							if (sent_by == chat_window.get_currently_selected_friend()->text()) {
								emit update_chatbox_signal(true);
							}
							else {
								chat_window.change_friend_colour(sent_by, Qt::red);
							}
						}
						else {
							chat_window.change_friend_colour(sent_by, Qt::red);
						}

						emit play_sfx_signal(SoundEffect::NEW_MESSAGE);
						emit alert_signal(3000);
					}

					break;
				}

				case ReceivedMessageType::NEW_FRIENDSHIP_REQUEST: {
					std::string sender = received_json["sent-by"];

					chat_window.add_new_friend_request(QString::fromStdString(sender));
					emit play_sfx_signal(SoundEffect::NEW_FRIEND_REQUEST);
					emit alert_signal(5000);

					break;
				}

				case ReceivedMessageType::FRIENDSHIP_REQUEST_UPDATE: {
					std::string request_recipient = received_json["request-recipient"];
					bool is_accepted = received_json["is_accepted"];

					if (is_accepted) {
						chat_window.add_new_friend(QString::fromStdString(request_recipient));
					}
					else {
						QString popup_window_text = QString::fromStdString(request_recipient) + " denied your friend request.";

						emit sig_show_popup_message(popup_window_text, "Friend request update", QMessageBox::Information, ":(");
					}
					break;
				}
																   
				case ReceivedMessageType::FRIEND_DELETION_UPDATE: {
					std::string deleted_friend = received_json["deleted-user"];
					bool is_successful = received_json["success"];

					if (!is_successful) {
						QString error = QString::fromStdString(received_json["reason"]);
						QString popup_window_text = "Can't delete '" + QString::fromStdString(deleted_friend) + "', error: " + error;

						emit sig_show_popup_message(popup_window_text);
					}
					else {
						chat_window.remove_from_friends_list(QString::fromStdString(deleted_friend));
						emit play_sfx_signal(SoundEffect::FRIEND_DELETED);
					}
					break;
				}

				case ReceivedMessageType::DELETED_BY_FRIEND: {
					std::string deleted_by = received_json["deleted-by"];

					chat_window.remove_from_friends_list(QString::fromStdString(deleted_by));
					break;
				}

				case ReceivedMessageType::FORCED_LOGOUT: {
					std::string reason = received_json["reason"];

					emit forced_logout_signal(QString::fromStdString(reason));
					break;
				}

				case ReceivedMessageType::FETCH_MESSAGES_REQUEST_RESPONSE: {
					bool is_successful = received_json["success"];

					if (!is_successful) {
						QString reason = QString::fromStdString(received_json["reason"]);
						QString user = QString::fromStdString(received_json["user"]);

						emit sig_show_popup_message("Error while getting message history with " + user + ", error: " + reason);
					}
					else {
						try {
							QString friend_name = QString::fromStdString(received_json["user"]);
							std::vector<std::string> message_json_vector = received_json["messages"].get<std::vector<std::string>>();
							QList<QVariant> message_json_qlist;

							for (auto&& message_json : message_json_vector) {
								nlohmann::json json = nlohmann::json::parse(message_json);

								unsigned long long sent_at_ulonglong = json["sent-at"];

								QString sent_by = QString::fromStdString(json["sent-by"]);
								QString message_content = QString::fromStdString(json["message-content"]);
								QString sent_at = QString::fromStdString(Globals::time::unix_time_to_readable_string(sent_at_ulonglong));

								QString finalized_qstr = Globals::generate_message(sent_at, sent_by, message_content);
								message_json_qlist.append(finalized_qstr);
							}

							chat_window.set_friend_data(friend_name, message_json_qlist);


							if (chat_window.get_currently_selected_friend()->text() == friend_name) {
								emit update_chatbox_signal(false);
							}
						}
						catch (const std::exception& e) {
							std::cout << e.what() << "\n";
						}
						catch (...) {
							std::cout << "--- Non std::exception caught ---" << "\n";
						}
					}
					break;
				}

				case ReceivedMessageType::DUMMY_MESSAGE:
					break;

				case ReceivedMessageType::GET_STATUSES_RESPONSE: {
					nlohmann::json friend_statuses = received_json["is_friend_online"];
					emit update_friend_icons_signal(friend_statuses);

					break;
				}

				default: {
					std::cerr << "--- RECEIVED UNRECOGNIZED MESSAGE TYPE: " << received_json["message-type"] << " ---" << "\n";
				}
			}

			received_messages.erase(received_messages.begin()); // Remove the message from the vector after we're done with it.
			mutex.unlock();
		}
	}
}

void MainWidget::check_friend_statuses_forever() {
	std::this_thread::sleep_for(std::chrono::seconds(60));

	if (connection_manager.has_logged_in) {
		request_friend_statuses();
	}
}

void MainWidget::swap_to_register_window() {
	setCurrentIndex(REGISTER_WINDOW);
	setWindowTitle("Konkon - Register");
}

void MainWidget::swap_to_login_window() {
	setCurrentIndex(LOGIN_WINDOW);
	setWindowTitle("Konkon - Login");
}

void MainWidget::slot_show_popup_message(QString message, QString title, QMessageBox::Icon icon, QString custom_button_text) {
	Globals::UI::show_popup_window(message, title, icon, custom_button_text);
}

void MainWidget::send_login_info(QString username, QString password) {
	chat_window.username = username;
	connection_manager.username = username.toStdString();

	nlohmann::json json_obj;
	json_obj["message-type"] = "login-request";
	json_obj["username"] = username.toStdString();
	json_obj["password"] = password.toStdString();

	std::string serialized_data = json_obj.dump();
	connection_manager.send(serialized_data);
}

void MainWidget::send_registration_info(QString username, QString password) {
	nlohmann::json json_obj;
	json_obj["message-type"] = "registration-request";
	json_obj["username"] = username.toStdString();
	json_obj["password"] = password.toStdString();

	std::string serialized_data = json_obj.dump();
	connection_manager.send(serialized_data);
}

void MainWidget::login_successful_handler(std::vector<std::string> friends, std::vector<std::string> friend_requests) {
	chat_window.reset();
	chat_window.setup(friends, friend_requests);
	setCurrentIndex(CHAT_WINDOW);
	setWindowTitle("Konkon - " + QString::fromStdString(connection_manager.username));
	request_friend_statuses();
}

void MainWidget::login_failed_handler(QString reason) {
	Globals::UI::show_popup_window(reason);
}

void MainWidget::registration_successful_handler() {
	Globals::UI::show_popup_window("Successfully signed up!");
}

void MainWidget::registration_failed_handler(QString reason) {
	Globals::UI::show_popup_window(reason);
}

void MainWidget::exit_handler() {
	nlohmann::json json_obj;
	json_obj["message-type"] = "disconnection-notification";
	json_obj["username"] = connection_manager.username;

	std::string serialized_data = json_obj.dump();
	connection_manager.send(serialized_data);
}

void MainWidget::disconnection_handler() {
	int pressed = Globals::UI::show_popup_window("Lost connection to the server. Please check your internet connection and try connecting again.");

	if (pressed == QMessageBox::Ok)
		exit(EXIT_FAILURE);
}

void MainWidget::friend_request_handler(QString request_target) {
	if (request_target == QString::fromStdString(connection_manager.username)) {
		Globals::UI::show_popup_window("You can't send a friendship request to yourself...");
	}
	else {
		nlohmann::json json_obj;
		json_obj["message-type"] = "friend-request";
		json_obj["from"] = connection_manager.username;
		json_obj["to"] = request_target.toStdString();
		json_obj["cookie"] = connection_manager.session_cookie;

		std::string serialized_data = json_obj.dump();
		connection_manager.send(serialized_data);
	}
}

void MainWidget::friend_request_response_handler(bool is_accepted, QString request_sender) {
	nlohmann::json json_obj;
	json_obj["message-type"] = "friend-request-response";
	json_obj["accepted"] = is_accepted;
	json_obj["request_sender"] = request_sender.toStdString();
	json_obj["request_replier"] = connection_manager.username;
	json_obj["cookie"] = connection_manager.session_cookie;

	std::string serialized_data = json_obj.dump();
	connection_manager.send(serialized_data);
}

void MainWidget::sent_message_handler(QString message_content, QString message_target) {
	nlohmann::json json_obj;
	json_obj["message-type"] = "send-message";
	json_obj["from"] = connection_manager.username;
	json_obj["to"] = message_target.toStdString();
	json_obj["cookie"] = connection_manager.session_cookie;
	json_obj["message-content"] = message_content.toStdString();

	std::string serialized_data = json_obj.dump();
	connection_manager.send(serialized_data);
}

void MainWidget::friend_deletion_handler(QString username) {
	nlohmann::json json_obj;
	json_obj["message-type"] = "friend-deletion-request";
	json_obj["username"] = connection_manager.username;
	json_obj["cookie"] = connection_manager.session_cookie;
	json_obj["deleted-person"] = username.toStdString();
	
	std::string serialized_data = json_obj.dump();
	connection_manager.send(serialized_data);
}

void MainWidget::logout_handler() {
	nlohmann::json json_obj;
	json_obj["message-type"] = "logout-notification";
	json_obj["username"] = connection_manager.username;

	std::string serialized_data = json_obj.dump();
	connection_manager.send(serialized_data);

	connection_manager.reset_info();
	chat_window.reset();

	setCurrentIndex(LOGIN_WINDOW);
}

void MainWidget::forced_logout_handler(QString reason) {
	int response = Globals::UI::show_popup_window(reason);

	connection_manager.reset_info();
	chat_window.reset();

	setCurrentIndex(LOGIN_WINDOW);
}

void MainWidget::request_messages(QString friend_username, int max_index) {
	nlohmann::json json_obj;
	json_obj["message-type"] = "fetch-messages-request";
	json_obj["requester"] = connection_manager.username;
	json_obj["cookie"] = connection_manager.session_cookie;
	json_obj["other-participant"] = friend_username.toStdString();
	json_obj["max_index"] = max_index;

	std::string serialized_data = json_obj.dump();
	connection_manager.send(serialized_data);
}

void MainWidget::request_friend_statuses() {
	QListWidget* list_widget = chat_window.get_friends_list_object();
	std::vector<std::string> friends_vec;

	for (int i = 0; i < list_widget->count(); i++) {
		QListWidgetItem* item = list_widget->item(i);
		friends_vec.push_back(item->text().toStdString());
	}

	nlohmann::json json_obj;
	json_obj["message-type"] = "get-statuses";
	json_obj["friends"] = friends_vec;

	std::string serialized_data = json_obj.dump();
	connection_manager.send(serialized_data);
}

void MainWidget::update_chatbox_slot(bool is_for_new_message) {
	chat_window.update_chatbox(is_for_new_message);
}

void MainWidget::update_friend_icons(nlohmann::json friend_statuses) {
	for (auto it : friend_statuses.items()) {
		chat_window.update_friend_icon(QString::fromStdString(it.key()), it.value());
	}
}

void MainWidget::play_sfx(SoundEffect which_sfx) {
	switch (which_sfx) {
		case SoundEffect::NEW_MESSAGE:
			chat_window.new_message_sfx.play();
			break;
		case SoundEffect::NEW_FRIEND_REQUEST:
			chat_window.new_friend_request_sfx.play();
			break;
		case SoundEffect::FRIEND_DELETED:
			chat_window.friend_deleted_sfx.play();
			break;
		case SoundEffect::LOGIN_SUCCESSFUL:
			chat_window.login_successful_sfx.play();
			break;
		default:
			return;
	}
}

void MainWidget::create_alert(int duration_in_milliseconds) {
	QApplication::alert(this, duration_in_milliseconds);
}