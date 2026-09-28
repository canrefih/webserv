// tests/test_person2_http.cpp

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <dirent.h>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdlib>

static int g_passed = 0;
static int g_failed = 0;

static void result(const std::string &name, bool ok)
{
    if (ok)
    {
        ++g_passed;
        std::cout << "[PASS] " << name << std::endl;
    }
    else
    {
        ++g_failed;
        std::cout << "[FAIL] " << name << std::endl;
    }
}

static int connectServer()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        return -1;

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);

    if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) != 1)
    {
        close(fd);
        return -1;
    }

    if (connect(fd, reinterpret_cast<struct sockaddr *>(&addr),
                sizeof(addr)) < 0)
    {
        close(fd);
        return -1;
    }

    return fd;
}

static std::string receiveResponse(int fd)
{
    std::string response;
    char buffer[4096];

    for (;;)
    {
        ssize_t n = recv(fd, buffer, sizeof(buffer), 0);

        if (n > 0)
        {
            response.append(buffer, static_cast<std::size_t>(n));

            /*
             * We only need enough data to determine the response.
             * Avoid waiting forever on keep-alive.
             */
            if (response.find("\r\n\r\n") != std::string::npos)
                break;
        }
        else if (n == 0)
        {
            break;
        }
        else
        {
            break;
        }
    }

    return response;
}

static std::string getStatusLine(const std::string &response)
{
    std::size_t end = response.find("\r\n");

    if (end == std::string::npos)
        return response;

    return response.substr(0, end);
}

static int getStatusCode(const std::string &response)
{
    std::istringstream stream(response);
    std::string version;
    int status = 0;

    stream >> version >> status;
    return status;
}

static std::string readTestFile(const std::string &path)
{
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file)
        return "";

    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
}

static std::string findLatestUpload()
{
    DIR *dir = opendir("www/uploads");
    if (!dir)
        return "";

    std::string latestPath;
    long latestNumber = -1;

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;

        if (name.find("upload-") != 0)
            continue;

        if (name.size() < 12 ||
            name.substr(name.size() - 4) != ".txt")
            continue;

        std::string numberText =
            name.substr(7, name.size() - 11);

        if (numberText.empty())
            continue;

        bool valid = true;
        for (std::size_t i = 0; i < numberText.size(); ++i)
        {
            if (numberText[i] < '0' || numberText[i] > '9')
            {
                valid = false;
                break;
            }
        }

        if (!valid)
            continue;

        long number = std::atol(numberText.c_str());

        if (number > latestNumber)
        {
            latestNumber = number;
            latestPath = "www/uploads/" + name;
        }
    }

    closedir(dir);
    return latestPath;
}

/*
 * Test the parser directly.
 *
 * This establishes whether HttpRequest itself understands
 * the Transfer-Encoding header. It should at least preserve
 * the header for Server-level framing logic.
 */
static void testChunkedHeaderParsing()
{
    HttpRequest request;

    std::string raw =
        "POST / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n";

    bool parsed = request.parse(raw);

    bool ok = parsed &&
              request.getHeader("Transfer-Encoding") == "chunked";

    result("HttpRequest parses Transfer-Encoding: chunked", ok);
}

/*
 * Current architecture handles HTTP framing in Server, not
 * HttpRequest. Therefore this integration test sends an
 * actual chunked request.
 *
 * / is expected to reject POST according to the standard test
 * configuration, but it must NOT answer 400 because of the
 * chunked framing.
 */
static void testChunkedRequest()
{
    std::string before = findLatestUpload();

    int fd = connectServer();

    if (fd < 0)
    {
        result("Chunked request body is decoded correctly", false);
        return;
    }

    const char *request =
        "POST /post_body HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Connection: close\r\n"
        "\r\n"
        "5\r\n"
        "hello\r\n"
        "0\r\n"
        "\r\n";

    send(fd, request, std::strlen(request), 0);

    std::string response = receiveResponse(fd);
    close(fd);

    std::string after = findLatestUpload();

    bool statusOk = getStatusCode(response) == 201;
    bool newFile = !after.empty() && after != before;
    bool bodyOk = newFile && readTestFile(after) == "hello";

    bool ok = statusOk && newFile && bodyOk;

    result("Chunked request body is decoded correctly", ok);

    if (!ok)
    {
        std::cout << "       response: "
                  << getStatusLine(response) << std::endl;

        if (!after.empty())
            std::cout << "       upload body: ["
                      << readTestFile(after)
                      << "]"
                      << std::endl;
    }
}

