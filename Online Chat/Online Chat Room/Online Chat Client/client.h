#pragma once
#pragma comment(lib, "Ws2_32.lib")

#define CLIENT_ADDRESS "127.0.0.1"
#define CLIENT_PORT 4567

#include "message.h"

#include <winsock2.h>

#include <atomic>
#include <string>
#include <thread>

namespace chat_room{
	typedef class Client {
	private:
		std::atomic<bool> thread_terminate = false;
		std::atomic<bool> thread_can_write = false;

		SOCKET client_socket;

		uint32_t uid;

		std::vector<uint32_t> receiver_id;

		std::thread thread_w;

		std::thread thread_r;

		// Functions for thread.
		void write(void);

		void read(void);

		// Message handler;
		int message_handler(const Message& message);
	public:
		Client(const std::vector<uint32_t>& receiver_id, const uint32_t& uid);

		virtual ~Client();
	} Client;
}