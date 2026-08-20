#ifndef SERVER_HPP
#define SERVER_HPP

#include "Config.hpp"
#include "ServerConfig.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

#include <vector>
#include <map>
#include <string>
#include <poll.h>

class Server
{
	private:
		const Config		&_config;
		std::vector<int>	_listenFds;
		std::vector<pollfd>	_pollFds;
		std::map<int, std::string> _clientBuffers;
		std::map<int, std::string> _clientWriteBuffers;
		std::map<int, const ServerConfig *> _clientServers;

		std::string readFile(const std::string &path);
		std::string getContentType(const std::string &path);
		std::string generateDirectoryListing(
			const std::string &path,
			const std::string &url
		);

		void createSockets();
		void setNonBlocking(int fd);
		void bindAndListenSocket(int fd, const ServerConfig &serverConfig);

		void addPollFd(int fd, short events);
		void acceptClient(int listenFd, const ServerConfig &serverConfig);
		void handleClientRead(std::size_t index);
		void removeClient(std::size_t index);
		void handleClientWrite(std::size_t index);

		bool fileExists(const std::string &path);
		bool isDirectory(const std::string &path);

		void setErrorResponse(
				const ServerConfig &serverConfig,
				HttpResponse &response,
				int statusCode,
				const std::string &statusText,
				const std::string &defaultBody
		);

	public:
		Server(const Config &config);
		~Server();

		void run();
};

#endif