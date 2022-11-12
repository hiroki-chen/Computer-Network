#pragma once
#include "message.h"

#include <WinSock2.h>
#include <stdexcept>
#include <vector>

std::string
get_error_message(const std::runtime_error& e, const int& error_code);

int send_message(const SOCKET& socket, const chat_room::Message& message);

std::string
serialize_message(const chat_room::Message& message);

std::string
format_message(const chat_room::Message& message);

chat_room::Message
recv_message(const SOCKET& socket);

chat_room::Message
deserialize_message(const std::string& str);

std::vector<std::string>
split_string(const std::string& str, const std::string& regex);

void serialize_header(char* data, const uint32_t& message_len);

chat_room::ProtocolHeader deserialize_header(char* data);