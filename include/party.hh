/*
 Copyright (c) 2021 Haobin Chen

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef PARTY_HH
#define PARTY_HH

#include <arpa/inet.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/socketvar.h>

#include <cxxopts.hh>
#include <fstream>
#include <party.hh>
#include <queue>
#include <string>
#include <unordered_set>

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif
#ifndef RESET_CONNECT
#define RESET_CONNECT -2
#endif

#define SERVER true
#define CLIENT false
#define MAXIMUM_ESTABLISH_ATTEMPT 8
#define STORAGE_SIZE 4096

using socket_type = int;

namespace fake_tcp {
/**
 * @brief Connection party.
 *
 */

static const std::string default_upload_path = "../upload/";

static const std::string default_file_path = "../test/";

template <class T, int max_len, class Container = std::deque<T>>
class fixed_queue : public std::deque<T> {
 public:
  void push(const T& value) {
    if (this->size() == max_len) {
      this->c.pop_front();
    }
    std::deque<T>::push_back(value);
  }

  bool find(const T& value) {
    for (auto iter = this->cbegin(); iter != this->cend(); iter++) {
      if (*iter == value) {
        return true;
      }
    }
    return false;
  }
};

// Notes:
// * The client does not care to which it is communicating because it just wants
// * to send something. Thus for client, it uses send().
//
// * For the server, since it wants to determine the source of the message, it
// * must first check the source from recvfrom(), and then it can send message
// * back.
//
// * The router should bind to the address **only after** the server is on...
//
// * Also note that the router cannot set packet loss rate to 2...
class Party final {
 private:
  // type == false => This is a client.
  // type == true  => This is a server.
  const bool type;

  std::ifstream* const input_file;

  // Whether the connection is established.
  bool established = false;

  socket_type socket;

  struct sockaddr_in addr;

  struct sockaddr_in dst_addr;

  // We maintain a random number pool to check if a random number is reused.
  std::unordered_set<uint32_t> sequence_pool;

  // Prevent that a buffer is resent.
  fixed_queue<uint32_t, 0xff> received_sequences;

  std::vector<unsigned char*> storage;

  std::vector<std::ofstream> upload_files;

  std::unordered_set<std::string> file_name;  // Files already created.

  size_t maximum_storage_size;

  //========== Functions ===========//
  void init(const std::string& address, const std::string& port);

  void run_server(void);

  void run_client(void);

  void send_file(const std::string& path_prefix = "./test");

  void send_file_information(const std::string& file_name);

  void send_file_content(unsigned char* buffer, const uint32_t& length);

  /**
   * @brief When an SYN packet is received, the server will try to establish a
   * connection with the client.
   *
   */
  int establish_connection_server(const uint32_t& sequence);

  int establish_connection_client(const uint32_t& sequence);

  int connect_to_server();

  void response_server(unsigned char* message, const size_t& length,
                       const uint32_t& src_ip, const uint32_t& dst_ip);

  void response_client(unsigned char* message, const size_t& length,
                       const uint32_t& src_ip, const uint32_t& dst_ip);

 public:
  Party() = delete;

  Party(const cxxopts::ParseResult& result);

  void run(void);

  ~Party();
};
}  // namespace fake_tcp

#endif