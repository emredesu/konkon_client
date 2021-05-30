#include "chatwindow.h"

ChatWindow::ChatWindow(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	try {
		new_message_sfx.setSource(QUrl::fromLocalFile("sfx/new_message.wav"));
		new_friend_request_sfx.setSource(QUrl::fromLocalFile("sfx/new_friend_request.wav"));
		friend_deleted_sfx.setSource(QUrl::fromLocalFile("sfx/friend_deleted.wav"));
		login_successful_sfx.setSource(QUrl::fromLocalFile("sfx/login_successful.wav"));

		online_icon = QIcon("sprites/online.png");
		offline_icon = QIcon("sprites/offline.png");
		new_message_icon = QIcon("sprites/new_message.png");
	}
	catch (const std::exception& e) {
		std::cerr << "Error while loading sfx/icons: " << e.what() << "\n";
	}
	catch (...) {
		std::cerr << "Non-std::exception caught while loading sfx/icons." << "\n";
	}

	new_message_sfx.setVolume(1);
	new_friend_request_sfx.setVolume(1);
	friend_deleted_sfx.setVolume(1);

	connect(ui.friends_list, &QListWidget::itemActivated, this, &ChatWindow::friend_selected);
	connect(ui.friend_requests_list, &QListWidget::itemActivated, this, &ChatWindow::friend_request_selected);
	connect(ui.messages_list, &QListWidget::itemActivated, this, &ChatWindow::message_double_clicked);

	ui.friends_list->setContextMenuPolicy(Qt::CustomContextMenu);
	ui.friend_requests_list->setContextMenuPolicy(Qt::CustomContextMenu);

	connect(ui.friends_list, &QWidget::customContextMenuRequested, this, &ChatWindow::display_context_menu_on_friends_list);
	connect(ui.friend_requests_list, &QWidget::customContextMenuRequested, this, &ChatWindow::display_context_menu_on_friend_requests_list);

	connect(ui.message_box, &CustomQTextEdit::enter_pressed, this, &ChatWindow::message_send_confirmed_slot);
}

ChatWindow::~ChatWindow()
{
}	

void ChatWindow::setup(std::vector<std::string> friends, std::vector<std::string> friend_requests) {
	// Set up friends list.
	for (auto&& user : friends) {
		ui.friends_list->addItem(QString::fromStdString(user));
	}

	// Set up friend requests list.
	for (auto&& user : friend_requests) {
		ui.friend_requests_list->addItem(QString::fromStdString(user));
	}
}

void ChatWindow::add_friend_button_clicked() {
	bool text_input_complete;
	QString friendship_request_target = QInputDialog::getText(this, tr("Friendship request"), tr("Please enter the username of the person to send a friend request to"),
		QLineEdit::Normal, QDir::home().dirName(), &text_input_complete);

	if (text_input_complete && !friendship_request_target.isEmpty()) {
		emit friendship_request_sent(friendship_request_target);
	}
	else if (text_input_complete && friendship_request_target.isEmpty()) {
		Globals::UI::show_popup_window("The username field can't be empty.");
	}
}

void ChatWindow::message_send_confirmed_slot() {
	QString message_content = ui.message_box->toPlainText();

	if (last_selected_friend == "")
		return;

	if (message_content.length() > 2000)
		Globals::UI::show_popup_window("Message length cannot be longer than 2000 characters!");
	else if (message_content.length() < 1)
		return;
	else {
		emit message_sent(message_content, last_selected_friend);

		QString current_time = QString::fromStdString(Globals::time::current_time_as_readable_string());
		QString msg_to_add_to_message_box = Globals::generate_message(current_time, username, message_content);

		QList<QVariant> friend_data = ui.friends_list->currentItem()->data(Qt::UserRole).toList();
		friend_data.insert(friend_data.begin(), msg_to_add_to_message_box);
		ui.friends_list->currentItem()->setData(Qt::UserRole, friend_data);
		
		ui.messages_list->addItem(msg_to_add_to_message_box);
		ui.messages_list->scrollToBottom();
		ui.message_box->clear();

	}
}

void ChatWindow::logout_button_clicked() {
	emit logout_requested();
}

void ChatWindow::reset() {
	ui.friends_list->clear();
	ui.friend_requests_list->clear();
	ui.messages_list->clear();
}

void ChatWindow::friend_selected(QListWidgetItem* item) {
	last_selected_friend = item->text();

	if (item->data(Qt::UserRole).isNull()) {
		ui.messages_list->addItem("Fetching messages, please wait...");
		emit messages_requested(last_selected_friend, 100); // Request the last 100 messages.
	}
	else {
		update_chatbox(false);
	}
}

void ChatWindow::update_chatbox(bool is_for_new_message) {
	QListWidgetItem* current_item = ui.friends_list->currentItem();

	if (current_item->data(Qt::UserRole).isNull())
		return;

	if (current_item->foreground() == Qt::red) { // If we had a new message notification from this friend, change it back to normal.
		current_item->setForeground(Qt::black);
	}

	ui.messages_list->clear();
	QList<QVariant> message_data = current_item->data(Qt::UserRole).toList();

	for (auto&& message_in_qlist : boost::adaptors::reverse(message_data)) { // Reverse the QList to add messages old-to-new.
		QString message = message_in_qlist.toString();
		ui.messages_list->addItem(message);
	}

	ui.messages_list->scrollToBottom();
}

