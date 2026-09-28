#include "../include/Signal.hpp"

#include <iostream>
#include <csignal>

int main()
{
    std::cout << "=== SIGNAL UNIT TESTS ===" << std::endl;

    std::cout << "\nTest 1: Initial running state..." << std::endl;

    g_serverRunning = true;

    if (!g_serverRunning)
    {
        std::cerr << "FAIL: Server should start in running state" << std::endl;
        return 1;
    }

    std::cout << "PASS: Initial state correct" << std::endl;

    std::cout << "\nTest 2: SIGINT handler..." << std::endl;

    g_serverRunning = true;
    signalHandler(SIGINT);

    if (g_serverRunning)
    {
        std::cerr << "FAIL: SIGINT should stop server" << std::endl;
        return 1;
    }

    std::cout << "PASS: SIGINT handler working" << std::endl;

    std::cout << "\nTest 3: SIGTERM handler..." << std::endl;

    g_serverRunning = true;
    signalHandler(SIGTERM);

    if (g_serverRunning)
    {
        std::cerr << "FAIL: SIGTERM should stop server" << std::endl;
        return 1;
    }

    std::cout << "PASS: SIGTERM handler working" << std::endl;

    std::cout << "\nTest 4: Handler remains stopped..." << std::endl;

    signalHandler(SIGINT);

    if (g_serverRunning)
    {
        std::cerr << "FAIL: Handler should keep server stopped"
                  << std::endl;
        return 1;
    }

    std::cout << "PASS: Handler state correct" << std::endl;

    std::cout << "\nTest 5: setupSignalHandlers..." << std::endl;

    g_serverRunning = true;
    setupSignalHandlers();

    raise(SIGINT);

    if (g_serverRunning)
    {
        std::cerr << "FAIL: SIGINT handler was not installed"
                  << std::endl;
        return 1;
    }

    std::cout << "PASS: SIGINT handler installed" << std::endl;

    g_serverRunning = true;

    raise(SIGTERM);

    if (g_serverRunning)
    {
        std::cerr << "FAIL: SIGTERM handler was not installed"
                  << std::endl;
        return 1;
    }

    std::cout << "PASS: SIGTERM handler installed" << std::endl;

    std::cout << "\n=== ALL SIGNAL UNIT TESTS PASSED ==="
              << std::endl;

    return 0;
}