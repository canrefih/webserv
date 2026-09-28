#include "ServerConfig.hpp"
#include "Location.hpp"

#include <iostream>
#include <string>

static int passed = 0;
static int total = 0;

static bool check(bool condition, const std::string &message)
{
    ++total;

    if (!condition)
    {
        std::cerr << "FAIL: " << message << std::endl;
        return false;
    }

    ++passed;
    std::cout << "PASS: " << message << std::endl;
    return true;
}

int main()
{
    std::cout << "=== SERVER CONFIG TESTS ===" << std::endl;

    /*
     * 1-7: Default values
     */

    std::cout << "\n--- Default values ---" << std::endl;

    {
        ServerConfig config;

        if (!check(config.getHost() == "127.0.0.1",
                   "Default host"))
            return 1;

        if (!check(config.getPort() == 8080,
                   "Default port"))
            return 1;

        if (!check(config.getRoot() == "./www",
                   "Default root"))
            return 1;

        if (!check(config.getIndex() == "index.html",
                   "Default index"))
            return 1;

        if (!check(config.getAutoIndex() == false,
                   "Default autoindex"))
            return 1;

        if (!check(config.getUploadPath().empty(),
                   "Default upload path is empty"))
            return 1;

        if (!check(config.getClientMaxBodySize() == 2097152,
                   "Default client max body size"))
            return 1;
    }

    /*
     * 8-15: Setters
     */

    std::cout << "\n--- Setters ---" << std::endl;

    {
        ServerConfig config;

        config.setHost("0.0.0.0");
        config.setPort(9090);
        config.setRoot("/var/www");
        config.setIndex("home.html");
        config.setAutoIndex(true);
        config.setUploadPath("/tmp/uploads");
        config.setClientMaxBodySize(10485760);

        if (!check(config.getHost() == "0.0.0.0",
                   "setHost/getHost"))
            return 1;

        if (!check(config.getPort() == 9090,
                   "setPort/getPort"))
            return 1;

        if (!check(config.getRoot() == "/var/www",
                   "setRoot/getRoot"))
            return 1;

        if (!check(config.getIndex() == "home.html",
                   "setIndex/getIndex"))
            return 1;

        if (!check(config.getAutoIndex() == true,
                   "setAutoIndex/getAutoIndex"))
            return 1;

        if (!check(config.getUploadPath() == "/tmp/uploads",
                   "setUploadPath/getUploadPath"))
            return 1;

        if (!check(config.getClientMaxBodySize() == 10485760,
                   "setClientMaxBodySize/getClientMaxBodySize"))
            return 1;
    }

    /*
     * 16-18: Location management
     */

    std::cout << "\n--- Location management ---" << std::endl;

    {
        ServerConfig config;

        Location root("/");
        Location api("/api");
        Location upload("/upload");

        config.addLocation(root);
        config.addLocation(api);
        config.addLocation(upload);

        if (!check(config.getLocations().size() == 3,
                   "addLocation adds locations"))
            return 1;

        if (!check(config.getLocations()[0].getPath() == "/",
                   "First location path"))
            return 1;

        if (!check(config.getLocations()[1].getPath() == "/api",
                   "Second location path"))
            return 1;

        if (!check(config.getLocations()[2].getPath() == "/upload",
                   "Third location path"))
            return 1;
    }

    /*
     * 19-21: Mutable / const location access
     */

    std::cout << "\n--- Location access ---" << std::endl;

    {
        ServerConfig config;

        config.addLocation(Location("/"));

        config.getLocations()[0].setRoot("/var/www");

        if (!check(config.getLocations()[0].getRoot() == "/var/www",
                   "Mutable getLocations works"))
            return 1;

        const ServerConfig &constConfig = config;

        if (!check(constConfig.getLocations().size() == 1,
                   "Const getLocations works"))
            return 1;

        if (!check(constConfig.getLocations()[0].getRoot() == "/var/www",
                   "Const location access works"))
            return 1;
    }

    /*
     * 22-28: findLocation exact matches
     */

    std::cout << "\n--- findLocation exact matches ---" << std::endl;

    {
        ServerConfig config;

        config.addLocation(Location("/"));
        config.addLocation(Location("/api"));
        config.addLocation(Location("/upload"));

        const Location *loc;

        loc = config.findLocation("/");
        if (!check(loc != NULL && loc->getPath() == "/",
                   "findLocation('/')"))
            return 1;

        loc = config.findLocation("/api");
        if (!check(loc != NULL && loc->getPath() == "/api",
                   "findLocation('/api') exact"))
            return 1;

        loc = config.findLocation("/upload");
        if (!check(loc != NULL && loc->getPath() == "/upload",
                   "findLocation('/upload') exact"))
            return 1;

        loc = config.findLocation("/missing");
        if (!check(loc != NULL && loc->getPath() == "/",
                   "Unknown path falls back to '/'"))
            return 1;
    }

    /*
     * 29-35: findLocation prefix matches
     */

    std::cout << "\n--- findLocation prefix matches ---" << std::endl;

    {
        ServerConfig config;

        config.addLocation(Location("/"));
        config.addLocation(Location("/api"));
        config.addLocation(Location("/api/v1"));
        config.addLocation(Location("/upload"));

        const Location *loc;

        loc = config.findLocation("/api/users");
        if (!check(loc != NULL && loc->getPath() == "/api",
                   "findLocation('/api/users')"))
            return 1;

        loc = config.findLocation("/api/v1/users");
        if (!check(loc != NULL && loc->getPath() == "/api/v1",
                   "Longest prefix match"))
            return 1;

        loc = config.findLocation("/upload/file.txt");
        if (!check(loc != NULL && loc->getPath() == "/upload",
                   "findLocation('/upload/file.txt')"))
            return 1;

        loc = config.findLocation("/something/here");
        if (!check(loc != NULL && loc->getPath() == "/",
                   "Root location matches arbitrary path"))
            return 1;
    }

    /*
     * 36-39: Prefix boundary protection
     */

    std::cout << "\n--- Prefix boundary ---" << std::endl;

    {
        ServerConfig config;

        config.addLocation(Location("/api"));
        config.addLocation(Location("/"));

        const Location *loc;

        loc = config.findLocation("/api/users");
        if (!check(loc != NULL && loc->getPath() == "/api",
                   "/api matches /api/users"))
            return 1;

        loc = config.findLocation("/apix");
        if (!check(loc != NULL && loc->getPath() == "/",
                   "/api must NOT match /apix"))
            return 1;

        loc = config.findLocation("/api2");
        if (!check(loc != NULL && loc->getPath() == "/",
                   "/api must NOT match /api2"))
            return 1;

        loc = config.findLocation("/api/");
        if (!check(loc != NULL && loc->getPath() == "/api",
                   "/api matches /api/"))
            return 1;
    }

    /*
     * 40-43: Longest prefix
     */

    std::cout << "\n--- Longest prefix ---" << std::endl;

    {
        ServerConfig config;

        config.addLocation(Location("/"));
        config.addLocation(Location("/api"));
        config.addLocation(Location("/api/v1"));
        config.addLocation(Location("/api/v1/users"));

        const Location *loc;

        loc = config.findLocation("/api/v1/users/profile");

        if (!check(loc != NULL &&
                   loc->getPath() == "/api/v1/users",
                   "findLocation chooses longest matching prefix"))
            return 1;

        loc = config.findLocation("/api/v1/test");

        if (!check(loc != NULL &&
                   loc->getPath() == "/api/v1",
                   "findLocation chooses second-longest prefix"))
            return 1;
    }

    /*
     * 44-46: Empty locations
     */

    std::cout << "\n--- Empty location collection ---" << std::endl;

    {
        ServerConfig config;

        if (!check(config.getLocations().empty(),
                   "New config has no locations"))
            return 1;

        const Location *loc = config.findLocation("/anything");

        if (!check(loc == NULL,
                   "findLocation returns NULL without locations"))
            return 1;
    }

    /*
     * 47-50: Error pages
     */

    std::cout << "\n--- Error pages ---" << std::endl;

    {
        ServerConfig config;

        if (!check(config.getErrorPage(404) == NULL,
                   "Unknown error page returns NULL"))
            return 1;

        config.setErrorPage(404, "/404.html");

        const std::string *page = config.getErrorPage(404);

        if (!check(page != NULL,
                   "Configured error page is found"))
            return 1;

        if (!check(*page == "/404.html",
                   "Configured error page path"))
            return 1;

        config.setErrorPage(500, "/500.html");

        page = config.getErrorPage(500);

        if (!check(page != NULL && *page == "/500.html",
                   "Second error page works"))
            return 1;
    }

    /*
     * 51-53: Error page overwrite
     */

    std::cout << "\n--- Error page overwrite ---" << std::endl;

    {
        ServerConfig config;

        config.setErrorPage(404, "/old.html");
        config.setErrorPage(404, "/new.html");

        const std::string *page = config.getErrorPage(404);

        if (!check(page != NULL && *page == "/new.html",
                   "Error page can be replaced"))
            return 1;
    }

    /*
     * 54-56: Independent server configs
     */

    std::cout << "\n--- Independent configs ---" << std::endl;

    {
        ServerConfig first;
        ServerConfig second;

        first.setPort(8080);
        first.setRoot("/first");

        second.setPort(9090);
        second.setRoot("/second");

        if (!check(first.getPort() == 8080 &&
                   second.getPort() == 9090,
                   "Server ports are independent"))
            return 1;

        if (!check(first.getRoot() == "/first" &&
                   second.getRoot() == "/second",
                   "Server roots are independent"))
            return 1;
    }

    std::cout << "\n--- Location matching with trailing slashes ---" << std::endl;

    {
        ServerConfig config;

        Location locRoot("/");
        Location locPutTest("/put_test/");
        Location locDocs("/docs");

        config.addLocation(locRoot);
        config.addLocation(locPutTest);
        config.addLocation(locDocs);

        // Test 1: Sonu slash ile biten location (/put_test/) altındaki dosya eşleşiyor mu?
        const Location *match1 = config.findLocation("/put_test/file.txt");
        if (!check(match1 != NULL && match1->getPath() == "/put_test/",
                   "Location with trailing slash matches nested request"))
            return 1;

        // Test 2: Sonu slash ile bitmeyen location (/docs) altındaki dosya eşleşiyor mu?
        const Location *match2 = config.findLocation("/docs/api.html");
        if (!check(match2 != NULL && match2->getPath() == "/docs",
                   "Location without trailing slash matches nested request"))
            return 1;

        // Test 3: Kelime benzerliği olan ama farklı dizin olan istek eleniyor mu? (/docs_fake -> / olmalı)
        const Location *match3 = config.findLocation("/docs_fake/file.txt");
        if (!check(match3 != NULL && match3->getPath() == "/",
                   "Partial string prefix match correctly rejected"))
            return 1;
    }

    /*
     * Summary
     */

    std::cout << "\n=== SERVER CONFIG TEST SUMMARY ===" << std::endl;
    std::cout << "Passed: " << passed
              << "/" << total << std::endl;

    if (passed != total)
    {
        std::cerr << "SERVER CONFIG TESTS FAILED" << std::endl;
        return 1;
    }

    std::cout << "ALL SERVER CONFIG TESTS PASSED" << std::endl;
    return 0;
}
