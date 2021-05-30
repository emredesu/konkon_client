#include "connectionmanager.h"

ConnectionManager::ConnectionManager() : ssl_context(boost::asio::ssl::context::tls), socket(io_context, ssl_context) {
	ssl_context.set_verify_mode(boost::asio::ssl::verify_peer);
}

void ConnectionManager::connect() {
	try {
		/* Explicitly tell the client to trust server.crt as that is a self-signed certificate - we normally
		wouldn't need that while connecting to a server with a trusted certificate.

		--- You should remove this try - catch block when working with your own trusted server certificate. --- */
		ssl_context.load_verify_file("server.crt");
	}
	catch (const boost::system::system_error& e) {
		std::cerr << "Error while loading certificate file: " << e.what() << "\n";
		throw std::exception("Could not load the certificate file.");
	}

	try {
		using json = nlohmann::json;

		std::ifstream ifs("client_config.json");
		json config_file = json::parse(ifs);

		host = config_file["connection_info"][0]["server_address"];
		port = config_file["connection_info"][0]["server_port"];
	}
	catch (const std::ifstream::failure& e) {
		std::cerr << "Error while loading client_config.json: " << e.what() << "\n";
		throw std::exception("Could not open client_config.json.");
	}
	catch (const nlohmann::json::exception& e) {
		std::cerr << "Error while parsing JSON: " << e.what() << "\n";
		throw std::exception("Could not parse client_config.json.");
	}

	boost::asio::ip::tcp::resolver resolver(io_context);

	try {
		auto endpoints = resolver.resolve(host, port);
		boost::asio::connect(socket.next_layer(), endpoints);
	}
	catch (const boost::system::system_error& e) {
		std::cerr << "Error while connecting: " << e.what() << "\n";
		throw std::exception("Cannot connect to the server. Please check if you are connected to a network. If you are, the server might be down.");
	}

	try {
		socket.handshake(boost::asio::ssl::stream_base::client);
	}
	catch (const boost::system::system_error& e) {
		std::cerr << "Error during handshake: " << e.what() << "\n";
		throw std::exception("SSL handshake with the server failed.");
	}

	has_connected = true;
}

bool ConnectionManager::send(std::string message) {
	boost::system::error_code io_error;

	if (has_connected) {
		boost::asio::write(socket, boost::asio::buffer(message), io_error);

		if (io_error) {
			std::cerr << "Error while sending message " << message << ", error: " << io_error.message() << "\n";
			return false;
		}
		else {
			return true;
		}
	}
	else {
		std::cerr << "Error while sending message " << message << "not connected." << "\n";
		return false;
	}
}

std::string ConnectionManager::receive() {
	boost::system::error_code io_error;
	boost::asio::streambuf receive_buffer;

	if (has_connected) {
		boost::asio::read_until(socket, receive_buffer, message_delimiter, io_error);
		if (io_error && io_error != boost::asio::error::eof) {
			switch (io_error.value()) {
			case boost::asio::error::connection_reset:
				std::cerr << "Lost connection to the server." << "\n";
				return "DISCONNECTED";
				break;
			case boost::asio::error::connection_aborted:
				return "DISCONNECTED";
				break;
			default:
				std::cerr << "Unhandled read error: " << io_error.message() << " Code: " << io_error.value() << "\n";
				return "UNHANDLED_ERROR";
			}
		}
		else {
			std::string received_data = boost::asio::buffer_cast<const char*>(receive_buffer.data());
			return received_data;
		}
		
	}
	else {
		return "NOT_CONNECTED";
	}
}

void ConnectionManager::reset_info() {
	has_logged_in = false;
	username = "";
	session_cookie = "";
}