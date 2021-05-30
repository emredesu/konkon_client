#pragma once

#ifdef _WIN32
#include <sdkddkver.h> // Gets rid of Boost's "please define target" warnings on Windows.
#endif

#include <iostream>
#include <string>
#include <fstream>
#include <map>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <nlohmann/json.hpp>

class ConnectionManager {
	private:
		boost::asio::io_context io_context;
		boost::asio::ssl::context ssl_context;
		boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket;
		std::string host = "";
		std::string port = "";

		std::string message_delimiter = "\r\n\r\n";

	public:
		bool has_connected = false;
		
		std::string username = "";
		std::string session_cookie = "";
		bool has_logged_in = false;

		ConnectionManager();
		bool send(std::string message); // Returns false on failure and true on success.
		std::string receive(); // Returns the data received.
		void connect();
		void reset_info();
};