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
#include <iostream>
#include <parser.hh>
#include <party.hh>

using fake_tcp::Command_parser;
using fake_tcp::Party;

// The client will send a file to the server.
int main(int argc, const char** argv) {
  try {
    Command_parser* const parser = new Command_parser(argc, argv);
    parser->parse();
    Party* const party = new Party(parser->get_result());
    party->run();
  } catch (const std::exception& e) {
    std::cerr << "Unexpected error: " << e.what() << std::endl;
    std::terminate();
  } catch (...) {
    std::terminate();
  }

  return 0;
}