void ChatWindow::friend_request_selected(QListWidgetItem* item) {
	QString item_text = item->text();

	QString popup_text = "Would you like to add " + item_text + " as a friend?";

	int reply = Globals::UI::show_popup_question_window(this, "Confirmation", popup_text, false);

	if (reply == QMessageBox::Yes) {
		qDeleteAll(ui.friend_requests_list->findItems(item_text, Qt::MatchFixedString));

		emit friendship_request_responded(true, item_text);
		ui.friends_list->addItem(item_text);
	}
	else if (reply == QMessageBox::No) {
		qDeleteAll(ui.friend_requests_list->findItems(item_text, Qt::MatchFixedString));

		emit friendship_request_responded(false, item_text);
	}
}

QListWidgetItem* ChatWindow::get_currently_selected_friend() {
	return ui.friends_list->currentItem();
}

void ChatWindow::add_new_friend_request(QString sender) {
	ui.friend_requests_list->addItem(sender);
}

void ChatWindow::add_new_friend(QString username) {
	ui.friends_list->addItem(username);
}

void ChatWindow::remove_from_friends_list(QString username) {
	qDeleteAll(ui.friends_list->findItems(username, Qt::MatchFixedString));
}

void ChatWindow::display_context_menu_on_friends_list() {
	QMenu context_menu("Context menu", this);

	QAction delete_friend_action("Delete friend", this);
	connect(&delete_friend_action, &QAction::triggered, this, &ChatWindow::friend_removal_requested_slot);
	context_menu.addAction(&delete_friend_action);

	context_menu.exec(QCursor::pos());
}

void ChatWindow::friend_removal_requested_slot() {
	QString currently_selected_friend = ui.friends_list->currentItem()->text();
	int response = Globals::UI::show_popup_question_window(this, "Confirmation", "Are you sure that you'd like to delete " + currently_selected_friend + "?", true);

	if (response == QMessageBox::Yes)
		emit friend_removal_requested(ui.friends_list->currentItem()->text());
}

void ChatWindow::display_context_menu_on_friend_requests_list() {
	QMenu context_menu("Context menu", this);

	QAction accept_friend_request_action("Accept request", this);
	QAction reject_friend_request_action("Reject request", this);

	QString current_user = ui.friend_requests_list->currentItem()->text();

	connect(&accept_friend_request_action, &QAction::triggered, [=] {qDeleteAll(ui.friend_requests_list->findItems(current_user, Qt::MatchFixedString));
																	 emit friendship_request_responded(true, current_user);
																	 ui.friends_list->addItem(current_user); });
	connect(&reject_friend_request_action, &QAction::triggered, [=] {qDeleteAll(ui.friend_requests_list->findItems(current_user, Qt::MatchFixedString));
																	 emit friendship_request_responded(false, current_user); });

	context_menu.addAction(&accept_friend_request_action);
	context_menu.addAction(&reject_friend_request_action);

	context_menu.exec(QCursor::pos());
}

void ChatWindow::set_friend_data(QString friend_username, QVariant data) {
	QList<QListWidgetItem*> items = ui.friends_list->findItems(friend_username, Qt::MatchExactly);

	if (items.size() > 0) {
		items[0]->setData(Qt::UserRole, data);
	}
	else {
		std::cerr << "Something went wrong with ChatWindow::set_friend_data." << "\n";
	}
}

QListWidget* ChatWindow::get_friends_list_object() {
	return ui.friends_list;
}

void ChatWindow::keyPressEvent(QKeyEvent* event) {
	switch (event->key()) {
	case Qt::Key_Enter: // Make "return (enter)" and "numpad enter" keys do the same thing as pressing the "send" button.
	case Qt::Key_Return:
		message_send_confirmed_slot(); // TODO - do not call a slot, that's probably bad practice
		break;
	}
}

void ChatWindow::update_friend_icon(QString friend_username, bool is_online) {
	QList<QListWidgetItem*> items = ui.friends_list->findItems(friend_username, Qt::MatchExactly);

	if (items.size() > 0) {
		is_online ? items[0]->setIcon(online_icon) : items[0]->setIcon(offline_icon);
	}
}

void ChatWindow::change_friend_colour(QString username, QBrush qtcolour) { // TODO - this function crashes when there is no friends selected, find out why
	try {
		QList<QListWidgetItem*> items = ui.friends_list->findItems(username, Qt::MatchExactly);

		if (items.size() > 0) {
			items[0]->setForeground(qtcolour);
		}
	}
	catch (const std::exception & e) {
		std::cerr << e.what() << "\n";
	}
	catch (...) {
		std::cerr << "... caught" << "\n";
	}
}

QListWidgetItem* ChatWindow::get_friend_qlistwidgetitem_object_by_name(QString friend_username) {
	QList<QListWidgetItem*> items = ui.friends_list->findItems(friend_username, Qt::MatchExactly);

	if (items.size() > 0)
		return items[0];
	else
		return nullptr;
}

void ChatWindow::message_double_clicked(QListWidgetItem* item) {
	QClipboard* clipboard = QApplication::clipboard();
	clipboard->setText(item->text());
}