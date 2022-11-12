#include "client.h"
#include "utils.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <memory>

int main(int argc, const char** argv)
{
    try {
        std::cout << "Enter the receiver id (comma delimiter) and your uid!" << std::endl;
        std::string receiver_id;
        uint32_t uid;
        std::cin >> receiver_id >> uid;

        std::vector<std::string> id = split_string(receiver_id, ",");
        std::vector<uint32_t> id_vec;
        std::transform(id.begin(), id.end(), std::back_inserter(id_vec), [](const std::string& id) {
            return std::stoul(id);
        });

        const std::unique_ptr<chat_room::Client> client = std::make_unique<chat_room::Client>(id_vec, uid);
        while (true);
    } catch (const std::exception& e) {
        std::cout << e.what() << WSAGetLastError() << std::endl;
    }

    return 0;
}