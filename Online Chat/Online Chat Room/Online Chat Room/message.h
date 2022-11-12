#pragma once

#define CLIENT_DEFAULT_MESSAGE_LEN 2048

#define DEFAULT_HEADER_SIZE sizeof(struct chat_room::ProtocolHeader)
#define DEFAULT_VERSION 0x1L
#define DEFAULT_MAGIC_NUMBER 0x99

#include <cstdint>
#include <string>
#include <vector>
// Class for message type.

namespace chat_room {
	typedef enum message_type {
		EMPTY = 0,
		INVALID = 1,
		END = 2,
		DISCONNECT = 3,
		GREETING = 4, // ESTABLISHED
		NORMAL = 5
	} message_type;

	typedef struct Message {
		message_type type;

		uint32_t sender_id;	  // The source.

		std::vector<uint32_t> receiver_id; // To which is the message sent.

		std::string content;
	} Message;

	typedef struct ProtocolHeader {
		// Protocol version.
		uint16_t version;

		// Magic number.
		uint16_t magic;

		// The length of the message.
		uint32_t length;
	} Protocol;
}