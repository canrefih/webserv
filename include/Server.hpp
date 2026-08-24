#ifndef SERVER_HPP
#define SERVER_HPP

#include "Config.hpp"
#include "ServerConfig.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "RequestHandler.hpp"

#include <vector>
#include <map>
#include <string>
#include <poll.h>
#include <csignal>

/*
socket()
   ↓
bind()
   ↓
listen()
   ↓
poll()
   ↓
accept()
   ↓
recv()
   ↓
process the request
   ↓
send()

Server class manages the lifecycle of the server, including socket creation, binding, listening, and handling client connections.
It uses poll() to monitor multiple file descriptors for incoming connections and data.
The server reads HTTP requests from clients, processes them using RequestHandler, and sends back HTTP responses.
It also handles graceful shutdown on receiving termination signals.
*/

class Server
{
	private:
		const Config		&_config;
		std::vector<int>	_listenFds;
		std::vector<pollfd>	_pollFds;
		std::map<int, std::string> _clientBuffers; // Maps client socket file descriptors to their corresponding request buffers
		std::map<int, std::string> _clientWriteBuffers; // Maps client socket file descriptors to their corresponding write buffers
		std::map<int, const ServerConfig *> _clientServers; // Maps client socket file descriptors to their corresponding server configurations
		std::map<int, bool> _clientKeepAlive; // Maps client socket file descriptors to their corresponding keep-alive status

		void createSockets();
		void setNonBlocking(int fd);
		void bindAndListenSocket(int fd, const ServerConfig &serverConfig);

		void addPollFd(int fd, short events);
		void acceptClient(int listenFd, const ServerConfig &serverConfig);
		void handleClientRead(std::size_t index);
		void removeClient(std::size_t index);
		void handleClientWrite(std::size_t index);

	public:
		Server(const Config &config);
		~Server();

		void run();
};

#endif