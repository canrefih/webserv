#include "RequestHandler.hpp"
#include "ServerConfig.hpp"
#include "Location.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cassert>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

static int g_passed = 0;
static int g_failed = 0;

static const std::string TEST_ROOT = "/tmp/webserv_request_handler_test";
static const std::string WWW_ROOT = TEST_ROOT + "/www";
static const std::string LOC_ROOT = TEST_ROOT + "/static";
static const std::string UPLOAD_ROOT = TEST_ROOT + "/uploads";

static void printResult(const std::string &name, bool ok)
{
    if (ok)
    {
        std::cout << "[PASS] " << name << std::endl;
        ++g_passed;
    }
    else
    {
        std::cout << "[FAIL] " << name << std::endl;
        ++g_failed;
    }
}

static bool fileExists(const std::string &path)
{
    struct stat st;

    if (stat(path.c_str(), &st) != 0)
        return false;

    return S_ISREG(st.st_mode);
}

static bool directoryExists(const std::string &path)
{
    struct stat st;

    if (stat(path.c_str(), &st) != 0)
        return false;

    return S_ISDIR(st.st_mode);
}

static bool writeFile(const std::string &path, const std::string &content)
{
    std::ofstream file(path.c_str(), std::ios::binary);

    if (!file.is_open())
        return false;

    file.write(content.c_str(), content.size());

    return file.good();
}

static std::string readFile(const std::string &path)
{
    std::ifstream file(path.c_str(), std::ios::binary);

    if (!file.is_open())
        return "";

    std::ostringstream content;
    content << file.rdbuf();

    return content.str();
}

static bool makeDirectory(const std::string &path)
{
    if (mkdir(path.c_str(), 0755) == 0)
        return true;

    return directoryExists(path);
}

static bool removeDirectoryContents(const std::string &path)
{
    DIR *dir = opendir(path.c_str());

    if (dir == NULL)
        return true;

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;

        if (name == "." || name == "..")
            continue;

        std::string fullPath = path + "/" + name;

        struct stat st;

        if (stat(fullPath.c_str(), &st) != 0)
        {
            closedir(dir);
            return false;
        }

        if (S_ISDIR(st.st_mode))
        {
            if (!removeDirectoryContents(fullPath))
            {
                closedir(dir);
                return false;
            }

            if (rmdir(fullPath.c_str()) != 0)
            {
                closedir(dir);
                return false;
            }
        }
        else
        {
            if (unlink(fullPath.c_str()) != 0)
            {
                closedir(dir);
                return false;
            }
        }
    }

    closedir(dir);
    return true;
}

static bool cleanTestEnvironment()
{
    if (!directoryExists(TEST_ROOT))
        return true;

    if (!removeDirectoryContents(TEST_ROOT))
        return false;

    return true;
}

static bool setupTestEnvironment()
{
    if (!cleanTestEnvironment())
        return false;

    if (!makeDirectory(TEST_ROOT))
        return false;

    if (!makeDirectory(WWW_ROOT))
        return false;

    if (!makeDirectory(LOC_ROOT))
        return false;

    if (!makeDirectory(UPLOAD_ROOT))
        return false;

    if (!makeDirectory(WWW_ROOT + "/dir"))
        return false;

    if (!makeDirectory(WWW_ROOT + "/emptydir"))
        return false;

    if (!makeDirectory(WWW_ROOT + "/nested"))
        return false;

    if (!makeDirectory(WWW_ROOT + "/nested/child"))
        return false;

    return true;
}

static HttpRequest makeRequest(const std::string &method,
                               const std::string &target,
                               const std::string &body = "")
{
    HttpRequest request;

    std::string raw;

    raw += method;
    raw += " ";
    raw += target;
    raw += " HTTP/1.1\r\n";
    raw += "Host: localhost\r\n";
    raw += "\r\n";
    raw += body;

    if (!request.parse(raw))
    {
        std::cerr << "Could not parse test request: "
                  << method << " " << target << std::endl;
    }

    return request;
}

static int getStatusCode(const HttpResponse &response)
{
    std::string raw = response.toString();

    std::size_t firstSpace = raw.find(' ');

    if (firstSpace == std::string::npos)
        return -1;

    std::size_t secondSpace = raw.find(' ', firstSpace + 1);

    if (secondSpace == std::string::npos)
        return -1;

    std::string code = raw.substr(firstSpace + 1,
                                  secondSpace - firstSpace - 1);

    return atoi(code.c_str());
}

static std::string getResponseBody(const HttpResponse &response)
{
    std::string raw = response.toString();

    std::size_t separator = raw.find("\r\n\r\n");

    if (separator == std::string::npos)
        return "";

    return raw.substr(separator + 4);
}

static bool responseHasHeader(const HttpResponse &response,
                              const std::string &name,
                              const std::string &value)
{
    std::string raw = response.toString();

    std::string expected = name + ": " + value + "\r\n";

    return raw.find(expected) != std::string::npos;
}

static void runRequest(RequestHandler &handler,
                       const std::string &method,
                       const std::string &target,
                       HttpResponse &response,
                       const std::string &body = "")
{
    HttpRequest request = makeRequest(method, target, body);

    handler.handleRequest(request, response);
}

/*
 * ------------------------------------------------------------
 * Configuration helpers
 * ------------------------------------------------------------
 */

static ServerConfig makeBaseConfig()
{
    ServerConfig config;

    config.setRoot(WWW_ROOT);
    config.setIndex("index.html");
    config.setAutoIndex(false);
    config.setUploadPath(UPLOAD_ROOT);
    config.setClientMaxBodySize(2 * 1024 * 1024);

    return config;
}

static Location makeStaticLocation()
{
    Location location("/static");

    location.setRoot(LOC_ROOT);
    location.setIndex("location.html");

    return location;
}

static Location makeUploadLocation()
{
    Location location("/upload");

    location.setUpload(true);
    location.setUploadStore(UPLOAD_ROOT);

    location.addMethod("GET");
    location.addMethod("POST");
    location.addMethod("DELETE");

    return location;
}

/*
 * ------------------------------------------------------------
 * GET tests
 * ------------------------------------------------------------
 */

static void testGetExistingFile()
{
    writeFile(WWW_ROOT + "/hello.txt", "Hello Webserv!");

    ServerConfig config = makeBaseConfig();
    RequestHandler handler(config);

    HttpResponse response;

    runRequest(handler, "GET", "/hello.txt", response);

    bool ok = getStatusCode(response) == 200 &&
              getResponseBody(response) == "Hello Webserv!" &&
              responseHasHeader(response,
                                "Content-Type",
                                "text/plain");

    printResult("GET existing file", ok);
}

static void testGetMissingFile()
{
    ServerConfig config = makeBaseConfig();
    RequestHandler handler(config);

    HttpResponse response;

    runRequest(handler, "GET", "/does-not-exist.txt", response);

    bool ok = getStatusCode(response) == 404;

    printResult("GET missing file -> 404", ok);
}

static void testGetEmptyFile()
{
    writeFile(WWW_ROOT + "/empty.txt", "");

    ServerConfig config = makeBaseConfig();
    RequestHandler handler(config);

    HttpResponse response;

    runRequest(handler, "GET", "/empty.txt", response);

    bool ok = getStatusCode(response) == 200 &&
              getResponseBody(response).empty();

    printResult("GET empty file", ok);
}

static void testGetBinaryFile()
{
    std::string binary;

    binary.push_back('\0');
    binary.push_back('\1');
    binary.push_back('\2');
    binary.push_back('\n');
    binary.push_back('\r');
    binary.push_back(static_cast<char>(0xff));
    binary.push_back('A');

    writeFile(WWW_ROOT + "/binary.bin", binary);

    ServerConfig config = makeBaseConfig();
    RequestHandler handler(config);

    HttpResponse response;

    runRequest(handler, "GET", "/binary.bin", response);

    bool ok = getStatusCode(response) == 200 &&
              getResponseBody(response) == binary &&
              responseHasHeader(response,
                                "Content-Type",
                                "application/octet-stream");

    printResult("GET binary file", ok);
}

static void testGetContentTypes()
{
    writeFile(WWW_ROOT + "/page.html", "html");
    writeFile(WWW_ROOT + "/style.css", "css");
    writeFile(WWW_ROOT + "/script.js", "js");
    writeFile(WWW_ROOT + "/data.json", "json");
    writeFile(WWW_ROOT + "/image.png", "png");
    writeFile(WWW_ROOT + "/image.jpg", "jpg");
    writeFile(WWW_ROOT + "/image.jpeg", "jpeg");
    writeFile(WWW_ROOT + "/image.gif", "gif");
    writeFile(WWW_ROOT + "/image.svg", "svg");
    writeFile(WWW_ROOT + "/unknown.xyz", "unknown");

    ServerConfig config = makeBaseConfig();
    RequestHandler handler(config);

    bool ok = true;

    {
        HttpResponse response;
        runRequest(handler, "GET", "/page.html", response);
        ok = ok && responseHasHeader(response,
                                     "Content-Type",
                                     "text/html");
    }

    {
        HttpResponse response;
        runRequest(handler, "GET", "/style.css", response);
        ok = ok && responseHasHeader(response,
                                     "Content-Type",
                                     "text/css");
    }

    {
        HttpResponse response;
        runRequest(handler, "GET", "/script.js", response);
        ok = ok && responseHasHeader(response,
                                     "Content-Type",
                                     "application/javascript");
    }

    {
        HttpResponse response;
        runRequest(handler, "GET", "/data.json", response);
        ok = ok && responseHasHeader(response,
                                     "Content-Type",
                                     "application/json");
    }

    {
        HttpResponse response;
        runRequest(handler, "GET", "/image.png", response);
        ok = ok && responseHasHeader(response,
                                     "Content-Type",
                                     "image/png");
    }

    {
        HttpResponse response;
        runRequest(handler, "GET", "/image.jpg", response);
        ok = ok && responseHasHeader(response,
                                     "Content-Type",
                                     "image/jpeg");
    }

    {
        HttpResponse response;
        runRequest(handler, "GET", "/image.jpeg", response);
        ok = ok && responseHasHeader(response,
                                     "Content-Type",
                                     "image/jpeg");
    }

    {
        HttpResponse response;
        runRequest(handler, "GET", "/image.gif", response);
        ok = ok && responseHasHeader(response,
                                     "Content-Type",
                                     "image/gif");
    }

    {
        HttpResponse response;
        runRequest(handler, "GET", "/image.svg", response);
        ok = ok && responseHasHeader(response,
                                     "Content-Type",
                                     "image/svg+xml");
    }

    {
        HttpResponse response;
        runRequest(handler, "GET", "/unknown.xyz", response);
        ok = ok && responseHasHeader(response,
                                     "Content-Type",
                                     "application/octet-stream");
    }

    printResult("GET content-type mapping", ok);
}

