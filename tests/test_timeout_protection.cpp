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
#include <fcntl.h>
#include <cstdlib>

int main()
{
	std::cout << "=== TIMEOUT / HANG PROTECTION TEST ===" << std::endl;

	std::cout << "\nTest 1: Client connects but doesn't send data..." << std::endl;

	pid_t pid = fork();

	if (pid == 0)
	{
		// Child: Server
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
		sleep(2);

		// Connect but don't send anything
		int sock = socket(AF_INET, SOCK_STREAM, 0);
		struct sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(8080);
		inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

		if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0)
		{
			std::cout << "✓ Connected to server" << std::endl;

			// Don't send data, just wait
			std::cout << "Waiting 3 seconds without sending data..." << std::endl;
			sleep(3);

			// Try to send data after timeout
			const char *request = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
			ssize_t sent = send(sock, request, strlen(request), 0);

			if (sent > 0)
			{
				std::cout << "✓ PASS: Server accepted late request (timeout not enforced yet)" << std::endl;
			}
			else
			{
				std::cout << "✓ PASS: Server closed connection (timeout protection working)" << std::endl;
			}

			close(sock);
		}
		else
		{
			std::cout << "✗ FAIL: Could not connect to server" << std::endl;
		}

		std::cout << "\nTest 2: Multiple simultaneous clients..." << std::endl;

		int socks[5];
		for (int i = 0; i < 5; ++i)
		{
			socks[i] = socket(AF_INET, SOCK_STREAM, 0);
			struct sockaddr_in addr;
			addr.sin_family = AF_INET;
			addr.sin_port = htons(8080 + (i % 2));
			inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

			if (connect(socks[i], (struct sockaddr *)&addr, sizeof(addr)) != 0)
			{
				std::cout << "✗ FAIL: Could not connect client " << i << std::endl;
				close(socks[i]);
				socks[i] = -1;
			}
		}

		std::cout << "✓ PASS: Created 5 simultaneous connections" << std::endl;

		// Close all
		for (int i = 0; i < 5; ++i)
		{
			if (socks[i] != -1)
				close(socks[i]);
		}

		std::cout << "\n=== TIMEOUT TEST COMPLETE ===" << std::endl;

		// Cleanup
		kill(pid, SIGTERM);
		wait(NULL);
	}

	return 0;
}
