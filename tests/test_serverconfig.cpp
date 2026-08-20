#include "../include/ServerConfig.hpp"
#include <iostream>

int main()
{
	std::cout << "=== SERVERCONFIG TESTS ===" << std::endl;
	
	ServerConfig server;
	
	std::cout << "\nTest 1: Check default values..." << std::endl;
	if (server.getHost() != "127.0.0.1" ||
	    server.getPort() != 8080 ||
	    server.getRoot() != "./www" ||
	    server.getIndex() != "index.html" ||
	    server.getAutoIndex() != false)
	{
		std::cerr << "FAIL: Default values incorrect" << std::endl;
		return 1;
	}
	std::cout << "PASS: All default values correct" << std::endl;
	
	std::cout << "\nTest 2: Test setters..." << std::endl;
	server.setHost("0.0.0.0");
	server.setPort(3000);
	server.setRoot("/var/www");
	server.setIndex("home.html");
	server.setAutoIndex(true);
	server.setClientMaxBodySize(10485760);
	
	if (server.getHost() != "0.0.0.0" ||
	    server.getPort() != 3000 ||
	    server.getRoot() != "/var/www" ||
	    server.getIndex() != "home.html" ||
	    server.getAutoIndex() != true ||
	    server.getClientMaxBodySize() != 10485760)
	{
		std::cerr << "FAIL: Setters not working correctly" << std::endl;
		return 1;
	}
	std::cout << "PASS: All setters working" << std::endl;
	
	std::cout << "\nTest 3: Test error page mapping..." << std::endl;
	server.setErrorPage(404, "/404.html");
	server.setErrorPage(500, "/500.html");
	
	const std::string *page404 = server.getErrorPage(404);
	const std::string *page500 = server.getErrorPage(500);
	
	if (page404 == NULL || *page404 != "/404.html" ||
	    page500 == NULL || *page500 != "/500.html")
	{
		std::cerr << "FAIL: Error page mapping failed" << std::endl;
		return 1;
	}
	
	const std::string *page403 = server.getErrorPage(403);
	if (page403 != NULL)
	{
		std::cerr << "FAIL: Non-existent error page should return NULL" << std::endl;
		return 1;
	}
	std::cout << "PASS: Error page mapping working" << std::endl;
	
	std::cout << "\nTest 4: Test location handling..." << std::endl;
	Location loc1("/api");
	loc1.setRoot("/api/root");
	loc1.addMethod("GET");
	loc1.addMethod("POST");
	
	Location loc2("/static");
	loc2.setRoot("/static/root");
	loc2.addMethod("GET");
	
	server.addLocation(loc1);
	server.addLocation(loc2);
	
	const std::vector<Location> &locations = server.getLocations();
	if (locations.size() != 2)
	{
		std::cerr << "FAIL: Expected 2 locations, got " << locations.size() << std::endl;
		return 1;
	}
	std::cout << "PASS: Locations added correctly" << std::endl;
	
	std::cout << "\nTest 5: Test findLocation..." << std::endl;
	const Location *found = server.findLocation("/api/test");
	if (found == NULL || found->getPath() != "/api")
	{
		std::cerr << "FAIL: findLocation failed for /api/test" << std::endl;
		return 1;
	}
	std::cout << "PASS: findLocation working correctly" << std::endl;
	
	std::cout << "\nTest 6: Test findLocation with root path..." << std::endl;
	found = server.findLocation("/");
	if (found != NULL)
	{
		std::cerr << "FAIL: Root path should not match any location" << std::endl;
		return 1;
	}
	std::cout << "PASS: Root path handling correct" << std::endl;
	
	std::cout << "\n=== ALL SERVERCONFIG TESTS PASSED ===" << std::endl;
	return 0;
}
