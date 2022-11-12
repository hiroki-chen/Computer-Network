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
#define BUFFER_SIZE 1024
#define TIMEOUT 1000ms

#include <cstdint>

namespace fake_tcp {
struct PseudoHeader {
    uint32_t saddr;

    uint32_t daddr;

    // MUST BE ZERO FOR ALIGNMENT.
    uint8_t  zeros;

    uint8_t  protocol;

    uint16_t packet_len;
};

struct ProtocolHeader{
    uint8_t version;

    uint8_t flag;

    uint16_t checksum;

    uint32_t sequence_number;

    uint32_t acknowledgement_number;
};

enum header_type {
    ERR, // 0b1
    SYN, // 0b10
    ACK, // 0b100
    RST, // 0b1000
    FIN, // 0b10000
};
} // namespace fake_tcp

#endif