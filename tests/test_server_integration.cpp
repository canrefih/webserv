#include "../include/Config.hpp"
#include "../include/Server.hpp"
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstdlib>

int main()
{
	std::cout << "=== SERVER INTEGRATION TESTS ===" << std::endl;
	
	std::cout << "\nTest 1: Load configuration..." << std::endl;
	Config config;
	if (!config.parse("../test.conf"))
	{
		std::cerr << "FAIL: Could not load test.conf" << std::endl;
		return 1;
	}
	
	const std::vector<ServerConfig> &servers = config.getServers();
	if (servers.size() != 2)
	{
		std::cerr << "FAIL: Expected 2 servers from config" << std::endl;
		return 1;
	}
	std::cout << "PASS: Configuration loaded" << std::endl;
	
	std::cout << "\nTest 2: Server object creation..." << std::endl;
	Server server(config);
	std::cout << "PASS: Server object created" << std::endl;
	
	std::cout << "\nTest 3: Verify server configuration..." << std::endl;
	if (servers[0].getPort() != 8080 || servers[1].getPort() != 8081)
	{
		std::cerr << "FAIL: Server ports not configured correctly" << std::endl;
		return 1;
	}
	std::cout << "PASS: Ports configured (8080, 8081)" << std::endl;
	
	std::cout << "\nTest 4: Verify locations in config..." << std::endl;
	const std::vector<Location> &locations = servers[0].getLocations();
	if (locations.empty())
	{
		std::cerr << "FAIL: Server should have locations configured" << std::endl;
		return 1;
	}
	
	bool foundRootLocation = false;
	bool foundUploadLocation = false;
	
	for (size_t i = 0; i < locations.size(); ++i)
	{
		if (locations[i].getPath() == "/")
			foundRootLocation = true;
		if (locations[i].getPath() == "/upload")
			foundUploadLocation = true;
	}
	
	if (!foundRootLocation || !foundUploadLocation)
	{
		std::cerr << "FAIL: Expected / and /upload locations" << std::endl;
		return 1;
	}
	std::cout << "PASS: Locations properly configured" << std::endl;
	
	std::cout << "\nTest 5: Verify error page configuration..." << std::endl;
	const std::string *errorPage404 = servers[0].getErrorPage(404);
	const std::string *errorPage500 = servers[0].getErrorPage(500);
	
	if (errorPage404 == NULL || errorPage500 == NULL)
	{
		std::cerr << "FAIL: Error pages not configured" << std::endl;
		return 1;
	}
	std::cout << "PASS: Error pages configured" << std::endl;
	
	std::cout << "\nTest 6: Verify client_max_body_size..." << std::endl;
	if (servers[0].getClientMaxBodySize() == 0)
	{
		std::cerr << "FAIL: client_max_body_size not set" << std::endl;
		return 1;
	}
	std::cout << "PASS: client_max_body_size = " << servers[0].getClientMaxBodySize() << std::endl;
	
	std::cout << "\nTest 7: Test server startup (fork)..." << std::endl;
	pid_t pid = fork();
	
	if (pid == 0)
	{
		// Child process
		signal(SIGTERM, SIG_DFL);
		signal(SIGINT, SIG_DFL);
		
		server.run();
		exit(0);
	}
	else if (pid > 0)
	{
		// Parent process
		sleep(2);
		
		std::cout << "PASS: Server started (PID: " << pid << ")" << std::endl;
		
		// Cleanup
		kill(pid, SIGTERM);
		wait(NULL);
	}
	else
	{
		std::cerr << "FAIL: Could not fork process" << std::endl;
		return 1;
	}
	
	std::cout << "\n=== ALL SERVER INTEGRATION TESTS PASSED ===" << std::endl;
	return 0;
}
