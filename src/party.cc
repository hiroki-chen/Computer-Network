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
#include <chrono>
#include <excepts.hh>
#include <party.hh>
#include <protocol.hh>
#include <sstream>
#include <thread>
#include <timer.hh>
#include <utils.hh>
#include <vector>

using namespace std::literals::chrono_literals;

fake_tcp::Party::Party(const cxxopts::ParseResult& result)
    : type(result["server"].as<bool>()),
      input_file(new std::ifstream(result["input"].as<std::string>())),
      socket(-1) {
  // Get necessary parameters.
  const std::string address = result["address"].as<std::string>();
  const std::string port = result["port"].as<std::string>();
  // Create the buffer.
  maximum_storage_size = 4096;

  try {
    init(address, port);
  } catch (const fake_tcp::socket_error& e) {
    std::cerr << e.what() << std::endl;
    std::terminate();
  }
}

void fake_tcp::Party::init(const std::string& address,
                           const std::string& port) {
  // Create a socket.
  if ((socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR) {
    std::ostringstream oss;
    oss << "Error[" << errno << "]: "
        << "Cannot initialize the socket!";
    throw fake_tcp::socket_error(oss.str());
  }

  bzero(&addr, sizeof(addr));

  // using IPv4
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(address.c_str());
  addr.sin_port = htons(std::stoul(port));

  if (type == SERVER) {
    // Bind to the address.
    if (::bind(socket, (const struct sockaddr*)(&addr), sizeof(addr)) ==
        SOCKET_ERROR) {
      std::ostringstream oss;
      oss << "Error[" << errno << "]: "
          << "Cannot bind to the given address!";
      throw fake_tcp::socket_error(oss.str());
    }

  } else {
    if (::connect(socket, (struct sockaddr*)(&addr), sizeof(addr)) ==
        SOCKET_ERROR) {
      std::ostringstream oss;
      oss << "Error[" << errno << "]: "
          << "Cannot connect to the given address! The server seems to be "
             "offline.";
      throw fake_tcp::socket_error(oss.str());
    }
  }
}

void fake_tcp::Party::run(void) {
  type == SERVER ? run_server() : run_client();
}

void fake_tcp::Party::run_server(void) {
  // Message_len will be a member of fake_tcp::PseudoHeader.
  std::cout << "The server is running!" << std::endl;
  uint16_t message_len = 0;

  do {
    try {
      unsigned char* recv_buf = new unsigned char[BUFFER_SIZE];
      bzero(recv_buf, BUFFER_SIZE);
      handle_recv(socket, recv_buf, &addr, &dst_addr);

      // Extract source ip from the saddr struct.
      uint32_t source_ip = ((struct sockaddr_in*)(&addr))->sin_addr.s_addr;
      uint32_t destination_ip = dst_addr.sin_addr.s_addr;

      // Checksum ok. Response to the client.
      response_server(recv_buf, message_len, source_ip, destination_ip);
    } catch (const fake_tcp::invalid_header& e) {
      std::cerr << e.what() << std::endl;
      continue;
    }

  } while (true);
}

void fake_tcp::Party::run_client(void) {
  // The client only does three things:
  // 1. Establish a connection with the server.
  // 2. Send the files to the server.
  // 3. Close the connection by sending the FIN_FLAG to the server.
  connect_to_server();
  // Send files.
  send_file("../test");
  // Close connection.
}

int fake_tcp::Party::establish_connection_server(const uint32_t& sequence) {
  try {
    unsigned char* recv_buffer = new unsigned char[BUFFER_SIZE];
    handle_recv(socket, recv_buffer, &addr, &dst_addr);

    const uint8_t flag = *((uint8_t*)(recv_buffer + 1));
    const uint32_t seq = *((uint32_t*)(recv_buffer + 4));
    const uint32_t ack = *((uint32_t*)(recv_buffer + 8));

    std::cout << seq << ", " << ack << std::endl;
    if (flag == fake_tcp::ACK_FLAG && ack == sequence + 1) {
      std::cout << "[Server] Connection established!" << std::endl;
      storage.emplace_back(new unsigned char[STORAGE_SIZE]);
      return 0;
    } else {
      throw fake_tcp::invalid_header("[Server] Seq and Ack are incorrect!");
    }

  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }

  return -1;
}

int fake_tcp::Party::establish_connection_client(const uint32_t& sequence) {
  // Wait for server's ack.
  try {
    unsigned char* recv_buffer = new unsigned char[BUFFER_SIZE];
    handle_recv(socket, recv_buffer, &addr, &dst_addr);

    const uint8_t flag = *((uint8_t*)(recv_buffer + 1));
    // Received sequence.
    const uint32_t seq = *((uint32_t*)(recv_buffer + 4));
    const uint32_t ack = *((uint32_t*)(recv_buffer + 8));

    if (flag == fake_tcp::ACK_FLAG && ack == sequence + 1) {
      std::cout << "[Client] Handshake received! Sending Ack..." << std::endl;

      unsigned char* send_buf = new unsigned char[HEADER_SIZE];
      bzero(send_buf, HEADER_SIZE);
      const uint32_t sequence_number = uniform_random(sequence_pool);

      *((uint8_t*)(send_buf)) = PROTOCOL_VERSION;
      *((uint8_t*)(send_buf + 1)) = fake_tcp::ACK_FLAG;
      *((uint16_t*)(send_buf + 2)) = 0b0;
      *((uint32_t*)(send_buf + 4)) = sequence_number;
      // Response to the sender.
      *((uint32_t*)(send_buf + 8)) = (seq + 1) % 0xffffffff;

      bool context = false;
      do_check(&context, src_ip, dst_ip, send_buf, HEADER_SIZE);

      sendto(socket, send_buf, HEADER_SIZE, 0, (struct sockaddr*)(&dst_addr),
             sizeof(sockaddr));

      std::cout << "[Client] Connection established!" << std::endl;

      return 0;
    } else {
      throw fake_tcp::invalid_header("[Client] Seq and Ack are incorrect!");
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
}

int fake_tcp::Party::connect_to_server() {
  std::atomic_bool satisfied = false;
  while (satisfied == false) {
    // Send an SYN flag header to the server.
    const uint32_t sequence = uniform_random(sequence_pool);
    unsigned char* header = new unsigned char[HEADER_SIZE];
    bzero(header, HEADER_SIZE);
    *((uint8_t*)(header)) = PROTOCOL_VERSION;
    *((uint8_t*)(header + 1)) = fake_tcp::SYN_FLAG;
    *((uint16_t*)(header + 2)) = 0;
    *((uint32_t*)(header + 4)) = sequence;
    *((uint32_t*)(header + 8)) = 0;

    bool context = false;
    do_check(&context, src_ip, dst_ip, header, HEADER_SIZE);
    std::cout << "[Client] Checksum: " << *(uint16_t*)(header + 2) << std::endl;
    send(socket, header, HEADER_SIZE, 0);

    std::thread t([&]() {
      std::cout << "[Client] Waiting for Ack..." << std::endl;
      std::uint32_t attempt_times = 0;

      std::thread t([&]() {
        while (++attempt_times <= MAXIMUM_ESTABLISH_ATTEMPT) {
          // Create a thread and let it receive the ack.
          std::thread t([&]() {
            while (satisfied == false) {
              if (establish_connection_client(sequence) == 0) {
                satisfied = true;
              }
            }
          });
          t.detach();

          // Sleep.
          std::this_thread::sleep_for(TIMEOUT);
          if (satisfied == true) {
            break;
          }
          std::cout << "[Client] Timed out. Try again." << std::endl;
        }
      });
      t.join();

      if (satisfied == false) {
        std::cerr << "[Client] Handshake failed!" << std::endl;
        sequence_pool.clear();
      }
    });

    t.detach();
    std::this_thread::sleep_for(5000ms);
  }
  return 0;
}

void fake_tcp::Party::response_server(unsigned char* message,
                                      const size_t& length,
                                      const uint32_t& src_ip,
                                      const uint32_t& dst_ip) {
  // Extract header.
  const uint8_t version = *((uint8_t*)(message));
  const uint8_t flag = *((uint8_t*)(message + 1));
  // Since checksum is already checked before responsing to the client,
  // we can omit it.
  const uint32_t seq = *((uint32_t*)(message + 4));
  const uint32_t ack = *((uint32_t*)(message + 8));
  // Add to pool
  sequence_pool.insert(seq);

  /*---------------------
        ERR,  0b1
        SYN,  0b10
        ACK,  0b100
        RST,  0b1000
        FIN,  0b10000
  ----------------------*/
  switch (flag) {
    case fake_tcp::ERR_FLAG: {
      throw fake_tcp::invalid_header(
          "Invalid header! The message is discarded!");
    }
    case fake_tcp::SYN_FLAG: {
      std::cout << "[Server] Syn request received. "
                << "Sending back Seq and Ack..." << std::endl;
      // No body.
      unsigned char* send_buf = new unsigned char[HEADER_SIZE];
      bzero(send_buf, HEADER_SIZE);
      const uint32_t sequence_number = uniform_random(sequence_pool);

      *((uint8_t*)(send_buf)) = PROTOCOL_VERSION;
      *((uint8_t*)(send_buf + 1)) = fake_tcp::ACK_FLAG;
      *((uint16_t*)(send_buf + 2)) = 0b0;
      *((uint32_t*)(send_buf + 4)) = sequence_number;
      *((uint32_t*)(send_buf + 8)) = (seq + 1) % 0xffffffff;

      bool context = false;
      do_check(&context, dst_ip, src_ip, send_buf, HEADER_SIZE);

      sendto(socket, send_buf, HEADER_SIZE, 0, (struct sockaddr*)(&dst_addr),
             sizeof(sockaddr));

      // Start a timer.
      std::atomic_bool satisfied = false;
      // How many times does the server retry to send the packet back to the
      // client.
      std::uint32_t attempt_time = 0;

      // Thread t1 is used to resend packet.
      std::thread t1([&]() {
        while (satisfied == false &&
               attempt_time++ <= MAXIMUM_ESTABLISH_ATTEMPT) {
          // Create an inner thread.
          // Use signal.
          sendto(socket, send_buf, HEADER_SIZE, 0,
                 (struct sockaddr*)(&dst_addr), sizeof(sockaddr));
          std::this_thread::sleep_for(TIMEOUT);
        }

        if (satisfied == false) {
          std::cerr << "[Server] Handshake failed. The client does not respond."
                    << std::endl;
        }
      });

      // Thread t2 is a blocking thread that reads from the socket.
      std::thread t2([&]() {
        while (satisfied == false) {
          if (establish_connection_server(sequence_number) == 0) {
            satisfied = true;
          }
        }
      });

      t1.join();
      t2.join();

      return;
    }
    case fake_tcp::ACK_FLAG: {
      // This happens when the client needs to disconnect to the server.
      std::cout << "[Server] Ack received. "
                << "Sending back Seq and Ack..." << std::endl;
    }
    case fake_tcp::BEGIN_FLAG: {
      std::cout << "HELLO WORLD!\n";

      // Extract file information.
      const std::string file_name(
          reinterpret_cast<char*>(message + HEADER_SIZE));
      std::cout << "[Server] The client wants to upload file " << file_name
                << "!" << std::endl;
    }
  }
}

void fake_tcp::Party::response_client(unsigned char* message,
                                      const size_t& length,
                                      const uint32_t& src_ip,
                                      const uint32_t& dst_ip) {}

void fake_tcp::Party::send_file(const std::string& path_prefix) {
  // Get all the files from the directory.
  // Does not check if the path is a file.
  // Please make sure that the path is a directory.
  std::vector<std::string> files = create_files(path_prefix);

  // Set default timeout for the socket. For my convenience.
  struct timeval timeout;
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;
  // Try to set the timeout.
  setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

  for (uint32_t i = 0; i < files.size(); i++) {
    // Check if the file is valid.
    std::ifstream file =
        std::ifstream(files[i], std::ifstream::ate | std::ifstream::binary);
    if (file.good()) {
      // Create a buffer. Batch size is 1024 bytes. (1KB)
      unsigned char* send_buf = new unsigned char[BUFFER_SIZE];
      std::cout << "[Client] Sending file " << files[i] << " of size "
                << file.tellg() << "bytes..." << std::endl;

      while (!file.eof()) {
        // Read 1024 bytes from the file.
        file.read(reinterpret_cast<char*>(send_buf), BUFFER_SIZE);

        // Step 1: Send the file information to the server.
        send_file_information(files[i]);
      }
    }
  }
}

void fake_tcp::Party::send_file_information(const std::string& file_name) {
  // The buffer contains the filename.
  unsigned char* send_buf = new unsigned char[HEADER_SIZE + file_name.size()];
  bzero(send_buf, HEADER_SIZE + file_name.size());
  const uint32_t sequence_number = uniform_random(sequence_pool);

  *((uint8_t*)(send_buf)) = PROTOCOL_VERSION;
  *((uint8_t*)(send_buf + 1)) = fake_tcp::BEGIN_FLAG;
  *((uint16_t*)(send_buf + 2)) = 0b0;
  *((uint32_t*)(send_buf + 4)) = sequence_number;
  // Does not need to set ack bit.
  *((uint32_t*)(send_buf + 8)) = 0b0;

  // Insert message body.
  memcpy((void*)(send_buf + HEADER_SIZE), (void*)file_name.c_str(),
         file_name.size());

  bool context = false;
  do_check(&context, dst_ip, src_ip, send_buf, HEADER_SIZE + file_name.size());

  // Wait for ACK...
  
  sendto(socket, send_buf, HEADER_SIZE + file_name.size(), 0,
         (struct sockaddr*)(&dst_addr), sizeof(sockaddr));

  std::atomic_bool satisfied = false;
  std::thread t([&]() {
    std::cout << "[Client] Waiting for Ack..." << std::endl;
    
  });

  t.join();
}