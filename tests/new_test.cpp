#include "RequestHandler.hpp"
#include "ServerConfig.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

#define WWW_ROOT "./test_www"

static void writeFile(const std::string &path, const std::string &content)
{
    std::ofstream file(path.c_str());
    file << content;
    file.close();
}

static void setupTestEnvironment()
{
    mkdir(WWW_ROOT, 0755);
    mkdir((std::string(WWW_ROOT) + "/uploads").c_str(), 0755);
}

static void runRequest(RequestHandler &handler, const std::string &method, const std::string &uri, HttpResponse &response, const std::string &body = "")
{
    HttpRequest request;
    std::ostringstream raw;
    raw << method << " " << uri << " HTTP/1.1\r\n";
    raw << "Host: localhost\r\n";
    if (!body.empty())
        raw << "Content-Length: " << body.size() << "\r\n";
    raw << "\r\n";
    raw << body;

    request.parse(raw.str());
    handler.handleRequest(request, response);
}

// HttpResponse nesnesinden ham metni çeken yardımcı fonksiyon.
// NOT: HttpResponse sınıfınızdaki string üretme metodunun adı 'toString()' değilse 
// buradaki res.toString() kısmını projenizdeki metot adı ile güncelleyin (örn: res.toRawString()).
static std::string getRawResponse(HttpResponse &res)
{
    return res.toString();
}

static int getStatusCode(HttpResponse &res)
{
    std::string raw = getRawResponse(res);
    std::istringstream iss(raw);
    std::string version;
    int code = 0;
    iss >> version >> code;
    return code;
}

static std::string getResponseBody(HttpResponse &res)
{
    std::string raw = getRawResponse(res);
    size_t bodyPos = raw.find("\r\n\r\n");
    if (bodyPos != std::string::npos)
        return raw.substr(bodyPos + 4);
    return "";
}

static std::string getHeader(HttpResponse &res, const std::string &key)
{
    std::string raw = getRawResponse(res);
    std::istringstream iss(raw);
    std::string line;
    while (std::getline(iss, line))
    {
        if (line.find(key + ":") == 0)
        {
            size_t valPos = line.find(":") + 1;
            while (valPos < line.size() && (line[valPos] == ' ' || line[valPos] == '\t'))
                valPos++;
            size_t endPos = line.find_last_not_of("\r\n");
            return line.substr(valPos, endPos - valPos + 1);
        }
    }
    return "";
}

static void printResult(const std::string &testName, bool ok)
{
    if (ok)
        std::cout << "[PASS] " << testName << std::endl;
    else
        std::cout << "[FAIL] " << testName << std::endl;
}

// ------------------ TEST SENARYOLARI ------------------

static void testHttpRedirection()
{
    ServerConfig config;
    config.setRoot(WWW_ROOT);
    Location loc;
    loc.setPath("/old-path");
    // Location::setRedirection (int code, const std::string &path)
    loc.setRedirection(301, "http://localhost/new-path");
    config.addLocation(loc);

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler, "GET", "/old-path", response);

    bool ok = (getStatusCode(response) == 301) &&
              (getHeader(response, "Location") == "http://localhost/new-path");

    printResult("HTTP Redirection (301 Redirect)", ok);
}

static void testMaxBodySizeExceeded()
{
    ServerConfig config;
    config.setRoot(WWW_ROOT);
    config.setClientMaxBodySize(10);

    RequestHandler handler(config);
    HttpResponse response;

    std::string largeBody = "This is a body larger than 10 bytes";
    runRequest(handler, "POST", "/upload", response, largeBody);

    bool ok = (getStatusCode(response) == 413);

    printResult("Max Body Size Enforcement (413 Payload Too Large)", ok);
}

static void testPathTraversalGuard()
{
    ServerConfig config;
    config.setRoot(WWW_ROOT);

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler, "GET", "/../../etc/passwd", response);

    bool ok = (getStatusCode(response) == 400);

    printResult("Path Traversal Guard (400 Bad Request)", ok);
}

static void testPostWithoutUpload()
{
    ServerConfig config;
    config.setRoot(WWW_ROOT);
    Location loc;
    loc.setPath("/no-upload");
    loc.setUpload(false);
    config.addLocation(loc);

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler, "POST", "/no-upload", response, "data");

    bool ok = (getStatusCode(response) == 405);

    printResult("POST with Upload Disabled (405 Method Not Allowed)", ok);
}

static void testCustomErrorPage()
{
    ServerConfig config;
    config.setRoot(WWW_ROOT);
    config.setErrorPage(404, "/404.html");
    writeFile(std::string(WWW_ROOT) + "/404.html", "Custom 404 Page Content");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler, "GET", "/non-existent-file.txt", response);

    bool ok = (getStatusCode(response) == 404) &&
              (getResponseBody(response) == "Custom 404 Page Content");

    printResult("Custom Error Page (404 Page Integration)", ok);
}

int main()
{
    setupTestEnvironment();

    std::cout << "--- WEBSERV INTEGRATION TESTS ---" << std::endl;
    testHttpRedirection();
    testMaxBodySizeExceeded();
    testPathTraversalGuard();
    testPostWithoutUpload();
    testCustomErrorPage();

    return 0;
}