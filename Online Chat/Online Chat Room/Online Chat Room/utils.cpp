#include "utils.h"
//#include "server.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iostream>
#include <thread>
#include <regex>
#include <sstream>

#pragma warning(disable : 4996)

std::string
get_error_message(const std::runtime_error& e, const int& error_code)
{
	std::string message = e.what();
	message.append(" " + std::to_string(error_code));

	return message;
}

int send_message(const SOCKET& socket, const chat_room::Message& message)
{
	const std::string serialized_mesasge = serialize_message(message);

	// Serailize the header.
	char* message_buf = new char[DEFAULT_HEADER_SIZE + serialized_mesasge.size()];
	memset(message_buf, 0, DEFAULT_HEADER_SIZE + serialized_mesasge.size());
	serialize_header(message_buf, serialized_mesasge.size());
	memcpy(message_buf + DEFAULT_HEADER_SIZE, serialized_mesasge.data(), serialized_mesasge.size());

	// Send the message
	const int status_ok = send(socket, message_buf, DEFAULT_HEADER_SIZE + serialized_mesasge.size(), 0);

	return status_ok;
}

std::string
serialize_message(const chat_room::Message& message)
{
	std::ostringstream oss;
	std::string s;
	const std::vector<uint32_t> receiver_id = message.receiver_id;

	oss << message.type << "_"
		<< message.sender_id << "_";

	std::for_each(receiver_id.begin(), receiver_id.end(), [&s](const uint32_t& id) {
		s.append(std::to_string(id) + ",");
		}
	);
	s.pop_back();
	
	oss << s << "_" << message.content;

	return oss.str();
}

chat_room::Message
deserialize_message(const std::string& str)
{
	const std::vector<std::string> tokens = split_string(str, "_");

	chat_room::Message message;

	const uint32_t type = std::stoul(tokens[0]);
	switch (type) {
	case 0:
		message.type = chat_room::message_type::EMPTY;
		break;
	case 2:
		message.type = chat_room::message_type::END;
		break;
	case 3:
		message.type = chat_room::message_type::DISCONNECT;
		break;
	case 4:
		message.type = chat_room::message_type::GREETING;
		break;
	case 5:
		message.type = chat_room::message_type::NORMAL;
		break;
	default:
		message.type = chat_room::message_type::INVALID;
		break;
	}
	message.sender_id = std::stoul(tokens[1]);
	message.content = tokens[3];

	// Split receivers.
	std::vector<std::string> receivers = split_string(tokens[2], ",");
	std::vector<uint32_t> receiver_id;
	std::transform(
		receivers.begin(), receivers.end(), std::back_inserter(receiver_id), 
		[](const std::string& id) {
			return std::stoul(id);
		});
	message.receiver_id = receiver_id;
	return message;
}

std::string
format_message(const chat_room::Message& message)
{
	std::ostringstream oss;
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	std::time_t cur_time = std::chrono::system_clock::to_time_t(now);
	std::string time = std::ctime(&cur_time);
	time.pop_back();
	oss << "[" << time << "] Client "
			   << (message.sender_id == 0xffffffff ? "Server" : std::to_string(message.sender_id)) << ": "
			   << message.content << std::endl;
	return oss.str();
}

std::vector<std::string>
split_string(const std::string& str, const std::string& regex_str)
{
	std::regex pattern(regex_str);

	std::vector<std::string> res(
		std::sregex_token_iterator(str.begin(), str.end(), pattern, -1),
		std::sregex_token_iterator());

	return res;
}

chat_room::Message
recv_message(const SOCKET& socket)
{
	char* message_buf = new char[CLIENT_DEFAULT_MESSAGE_LEN];
	memset(message_buf, 0, CLIENT_DEFAULT_MESSAGE_LEN);

	int result = 0;
	if ((result = recv(socket, message_buf, CLIENT_DEFAULT_MESSAGE_LEN, 0))
		== SOCKET_ERROR) {
		chat_room::Message message;
		message.type = chat_room::message_type::INVALID;
		return message;
	}

	chat_room::ProtocolHeader header = deserialize_header(message_buf);

	if (header.version != DEFAULT_VERSION || header.magic != DEFAULT_MAGIC_NUMBER) {
		std::cout << "INVALID HEADER TYPE\n";
	}

	return deserialize_message(std::string(message_buf + DEFAULT_HEADER_SIZE, header.length));
}

void serialize_header(char* data, const uint32_t& message_len)
{
	*(uint16_t*)data = DEFAULT_VERSION;
	*(uint16_t*)(data + 2) = DEFAULT_MAGIC_NUMBER;
	*(uint32_t*)(data + 4) = message_len;
}

chat_room::ProtocolHeader
deserialize_header(char* data)
{
	chat_room::ProtocolHeader header;
	header.version = *(uint16_t*)data;
	header.magic = *(uint16_t*)(data + 2);
	header.length = *(uint32_t*)(data + 4);

	return header;
}