/*
 * Specifically test the terminating zero-size chunk.
 *
 * The current bug observed with tester is:
 *
 *   POST headers
 *   -> server processes request
 *   -> "0" remains in buffer
 *   -> server tries to parse "0" as another request
 *   -> 400
 *
 * This test is intentionally isolated around that behavior.
 */
static void testChunkedTerminatingChunk()
{
    int fd = connectServer();

    if (fd < 0)
    {
        result("Chunked terminating 0-chunk is handled", false);
        return;
    }

    const char *request =
        "POST / HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Connection: close\r\n"
        "\r\n"
        "0\r\n"
        "\r\n";

    send(fd, request, std::strlen(request), 0);

    std::string response = receiveResponse(fd);
    close(fd);

    int status = getStatusCode(response);

    bool ok = status != 400 && status != 0;

    result("Chunked terminating 0-chunk is handled", ok);

    if (!ok)
        std::cout << "       response: "
                  << getStatusLine(response) << std::endl;
}

/*
 * HTTP version handling.
 *
 * The server should reject unsupported HTTP versions instead
 * of treating them as normal requests.
 */
static void testUnsupportedHttpVersion()
{
    int fd = connectServer();

    if (fd < 0)
    {
        result("Unsupported HTTP version returns 505", false);
        return;
    }

    const char *request =
        "GET / HTTP/9.9\r\n"
        "Host: localhost:8080\r\n"
        "Connection: close\r\n"
        "\r\n";

    send(fd, request, std::strlen(request), 0);

    std::string response = receiveResponse(fd);
    close(fd);

    bool ok = getStatusCode(response) == 505;

    result("Unsupported HTTP version returns 505", ok);

    if (!ok)
        std::cout << "       response: "
                  << getStatusLine(response) << std::endl;
}

/*
 * HTTP/1.1 POST without Content-Length and without
 * Transfer-Encoding.
 *
 * This is specifically checking whether the server has
 * a 411 policy rather than blindly accepting the request.
 */
static void testMissingBodyLength()
{
    int fd = connectServer();

    if (fd < 0)
    {
        result("POST without body framing returns 411", false);
        return;
    }

    const char *request =
        "POST /post_body HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Connection: close\r\n"
        "\r\n";

    send(fd, request, std::strlen(request), 0);

    std::string response = receiveResponse(fd);
    close(fd);

    bool ok = getStatusCode(response) == 411;

    result("POST without body framing returns 411", ok);

    if (!ok)
        std::cout << "       response: "
                  << getStatusLine(response) << std::endl;
}

/*
 * Generic response header interface.
 */
static void testGenericResponseHeader()
{
    HttpResponse response;

    response.setStatus(200, "OK");
    response.setBody("hello");
    response.setHeader("Connection", "keep-alive");
    response.setHeader("X-Test-Header", "person2");

    std::string output = response.toString();

    bool ok =
        output.find("Connection: keep-alive\r\n") != std::string::npos &&
        output.find("X-Test-Header: person2\r\n") != std::string::npos;

    result("HttpResponse generic setHeader()", ok);
}

/*
 * Location header itself.
 *
 * Redirect generation may not exist yet, but the generic
 * header mechanism should be capable of carrying Location.
 */
static void testLocationHeader()
{
    HttpResponse response;

    response.setStatus(302, "Found");
    response.setHeader("Location", "/new-location");

    std::string output = response.toString();

    bool ok =
        output.find("Location: /new-location\r\n") != std::string::npos;

    result("HttpResponse can generate Location header", ok);
}

/*
 * Percent-decoded traversal must not accidentally expose
 * files outside the configured root.
 *
 * This is intentionally a network test: it does not assume
 * the exact error status, only that traversal isn't served
 * successfully.
 */
