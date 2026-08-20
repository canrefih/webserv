#include "../include/HttpResponse.hpp"
#include <iostream>
#include <cstring>

int main()
{
	std::cout << "=== HTTP RESPONSE TESTS ===" << std::endl;
	
	std::cout << "\nTest 1: Check default response..." << std::endl;
	HttpResponse resp1;
	std::string output = resp1.toString();
	
	if (output.find("HTTP/1.1 200 OK") == std::string::npos)
	{
		std::cerr << "FAIL: Default status line incorrect" << std::endl;
		return 1;
	}
	
	if (output.find("Content-Type: text/plain") == std::string::npos)
	{
		std::cerr << "FAIL: Default content type incorrect" << std::endl;
		return 1;
	}
	
	if (output.find("Connection: close") == std::string::npos)
	{
		std::cerr << "FAIL: Connection header missing" << std::endl;
		return 1;
	}
	std::cout << "PASS: Default response correct" << std::endl;
	
	std::cout << "\nTest 2: Test setStatus..." << std::endl;
	HttpResponse resp2;
	resp2.setStatus(404, "Not Found");
	output = resp2.toString();
	
	if (output.find("HTTP/1.1 404 Not Found") == std::string::npos)
	{
		std::cerr << "FAIL: Status line not set correctly" << std::endl;
		return 1;
	}
	std::cout << "PASS: setStatus working" << std::endl;
	
	std::cout << "\nTest 3: Test setBody and setContentType..." << std::endl;
	HttpResponse resp3;
	resp3.setStatus(200, "OK");
	resp3.setBody("<html><body>Test</body></html>");
	resp3.setContentType("text/html");
	output = resp3.toString();
	
	if (output.find("Content-Length: 30") == std::string::npos)
	{
		std::cerr << "FAIL: Content-Length incorrect" << std::endl;
		return 1;
	}
	
	if (output.find("Content-Type: text/html") == std::string::npos)
	{
		std::cerr << "FAIL: Content-Type not set" << std::endl;
		return 1;
	}
	
	if (output.find("<html><body>Test</body></html>") == std::string::npos)
	{
		std::cerr << "FAIL: Body not in response" << std::endl;
		return 1;
	}
	std::cout << "PASS: Body and content type working" << std::endl;
	
	std::cout << "\nTest 4: Test setHeader..." << std::endl;
	HttpResponse resp4;
	resp4.setStatus(200, "OK");
	resp4.setBody("test");
	resp4.setContentType("text/plain");
	resp4.setHeader("Connection", "keep-alive");
	resp4.setHeader("Cache-Control", "no-cache");
	output = resp4.toString();
	
	if (output.find("Connection: keep-alive") == std::string::npos)
	{
		std::cerr << "FAIL: Custom Connection header not found" << std::endl;
		return 1;
	}
	
	if (output.find("Cache-Control: no-cache") == std::string::npos)
	{
		std::cerr << "FAIL: Custom Cache-Control header not found" << std::endl;
		return 1;
	}
	std::cout << "PASS: setHeader working" << std::endl;
	
	std::cout << "\nTest 5: Test multiple custom headers..." << std::endl;
	HttpResponse resp5;
	resp5.setStatus(200, "OK");
	resp5.setBody("data");
	resp5.setContentType("application/json");
	resp5.setHeader("X-Custom-1", "value1");
	resp5.setHeader("X-Custom-2", "value2");
	resp5.setHeader("X-Custom-3", "value3");
	output = resp5.toString();
	
	if (output.find("X-Custom-1: value1") == std::string::npos ||
	    output.find("X-Custom-2: value2") == std::string::npos ||
	    output.find("X-Custom-3: value3") == std::string::npos)
	{
		std::cerr << "FAIL: Not all custom headers found" << std::endl;
		return 1;
	}
	std::cout << "PASS: Multiple custom headers working" << std::endl;
	
	std::cout << "\nTest 6: Test error response..." << std::endl;
	HttpResponse resp6;
	resp6.setStatus(500, "Internal Server Error");
	resp6.setBody("An error occurred");
	resp6.setContentType("text/plain");
	output = resp6.toString();
	
	if (output.find("HTTP/1.1 500 Internal Server Error") == std::string::npos)
	{
		std::cerr << "FAIL: Error status line incorrect" << std::endl;
		return 1;
	}
	
	if (output.find("An error occurred") == std::string::npos)
	{
		std::cerr << "FAIL: Error body not in response" << std::endl;
		return 1;
	}
	std::cout << "PASS: Error response correct" << std::endl;
	
	std::cout << "\nTest 7: Test empty body content length..." << std::endl;
	HttpResponse resp7;
	resp7.setStatus(204, "No Content");
	resp7.setBody("");
	output = resp7.toString();
	
	if (output.find("Content-Length: 0") == std::string::npos)
	{
		std::cerr << "FAIL: Content-Length should be 0 for empty body" << std::endl;
		return 1;
	}
	std::cout << "PASS: Empty body handled correctly" << std::endl;
	
	std::cout << "\n=== ALL HTTP RESPONSE TESTS PASSED ===" << std::endl;
	return 0;
}
