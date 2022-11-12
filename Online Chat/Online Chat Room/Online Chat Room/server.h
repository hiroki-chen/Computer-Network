#pragma once
#pragma comment (lib, "Ws2_32.lib")

#include <string>
#include <memory>
#include <map>
#include <thread>
#include <vector>

#include <winsock2.h> // For socket.

#include "message.h"

#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT 4567
#define MAXIMUM_ALLOWED_CLIENTS 128

namespace chat_room {
	typedef class Server {
	private:
		// The server's address
		const std::string address;

		// The server's port
		const uint32_t port;

		// Maximum clients connected to the server at one time.
		const uint32_t maximum_allowed_clients;

		// The main socket for connection.
		SOCKET master_socket;

		// The client socket vector.
		std::vector<SOCKET> client_sockets;

		// Thread pool
		std::vector<std::thread> threads;

		struct sockaddr_in server_addr;

		Server(const std::string& address,
			   const uint32_t& port,
			   const uint32_t& maximum_allowed_clients);

		// Initialize the server. Invoked by constructor.
		int init(void);

		// Handle chat.
		void handle_connection(const SOCKET& socket);

		// Broadcast the message.
		void broadcast_message(const chat_room::Message& message);

		// Close all connections to the client.
		void cleanup_client(void);

		// Greeting!
		void greet(void);
	public:
		Server() = delete;

		Server(const Server& server) = delete;

		Server& operator=(const Server& server) = delete;

		virtual ~Server();

		static std::shared_ptr<Server>
		get_instance(void);

		std::string
		get_address(void);

		uint32_t
		get_port(void);

		// Waiting for incoming connections.
		// Helper function.
		void wait_for_new_connections(void);

		// void listen(void);
	} Server;
}