static void testEncodedTraversal()
{
    int fd = connectServer();

    if (fd < 0)
    {
        result("Encoded .. traversal is rejected", false);
        return;
    }

    const char *request =
        "GET /%2e%2e/%2e%2e/etc/passwd HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Connection: close\r\n"
        "\r\n";

    send(fd, request, std::strlen(request), 0);

    std::string response = receiveResponse(fd);
    close(fd);

    int status = getStatusCode(response);

    /*
     * Successful 200 would indicate a serious traversal problem.
     */
    bool ok = status != 200;

    result("Encoded .. traversal is rejected", ok);

    if (!ok)
        std::cout << "       response: "
                  << getStatusLine(response) << std::endl;
}

static void testChunkedKeepAlive()
{
    int fd = connectServer();

    if (fd < 0)
    {
        result("Chunked request followed by second request works", false);
        return;
    }

    const char *request1 =
        "POST / HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "5\r\n"
        "hello\r\n"
        "0\r\n"
        "\r\n";

    send(fd, request1, std::strlen(request1), 0);

    std::string response1 = receiveResponse(fd);

    const char *request2 =
        "GET / HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Connection: close\r\n"
        "\r\n";

    send(fd, request2, std::strlen(request2), 0);

    std::string response2 = receiveResponse(fd);

    close(fd);

    int status1 = getStatusCode(response1);
    int status2 = getStatusCode(response2);

    bool ok = status1 != 400 &&
              status1 != 0 &&
              status2 == 200;

    result("Chunked request followed by second request works", ok);

    if (!ok)
    {
        std::cout << "       response1: "
                  << getStatusLine(response1) << std::endl;
        std::cout << "       response2: "
                  << getStatusLine(response2) << std::endl;
    }
}

static void testChunkedPartialReceive()
{
    int fd = connectServer();

    if (fd < 0)
    {
        result("Chunked request survives partial TCP reads", false);
        return;
    }

    const char *part1 =
        "POST / HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "5\r\n"
        "hel";

    const char *part2 =
        "lo\r\n"
        "0\r\n"
        "\r\n";

    send(fd, part1, std::strlen(part1), 0);

    /*
     * Give the server a chance to process the incomplete chunk.
     * The request must NOT be processed yet.
     */
    usleep(100000);

    send(fd, part2, std::strlen(part2), 0);

    std::string response = receiveResponse(fd);
    close(fd);

    int status = getStatusCode(response);

    bool ok = status != 400 &&
              status != 0;

    result("Chunked request survives partial TCP reads", ok);

    if (!ok)
        std::cout << "       response: "
                  << getStatusLine(response) << std::endl;
}

static void testChunkedMultipleChunks()
{
    std::string before = findLatestUpload();

    int fd = connectServer();

    if (fd < 0)
    {
        result("Chunked request with multiple chunks works", false);
        return;
    }

    const char *request =
        "POST /post_body HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Connection: close\r\n"
        "\r\n"
        "5\r\n"
        "hello\r\n"
        "6\r\n"
        " world\r\n"
        "0\r\n"
        "\r\n";

    send(fd, request, std::strlen(request), 0);

    std::string response = receiveResponse(fd);
    close(fd);

    std::string after = findLatestUpload();

    bool statusOk = getStatusCode(response) == 201;
    bool newFile = !after.empty() && after != before;
    bool bodyOk = newFile && readTestFile(after) == "hello world";

    bool ok = statusOk && newFile && bodyOk;

    result("Chunked request with multiple chunks works", ok);

    if (!ok)
    {
        std::cout << "       response: "
                  << getStatusLine(response) << std::endl;

        if (!after.empty())
            std::cout << "       upload body: ["
                      << readTestFile(after)
                      << "]"
                      << std::endl;
    }
}