static void testGetIndex()
{
    writeFile(WWW_ROOT + "/dir/index.html", "Directory index");

    ServerConfig config = makeBaseConfig();
    RequestHandler handler(config);

    HttpResponse response;

    runRequest(handler, "GET", "/dir/", response);

    bool ok = getStatusCode(response) == 200 &&
              getResponseBody(response) == "Directory index";

    printResult("GET directory with index", ok);
}

static void testGetIndexWithoutTrailingSlash()
{
    writeFile(WWW_ROOT + "/dir/index.html", "Directory index");

    ServerConfig config = makeBaseConfig();
    RequestHandler handler(config);

    HttpResponse response;

    runRequest(handler, "GET", "/dir", response);

    bool ok = getStatusCode(response) == 200 &&
              getResponseBody(response) == "Directory index";

    printResult("GET directory without trailing slash", ok);
}

static void testGetAutoIndex()
{
    writeFile(WWW_ROOT + "/emptydir/a.txt", "A");
    writeFile(WWW_ROOT + "/emptydir/b.txt", "B");

    ServerConfig config = makeBaseConfig();
    config.setAutoIndex(true);

    RequestHandler handler(config);

    HttpResponse response;

    runRequest(handler, "GET", "/emptydir/", response);

    std::string body = getResponseBody(response);

    bool ok = getStatusCode(response) == 200 &&
              responseHasHeader(response,
                                "Content-Type",
                                "text/html") &&
              body.find("a.txt") != std::string::npos &&
              body.find("b.txt") != std::string::npos;

    printResult("GET autoindex directory", ok);
}

// A directory with no index and autoindex off has nothing to show: 404 (expected by the 42 tester, allowed by RFC 9110 instead of 403)
static void testGetDirectoryNotFoundWithoutAutoIndex()
{
    ServerConfig config = makeBaseConfig();
    config.setAutoIndex(false);

    RequestHandler handler(config);

    HttpResponse response;

    runRequest(handler, "GET", "/emptydir/", response);

    bool ok = getStatusCode(response) == 404;

    printResult("GET directory without index/autoindex -> 404", ok);
}

static void testGetLocationRoot()
{
    writeFile(LOC_ROOT + "/location.html", "Location root works");

    ServerConfig config = makeBaseConfig();

    Location location = makeStaticLocation();
    config.addLocation(location);

    RequestHandler handler(config);

    HttpResponse response;

    runRequest(handler, "GET", "/static/location.html", response);

    bool ok = getStatusCode(response) == 200 &&
              getResponseBody(response) == "Location root works";

    printResult("GET location-specific root", ok);
}

static void testGetLocationIndex()
{
    writeFile(LOC_ROOT + "/location.html", "Location index");

    ServerConfig config = makeBaseConfig();

    Location location = makeStaticLocation();
    config.addLocation(location);

    RequestHandler handler(config);

    HttpResponse response;

    runRequest(handler, "GET", "/static/", response);

    bool ok = getStatusCode(response) == 200 &&
              getResponseBody(response) == "Location index";

    printResult("GET location-specific index", ok);
}

static void testGetQueryString()
{
    writeFile(WWW_ROOT + "/query.txt", "query works");

    ServerConfig config = makeBaseConfig();
    RequestHandler handler(config);

    HttpResponse response;

    runRequest(handler,
               "GET",
               "/query.txt?foo=bar&hello=world",
               response);

    bool ok = getStatusCode(response) == 200 &&
              getResponseBody(response) == "query works";

    printResult("GET query string does not affect filesystem path", ok);
}

/*
 * ------------------------------------------------------------
 * Security / path traversal
 * ------------------------------------------------------------
 */

static void testGetPathTraversal()
{
    std::string secretPath = TEST_ROOT + "/secret.txt";

    writeFile(secretPath, "VERY SECRET");

    ServerConfig config = makeBaseConfig();
    RequestHandler handler(config);

    HttpRequest request;

    std::string raw =
        "GET /../secret.txt HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    bool parsed = request.parse(raw);

    if (!parsed)
    {
        /*
         * If URL parsing rejects the traversal before RequestHandler,
         * that is also an acceptable security result.
         */
        printResult("GET path traversal rejected", true);
        return;
    }

    HttpResponse response;
    handler.handleRequest(request, response);

    bool ok = getStatusCode(response) != 200;

    printResult("GET path traversal rejected", ok);
}

static void testDeletePathTraversal()
{
    std::string secretPath = TEST_ROOT + "/delete-secret.txt";

    writeFile(secretPath, "DELETE ME NEVER");

    ServerConfig config = makeBaseConfig();
    RequestHandler handler(config);

    HttpRequest request;

    std::string raw =
        "DELETE /../delete-secret.txt HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    bool parsed = request.parse(raw);

    if (!parsed)
    {
        printResult("DELETE path traversal rejected", true);
        return;
    }

    HttpResponse response;
    handler.handleRequest(request, response);

    bool ok = getStatusCode(response) != 204 &&
              fileExists(secretPath);

    printResult("DELETE path traversal rejected", ok);
}

/*
 * ------------------------------------------------------------
 * Autoindex escaping
 * ------------------------------------------------------------
 */

static void testAutoIndexHtmlEscaping()
{
    writeFile(WWW_ROOT + "/emptydir/<script>.txt", "bad");
    writeFile(WWW_ROOT + "/emptydir/a&b.txt", "ampersand");
    writeFile(WWW_ROOT + "/emptydir/normal.txt", "normal");

    ServerConfig config = makeBaseConfig();
    config.setAutoIndex(true);

    RequestHandler handler(config);

    HttpResponse response;

    runRequest(handler, "GET", "/emptydir/", response);

    std::string body = getResponseBody(response);

    /*
     * These checks intentionally test safe HTML output.
     * The current RequestHandler implementation is expected
     * to fail this if filenames are inserted without escaping.
     */
    bool hasEscapedScript =
        body.find("&lt;script&gt;.txt") != std::string::npos;

    bool hasEscapedAmpersand =
        body.find("a&amp;b.txt") != std::string::npos;

    bool hasRawScript =
        body.find("<script>.txt") != std::string::npos;

    bool ok = getStatusCode(response) == 200 &&
              hasEscapedScript &&
              hasEscapedAmpersand &&
              !hasRawScript;

    printResult("GET autoindex HTML escaping", ok);
}

/*
 * ------------------------------------------------------------
 * POST / uploads
 * ------------------------------------------------------------
 */

static void testPostUpload()
{
    ServerConfig config = makeBaseConfig();

    Location location = makeUploadLocation();
    config.addLocation(location);

    RequestHandler handler(config);

    const std::string body = "hello upload";

    HttpResponse response;

    runRequest(handler,
               "POST",
               "/upload/test",
               response,
               body);

    bool foundCorrectFile = false;

    DIR *dir = opendir(UPLOAD_ROOT.c_str());

    if (dir != NULL)
    {
        struct dirent *entry;

        while ((entry = readdir(dir)) != NULL)
        {
            std::string name = entry->d_name;

            if (name == "." || name == "..")
                continue;

            std::string path = UPLOAD_ROOT + "/" + name;

            if (fileExists(path) && readFile(path) == body)
            {
                foundCorrectFile = true;
                break;
            }
        }

        closedir(dir);
    }

    bool ok = getStatusCode(response) == 201 &&
              foundCorrectFile;

    printResult("POST upload enabled", ok);
}

static void testPostUploadDisabled()
{
    ServerConfig config = makeBaseConfig();

    Location location("/upload");
    location.setUpload(false);
    location.setUploadStore(UPLOAD_ROOT);

    config.addLocation(location);

    RequestHandler handler(config);

    HttpResponse response;

    runRequest(handler,
               "POST",
               "/upload/test",
               response,
               "should fail");

    bool ok = getStatusCode(response) == 405;

    printResult("POST upload disabled -> 405", ok);
}

static void testPostUploadStoreMissing()
{
    ServerConfig config = makeBaseConfig();

    Location location("/upload");
    location.setUpload(true);
    location.setUploadStore(TEST_ROOT + "/does-not-exist");

    config.addLocation(location);

    RequestHandler handler(config);

    HttpResponse response;

    runRequest(handler,
               "POST",
               "/upload/test",
               response,
               "upload");

    bool ok = getStatusCode(response) == 500;

    printResult("POST upload store missing -> 500", ok);
}

static void testPostEmptyBody()
{
    ServerConfig config = makeBaseConfig();

    Location location = makeUploadLocation();
    config.addLocation(location);

    RequestHandler handler(config);

    HttpResponse response;

    runRequest(handler,
               "POST",
               "/upload/empty",
               response,
               "");

    bool foundEmptyFile = false;

    DIR *dir = opendir(UPLOAD_ROOT.c_str());

    if (dir != NULL)
    {
        struct dirent *entry;

        while ((entry = readdir(dir)) != NULL)
        {
            std::string name = entry->d_name;

            if (name == "." || name == "..")
                continue;

            std::string path = UPLOAD_ROOT + "/" + name;

            if (fileExists(path) && readFile(path).empty())
            {
                foundEmptyFile = true;
                break;
            }
        }

        closedir(dir);
    }

    bool ok = getStatusCode(response) == 201 &&
              foundEmptyFile;

    printResult("POST empty body", ok);
}

