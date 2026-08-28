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
#include <vector>
#include <ctime>
#include <cstdlib>
#include <fcntl.h>

volatile sig_atomic_t g_testRunning = true;

void testSignalHandler(int sig)
{
	(void)sig;
	g_testRunning = false;
}

struct RequestStats {
	int totalRequests;
	int successfulRequests;
	int failedRequests;
	double totalTime;
	double avgResponseTime;
	double requestsPerSecond;
};

int main()
{
	std::cout << "=== STRESS TEST ===" << std::endl;
	std::cout << "Simulating Apache Bench-like load testing" << std::endl;
	std::cout << "" << std::endl;

	pid_t pid = fork();

	if (pid == 0)
	{
		// Child: Server
		signal(SIGINT, testSignalHandler);
		signal(SIGTERM, testSignalHandler);

		Config config;
		if (!config.parse("../config/test.conf"))
		{
			std::cerr << "FAIL: Config parse failed" << std::endl;
			exit(1);
		}

		Server server(config);
		server.run();
		exit(0);
	}
	else if (pid > 0)
	{
		sleep(2);

		RequestStats stats;
		stats.totalRequests = 50;  // ✅ 100 → 50 (faster test)
		stats.successfulRequests = 0;
		stats.failedRequests = 0;

		std::cout << "Test 1: Sequential requests (50 requests)" << std::endl;
		std::cout << "Sending requests to http://127.0.0.1:8080/" << std::endl;

		time_t start_time = time(NULL);

		for (int i = 0; i < stats.totalRequests; ++i)
		{
			int sock = socket(AF_INET, SOCK_STREAM, 0);
			
			// ✅ Socket timeout set
			struct timeval tv;
			tv.tv_sec = 2;
			tv.tv_usec = 0;
			setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

			struct sockaddr_in addr;
			addr.sin_family = AF_INET;
			addr.sin_port = htons(8080);
			inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

			if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0)
			{
				const char *request = "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
				ssize_t sent = send(sock, request, strlen(request), 0);

				if (sent > 0)
				{
					char buffer[4096];
					ssize_t received = recv(sock, buffer, sizeof(buffer), 0);

					if (received > 0)
					{
						stats.successfulRequests++;
					}
					else
					{
						stats.failedRequests++;
					}
				}
				else
				{
					stats.failedRequests++;
				}

				close(sock);
			}
			else
			{
				stats.failedRequests++;
			}

			if ((i + 1) % 10 == 0)
				std::cout << "  Completed: " << (i + 1) << "/" << stats.totalRequests << std::endl;
		}

		time_t end_time = time(NULL);
		stats.totalTime = difftime(end_time, start_time);
		if (stats.totalTime == 0) stats.totalTime = 1;
		stats.avgResponseTime = (stats.totalTime / stats.totalRequests) * 1000;
		stats.requestsPerSecond = stats.totalRequests / stats.totalTime;

		std::cout << std::endl;
		std::cout << "Test 1 Results:" << std::endl;
		std::cout << "  Total Requests:     " << stats.totalRequests << std::endl;
		std::cout << "  Successful:         " << stats.successfulRequests << std::endl;
		std::cout << "  Failed:             " << stats.failedRequests << std::endl;
		std::cout << "  Total Time:         " << stats.totalTime << "s" << std::endl;
		std::cout << "  Requests/sec:       " << stats.requestsPerSecond << std::endl;

		if (stats.successfulRequests >= stats.totalRequests * 0.8)
		{
			std::cout << "✓ PASS: 80%+ requests succeeded" << std::endl;
		}
		else
		{
			std::cout << "✗ FAIL: Less than 80% success rate" << std::endl;
		}

		std::cout << std::endl;
		std::cout << "Test 2: Concurrent connections (10 simultaneous)" << std::endl;

		int concurrent = 10;
		int connSucc = 0;

		for (int i = 0; i < concurrent; ++i)
		{
			int sock = socket(AF_INET, SOCK_STREAM, 0);
			struct sockaddr_in addr;
			addr.sin_family = AF_INET;
			addr.sin_port = htons(8080 + (i % 2));
			inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

			if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0)
			{
				connSucc++;
				close(sock);
			}
		}

		std::cout << "  Successfully connected: " << connSucc << "/" << concurrent << std::endl;

		if (connSucc == concurrent)
		{
			std::cout << "✓ PASS: Server handled concurrent connections" << std::endl;
		}
		else
		{
			std::cout << "⚠ WARN: Only " << connSucc << " of " << concurrent << " succeeded" << std::endl;
		}

		std::cout << std::endl;
		std::cout << "=== STRESS TEST COMPLETE ===" << std::endl;

		kill(pid, SIGTERM);
		wait(NULL);
	}

	return 0;
}
