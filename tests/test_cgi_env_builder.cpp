#include "CGIEnvBuilder.hpp"
#include "HttpRequest.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>

static int g_tests = 0;
static int g_passed = 0;

static void check(bool condition, const std::string &testName)
{
    ++g_tests;

    if (condition)
    {
        ++g_passed;
        std::cout << "[PASS] " << testName << std::endl;
    }
    else
        std::cout << "[FAIL] " << testName << std::endl;
}

static bool hasEnv(const std::vector<std::string> &env,
                   const std::string &expected)
{
    for (std::size_t i = 0; i < env.size(); ++i)
    {
        if (env[i] == expected)
            return (true);
    }
    return (false);
}

static bool hasPrefix(const std::vector<std::string> &env,
                      const std::string &prefix)
{
    for (std::size_t i = 0; i < env.size(); ++i)
    {
        if (env[i].compare(0, prefix.size(), prefix) == 0)
            return (true);
    }
    return (false);
}

static int countPrefix(const std::vector<std::string> &env,
                       const std::string &prefix)
{
    int count = 0;

    for (std::size_t i = 0; i < env.size(); ++i)
    {
        if (env[i].compare(0, prefix.size(), prefix) == 0)
            ++count;
    }
    return (count);
}

static HttpRequest makeRequest(const std::string &rawRequest)
{
    HttpRequest request;

    if (!request.parse(rawRequest))
    {
        std::cerr << "[ERROR] HttpRequest::parse() failed." << std::endl;
        std::exit(1);
    }

    return (request);
}

static void testBasicGet()
{
    HttpRequest request = makeRequest(
        "GET /cgi-bin/hello.py HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "User-Agent: TestBrowser/1.0\r\n"
        "\r\n"
    );

    std::vector<std::string> env = buildCGIEnv(
        request,
        "example.com",
        8080,
        "/var/www/cgi-bin/hello.py"
    );

    check(hasEnv(env, "REQUEST_METHOD=GET"),
          "GET: REQUEST_METHOD");

    check(hasEnv(env, "SCRIPT_NAME=/cgi-bin/hello.py"),
          "GET: SCRIPT_NAME");

    check(hasEnv(env, "SCRIPT_FILENAME=/var/www/cgi-bin/hello.py"),
          "GET: SCRIPT_FILENAME");

    check(hasEnv(env, "QUERY_STRING="),
          "GET: empty QUERY_STRING");

    check(hasEnv(env, "SERVER_PROTOCOL=HTTP/1.1"),
          "GET: SERVER_PROTOCOL");

    check(hasEnv(env, "SERVER_NAME=example.com"),
          "GET: SERVER_NAME");

    check(hasEnv(env, "SERVER_PORT=8080"),
          "GET: SERVER_PORT");

    check(hasEnv(env, "GATEWAY_INTERFACE=CGI/1.1"),
          "GET: GATEWAY_INTERFACE");

    check(hasEnv(env, "REDIRECT_STATUS=200"),
          "GET: REDIRECT_STATUS");

    check(hasEnv(env, "CONTENT_LENGTH=0"),
          "GET: CONTENT_LENGTH");

    check(hasEnv(env, "CONTENT_TYPE="),
          "GET: empty CONTENT_TYPE");

    check(hasEnv(env, "HTTP_USER_AGENT=TestBrowser/1.0"),
          "GET: User-Agent -> HTTP_USER_AGENT");
}

static void testQueryString()
{
    HttpRequest request = makeRequest(
        "GET /cgi-bin/test.py?name=alice&age=25 HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "\r\n"
    );

    std::vector<std::string> env = buildCGIEnv(
        request,
        "example.com",
        8080,
        "/var/www/cgi-bin/test.py"
    );

    check(hasEnv(env, "SCRIPT_NAME=/cgi-bin/test.py"),
          "Query: SCRIPT_NAME excludes query");

    check(hasEnv(env, "QUERY_STRING=name=alice&age=25"),
          "Query: QUERY_STRING");
}

static void testPost()
{
    HttpRequest request = makeRequest(
        "POST /cgi-bin/test.py HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: 19\r\n"
        "\r\n"
        "name=alice&age=25"
    );

    std::vector<std::string> env = buildCGIEnv(
        request,
        "example.com",
        8080,
        "/var/www/cgi-bin/test.py"
    );

    check(hasEnv(env, "REQUEST_METHOD=POST"),
          "POST: REQUEST_METHOD");

    check(hasEnv(env, "CONTENT_TYPE=application/x-www-form-urlencoded"),
          "POST: CONTENT_TYPE");

    check(hasEnv(env, "CONTENT_LENGTH=17"),
          "POST: CONTENT_LENGTH from actual body size");
}

