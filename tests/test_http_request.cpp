#include "../include/HttpRequest.hpp"
#include <iostream>

int main()
{
	std::cout << "=== HTTP REQUEST TESTS ===" << std::endl;
	
	std::cout << "\nTest 1: Parse simple GET request..." << std::endl;
	HttpRequest req1;
	std::string rawReq1 = "GET / HTTP/1.1\r\nHost: localhost\r\nUser-Agent: test\r\n\r\n";
	
	if (!req1.parse(rawReq1))
	{
		std::cerr << "FAIL: Could not parse simple GET request" << std::endl;
		return 1;
	}
	
	if (req1.getMethod() != "GET" ||
	    req1.getTarget() != "/" ||
	    req1.getVersion() != "HTTP/1.1")
	{
		std::cerr << "FAIL: Request line parsing incorrect" << std::endl;
		return 1;
	}
	
	if (req1.getHeader("Host") != "localhost")
	{
		std::cerr << "FAIL: Host header not parsed correctly" << std::endl;
		return 1;
	}
	std::cout << "PASS: Simple GET request parsed" << std::endl;
	
	std::cout << "\nTest 2: Parse POST request with body..." << std::endl;
	HttpRequest req2;
	std::string rawReq2 = "POST /api HTTP/1.1\r\nHost: example.com\r\nContent-Length: 13\r\n\r\nHello, World!";
	
	if (!req2.parse(rawReq2))
	{
		std::cerr << "FAIL: Could not parse POST request" << std::endl;
		return 1;
	}
	
	if (req2.getMethod() != "POST" || req2.getTarget() != "/api")
	{
		std::cerr << "FAIL: POST request line incorrect" << std::endl;
		return 1;
	}
	
	if (req2.getHeader("Content-Length") != "13")
	{
		std::cerr << "FAIL: Content-Length header incorrect" << std::endl;
		return 1;
	}
	
	if (req2.getBody() != "Hello, World!")
	{
		std::cerr << "FAIL: Body not parsed correctly. Got: '" << req2.getBody() << "'" << std::endl;
		return 1;
	}
	std::cout << "PASS: POST request with body parsed" << std::endl;
	
	std::cout << "\nTest 3: Parse multiple headers..." << std::endl;
	HttpRequest req3;
	std::string rawReq3 = "GET /test HTTP/1.1\r\nHost: test.com\r\nAccept: */*\r\nConnection: close\r\n\r\n";
	
	if (!req3.parse(rawReq3))
	{
		std::cerr << "FAIL: Could not parse request with multiple headers" << std::endl;
		return 1;
	}
	
	const std::map<std::string, std::string> &headers = req3.getHeaders();
	if (headers.size() != 3)
	{
		std::cerr << "FAIL: Expected 3 headers, got " << headers.size() << std::endl;
		return 1;
	}
	
	if (req3.getHeader("Accept") != "*/*")
	{
		std::cerr << "FAIL: Accept header incorrect" << std::endl;
		return 1;
	}
	std::cout << "PASS: Multiple headers parsed" << std::endl;
	
	std::cout << "\nTest 4: Parse header with leading spaces..." << std::endl;
	HttpRequest req4;
	std::string rawReq4 = "GET / HTTP/1.1\r\nHost:   example.com   \r\n\r\n";
	
	if (!req4.parse(rawReq4))
	{
		std::cerr << "FAIL: Could not parse header with spaces" << std::endl;
		return 1;
	}
	
	std::string hostValue = req4.getHeader("Host");
	if (hostValue.empty() || hostValue[0] == ' ')
	{
		std::cerr << "FAIL: Header value space trimming incorrect" << std::endl;
		return 1;
	}
	std::cout << "PASS: Header spaces handled" << std::endl;
	
	std::cout << "\nTest 5: Get non-existent header..." << std::endl;
	HttpRequest req5;
	std::string rawReq5 = "GET / HTTP/1.1\r\n\r\n";
	req5.parse(rawReq5);
	
	const std::string &noHeader = req5.getHeader("Non-Existent");
	if (!noHeader.empty())
	{
		std::cerr << "FAIL: Non-existent header should return empty string" << std::endl;
		return 1;
	}
	std::cout << "PASS: Non-existent header returns empty" << std::endl;
	
	std::cout << "\nTest 6: Parse DELETE request..." << std::endl;
	HttpRequest req6;
	std::string rawReq6 = "DELETE /file.txt HTTP/1.1\r\nHost: localhost\r\n\r\n";
	
	if (!req6.parse(rawReq6))
	{
		std::cerr << "FAIL: Could not parse DELETE request" << std::endl;
		return 1;
	}
	
	if (req6.getMethod() != "DELETE")
	{
		std::cerr << "FAIL: DELETE method not recognized" << std::endl;
		return 1;
	}
	std::cout << "PASS: DELETE request parsed" << std::endl;
	
	std::cout << "\n=== ALL HTTP REQUEST TESTS PASSED ===" << std::endl;
	return 0;
}
