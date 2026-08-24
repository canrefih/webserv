#include "../include/Config.hpp"
#include "../include/Server.hpp"
#include "../include/HttpRequest.hpp"
#include "../include/HttpResponse.hpp"
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
	std::cout << "=== KEEP-ALIVE TEST ===" << std::endl;

	std::cout << "\nTest 1: HttpResponse setHeader() for Keep-Alive..." << std::endl;

	HttpResponse resp;
	resp.setStatus(200, "OK");
	resp.setBody("Hello");
	resp.setContentType("text/plain");
	resp.setHeader("Connection", "keep-alive");

	std::string output = resp.toString();

	if (output.find("Connection: keep-alive") != std::string::npos)
	{
		std::cout << "✓ PASS: Custom Connection header set correctly" << std::endl;
	}
	else
	{
		std::cout << "✗ FAIL: Connection header not found in response" << std::endl;
	}

	std::cout << "\nTest 2: Multiple requests on same connection..." << std::endl;

	pid_t pid = fork();

	if (pid == 0)
	{
		// Child: Server
		Config config;
		if (!config.parse("../test.conf"))
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

		int sock = socket(AF_INET, SOCK_STREAM, 0);
		struct sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(8080);
		inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

		if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0)
		{
			std::cout << "✓ Connected" << std::endl;

			// Request 1
			const char *req1 = "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: keep-alive\r\n\r\n";
			send(sock, req1, strlen(req1), 0);

			char buffer[4096];
			ssize_t rec1 = recv(sock, buffer, sizeof(buffer), 0);

			if (rec1 > 0)
			{
				std::cout << "✓ Received first response" << std::endl;

				// Request 2 on same connection
				const char *req2 = "GET /www/testdir HTTP/1.1\r\nHost: localhost\r\n\r\n";
				send(sock, req2, strlen(req2), 0);

				ssize_t rec2 = recv(sock, buffer, sizeof(buffer), 0);

				if (rec2 > 0)
				{
					std::cout << "✓ PASS: Received second response on same connection (keep-alive working)" << std::endl;
				}
				else
				{
					std::cout << "⚠ Connection closed after first request (keep-alive not implemented yet)" << std::endl;
				}
			}

			close(sock);
		}

		kill(pid, SIGTERM);
		wait(NULL);

		std::cout << "\n=== KEEP-ALIVE TEST COMPLETE ===" << std::endl;
	}

	return 0;
}