static void testPostLargeBody()
{
    std::string body;

    for (int i = 0; i < 100000; ++i)
        body += "0123456789";

    ServerConfig config = makeBaseConfig();

    Location location = makeUploadLocation();
    config.addLocation(location);

    RequestHandler handler(config);

    HttpResponse response;

    runRequest(handler,
               "POST",
               "/upload/large",
               response,
               body);

    bool found = false;

    DIR *dir = opendir(UPLOAD_ROOT.c_str());

    if (dir != NULL)
    {
        struct dirent *entry;

        while ((entry = readdir(dir)) != NULL)
        {
            std::string name = entry->d_name;

            if (name == "." || name == "..")
                continue;

            std::string path = UPLOAD_ROOT + "/" + name;

            if (fileExists(path) && readFile(path) == body)
            {
                found = true;
                break;
            }
        }

        closedir(dir);
    }

    bool ok = getStatusCode(response) == 201 &&
              found;

    printResult("POST large body", ok);
}

/*
 * ------------------------------------------------------------
 * DELETE
 * ------------------------------------------------------------
 */

static void testDeleteExistingFile()
{
    std::string path = WWW_ROOT + "/delete.txt";

    writeFile(path, "delete me");

    ServerConfig config = makeBaseConfig();
    RequestHandler handler(config);

    HttpResponse response;

    runRequest(handler, "DELETE", "/delete.txt", response);

    bool ok = getStatusCode(response) == 204 &&
              getResponseBody(response).empty() &&
              !fileExists(path);

    printResult("DELETE existing file", ok);
}

static void testDeleteMissingFile()
{
    ServerConfig config = makeBaseConfig();
    RequestHandler handler(config);

    HttpResponse response;

    runRequest(handler,
               "DELETE",
               "/does-not-exist.txt",
               response);

    bool ok = getStatusCode(response) == 404;

    printResult("DELETE missing file -> 404", ok);
}

static void testDeleteDirectory()
{
    ServerConfig config = makeBaseConfig();
    RequestHandler handler(config);

    HttpResponse response;

    runRequest(handler, "DELETE", "/emptydir/", response);

    bool ok = getStatusCode(response) == 403 &&
              directoryExists(WWW_ROOT + "/emptydir");

    printResult("DELETE directory -> 403", ok);
}

/*
 * ------------------------------------------------------------
 * Method handling
 * ------------------------------------------------------------
 */

static void testUnsupportedMethods()
{
    ServerConfig config = makeBaseConfig();
    RequestHandler handler(config);

    const char *methods[] =
    {
        "PUT",
        "PATCH",
        "HEAD",
        "OPTIONS",
        "CONNECT",
        "TRACE"
    };

    bool ok = true;

    std::size_t count = sizeof(methods) / sizeof(methods[0]);

    for (std::size_t i = 0; i < count; ++i)
    {
        HttpResponse response;

        runRequest(handler,
                   methods[i],
                   "/hello.txt",
                   response);

        if (getStatusCode(response) != 405)
            ok = false;
    }

    printResult("Unsupported HTTP methods -> 405", ok);
}

static void testLocationAllowedMethods()
{
    writeFile(LOC_ROOT + "/method.txt", "method test");

    ServerConfig config = makeBaseConfig();

    Location location("/static");
    location.setRoot(LOC_ROOT);
    location.addMethod("GET");

    config.addLocation(location);

    RequestHandler handler(config);

    bool ok = true;

    {
        HttpResponse response;

        runRequest(handler,
                   "GET",
                   "/static/method.txt",
                   response);

        if (getStatusCode(response) != 200)
            ok = false;
    }

    {
        HttpResponse response;

        runRequest(handler,
                   "DELETE",
                   "/static/method.txt",
                   response);

        if (getStatusCode(response) != 405)
            ok = false;
    }

    printResult("Location allowed-method restriction", ok);
}

/*
 * ------------------------------------------------------------
 * Location matching
 * ------------------------------------------------------------
 */

static void testLongestLocationMatch()
{
    std::string rootA = TEST_ROOT + "/location_a";
    std::string rootB = TEST_ROOT + "/location_b";

    makeDirectory(rootA);
    makeDirectory(rootB);

    writeFile(rootA + "/file.txt", "SHORT LOCATION");
    writeFile(rootB + "/file.txt", "LONG LOCATION");

    ServerConfig config = makeBaseConfig();

    Location shortLocation("/foo");
    shortLocation.setRoot(rootA);

    Location longLocation("/foo/bar");
    longLocation.setRoot(rootB);

    config.addLocation(shortLocation);
    config.addLocation(longLocation);

    RequestHandler handler(config);

    HttpResponse response;

    runRequest(handler,
               "GET",
               "/foo/bar/file.txt",
               response);

    bool ok = getStatusCode(response) == 200 &&
              getResponseBody(response) == "LONG LOCATION";

    printResult("Longest matching location is selected", ok);
}

/*
 * ------------------------------------------------------------
 * Custom error pages
 * ------------------------------------------------------------
 */

static void testCustom404Page()
{
    writeFile(WWW_ROOT + "/404.html", "CUSTOM 404 PAGE");

    ServerConfig config = makeBaseConfig();
    config.setErrorPage(404, "/404.html");

    RequestHandler handler(config);

    HttpResponse response;

    runRequest(handler,
               "GET",
               "/missing-custom-page.txt",
               response);

    bool ok = getStatusCode(response) == 404 &&
              getResponseBody(response) == "CUSTOM 404 PAGE";

    printResult("Custom 404 error page", ok);
}

/*
 * ------------------------------------------------------------
 * CGI resolution
 * ------------------------------------------------------------
 */

static void testResolveCgi()
{
    std::string script = WWW_ROOT + "/cgi-test.py";

    writeFile(script,
              "#!/usr/bin/env python\n"
              "print('hello')\n");

    ServerConfig config = makeBaseConfig();

    Location location("/cgi");
    location.setRoot(WWW_ROOT);
    location.addCgiExtension(".py", "/usr/bin/python");

    config.addLocation(location);

    RequestHandler handler(config);

    HttpRequest request = makeRequest("GET",
                                      "/cgi/cgi-test.py?foo=bar");

    std::string scriptPath;
    std::string interpreterPath;

    bool result = handler.resolveCGI(request,
                                     config.findLocation("/cgi/cgi-test.py"),
                                     scriptPath,
                                     interpreterPath);

    bool ok = result &&
              scriptPath == script &&
              interpreterPath == "/usr/bin/python";

    printResult("resolveCGI valid script + query string", ok);
}

static void testResolveCgiWrongExtension()
{
    std::string script = WWW_ROOT + "/not-cgi.txt";

    writeFile(script, "not cgi");

    ServerConfig config = makeBaseConfig();

    Location location("/cgi");
    location.setRoot(WWW_ROOT);
    location.addCgiExtension(".py", "/usr/bin/python");

    config.addLocation(location);

    RequestHandler handler(config);

    HttpRequest request = makeRequest("GET",
                                      "/cgi/not-cgi.txt");

    std::string scriptPath;
    std::string interpreterPath;

    bool result = handler.resolveCGI(request,
                                     config.findLocation("/cgi/not-cgi.txt"),
                                     scriptPath,
                                     interpreterPath);

    printResult("resolveCGI rejects wrong extension", !result);
}

static void testResolveCgiMissingScript()
{
    ServerConfig config = makeBaseConfig();

    Location location("/cgi");
    location.setRoot(WWW_ROOT);
    location.addCgiExtension(".py", "/usr/bin/python");

    config.addLocation(location);

    RequestHandler handler(config);

    HttpRequest request = makeRequest("GET",
                                      "/cgi/missing.py");

    std::string scriptPath;
    std::string interpreterPath;

    bool result = handler.resolveCGI(request,
                                     config.findLocation("/cgi/missing.py"),
                                     scriptPath,
                                     interpreterPath);

    printResult("resolveCGI rejects missing script", !result);
}

static void testResolveCgiTraversal()
{
    std::string secret = TEST_ROOT + "/secret.py";

    writeFile(secret, "SECRET");

    ServerConfig config = makeBaseConfig();

    Location location("/cgi");
    location.setRoot(WWW_ROOT);
    location.addCgiExtension(".py", "/usr/bin/python");

    config.addLocation(location);

    RequestHandler handler(config);

    HttpRequest request;

    std::string raw =
        "GET /cgi/../secret.py HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    bool parsed = request.parse(raw);

    if (!parsed)
    {
        printResult("resolveCGI traversal rejected", true);
        return;
    }

    std::string scriptPath;
    std::string interpreterPath;

    bool result = handler.resolveCGI(request,
                                     config.findLocation("/cgi/../secret.py"),
                                     scriptPath,
                                     interpreterPath);

    printResult("resolveCGI traversal rejected", !result);
}

static void testAutoIndexEmptyUrl()
{
    makeDirectory(WWW_ROOT + "/empty_url");

    ServerConfig config = makeBaseConfig();
    config.setAutoIndex(true);

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler, "GET", "/empty_url", response);

    bool ok = getStatusCode(response) == 200;
    printResult("GET autoindex without trailing slash", ok);
}

