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
#include <progress_bar.hh>
#include <protocol.hh>
#include <sstream>
#include <thread>
#include <timer.hh>
#include <utils.hh>
#include <vector>

using namespace std::literals::chrono_literals;

static std::unordered_map<uint32_t, bool> get_hashmap(const uint32_t& left,
                                                      const uint32_t& right) {
  std::unordered_map<uint32_t, bool> ret;

  for (uint32_t i = left; i <= right; i++) {
    ret[i] = false;
  }

  return ret;
}

void fake_tcp::Party::handle_batch_ack(
    const uint32_t& left, const uint32_t& right,
    std::vector<std::pair<unsigned char*, uint32_t>>& buffer_,
    const uint32_t& cur_sequence) {
  uint32_t acked = 0;
  std::unordered_map<uint32_t, bool> map = std::move(get_hashmap(left, right));

  do {
    send_file_content(buffer_, cur_sequence, map);
    std::this_thread::sleep_for(2 * TIMEOUT);

    uint32_t attempted = 0;

    while (attempted++ <= 3 * map.size()) {
      unsigned char* recv_buf = new unsigned char[HEADER_SIZE];
      if (handle_recv(socket, recv_buf, &addr, &dst_addr) == 0) {
        // Status ok, check ack flag.
        uint8_t flag = *((uint8_t*)(recv_buf + 1));

        if (flag != ACK_FLAG) {
          std::cout << "[Client] Received garbage message. Discard."
                    << std::endl;
        } else {
          // ACK received. Check validity.
          uint32_t ack = *((uint32_t*)(recv_buf + 8));
          uint32_t window_size = *((uint32_t*)(recv_buf + 16));
          this->window_size = window_size;
          std::cout << "[Client] Current window size is " << window_size
                    << std::endl;

          if (map.count(ack - 1) == 0) {
            std::cout << "[Client] Received garbage message. Discard. ACK = "
                      << ack << std::endl;
          } else if (map[ack - 1] == true) {
            std::cout << "[Client] Received repeated ack. Discard."
                      << std::endl;
          } else {
            std::cout << "[Client] Received valid ack!" << std::endl;
            // std::cout << ack << std::endl;
            acked++;
            map[ack - 1] = true;

            if (acked == map.size()) {
              return;
            }
          }
        }
      }
    }
  } while (true);
}

