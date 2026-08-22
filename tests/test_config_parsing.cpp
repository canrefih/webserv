#include "../include/Config.hpp"
#include <iostream>
#include <cassert>

int main()
{
	std::cout << "=== CONFIG PARSING TESTS ===" << std::endl;
	
	Config config;
	
	std::cout << "\nTest 1: Parse valid config file..." << std::endl;
	if (!config.parse("config/test.conf"))
	{
		std::cerr << "FAIL: Could not parse test.conf" << std::endl;
		return 1;
	}
	std::cout << "PASS: Config parsed successfully" << std::endl;
	
	std::cout << "\nTest 2: Check server count..." << std::endl;
	const std::vector<ServerConfig> &servers = config.getServers();
	if (servers.size() != 2)
	{
		std::cerr << "FAIL: Expected 2 servers, got " << servers.size() << std::endl;
		return 1;
	}
	std::cout << "PASS: Found 2 servers" << std::endl;
	
	std::cout << "\nTest 3: Check first server config..." << std::endl;
	if (servers[0].getHost() != "127.0.0.1" ||
	    servers[0].getPort() != 8080 ||
	    servers[0].getRoot() != "./www" ||
	    servers[0].getIndex() != "index.html" ||
	    servers[0].getAutoIndex() != false)
	{
		std::cerr << "FAIL: Server 1 config incorrect" << std::endl;
		return 1;
	}
	std::cout << "PASS: Server 1 config correct" << std::endl;
	
	std::cout << "\nTest 4: Check client_max_body_size..." << std::endl;
	if (servers[0].getClientMaxBodySize() != 1048576)
	{
		std::cerr << "FAIL: Expected 1M (1048576), got " << servers[0].getClientMaxBodySize() << std::endl;
		return 1;
	}
	std::cout << "PASS: client_max_body_size = 1M" << std::endl;
	
	std::cout << "\nTest 5: Check second server config..." << std::endl;
	if (servers[1].getPort() != 8081 ||
	    servers[1].getAutoIndex() != true ||
	    servers[1].getClientMaxBodySize() != 5242880)
	{
		std::cerr << "FAIL: Server 2 config incorrect" << std::endl;
		return 1;
	}
	std::cout << "PASS: Server 2 config correct" << std::endl;
	
	std::cout << "\nTest 6: Test getServerByPort..." << std::endl;
	const ServerConfig *server = config.getServerByPort(8080);
	if (server == NULL || server->getPort() != 8080)
	{
		std::cerr << "FAIL: getServerByPort(8080) failed" << std::endl;
		return 1;
	}
	std::cout << "PASS: getServerByPort working correctly" << std::endl;
	
	std::cout << "\nTest 7: Test getServerByPort with non-existent port..." << std::endl;
	server = config.getServerByPort(9999);
	if (server != NULL)
	{
		std::cerr << "FAIL: getServerByPort(9999) should return NULL" << std::endl;
		return 1;
	}
	std::cout << "PASS: getServerByPort correctly returns NULL" << std::endl;
	
	std::cout << "\n=== ALL CONFIG PARSING TESTS PASSED ===" << std::endl;
	return 0;
}
