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
#include <csignal>
#include <iostream>
#include <parser.hh>
#include <party.hh>
#include <thread>

using fake_tcp::Command_parser;
using fake_tcp::Party;

std::sig_atomic_t sig_exit = 0;

void handler(std::sig_atomic_t t) {
  std::cerr << std::endl
            << std::flush << "[@main] SIGINT: will exit..." << std::endl;
  sig_exit = 1;
}

// The client will send a file to the server.
int main(int argc, const char** argv) {
  // Register SIGINT handler.
  std::signal(SIGINT, handler);

  try {
    std::thread main_thread([&] {
      Command_parser* const parser = new Command_parser(argc, argv);
      parser->parse();
      Party* const party = new Party(parser->get_result());
      party->run();
    });

    main_thread.detach();

    while (!sig_exit)
      ;
    return 0;
  } catch (...) {
    return 1;
  }
}