static void testAutoIndexQuoteEscaping()
{
    std::string dir = WWW_ROOT + "/quotes";
    makeDirectory(dir);

    writeFile(dir + "/quote\"file.txt", "quote");
    writeFile(dir + "/apostrophe'file.txt", "apostrophe");

    ServerConfig config = makeBaseConfig();
    config.setAutoIndex(true);

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler, "GET", "/quotes/", response);

    std::string body = getResponseBody(response);

    bool hasEscapedQuote =
        body.find("quote&quot;file.txt") != std::string::npos;

    bool hasEscapedApostrophe =
        body.find("apostrophe&#39;file.txt") != std::string::npos;

    bool hasRawQuote =
        body.find("quote\"file.txt") != std::string::npos;

    bool hasRawApostrophe =
        body.find("apostrophe'file.txt") != std::string::npos;

    bool ok = getStatusCode(response) == 200 &&
              hasEscapedQuote &&
              hasEscapedApostrophe &&
              !hasRawQuote &&
              !hasRawApostrophe;

    printResult("GET autoindex quote/apostrophe escaping", ok);
}

static void testUppercaseMimeTypes()
{
    writeFile(WWW_ROOT + "/UPPER.HTML", "<html></html>");
    writeFile(WWW_ROOT + "/UPPER.CSS", "body {}");
    writeFile(WWW_ROOT + "/UPPER.JSON", "{}");
    writeFile(WWW_ROOT + "/UPPER.PNG", "png");

    ServerConfig config = makeBaseConfig();
    RequestHandler handler(config);

    bool ok = true;

    {
        HttpResponse response;
        runRequest(handler, "GET", "/UPPER.HTML", response);

        if (!responseHasHeader(response, "Content-Type", "text/html"))
            ok = false;
    }

    {
        HttpResponse response;
        runRequest(handler, "GET", "/UPPER.CSS", response);

        if (!responseHasHeader(response, "Content-Type", "text/css"))
            ok = false;
    }

    {
        HttpResponse response;
        runRequest(handler, "GET", "/UPPER.JSON", response);

        if (!responseHasHeader(response,
                               "Content-Type",
                               "application/json"))
            ok = false;
    }

    {
        HttpResponse response;
        runRequest(handler, "GET", "/UPPER.PNG", response);

        if (!responseHasHeader(response, "Content-Type", "image/png"))
            ok = false;
    }

    printResult("GET uppercase MIME extensions", ok);
}

static void testLocationPathBoundary()
{
    writeFile(WWW_ROOT + "/static2.txt", "server-root");

    Location location("/static");
    location.setRoot(LOC_ROOT);

    ServerConfig config = makeBaseConfig();
    config.addLocation(location);

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler, "GET", "/static2.txt", response);

    bool ok = getStatusCode(response) == 200 &&
              getResponseBody(response) == "server-root";

    printResult("Location /static does not match /static2.txt", ok);
}

static void testDeleteLocationRoot()
{
    std::string path = LOC_ROOT + "/delete-location.txt";
    writeFile(path, "delete me");

    Location location("/files");
    location.setRoot(LOC_ROOT);

    ServerConfig config = makeBaseConfig();
    config.addLocation(location);

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "DELETE",
               "/files/delete-location.txt",
               response);

    bool ok = getStatusCode(response) == 204 &&
              !fileExists(path);

    printResult("DELETE uses location-specific root", ok);
}

static void testPostBinaryBody()
{
    ServerConfig config = makeBaseConfig();

    Location location("/binary-upload");
    location.setUpload(true);
    location.setUploadStore(UPLOAD_ROOT);
    config.addLocation(location);

    RequestHandler handler(config);

    std::string body;
    body.push_back('A');
    body.push_back('\0');
    body.push_back('B');
    body.push_back('\1');
    body.push_back(static_cast<char>(0xff));
    body.push_back('C');
    body.push_back('D');
    body.push_back('E');

    HttpResponse response;
	HttpRequest debugRequest;

	std::string rawRequest;
	rawRequest += "POST /binary-upload HTTP/1.1\r\n";
	rawRequest += "Host: localhost\r\n";
	rawRequest += "Content-Length: ";
	std::stringstream lengthStream;
	lengthStream << body.size();
	rawRequest += lengthStream.str();
	rawRequest += "\r\n";
	rawRequest += "\r\n";
	rawRequest += body;

	debugRequest.parse(rawRequest);

    runRequest(handler,
               "POST",
               "/binary-upload",
               response,
               body);

    bool foundCorrectFile = false;

    DIR *dir = opendir(UPLOAD_ROOT.c_str());
    if (dir != NULL)
    {
        struct dirent *entry;

        while ((entry = readdir(dir)) != NULL)
        {
            std::string name = entry->d_name;

            if (name == "." || name == "..")
                continue;

            std::string path = UPLOAD_ROOT + "/" + name;

            if (fileExists(path) && readFile(path) == body)
            {
                foundCorrectFile = true;
                break;
            }
        }

        closedir(dir);
    }

    bool ok = getStatusCode(response) == 201 &&
              foundCorrectFile;

    printResult("POST preserves binary body", ok);
}

static void testHttpRequestPreservesBinaryBody()
{
    std::string body;
    body.push_back('A');
    body.push_back('\0');
    body.push_back('B');
    body.push_back('\1');
    body.push_back(static_cast<char>(0xff));
    body.push_back('C');
    body.push_back('D');
    body.push_back('E');

    std::string rawRequest;
    rawRequest += "POST /binary-upload HTTP/1.1\r\n";
    rawRequest += "Host: localhost\r\n";
    rawRequest += "Content-Length: 8\r\n";
    rawRequest += "\r\n";
    rawRequest += body;

    HttpRequest request;

    bool parsed = request.parse(rawRequest);

    bool ok = parsed &&
              request.getBody().size() == body.size() &&
              request.getBody() == body;

    printResult("HttpRequest preserves binary body", ok);
}

static void testMultiplePostUploads()
{
    ServerConfig config = makeBaseConfig();

    Location location("/upload-multiple");
    location.setUpload(true);
    location.setUploadStore(UPLOAD_ROOT);
    config.addLocation(location);

    RequestHandler handler(config);

    HttpResponse response1;
    runRequest(handler,
               "POST",
               "/upload-multiple",
               response1,
               "first");

    HttpResponse response2;
    runRequest(handler,
               "POST",
               "/upload-multiple",
               response2,
               "second");

    bool foundFirst = false;
    bool foundSecond = false;

    DIR *dir = opendir(UPLOAD_ROOT.c_str());
    if (dir != NULL)
    {
        struct dirent *entry;

        while ((entry = readdir(dir)) != NULL)
        {
            std::string name = entry->d_name;

            if (name == "." || name == "..")
                continue;

            std::string path = UPLOAD_ROOT + "/" + name;

            if (!fileExists(path))
                continue;

            std::string content = readFile(path);

            if (content == "first")
                foundFirst = true;

            if (content == "second")
                foundSecond = true;
        }

        closedir(dir);
    }

    bool ok = getStatusCode(response1) == 201 &&
              getStatusCode(response2) == 201 &&
              foundFirst &&
              foundSecond;

    printResult("Multiple POST requests create distinct files", ok);
}

static void testCustom403ErrorPage()
{
    ServerConfig config = makeBaseConfig();

    writeFile(WWW_ROOT + "/403.html",
              "<html><body>custom forbidden</body></html>");

    config.setErrorPage(403, "/403.html");

    makeDirectory(WWW_ROOT + "/protected");

    RequestHandler handler(config);
    HttpResponse response;

    // DELETE on a directory is still refused with 403 (a GET on a directory without index is now a 404)
    runRequest(handler,
               "DELETE",
               "/protected",
               response);

    bool ok = getStatusCode(response) == 403 &&
              getResponseBody(response).find("custom forbidden") !=
                  std::string::npos;

    printResult("Custom 403 error page", ok);
}

static void testCustom405ErrorPage()
{
    ServerConfig config = makeBaseConfig();

    writeFile(WWW_ROOT + "/405.html",
              "<html><body>custom method not allowed</body></html>");

    config.setErrorPage(405, "/405.html");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "PUT",
               "/index.html",
               response);

    bool ok = getStatusCode(response) == 405 &&
              getResponseBody(response).find(
                  "custom method not allowed") != std::string::npos;

    printResult("Custom 405 error page", ok);
}

static void testLocationDisablesServerAutoIndex()
{
    ServerConfig config = makeBaseConfig();
    config.setAutoIndex(true);

    Location location("/no-list");
    location.setRoot(LOC_ROOT);
    location.setAutoIndex(false);

    config.addLocation(location);

    makeDirectory(LOC_ROOT + "/files");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/no-list/files",
               response);

    bool ok = getStatusCode(response) == 404; // No listing and no index: nothing to show

    printResult("Location autoindex disables server autoindex", ok);
}

static void testLocationEnablesServerAutoIndex()
{
    ServerConfig config = makeBaseConfig();
    config.setAutoIndex(false);

    Location location("/list");
    location.setRoot(LOC_ROOT);
    location.setAutoIndex(true);

    config.addLocation(location);

    makeDirectory(LOC_ROOT + "/files");
    writeFile(LOC_ROOT + "/files/test.txt", "hello");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/list/files",
               response);

    bool ok = getStatusCode(response) == 200 &&
              getResponseBody(response).find("test.txt") !=
                  std::string::npos;

    printResult("Location autoindex enables directory listing", ok);
}

static void testLocationIndexOverridesServerIndex()
{
    ServerConfig config = makeBaseConfig();
    config.setIndex("index.html");

    writeFile(WWW_ROOT + "/index.html", "server index");

    Location location("/custom-index");
    location.setRoot(LOC_ROOT);
    location.setIndex("home.html");

    config.addLocation(location);

    makeDirectory(LOC_ROOT + "/page");
    writeFile(LOC_ROOT + "/page/home.html", "location index");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/custom-index/page/",
               response);

    bool ok = getStatusCode(response) == 200 &&
              getResponseBody(response) == "location index";

    printResult("Location index overrides server index", ok);
}

static void testPostMethodRestriction()
{
    ServerConfig config = makeBaseConfig();

    Location location("/restricted-upload");
    location.setUpload(true);
    location.setUploadStore(UPLOAD_ROOT);
    location.addMethod("GET");

    config.addLocation(location);

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "POST",
               "/restricted-upload",
               response,
               "hello");

    bool ok = getStatusCode(response) == 405;

    printResult("POST respects location method restriction", ok);
}

