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

	std::cout << "\nTest 7: Test default string values..." << std::endl;
	Location loc6;

	if (loc6.getPath() != "" ||
	    loc6.getRoot() != "" ||
	    loc6.getIndex() != "" ||
	    loc6.getUploadStore() != "")
	{
		std::cerr << "FAIL: Default string values should be empty" << std::endl;
		return 1;
	}
	std::cout << "PASS: Default string values correct" << std::endl;


	std::cout << "\nTest 8: Test setPath..." << std::endl;
	Location loc7;

	loc7.setPath("/new/path");

	if (loc7.getPath() != "/new/path")
	{
		std::cerr << "FAIL: setPath not working" << std::endl;
		return 1;
	}
	std::cout << "PASS: setPath working" << std::endl;


	std::cout << "\nTest 9: Test changing path..." << std::endl;
	loc7.setPath("/first");
	loc7.setPath("/second");

	if (loc7.getPath() != "/second")
	{
		std::cerr << "FAIL: setPath should replace previous path" << std::endl;
		return 1;
	}
	std::cout << "PASS: Path replacement working" << std::endl;


	std::cout << "\nTest 10: Test autoindex false..." << std::endl;
	Location loc8;

	loc8.setAutoIndex(true);
	loc8.setAutoIndex(false);

	if (loc8.getAutoIndex() != false ||
	    loc8.isAutoIndexSet() != true)
	{
		std::cerr << "FAIL: setAutoIndex(false) incorrect" << std::endl;
		return 1;
	}
	std::cout << "PASS: autoindex false working" << std::endl;


	std::cout << "\nTest 11: Test upload false..." << std::endl;
	Location loc9;

	loc9.setUpload(true);
	loc9.setUpload(false);

	if (loc9.getUpload() != false)
	{
		std::cerr << "FAIL: setUpload(false) incorrect" << std::endl;
		return 1;
	}
	std::cout << "PASS: upload false working" << std::endl;


	std::cout << "\nTest 12: Test upload store replacement..." << std::endl;
	Location loc10;

	loc10.setUploadStore("/tmp/one");
	loc10.setUploadStore("/tmp/two");

	if (loc10.getUploadStore() != "/tmp/two")
	{
		std::cerr << "FAIL: Upload store replacement incorrect" << std::endl;
		return 1;
	}
	std::cout << "PASS: Upload store replacement working" << std::endl;


	std::cout << "\nTest 13: Test empty allowed methods..." << std::endl;
	Location loc11("/");

	if (!loc11.isMethodAllowed("GET") ||
	    !loc11.isMethodAllowed("POST") ||
	    !loc11.isMethodAllowed("DELETE") ||
	    !loc11.isMethodAllowed("PATCH") ||
	    !loc11.isMethodAllowed("ANYTHING"))
	{
		std::cerr << "FAIL: Empty method list should allow any method" << std::endl;
		return 1;
	}
	std::cout << "PASS: Empty method list allows all methods" << std::endl;


	std::cout << "\nTest 14: Test method filtering..." << std::endl;
	Location loc12("/");

	loc12.addMethod("GET");

	if (!loc12.isMethodAllowed("GET"))
	{
		std::cerr << "FAIL: GET should be allowed" << std::endl;
		return 1;
	}

	if (loc12.isMethodAllowed("POST"))
	{
		std::cerr << "FAIL: POST should not be allowed" << std::endl;
		return 1;
	}

	if (loc12.isMethodAllowed("DELETE"))
	{
		std::cerr << "FAIL: DELETE should not be allowed" << std::endl;
		return 1;
	}

	std::cout << "PASS: Single method filtering working" << std::endl;


	std::cout << "\nTest 15: Test method case sensitivity..." << std::endl;
	Location loc13("/");

	loc13.addMethod("GET");

	if (loc13.isMethodAllowed("get"))
	{
		std::cerr << "FAIL: Method matching should be case-sensitive" << std::endl;
		return 1;
	}
	std::cout << "PASS: Method matching is case-sensitive" << std::endl;


	std::cout << "\nTest 16: Test multiple methods..." << std::endl;
	Location loc14("/");

	loc14.addMethod("GET");
	loc14.addMethod("POST");
	loc14.addMethod("DELETE");

	if (!loc14.isMethodAllowed("GET") ||
	    !loc14.isMethodAllowed("POST") ||
	    !loc14.isMethodAllowed("DELETE"))
	{
		std::cerr << "FAIL: Multiple methods not handled correctly" << std::endl;
		return 1;
	}
	std::cout << "PASS: Multiple methods working" << std::endl;


	std::cout << "\nTest 17: Test duplicate method..." << std::endl;
	Location loc15("/");

	loc15.addMethod("GET");
	loc15.addMethod("GET");

	if (!loc15.isMethodAllowed("GET"))
	{
		std::cerr << "FAIL: Duplicate method should remain allowed" << std::endl;
		return 1;
	}
	std::cout << "PASS: Duplicate method handled safely" << std::endl;


	std::cout << "\nTest 18: Test unknown method after filtering..." << std::endl;
	Location loc16("/");

	loc16.addMethod("GET");

	if (loc16.isMethodAllowed("OPTIONS"))
	{
		std::cerr << "FAIL: Unconfigured method should not be allowed" << std::endl;
		return 1;
	}
	std::cout << "PASS: Unknown method correctly rejected" << std::endl;


	std::cout << "\nTest 19: Test CGI default state..." << std::endl;
	Location loc17("/cgi");

	if (loc17.isCgiExtension(".py"))
	{
		std::cerr << "FAIL: CGI extension should not exist by default" << std::endl;
		return 1;
	}

	if (loc17.getCgiInterpreter(".py") != "")
	{
		std::cerr << "FAIL: Unknown CGI interpreter should be empty" << std::endl;
		return 1;
	}

	std::cout << "PASS: CGI default state correct" << std::endl;


	std::cout << "\nTest 20: Test CGI mapping..." << std::endl;
	Location loc18("/cgi");

	loc18.addCgiExtension(".py", "/usr/bin/python3");
	loc18.addCgiExtension(".php", "/usr/bin/php");

	if (!loc18.isCgiExtension(".py") ||
	    !loc18.isCgiExtension(".php"))
	{
		std::cerr << "FAIL: CGI extensions not registered" << std::endl;
		return 1;
	}

	if (loc18.getCgiInterpreter(".py") != "/usr/bin/python3" ||
	    loc18.getCgiInterpreter(".php") != "/usr/bin/php")
	{
		std::cerr << "FAIL: CGI interpreter mapping incorrect" << std::endl;
		return 1;
	}

	std::cout << "PASS: Multiple CGI mappings working" << std::endl;


	std::cout << "\nTest 21: Test CGI overwrite..." << std::endl;
	Location loc19("/cgi");

	loc19.addCgiExtension(".py", "/old/python");
	loc19.addCgiExtension(".py", "/new/python");

	if (loc19.getCgiInterpreter(".py") != "/new/python")
	{
		std::cerr << "FAIL: CGI mapping should be replaceable" << std::endl;
		return 1;
	}
	std::cout << "PASS: CGI mapping overwrite working" << std::endl;


	std::cout << "\nTest 22: Test unknown CGI extension..." << std::endl;
	Location loc20("/cgi");

	loc20.addCgiExtension(".py", "/usr/bin/python3");

	if (loc20.isCgiExtension(".pl"))
	{
		std::cerr << "FAIL: Unknown CGI extension should return false" << std::endl;
		return 1;
	}

	if (loc20.getCgiInterpreter(".pl") != "")
	{
		std::cerr << "FAIL: Unknown CGI interpreter should be empty" << std::endl;
		return 1;
	}

	std::cout << "PASS: Unknown CGI extension handled correctly" << std::endl;


	std::cout << "\nTest 23: Test CGI extension case sensitivity..." << std::endl;
	Location loc21("/cgi");

	loc21.addCgiExtension(".py", "/usr/bin/python3");

	if (loc21.isCgiExtension(".PY"))
	{
		std::cerr << "FAIL: CGI extension matching should be case-sensitive" << std::endl;
		return 1;
	}

	std::cout << "PASS: CGI extension matching is case-sensitive" << std::endl;


	std::cout << "\nTest 24: Test empty CGI extension..." << std::endl;
	Location loc22("/cgi");

	loc22.addCgiExtension("", "/usr/bin/python3");

	if (!loc22.isCgiExtension(""))
	{
		std::cerr << "FAIL: Empty CGI extension was not stored" << std::endl;
		return 1;
	}

	if (loc22.getCgiInterpreter("") != "/usr/bin/python3")
	{
		std::cerr << "FAIL: Empty CGI extension mapping incorrect" << std::endl;
		return 1;
	}

	std::cout << "PASS: Empty CGI extension mapping works" << std::endl;


	std::cout << "\nTest 25: Test empty CGI interpreter..." << std::endl;
	Location loc23("/cgi");

	loc23.addCgiExtension(".py", "");

	if (!loc23.isCgiExtension(".py"))
	{
		std::cerr << "FAIL: Extension should still be registered" << std::endl;
		return 1;
	}

	if (loc23.getCgiInterpreter(".py") != "")
	{
		std::cerr << "FAIL: Empty interpreter should remain empty" << std::endl;
		return 1;
	}

	std::cout << "PASS: Empty CGI interpreter handled" << std::endl;


	std::cout << "\nTest 26: Test const access..." << std::endl;
	const Location constLoc("/const");

	if (constLoc.getPath() != "/const" ||
	    constLoc.getRoot() != "" ||
	    constLoc.getIndex() != "" ||
	    constLoc.getAutoIndex() != false ||
	    constLoc.getUpload() != false ||
	    constLoc.getUploadStore() != "")
	{
		std::cerr << "FAIL: Const getter access incorrect" << std::endl;
		return 1;
	}

	std::cout << "PASS: Const getter access working" << std::endl;


	std::cout << "\nTest 27: Test path with nested structure..." << std::endl;
	Location loc24("/api/v1/users");

	if (loc24.getPath() != "/api/v1/users")
	{
		std::cerr << "FAIL: Nested location path incorrect" << std::endl;
		return 1;
	}

	std::cout << "PASS: Nested location path working" << std::endl;


	std::cout << "\nTest 28: Test independent Location objects..." << std::endl;
	Location first("/first");
	Location second("/second");

	first.setRoot("/root1");
	second.setRoot("/root2");

	first.setAutoIndex(true);
	second.setUpload(true);

	if (first.getPath() != "/first" ||
	    first.getRoot() != "/root1" ||
	    first.getAutoIndex() != true ||
	    first.getUpload() != false)
	{
		std::cerr << "FAIL: First Location state corrupted" << std::endl;
		return 1;
	}

	if (second.getPath() != "/second" ||
	    second.getRoot() != "/root2" ||
	    second.getAutoIndex() != false ||
	    second.getUpload() != true)
	{
		std::cerr << "FAIL: Second Location state corrupted" << std::endl;
		return 1;
	}

	std::cout << "PASS: Location objects are independent" << std::endl;


	std::cout << "\n=== ALL EXTENDED LOCATION TESTS PASSED ===" << std::endl;

	std::cout << "\n=== ALL LOCATION TESTS PASSED ===" << std::endl;
	return 0;
}
