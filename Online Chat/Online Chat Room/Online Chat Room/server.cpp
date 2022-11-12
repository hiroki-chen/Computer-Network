#include "server.h"
#include "utils.h"

#include <algorithm>
#include <iostream>
#include <Ws2tcpip.h>

std::string
chat_room::Server::get_address(void)
{
	return address;
}

uint32_t
chat_room::Server::get_port(void)
{
	return port;
}

std::shared_ptr<chat_room::Server>
chat_room::Server::get_instance(void)
{
	std::shared_ptr<Server> instance{ new Server(SERVER_ADDRESS,
												 SERVER_PORT,
												 MAXIMUM_ALLOWED_CLIENTS) };
	return instance;
}

chat_room::Server::Server(
	const std::string& address,
	const uint32_t& port,
	const uint32_t& maximum_allowed_clients)
	: address(address)
	, port(port)
	, maximum_allowed_clients(maximum_allowed_clients)
{
	init();
	std::cout << "The server is initialized and waiting for connections." << std::endl;
}

int chat_room::Server::init(void)
{
	try {
		WSADATA wsa_data;

		if (WSAStartup(MAKEWORD(2, 2), &wsa_data)
			!= NO_ERROR) {
			throw std::runtime_error("Cannot initialize wsaData!");
		}

		if ((master_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))
			== INVALID_SOCKET) {
			throw std::runtime_error("INVALID SOCKET");
		}

		// Bind the socket to the listening ip and port.
		ZeroMemory(&server_addr, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(SERVER_PORT);
		inet_pton(AF_INET, SERVER_ADDRESS, &server_addr.sin_addr);

		if (bind(master_socket, (sockaddr*)(&server_addr), sizeof(server_addr)) == SOCKET_ERROR) {
			throw std::runtime_error("BIND ERROR");
		}

		if (listen(master_socket, MAXIMUM_ALLOWED_CLIENTS) == SOCKET_ERROR) {
			throw std::runtime_error("CANNOT START TO LISTEN");
		}

		std::cout << "The server's socket is already binded to the address." << std::endl;

		// Status OK.
		return 0;
	}
	catch (const std::runtime_error& e) {
		std::cout << "Server cannot be initialized!" << std::endl;
		std::cout << "The reason is " << get_error_message(e, WSAGetLastError()) << std::endl;

		// Clean up invalid wsa data.
		WSACleanup();
		closesocket(master_socket);

		return 1;
	}
}

void chat_room::Server::wait_for_new_connections(void)
{
	try {
		SOCKET client = SOCKET_ERROR;
		std::cout << "Waiting for new connections\n";
		while (client = accept(master_socket, NULL, NULL)) {
			client_sockets.push_back(client);
			// Create a new thread.
			std::thread single_handler([this, client] { this->handle_connection(client); });

			// Avoid being deconstructed after loop ends.
			single_handler.detach();

			std::cout << "Detected a new client! Welcome :)" << std::endl;
			greet();
		}

		if (client_sockets.size() > MAXIMUM_ALLOWED_CLIENTS) {
			throw std::runtime_error("TOO MANY CLIENTS");
		}
	} catch (const std::runtime_error& e) {
		//std::cout << client << std::endl;
		std::cout << get_error_message(e, WSAGetLastError()) << std::endl;
		WSACleanup();
	}

	Sleep(100);
}

void chat_room::Server::cleanup_client(void)
{
	for (SOCKET client : client_sockets) {
		Message message;
		message.type = message_type::DISCONNECT;

		send_message(client, message);
		closesocket(client);
	}
}

chat_room::Server::~Server()
{
	cleanup_client();
	WSACleanup();
	closesocket(master_socket);
	std::cout << "The server shutdowns. Goodbye :)" << std::endl;
}

void chat_room::Server::broadcast_message(const chat_room::Message& message)
{
	const uint32_t sender_id = message.sender_id;
	const std::vector<uint32_t> receiver_id = message.receiver_id;
	const uint32_t client_number = client_sockets.size();

	for (uint32_t id : receiver_id) {
		if (sender_id >= client_number || id >= client_number) {
			std::cout << "INVALID RECEIVER OR SENDER ID" << std::endl;
			return;
		}

		const SOCKET target_socket = client_sockets[id];

		send_message(target_socket, message);
	}
}

// When the client wants to send something to another client.
void chat_room::Server::handle_connection(const SOCKET& socket)
{
	std::cout << "Connection is established with one client." << std::endl;

	bool flag = true;
	while (flag) {
		const Message message = recv_message(socket);

		std::cout << "message_type: " << message.type << std::endl;

		switch (message.type) {
		case message_type::GREETING:
			std::cout << "The connection is established." << std::endl;
			break;
		case message_type::NORMAL:
			broadcast_message(message);
			break;
		default:
			flag = false;
			break;
		}
	}
}

void chat_room::Server::greet(void)
{
	Message message;
	message.type = message_type::GREETING;
	message.content = "Hello";
	message.sender_id = 0xffffffff;
	message.receiver_id = { client_sockets.size() - 1 };
	send_message(client_sockets.back(), message);
}