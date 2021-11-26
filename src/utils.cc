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
#include <arpa/inet.h>
#include <sys/socket.h>

#include <bitset>
#include <cstring>
#include <excepts.hh>
#include <filesystem>
#include <iostream>
#include <random>
#include <stdexcept>
#include <utils.hh>

uint32_t fake_tcp::uniform_random(std::unordered_set<uint32_t>& sequence_pool) {
  std::random_device rd;
  std::mt19937 engine{rd()};
  std::uniform_int_distribution<uint32_t> distribution(0, 0xffffffff);

  // Prevent from reusing sequence numbers.
  uint32_t ans = distribution(engine);
  while (sequence_pool.count(ans)) {
    ans = distribution(engine);
  }
  sequence_pool.insert(ans);
  return ans;
}

void fake_tcp::do_check(bool* context, const uint32_t& src_addr,
                        const uint32_t& dst_addr, unsigned char* buffer,
                        const size_t& message_len) {
  fake_tcp::PseudoHeader pseudo_header;
  memset(&pseudo_header, 0, PSEUDO_SIZE);
  pseudo_header.saddr = src_addr;
  pseudo_header.daddr = dst_addr;
  pseudo_header.protocol = PROTOCOL_VERSION;
  pseudo_header.packet_len = message_len;

  // Create a full header.
  unsigned char* data = new unsigned char[PSEUDO_SIZE + message_len];
  memset(data, 0, PSEUDO_SIZE + message_len);
  memcpy(data, &pseudo_header, PSEUDO_SIZE);
  memcpy(data + PSEUDO_SIZE, buffer, message_len);

  // Check whether the header is valid or not.
  uint16_t res = checksum(data, PSEUDO_SIZE + message_len);

  if (*context == true) {
    *context = (res == 0);
  } else {
    *((uint16_t*)(buffer + 2)) = res;
  }
}

uint16_t fake_tcp::checksum(unsigned char* header, size_t len) {
  // Check.
  uint16_t* ptr = reinterpret_cast<uint16_t*>(header);
  uint16_t sum = 0;
  while (len > 1) {
    sum += *(ptr++);
    len -= 2;
  }
  if (len) {
    sum += ((*ptr) & htons(0xff00));
  }
  // Fold sum to 16 bits: add carrier to the result
  while (sum >> 16) {
    sum = (sum & 0xffff) + (sum >> 16);
  }

  return (uint16_t)(~sum);
}

int fake_tcp::handle_recv(const int& socket, unsigned char* recv_buffer,
                          sockaddr_in* local_addr, sockaddr_in* remote_addr) {
  socklen_t size = sizeof(sockaddr_in);
  size_t message_len = recvfrom(socket, recv_buffer, BUFFER_SIZE, 0,
                                (struct sockaddr*)remote_addr, &size);

  if (message_len == 0) {
    throw fake_tcp::connect_closed("The client closed the connection!");
  } else if (message_len < 0) {
    throw fake_tcp::socket_error("The socket is dead!");
  } else {
    bool context = true;
    //uint32_t source_ip = remote_addr->sin_addr.s_addr;
    //uint32_t destination_ip = local_addr->sin_addr.s_addr;
    do_check(&context, fake_tcp::src_ip, fake_tcp::dst_ip, recv_buffer, message_len);

    if (context == false) {
      throw fake_tcp::invalid_header("Checksum is incorrect!");
    }
  }

  return 0;
}

std::string fake_tcp::dump_binary(unsigned char* buffer, size_t length) {
  std::string ans;

  while (length--) {
    ans.append(std::bitset<8>(*(buffer++)).to_string());
    ans.append("\n");
  }

  return ans;
}

std::vector<std::string> fake_tcp::create_files(
    const std::string& path_prefix) {
  try {
    std::vector<std::string> files;
    const std::filesystem::path directory(path_prefix);

    for (const auto& entry :
         std::filesystem::recursive_directory_iterator(directory)) {
      if (entry.is_regular_file()) {
        files.emplace_back(entry.path().string());
      }
    }

    return files;
  } catch (const std::filesystem::filesystem_error& e) {
    std::cerr << e.what() << std::endl;
    exit(1);
  }
}