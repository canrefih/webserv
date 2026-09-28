#include "../include/HttpResponse.hpp"

#include <iostream>
#include <string>
#include <cassert>

int main()
{
    std::cout << "=== HTTP RESPONSE TESTS ===" << std::endl;

    // ---------------------------------------------------------
    // Test 1: Default response
    // ---------------------------------------------------------
    std::cout << "\nTest 1: Check default response..." << std::endl;

    HttpResponse resp1;
    std::string output = resp1.toString();

    if (output.find("HTTP/1.1 200 OK\r\n") == std::string::npos)
    {
        std::cerr << "FAIL: Default status line incorrect" << std::endl;
        return 1;
    }

    if (output.find("Content-Type: text/plain\r\n") == std::string::npos)
    {
        std::cerr << "FAIL: Default content type incorrect" << std::endl;
        return 1;
    }

    if (output.find("Content-Length: 0\r\n") == std::string::npos)
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

    if (output.find("HTTP/1.1 404 Not Found\r\n") == std::string::npos)
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

    if (output.find("Content-Length: 30\r\n") == std::string::npos)
    {
        std::cerr << "FAIL: Content-Length incorrect" << std::endl;
        return 1;
    }

    if (output.find("Content-Type: text/html\r\n") == std::string::npos)
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

    if (output.find("Connection: keep-alive\r\n") == std::string::npos)
    {
        std::cerr << "FAIL: Custom Connection header not found" << std::endl;
        return 1;
    }

    if (output.find("Cache-Control: no-cache\r\n") == std::string::npos)
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

    if (output.find("X-Custom-1: value1\r\n") == std::string::npos ||
        output.find("X-Custom-2: value2\r\n") == std::string::npos ||
        output.find("X-Custom-3: value3\r\n") == std::string::npos)
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

    if (output.find("X-Test: second\r\n") == std::string::npos)
    {
        std::cerr << "FAIL: Custom header was not overwritten" << std::endl;
        return 1;
    }

    if (output.find("X-Test: first\r\n") != std::string::npos)
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

    if (output.find("Connection: keep-alive\r\n") == std::string::npos)
    {
        std::cerr << "FAIL: Keep-Alive header not generated" << std::endl;
        return 1;
    }

    if (output.find("Connection: close\r\n") != std::string::npos)
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

    if (output.find("Connection: close\r\n") == std::string::npos)
    {
        std::cerr << "FAIL: Connection close header not generated" << std::endl;
        return 1;
    }

    if (output.find("Connection: keep-alive\r\n") != std::string::npos)
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

    if (output.find("HTTP/1.1 500 Internal Server Error\r\n") == std::string::npos)
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

    if (output.find("Content-Length: 0\r\n") == std::string::npos)
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
    // Test 12: 201 Created
    // ---------------------------------------------------------
    std::cout << "\nTest 12: Test 201 Created..." << std::endl;

    HttpResponse resp12;
    resp12.setStatus(201, "Created");
    resp12.setBody("created");

    output = resp12.toString();

    if (output.find("HTTP/1.1 201 Created\r\n") == std::string::npos)
    {
        std::cerr << "FAIL: 201 status line incorrect" << std::endl;
        return 1;
    }

    std::cout << "PASS: 201 Created working" << std::endl;

    // ---------------------------------------------------------
    // Test 13: 400 Bad Request
    // ---------------------------------------------------------
    std::cout << "\nTest 13: Test 400 Bad Request..." << std::endl;

    HttpResponse resp13;
    resp13.setStatus(400, "Bad Request");
    resp13.setBody("bad request");

    output = resp13.toString();

    if (output.find("HTTP/1.1 400 Bad Request\r\n") == std::string::npos)
    {
        std::cerr << "FAIL: 400 status line incorrect" << std::endl;
        return 1;
    }

    std::cout << "PASS: 400 Bad Request working" << std::endl;

    // ---------------------------------------------------------
    // Test 14: 413 Payload Too Large
    // ---------------------------------------------------------
    std::cout << "\nTest 14: Test 413 Payload Too Large..." << std::endl;

    HttpResponse resp14;
    resp14.setStatus(413, "Payload Too Large");
    resp14.setBody("payload too large");

    output = resp14.toString();

    if (output.find("HTTP/1.1 413 Payload Too Large\r\n") == std::string::npos)
    {
        std::cerr << "FAIL: 413 status line incorrect" << std::endl;
        return 1;
    }

    std::cout << "PASS: 413 Payload Too Large working" << std::endl;

    // ---------------------------------------------------------
    // Test 15: Binary / null byte body
    // ---------------------------------------------------------
    std::cout << "\nTest 15: Test binary body with null byte..." << std::endl;

    HttpResponse resp15;
    std::string binaryBody("abc\0def", 7);
    resp15.setBody(binaryBody);

    output = resp15.toString();

    if (output.find("Content-Length: 7\r\n") == std::string::npos)
    {
        std::cerr << "FAIL: Binary body Content-Length incorrect" << std::endl;
        return 1;
    }

    std::size_t binaryBodyPosition = output.find("\r\n\r\n");

    if (binaryBodyPosition == std::string::npos)
    {
        std::cerr << "FAIL: Binary response separator missing" << std::endl;
        return 1;
    }

    binaryBodyPosition += 4;

    if (output.size() - binaryBodyPosition != 7)
    {
        std::cerr << "FAIL: Binary body size incorrect" << std::endl;
        return 1;
    }

    if (output.compare(binaryBodyPosition, 7, binaryBody) != 0)
    {
        std::cerr << "FAIL: Binary body content changed" << std::endl;
        return 1;
    }

    std::cout << "PASS: Binary body handled correctly" << std::endl;

    // ---------------------------------------------------------
    // Test 16: Custom Content-Length conflict
    // ---------------------------------------------------------
    std::cout << "\nTest 16: Test custom Content-Length header..." << std::endl;

    HttpResponse resp16;
    resp16.setBody("hello");
    resp16.setHeader("Content-Length", "999");

    output = resp16.toString();

    std::size_t firstLength = output.find("Content-Length:");
    std::size_t secondLength = output.find(
        "Content-Length:",
        firstLength == std::string::npos ? 0 : firstLength + 1
    );

    if (firstLength == std::string::npos)
    {
        std::cerr << "FAIL: Content-Length header missing" << std::endl;
        return 1;
    }

    if (secondLength != std::string::npos)
    {
        std::cerr << "FAIL: Duplicate Content-Length headers found" << std::endl;
        return 1;
    }

    if (output.find("Content-Length: 5\r\n") == std::string::npos)
    {
        std::cerr << "FAIL: Automatic Content-Length was replaced incorrectly"
                  << std::endl;
        return 1;
    }

    std::cout << "PASS: Content-Length conflict handled correctly" << std::endl;

    // ---------------------------------------------------------
    // Test 17: Custom Content-Type conflict
    // ---------------------------------------------------------
    std::cout << "\nTest 17: Test custom Content-Type header..." << std::endl;

    HttpResponse resp17;
    resp17.setContentType("text/html");
    resp17.setHeader("Content-Type", "application/json");

    output = resp17.toString();

    std::size_t firstType = output.find("Content-Type:");
    std::size_t secondType = output.find(
        "Content-Type:",
        firstType == std::string::npos ? 0 : firstType + 1
    );

    if (firstType == std::string::npos)
    {
        std::cerr << "FAIL: Content-Type header missing" << std::endl;
        return 1;
    }

    if (secondType != std::string::npos)
    {
        std::cerr << "FAIL: Duplicate Content-Type headers found" << std::endl;
        return 1;
    }

    std::cout << "PASS: Content-Type conflict handled correctly" << std::endl;

    // ---------------------------------------------------------
    // Test 18: Header CRLF injection protection
    // ---------------------------------------------------------
    std::cout << "\nTest 18: Test header CRLF injection..." << std::endl;

    HttpResponse resp18;
    resp18.setHeader(
        "X-Test",
        "safe\r\nInjected-Header: malicious"
    );

    output = resp18.toString();

    if (output.find("Injected-Header: malicious") != std::string::npos)
    {
        std::cerr << "FAIL: CRLF header injection is possible" << std::endl;
        return 1;
    }

    std::cout << "PASS: Header CRLF injection blocked" << std::endl;

    std::cout << "--- Running HttpResponse Tests ---" << std::endl;

    // Test 1: Basic Response Generation
    {
        HttpResponse res;
        res.setStatus(200, "OK");
        res.setBody("Hello World");
        
        std::string expected = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 11\r\n"
            "Content-Type: text/plain\r\n"
            "\r\n"
            "Hello World";

        assert(res.toString() == expected);
        std::cout << "PASS: Basic response generation" << std::endl;
    }

    // Test 2: Case-insensitive Header Override & Duplicate Prevention
    {
        HttpResponse res;
        res.setContentType("text/html");
        res.setHeader("content-type", "application/json"); // Case-insensitive duplicate
        res.setHeader("CONTENT-LENGTH", "999");            // Case-insensitive duplicate

        std::string raw = res.toString();
        
        // Ensure content-type/content-length appear only ONCE
        std::size_t ctCount = 0;
        std::size_t pos = 0;
        while ((pos = raw.find("Content-Type:", pos)) != std::string::npos) { ctCount++; pos++; }
        
        assert(ctCount == 1);
        assert(res.getHeader("CONTENT-TYPE") == "application/json"); // Case-insensitive getHeader
        std::cout << "PASS: Case-insensitive header override and duplicate prevention" << std::endl;
    }

    // Test 3: Set-Cookie Overwrite Behavior
    {
        HttpResponse res;
        res.setHeader("Set-Cookie", "id=123");
        res.setHeader("set-cookie", "theme=dark"); // Overwrites "id=123" (case-insensitive)

        std::string raw = res.toString();
        
        // "theme=dark" var olmalı, "id=123" ezilmiş olmalı
        assert(raw.find("theme=dark") != std::string::npos);
        assert(raw.find("id=123") == std::string::npos);
        std::cout << "PASS: Set-Cookie correctly overwrites previous value" << std::endl;
    }

    // Test 4: Empty Content-Type
    {
        HttpResponse res;
        res.setContentType(""); // Empty
        
        std::string raw = res.toString();
        assert(raw.find("Content-Type:") == std::string::npos);
        std::cout << "PASS: Empty Content-Type omitted correctly" << std::endl;
    }

    // Test 5: Header Injection Prevention (CRLF rejection)
    {
        HttpResponse res;
        res.setHeader("X-Header\r\nInjected", "Value");
        res.setHeader("X-Header2", "Value\r\nInjected");

        assert(res.getHeader("X-Header").empty());
        assert(res.getHeader("X-Header2").empty());
        std::cout << "PASS: CRLF Injection in headers rejected" << std::endl;
    }

    // ---------------------------------------------------------
    // Final result
    // ---------------------------------------------------------
    std::cout << "\n=== ALL HTTP RESPONSE TESTS PASSED ===" << std::endl;

    return 0;
}
