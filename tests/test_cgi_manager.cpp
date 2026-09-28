#include "CGIManager.hpp"
#include "HttpRequest.hpp"
#include "ServerConfig.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/wait.h>
#include <sstream>

/*
 * CGIManager integration tests.
 *
 * These tests exercise:
 *
 *  1. GET CGI
 *  2. POST CGI
 *  3. Query string
 *  4. Large request body
 *  5. Large CGI output
 *  6. CGI non-zero exit
 *  7. CGI crash
 *  8. Empty CGI output
 *  9. Client disconnect / abortForClient()
 * 10. Duplicate CGI start for same client
 *
 * The tests drive CGIManager through poll(), just like Server would.
 */

static int g_tests = 0;
static int g_passed = 0;

static void check(bool condition, const std::string &name)
{
    ++g_tests;

    if (condition)
    {
        ++g_passed;
        std::cout << "[PASS] " << name << std::endl;
    }
    else
        std::cout << "[FAIL] " << name << std::endl;
}

static HttpRequest makeRequest(const std::string &raw)
{
    HttpRequest request;

    if (!request.parse(raw))
    {
        std::cerr << "[ERROR] HttpRequest::parse() failed." << std::endl;
        std::exit(1);
    }

    return (request);
}

static bool contains(const std::string &str, const std::string &needle)
{
    return (str.find(needle) != std::string::npos);
}

/*
 * Run the manager until a ready response appears.
 *
 * This simulates the relevant part of Server's poll loop.
 */
static bool driveUntilReady(CGIManager &manager,
                            std::vector<pollfd> &pollFds,
                            int &clientFd,
                            std::string &response,
                            bool &keepAlive,
                            int timeoutMs)
{
    const int stepMs = 50;
    int elapsed = 0;

    while (elapsed < timeoutMs)
    {
        if (manager.popReady(clientFd, response, keepAlive))
            return (true);

        manager.checkTimeouts(pollFds);

        if (pollFds.empty())
        {
            usleep(stepMs * 1000);
            elapsed += stepMs;
            continue;
        }

        int ret = poll(&pollFds[0], pollFds.size(), stepMs);

        if (ret < 0)
        {
            if (errno == EINTR)
                continue;

            std::cerr << "[ERROR] poll() failed: "
                      << strerror(errno) << std::endl;
            return (false);
        }

        if (ret == 0)
        {
            elapsed += stepMs;
            continue;
        }

        /*
         * Copy the events first because handleEvent() is allowed
         * to erase entries from pollFds.
         */
        std::vector<std::pair<int, short> > ready;

        for (std::size_t i = 0; i < pollFds.size(); ++i)
        {
            if (pollFds[i].revents != 0)
                ready.push_back(
                    std::make_pair(pollFds[i].fd, pollFds[i].revents)
                );
        }

        for (std::size_t i = 0; i < ready.size(); ++i)
            manager.handleEvent(
                ready[i].first,
                ready[i].second,
                pollFds
            );

        elapsed += stepMs;
    }

    return (manager.popReady(clientFd, response, keepAlive));
}

/*
 * Test 1:
 * Basic GET CGI.
 */
static void testGet()
{
    std::cout << "\n--- Test 1: GET CGI ---" << std::endl;

    CGIManager manager;
    std::vector<pollfd> pollFds;

    ServerConfig config;

    HttpRequest request = makeRequest(
        "GET /cgi-bin/test_echo.py HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "User-Agent: CGIManagerTest\r\n"
        "\r\n"
    );

    /*
     * Use a fake client fd. CGIManager only needs the fd as an ID.
     */
    const int clientFd = 1001;

    std::string immediateError;

    bool started = manager.start(
        clientFd,
        request,
        config,
        "www/cgi-bin/test_echo.py",
        "/usr/bin/python3",
        pollFds,
        immediateError
    );

    check(started, "GET: CGIManager::start() succeeds");

    if (!started)
    {
        std::cout << immediateError << std::endl;
        return;
    }

    check(!pollFds.empty(), "GET: CGI pipe fds registered");

    int readyClient = -1;
    std::string response;
    bool keepAlive = false;

    bool ready = driveUntilReady(
        manager,
        pollFds,
        readyClient,
        response,
        keepAlive,
        5000
    );

    check(ready, "GET: response becomes ready");

    if (ready)
    {
        check(readyClient == clientFd,
              "GET: correct client fd returned");

        check(contains(response, "200"),
              "GET: response contains 200");

        check(contains(response, "Content-Type"),
              "GET: response contains Content-Type");

        check(contains(response, "METHOD=GET"),
              "GET: CGI output reaches response");

        check(keepAlive,
              "GET: keep-alive remains enabled");
    }
}

/*
 * Test 2:
 * POST body must be written to CGI stdin and echoed back.
 */
