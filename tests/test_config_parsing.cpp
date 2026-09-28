#include "../include/Config.hpp"
#include "../include/ServerConfig.hpp"
#include "../include/Location.hpp"

#include <iostream>
#include <fstream>

int main()
{
	std::cout << "=== CONFIG PARSING TESTS ===" << std::endl;

	Config config;

	std::cout << "\nTest 1: Parse valid config file..." << std::endl;
	if (!config.parse("config/test.conf"))
	{
		std::cerr << "FAIL: Could not parse test.conf" << std::endl;
		return 1;
	}
	std::cout << "PASS: Config parsed successfully" << std::endl;

	const std::vector<ServerConfig> &servers = config.getServers();

	std::cout << "\nTest 2: Check server count..." << std::endl;
	if (servers.size() != 2)
	{
		std::cerr << "FAIL: Expected 2 servers, got "
				<< servers.size() << std::endl;
		return 1;
	}
	std::cout << "PASS: Found 2 servers" << std::endl;

	std::cout << "\nTest 3: Check first server basic config..." << std::endl;
	if (servers[0].getHost() != "127.0.0.1" ||
		servers[0].getPort() != 8080 ||
		servers[0].getRoot() != "./www" ||
		servers[0].getIndex() != "index.html" ||
		servers[0].getAutoIndex() != true)
	{
		std::cerr << "FAIL: Server 1 basic config incorrect"
				<< std::endl;
		return 1;
	}
	std::cout << "PASS: Server 1 basic config correct" << std::endl;

	std::cout << "\nTest 4: Check first server client_max_body_size..."
			<< std::endl;
	if (servers[0].getClientMaxBodySize() != 10 * 1024 * 1024)
	{
		std::cerr << "FAIL: Expected 10M, got "
				<< servers[0].getClientMaxBodySize() << std::endl;
		return 1;
	}
	std::cout << "PASS: Server 1 client_max_body_size = 10M"
			<< std::endl;

	std::cout << "\nTest 5: Check first server error pages..."
			<< std::endl;
	const std::string *error404 = servers[0].getErrorPage(404);
	const std::string *error500 = servers[0].getErrorPage(500);

	if (error404 == NULL ||
		error500 == NULL ||
		*error404 != "/404.html" ||
		*error500 != "/500.html")
	{
    std::cerr << "FAIL: Server 1 error pages incorrect"
              << std::endl;
    return 1;
}
	std::cout << "PASS: Server 1 error pages correct" << std::endl;

	std::cout << "\nTest 6: Check first server locations..."
			<< std::endl;

	const std::vector<Location> &locations = servers[0].getLocations();

	if (locations.size() != 4)
	{
		std::cerr << "FAIL: Expected 4 locations, got "
				<< locations.size() << std::endl;
		return 1;
	}

	if (locations[0].getPath() != "/" ||
		locations[1].getPath() != "/upload" ||
		locations[2].getPath() != "/api" ||
		locations[3].getPath() != "/cgi-bin")
	{
		std::cerr << "FAIL: Location paths incorrect"
				<< std::endl;
		return 1;
	}

	std::cout << "PASS: 4 locations parsed correctly" << std::endl;

	std::cout << "\nTest 7: Check / location methods..." << std::endl;
	if (!locations[0].isMethodAllowed("GET") ||
		!locations[0].isMethodAllowed("POST") ||
		locations[0].isMethodAllowed("DELETE"))
	{
		std::cerr << "FAIL: / allow_methods parsed incorrectly"
				<< std::endl;
		return 1;
	}
	std::cout << "PASS: / allow_methods parsed correctly" << std::endl;

	std::cout << "\nTest 8: Check /upload location..." << std::endl;
	if (!locations[1].getUpload() ||
		locations[1].getUploadStore() != "./uploads")
	{
		std::cerr << "FAIL: /upload config parsed incorrectly"
				<< std::endl;
		return 1;
	}
	std::cout << "PASS: /upload config parsed correctly" << std::endl;

	std::cout << "\nTest 9: Check /api location methods..." << std::endl;
	if (!locations[2].isMethodAllowed("GET") ||
		!locations[2].isMethodAllowed("POST") ||
		!locations[2].isMethodAllowed("DELETE") ||
		locations[2].isMethodAllowed("PUT"))
	{
		std::cerr << "FAIL: /api allow_methods parsed incorrectly"
				<< std::endl;
		return 1;
	}
	std::cout << "PASS: /api allow_methods parsed correctly" << std::endl;

	std::cout << "\nTest 10: Check CGI location..." << std::endl;
	if (!locations[3].isCgiExtension(".py") ||
		locations[3].getCgiInterpreter(".py") != "/usr/bin/python3")
	{
		std::cerr << "FAIL: CGI configuration parsed incorrectly"
				<< std::endl;
		return 1;
	}
	std::cout << "PASS: CGI configuration parsed correctly" << std::endl;

	std::cout << "\nTest 11: Check second server basic config..."
			<< std::endl;
	if (servers[1].getHost() != "127.0.0.1" ||
		servers[1].getPort() != 8081 ||
		servers[1].getRoot() != "./www2" ||
		servers[1].getIndex() != "index.html" ||
		servers[1].getAutoIndex() != false)
	{
		std::cerr << "FAIL: Server 2 basic config incorrect"
				<< std::endl;
		return 1;
	}
	std::cout << "PASS: Server 2 basic config correct" << std::endl;

	std::cout << "\nTest 12: Check second server client_max_body_size..."
			<< std::endl;
	if (servers[1].getClientMaxBodySize() != 5 * 1024 * 1024)
	{
		std::cerr << "FAIL: Expected 5M, got "
				<< servers[1].getClientMaxBodySize() << std::endl;
		return 1;
	}
	std::cout << "PASS: Server 2 client_max_body_size = 5M"
			<< std::endl;

	std::cout << "\nTest 13: Check second server error page..."
			<< std::endl;
	const std::string *error404Second = servers[1].getErrorPage(404);

	if (error404Second == NULL ||
		*error404Second != "/custom404.html")
	{
		std::cerr << "FAIL: Server 2 error page incorrect"
				<< std::endl;
		return 1;
	}
	std::cout << "PASS: Server 2 error page correct" << std::endl;

	std::cout << "\nTest 14: Check second server locations..."
			<< std::endl;

	const std::vector<Location> &locations2 = servers[1].getLocations();

	if (locations2.size() != 3)
	{
		std::cerr << "FAIL: Expected 3 locations, got "
				<< locations2.size() << std::endl;
		return 1;
	}

	if (locations2[0].getPath() != "/" ||
		locations2[1].getPath() != "/api" ||
		locations2[2].getPath() != "/cgi-bin")
	{
		std::cerr << "FAIL: Server 2 location paths incorrect"
				<< std::endl;
		return 1;
	}

	std::cout << "PASS: Server 2 locations parsed correctly"
			<< std::endl;

	std::cout << "\nTest 15: Check getServerByPort..." << std::endl;

	const ServerConfig *server = config.getServerByPort(8080);

	if (server == NULL || server->getPort() != 8080)
	{
		std::cerr << "FAIL: getServerByPort(8080) failed"
				<< std::endl;
		return 1;
	}

	server = config.getServerByPort(8081);

	if (server == NULL || server->getPort() != 8081)
	{
		std::cerr << "FAIL: getServerByPort(8081) failed"
				<< std::endl;
		return 1;
	}

	std::cout << "PASS: getServerByPort working correctly"
			<< std::endl;

	std::cout << "\nTest 16: Check non-existent port..." << std::endl;

	server = config.getServerByPort(9999);

	if (server != NULL)
	{
		std::cerr << "FAIL: getServerByPort(9999) should return NULL"
				<< std::endl;
		return 1;
	}

	std::cout << "PASS: Non-existent port returns NULL" << std::endl;

	std::cout << "\nTest 17: Port-only listen syntax (e.g. listen 8080;)..." << std::endl;
    {
        std::ofstream file("config/test_port_only.conf");
        file << "server {\n";
        file << "    listen 8080;\n";
        file << "}\n";
    }
    if (!config.parse("config/test_port_only.conf"))
    {
        std::cerr << "FAIL: listen without IP prefix should be valid" << std::endl;
        return 1;
    }
    std::cout << "PASS: Port-only listen accepted" << std::endl;

    std::cout << "\nTest 18: Check server_name directive support..." << std::endl;
    {
        std::ofstream file("config/test_server_name.conf");
        file << "server {\n";
        file << "    listen 8080;\n";
        file << "    server_name example.com www.example.com;\n";
        file << "}\n";
    }
    if (!config.parse("config/test_server_name.conf"))
    {
        std::cerr << "FAIL: server_name directive should be parsed" << std::endl;
        return 1;
    }
    std::cout << "PASS: server_name parsed" << std::endl;

    std::cout << "\nTest 19: Check HTTP Redirection (return directive)..." << std::endl;
    {
        std::ofstream file("config/test_return.conf");
        file << "server {\n";
        file << "    listen 8080;\n";
        file << "    location /old {\n";
        file << "        return 301 /new_path;\n";
        file << "    }\n";
        file << "}\n";
    }
    if (!config.parse("config/test_return.conf"))
    {
        std::cerr << "FAIL: return/redirection directive in location should be parsed" << std::endl;
        return 1;
    }
    std::cout << "PASS: HTTP Redirection (return) parsed" << std::endl;

    std::cout << "\nTest 20: Curly brace on new line (server \\n {)..." << std::endl;
    {
        std::ofstream file("config/test_newline_brace.conf");
        file << "server\n{\n";
        file << "    listen 8080;\n";
        file << "}\n";
    }
    if (!config.parse("config/test_newline_brace.conf"))
    {
        std::cerr << "FAIL: Opening brace on new line should be valid" << std::endl;
        return 1;
    }
    std::cout << "PASS: Newline brace syntax accepted" << std::endl;

    std::cout << "\nTest 21: Multiple status codes in error_page..." << std::endl;
    {
        std::ofstream file("config/test_multi_error.conf");
        file << "server {\n";
        file << "    listen 8080;\n";
        file << "    error_page 404 403 500 /error.html;\n";
        file << "}\n";
    }
    if (!config.parse("config/test_multi_error.conf"))
    {
        std::cerr << "FAIL: Multiple error codes in one error_page directive should be parsed" << std::endl;
        return 1;
    }
    std::cout << "PASS: Multiple error codes parsed" << std::endl;

	std::cout << "\n=== ALL VALID CONFIG TESTS PASSED ==="
			<< std::endl;

	return 0;

}