fake_tcp::Party::Party(const cxxopts::ParseResult& result)
    : type(result["server"].as<bool>()),
      input_file(new std::ifstream(result["input"].as<std::string>())),
      socket(-1),
      cur_sequence(0ul),
      window_left(0ul),
      window_right(MAXIMUM_WINDOW_SIZE),
      window_size(MAXIMUM_WINDOW_SIZE),
      next_sequence(0ul),
      last_acked(0ul) {
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
  ssize_t message_len = 0;

  // Delete upload folder.
  std::system("rm ../upload/*");

  do {
    try {
      // Prepare necessary variables for receiving the message.
      unsigned char* recv_buf = new unsigned char[BUFFER_SIZE];
      bzero(recv_buf, BUFFER_SIZE);
      handle_recv(socket, recv_buf, &addr, &dst_addr);

      // Extract source ip from the saddr struct.
      // uint32_t source_ip = ((struct
      // sockaddr_in*)(&addr))->sin_addr.s_addr; uint32_t destination_ip =
      // dst_addr.sin_addr.s_addr; Router is intercepting information...

      // Checksum ok. Response to the client.
      response_server(recv_buf, message_len, src_ip, dst_ip);
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
  exit(0);
}

int fake_tcp::Party::establish_connection_server(const uint32_t& sequence) {
  try {
    unsigned char* recv_buffer = new unsigned char[BUFFER_SIZE];
    handle_recv(socket, recv_buffer, &addr, &dst_addr);

    const uint8_t flag = *((uint8_t*)(recv_buffer + 1));
    const uint32_t seq = *((uint32_t*)(recv_buffer + 4));
    const uint32_t ack = *((uint32_t*)(recv_buffer + 8));

    if (flag == fake_tcp::ACK_FLAG && ack == sequence + 1) {
      std::cout << "[Server] Connection established!" << std::endl;

      established = true;
      return 0;
    } else if (flag == fake_tcp::BEGIN_FLAG) {
      // Send back reset flag.
      std::cout << "[Server] Not yet established! Try to send a RESET flag..."
                << std::endl;
      unsigned char* send_buf = new unsigned char[HEADER_SIZE];
      bzero(send_buf, HEADER_SIZE);

      // Set message header.
      *((uint8_t*)(send_buf)) = PROTOCOL_VERSION;
      *((uint8_t*)(send_buf + 1)) = fake_tcp::RST_FLAG;
      *((uint16_t*)(send_buf + 2)) = 0b0;
      *((uint32_t*)(send_buf + 4)) = 0b0;
      *((uint32_t*)(send_buf + 8)) = 0b0;

      bool context = false;
      do_check(&context, fake_tcp::dst_ip, fake_tcp::src_ip, send_buf,
               HEADER_SIZE);

      sendto(socket, send_buf, HEADER_SIZE, 0, (struct sockaddr*)(&dst_addr),
             sizeof(sockaddr));

      return -1;
    }

  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
}

int fake_tcp::Party::establish_connection_client(const uint32_t& sequence) {
  // Wait for server's ack.
  try {
    unsigned char* recv_buffer = new unsigned char[BUFFER_SIZE];

    if (handle_recv(socket, recv_buffer, &addr, &dst_addr) ==
        -1 /* Timed out. */) {
      return -1;
    }

    const uint8_t flag = *((uint8_t*)(recv_buffer + 1));
    // Received sequence.
    const uint32_t seq = *((uint32_t*)(recv_buffer + 4));
    const uint32_t ack = *((uint32_t*)(recv_buffer + 8));

    if (flag == fake_tcp::ACK_FLAG && ack == sequence + 1) {
      std::cout << "[Client] Handshake received! Sending Ack..." << std::endl;

      unsigned char* send_buf = new unsigned char[HEADER_SIZE];
      bzero(send_buf, HEADER_SIZE);
      const uint32_t sequence_number = cur_sequence++ % UINT32_MAX;

      *((uint8_t*)(send_buf)) = PROTOCOL_VERSION;
      *((uint8_t*)(send_buf + 1)) = fake_tcp::ACK_FLAG;
      *((uint16_t*)(send_buf + 2)) = 0b0;
      *((uint32_t*)(send_buf + 4)) = sequence_number;
      // Response to the sender.
      *((uint32_t*)(send_buf + 8)) = (seq + 1) % 0xffffffff;

      bool context = false;
      do_check(&context, src_ip, dst_ip, send_buf, HEADER_SIZE);

      send(socket, send_buf, HEADER_SIZE, 0);

      std::cout << "[Client] Connection established!" << std::endl;

      return 0;
    } else {
      throw fake_tcp::invalid_header("[Client] Seq and Ack are incorrect!");
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return -1;
  }
}

int fake_tcp::Party::connect_to_server() {
  // Send an SYN flag header to the server.
  const uint32_t sequence = cur_sequence++ % UINT32_MAX;
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

  while (establish_connection_client(sequence) != 0) {
    std::cout << "[Client] Waiting for Ack..." << std::endl;
    send(socket, header, HEADER_SIZE, 0);
  }

  return 0;
}

void fake_tcp::Party::check_storage_full(bool force_clear) {
  if (storage.size() == MAXIMUM_WINDOW_SIZE / BUFFER_SIZE || force_clear) {
    std::cout << "[Server] Flushing the cache..." << std::endl;
    for (auto item : storage) {
      auto buff = item.second;
      upload_files.back().write(reinterpret_cast<char*>(buff.first),
                                buff.second);
      upload_files.back().flush();
    }
    storage.clear();
  }

  if (force_clear && !upload_files.empty()) {
    upload_files.back().close();
  }
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
  // Print log.
  std::cout << "[Server] Received message: ACK = " << ack << ", SEQ = " << seq
            << std::endl;

  // The server is not ready.
  if (!established && flag != SYN_FLAG) {
    std::cout << "[Server] Not yet established! Try to send a RESET flag..."
              << std::endl;
    unsigned char* send_buf = new unsigned char[HEADER_SIZE];
    bzero(send_buf, HEADER_SIZE);

    // Set message header.
    *((uint8_t*)(send_buf)) = PROTOCOL_VERSION;
    *((uint8_t*)(send_buf + 1)) = fake_tcp::RST_FLAG;
    *((uint16_t*)(send_buf + 2)) = 0b0;
    *((uint32_t*)(send_buf + 4)) = 0b0;
    *((uint32_t*)(send_buf + 8)) = 0b0;

    bool context = false;
    do_check(&context, fake_tcp::dst_ip, fake_tcp::src_ip, send_buf,
             HEADER_SIZE);

    sendto(socket, send_buf, HEADER_SIZE, 0, (struct sockaddr*)(&dst_addr),
           sizeof(sockaddr));

    return;
  }

  /*---------------------
        ERR,  0b1
        SYN,  0b10
        ACK,  0b100
        RST,  0b1000
        FIN,  0b10000
        BEGIN,0b100000
        FILE, 0b1000000
  ----------------------*/
  switch (flag) {
    case fake_tcp::ERR_FLAG: {
      // The peer sent an error.
      throw fake_tcp::invalid_header(
          "Invalid header! The message is discarded!");
    }
    case fake_tcp::SYN_FLAG: {
      // Begin to connect.
      std::cout << "[Server] Syn request received. "
                << "Sending back Seq and Ack..." << std::endl;
      unsigned char* send_buf = new unsigned char[HEADER_SIZE];
      bzero(send_buf, HEADER_SIZE);
      const uint32_t sequence_number = cur_sequence++ % UINT32_MAX;

      // Set message header.
      *((uint8_t*)(send_buf)) = PROTOCOL_VERSION;
      *((uint8_t*)(send_buf + 1)) = fake_tcp::ACK_FLAG;
      *((uint16_t*)(send_buf + 2)) = 0b0;
      *((uint32_t*)(send_buf + 4)) = sequence_number;
      *((uint32_t*)(send_buf + 8)) = (seq + 1) % 0xffffffff;

      bool context = false;
      do_check(&context, fake_tcp::dst_ip, fake_tcp::src_ip, send_buf,
               HEADER_SIZE);

      sendto(socket, send_buf, HEADER_SIZE, 0, (struct sockaddr*)(&dst_addr),
             sizeof(sockaddr));

      std::this_thread::sleep_for(TIMEOUT);
      int response = -1;
      while (response != 0) {
        response = establish_connection_server(sequence_number);
        if (response == -1) {
          run_server();
        }
        std::this_thread::sleep_for(TIMEOUT);
      }

      return;
    }

    // The client is sending the file...
    case fake_tcp::FILE_FLAG: {
      // Set default timeout for the socket. Just for convenience.
      struct timeval timeout;
      timeout.tv_sec = 0;
      timeout.tv_usec = 1000000;
      // Try to set the timeout.
      setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
      // The message is received again. Throw it away.
      if (received_sequences.find(seq)) {
        std::cout << "[Server] Received repeated batch of the message."
                  << std::endl;
        return;
      }

      std::cout << "[Server] Received one batch of the message." << std::endl;
      // Check if this message is out of order.
      if (next_sequence == seq) {
        std::cout << "[Server] Sequence does match the expected one."
                  << std::endl;
        // Increment the next sequence expected.
        next_sequence++;
      }

      // Push to the queue.
      received_sequences.push_back(seq);
      const uint32_t message_len = *((uint32_t*)(message + 12));
      // Write to the storage.
      unsigned char* data = new unsigned char[message_len];
      memcpy(data, message + HEADER_SIZE, message_len);
      storage[seq] = std::make_pair(data, message_len);
      check_storage_full();

      // Send back ack.
      unsigned char* send_buf = new unsigned char[HEADER_SIZE];
      bzero(send_buf, HEADER_SIZE);
      // Set message header.
      *((uint8_t*)(send_buf)) = PROTOCOL_VERSION;
      *((uint8_t*)(send_buf + 1)) = fake_tcp::ACK_FLAG;
      *((uint16_t*)(send_buf + 2)) = 0b0;
      *((uint32_t*)(send_buf + 8)) = (seq + 1) % 0xffffffff;
      *((uint32_t*)(send_buf + 16)) =
          MAXIMUM_WINDOW_SIZE / BUFFER_SIZE - storage.size();

      bool context = false;
      do_check(&context, fake_tcp::dst_ip, fake_tcp::src_ip, send_buf,
               HEADER_SIZE);

      sendto(socket, send_buf, HEADER_SIZE, 0, (struct sockaddr*)(&dst_addr),
             sizeof(sockaddr));
      return;
    }

    case fake_tcp::BEGIN_FLAG: {
      // Extract file information.
      const std::string file_name(
          reinterpret_cast<char*>(message + HEADER_SIZE));
      std::cout << "[Server] The client wants to upload file " << file_name
                << "!" << std::endl;

      received_sequences.clear();
      check_storage_full(true);

      // Send back the ack flag.
      unsigned char* send_buf = new unsigned char[HEADER_SIZE];
      bzero(send_buf, HEADER_SIZE);
      // Set message header.
      *((uint8_t*)(send_buf)) = PROTOCOL_VERSION;
      *((uint8_t*)(send_buf + 1)) = fake_tcp::ACK_FLAG;
      *((uint16_t*)(send_buf + 2)) = 0b0;
      *((uint32_t*)(send_buf + 4)) = seq;
      *((uint32_t*)(send_buf + 8)) = seq + 1;
      // Set window size.
      *((uint32_t*)(send_buf + 16)) = MAXIMUM_WINDOW_SIZE / BUFFER_SIZE;

      std::cout << "[Server] Sending back with ACK " << seq + 1 << std::endl;

      bool context = false;  // Indicate that we need to set the checksum.
      do_check(&context, fake_tcp::src_ip, fake_tcp::dst_ip, send_buf,
               HEADER_SIZE);

      // Send it back.
      sendto(socket, send_buf, HEADER_SIZE, 0, (struct sockaddr*)(&dst_addr),
             sizeof(sockaddr));
      // Add to the file list.
      if (this->file_name.find(file_name) == this->file_name.end()) {
        // Create new file.
        this->file_name.insert(file_name);
        upload_files.emplace_back(
            std::ofstream(default_upload_path + file_name,
                          std::ofstream::binary | std::ofstream::out));
        std::cout << "[Server] Created new file!" << std::endl;
      }
      next_sequence = 1ul;

      return;
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

  // Set default timeout for the socket. Just for convenience.
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 100000;
  // Try to set the timeout.
  setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  for (uint32_t i = 0; i < files.size(); i++) {
    // Check if the file is valid.
    std::ifstream file(path_prefix + "/" + files[i],
                       std::ifstream::in | std::ifstream::binary);
    if (file.good()) {
      // Get the file size.
      file.seekg(0, std::ifstream::end);
      const std::streamsize file_size = file.tellg();
      file.seekg(0, std::ifstream::beg);

      // Create a buffer. Batch size is 1024 bytes. (1KB)
      unsigned char* send_buf = new unsigned char[BUFFER_SIZE - HEADER_SIZE];
      std::cout << std::endl
                << std::flush << "[Client] Sending file " << files[i]
                << " with size " << file_size << " bytes." << std::endl;

      cur_sequence = 0ul;

      // Step 1: Send the file information to the server.
      send_file_information(files[i]);

      std::atomic<std::streamsize> read_size = 0;

      // Create a progress bar.
      // ProgressBar bar(file_size, files[i].data());

      // Set a time recorder.
      auto begin = std::chrono::high_resolution_clock::now();
      bool finish = false;
      while (!finish) {
        // Create a buffer pool.
        std::vector<std::pair<unsigned char*, uint32_t>> buffer_;

        std::cout << window_size << std::endl;

        for (uint32_t i = 0; i < window_size; i++) {
          bzero(send_buf, BUFFER_SIZE - HEADER_SIZE);
          // Read 1024 - HEADERSIZE bytes from the file.
          // Reserve some place for the buffer.
          file.read(reinterpret_cast<char*>(send_buf),
                    BUFFER_SIZE - HEADER_SIZE);
          unsigned char* buf = new unsigned char[BUFFER_SIZE - HEADER_SIZE];
          memcpy(buf, send_buf, BUFFER_SIZE - HEADER_SIZE);
          std::streamsize count = file.gcount();
          // If nothing has been read, break
          if (!count) {
            finish = true;
            break;
          }
          buffer_.emplace_back(std::make_pair(buf, count));
          // Increment progress bar.
          read_size += count;
          // bar.Progressed(read_size);
        }

        // Step 2: Begin to send file content to the server.
        // In Lab2, we use a sliding window.
        // Set and current sent window.
        uint32_t left = cur_sequence;
        uint32_t right = cur_sequence + buffer_.size() - 1;

        std::cout << left << ", " << right << std::endl;

        handle_batch_ack(left, right, buffer_, cur_sequence);
        // Increment.
        cur_sequence += buffer_.size();
      }
      auto end = std::chrono::high_resolution_clock::now();
      double elapsed = std::chrono::duration<double>(end - begin).count();

      std::cout << std::endl
                << std::flush << "\033[1;31m[Client] Successfully sent a file!"
                << " Time used: " << elapsed << " s."
                << " Speed: " << file_size / elapsed << " B/s.\033[0m"
                << std::endl;
    }
  }
}

void fake_tcp::Party::send_file_information(const std::string& file_name) {
  // The buffer contains the filename.
  unsigned char* send_buf = new unsigned char[HEADER_SIZE + file_name.size()];
  bzero(send_buf, HEADER_SIZE + file_name.size());
  const uint32_t sequence_number = cur_sequence++ % UINT32_MAX;

  // Set message header.
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
  bool is_lost = false;
  bool is_reset = false;
  do {
    // Send the message to the server.
    send(socket, send_buf, HEADER_SIZE + file_name.size(), 0);
    // Sleep for a while and wait for the server.
    //  std::this_thread::sleep_for(500ms);
    // Prepare some variables.
    ssize_t message_len = 0;
    unsigned char* recv_buffer = new unsigned char[BUFFER_SIZE];

    // Receive from socket.
    message_len = recv(socket, recv_buffer, BUFFER_SIZE, 0);
    // Check status.
    if (message_len > 0) {
      // Status ok, check ack flag.
      uint8_t flag = *((uint8_t*)(recv_buffer + 1));
      // Received a reset flag: try to reconnect.
      if (flag == RST_FLAG) {
        std::cout << "[Client] Received reset flag! The handshake is not yet "
                     "completed."
                  << std::endl;
        is_reset = true;
        break;
      }
      if (flag != ACK_FLAG) {
        std::cout << "[Client] Received garbage message. Discard." << std::endl;
        continue;
      } else {
        // ACK received. Check validity.
        uint32_t ack = *((uint32_t*)(recv_buffer + 8));
        if (ack != sequence_number + 1) {
          std::cout << "ACK: " << ack << " Seq: " << sequence_number
                    << std::endl;
          std::cout << "[Client] Server sent an invalid ack to the client!"
                    << std::endl;
          continue;
        } else {
          // ACK received and it is valid!
          std::cout << "[Client] Successfully sent file information!"
                    << std::endl;
          const uint32_t window_size_cur = *((uint32_t*)(recv_buffer + 16));
          std::cout << "[Client] Current window size is " << window_size_cur
                    << std::endl;
          window_size = window_size_cur;
          break;
        }
      }
    } else if (message_len < 0) {
      // The socket may have timed out.
      // Check errno.
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
        std::cout << "[Client] The server does not respond!" << std::endl;
        continue;
      } else {
        std::cout << "[Client] Internal error with the socket! Terminating..."
                  << std::endl;
        exit(1);
      }
    } else {
      // The peer closed the connection. We should invoke
      // establish_connection and then break.
      std::cout << "[Client] The server is lost... Trying to reconnect!"
                << std::endl;
      // Set the flag to be true.
      is_lost = true;
      break;
    }
  } while (true);

  // If the server is lost, we re-run the client again.
  if (is_lost || is_reset) {
    run_client();
  }
}

void fake_tcp::Party::send_file_content(
    const std::vector<std::pair<unsigned char*, uint32_t>>& buffer_,
    const uint32_t& start, const std::unordered_map<uint32_t, bool>& map) {
  uint32_t cur = start;
  for (auto buffer : buffer_) {
    if (map.at(cur) != 0) {
      cur++;
      continue;
    }

    // Build a send buffer.
    unsigned char* send_buf = new unsigned char[buffer.second + HEADER_SIZE];
    bzero(send_buf, HEADER_SIZE + buffer.second);
    const uint32_t sequence_number = cur++;

    // Set message header.
    *((uint8_t*)(send_buf)) = PROTOCOL_VERSION;
    *((uint8_t*)(send_buf + 1)) = fake_tcp::FILE_FLAG;
    *((uint16_t*)(send_buf + 2)) = 0b0;
    *((uint32_t*)(send_buf + 4)) = sequence_number;
    *((uint32_t*)(send_buf + 12)) = buffer.second;

    // Append the message body to the header.
    memcpy(send_buf + HEADER_SIZE, buffer.first, buffer.second);
    bool context = false;
    do_check(&context, src_ip, dst_ip, send_buf, buffer.second + HEADER_SIZE);
    send(socket, send_buf, buffer.second + HEADER_SIZE, 0);

    std::this_thread::sleep_for(TIMEOUT);
  }
}

fake_tcp::Party::~Party() {
  for (auto& file : upload_files) {
    file.flush();
  }
}