static void testPostWithQuery()
{
    HttpRequest request = makeRequest(
        "POST /cgi-bin/test.py?foo=bar&x=123 HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "hello=world"
    );

    std::vector<std::string> env = buildCGIEnv(
        request,
        "example.com",
        8080,
        "/var/www/cgi-bin/test.py"
    );

    check(hasEnv(env, "SCRIPT_NAME=/cgi-bin/test.py"),
          "POST + query: SCRIPT_NAME");

    check(hasEnv(env, "QUERY_STRING=foo=bar&x=123"),
          "POST + query: QUERY_STRING");

    check(hasEnv(env, "CONTENT_LENGTH=11"),
          "POST + query: CONTENT_LENGTH");

    check(hasEnv(env, "CONTENT_TYPE=application/x-www-form-urlencoded"),
          "POST + query: CONTENT_TYPE");
}

static void testCustomHeaders()
{
    HttpRequest request = makeRequest(
        "GET /cgi-bin/test.py HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "User-Agent: TestAgent\r\n"
        "Accept: text/html\r\n"
        "X-Test-Header: hello\r\n"
        "X-Custom-Value: 12345\r\n"
        "\r\n"
    );

    std::vector<std::string> env = buildCGIEnv(
        request,
        "example.com",
        8080,
        "/var/www/cgi-bin/test.py"
    );

    check(hasEnv(env, "HTTP_USER_AGENT=TestAgent"),
          "Headers: User-Agent conversion");

    check(hasEnv(env, "HTTP_ACCEPT=text/html"),
          "Headers: Accept conversion");

    check(hasEnv(env, "HTTP_X_TEST_HEADER=hello"),
          "Headers: X-Test-Header conversion");

    check(hasEnv(env, "HTTP_X_CUSTOM_VALUE=12345"),
          "Headers: X-Custom-Value conversion");
}

static void testContentHeadersNotDuplicated()
{
    HttpRequest request = makeRequest(
        "POST /cgi-bin/test.py HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "hello"
    );

    std::vector<std::string> env = buildCGIEnv(
        request,
        "example.com",
        8080,
        "/var/www/cgi-bin/test.py"
    );

    check(hasEnv(env, "CONTENT_TYPE=text/plain"),
          "Content headers: CONTENT_TYPE exists");

    check(hasEnv(env, "CONTENT_LENGTH=5"),
          "Content headers: CONTENT_LENGTH exists");

    check(!hasPrefix(env, "HTTP_CONTENT_TYPE="),
          "Content headers: HTTP_CONTENT_TYPE not duplicated");

    check(!hasPrefix(env, "HTTP_CONTENT_LENGTH="),
          "Content headers: HTTP_CONTENT_LENGTH not duplicated");

    check(countPrefix(env, "CONTENT_TYPE=") == 1,
          "Content headers: CONTENT_TYPE appears once");

    check(countPrefix(env, "CONTENT_LENGTH=") == 1,
          "Content headers: CONTENT_LENGTH appears once");
}

static void testEmptyBody()
{
    HttpRequest request = makeRequest(
        "POST /cgi-bin/test.py HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 0\r\n"
        "\r\n"
    );

    std::vector<std::string> env = buildCGIEnv(
        request,
        "example.com",
        8080,
        "/var/www/cgi-bin/test.py"
    );

    check(hasEnv(env, "CONTENT_LENGTH=0"),
          "Empty body: CONTENT_LENGTH=0");

    check(hasEnv(env, "CONTENT_TYPE=text/plain"),
          "Empty body: CONTENT_TYPE");
}

static void testServerInformation()
{
    HttpRequest request = makeRequest(
        "GET /index.html HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n"
    );

    std::vector<std::string> env = buildCGIEnv(
        request,
        "my-web-server",
        4242,
        "/srv/www/index.html"
    );

    check(hasEnv(env, "SERVER_NAME=my-web-server"),
          "Server info: SERVER_NAME");

    check(hasEnv(env, "SERVER_PORT=4242"),
          "Server info: SERVER_PORT");

    check(hasEnv(env, "SERVER_PROTOCOL=HTTP/1.1"),
          "Server info: SERVER_PROTOCOL");
}

int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << "      CGIEnvBuilder Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;

    testBasicGet();
    testQueryString();
    testPost();
    testPostWithQuery();
    testCustomHeaders();
    testContentHeadersNotDuplicated();
    testEmptyBody();
    testServerInformation();

    std::cout << "========================================" << std::endl;
    std::cout << "Passed: " << g_passed << "/" << g_tests << std::endl;
    std::cout << "========================================" << std::endl;

    if (g_passed == g_tests)
    {
        std::cout << "ALL TESTS PASSED!" << std::endl;
        return (0);
    }

    std::cout << "SOME TESTS FAILED!" << std::endl;
    return (1);
}