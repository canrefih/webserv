#include "../include/Config.hpp"
#include "../include/Server.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <signal.h>
#include <sys/wait.h>
#include <cstdlib>

int main()
{
	std::cout << "=== MULTI-SOCKET LISTENING TEST ===" << std::endl;

	std::cout << "\nTest: Server listening on multiple ports..." << std::endl;

	pid_t pid = fork();

	if (pid == 0)
	{
		// Child: Server çalışsın
		Config config;
		if (!config.parse("../config/test.conf"))
		{
			std::cerr << "FAIL: Could not parse config" << std::endl;
			exit(1);
		}

		Server server(config);
		server.run();
		exit(0);
	}
	else if (pid > 0)
	{
		// Parent: Test et
		sleep(2);

		std::cout << "\nConnecting to port 8080..." << std::endl;
		int sock1 = socket(AF_INET, SOCK_STREAM, 0);
		struct sockaddr_in addr1;
		addr1.sin_family = AF_INET;
		addr1.sin_port = htons(8080);
		inet_pton(AF_INET, "127.0.0.1", &addr1.sin_addr);

		if (connect(sock1, (struct sockaddr *)&addr1, sizeof(addr1)) == 0)
		{
			std::cout << "✓ PASS: Connected to port 8080" << std::endl;
			close(sock1);
		}
		else
		{
			std::cout << "✗ FAIL: Could not connect to port 8080" << std::endl;
		}

		std::cout << "\nConnecting to port 8081..." << std::endl;
		int sock2 = socket(AF_INET, SOCK_STREAM, 0);
		struct sockaddr_in addr2;
		addr2.sin_family = AF_INET;
		addr2.sin_port = htons(8081);
		inet_pton(AF_INET, "127.0.0.1", &addr2.sin_addr);

		if (connect(sock2, (struct sockaddr *)&addr2, sizeof(addr2)) == 0)
		{
			std::cout << "✓ PASS: Connected to port 8081" << std::endl;
			close(sock2);
		}
		else
		{
			std::cout << "✗ FAIL: Could not connect to port 8081" << std::endl;
		}

		std::cout << "\nConnecting to port 8080 again..." << std::endl;
		int sock3 = socket(AF_INET, SOCK_STREAM, 0);
		struct sockaddr_in addr3;
		addr3.sin_family = AF_INET;
		addr3.sin_port = htons(8080);
		inet_pton(AF_INET, "127.0.0.1", &addr3.sin_addr);

		if (connect(sock3, (struct sockaddr *)&addr3, sizeof(addr3)) == 0)
		{
			std::cout << "✓ PASS: Connected to port 8080 (multiple connections work)" << std::endl;
			close(sock3);
		}
		else
		{
			std::cout << "✗ FAIL: Could not connect to port 8080 again" << std::endl;
		}

		std::cout << "\n=== MULTI-SOCKET TEST COMPLETE ===" << std::endl;

		// Cleanup
		kill(pid, SIGTERM);
		wait(NULL);
	}
	else
	{
		std::cerr << "FAIL: Could not fork" << std::endl;
		return 1;
	}

	return 0;
}
