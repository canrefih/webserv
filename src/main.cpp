#include "Config.hpp"
#include "Server.hpp"
#include "Signal.hpp"

#include <iostream> // For std::cout, std::cerr
#include <csignal> // For signal handling

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		std::cerr << "Usage: ./webserv [configuration file]" << std::endl;
		return 1;
	}

	setupSignalHandlers();

	Config config; // Create a Config object to hold the server configuration

	if (!config.parse(argv[1]))
		return 1;

	Server server(config); // Create a Server object with the parsed configuration
	server.run(); // Start the server's main loop

	return 0;
}