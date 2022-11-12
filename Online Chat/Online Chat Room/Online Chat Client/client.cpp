#include "client.h"
#include "message.h"
#include "utils.h"

#include <thread>
#include <iostream>
#include <stdexcept>
#include <Ws2tcpip.h>
#include <WinCon.h>

chat_room::Client::Client(const std::vector<uint32_t>& receiver_id, const uint32_t& uid)
	: receiver_id(receiver_id)
	, uid(uid)
{
	WSADATA wsa_data;
	if (WSAStartup(MAKEWORD(2, 2), &wsa_data)
		!= NO_ERROR) {
		throw std::runtime_error("Cannot initialize wsaData!");
	}

	client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	sockaddr_in client_addr;
	ZeroMemory(&client_addr, sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(CLIENT_PORT);
	inet_pton(AF_INET, CLIENT_ADDRESS, &client_addr.sin_addr);

	if (connect(client_socket, (sockaddr*)(&client_addr), sizeof(client_addr))
		== SOCKET_ERROR) {
		std::cout << WSAGetLastError() << std::endl;
		throw std::runtime_error("CANNOT CONNECT TO THE SERVER");
	}

	std::cout << "The client is connected to the server!" << std::endl;

	// Create two threads.
	thread_w = std::thread([this] { this->write(); });
	thread_r = std::thread([this] { this->read();  });
	thread_w.detach();
	thread_r.detach();
}

int chat_room::Client::message_handler(const Message& message)
{
	message_type type = message.type;

	switch (type) {
	case message_type::GREETING:
	{
		std::cout << format_message(message);
		// Send back greeting!
		Message message_back;
		message_back.type = message_type::GREETING;
		message_back.content = "Hello";
		message_back.sender_id = uid;
		message_back.receiver_id = { 0xffffffff };
		send_message(client_socket, message_back);

		thread_can_write = true;
		
		return 0;
	}
	case INVALID:
		return 1;
	case NORMAL:
	{
		std::cout << format_message(message);
		return 0;
	}

	case message_type::EMPTY:
		return 0;

	case message_type::END:
	case message_type::DISCONNECT:
		shutdown(client_socket, SD_BOTH);
		return 1;

	default:
		throw std::invalid_argument("INVALID MESSAGE TYPE");
	}
}

void chat_room::Client::read(void)
{
	while (true) {
		const Message message = recv_message(client_socket);

		message_handler(message);
	}
}

void chat_room::Client::write(void)
{
	// This function will continuously print messages to the console.
	while (true) {
		if (thread_can_write == true) {
			std::string s;
			std::cin >> s;

			Message message;
			message.sender_id = uid;
			message.content = s;
			message.type = message_type::NORMAL;
			message.receiver_id = receiver_id;
			send_message(client_socket, message);

			HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
			SetConsoleTextAttribute(hConsole, 1);
			std::cout << "(YOU) " << format_message(message);
			std::cout.flush();
			SetConsoleTextAttribute(hConsole, 15);
		}
	}

}

chat_room::Client::~Client()
{
	WSACleanup();
	closesocket(client_socket);

	std::cout << "CLient closed. Goodbye :)" << std::endl;
}