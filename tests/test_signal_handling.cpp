#include "../include/Config.hpp"
#include "../include/Server.hpp"
#include "../include/Signal.hpp"

#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <csignal>
#include <cstdlib>

int main()
{
	std::cout << "=== SIGNAL HANDLING TEST ===" << std::endl;

	pid_t pid = fork();

	if (pid == 0)
	{
		Config config;
		if (!config.parse("../config/test.conf"))
		{
			std::cerr << "FAIL: Config parse failed" << std::endl;
			exit(1);
		}

		setupSignalHandlers();

		Server server(config);
		server.run();
		exit(0);
	}
	else if (pid > 0)
	{
		sleep(3);

		std::cout << "Sending SIGINT to server (PID "
				  << pid << ")..." << std::endl;

		kill(pid, SIGTERM);

		int status;
		waitpid(pid, &status, 0);

		if (WIFEXITED(status))
		{
			std::cout << "✓ PASS: Server exited gracefully on SIGINT"
					  << std::endl;
		}
		else
		{
			std::cout << "✗ FAIL: Server terminated by signal (status: "
					  << status << ")" << std::endl;
		}
	}

	return 0;
}