static void testPost()
{
    std::cout << "\n--- Test 2: POST CGI ---" << std::endl;

    CGIManager manager;
    std::vector<pollfd> pollFds;

    ServerConfig config;

    const std::string body = "hello=webserv";

    HttpRequest request = makeRequest(
        "POST /cgi-bin/test_echo.py HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: 13\r\n"
        "\r\n"
        "hello=webserv"
    );

    const int clientFd = 1002;
    std::string immediateError;

    bool started = manager.start(
        clientFd,
        request,
        config,
        "www/cgi-bin/test_echo.py",
        "/usr/bin/python3",
        pollFds,
        immediateError
    );

    check(started, "POST: CGIManager::start() succeeds");

    if (!started)
        return;

    int readyClient = -1;
    std::string response;
    bool keepAlive = false;

    bool ready = driveUntilReady(
        manager,
        pollFds,
        readyClient,
        response,
        keepAlive,
        5000
    );

    check(ready, "POST: response becomes ready");

    if (ready)
    {
        check(contains(response, body),
              "POST: CGI received request body");

        check(contains(response, "CONTENT_LENGTH=13"),
              "POST: CGI received correct CONTENT_LENGTH");
    }
}

/*
 * Test 3:
 * Query string must reach CGI.
 */
static void testQuery()
{
    std::cout << "\n--- Test 3: Query String ---" << std::endl;

    CGIManager manager;
    std::vector<pollfd> pollFds;

    ServerConfig config;

    HttpRequest request = makeRequest(
        "GET /cgi-bin/test_echo.py?name=alice&x=123 HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n"
    );

    const int clientFd = 1003;
    std::string immediateError;

    bool started = manager.start(
        clientFd,
        request,
        config,
        "www/cgi-bin/test_echo.py",
        "/usr/bin/python3",
        pollFds,
        immediateError
    );

    check(started, "Query: CGIManager::start() succeeds");

    if (!started)
        return;

    int readyClient = -1;
    std::string response;
    bool keepAlive = false;

    bool ready = driveUntilReady(
        manager,
        pollFds,
        readyClient,
        response,
        keepAlive,
        5000
    );

    check(ready, "Query: response becomes ready");

    if (ready)
    {
        check(
            contains(response, "name=alice&x=123"),
            "Query: QUERY_STRING reaches CGI"
        );
    }
}

/*
 * Test 4:
 * Large request body.
 *
 * This is important because the body can be larger than a pipe buffer.
 * CGIManager must use POLLOUT instead of trying to write everything
 * in one blocking write().
 */
static void testLargePost()
{
    std::cout << "\n--- Test 4: Large POST ---" << std::endl;

    CGIManager manager;
    std::vector<pollfd> pollFds;

    ServerConfig config;

    std::string body(128 * 1024, 'A');

    std::ostringstream requestStream;
    requestStream
        << "POST /cgi-bin/test_echo.py HTTP/1.1\r\n"
        << "Host: localhost\r\n"
        << "Content-Type: text/plain\r\n"
        << "Content-Length: " << body.size() << "\r\n"
        << "\r\n"
        << body;

    HttpRequest request = makeRequest(requestStream.str());

    const int clientFd = 1004;
    std::string immediateError;

    bool started = manager.start(
        clientFd,
        request,
        config,
        "www/cgi-bin/test_echo.py",
        "/usr/bin/python3",
        pollFds,
        immediateError
    );

    check(started, "Large POST: CGIManager::start() succeeds");

    if (!started)
        return;

    int readyClient = -1;
    std::string response;
    bool keepAlive = false;

    bool ready = driveUntilReady(
        manager,
        pollFds,
        readyClient,
        response,
        keepAlive,
        10000
    );

    check(ready, "Large POST: response becomes ready");

    if (ready)
    {
        check(
            contains(response, body),
            "Large POST: complete body reaches CGI"
        );
    }
}

/*
 * Test 5:
 * Large CGI output.
 *
 * test_large_output.py must generate output larger than a typical
 * pipe buffer. This verifies that CGIManager continuously drains
 * stdout instead of blocking.
 */
static void testLargeOutput()
{
    std::cout << "\n--- Test 5: Large CGI Output ---" << std::endl;

    CGIManager manager;
    std::vector<pollfd> pollFds;

    ServerConfig config;

    HttpRequest request = makeRequest(
        "GET /cgi-bin/test_large_output.py HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n"
    );

    const int clientFd = 1005;
    std::string immediateError;

    bool started = manager.start(
        clientFd,
        request,
        config,
        "www/cgi-bin/test_large_output.py",
        "/usr/bin/python3",
        pollFds,
        immediateError
    );

    check(started, "Large output: CGIManager::start() succeeds");

    if (!started)
        return;

    int readyClient = -1;
    std::string response;
    bool keepAlive = false;

    bool ready = driveUntilReady(
        manager,
        pollFds,
        readyClient,
        response,
        keepAlive,
        10000
    );

    check(ready, "Large output: response becomes ready");

    if (ready)
    {
        /*
         * The helper script outputs 128 KB of X characters.
         */
        check(
            response.size() > 100 * 1024,
            "Large output: complete large response received"
        );
    }
}