static void testDeleteMethodRestriction()
{
    ServerConfig config = makeBaseConfig();

    std::string path = WWW_ROOT + "/protected-delete.txt";
    writeFile(path, "delete me");

    Location location("/protected-delete");
    location.setRoot(WWW_ROOT);
    location.addMethod("GET");

    config.addLocation(location);

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "DELETE",
               "/protected-delete/protected-delete.txt",
               response);

    bool ok = getStatusCode(response) == 405 &&
              fileExists(path);

    printResult("DELETE respects location method restriction", ok);
}

static void testDeleteNestedLocationFile()
{
    ServerConfig config = makeBaseConfig();

    Location location("/static");
    location.setRoot(LOC_ROOT);
    location.addMethod("DELETE");

    config.addLocation(location);

    makeDirectory(LOC_ROOT + "/nested");
    writeFile(LOC_ROOT + "/nested/delete.txt", "remove me");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "DELETE",
               "/static/nested/delete.txt",
               response);

    bool ok = getStatusCode(response) == 204 &&
              !fileExists(LOC_ROOT + "/nested/delete.txt");

    printResult("DELETE nested file through location root", ok);
}

static void testDirectoryMissingIndex()
{
    ServerConfig config = makeBaseConfig();
    config.setIndex("does-not-exist.html");
    config.setAutoIndex(false);

    makeDirectory(WWW_ROOT + "/missing-index");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/missing-index/",
               response);

    bool ok = getStatusCode(response) == 404;

    printResult("GET directory with missing index -> 404", ok);
}

static void testEmptyDirectoryIndex()
{
    ServerConfig config = makeBaseConfig();

    makeDirectory(WWW_ROOT + "/empty-index");
    writeFile(WWW_ROOT + "/empty-index/index.html", "");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/empty-index/",
               response);

    bool ok = getStatusCode(response) == 200 &&
              getResponseBody(response).empty();

    printResult("GET directory with empty index", ok);
}

static void testHeadMethod()
{
    ServerConfig config = makeBaseConfig();

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "HEAD",
               "/index.html",
               response);

    bool ok = getStatusCode(response) == 405;

    printResult("HEAD currently returns 405", ok);
}

static void testOptionsMethod()
{
    ServerConfig config = makeBaseConfig();

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "OPTIONS",
               "/index.html",
               response);

    bool ok = getStatusCode(response) == 405;

    printResult("OPTIONS currently returns 405", ok);
}

static void testAutoIndexMultipleEntries()
{
    ServerConfig config = makeBaseConfig();
    config.setAutoIndex(true);

    makeDirectory(WWW_ROOT + "/listing");
    writeFile(WWW_ROOT + "/listing/a.txt", "a");
    writeFile(WWW_ROOT + "/listing/b.txt", "b");
    writeFile(WWW_ROOT + "/listing/c.html", "c");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/listing/",
               response);

    std::string body = getResponseBody(response);

    bool ok = getStatusCode(response) == 200 &&
              body.find("a.txt") != std::string::npos &&
              body.find("b.txt") != std::string::npos &&
              body.find("c.html") != std::string::npos;

    printResult("Autoindex contains multiple entries", ok);
}

static void testAutoIndexSkipsDotEntries()
{
    ServerConfig config = makeBaseConfig();
    config.setAutoIndex(true);

    makeDirectory(WWW_ROOT + "/dot-list");
    writeFile(WWW_ROOT + "/dot-list/file.txt", "hello");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/dot-list/",
               response);

    std::string body = getResponseBody(response);

    bool ok = getStatusCode(response) == 200 &&
              body.find(">.</a>") == std::string::npos &&
              body.find(">..</a>") == std::string::npos;

    printResult("Autoindex skips dot entries", ok);
}

static void testPostBodyAtExactLimit()
{
    ServerConfig config = makeBaseConfig();
    config.setClientMaxBodySize(8);

    Location location("/limit");
    location.setUpload(true);
    location.setUploadStore(UPLOAD_ROOT);
    config.addLocation(location);

    RequestHandler handler(config);
    HttpResponse response;

    std::string body = "12345678";

    runRequest(handler,
               "POST",
               "/limit",
               response,
               body);

    bool ok = getStatusCode(response) == 201;

    printResult("POST body exactly at client max size", ok);
}

static void testPostBodyOverLimit()
{
    ServerConfig config = makeBaseConfig();
    config.setClientMaxBodySize(8);

    Location location("/limit-over");
    location.setUpload(true);
    location.setUploadStore(UPLOAD_ROOT);
    config.addLocation(location);

    RequestHandler handler(config);
    HttpResponse response;

    std::string body = "123456789";

    runRequest(handler,
               "POST",
               "/limit-over",
               response,
               body);

    bool ok = getStatusCode(response) == 413;

    printResult("POST body over client max size -> 413", ok);
}

static void testPostHugeBodyOverLimit()
{
    ServerConfig config = makeBaseConfig();
    config.setClientMaxBodySize(16);

    Location location("/huge");
    location.setUpload(true);
    location.setUploadStore(UPLOAD_ROOT);
    config.addLocation(location);

    RequestHandler handler(config);
    HttpResponse response;

    std::string body(1024, 'X');

    runRequest(handler,
               "POST",
               "/huge",
               response,
               body);

    bool ok = getStatusCode(response) == 413;

    printResult("POST huge body over limit -> 413", ok);
}

static void testPostResponse()
{
    ServerConfig config = makeBaseConfig();

    Location location("/post-response");
    location.setUpload(true);
    location.setUploadStore(UPLOAD_ROOT);
    config.addLocation(location);

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "POST",
               "/post-response",
               response,
               "hello");

    bool ok = getStatusCode(response) == 201 &&
              getResponseBody(response).empty();

    printResult("POST returns 201 with empty response body", ok);
}

static void testDeleteResponseBodyEmpty()
{
    ServerConfig config = makeBaseConfig();

    std::string path = WWW_ROOT + "/delete-empty-body.txt";
    writeFile(path, "delete me");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "DELETE",
               "/delete-empty-body.txt",
               response);

    bool ok = getStatusCode(response) == 204 &&
              getResponseBody(response).empty();

    printResult("DELETE 204 has empty response body", ok);
}

static void testDeleteMissingResponseBody()
{
    ServerConfig config = makeBaseConfig();

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "DELETE",
               "/does-not-exist.txt",
               response);

    bool ok = getStatusCode(response) == 404 &&
              !getResponseBody(response).empty();

    printResult("DELETE missing file returns error body", ok);
}

static void testGetBinaryExactBytes()
{
    ServerConfig config = makeBaseConfig();

    std::string body;
    body.push_back('A');
    body.push_back('\0');
    body.push_back('B');
    body.push_back('\1');
    body.push_back(static_cast<char>(0xff));
    body.push_back('\0');
    body.push_back('C');

    writeFile(WWW_ROOT + "/exact.bin", body);

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/exact.bin",
               response);

    bool ok = getStatusCode(response) == 200 &&
              getResponseBody(response) == body;

    printResult("GET preserves binary bytes exactly", ok);
}

static void testMixedCaseHtmlMime()
{
    ServerConfig config = makeBaseConfig();

    writeFile(WWW_ROOT + "/page.HtMl", "<html></html>");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/page.HtMl",
               response);

    bool ok = getStatusCode(response) == 200 &&
              responseHasHeader(response,
                                "Content-Type",
                                "text/html");

    printResult("GET mixed-case HTML MIME type", ok);
}

static void testMixedCaseCssMime()
{
    ServerConfig config = makeBaseConfig();

    writeFile(WWW_ROOT + "/style.CsS", "body {}");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/style.CsS",
               response);

    bool ok = getStatusCode(response) == 200 &&
              responseHasHeader(response,
                                "Content-Type",
                                "text/css");

    printResult("GET mixed-case CSS MIME type", ok);
}

static void testMixedCaseJsonMime()
{
    ServerConfig config = makeBaseConfig();

    writeFile(WWW_ROOT + "/data.JsOn", "{}");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/data.JsOn",
               response);

    bool ok = getStatusCode(response) == 200 &&
              responseHasHeader(response,
                                "Content-Type",
                                "application/json");

    printResult("GET mixed-case JSON MIME type", ok);
}

static void testUnknownExtensionMime()
{
    ServerConfig config = makeBaseConfig();

    writeFile(WWW_ROOT + "/file.xyz", "hello");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/file.xyz",
               response);

    bool ok = getStatusCode(response) == 200 &&
              responseHasHeader(response,
                                "Content-Type",
                                "application/octet-stream");

    printResult("GET unknown extension uses octet-stream", ok);
}

static void testLocationQueryString()
{
    ServerConfig config = makeBaseConfig();

    Location location("/static");
    location.setRoot(LOC_ROOT);
    config.addLocation(location);

    writeFile(LOC_ROOT + "/query.html", "location query works");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/static/query.html?foo=bar&x=123",
               response);

    bool ok = getStatusCode(response) == 200 &&
              getResponseBody(response) == "location query works";

    printResult("Location GET ignores query string for filesystem path", ok);
}

static void testLocationRootTrailingSlash()
{
    ServerConfig config = makeBaseConfig();

    Location location("/trailing");
    location.setRoot(LOC_ROOT + "/");
    config.addLocation(location);

    writeFile(LOC_ROOT + "/hello.txt", "trailing root");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/trailing/hello.txt",
               response);

    bool ok = getStatusCode(response) == 200 &&
              getResponseBody(response) == "trailing root";

    printResult("Location root with trailing slash", ok);
}

static void testServerRootTrailingSlash()
{
    ServerConfig config = makeBaseConfig();
    config.setRoot(WWW_ROOT + "/");

    writeFile(WWW_ROOT + "/trailing-root.txt", "server trailing root");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/trailing-root.txt",
               response);

    bool ok = getStatusCode(response) == 200 &&
              getResponseBody(response) == "server trailing root";

    printResult("Server root with trailing slash", ok);
}