static void testChunkedHexSize()
{
    std::string before = findLatestUpload();

    int fd = connectServer();

    if (fd < 0)
    {
        result("Chunked hexadecimal size is parsed correctly", false);
        return;
    }

    const char *request =
        "POST /post_body HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Connection: close\r\n"
        "\r\n"
        "A\r\n"
        "0123456789\r\n"
        "0\r\n"
        "\r\n";

    send(fd, request, std::strlen(request), 0);

    std::string response = receiveResponse(fd);
    close(fd);

    std::string after = findLatestUpload();

    bool statusOk = getStatusCode(response) == 201;
    bool newFile = !after.empty() && after != before;
    bool bodyOk = newFile && readTestFile(after) == "0123456789";

    bool ok = statusOk && newFile && bodyOk;

    result("Chunked hexadecimal size is parsed correctly", ok);

    if (!ok)
    {
        std::cout << "       response: "
                  << getStatusLine(response) << std::endl;

        if (!after.empty())
            std::cout << "       upload body: ["
                      << readTestFile(after)
                      << "]"
                      << std::endl;
    }
}

static void testChunkedLowercaseHexSize()
{
    int fd = connectServer();

    if (fd < 0)
    {
        result("Chunked lowercase hexadecimal size works", false);
        return;
    }

    const char *request =
        "POST / HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Connection: close\r\n"
        "\r\n"
        "a\r\n"
        "0123456789\r\n"
        "0\r\n"
        "\r\n";

    send(fd, request, std::strlen(request), 0);

    std::string response = receiveResponse(fd);
    close(fd);

    int status = getStatusCode(response);

    bool ok = status != 400 && status != 0;

    result("Chunked lowercase hexadecimal size works", ok);

    if (!ok)
        std::cout << "       response: "
                  << getStatusLine(response) << std::endl;
}

static void testChunkedExtension()
{
    int fd = connectServer();

    if (fd < 0)
    {
        result("Chunk extension does not break framing", false);
        return;
    }

    const char *request =
        "POST / HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Connection: close\r\n"
        "\r\n"
        "5;foo=bar\r\n"
        "hello\r\n"
        "0\r\n"
        "\r\n";

    send(fd, request, std::strlen(request), 0);

    std::string response = receiveResponse(fd);
    close(fd);

    int status = getStatusCode(response);

    bool ok = status != 400 && status != 0;

    result("Chunk extension does not break framing", ok);

    if (!ok)
        std::cout << "       response: "
                  << getStatusLine(response) << std::endl;
}

static void testChunkedSplitHeader()
{
    int fd = connectServer();

    if (fd < 0)
    {
        result("Chunked header survives split TCP read", false);
        return;
    }

    const char *part1 =
        "POST / HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Connection: close\r\n"
        "\r\n"
        "5";

    const char *part2 =
        "\r\n"
        "hello\r\n"
        "0\r\n"
        "\r\n";

    send(fd, part1, std::strlen(part1), 0);

    usleep(100000);

    send(fd, part2, std::strlen(part2), 0);

    std::string response = receiveResponse(fd);
    close(fd);

    int status = getStatusCode(response);

    bool ok = status != 400 && status != 0;

    result("Chunked header survives split TCP read", ok);

    if (!ok)
        std::cout << "       response: "
                  << getStatusLine(response) << std::endl;
}

static void testChunkedSplitCrlf()
{
    int fd = connectServer();

    if (fd < 0)
    {
        result("Chunked CRLF survives split TCP read", false);
        return;
    }

    const char *part1 =
        "POST / HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Connection: close\r\n"
        "\r\n"
        "5\r\n"
        "hello\r";

    const char *part2 =
        "\n"
        "0\r\n"
        "\r\n";

    send(fd, part1, std::strlen(part1), 0);

    usleep(100000);

    send(fd, part2, std::strlen(part2), 0);

    std::string response = receiveResponse(fd);
    close(fd);

    int status = getStatusCode(response);

    bool ok = status != 400 && status != 0;

    result("Chunked CRLF survives split TCP read", ok);

    if (!ok)
        std::cout << "       response: "
                  << getStatusLine(response) << std::endl;
}

static void testChunkedInvalidSize()
{
    int fd = connectServer();

    if (fd < 0)
    {
        result("Malformed chunk size is rejected safely", false);
        return;
    }

    const char *request =
        "POST / HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Connection: close\r\n"
        "\r\n"
        "ZZZ\r\n"
        "hello\r\n"
        "0\r\n"
        "\r\n";

    send(fd, request, std::strlen(request), 0);

    std::string response = receiveResponse(fd);
    close(fd);

    int status = getStatusCode(response);

    /*
     * Exact error policy can be decided during implementation.
     * For now the important requirement is:
     *
     *   - no 200
     *   - no hang
     *   - no crash
     */
    bool ok = status != 200 && status != 0;

    result("Malformed chunk size is rejected safely", ok);

    if (!ok)
        std::cout << "       response: "
                  << getStatusLine(response) << std::endl;
}

