#include "../include/HttpRequest.hpp"

#include <iostream>

int main()
{
	std::cout << "=== HTTP REQUEST TESTS ===" << std::endl;

	// ---------------------------------------------------------
	// Test 1: Basic request parsing
	// ---------------------------------------------------------
	std::cout << "\nTest 1: Basic request parsing..." << std::endl;

	HttpRequest request1;

	std::string rawRequest1 =
		"GET /index.html HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Connection: keep-alive\r\n"
		"\r\n";

	if (!request1.parse(rawRequest1))
	{
		std::cerr << "FAIL: Request parsing failed" << std::endl;
		return 1;
	}

	if (request1.getMethod() != "GET")
	{
		std::cerr << "FAIL: Method incorrect" << std::endl;
		return 1;
	}

	if (request1.getTarget().getPath() != "/index.html")
	{
		std::cerr << "FAIL: Target incorrect" << std::endl;
		return 1;
	}

	if (request1.getVersion() != "HTTP/1.1")
	{
		std::cerr << "FAIL: Version incorrect" << std::endl;
		return 1;
	}

	std::cout << "PASS: Basic request parsing" << std::endl;

	// ---------------------------------------------------------
	// Test 2: Header parsing
	// ---------------------------------------------------------
	std::cout << "\nTest 2: Header parsing..." << std::endl;

	if (request1.getHeader("Host") != "localhost")
	{
		std::cerr << "FAIL: Host header incorrect" << std::endl;
		return 1;
	}

	if (request1.getHeader("Connection") != "keep-alive")
	{
		std::cerr << "FAIL: Connection header incorrect" << std::endl;
		return 1;
	}

	std::cout << "PASS: Header parsing" << std::endl;

	// ---------------------------------------------------------
	// Test 3: Case-insensitive header lookup
	// ---------------------------------------------------------
	std::cout << "\nTest 3: Case-insensitive header lookup..." << std::endl;

	HttpRequest request2;

	std::string rawRequest2 =
		"GET / HTTP/1.1\r\n"
		"hOsT: localhost\r\n"
		"cOnNeCtIoN: keep-alive\r\n"
		"CONTENT-LENGTH: 10\r\n"
		"\r\n"
		"1234567890";

	if (!request2.parse(rawRequest2))
	{
		std::cerr << "FAIL: Mixed-case request parsing failed" << std::endl;
		return 1;
	}

	// Host
	if (request2.getHeader("Host") != "localhost")
	{
		std::cerr << "FAIL: Host lookup failed" << std::endl;
		return 1;
	}

	if (request2.getHeader("host") != "localhost")
	{
		std::cerr << "FAIL: lowercase Host lookup failed" << std::endl;
		return 1;
	}

	if (request2.getHeader("HOST") != "localhost")
	{
		std::cerr << "FAIL: uppercase Host lookup failed" << std::endl;
		return 1;
	}

	if (request2.getHeader("HoSt") != "localhost")
	{
		std::cerr << "FAIL: mixed-case Host lookup failed" << std::endl;
		return 1;
	}

	// Connection
	if (request2.getHeader("Connection") != "keep-alive")
	{
		std::cerr << "FAIL: Connection lookup failed" << std::endl;
		return 1;
	}

	if (request2.getHeader("connection") != "keep-alive")
	{
		std::cerr << "FAIL: lowercase Connection lookup failed" << std::endl;
		return 1;
	}

	if (request2.getHeader("CONNECTION") != "keep-alive")
	{
		std::cerr << "FAIL: uppercase Connection lookup failed" << std::endl;
		return 1;
	}

	if (request2.getHeader("CoNnEcTiOn") != "keep-alive")
	{
		std::cerr << "FAIL: mixed-case Connection lookup failed" << std::endl;
		return 1;
	}

	// Content-Length
	if (request2.getHeader("Content-Length") != "10")
	{
		std::cerr << "FAIL: Content-Length lookup failed" << std::endl;
		return 1;
	}

	if (request2.getHeader("CONTENT-LENGTH") != "10")
	{
		std::cerr << "FAIL: uppercase Content-Length lookup failed" << std::endl;
		return 1;
	}

	std::cout << "PASS: Header lookup is case-insensitive" << std::endl;

	// ---------------------------------------------------------
	// Test 4: Header names are normalized internally
	// ---------------------------------------------------------
	std::cout << "\nTest 4: Header name normalization..." << std::endl;

	const std::map<std::string, std::string> &headers =
		request2.getHeaders();

	if (headers.find("host") == headers.end())
	{
		std::cerr << "FAIL: Host was not normalized to lowercase" << std::endl;
		return 1;
	}

	if (headers.find("connection") == headers.end())
	{
		std::cerr << "FAIL: Connection was not normalized to lowercase"
				  << std::endl;
		return 1;
	}

	if (headers.find("content-length") == headers.end())
	{
		std::cerr << "FAIL: Content-Length was not normalized to lowercase"
				  << std::endl;
		return 1;
	}

	if (headers.find("Host") != headers.end())
	{
		std::cerr << "FAIL: Uppercase Host key exists internally" << std::endl;
		return 1;
	}

	if (headers.find("Connection") != headers.end())
	{
		std::cerr << "FAIL: Mixed-case Connection key exists internally"
				  << std::endl;
		return 1;
	}

	std::cout << "PASS: Header names normalized correctly" << std::endl;

	// ---------------------------------------------------------
	// Test 5: Missing header
	// ---------------------------------------------------------
	std::cout << "\nTest 5: Missing header..." << std::endl;

	if (!request2.getHeader("User-Agent").empty())
	{
		std::cerr << "FAIL: Missing header should return empty string"
				  << std::endl;
		return 1;
	}

	if (!request2.getHeader("USER-AGENT").empty())
	{
		std::cerr << "FAIL: Missing header lookup should be case-insensitive"
				  << std::endl;
		return 1;
	}

	std::cout << "PASS: Missing header handled correctly" << std::endl;

	// ---------------------------------------------------------
	// Test 6: Request body
	// ---------------------------------------------------------
	std::cout << "\nTest 6: Request body..." << std::endl;

	if (request2.getBody() != "1234567890")
	{
		std::cerr << "FAIL: Request body incorrect" << std::endl;
		return 1;
	}

	std::cout << "PASS: Request body handled correctly" << std::endl;

	// ---------------------------------------------------------
	// Test 7: Invalid request
	// ---------------------------------------------------------
	std::cout << "\nTest 7: Invalid request..." << std::endl;

	HttpRequest request3;

	if (request3.parse("INVALID REQUEST\r\n\r\n"))
	{
		std::cerr << "FAIL: Invalid request was accepted" << std::endl;
		return 1;
	}

	std::cout << "PASS: Invalid request rejected" << std::endl;

	// ---------------------------------------------------------
	// Test 8: Invalid header
	// ---------------------------------------------------------
	std::cout << "\nTest 8: Invalid header..." << std::endl;

	HttpRequest request4;

	std::string invalidHeaderRequest =
		"GET / HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"InvalidHeaderWithoutColon\r\n"
		"\r\n";

	if (request4.parse(invalidHeaderRequest))
	{
		std::cerr << "FAIL: Invalid header was accepted" << std::endl;
		return 1;
	}

	std::cout << "PASS: Invalid header rejected" << std::endl;

	// ---------------------------------------------------------
	// Final result
	// ---------------------------------------------------------
	std::cout << "\n=== ALL HTTP REQUEST TESTS PASSED ===" << std::endl;

	return 0;
}