/*
 * Test 6:
 * CGI exits with non-zero status.
 *
 * test_exit_42.py must exit(42).
 *
 * Expected manager behavior: 502 Bad Gateway.
 */
static void testNonZeroExit()
{
    std::cout << "\n--- Test 6: Non-zero CGI Exit ---" << std::endl;

    CGIManager manager;
    std::vector<pollfd> pollFds;

    ServerConfig config;

    HttpRequest request = makeRequest(
        "GET /cgi-bin/test_exit_42.py HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n"
    );

    const int clientFd = 1006;
    std::string immediateError;

    bool started = manager.start(
        clientFd,
        request,
        config,
        "www/cgi-bin/test_exit_42.py",
        "/usr/bin/python3",
        pollFds,
        immediateError
    );

    check(started, "Non-zero exit: CGI starts");

    if (!started)
        return;

    int readyClient = -1;
    std::string response;
    bool keepAlive = true;

    bool ready = driveUntilReady(
        manager,
        pollFds,
        readyClient,
        response,
        keepAlive,
        5000
    );

    check(ready, "Non-zero exit: error response becomes ready");

    if (ready)
    {
        check(
            contains(response, "502"),
            "Non-zero exit: manager returns 502"
        );

        check(
            !keepAlive,
            "Non-zero exit: connection is closed"
        );
    }
}

/*
 * Test 7:
 * CGI crashes with SIGSEGV.
 *
 * Expected manager behavior: 502 Bad Gateway.
 */
static void testCrash()
{
    std::cout << "\n--- Test 7: CGI Crash ---" << std::endl;

    CGIManager manager;
    std::vector<pollfd> pollFds;

    ServerConfig config;

    HttpRequest request = makeRequest(
        "GET /cgi-bin/test_crash.py HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n"
    );

    const int clientFd = 1007;
    std::string immediateError;

    bool started = manager.start(
        clientFd,
        request,
        config,
        "www/cgi-bin/test_crash.py",
        "/usr/bin/python3",
        pollFds,
        immediateError
    );

    check(started, "Crash: CGI starts");

    if (!started)
        return;

    int readyClient = -1;
    std::string response;
    bool keepAlive = true;

    bool ready = driveUntilReady(
        manager,
        pollFds,
        readyClient,
        response,
        keepAlive,
        5000
    );

    check(ready, "Crash: error response becomes ready");

    if (ready)
    {
        check(
            contains(response, "502"),
            "Crash: manager returns 502"
        );

        check(
            !keepAlive,
            "Crash: connection is closed"
        );
    }
}

/*
 * Test 8:
 * CGI produces no stdout but exits successfully.
 *
 * Expected manager behavior: successful HTTP response with empty body.
 */
static void testEmptyOutput()
{
    std::cout << "\n--- Test 8: Empty CGI Output ---" << std::endl;

    CGIManager manager;
    std::vector<pollfd> pollFds;

    ServerConfig config;

    HttpRequest request = makeRequest(
        "GET /cgi-bin/test_empty.py HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n"
    );

    const int clientFd = 1008;
    std::string immediateError;

    bool started = manager.start(
        clientFd,
        request,
        config,
        "www/cgi-bin/test_empty.py",
        "/usr/bin/python3",
        pollFds,
        immediateError
    );

    check(started, "Empty output: CGI starts");

    if (!started)
        return;

    int readyClient = -1;
    std::string response;
    bool keepAlive = false;

    bool ready = driveUntilReady(
        manager,
        pollFds,
        readyClient,
        response,
        keepAlive,
        5000
    );

    check(ready, "Empty output: response becomes ready");

    if (ready)
    {
        check(
            contains(response, "200"),
            "Empty output: successful CGI returns 200"
        );
    }
}

/*
 * Test 9:
 * Client disconnects while CGI is running.
 *
 * test_sleep.py must sleep for several seconds.
 *
 * abortForClient() should:
 *   - kill the CGI
 *   - remove its pipe fds from pollFds
 *   - remove its session
 *   - not enqueue a response
 */