static void testRootLocation()
{
    ServerConfig config = makeBaseConfig();

    Location location("/");
    location.setRoot(LOC_ROOT);
    config.addLocation(location);

    writeFile(LOC_ROOT + "/root-location.txt", "root location");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/root-location.txt",
               response);

    bool ok = getStatusCode(response) == 200 &&
              getResponseBody(response) == "root location";

    printResult("Root location / is handled correctly", ok);
}

static void testCgiUppercaseExtension()
{
    ServerConfig config = makeBaseConfig();

    Location location("/cgi");
    location.setRoot(WWW_ROOT);
    location.addCgiExtension(".PY", "/usr/bin/python3");
    config.addLocation(location);

    writeFile(WWW_ROOT + "/test.PY", "print('hello')");

    RequestHandler handler(config);

    HttpRequest request = makeRequest("GET", "/cgi/test.PY");

    const Location *found = config.findLocation("/cgi");

    std::string scriptPath;
    std::string interpreterPath;

    bool resolved = handler.resolveCGI(request,
                                       found,
                                       scriptPath,
                                       interpreterPath);

    bool ok = resolved &&
              scriptPath == WWW_ROOT + "/test.PY" &&
              interpreterPath == "/usr/bin/python3";

    printResult("resolveCGI uppercase extension", ok);
}

static void testCgiQueryString()
{
    ServerConfig config = makeBaseConfig();

    Location location("/cgi-query");
    location.setRoot(WWW_ROOT);
    location.addCgiExtension(".py", "/usr/bin/python3");
    config.addLocation(location);

    writeFile(WWW_ROOT + "/hello.py", "print('hello')");

    RequestHandler handler(config);

    HttpRequest request =
        makeRequest("GET", "/cgi-query/hello.py?name=test");

    const Location *found = config.findLocation("/cgi-query");

    std::string scriptPath;
    std::string interpreterPath;

    bool resolved = handler.resolveCGI(request,
                                       found,
                                       scriptPath,
                                       interpreterPath);

    bool ok = resolved &&
              scriptPath == WWW_ROOT + "/hello.py" &&
              interpreterPath == "/usr/bin/python3";

    printResult("resolveCGI ignores query string", ok);
}

static void testCgiWithoutConfiguredLocation()
{
    ServerConfig config = makeBaseConfig();

    writeFile(WWW_ROOT + "/test.py", "print('hello')");

    RequestHandler handler(config);

    HttpRequest request = makeRequest("GET", "/test.py");

    std::string scriptPath;
    std::string interpreterPath;

    bool resolved = handler.resolveCGI(request,
                                       NULL,
                                       scriptPath,
                                       interpreterPath);

    bool ok = !resolved;

    printResult("resolveCGI rejects missing location", ok);
}

static void testNestedUploadStore()
{
    ServerConfig config = makeBaseConfig();

    std::string nestedUpload = UPLOAD_ROOT + "/nested";
    makeDirectory(nestedUpload);

    Location location("/nested-upload");
    location.setUpload(true);
    location.setUploadStore(nestedUpload);
    config.addLocation(location);

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "POST",
               "/nested-upload",
               response,
               "nested upload");

    bool found = false;

    DIR *dir = opendir(nestedUpload.c_str());
    if (dir != NULL)
    {
        struct dirent *entry;

        while ((entry = readdir(dir)) != NULL)
        {
            std::string name = entry->d_name;

            if (name == "." || name == "..")
                continue;

            std::string path = nestedUpload + "/" + name;

            if (fileExists(path) &&
                readFile(path) == "nested upload")
            {
                found = true;
                break;
            }
        }

        closedir(dir);
    }

    bool ok = getStatusCode(response) == 201 && found;

    printResult("POST supports nested upload store", ok);
}

static void testUploadLocationGet()
{
    ServerConfig config = makeBaseConfig();

    Location location("/upload-get");
    location.setRoot(LOC_ROOT);
    location.setUpload(true);
    location.setUploadStore(UPLOAD_ROOT);
    location.addMethod("GET");
    location.addMethod("POST");

    config.addLocation(location);

    writeFile(LOC_ROOT + "/file.txt", "GET works");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/upload-get/file.txt",
               response);

    bool ok = getStatusCode(response) == 200 &&
              getResponseBody(response) == "GET works";

    printResult("Upload location still supports GET", ok);
}

static void testGetRootDirectory()
{
    ServerConfig config = makeBaseConfig();

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/",
               response);

    bool ok = getStatusCode(response) == 403 ||
              getStatusCode(response) == 200;

    printResult("GET root directory", ok);
}

static void testGetDoubleSlash()
{
    ServerConfig config = makeBaseConfig();

    writeFile(WWW_ROOT + "/double-slash.txt", "double slash");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "//double-slash.txt",
               response);

    bool ok = getStatusCode(response) == 200 &&
              getResponseBody(response) == "double slash";

    printResult("GET double slash path", ok);
}

static void testGetDotPath()
{
    ServerConfig config = makeBaseConfig();

    writeFile(WWW_ROOT + "/dot.txt", "dot path");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/./dot.txt",
               response);

    bool ok = getStatusCode(response) == 200 &&
              getResponseBody(response) == "dot path";

    printResult("GET dot path", ok);
}

static void testGetDirectoryDot()
{
    ServerConfig config = makeBaseConfig();

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/dir/.",
               response);

    bool ok = getStatusCode(response) == 403 ||
              getStatusCode(response) == 200;

    printResult("GET directory dot path", ok);
}

static void testGetDirectoryDotDot()
{
    ServerConfig config = makeBaseConfig();

    writeFile(WWW_ROOT + "/parent-target.txt", "parent target");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/dir/..",
               response);

    bool ok = getStatusCode(response) != 200 ||
              getResponseBody(response) != "parent target";

    printResult("GET directory dot-dot does not escape root", ok);
}

static void testGetNestedTraversalRejected()
{
    ServerConfig config = makeBaseConfig();

    writeFile(TEST_ROOT + "/secret.txt", "secret");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/foo/../secret.txt",
               response);

    bool ok = getStatusCode(response) != 200 ||
              getResponseBody(response) != "secret";

    printResult("GET nested traversal cannot escape root", ok);
}

static void testDeleteRootDirectory()
{
    ServerConfig config = makeBaseConfig();

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "DELETE",
               "/",
               response);

    bool ok = getStatusCode(response) == 403;

    printResult("DELETE root directory -> 403", ok);
}

static void testDeleteDirectoryWithSlash()
{
    ServerConfig config = makeBaseConfig();

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "DELETE",
               "/dir/",
               response);

    bool ok = getStatusCode(response) == 403;

    printResult("DELETE directory with slash -> 403", ok);
}

static void testOversizedPostDoesNotCreateFile()
{
    ServerConfig config = makeBaseConfig();
    config.setClientMaxBodySize(8);

    Location location("/safe-upload");
    location.setUpload(true);
    location.setUploadStore(UPLOAD_ROOT);
    config.addLocation(location);

    RequestHandler handler(config);
    HttpResponse response;

    int beforeCount = 0;

    DIR *beforeDir = opendir(UPLOAD_ROOT.c_str());
    if (beforeDir != NULL)
    {
        struct dirent *entry;

        while ((entry = readdir(beforeDir)) != NULL)
        {
            std::string name = entry->d_name;

            if (name == "." || name == "..")
                continue;

            std::string path = UPLOAD_ROOT + "/" + name;

            if (fileExists(path))
                ++beforeCount;
        }

        closedir(beforeDir);
    }

    runRequest(handler,
               "POST",
               "/safe-upload",
               response,
               "123456789");

    int afterCount = 0;

    DIR *afterDir = opendir(UPLOAD_ROOT.c_str());
    if (afterDir != NULL)
    {
        struct dirent *entry;

        while ((entry = readdir(afterDir)) != NULL)
        {
            std::string name = entry->d_name;

            if (name == "." || name == "..")
                continue;

            std::string path = UPLOAD_ROOT + "/" + name;

            if (fileExists(path))
                ++afterCount;
        }

        closedir(afterDir);
    }

    bool ok = getStatusCode(response) == 413 &&
              beforeCount == afterCount;

    printResult("Oversized POST does not create upload file", ok);
}

static void testPostBinaryAtExactLimit()
{
    ServerConfig config = makeBaseConfig();
    config.setClientMaxBodySize(5);

    Location location("/binary-limit");
    location.setUpload(true);
    location.setUploadStore(UPLOAD_ROOT);
    config.addLocation(location);

    std::string body;
    body.push_back('A');
    body.push_back('\0');
    body.push_back('B');
    body.push_back('\1');
    body.push_back(static_cast<char>(0xff));

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "POST",
               "/binary-limit",
               response,
               body);

    bool found = false;

    DIR *dir = opendir(UPLOAD_ROOT.c_str());
    if (dir != NULL)
    {
        struct dirent *entry;

        while ((entry = readdir(dir)) != NULL)
        {
            std::string name = entry->d_name;

            if (name == "." || name == "..")
                continue;

            std::string path = UPLOAD_ROOT + "/" + name;

            if (fileExists(path) &&
                readFile(path) == body)
            {
                found = true;
                break;
            }
        }

        closedir(dir);
    }

    bool ok = getStatusCode(response) == 201 && found;

    printResult("POST binary body exactly at limit", ok);
}

static void testPostBinaryOverLimit()
{
    ServerConfig config = makeBaseConfig();
    config.setClientMaxBodySize(5);

    Location location("/binary-over-limit");
    location.setUpload(true);
    location.setUploadStore(UPLOAD_ROOT);
    config.addLocation(location);

    std::string body;
    body.push_back('A');
    body.push_back('\0');
    body.push_back('B');
    body.push_back('\1');
    body.push_back(static_cast<char>(0xff));
    body.push_back('C');

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "POST",
               "/binary-over-limit",
               response,
               body);

    bool ok = getStatusCode(response) == 413;

    printResult("POST binary body over limit -> 413", ok);
}

