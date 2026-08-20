#!/bin/bash

echo "Compiling tests..."
echo ""

# Test 1
echo -n "test_config_parsing... "
g++ -Wall -Wextra -Werror -std=c++98 -I../include -c test_config_parsing.cpp -o test_config_parsing.o 2>/dev/null
g++ -std=c++98 test_config_parsing.o ../src/Config.o ../src/ServerConfig.o ../src/Location.o -o test_config_parsing 2>/dev/null
if [ -f test_config_parsing ]; then echo "OK"; else echo "FAIL"; fi

# Test 2
echo -n "test_serverconfig... "
g++ -Wall -Wextra -Werror -std=c++98 -I../include -c test_serverconfig.cpp -o test_serverconfig.o 2>/dev/null
g++ -std=c++98 test_serverconfig.o ../src/ServerConfig.o ../src/Location.o -o test_serverconfig 2>/dev/null
if [ -f test_serverconfig ]; then echo "OK"; else echo "FAIL"; fi

# Test 3
echo -n "test_location... "
g++ -Wall -Wextra -Werror -std=c++98 -I../include -c test_location.cpp -o test_location.o 2>/dev/null
g++ -std=c++98 test_location.o ../src/Location.o -o test_location 2>/dev/null
if [ -f test_location ]; then echo "OK"; else echo "FAIL"; fi

# Test 4
echo -n "test_http_request... "
g++ -Wall -Wextra -Werror -std=c++98 -I../include -c test_http_request.cpp -o test_http_request.o 2>/dev/null
g++ -std=c++98 test_http_request.o ../src/HttpRequest.o -o test_http_request 2>/dev/null
if [ -f test_http_request ]; then echo "OK"; else echo "FAIL"; fi

# Test 5
echo -n "test_http_response... "
g++ -Wall -Wextra -Werror -std=c++98 -I../include -c test_http_response.cpp -o test_http_response.o 2>/dev/null
g++ -std=c++98 test_http_response.o ../src/HttpResponse.o -o test_http_response 2>/dev/null
if [ -f test_http_response ]; then echo "OK"; else echo "FAIL"; fi

# Test 6
echo -n "test_server_integration... "
g++ -Wall -Wextra -Werror -std=c++98 -I../include -c test_server_integration.cpp -o test_server_integration.o 2>/dev/null
g++ -std=c++98 test_server_integration.o ../src/Config.o ../src/ServerConfig.o ../src/Location.o ../src/Server.o ../src/HttpRequest.o ../src/HttpResponse.o -o test_server_integration 2>/dev/null
if [ -f test_server_integration ]; then echo "OK"; else echo "FAIL"; fi

echo ""
echo "Compilation complete. Run: ./run_all_tests.sh"
