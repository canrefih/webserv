#ifndef SIGNAL_HPP
#define SIGNAL_HPP

#include <csignal>

extern volatile sig_atomic_t g_serverRunning;

void signalHandler(int sig);
void setupSignalHandlers();

#endif