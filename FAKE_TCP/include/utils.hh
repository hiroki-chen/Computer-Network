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
#ifndef UTILS_HH
#define UTILS_HH

#include <protocol.hh>

#include <string>
#include <unordered_set>

#include <arpa/inet.h>

namespace fake_tcp {
/**
 * @brief Generate non-repeating random numbers according to the sequence pool.
 * 
 * @param sequence_pool 
 * @return uint32_t 
 */
uint32_t uniform_random(std::unordered_set<uint32_t>& sequence_pool);

/**
 * @brief A helper function for checking the header.
 * 
 * @param context   if context = true then we check if the header is valid;
 *                  if context = false then we calculate the checksum. 
 *                  Specially, if context = true, context will be set to true if the checksum is valid.
 * @param src_addr 
 * @param dst_addr 
 * @param recv_buf 
 * @param message_len 
 */
void do_check(
    bool* context, const uint32_t& src_addr, const uint32_t& dst_addr,
    unsigned char* recv_buf, const size_t& message_len);

/**
 * @brief Check if the header is valid / Calculate the checksum, according to the context.

 * @param data
 * @param len       The length of the message in byte.
 * 
 * @return uint16_t
 */
uint16_t checksum(unsigned char* header, size_t len);

/**
 * @brief 
 * 
 * @param socket 
 * @param recv_buffer 
 * @param src_addr 
 * @param dst_addr 
 * @return int 
 */
int handle_recv(const int& socket, unsigned char* recv_buffer, sockaddr_in* local_addr, sockaddr_in* remote_addr);

/**
 * @brief Dump the given message to its binary form.
 * 
 * @param buffer 
 * @param length 
 * @return std::string 
 */
std::string
dump_binary(unsigned char* buffer, size_t length);

} // namespace fake_tcp

#endif