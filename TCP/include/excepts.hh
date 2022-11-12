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
#ifndef EXCEPTS_HH
#define EXCEPTS_HH

#include <stdexcept>
#include <string>

namespace fake_tcp {
class invalid_header : public std::exception {
private:
    const std::string information;

public:
    invalid_header() = delete;

    invalid_header(const std::string& information)
        : information(information)
    {
    }

    const char* what(void) const noexcept override { return information.c_str(); }
};

class connection_reset : public std::exception {
private:
    const std::string information;

public:
    connection_reset() = delete;

    connection_reset(const std::string& information)
        : information(information)
    {
    }

    const char* what(void) const noexcept override { return information.c_str(); }
};

class socket_error : public std::exception {
private:
    const std::string information;

public:
    socket_error() = delete;

    socket_error(const std::string& information)
        : information(information)
    {
    }

    const char* what(void) const noexcept override { return information.c_str(); }
};

class connect_timeout : public std::exception {
private:
    const std::string information;

public:
    connect_timeout() = delete;

    connect_timeout(const std::string& information)
        : information(information)
    {
    }

    const char* what(void) const noexcept override { return information.c_str(); }
};


class connect_closed : public std::exception {
private:
    const std::string information;

public:
    connect_closed() = delete;

    connect_closed(const std::string& information)
        : information(information)
    {
    }

    const char* what(void) const noexcept override { return information.c_str(); }
};
} // namespace fake_tcp

#endif