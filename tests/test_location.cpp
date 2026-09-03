#include "../include/Location.hpp"
#include <iostream>

int main()
{
	std::cout << "=== LOCATION TESTS ===" << std::endl;

	std::cout << "\nTest 1: Test default constructor..." << std::endl;
	Location loc1;
	if (loc1.getAutoIndex() != false || loc1.getUpload() != false)
	{
		std::cerr << "FAIL: Default constructor values incorrect" << std::endl;
		return 1;
	}
	std::cout << "PASS: Default constructor correct" << std::endl;

	std::cout << "\nTest 2: Test path constructor..." << std::endl;
	Location loc2("/api");
	if (loc2.getPath() != "/api")
	{
		std::cerr << "FAIL: Path constructor not setting path" << std::endl;
		return 1;
	}
	std::cout << "PASS: Path constructor correct" << std::endl;

	std::cout << "\nTest 3: Test setters and getters..." << std::endl;
	loc2.setRoot("/var/api");
	loc2.setIndex("index.php");
	loc2.setAutoIndex(true);
	loc2.setUpload(true);
	loc2.setUploadStore("/tmp/uploads");

	if (loc2.getRoot() != "/var/api" ||
	    loc2.getIndex() != "index.php" ||
	    loc2.getAutoIndex() != true ||
	    loc2.getUpload() != true ||
	    loc2.getUploadStore() != "/tmp/uploads")
	{
		std::cerr << "FAIL: Setters/getters not working correctly" << std::endl;
		return 1;
	}
	std::cout << "PASS: Setters/getters working" << std::endl;

	std::cout << "\nTest 4: Test autoindexSet flag..." << std::endl;
	Location loc3("/static");
	if (loc3.isAutoIndexSet() != false)
	{
		std::cerr << "FAIL: New location should have autoindexSet = false" << std::endl;
		return 1;
	}
	loc3.setAutoIndex(true);
	if (loc3.isAutoIndexSet() != true)
	{
		std::cerr << "FAIL: After setAutoIndex, isAutoIndexSet should be true" << std::endl;
		return 1;
	}
	std::cout << "PASS: autoindexSet flag working" << std::endl;

	std::cout << "\nTest 5: Test allowed methods..." << std::endl;
	Location loc4("/upload");

	if (!loc4.isMethodAllowed("GET") ||
	    !loc4.isMethodAllowed("POST") ||
	    !loc4.isMethodAllowed("DELETE"))
	{
		std::cerr << "FAIL: Empty methods list should allow all" << std::endl;
		return 1;
	}

	loc4.addMethod("GET");
	loc4.addMethod("POST");

	if (!loc4.isMethodAllowed("GET") ||
	    !loc4.isMethodAllowed("POST"))
	{
		std::cerr << "FAIL: Added methods should be allowed" << std::endl;
		return 1;
	}

	if (loc4.isMethodAllowed("DELETE"))
	{
		std::cerr << "FAIL: DELETE should not be allowed" << std::endl;
		return 1;
	}
	std::cout << "PASS: Method filtering working" << std::endl;

	std::cout << "\nTest 6: Test CGI extension mapping..." << std::endl;
	Location loc5("/cgi-bin");

	if (loc5.isCgiExtension(".py"))
	{
		std::cerr << "FAIL: No extension should be configured by default" << std::endl;
		return 1;
	}

	loc5.addCgiExtension(".py", "/usr/bin/python3");

	if (!loc5.isCgiExtension(".py") || loc5.getCgiInterpreter(".py") != "/usr/bin/python3")
	{
		std::cerr << "FAIL: Added CGI extension not recognized correctly" << std::endl;
		return 1;
	}

	if (loc5.isCgiExtension(".php") || loc5.getCgiInterpreter(".php") != "")
	{
		std::cerr << "FAIL: Unknown extension should not be recognized" << std::endl;
		return 1;
	}
	std::cout << "PASS: CGI extension mapping working" << std::endl;

	std::cout << "\n=== ALL LOCATION TESTS PASSED ===" << std::endl;
	return 0;
}