static void testChunkedAndContentLength()
{
    int fd = connectServer();

    if (fd < 0)
    {
        result("Chunked plus Content-Length is handled safely", false);
        return;
    }

    const char *request =
        "POST / HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Content-Length: 5\r\n"
        "Connection: close\r\n"
        "\r\n"
        "5\r\n"
        "hello\r\n"
        "0\r\n"
        "\r\n";

    send(fd, request, std::strlen(request), 0);

    std::string response = receiveResponse(fd);
    close(fd);

    int status = getStatusCode(response);

    /*
     * We do not want this ambiguous framing to become a
     * successful normal request.
     */
    bool ok = status != 200;

    result("Chunked plus Content-Length is handled safely", ok);

    if (!ok)
        std::cout << "       response: "
                  << getStatusLine(response) << std::endl;
}

static void testChunkedHugeSize()
{
    int fd = connectServer();

    if (fd < 0)
    {
        result("Huge chunk size is handled without crashing", false);
        return;
    }

    const char *request =
        "POST / HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Connection: close\r\n"
        "\r\n"
        "FFFFFFFFFFFFFFFF\r\n";

    send(fd, request, std::strlen(request), 0);

    std::string response = receiveResponse(fd);
    close(fd);

    int status = getStatusCode(response);

    /*
     * We mainly care that the server does not interpret this
     * as a normal request or hang indefinitely.
     */
    bool ok = status != 200 && status != 0;

    result("Huge chunk size is handled without crashing", ok);

    if (!ok)
        std::cout << "       response: "
                  << getStatusLine(response) << std::endl;
}

static void testContentLengthZero()
{
    int fd = connectServer();
    if (fd < 0)
    {
        result("Content-Length: 0 is accepted", false);
        return;
    }

    const char *request =
        "POST /post_body HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n"
        "\r\n";

    send(fd, request, std::strlen(request), 0);
    std::string response = receiveResponse(fd);
    close(fd);

    bool ok = getStatusCode(response) == 201;
    result("Content-Length: 0 is accepted", ok);

    if (!ok)
        std::cout << "       response: "
                  << getStatusLine(response) << std::endl;
}

static void testContentLengthExact()
{
    int fd = connectServer();
    if (fd < 0)
    {
        result("Content-Length exact body length works", false);
        return;
    }

    const char *request =
        "POST /post_body HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Length: 5\r\n"
        "Connection: close\r\n"
        "\r\n"
        "hello";

    send(fd, request, std::strlen(request), 0);
    std::string response = receiveResponse(fd);
    close(fd);

    bool ok = getStatusCode(response) == 201;
    result("Content-Length exact body length works", ok);

    if (!ok)
        std::cout << "       response: "
                  << getStatusLine(response) << std::endl;
}

static void testContentLengthTooShort()
{
    std::string before = findLatestUpload();

    int fd = connectServer();
    if (fd < 0)
    {
        result("Content-Length shorter than available body is handled",
               false);
        return;
    }

    const char *request =
        "POST /post_body HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Length: 3\r\n"
        "Connection: close\r\n"
        "\r\n"
        "hello";

    send(fd, request, std::strlen(request), 0);
    std::string response = receiveResponse(fd);
    close(fd);

    std::string after = findLatestUpload();

    bool statusOk = getStatusCode(response) == 201;
    bool newFile = !after.empty() && after != before;

    /*
     * Content-Length says the body is exactly 3 bytes.
     * Therefore only "hel" belongs to this request.
     */
    bool bodyOk = newFile && readTestFile(after) == "hel";

    bool ok = statusOk && newFile && bodyOk;

    result("Content-Length shorter than available body is handled",
           ok);

    if (!ok)
    {
        std::cout << "       response: "
                  << getStatusLine(response) << std::endl;

        if (!after.empty())
            std::cout << "       upload body: ["
                      << readTestFile(after)
                      << "]"
                      << std::endl;
    }
}