static void testAbortForClient()
{
    std::cout << "\n--- Test 9: Client Disconnect ---" << std::endl;

    CGIManager manager;
    std::vector<pollfd> pollFds;

    ServerConfig config;

    HttpRequest request = makeRequest(
        "GET /cgi-bin/test_sleep.py HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n"
    );

    const int clientFd = 1009;
    std::string immediateError;

    bool started = manager.start(
        clientFd,
        request,
        config,
        "www/cgi-bin/test_sleep.py",
        "/usr/bin/python3",
        pollFds,
        immediateError
    );

    check(started, "Client disconnect: CGI starts");

    if (!started)
        return;

    check(
        !pollFds.empty(),
        "Client disconnect: CGI fds registered"
    );

    manager.abortForClient(clientFd, pollFds);

	int readyClientFd = -1;
	std::string readyResponse;
	bool readyKeepAlive = false;

	check(
		!manager.popReady(
			readyClientFd,
			readyResponse,
			readyKeepAlive
		),
		"Client disconnect: no response queued"
	);

    check(
        pollFds.empty(),
        "Client disconnect: CGI fds removed from poll list"
    );

    /*
     * Give the killed child a little time to terminate.
     */
    usleep(100 * 1000);
}

/*
 * Test 10:
 * Starting another CGI for the same client fd must not silently
 * overwrite the existing session.
 */
static void testDuplicateClient()
{
    std::cout << "\n--- Test 10: Duplicate Client CGI ---" << std::endl;

    CGIManager manager;
    std::vector<pollfd> pollFds;

    ServerConfig config;

    HttpRequest request = makeRequest(
        "GET /cgi-bin/test_sleep.py HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n"
    );

    const int clientFd = 1010;

    std::string error1;

    bool first = manager.start(
        clientFd,
        request,
        config,
        "www/cgi-bin/test_sleep.py",
        "/usr/bin/python3",
        pollFds,
        error1
    );

    check(first, "Duplicate client: first CGI starts");

    if (!first)
        return;

    std::size_t fdsAfterFirst = pollFds.size();

    std::string error2;

    bool second = manager.start(
        clientFd,
        request,
        config,
        "www/cgi-bin/test_sleep.py",
        "/usr/bin/python3",
        pollFds,
        error2
    );

    check(
        !second,
        "Duplicate client: second CGI is rejected"
    );

    check(
        pollFds.size() == fdsAfterFirst,
        "Duplicate client: poll fd list is not corrupted"
    );

    manager.abortForClient(clientFd, pollFds);
}

/*
 * Test 11:
 * A CGI can close stdout before its process exits.
 *
 * test_close_stdout_sleep.py should:
 *   - close stdout
 *   - sleep briefly
 *   - exit(42)
 *
 * A robust manager must NOT immediately treat stdout EOF as
 * successful completion. It should wait until the child is reaped,
 * then detect the non-zero exit and return 502.
 *
 * This test is especially useful for detecting the subtle
 * "stdout EOF != process exited" bug.
 */
static void testStdoutClosesBeforeProcess()
{
    std::cout << "\n--- Test 11: Stdout EOF Before Process Exit ---" << std::endl;

    CGIManager manager;
    std::vector<pollfd> pollFds;

    ServerConfig config;

    HttpRequest request = makeRequest(
        "GET /cgi-bin/test_close_stdout_sleep.py HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n"
    );

    const int clientFd = 1011;
    std::string immediateError;

    bool started = manager.start(
        clientFd,
        request,
        config,
        "www/cgi-bin/test_close_stdout_sleep.py",
        "/usr/bin/python3",
        pollFds,
        immediateError
    );

    check(
        started,
        "Stdout early close: CGI starts"
    );

    if (!started)
        return;

    int readyClient = -1;
    std::string response;
    bool keepAlive = true;

    bool ready = driveUntilReady(
        manager,
        pollFds,
        readyClient,
        response,
        keepAlive,
        5000
    );

    check(
        ready,
        "Stdout early close: response eventually becomes ready"
    );

    if (ready)
    {
        check(
            contains(response, "502"),
            "Stdout early close: non-zero exit is detected"
        );

        check(
            !keepAlive,
            "Stdout early close: connection is closed"
        );
    }
}

int main()
{
    std::cout << "============================================"
              << std::endl;
    std::cout << "        CGIManager Test Suite"
              << std::endl;
    std::cout << "============================================"
              << std::endl;

    testGet();
    testPost();
    testQuery();
    testLargePost();
    testLargeOutput();
    testNonZeroExit();
    testCrash();
    testEmptyOutput();
    testAbortForClient();
    testDuplicateClient();
    testStdoutClosesBeforeProcess();

    std::cout << "\n============================================"
              << std::endl;
    std::cout << "Passed: " << g_passed
              << "/" << g_tests << std::endl;
    std::cout << "============================================"
              << std::endl;

    if (g_passed == g_tests)
    {
        std::cout << "ALL TESTS PASSED!" << std::endl;
        return (0);
    }

    std::cout << "SOME TESTS FAILED!" << std::endl;
    return (1);
}