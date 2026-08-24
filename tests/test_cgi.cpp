#include "../cgi/CGIHandler.hpp"
#include "../cgi/CGIEnvBuilder.hpp"
#include "../include/HttpRequest.hpp"

#include <iostream>
#include <sstream>
#include <poll.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <csignal>

/*
Drives one CGIHandler end-to-end: writes the request body to its
stdin, reads its stdout until EOF, then reaps it. Everything here
mirrors what the server's poll() loop is expected to do, just
synchronously and with a hard timeout so a broken script can't hang
the test.
*/
static bool runCGI(const std::string &scriptPath, const std::string &interpreter,
					const HttpRequest &request, const std::string &serverName, int serverPort,
					std::string &output, int &exitCode)
{
	std::vector<std::string> env = buildCGIEnv(request, serverName, serverPort);

	CGIHandler cgi;
	cgi.setup(scriptPath, interpreter, env);
	if (!cgi.start())
	{
		std::cerr << "FAIL: CGIHandler::start() failed" << std::endl;
		return false;
	}

	const std::string &body = request.getBody();
	std::size_t written = 0;
	bool stdinOpen = true;
	bool stdoutOpen = true;

	if (body.empty())
	{
		cgi.closeStdinFd();
		stdinOpen = false;
	}

	output.clear();
	std::time_t start = std::time(NULL);

	while (stdoutOpen)
	{
		if (std::time(NULL) - start > 5)
		{
			std::cerr << "FAIL: CGI timed out" << std::endl;
			return false;
		}

		struct pollfd fds[2];
		int nfds = 0;
		int inIdx = -1, outIdx = -1;

		if (stdinOpen)
		{
			fds[nfds].fd = cgi.getStdinFd();
			fds[nfds].events = POLLOUT;
			inIdx = nfds++;
		}
		if (stdoutOpen)
		{
			fds[nfds].fd = cgi.getStdoutFd();
			fds[nfds].events = POLLIN;
			outIdx = nfds++;
		}

		int ret = poll(fds, nfds, 1000);
		if (ret < 0)
		{
			std::cerr << "FAIL: poll() error: " << strerror(errno) << std::endl;
			return false;
		}

		if (inIdx != -1 && (fds[inIdx].revents & POLLOUT))
		{
			ssize_t n = write(cgi.getStdinFd(), body.c_str() + written, body.size() - written);
			if (n > 0)
				written += static_cast<std::size_t>(n);
			if (written == body.size())
			{
				cgi.closeStdinFd();
				stdinOpen = false;
			}
		}

		if (outIdx != -1 && (fds[outIdx].revents & (POLLIN | POLLHUP)))
		{
			char buf[4096];
			ssize_t n = read(cgi.getStdoutFd(), buf, sizeof(buf));
			if (n > 0)
				output.append(buf, static_cast<std::size_t>(n));
			else if (n == 0)
			{
				cgi.closeStdoutFd();
				stdoutOpen = false;
			}
			else if (errno != EAGAIN && errno != EWOULDBLOCK)
			{
				std::cerr << "FAIL: read() error: " << strerror(errno) << std::endl;
				return false;
			}
		}
	}

	exitCode = -1;
	while (cgi.tryWait(exitCode) == 0)
		usleep(10000);

	return true;
}

