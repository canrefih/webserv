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
       src/Location.cpp

TESTS_SRCS = tests/test_config_parsing.cpp\
		tests/test_http_request.cpp\
		tests/test_http_response.cpp\
		tests/test_location.cpp\
		tests/test_server_integration.cpp\
		tests/test_serverconfig.cpp\

CGI_TEST_SRCS = cgi/CGIHandler.cpp \
		cgi/CGIEnvBuilder.cpp \
		src/HttpRequest.cpp \
		tests/test_cgi.cpp

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

obj/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

run_tests: $(TESTS_OBJS) $(TESTS)
	@TESTS_BINARIES="$(TESTS)" ./tests/run_all_tests.sh

tests/bin/%.out: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(TESTS_OBJS) $< -o $@

test_cgi: $(CGI_TEST_SRCS)
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(CGI_TEST_SRCS) -o tests/bin/test_cgi.out
	./tests/bin/test_cgi.out

clean:
	rm -rf obj
	rm -rf tests/bin

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re test_cgi
