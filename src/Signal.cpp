#include "Signal.hpp"

volatile sig_atomic_t g_serverRunning = true;

void signalHandler(int sig)
{
	(void)sig;
	g_serverRunning = false;
}

void setupSignalHandlers()
{
	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);
}