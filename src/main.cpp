#include "Config.hpp"
#include "Server.hpp"

#include <iostream>

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		std::cerr << "Usage: ./webserv [configuration file]" << std::endl;
		return 1;
	}

	Config config;

	if (!config.parse(argv[1]))
		return 1;

	Server server(config);
	server.run();

	return 0;
}