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
#ifndef PROTOCOL_HH
#define PROTOCOL_HH

#ifndef PROTOCOL_VERSION
#define PROTOCOL_VERSION 0x01L
#endif

#define HEADER_SIZE sizeof(fake_tcp::ProtocolHeader)
#define PSEUDO_SIZE sizeof(fake_tcp::PseudoHeader)
#define BUFFER_SIZE 8192
#define TIMEOUT 300ms

#include <arpa/inet.h>
#include <cstdint>

namespace fake_tcp {

static const uint8_t ERR_FLAG = 0b1;
static const uint8_t SYN_FLAG = 0b10;
static const uint8_t ACK_FLAG = 0b100;
static const uint8_t RST_FLAG = 0b1000;
static const uint8_t FIN_FLAG = 0b10000;
static const uint8_t BEGIN_FLAG = 0b100000;
static const uint8_t FILE_FLAG = 0b10000000;

// Default types.
static const in_addr_t dst_ip = inet_addr("127.0.0.1");
static const in_addr_t src_ip = inet_addr("127.0.0.1");

struct PseudoHeader {
  uint32_t saddr;

  uint32_t daddr;

  // MUST BE ZERO FOR ALIGNMENT.
  uint8_t zeros;

  uint8_t protocol;

  uint16_t packet_len;
};

struct ProtocolHeader {
  uint8_t version;

  uint8_t flag;

  uint16_t checksum;

  uint32_t sequence_number;

  uint32_t acknowledgement_number;

  uint32_t message_len;
};

enum header_type {
  ERR,  // 0b1
  SYN,  // 0b10
  ACK,  // 0b100
  RST,  // 0b1000
  FIN,  // 0b10000
};
}  // namespace fake_tcp

#endif