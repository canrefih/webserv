#include "../include/HttpResponse.hpp"
#include <iostream>
#include <cstring>

int main()
{
	std::cout << "=== HTTP RESPONSE TESTS ===" << std::endl;

	// ---------------------------------------------------------
	// Test 1: Default response
	// ---------------------------------------------------------
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

	if (output.find("Content-Length: 0") == std::string::npos)
	{
		std::cerr << "FAIL: Default Content-Length should be 0" << std::endl;
		return 1;
	}

	std::cout << "PASS: Default response correct" << std::endl;

	// ---------------------------------------------------------
	// Test 2: setStatus
	// ---------------------------------------------------------
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

	// ---------------------------------------------------------
	// Test 3: setBody and setContentType
	// ---------------------------------------------------------
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

	// ---------------------------------------------------------
	// Test 4: setHeader
	// ---------------------------------------------------------
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

	// ---------------------------------------------------------
	// Test 5: Multiple custom headers
	// ---------------------------------------------------------
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

	// ---------------------------------------------------------
	// Test 6: Overwriting an existing custom header
	// ---------------------------------------------------------
	std::cout << "\nTest 6: Test custom header overwrite..." << std::endl;

	HttpResponse resp6;

	resp6.setHeader("X-Test", "first");
	resp6.setHeader("X-Test", "second");

	output = resp6.toString();

	if (output.find("X-Test: second") == std::string::npos)
	{
		std::cerr << "FAIL: Custom header was not overwritten" << std::endl;
		return 1;
	}

	if (output.find("X-Test: first") != std::string::npos)
	{
		std::cerr << "FAIL: Old custom header value still present" << std::endl;
		return 1;
	}

	std::cout << "PASS: Custom header overwrite working" << std::endl;

	// ---------------------------------------------------------
	// Test 7: Keep-Alive header
	// ---------------------------------------------------------
	std::cout << "\nTest 7: Test Keep-Alive header..." << std::endl;

	HttpResponse resp7;

	resp7.setStatus(200, "OK");
	resp7.setBody("keep-alive test");
	resp7.setContentType("text/plain");

	resp7.setHeader("Connection", "keep-alive");

	output = resp7.toString();

	if (output.find("Connection: keep-alive") == std::string::npos)
	{
		std::cerr << "FAIL: Keep-Alive header not generated" << std::endl;
		return 1;
	}

	if (output.find("Connection: close") != std::string::npos)
	{
		std::cerr << "FAIL: Connection: close found together with keep-alive" << std::endl;
		return 1;
	}

	std::cout << "PASS: Keep-Alive header working" << std::endl;

	// ---------------------------------------------------------
	// Test 8: Connection close header
	// ---------------------------------------------------------
	std::cout << "\nTest 8: Test Connection close header..." << std::endl;

	HttpResponse resp8;

	resp8.setStatus(200, "OK");
	resp8.setBody("close test");
	resp8.setContentType("text/plain");

	resp8.setHeader("Connection", "close");

	output = resp8.toString();

	if (output.find("Connection: close") == std::string::npos)
	{
		std::cerr << "FAIL: Connection close header not generated" << std::endl;
		return 1;
	}

	if (output.find("Connection: keep-alive") != std::string::npos)
	{
		std::cerr << "FAIL: Keep-Alive header found together with close" << std::endl;
		return 1;
	}

	std::cout << "PASS: Connection close header working" << std::endl;

	// ---------------------------------------------------------
	// Test 9: Error response
	// ---------------------------------------------------------
	std::cout << "\nTest 9: Test error response..." << std::endl;

	HttpResponse resp9;

	resp9.setStatus(500, "Internal Server Error");
	resp9.setBody("An error occurred");
	resp9.setContentType("text/plain");

	output = resp9.toString();

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

	// ---------------------------------------------------------
	// Test 10: Empty body / Content-Length
	// ---------------------------------------------------------
	std::cout << "\nTest 10: Test empty body content length..." << std::endl;

	HttpResponse resp10;

	resp10.setStatus(204, "No Content");
	resp10.setBody("");

	output = resp10.toString();

	if (output.find("Content-Length: 0") == std::string::npos)
	{
		std::cerr << "FAIL: Content-Length should be 0 for empty body" << std::endl;
		return 1;
	}

	std::cout << "PASS: Empty body handled correctly" << std::endl;

	// ---------------------------------------------------------
	// Test 11: Header appears before body
	// ---------------------------------------------------------
	std::cout << "\nTest 11: Test response structure..." << std::endl;

	HttpResponse resp11;

	resp11.setBody("hello");
	resp11.setHeader("Connection", "keep-alive");

	output = resp11.toString();

	std::size_t headerEnd = output.find("\r\n\r\n");
	std::size_t bodyPosition = output.find("hello");

	if (headerEnd == std::string::npos)
	{
		std::cerr << "FAIL: Header/body separator missing" << std::endl;
		return 1;
	}

	if (bodyPosition == std::string::npos || bodyPosition <= headerEnd)
	{
		std::cerr << "FAIL: Body is not after headers" << std::endl;
		return 1;
	}

	std::cout << "PASS: Response structure correct" << std::endl;

	// ---------------------------------------------------------
	// Final result
	// ---------------------------------------------------------
	std::cout << "\n=== ALL HTTP RESPONSE TESTS PASSED ===" << std::endl;

	return 0;
}