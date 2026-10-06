#include "../include/Config.hpp"

#include <iostream>
#include <fstream>
#include <cstdio>

static bool writeConfig(const std::string &filename,
const std::string &content)
{
	std::ofstream file(filename.c_str());

	if (!file.is_open())
		return false;

	file << content;
	file.close();

	return true;

}

static bool expectFailure(const std::string &name,
const std::string &content)
{
	const std::string filename = "config/test_invalid.conf";

	if (!writeConfig(filename, content))
	{
		std::cerr << "FAIL: Could not create temporary config for "
				<< name << std::endl;
		return false;
	}

	Config config;
	bool result = config.parse(filename);

	std::remove(filename.c_str());

	if (result)
	{
		std::cerr << "FAIL: " << name
				<< " should have been rejected" << std::endl;
		return false;
	}

	std::cout << "PASS: " << name << std::endl;
	return true;

}

int main()
{
	std::cout << "=== INVALID CONFIG TESTS ===" << std::endl;

	int passed = 0;
	int total = 0;

	Config config;
	/*
	* Test 1
	* Configuration file does not exist.
	*/
	{
		++total;

		if (config.parse("config/does_not_exist.conf"))
		{
			std::cerr << "FAIL: Non-existent file should be rejected"
					<< std::endl;
			return 1;
		}

		std::cout << "PASS: Non-existent configuration file rejected"
				<< std::endl;
		++passed;
	}

	/*
	* Test 2
	* Empty configuration.
	*/
	{
		++total;

		if (expectFailure(
				"Empty configuration",
				""))
			++passed;
	}

	/*
	* Test 3
	* Directive outside a server block.
	*/
	{
		++total;

		if (expectFailure(
				"Directive outside server",
				"root ./www;\n"))
			++passed;
	}

	/*
	* Test 4
	* Unknown directive.
	*/
	{
		++total;

		if (expectFailure(
				"Unknown directive",
				"server {\n"
				"    listen 127.0.0.1:8080;\n"
				"    totally_unknown value;\n"
				"}\n"))
			++passed;
	}

	/*
	* Test 5
	* Missing opening brace after server.
	*/
	{
		++total;

		if (expectFailure(
				"Server without opening brace",
				"server\n"
				"    listen 127.0.0.1:8080;\n"
				"}\n"))
			++passed;
	}

	/*
	* Test 6
	* Unclosed server block.
	*/
	{
		++total;

		if (expectFailure(
				"Unclosed server block",
				"server {\n"
				"    listen 127.0.0.1:8080;\n"))
			++passed;
	}

	/*
	* Test 7
	* Unclosed location block.
	*/
	{
		++total;

		if (expectFailure(
				"Unclosed location block",
				"server {\n"
				"    listen 127.0.0.1:8080;\n"
				"    location / {\n"
				"        allow_methods GET;\n"
				"}\n"))
			++passed;
	}

	/*
	* Test 8
	* Nested location blocks.
	*/
	{
		++total;

		if (expectFailure(
				"Nested location blocks",
				"server {\n"
				"    listen 127.0.0.1:8080;\n"
				"    location / {\n"
				"        location /api {\n"
				"            allow_methods GET;\n"
				"        }\n"
				"    }\n"
				"}\n"))
			++passed;
	}

	/*
	* Test 9
	* Nested server blocks.
	*
	* Current Config.cpp may incorrectly accept this.
	* This test is intentionally important.
	*/
	{
		++total;

		if (expectFailure(
				"Nested server blocks",
				"server {\n"
				"    listen 127.0.0.1:8080;\n"
				"    server {\n"
				"        listen 127.0.0.1:8081;\n"
				"    }\n"
				"}\n"))
			++passed;
	}

	/*
	* Test 10
	* Invalid autoindex value.
	*/
	{
		++total;

		if (expectFailure(
				"Invalid autoindex value",
				"server {\n"
				"    listen 127.0.0.1:8080;\n"
				"    autoindex maybe;\n"
				"}\n"))
			++passed;
	}

	/*
	* Test 11
	* Invalid upload value.
	*/
	{
		++total;

		if (expectFailure(
				"Invalid upload value",
				"server {\n"
				"    listen 127.0.0.1:8080;\n"
				"    location /upload {\n"
				"        upload maybe;\n"
				"    }\n"
				"}\n"))
			++passed;
	}

	/*
	* Test 12
	* allow_methods outside location.
	*/
	{
		++total;

		if (expectFailure(
				"allow_methods outside location",
				"server {\n"
				"    listen 127.0.0.1:8080;\n"
				"    allow_methods GET POST;\n"
				"}\n"))
			++passed;
	}

	/*
	* Test 13
	* upload outside location.
	*/
	{
		++total;

		if (expectFailure(
				"upload outside location",
				"server {\n"
				"    listen 127.0.0.1:8080;\n"
				"    upload on;\n"
				"}\n"))
			++passed;
	}

	/*
	* Test 14
	* cgi_extension outside location.
	*/
	{
		++total;

		if (expectFailure(
				"cgi_extension outside location",
				"server {\n"
				"    listen 127.0.0.1:8080;\n"
				"    cgi_extension .py /usr/bin/python3;\n"
				"}\n"))
			++passed;
	}

	/*
	* Test 15
	* upload_store outside location.
	*/
	{
		++total;

		if (expectFailure(
				"upload_store outside location",
				"server {\n"
				"    listen 127.0.0.1:8080;\n"
				"    upload_store ./uploads;\n"
				"}\n"))
			++passed;
	}

	/*
	* Test 16
	* client_max_body_size inside location is allowed (as in nginx):
	* it overrides the server limit for that location only.
	* An invalid value is still rejected there.
	*/
	{
		++total;

		const std::string filename = "config/test_invalid.conf";
		Config locationConfig;
		bool ok = writeConfig(filename,
				"server {\n"
				"    listen 127.0.0.1:8080;\n"
				"    client_max_body_size 10M;\n"
				"    location /small {\n"
				"        client_max_body_size 100;\n"
				"    }\n"
				"}\n")
			&& locationConfig.parse(filename);

		std::remove(filename.c_str());

		if (ok)
		{
			const ServerConfig &server = locationConfig.getServers()[0];
			const Location *small = server.findLocation("/small");

			ok = small != NULL
				&& server.getClientMaxBodySize() == 10 * 1024 * 1024
				&& server.getClientMaxBodySize(small) == 100
				&& server.getClientMaxBodySize(server.findLocation("/other")) == 10 * 1024 * 1024;
		}

		if (ok)
			std::cout << "PASS: client_max_body_size inside location overrides server limit" << std::endl;
		else
			std::cerr << "FAIL: client_max_body_size inside location should override server limit" << std::endl;

		if (ok && expectFailure(
				"Invalid client_max_body_size inside location",
				"server {\n"
				"    listen 127.0.0.1:8080;\n"
				"    location / {\n"
				"        client_max_body_size abc;\n"
				"    }\n"
				"}\n"))
			++passed;
	}

	/*
	* Test 17
	* error_page inside location.
	*/
	{
		++total;

		if (expectFailure(
				"error_page inside location",
				"server {\n"
				"    listen 127.0.0.1:8080;\n"
				"    location / {\n"
				"        error_page 404 /404.html;\n"
				"    }\n"
				"}\n"))
			++passed;
	}

	/*
	* Test 18
	* listen inside location.
	*/
	{
		++total;

		if (expectFailure(
				"listen inside location",
				"server {\n"
				"    location / {\n"
				"        listen 127.0.0.1:8080;\n"
				"    }\n"
				"}\n"))
			++passed;
	}

	/*
	* Test 19
	* Invalid error page status code.
	*/
	{
		++total;

		if (expectFailure(
				"Invalid error_page status",
				"server {\n"
				"    listen 127.0.0.1:8080;\n"
				"    error_page 200 /ok.html;\n"
				"}\n"))
			++passed;
	}

	/*
	* Test 20
	* Missing error page path.
	*/
	{
		++total;

		if (expectFailure(
				"Missing error_page path",
				"server {\n"
				"    listen 127.0.0.1:8080;\n"
				"    error_page 404;\n"
				"}\n"))
			++passed;
	}

	/*
	* Test 21
	* Invalid CGI extension directive.
	*/
	{
		++total;

		if (expectFailure(
				"Invalid cgi_extension",
				"server {\n"
				"    listen 127.0.0.1:8080;\n"
				"    location /cgi-bin {\n"
				"        cgi_extension .py;\n"
				"    }\n"
				"}\n"))
			++passed;
	}

	/*
	* Test 22
	* Missing location path.
	*/
	{
		++total;

		if (expectFailure(
				"Missing location path",
				"server {\n"
				"    listen 127.0.0.1:8080;\n"
				"    location {\n"
				"        allow_methods GET;\n"
				"    }\n"
				"}\n"))
			++passed;
	}

	/*
	* Test 23
	* Invalid listen format.
	*/
	{
		++total;

		if (expectFailure(
				"Invalid listen format",
				"server {\n"
				"    listen 127.0.0.1;\n"
				"}\n"))
			++passed;
	}

	/*
	* Test 24
	* Non-numeric port.
	*
	* Current Config.cpp may incorrectly accept this because atoi()
	* returns 0.
	*/
	{
		++total;

		if (expectFailure(
				"Non-numeric port",
				"server {\n"
				"    listen 127.0.0.1:abc;\n"
				"}\n"))
			++passed;
	}

	/*
	* Test 25
	* Port out of valid TCP range.
	*
	* Current Config.cpp may incorrectly accept this.
	*/
	{
		++total;

		if (expectFailure(
				"Port above 65535",
				"server {\n"
				"    listen 127.0.0.1:99999;\n"
				"}\n"))
			++passed;
	}

	/*
	* Test 26
	* Invalid client_max_body_size.
	*
	* Current Config.cpp may incorrectly accept this because atoi()
	* returns 0.
	*/
	{
		++total;

		if (expectFailure(
				"Invalid client_max_body_size",
				"server {\n"
				"    listen 127.0.0.1:8080;\n"
				"    client_max_body_size abc;\n"
				"}\n"))
			++passed;
	}

	/*
	* Test 27
	* Invalid body size suffix.
	*
	* Current Config.cpp may incorrectly accept this.
	*/
	{
		++total;

		if (expectFailure(
				"Invalid body size suffix",
				"server {\n"
				"    listen 127.0.0.1:8080;\n"
				"    client_max_body_size 10XYZ;\n"
				"}\n"))
			++passed;
	}

	/*
	* Test 28
	* Invalid error page status text.
	*
	* Current Config.cpp may incorrectly accept this because atoi()
	* reads the numeric prefix.
	*/
	{
		++total;

		if (expectFailure(
				"Non-numeric error_page status",
				"server {\n"
				"    listen 127.0.0.1:8080;\n"
				"    error_page 404abc /404.html;\n"
				"}\n"))
			++passed;
	}

    std::cout << "\nTest 29: Location without opening brace..."
              << std::endl;
    {
        std::ofstream file("config/test_invalid.conf");
        file << "server {\n";
        file << "    listen 127.0.0.1:8080;\n";
        file << "    location /api\n";
        file << "        allow_methods GET;\n";
        file << "    }\n";
        file << "}\n";
    }

    if (config.parse("config/test_invalid.conf"))
    {
        std::cerr << "FAIL: location without '{' should be rejected"
                  << std::endl;
        return 1;
    }

    std::cout << "PASS: Location without '{' rejected"
              << std::endl;


    std::cout << "\nTest 30: Missing semicolon after listen..." << std::endl;
    {
        std::ofstream file("config/test_invalid.conf");
        file << "server {\n";
        file << "    listen 127.0.0.1:8080\n";
        file << "}\n";
    }

    if (config.parse("config/test_invalid.conf"))
    {
        std::cerr << "FAIL: missing semicolon after listen should be rejected"
                  << std::endl;
        return 1;
    }
    std::cout << "PASS: Missing semicolon after listen rejected"
              << std::endl;


    std::cout << "\nTest 31: Missing semicolon after root..." << std::endl;
    {
        std::ofstream file("config/test_invalid.conf");
        file << "server {\n";
        file << "    root ./www\n";
        file << "}\n";
    }

    if (config.parse("config/test_invalid.conf"))
    {
        std::cerr << "FAIL: missing semicolon after root should be rejected"
                  << std::endl;
        return 1;
    }
    std::cout << "PASS: Missing semicolon after root rejected"
              << std::endl;


    std::cout << "\nTest 32: Extra token after listen..." << std::endl;
    {
        std::ofstream file("config/test_invalid.conf");
        file << "server {\n";
        file << "    listen 127.0.0.1:8080 extra;\n";
        file << "}\n";
    }

    if (config.parse("config/test_invalid.conf"))
    {
        std::cerr << "FAIL: extra listen token should be rejected"
                  << std::endl;
        return 1;
    }
    std::cout << "PASS: Extra listen token rejected" << std::endl;


    std::cout << "\nTest 33: Extra token after root..." << std::endl;
    {
        std::ofstream file("config/test_invalid.conf");
        file << "server {\n";
        file << "    root ./www extra;\n";
        file << "}\n";
    }

    if (config.parse("config/test_invalid.conf"))
    {
        std::cerr << "FAIL: extra root token should be rejected"
                  << std::endl;
        return 1;
    }
    std::cout << "PASS: Extra root token rejected" << std::endl;


    std::cout << "\nTest 34: Extra token after server opening brace..."
              << std::endl;
    {
        std::ofstream file("config/test_invalid.conf");
        file << "server { extra\n";
        file << "    listen 127.0.0.1:8080;\n";
        file << "}\n";
    }

    if (config.parse("config/test_invalid.conf"))
    {
        std::cerr << "FAIL: extra server token should be rejected"
                  << std::endl;
        return 1;
    }
    std::cout << "PASS: Extra server token rejected" << std::endl;


    std::cout << "\nTest 35: Empty root value..." << std::endl;
    {
        std::ofstream file("config/test_invalid.conf");
        file << "server {\n";
        file << "    root;\n";
        file << "}\n";
    }

    if (config.parse("config/test_invalid.conf"))
    {
        std::cerr << "FAIL: empty root should be rejected"
                  << std::endl;
        return 1;
    }
    std::cout << "PASS: Empty root rejected" << std::endl;


    std::cout << "\nTest 36: Empty index value..." << std::endl;
    {
        std::ofstream file("config/test_invalid.conf");
        file << "server {\n";
        file << "    index;\n";
        file << "}\n";
    }

    if (config.parse("config/test_invalid.conf"))
    {
        std::cerr << "FAIL: empty index should be rejected"
                  << std::endl;
        return 1;
    }
    std::cout << "PASS: Empty index rejected" << std::endl;


    std::cout << "\nTest 37: Empty upload_store value..." << std::endl;
    {
        std::ofstream file("config/test_invalid.conf");
        file << "server {\n";
        file << "    location /upload {\n";
        file << "        upload_store;\n";
        file << "    }\n";
        file << "}\n";
    }

    if (config.parse("config/test_invalid.conf"))
    {
        std::cerr << "FAIL: empty upload_store should be rejected"
                  << std::endl;
        return 1;
    }
    std::cout << "PASS: Empty upload_store rejected" << std::endl;


    std::cout << "\nTest 38: allow_methods without methods..." << std::endl;
    {
        std::ofstream file("config/test_invalid.conf");
        file << "server {\n";
        file << "    location /api {\n";
        file << "        allow_methods;\n";
        file << "    }\n";
        file << "}\n";
    }

    if (config.parse("config/test_invalid.conf"))
    {
        std::cerr << "FAIL: allow_methods without methods should be rejected"
                  << std::endl;
        return 1;
    }
    std::cout << "PASS: Empty allow_methods rejected" << std::endl;


    std::cout << "\nTest 39: CGI extension without interpreter..." << std::endl;
    {
        std::ofstream file("config/test_invalid.conf");
        file << "server {\n";
        file << "    location /cgi-bin {\n";
        file << "        cgi_extension .py;\n";
        file << "    }\n";
        file << "}\n";
    }

    if (config.parse("config/test_invalid.conf"))
    {
        std::cerr << "FAIL: missing CGI interpreter should be rejected"
                  << std::endl;
        return 1;
    }
    std::cout << "PASS: Missing CGI interpreter rejected" << std::endl;


    std::cout << "\nTest 40: Listen with multiple colons..." << std::endl;
    {
        std::ofstream file("config/test_invalid.conf");
        file << "server {\n";
        file << "    listen 127.0.0.1:8080:1234;\n";
        file << "}\n";
    }

    if (config.parse("config/test_invalid.conf"))
    {
        std::cerr << "FAIL: multiple listen colons should be rejected"
                  << std::endl;
        return 1;
    }
    std::cout << "PASS: Multiple listen colons rejected" << std::endl;


    std::cout << "\nTest 41: Duplicate listen directive..." << std::endl;
    {
        std::ofstream file("config/test_invalid.conf");
        file << "server {\n";
        file << "    listen 127.0.0.1:8080;\n";
        file << "    listen 127.0.0.1:8081;\n";
        file << "}\n";
    }

    if (config.parse("config/test_invalid.conf"))
    {
        std::cerr << "FAIL: duplicate listen should be rejected"
                  << std::endl;
        return 1;
    }
    std::cout << "PASS: Duplicate listen rejected" << std::endl;


    std::cout << "\nTest 42: Duplicate root directive..." << std::endl;
    {
        std::ofstream file("config/test_invalid.conf");
        file << "server {\n";
        file << "    root ./www;\n";
        file << "    root ./www2;\n";
        file << "}\n";
    }

    if (config.parse("config/test_invalid.conf"))
    {
        std::cerr << "FAIL: duplicate root should be rejected"
                  << std::endl;
        return 1;
    }
    std::cout << "PASS: Duplicate root rejected" << std::endl;


	std::cout << "\nTest 43: Parse same Config object twice..." << std::endl;
	{
		Config repeatedConfig;

		if (!repeatedConfig.parse("config/test.conf"))
		{
			std::cerr << "FAIL: first parse failed" << std::endl;
			return 1;
		}

		if (!repeatedConfig.parse("config/test.conf"))
		{
			std::cerr << "FAIL: second parse failed" << std::endl;
			return 1;
		}

		if (repeatedConfig.getServers().size() != 2)
		{
			std::cerr << "FAIL: repeated parse accumulated servers, got "
					<< repeatedConfig.getServers().size()
					<< std::endl;
			return 1;
		}
	}

    std::cout << "PASS: Repeated parse does not duplicate servers"
              << std::endl;


    std::cout << "\nTest 44: Negative listen port..." << std::endl;
    {
        std::ofstream file("config/test_invalid.conf");
        file << "server {\n";
        file << "    listen 127.0.0.1:-1;\n";
        file << "}\n";
    }

    if (config.parse("config/test_invalid.conf"))
    {
        std::cerr << "FAIL: negative port should be rejected"
                  << std::endl;
        return 1;
    }
    std::cout << "PASS: Negative port rejected" << std::endl;


    std::cout << "\nTest 45: Zero listen port..." << std::endl;
    {
        std::ofstream file("config/test_invalid.conf");
        file << "server {\n";
        file << "    listen 127.0.0.1:0;\n";
        file << "}\n";
    }

    if (config.parse("config/test_invalid.conf"))
    {
        std::cerr << "FAIL: port 0 should be rejected"
                  << std::endl;
        return 1;
    }
    std::cout << "PASS: Port 0 rejected" << std::endl;


	std::cout << "\nTest 46: Empty CGI extension..." << std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    location /cgi-bin {\n";
		file << "        cgi_extension ; /usr/bin/python3;\n";
		file << "    }\n";
		file << "}\n";
	}

	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: empty CGI extension should be rejected"
				<< std::endl;
		return 1;
	}
	std::cout << "PASS: Empty CGI extension rejected" << std::endl;


    std::cout << "\nTest 47: Empty location path with spaces..."
              << std::endl;
    {
        std::ofstream file("config/test_invalid.conf");
        file << "server {\n";
        file << "    location    {\n";
        file << "    }\n";
        file << "}\n";
    }

    if (config.parse("config/test_invalid.conf"))
    {
        std::cerr << "FAIL: empty location path should be rejected"
                  << std::endl;
        return 1;
    }
    std::cout << "PASS: Empty location path rejected" << std::endl;


    std::cout << "\nTest 48: Invalid server directive syntax..." << std::endl;
    {
        std::ofstream file("config/test_invalid.conf");
        file << "server extra {\n";
        file << "    listen 127.0.0.1:8080;\n";
        file << "}\n";
    }

    if (config.parse("config/test_invalid.conf"))
    {
        std::cerr << "FAIL: invalid server syntax should be rejected"
                  << std::endl;
        return 1;
    }
    std::cout << "PASS: Invalid server syntax rejected" << std::endl;


    std::cout << "\nTest 49: Extra token after error_page..."
              << std::endl;
    {
        std::ofstream file("config/test_invalid.conf");
        file << "server {\n";
        file << "    error_page 404 /404.html extra;\n";
        file << "}\n";
    }

    if (config.parse("config/test_invalid.conf"))
    {
        std::cerr << "FAIL: extra error_page token should be rejected"
                  << std::endl;
        return 1;
    }
    std::cout << "PASS: Extra error_page token rejected" << std::endl;


    std::cout << "\nTest 50: Extra token after CGI extension..."
              << std::endl;
    {
        std::ofstream file("config/test_invalid.conf");
        file << "server {\n";
        file << "    location /cgi-bin {\n";
        file << "        cgi_extension .py /usr/bin/python3 extra;\n";
        file << "    }\n";
        file << "}\n";
    }

    if (config.parse("config/test_invalid.conf"))
    {
        std::cerr << "FAIL: extra CGI token should be rejected"
                  << std::endl;
        return 1;
    }
    std::cout << "PASS: Extra CGI token rejected" << std::endl;

	std::cout << "\nTest 51: Missing semicolon after index..."
			<< std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    index index.html\n";
		file << "}\n";
	}

	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: missing index semicolon should be rejected"
				<< std::endl;
		return 1;
	}
	std::cout << "PASS: Missing index semicolon rejected" << std::endl;

	std::cout << "\nTest 52: Extra token after index..."
			<< std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    index index.html extra;\n";
		file << "}\n";
	}

	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: extra index token should be rejected"
				<< std::endl;
		return 1;
	}
	std::cout << "PASS: Extra index token rejected" << std::endl;

	std::cout << "\nTest 53: Missing semicolon after autoindex..."
			<< std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    autoindex on\n";
		file << "}\n";
	}

	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: missing autoindex semicolon should be rejected"
				<< std::endl;
		return 1;
	}
	std::cout << "PASS: Missing autoindex semicolon rejected"
			<< std::endl;

	std::cout << "\nTest 54: Extra token after autoindex..."
			<< std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    autoindex on extra;\n";
		file << "}\n";
	}

	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: extra autoindex token should be rejected"
				<< std::endl;
		return 1;
	}
	std::cout << "PASS: Extra autoindex token rejected" << std::endl;

	std::cout << "\nTest 55: Missing semicolon after client_max_body_size..."
			<< std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    client_max_body_size 10M\n";
		file << "}\n";
	}

	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: missing client_max_body_size semicolon "
				<< "should be rejected"
				<< std::endl;
		return 1;
	}
	std::cout << "PASS: Missing client_max_body_size semicolon rejected"
			<< std::endl;

	std::cout << "\nTest 56: Extra token after client_max_body_size..."
			<< std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    client_max_body_size 10M extra;\n";
		file << "}\n";
	}

	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: extra client_max_body_size token "
				<< "should be rejected"
				<< std::endl;
		return 1;
	}
	std::cout << "PASS: Extra client_max_body_size token rejected"
			<< std::endl;

	std::cout << "\nTest 57: Overflow client_max_body_size..."
			<< std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    client_max_body_size 999999999999999999999999M;\n";
		file << "}\n";
	}

	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: overflowing client_max_body_size "
				<< "should be rejected"
				<< std::endl;
		return 1;
	}
	std::cout << "PASS: Overflow client_max_body_size rejected"
			<< std::endl;

	std::cout << "\nTest 58: Missing semicolon after allow_methods..."
			<< std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    location / {\n";
		file << "        allow_methods GET POST\n";
		file << "    }\n";
		file << "}\n";
	}

	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: missing allow_methods semicolon "
				<< "should be rejected"
				<< std::endl;
		return 1;
	}
	std::cout << "PASS: Missing allow_methods semicolon rejected"
			<< std::endl;

	std::cout << "\nTest 59: Invalid method in allow_methods..."
			<< std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    location / {\n";
		file << "        allow_methods GET BANANA;\n";
		file << "    }\n";
		file << "}\n";
	}

	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: invalid allow_methods value "
				<< "should be rejected"
				<< std::endl;
		return 1;
	}
	std::cout << "PASS: Invalid allow_methods value rejected"
			<< std::endl;

	std::cout << "\nTest 60: Missing semicolon after upload..."
			<< std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    location /upload {\n";
		file << "        upload on\n";
		file << "    }\n";
		file << "}\n";
	}

	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: missing upload semicolon should be rejected"
				<< std::endl;
		return 1;
	}
	std::cout << "PASS: Missing upload semicolon rejected"
			<< std::endl;

	std::cout << "\nTest 61: Extra token after upload..."
			<< std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    location /upload {\n";
		file << "        upload on extra;\n";
		file << "    }\n";
		file << "}\n";
	}

	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: extra upload token should be rejected"
				<< std::endl;
		return 1;
	}
	std::cout << "PASS: Extra upload token rejected" << std::endl;

	std::cout << "\nTest 62: Missing semicolon after upload_store..."
			<< std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    location /upload {\n";
		file << "        upload_store ./uploads\n";
		file << "    }\n";
		file << "}\n";
	}

	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: missing upload_store semicolon "
				<< "should be rejected"
				<< std::endl;
		return 1;
	}
	std::cout << "PASS: Missing upload_store semicolon rejected"
			<< std::endl;

	std::cout << "\nTest 63: Extra token after upload_store..."
			<< std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    location /upload {\n";
		file << "        upload_store ./uploads extra;\n";
		file << "    }\n";
		file << "}\n";
	}

	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: extra upload_store token "
				<< "should be rejected"
				<< std::endl;
		return 1;
	}
	std::cout << "PASS: Extra upload_store token rejected"
			<< std::endl;

	std::cout << "\nTest 64: Extra token after location opening brace..."
			<< std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    location / extra {\n";
		file << "    }\n";
		file << "}\n";
	}

	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: extra location token should be rejected"
				<< std::endl;
		return 1;
	}
	std::cout << "PASS: Extra location token rejected"
			<< std::endl;

	std::cout << "\nTest 65: Extra token after location opening brace..."
			<< std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    location / { extra\n";
		file << "    }\n";
		file << "}\n";
	}

	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: extra token after location '{' should be rejected"
				<< std::endl;
		return 1;
	}
	std::cout << "PASS: Extra token after location '{' rejected"
			<< std::endl;

	std::cout << "\nTest 66: Extra token after closing brace..."
			<< std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    location / {\n";
		file << "    } extra\n";
		file << "}\n";
	}

	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: extra token after '}' should be rejected"
				<< std::endl;
		return 1;
	}
	std::cout << "PASS: Extra token after '}' rejected" << std::endl;

	std::cout << "\nTest 67: Extra token after server closing brace..."
			<< std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "}\n";
		file << "extra\n";
	}

	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: directive after server block should be rejected"
				<< std::endl;
		return 1;
	}
	std::cout << "PASS: Token after server block rejected" << std::endl;

	std::cout << "\nTest 68: Extra token after index..."
			<< std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    index index.html extra;\n";
		file << "}\n";
	}

	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: extra index token should be rejected"
				<< std::endl;
		return 1;
	}
	std::cout << "PASS: Extra index token rejected" << std::endl;

	std::cout << "\nTest 69: Empty autoindex value..."
			<< std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    autoindex;\n";
		file << "}\n";
	}

	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: empty autoindex value should be rejected"
				<< std::endl;
		return 1;
	}
	std::cout << "PASS: Empty autoindex value rejected" << std::endl;

	std::cout << "\nTest 70: Empty upload value..."
			<< std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    location /upload {\n";
		file << "        upload;\n";
		file << "    }\n";
		file << "}\n";
	}

	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: empty upload value should be rejected"
				<< std::endl;
		return 1;
	}
	std::cout << "PASS: Empty upload value rejected" << std::endl;

	std::cout << "\nTest 71: Empty upload_store value..."
			<< std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    location /upload {\n";
		file << "        upload_store;\n";
		file << "    }\n";
		file << "}\n";

	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: empty upload_store value should be rejected"
				<< std::endl;
		return 1;
	}
	std::cout << "PASS: Empty upload_store value rejected" << std::endl;

	std::cout << "\nTest 72: Missing number in client_max_body_size..."
			<< std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    client_max_body_size M;\n";
		file << "}\n";
	}

	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: missing body size number should be rejected"
				<< std::endl;
		return 1;
	}
	std::cout << "PASS: Missing body size number rejected" << std::endl;

	std::cout << "\nTest 73: Invalid client_max_body_size suffix..."
			<< std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    client_max_body_size 10G;\n";
		file << "}\n";
	}

	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: invalid body size suffix should be rejected"
				<< std::endl;
		return 1;
	}
	std::cout << "PASS: Invalid body size suffix rejected" << std::endl;

	std::cout << "\nTest 74: Empty error_page path..."
			<< std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    error_page 404;\n";
		file << "}\n";
	}

	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: empty error_page path should be rejected"
				<< std::endl;
		return 1;
	}
	std::cout << "PASS: Empty error_page path rejected" << std::endl;

	std::cout << "\nTest 75: Empty CGI interpreter..."
			<< std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    location /cgi-bin {\n";
		file << "        cgi_extension .py ;\n";
		file << "    }\n";
		file << "}\n";
	}

	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: empty CGI interpreter should be rejected"
				<< std::endl;
		return 1;
	}
	std::cout << "PASS: Empty CGI interpreter rejected" << std::endl;

	std::cout << "\nTest 76: Empty listen value..." << std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    listen ;\n";
		file << "}\n";
	}
	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: empty listen value should be rejected" << std::endl;
		return 1;
	}
	std::cout << "PASS: Empty listen value rejected" << std::endl;


	std::cout << "\nTest 77: Empty root value with semicolon..." << std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    root ;\n";
		file << "}\n";
	}
	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: empty root value should be rejected" << std::endl;
		return 1;
	}
	std::cout << "PASS: Empty root value rejected" << std::endl;


	std::cout << "\nTest 78: Empty index value with semicolon..." << std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    index ;\n";
		file << "}\n";
	}
	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: empty index value should be rejected" << std::endl;
		return 1;
	}
	std::cout << "PASS: Empty index value rejected" << std::endl;


	std::cout << "\nTest 79: Empty error_page path..." << std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    error_page 404 ;\n";
		file << "}\n";
	}
	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: empty error_page path should be rejected" << std::endl;
		return 1;
	}
	std::cout << "PASS: Empty error_page path rejected" << std::endl;


	std::cout << "\nTest 80: Extra token after error_page..." << std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    error_page 404 /404.html extra;\n";
		file << "}\n";
	}
	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: extra error_page token should be rejected" << std::endl;
		return 1;
	}
	std::cout << "PASS: Extra error_page token rejected" << std::endl;


	std::cout << "\nTest 81: Extra token after client_max_body_size..." << std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    client_max_body_size 1K extra;\n";
		file << "}\n";
	}
	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: extra body size token should be rejected" << std::endl;
		return 1;
	}
	std::cout << "PASS: Extra body size token rejected" << std::endl;


	std::cout << "\nTest 82: Extra token after allow_methods..." << std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    location / {\n";
		file << "        allow_methods GET POST DELETE extra;\n";
		file << "    }\n";
		file << "}\n";
	}
	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: extra allow_methods token should be rejected" << std::endl;
		return 1;
	}
	std::cout << "PASS: Extra allow_methods token rejected" << std::endl;


	std::cout << "\nTest 83: Extra token after upload..." << std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    location /upload {\n";
		file << "        upload on extra;\n";
		file << "    }\n";
		file << "}\n";
	}
	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: extra upload token should be rejected" << std::endl;
		return 1;
	}
	std::cout << "PASS: Extra upload token rejected" << std::endl;


	std::cout << "\nTest 84: Extra token after upload_store..." << std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    location /upload {\n";
		file << "        upload_store ./uploads extra;\n";
		file << "    }\n";
		file << "}\n";
	}
	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: extra upload_store token should be rejected" << std::endl;
		return 1;
	}
	std::cout << "PASS: Extra upload_store token rejected" << std::endl;


	std::cout << "\nTest 85: Extra token after cgi_extension..." << std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    location /cgi-bin {\n";
		file << "        cgi_extension .py /usr/bin/python3 extra;\n";
		file << "    }\n";
		file << "}\n";
	}
	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: extra cgi_extension token should be rejected" << std::endl;
		return 1;
	}
	std::cout << "PASS: Extra cgi_extension token rejected" << std::endl;


	std::cout << "\nTest 86: Missing listen host..." << std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    listen :8080;\n";
		file << "}\n";
	}
	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: missing listen host should be rejected" << std::endl;
		return 1;
	}
	std::cout << "PASS: Missing listen host rejected" << std::endl;


	std::cout << "\nTest 87: Missing listen port..." << std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    listen 127.0.0.1:;\n";
		file << "}\n";
	}
	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: missing listen port should be rejected" << std::endl;
		return 1;
	}
	std::cout << "PASS: Missing listen port rejected" << std::endl;


	std::cout << "\nTest 88: Negative client_max_body_size..." << std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    client_max_body_size -1;\n";
		file << "}\n";
	}
	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: negative body size should be rejected" << std::endl;
		return 1;
	}
	std::cout << "PASS: Negative body size rejected" << std::endl;


	std::cout << "\nTest 89: Zero client_max_body_size..." << std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    client_max_body_size 0;\n";
		file << "}\n";
	}
	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: zero body size should be rejected" << std::endl;
		return 1;
	}
	std::cout << "PASS: Zero body size rejected" << std::endl;


	std::cout << "\nTest 90: Invalid error_page status 399..." << std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    error_page 399 /399.html;\n";
		file << "}\n";
	}
	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: status 399 should be rejected" << std::endl;
		return 1;
	}
	std::cout << "PASS: Status 399 rejected" << std::endl;


	std::cout << "\nTest 91: Invalid error_page status 600..." << std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    error_page 600 /600.html;\n";
		file << "}\n";
	}
	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: status 600 should be rejected" << std::endl;
		return 1;
	}
	std::cout << "PASS: Status 600 rejected" << std::endl;


	std::cout << "\nTest 92: Invalid allow_methods lowercase..." << std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    location / {\n";
		file << "        allow_methods get;\n";
		file << "    }\n";
		file << "}\n";
	}
	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: lowercase method should be rejected" << std::endl;
		return 1;
	}
	std::cout << "PASS: Lowercase method rejected" << std::endl;


	std::cout << "\nTest 93: Invalid upload value..." << std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    location /upload {\n";
		file << "        upload yes;\n";
		file << "    }\n";
		file << "}\n";
	}
	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: invalid upload value should be rejected" << std::endl;
		return 1;
	}
	std::cout << "PASS: Invalid upload value rejected" << std::endl;


	std::cout << "\nTest 94: Invalid autoindex value..." << std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    autoindex yes;\n";
		file << "}\n";
	}
	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: invalid autoindex value should be rejected" << std::endl;
		return 1;
	}
	std::cout << "PASS: Invalid autoindex value rejected" << std::endl;


	std::cout << "\nTest 95: Empty CGI extension..." << std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    location /cgi-bin {\n";
		file << "        cgi_extension ; /usr/bin/python3;\n";
		file << "    }\n";
		file << "}\n";
	}
	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: empty CGI extension should be rejected" << std::endl;
		return 1;
	}
	std::cout << "PASS: Empty CGI extension rejected" << std::endl;


	std::cout << "\nTest 96: CGI interpreter is semicolon only..." << std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    location /cgi-bin {\n";
		file << "        cgi_extension .py ;\n";
		file << "    }\n";
		file << "}\n";
	}
	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: empty CGI interpreter should be rejected" << std::endl;
		return 1;
	}
	std::cout << "PASS: Empty CGI interpreter rejected" << std::endl;


	std::cout << "\nTest 97: Location closing brace with extra token..." << std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    location / {\n";
		file << "    } extra\n";
		file << "}\n";
	}
	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: extra token after location '}' should be rejected" << std::endl;
		return 1;
	}
	std::cout << "PASS: Extra token after location '}' rejected" << std::endl;


	std::cout << "\nTest 98: Server closing brace with extra token..." << std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "} extra\n";
	}
	if (config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: extra token after server '}' should be rejected" << std::endl;
		return 1;
	}
	std::cout << "PASS: Extra token after server '}' rejected" << std::endl;


	std::cout << "\nTest 99: Multiple server blocks..." << std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    listen 127.0.0.1:8080;\n";
		file << "}\n";
		file << "server {\n";
		file << "    listen 127.0.0.1:8081;\n";
		file << "}\n";
	}
	if (!config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: multiple server blocks should be accepted" << std::endl;
		return 1;
	}
	if (config.getServers().size() != 2)
	{
		std::cerr << "FAIL: expected 2 server blocks" << std::endl;
		return 1;
	}
	std::cout << "PASS: Multiple server blocks accepted" << std::endl;


	std::cout << "\nTest 100: Multiple locations in one server..." << std::endl;
	{
		std::ofstream file("config/test_invalid.conf");
		file << "server {\n";
		file << "    listen 127.0.0.1:8080;\n";
		file << "    location / {\n";
		file << "        allow_methods GET;\n";
		file << "    }\n";
		file << "    location /api {\n";
		file << "        allow_methods GET POST;\n";
		file << "    }\n";
		file << "}\n";
	}
	if (!config.parse("config/test_invalid.conf"))
	{
		std::cerr << "FAIL: multiple locations should be accepted" << std::endl;
		return 1;
	}
	if (config.getServers().size() != 1)
	{
		std::cerr << "FAIL: expected 1 server block" << std::endl;
		return 1;
	}
	if (config.getServers()[0].getLocations().size() != 2)
	{
		std::cerr << "FAIL: expected 2 locations" << std::endl;
		return 1;
	}
	std::cout << "PASS: Multiple locations accepted" << std::endl;

	std::cout << "\nTest 101: Invalid syntax - Missing semicolon..." << std::endl;
    {
        std::ofstream file("config/test_invalid_syntax.conf");
        file << "server {\n";
        file << "    listen 8080\n"; // Noktalı virgül eksik
        file << "}\n";
    }
    if (config.parse("config/test_invalid_syntax.conf"))
    {
        std::cerr << "FAIL: Missing semicolon should return false" << std::endl;
        return 1;
    }
    else
    {
        passed++;
    }
    total++;
    std::cout << "PASS: Missing semicolon correctly rejected" << std::endl;

    std::cout << "\nTest 102: Invalid syntax - Unclosed block..." << std::endl;
    {
        std::ofstream file("config/test_unclosed.conf");
        file << "server {\n";
        file << "    listen 8080;\n";
        // Kapanış parantezi eksik
    }
    if (config.parse("config/test_unclosed.conf"))
    {
        std::cerr << "FAIL: Unclosed block should return false" << std::endl;
        return 1;
    }
    else
    {
        passed++;
    }
    total++;
    std::cout << "PASS: Unclosed block correctly rejected" << std::endl;

    std::cout << "\n=== ALL EXTENDED EDGE CASE TESTS PASSED ==="
              << std::endl;

	std::cout << "\n=== INVALID CONFIG TEST SUMMARY ==="
			<< std::endl;
	std::cout << "Passed: " << passed << "/" << total << std::endl;

	if (passed != total)
	{
		std::cerr << "SOME INVALID CONFIG TESTS FAILED"
				<< std::endl;
		return 1;
	}

	std::cout << "ALL INVALID CONFIG TESTS PASSED"
			<< std::endl;

	return 0;

}
}
