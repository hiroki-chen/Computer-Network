#include "server.h"

int main(int argc, const char** argv)
{
	const std::shared_ptr<chat_room::Server> server = chat_room::Server::get_instance();

	server.get()->wait_for_new_connections();
}