NAME = webserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98
CPPFLAGS = -Iinclude

SRCS = src/main.cpp \
       src/Config.cpp \
       src/Server.cpp \
       src/ServerConfig.cpp \
       src/HttpRequest.cpp \
       src/HttpResponse.cpp \
       src/Location.cpp \
       src/RequestHandler.cpp \
       src/Signal.cpp \
       src/CGIHandler.cpp \
       src/CGIEnvBuilder.cpp \
       src/URL.cpp

TESTS_SRCS = tests/test_config_parsing.cpp\
		tests/test_http_request.cpp\
		tests/test_http_response.cpp\
		tests/test_location.cpp\
		tests/test_server_integration.cpp\
		tests/test_serverconfig.cpp\
		tests/test_cgi.cpp\
		tests/test_keep_alive.cpp\
		tests/test_multi_socket.cpp\
		tests/test_signal_handling.cpp\
		tests/test_timeout_protection.cpp\
		tests/test_url.cpp

OBJS = $(SRCS:src/%.cpp=obj/%.o)
TESTS = $(TESTS_SRCS:tests/%.cpp=tests/bin/%.out)
TESTS_OBJS = $(filter-out obj/main.o,$(OBJS))

all: $(NAME)
debug:
	@echo $(SRCS)
	@echo
	@echo $(OBJS)
	@echo
	@echo $(TESTS)
	@echo

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

tests: $(TESTS_OBJS) $(TESTS)

obj/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

run_tests: $(TESTS_OBJS) $(TESTS)
	@TESTS_BINARIES="$(TESTS)" ./tests/run_all_tests.sh

tests/bin/%.out: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(TESTS_OBJS) $< -o $@

test_cgi: tests/bin/test_cgi.out
	./tests/bin/test_cgi.out

clean:
	rm -rf obj
	rm -rf tests/bin

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re test_cgi tests