static void testGetFileWithQueryString()
{
    ServerConfig config = makeBaseConfig();

    writeFile(WWW_ROOT + "/query-file.txt", "query file");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/query-file.txt?foo=bar",
               response);

    bool ok = getStatusCode(response) == 200 &&
              getResponseBody(response) == "query file";

    printResult("GET file with query string", ok);
}

static void testGetDirectoryWithQueryString()
{
    ServerConfig config = makeBaseConfig();

    writeFile(WWW_ROOT + "/dir/index.html", "directory query");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/dir/?foo=bar",
               response);

    bool ok = getStatusCode(response) == 200 &&
              getResponseBody(response) == "directory query";

    printResult("GET directory with query string", ok);
}

static void testGetEmptyQueryString()
{
    ServerConfig config = makeBaseConfig();

    writeFile(WWW_ROOT + "/empty-query.txt", "empty query");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/empty-query.txt?",
               response);

    bool ok = getStatusCode(response) == 200 &&
              getResponseBody(response) == "empty query";

    printResult("GET file with empty query string", ok);
}

static void testGetFragmentLikeTarget()
{
    ServerConfig config = makeBaseConfig();

    writeFile(WWW_ROOT + "/fragment.txt", "fragment");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/fragment.txt#section",
               response);

    bool ok = getStatusCode(response) != 200 ||
              getResponseBody(response) != "fragment";

    printResult("GET fragment-like target is not treated as query", ok);
}

static void testPatchMethod()
{
    ServerConfig config = makeBaseConfig();

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "PATCH",
               "/",
               response);

    bool ok = getStatusCode(response) == 405;

    printResult("PATCH returns 405", ok);
}

static void testConnectMethod()
{
    ServerConfig config = makeBaseConfig();

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "CONNECT",
               "/",
               response);

    bool ok = getStatusCode(response) == 405;

    printResult("CONNECT returns 405", ok);
}

static void testTraceMethod()
{
    ServerConfig config = makeBaseConfig();

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "TRACE",
               "/",
               response);

    bool ok = getStatusCode(response) == 405;

    printResult("TRACE returns 405", ok);
}

static void testDeleteActuallyRemovesFile()
{
    ServerConfig config = makeBaseConfig();

    std::string path = WWW_ROOT + "/really-delete.txt";
    writeFile(path, "delete me");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "DELETE",
               "/really-delete.txt",
               response);

    bool ok = getStatusCode(response) == 204 &&
              !fileExists(path);

    printResult("DELETE actually removes file", ok);
}

static void testDeleteThenGetReturns404()
{
    ServerConfig config = makeBaseConfig();

    std::string path = WWW_ROOT + "/delete-then-get.txt";
    writeFile(path, "gone");

    RequestHandler handler(config);

    HttpResponse deleteResponse;

    runRequest(handler,
               "DELETE",
               "/delete-then-get.txt",
               deleteResponse);

    HttpResponse getResponse;

    runRequest(handler,
               "GET",
               "/delete-then-get.txt",
               getResponse);

    bool ok = getStatusCode(deleteResponse) == 204 &&
              getStatusCode(getResponse) == 404;

    printResult("GET after DELETE returns 404", ok);
}

static void testDeleteTwice()
{
    ServerConfig config = makeBaseConfig();

    std::string path = WWW_ROOT + "/delete-twice.txt";
    writeFile(path, "gone");

    RequestHandler handler(config);

    HttpResponse firstResponse;

    runRequest(handler,
               "DELETE",
               "/delete-twice.txt",
               firstResponse);

    HttpResponse secondResponse;

    runRequest(handler,
               "DELETE",
               "/delete-twice.txt",
               secondResponse);

    bool ok = getStatusCode(firstResponse) == 204 &&
              getStatusCode(secondResponse) == 404;

    printResult("DELETE same file twice -> 404", ok);
}

static void testPostEmptyBodyCreatesEmptyFile()
{
    ServerConfig config = makeBaseConfig();

    Location location("/empty-upload");
    location.setUpload(true);
    location.setUploadStore(UPLOAD_ROOT);
    config.addLocation(location);

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "POST",
               "/empty-upload",
               response,
               "");

    bool foundEmptyFile = false;

    DIR *dir = opendir(UPLOAD_ROOT.c_str());

    if (dir != NULL)
    {
        struct dirent *entry;

        while ((entry = readdir(dir)) != NULL)
        {
            std::string name = entry->d_name;

            if (name == "." || name == "..")
                continue;

            std::string path = UPLOAD_ROOT + "/" + name;

            if (fileExists(path) &&
                readFile(path).empty())
            {
                foundEmptyFile = true;
                break;
            }
        }

        closedir(dir);
    }

    bool ok = getStatusCode(response) == 201 &&
              foundEmptyFile;

    printResult("POST empty body creates empty file", ok);
}

static void testPostUploadStoreIsFile()
{
    ServerConfig config = makeBaseConfig();

    std::string fakeStore = TEST_ROOT + "/upload-store-file";
    writeFile(fakeStore, "not a directory");

    Location location("/bad-store");
    location.setUpload(true);
    location.setUploadStore(fakeStore);
    config.addLocation(location);

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "POST",
               "/bad-store",
               response,
               "hello");

    bool ok = getStatusCode(response) == 500;

    printResult("POST upload store that is a file -> 500", ok);
}

static void testCgiLastExtension()
{
    ServerConfig config = makeBaseConfig();

    Location location("/cgi-last");
    location.setRoot(WWW_ROOT);
    location.addCgiExtension(".py", "/usr/bin/python3");
    config.addLocation(location);

    writeFile(WWW_ROOT + "/foo.bar.py", "print('hello')");

    RequestHandler handler(config);

    HttpRequest request =
        makeRequest("GET", "/cgi-last/foo.bar.py");

    const Location *found =
        config.findLocation("/cgi-last");

    std::string scriptPath;
    std::string interpreterPath;

    bool resolved =
        handler.resolveCGI(request,
                            found,
                            scriptPath,
                            interpreterPath);

    bool ok = resolved &&
              scriptPath == WWW_ROOT + "/foo.bar.py" &&
              interpreterPath == "/usr/bin/python3";

    printResult("resolveCGI uses last extension", ok);
}

static void testCgiWrongFinalExtension()
{
    ServerConfig config = makeBaseConfig();

    Location location("/cgi-final");
    location.setRoot(WWW_ROOT);
    location.addCgiExtension(".py", "/usr/bin/python3");
    config.addLocation(location);

    writeFile(WWW_ROOT + "/foo.py.txt", "not CGI");

    RequestHandler handler(config);

    HttpRequest request =
        makeRequest("GET", "/cgi-final/foo.py.txt");

    const Location *found =
        config.findLocation("/cgi-final");

    std::string scriptPath;
    std::string interpreterPath;

    bool resolved =
        handler.resolveCGI(request,
                            found,
                            scriptPath,
                            interpreterPath);

    bool ok = !resolved;

    printResult("resolveCGI rejects wrong final extension", ok);
}

static void testCgiDirectoryRejected()
{
    ServerConfig config = makeBaseConfig();

    Location location("/cgi-dir");
    location.setRoot(WWW_ROOT);
    location.addCgiExtension(".py", "/usr/bin/python3");
    config.addLocation(location);

    makeDirectory(WWW_ROOT + "/script.py");

    RequestHandler handler(config);

    HttpRequest request =
        makeRequest("GET", "/cgi-dir/script.py");

    const Location *found =
        config.findLocation("/cgi-dir");

    std::string scriptPath;
    std::string interpreterPath;

    bool resolved =
        handler.resolveCGI(request,
                            found,
                            scriptPath,
                            interpreterPath);

    bool ok = !resolved;

    printResult("resolveCGI rejects directory with CGI extension", ok);
}

static void testCgiMissingInterpreterStillResolves()
{
    ServerConfig config = makeBaseConfig();

    Location location("/cgi-interpreter");
    location.setRoot(WWW_ROOT);
    location.addCgiExtension(".py", "/does/not/exist");
    config.addLocation(location);

    writeFile(WWW_ROOT + "/interpreter.py", "print('hello')");

    RequestHandler handler(config);

    HttpRequest request =
        makeRequest("GET", "/cgi-interpreter/interpreter.py");

    const Location *found =
        config.findLocation("/cgi-interpreter");

    std::string scriptPath;
    std::string interpreterPath;

    bool resolved =
        handler.resolveCGI(request,
                            found,
                            scriptPath,
                            interpreterPath);

    bool ok = resolved &&
              scriptPath == WWW_ROOT + "/interpreter.py" &&
              interpreterPath == "/does/not/exist";

    printResult("resolveCGI returns configured interpreter path", ok);
}

static void testGetUppercaseUnknownExtension()
{
    ServerConfig config = makeBaseConfig();

    writeFile(WWW_ROOT + "/unknown.XYZ", "unknown");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/unknown.XYZ",
               response);

    bool ok = getStatusCode(response) == 200 &&
              responseHasHeader(response,
                                "Content-Type",
                                "application/octet-stream");

    printResult("GET uppercase unknown extension -> octet-stream", ok);
}

static void testGetFilenameWithMultipleDots()
{
    ServerConfig config = makeBaseConfig();

    writeFile(WWW_ROOT + "/archive.tar.gz", "gzip");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/archive.tar.gz",
               response);

    bool ok = getStatusCode(response) == 200 &&
              getResponseBody(response) == "gzip" &&
              responseHasHeader(response,
                                "Content-Type",
                                "application/octet-stream");

    printResult("GET filename with multiple dots", ok);
}