int main()
{
	std::cout << "=== CGI TESTS ===" << std::endl;

	const std::string interpreter = "/usr/bin/python3";
	const std::string script = "www/cgi-bin/test_echo.py";

	// ---------------------------------------------------------
	// Test 1: GET with a query string, no body
	// ---------------------------------------------------------
	std::cout << "\nTest 1: GET with query string..." << std::endl;
	{
		HttpRequest req;
		std::string raw =
			"GET /cgi-bin/test_echo.py?name=World HTTP/1.1\r\n"
			"Host: localhost\r\n"
			"\r\n";

		if (!req.parse(raw))
		{
			std::cerr << "FAIL: request parse failed" << std::endl;
			return 1;
		}

		std::string output;
		int exitCode = -1;
		if (!runCGI(script, interpreter, req, "localhost", 8080, output, exitCode))
			return 1;

		std::cout << output;
		if (exitCode != 0)
		{
			std::cerr << "FAIL: CGI exit code " << exitCode << std::endl;
			return 1;
		}
		if (output.find("METHOD=GET") == std::string::npos ||
			output.find("QUERY=name=World") == std::string::npos ||
			output.find("HTTP_HOST=localhost") == std::string::npos)
		{
			std::cerr << "FAIL: expected fields missing from CGI output" << std::endl;
			return 1;
		}
		std::cout << "PASS" << std::endl;
	}

	// ---------------------------------------------------------
	// Test 2: POST with a body
	// ---------------------------------------------------------
	std::cout << "\nTest 2: POST with body..." << std::endl;
	{
		HttpRequest req;
		std::string body = "Hello=World";
		std::ostringstream raw;
		raw << "POST /cgi-bin/test_echo.py HTTP/1.1\r\n"
			<< "Host: localhost\r\n"
			<< "Content-Type: text/plain\r\n"
			<< "Content-Length: " << body.size() << "\r\n"
			<< "\r\n"
			<< body;

		if (!req.parse(raw.str()))
		{
			std::cerr << "FAIL: request parse failed" << std::endl;
			return 1;
		}

		std::string output;
		int exitCode = -1;
		if (!runCGI(script, interpreter, req, "localhost", 8080, output, exitCode))
			return 1;

		std::cout << output;
		if (exitCode != 0)
		{
			std::cerr << "FAIL: CGI exit code " << exitCode << std::endl;
			return 1;
		}
		if (output.find("METHOD=POST") == std::string::npos ||
			output.find("BODY=Hello=World") == std::string::npos)
		{
			std::cerr << "FAIL: expected fields missing from CGI output" << std::endl;
			return 1;
		}
		std::cout << "PASS" << std::endl;
	}

	// ---------------------------------------------------------
	// Test 3: CGI killed by a signal (SIGTERM) before writing anything
	// ---------------------------------------------------------
	std::cout << "\nTest 3: CGI killed by SIGTERM..." << std::endl;
	{
		HttpRequest req;
		std::string raw =
			"GET /cgi-bin/test_kill_sigterm.py HTTP/1.1\r\n"
			"Host: localhost\r\n"
			"\r\n";

		if (!req.parse(raw))
		{
			std::cerr << "FAIL: request parse failed" << std::endl;
			return 1;
		}

		std::string output;
		int exitCode = 0;
		if (!runCGI("www/cgi-bin/test_kill_sigterm.py", interpreter, req, "localhost", 8080, output, exitCode))
			return 1;

		if (exitCode != -SIGTERM)
		{
			std::cerr << "FAIL: expected exitCode " << -SIGTERM << " (killed by SIGTERM), got " << exitCode << std::endl;
			return 1;
		}
		std::cout << "PASS (exitCode=" << exitCode << ")" << std::endl;
	}

	// ---------------------------------------------------------
	// Test 4: CGI crashes on its own (SIGSEGV), not a deliberate kill
	// ---------------------------------------------------------
	std::cout << "\nTest 4: CGI crashes with SIGSEGV..." << std::endl;
	{
		HttpRequest req;
		std::string raw =
			"GET /cgi-bin/test_crash_sigsegv.py HTTP/1.1\r\n"
			"Host: localhost\r\n"
			"\r\n";

		if (!req.parse(raw))
		{
			std::cerr << "FAIL: request parse failed" << std::endl;
			return 1;
		}

		std::string output;
		int exitCode = 0;
		if (!runCGI("www/cgi-bin/test_crash_sigsegv.py", interpreter, req, "localhost", 8080, output, exitCode))
			return 1;

		if (exitCode != -SIGSEGV)
		{
			std::cerr << "FAIL: expected exitCode " << -SIGSEGV << " (killed by SIGSEGV), got " << exitCode << std::endl;
			return 1;
		}
		std::cout << "PASS (exitCode=" << exitCode << ")" << std::endl;
	}

	std::cout << "\n=== ALL CGI TESTS PASSED ===" << std::endl;
	return 0;
}
