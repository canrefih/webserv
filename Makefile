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

OBJS = $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)
	rm -f tests/*.o 
	rm -f tests/test_config_parsing tests/test_serverconfig tests/test_location tests/test_http_request tests/test_http_response tests/test_server_integration

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re