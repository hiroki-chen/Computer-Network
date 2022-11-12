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
#include <parser.hh>

fake_tcp::Command_parser::Command_parser(const int& argc, const char** argv)
    : argc(argc), argv(argv) {
  options = new cxxopts::Options(
      "fakequic_protocol",
      "--------- A UPD-based Fake QUIC Protocol Simulator ---------");
  options->add_options()("s,server", "The type of the party.",
                         cxxopts::value<bool>()->default_value("false"))(
      "a,address", "The IP of the party.",
      cxxopts::value<std::string>()->default_value("127.0.0.1"))(
      "p,port", "The port of the party.",
      cxxopts::value<std::string>()->default_value("1234"))(
      "i,input", "The file you want to send.",
      cxxopts::value<std::string>()->default_value("./test/test_file"))(
      "h,help", "Print help information.");
}

void fake_tcp::Command_parser::parse(void) {
  result = options->parse(argc, argv);

  if (result.count("help")) {
    std::cout << options->help() << std::endl;
    exit(0);
  }
}
