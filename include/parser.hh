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
#ifndef PARSER_HH
#define PARSER_HH

#include <cxxopts.hh>

namespace fake_tcp {
class Command_parser final {
private:
    const int argc;

    const char** argv;

    cxxopts::Options* options;

    cxxopts::ParseResult result;

public:
    Command_parser() = delete;

    /**
     * @brief Construct a new Command_parser object
     * 
     * @param argc 
     * @param argv captured from 'main'
     */
    Command_parser(const int& argc, const char** argv);

    /**
     * @brief Parse the argument passed by the command line.
     * 
     */
    void parse(void);

    /**
     * @brief Get the result object
     * 
     * @return cxxopts::ParseResult 
     */
    cxxopts::ParseResult get_result(void) { return result; };
};
} // namespace fake_tcp

#endif