static void testGetLargeBinaryFile()
{
    ServerConfig config = makeBaseConfig();

    std::string body;

    for (int i = 0; i < 100000; ++i)
    {
        body.push_back(static_cast<char>(i % 256));
    }

    std::string path = WWW_ROOT + "/large.bin";
    writeFile(path, body);

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/large.bin",
               response);

    bool ok = getStatusCode(response) == 200 &&
              getResponseBody(response) == body;

    printResult("GET large binary file preserves all bytes", ok);
}

static void testUrlEncodedSpace()
{
    ServerConfig config = makeBaseConfig();

    writeFile(WWW_ROOT + "/hello world.txt", "encoded space");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/hello%20world.txt",
               response);

    bool ok = getStatusCode(response) == 200 &&
              getResponseBody(response) == "encoded space";

    printResult("GET percent-encoded space", ok);
}

static void testUrlEncodedSlash()
{
    ServerConfig config = makeBaseConfig();

    writeFile(WWW_ROOT + "/encoded-slash.txt", "encoded slash");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/encoded%2Fslash.txt",
               response);

    bool ok = getStatusCode(response) != 200 ||
              getResponseBody(response) != "encoded slash";

    printResult("GET percent-encoded slash does not bypass path handling", ok);
}

static void testUrlEncodedDot()
{
    ServerConfig config = makeBaseConfig();

    writeFile(WWW_ROOT + "/dot-file.txt", "dot file");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/%2E/dot-file.txt",
               response);

    bool ok = getStatusCode(response) == 200 &&
              getResponseBody(response) == "dot file";

    printResult("GET percent-encoded dot", ok);
}

static void testUrlEncodedDotDot()
{
    ServerConfig config = makeBaseConfig();

    writeFile(TEST_ROOT + "/secret-encoded.txt",
              "secret");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/%2E%2E/secret-encoded.txt",
               response);

    bool ok = getStatusCode(response) != 200 ||
              getResponseBody(response) != "secret";

    printResult("GET encoded dot-dot cannot escape root", ok);
}

static void testUrlEncodedNestedDotDot()
{
    ServerConfig config = makeBaseConfig();

    writeFile(TEST_ROOT + "/secret-nested.txt",
              "secret");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/dir/%2E%2E/secret-nested.txt",
               response);

    bool ok = getStatusCode(response) != 200 ||
              getResponseBody(response) != "secret";

    printResult("GET nested encoded dot-dot cannot escape root", ok);
}

static void testMalformedPercent()
{
    ServerConfig config = makeBaseConfig();

    writeFile(WWW_ROOT + "/malformed.txt", "malformed");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/malformed%2.txt",
               response);

    bool ok = getStatusCode(response) != 200 ||
              getResponseBody(response) != "malformed";

    printResult("GET malformed percent encoding", ok);
}

static void testBarePercent()
{
    ServerConfig config = makeBaseConfig();

    writeFile(WWW_ROOT + "/percent.txt", "percent");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/percent%.txt",
               response);

    bool ok = getStatusCode(response) != 200 ||
              getResponseBody(response) != "percent";

    printResult("GET bare percent character", ok);
}

static void testInvalidPercentHex()
{
    ServerConfig config = makeBaseConfig();

    writeFile(WWW_ROOT + "/invalid.txt", "invalid");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/invalid%GG.txt",
               response);

    bool ok = getStatusCode(response) != 200 ||
              getResponseBody(response) != "invalid";

    printResult("GET invalid percent hex", ok);
}

static void testUrlEncodedAmpersandFilename()
{
    ServerConfig config = makeBaseConfig();

    writeFile(WWW_ROOT + "/a&b.txt", "ampersand");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/a%26b.txt",
               response);

    bool ok = getStatusCode(response) == 200 &&
              getResponseBody(response) == "ampersand";

    printResult("GET percent-encoded ampersand", ok);
}

static void testUrlEncodedQuoteFilename()
{
    ServerConfig config = makeBaseConfig();

    writeFile(WWW_ROOT + "/quote\"file.txt", "quote");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/quote%22file.txt",
               response);

    bool ok = getStatusCode(response) == 200 &&
              getResponseBody(response) == "quote";

    printResult("GET percent-encoded quote", ok);
}

static void testUrlEncodedUnicodeByte()
{
    ServerConfig config = makeBaseConfig();

    std::string filename = "caf";
    filename.push_back(static_cast<char>(0xc3));
    filename.push_back(static_cast<char>(0xa9));
    filename += ".txt";

    writeFile(WWW_ROOT + "/" + filename, "unicode");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/caf%C3%A9.txt",
               response);

    bool ok = getStatusCode(response) == 200 &&
              getResponseBody(response) == "unicode";

    printResult("GET percent-encoded UTF-8 filename", ok);
}

static void testEncodedQueryCharacters()
{
    ServerConfig config = makeBaseConfig();

    writeFile(WWW_ROOT + "/query-encoded.txt",
              "query encoded");

    RequestHandler handler(config);
    HttpResponse response;

    runRequest(handler,
               "GET",
               "/query-encoded.txt?name=hello%20world&x=1",
               response);

    bool ok = getStatusCode(response) == 200 &&
              getResponseBody(response) == "query encoded";

    printResult("GET percent-encoded query string", ok);
}

/*
 * ------------------------------------------------------------
 * Main
 * ------------------------------------------------------------
 */

int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << " RequestHandler test suite" << std::endl;
    std::cout << "========================================" << std::endl;

    if (!setupTestEnvironment())
    {
        std::cerr << "Could not create test environment: "
                  << TEST_ROOT << std::endl;
        return 1;
    }

    /*
     * GET
     */
    testGetExistingFile();
    testGetMissingFile();
    testGetEmptyFile();
    testGetBinaryFile();
    testGetContentTypes();
    testGetIndex();
    testGetIndexWithoutTrailingSlash();
    testGetAutoIndex();
    testGetDirectoryNotFoundWithoutAutoIndex();
    testGetLocationRoot();
    testGetLocationIndex();
    testGetQueryString();

    /*
     * Security
     */
    testGetPathTraversal();
    testDeletePathTraversal();
    testAutoIndexHtmlEscaping();

    /*
     * POST
     */
    testPostUpload();
    testPostUploadDisabled();
    testPostUploadStoreMissing();
    testPostEmptyBody();
    testPostLargeBody();

    /*
     * DELETE
     */
    testDeleteExistingFile();
    testDeleteMissingFile();
    testDeleteDirectory();

    /*
     * Methods / locations
     */
    testUnsupportedMethods();
    testLocationAllowedMethods();
    testLongestLocationMatch();

    /*
     * Error pages
     */
    testCustom404Page();

    /*
     * CGI
     */
    testResolveCgi();
    testResolveCgiWrongExtension();
    testResolveCgiMissingScript();
    testResolveCgiTraversal();

	testAutoIndexEmptyUrl();
	testAutoIndexQuoteEscaping();
	testUppercaseMimeTypes();
	testLocationPathBoundary();
	testDeleteLocationRoot();
	testPostBinaryBody();
	testHttpRequestPreservesBinaryBody();
	testMultiplePostUploads();

	testCustom403ErrorPage();
	testCustom405ErrorPage();
	testLocationDisablesServerAutoIndex();
	testLocationEnablesServerAutoIndex();
	testLocationIndexOverridesServerIndex();
	testPostMethodRestriction();
	testDeleteMethodRestriction();
	testDeleteNestedLocationFile();
	testDirectoryMissingIndex();
	testEmptyDirectoryIndex();
	testHeadMethod();
	testOptionsMethod();
	testAutoIndexMultipleEntries();
	testAutoIndexSkipsDotEntries();

	testPostBodyAtExactLimit();
	testPostBodyOverLimit();
	testPostHugeBodyOverLimit();
	testPostResponse();
	testDeleteResponseBodyEmpty();
	testDeleteMissingResponseBody();
	testGetBinaryExactBytes();
	testMixedCaseHtmlMime();
	testMixedCaseCssMime();
	testMixedCaseJsonMime();
	testUnknownExtensionMime();
	testLocationQueryString();
	testLocationRootTrailingSlash();
	testServerRootTrailingSlash();
	testRootLocation();
	testCgiUppercaseExtension();
	testCgiQueryString();
	testCgiWithoutConfiguredLocation();
	testNestedUploadStore();
	testUploadLocationGet();

	testGetRootDirectory();
	testGetDoubleSlash();
	testGetDotPath();
	testGetDirectoryDot();
	testGetDirectoryDotDot();
	testGetNestedTraversalRejected();
	testDeleteRootDirectory();
	testDeleteDirectoryWithSlash();
	testOversizedPostDoesNotCreateFile();
	testPostBinaryAtExactLimit();
	testPostBinaryOverLimit();


	testGetFileWithQueryString();
	testGetDirectoryWithQueryString();
	testGetEmptyQueryString();
	testGetFragmentLikeTarget();

	testPatchMethod();
	testConnectMethod();
	testTraceMethod();

	testDeleteActuallyRemovesFile();
	testDeleteThenGetReturns404();
	testDeleteTwice();

	testPostEmptyBodyCreatesEmptyFile();
	testPostUploadStoreIsFile();

	testCgiLastExtension();
	testCgiWrongFinalExtension();
	testCgiDirectoryRejected();
	testCgiMissingInterpreterStillResolves();

	testGetUppercaseUnknownExtension();
	testGetFilenameWithMultipleDots();
	testGetLargeBinaryFile();

	testUrlEncodedSpace();
	testUrlEncodedSlash();
	testUrlEncodedDot();
	testUrlEncodedDotDot();
	testUrlEncodedNestedDotDot();
	testMalformedPercent();
	testBarePercent();
	testInvalidPercentHex();
	testUrlEncodedAmpersandFilename();
	testUrlEncodedQuoteFilename();
	testUrlEncodedUnicodeByte();
	testEncodedQueryCharacters();

    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Passed: " << g_passed << std::endl;
    std::cout << "Failed: " << g_failed << std::endl;
    std::cout << "========================================" << std::endl;

    cleanTestEnvironment();

    if (g_failed != 0)
        return 1;

    return 0;
}