static void testContentLengthInvalid()
{
    int fd = connectServer();
    if (fd < 0)
    {
        result("Invalid Content-Length is rejected", false);
        return;
    }

    const char *request =
        "POST /post_body HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Length: abc\r\n"
        "Connection: close\r\n"
        "\r\n"
        "hello";

    send(fd, request, std::strlen(request), 0);
    std::string response = receiveResponse(fd);
    close(fd);

    bool ok = getStatusCode(response) == 400;

    result("Invalid Content-Length is rejected", ok);

    if (!ok)
        std::cout << "       response: "
                  << getStatusLine(response) << std::endl;
}

static void testContentLengthNegative()
{
    int fd = connectServer();
    if (fd < 0)
    {
        result("Negative Content-Length is rejected", false);
        return;
    }

    const char *request =
        "POST /post_body HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Length: -1\r\n"
        "Connection: close\r\n"
        "\r\n";

    send(fd, request, std::strlen(request), 0);
    std::string response = receiveResponse(fd);
    close(fd);

    bool ok = getStatusCode(response) == 400;

    result("Negative Content-Length is rejected", ok);

    if (!ok)
        std::cout << "       response: "
                  << getStatusLine(response) << std::endl;
}

static void testContentLengthHuge()
{
    int fd = connectServer();
    if (fd < 0)
    {
        result("Huge Content-Length is rejected safely", false);
        return;
    }

    const char *request =
        "POST /post_body HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Length: 999999999999999999999999999999\r\n"
        "Connection: close\r\n"
        "\r\n";

    send(fd, request, std::strlen(request), 0);
    std::string response = receiveResponse(fd);
    close(fd);

    int status = getStatusCode(response);

    bool ok = status == 400 || status == 413;

    result("Huge Content-Length is rejected safely", ok);

    if (!ok)
        std::cout << "       response: "
                  << getStatusLine(response) << std::endl;
}

static void testContentLengthDuplicate()
{
    int fd = connectServer();
    if (fd < 0)
    {
        result("Duplicate Content-Length is rejected", false);
        return;
    }

    /*
     * Both values are followed by 10 bytes so that a broken
     * implementation cannot get stuck waiting for a body.
     */
    const char *request =
        "POST /post_body HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Length: 5\r\n"
        "Content-Length: 10\r\n"
        "Connection: close\r\n"
        "\r\n"
        "helloworld";

    send(fd, request, std::strlen(request), 0);
    std::string response = receiveResponse(fd);
    close(fd);

    bool ok = getStatusCode(response) == 400;

    result("Duplicate Content-Length is rejected", ok);

    if (!ok)
        std::cout << "       response: "
                  << getStatusLine(response) << std::endl;
}

int main()
{
    std::cout << "=== PERSON 2 HTTP AUDIT ===" << std::endl;
    std::cout << "Server must be running on 127.0.0.1:8080"
              << std::endl;
    std::cout << std::endl;

    testChunkedHeaderParsing();
    testChunkedRequest();
    testChunkedTerminatingChunk();
    testChunkedMultipleChunks();
    testChunkedHexSize();
    testChunkedLowercaseHexSize();
    testChunkedExtension();
    testChunkedSplitHeader();
    testChunkedSplitCrlf();
    testChunkedInvalidSize();
    testChunkedAndContentLength();
    testChunkedHugeSize();

    testChunkedKeepAlive();
    testChunkedPartialReceive();

    testUnsupportedHttpVersion();
    testMissingBodyLength();

    testGenericResponseHeader();
    testLocationHeader();

    testEncodedTraversal();

    testContentLengthZero();
    testContentLengthExact();
    testContentLengthTooShort();
    testContentLengthInvalid();
    testContentLengthNegative();
    testContentLengthHuge();
    testContentLengthDuplicate();

    std::cout << std::endl;
    std::cout << "=== RESULT ===" << std::endl;
    std::cout << "Passed: " << g_passed << std::endl;
    std::cout << "Failed: " << g_failed << std::endl;

    return g_failed == 0 ? 0 : 1;
}