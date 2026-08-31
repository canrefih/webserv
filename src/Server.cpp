#include "Server.hpp"
#include "Signal.hpp"

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <sstream>
#include <csignal>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <dirent.h> // For directory operations
#include <sys/socket.h> // For socket functions
#include <netdb.h> // For getaddrinfo
#include <netinet/in.h> // For sockaddr_in

Server::Server(const Config &config)
	: _config(config)
{
}

Server::~Server() // Destructor: Close all listening sockets
{
	std::vector<int>::iterator it;
	for (it = _listenFds.begin(); it != _listenFds.end(); ++it)
	{
		if (*it != -1)
			close(*it);
	}
}

void Server::setNonBlocking(int fd) // Set a socket to non-blocking mode
{
	int flags = fcntl(fd, F_GETFL, 0); // Get the current file status flags for the socket

	if (flags == -1)
	{
		std::cerr << "Error: fcntl(F_GETFL) failed: "
				  << strerror(errno) << std::endl;
		close(fd);
		std::exit(1);
	}
	/*
		To not block the server when accepting connections or reading/writing data, we set the socket to non-blocking mode.
		This allows the server to continue processing other sockets even if one socket is not ready for I/O.
	*/
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		std::cerr << "Error: fcntl(F_SETFL) failed: "
				  << strerror(errno) << std::endl;
		close(fd);
		std::exit(1);
	}
}

/*
fd 3 → listening socket
fd 4 → client A
fd 5 → client B
fd 6 → client C
Listening socket checks for incoming connections (accept)
Client sockets check for incoming data (read) or readiness to send data (write)
*/
// SOCKET CREATION
void Server::createSockets() // Create listening sockets for each server configuration
{
	const std::vector<ServerConfig> &servers = _config.getServers();

	if (servers.empty())
	{
		std::cerr << "Error: no servers configured" << std::endl;
		std::exit(1);
	}

	std::vector<ServerConfig>::const_iterator it;

	for (it = servers.begin(); it != servers.end(); ++it) // for each server configuration, create a socket
	{
		int fd = socket(AF_INET, SOCK_STREAM, 0); // Create a TCP socket

		if (fd == -1)
		{
			std::cerr << "Error: socket() failed: "
					  << strerror(errno) << std::endl;
			std::exit(1);
		}

		int opt = 1;

		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
					   &opt, sizeof(opt)) == -1) // Set socket option to reuse address
		{
			std::cerr << "Error: setsockopt() failed: "
					  << strerror(errno) << std::endl;
			close(fd);
			std::exit(1);
		}

		setNonBlocking(fd); // Set the socket to non-blocking mode
		_listenFds.push_back(fd); // Add the socket file descriptor to the list of listening sockets
	}
}

// BIND AND LISTEN
void Server::bindAndListenSocket(int fd, const ServerConfig &serverConfig) // Bind the socket to the specified host and port, and start listening for incoming connections
{
	struct addrinfo hints;
	struct addrinfo *result = NULL;

	std::memset(&hints, 0, sizeof(hints)); // Clear the hints structure to zero before using it

	// Set up the hints for getaddrinfo to specify the desired socket type and address family
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	std::ostringstream portStream;
	portStream << serverConfig.getPort(); // Convert the port number to a string for getaddrinfo

	std::string port = portStream.str(); // Get the port number as a string

	int status = getaddrinfo(
		serverConfig.getHost().c_str(),
		port.c_str(),
		&hints,
		&result
	); // Get address information for the specified host and port, using the hints to specify the desired socket type and address family

	if (status != 0)
	{
		std::cerr << "Error: getaddrinfo() failed: "
				  << gai_strerror(status) << std::endl;
		close(fd);
		std::exit(1);
	}

	if (bind(fd, result->ai_addr, result->ai_addrlen) == -1) // Bind the socket to the specified address and port
	{
		std::cerr << "Error: bind() failed on "
				  << serverConfig.getHost() << ":"
				  << serverConfig.getPort() << ": "
				  << strerror(errno) << std::endl;
		freeaddrinfo(result);
		close(fd);
		std::exit(1);
	}

	freeaddrinfo(result);

	if (listen(fd, SOMAXCONN) == -1) // Start listening for incoming connections on the socket, with a maximum backlog of SOMAXCONN. SOMAXCONN is a constant that specifies the maximum number of pending connections that can be queued for acceptance. It is defined in the system headers and represents the maximum value allowed by the operating system for the listen backlog.
	{
		std::cerr << "Error: listen() failed: "
				  << strerror(errno) << std::endl;
		close(fd);
		std::exit(1);
	}
}

/*
POLLIN: There is data to read on the socket
POLLOUT: Writing is now possible on the socket
POLLERR: An error occurred on the socket
request
	↓
┌────────┐
│POLLIN  │
└───┬────┘
	│
	▼
request ready
	│
	▼
response ready
	│
	▼
┌─────────┐
│ POLLOUT │
└───┬─────┘
	│
response done
	│
	▼
┌─────────┐
│ POLLIN  │
└─────────┘
*/
void Server::run() // Main server loop: Poll for events on listening and client sockets
{
	createSockets(); // Create listening sockets for each server configuration

	const std::vector<ServerConfig> &servers = _config.getServers();
	std::vector<int>::iterator it;
	std::size_t idx = 0;

	for (it = _listenFds.begin(); it != _listenFds.end(); ++it, ++idx)
	{
		bindAndListenSocket(*it, servers[idx]); // Bind and listen on the socket for the corresponding server configuration
		addPollFd(*it, POLLIN); // Add the listening socket to the poll file descriptor list for monitoring incoming connections

		std::cout << "Listening on "
				  << servers[idx].getHost()
				  << ":"
				  << servers[idx].getPort()
				  << std::endl;
	}

	std::cout << "Server started" << std::endl;

	while (g_serverRunning) // Main server loop: Poll for events on listening and client sockets
	{
		int ready = poll(
			&_pollFds[0],
			_pollFds.size(),
			1000  // 1 second timeout to allow checking g_serverRunning
		);

		if (ready == -1)
		{
			if (errno == EINTR)
				continue;  // Signal interrupted, check g_serverRunning again
			
			std::cerr << "Error: poll() failed: "
					  << strerror(errno) << std::endl;
			return;
		}

		if (ready == 0)
			continue;  // Timeout, again check for g_serverRunning

		std::size_t i = 0;

		while (i < _pollFds.size()) // Iterate through the poll file descriptors
		{
			bool isListenSocket = false;
			const ServerConfig *serverConfig = NULL;

			std::vector<int>::const_iterator listenIt;

			for (listenIt = _listenFds.begin(); listenIt != _listenFds.end(); ++listenIt)
			{
				if (_pollFds[i].fd == *listenIt) // Check if the current pollfd is a listening socket
				{
					isListenSocket = true;
					std::size_t serverIdx = listenIt - _listenFds.begin();
					serverConfig = &servers[serverIdx];
					break;
				}
			}

			if (isListenSocket) // If it's a listening socket, accept new client connections
			{
				if (_pollFds[i].revents & POLLIN)
					acceptClient(_pollFds[i].fd, *serverConfig);

				++i;
				continue;
			}

			bool removed = false;

			if (_pollFds[i].revents & POLLIN) // If the client socket is ready for reading, handle the read event
			{
				std::size_t oldSize = _pollFds.size();

				handleClientRead(i);

				if (_pollFds.size() != oldSize)
					removed = true;
			}

			if (!removed && (_pollFds[i].revents & POLLOUT)) // If the client socket is ready for writing, handle the write event
			{
				std::size_t oldSize = _pollFds.size();

				handleClientWrite(i);

				if (_pollFds.size() != oldSize)
					removed = true;
			}

			if (!removed) // If the client socket was not removed, move to the next pollfd
				++i;
		}
	}

	// Close the listening sockets and client connections when the server is shutting down
	std::cout << "Shutting down..." << std::endl;

	std::vector<int>::iterator listenIt;
	for (listenIt = _listenFds.begin(); listenIt != _listenFds.end(); ++listenIt)
	{
		if (*listenIt != -1)
			close(*listenIt);
	}

	std::map<int, std::string>::iterator bufferIt;
	for (bufferIt = _clientBuffers.begin(); bufferIt != _clientBuffers.end(); ++bufferIt)
	{
		if (bufferIt->first != -1)
			close(bufferIt->first);
	}

	std::cout << "Server stopped" << std::endl;
}

void Server::addPollFd(int fd, short events) // Add a file descriptor to the poll list with specified events (POLLIN, POLLOUT, etc.)
{
	struct pollfd pfd;

	pfd.fd = fd;
	pfd.events = events;
	pfd.revents = 0;

	_pollFds.push_back(pfd);
}

// ACCEPT CLIENT
void Server::acceptClient(int listenFd, const ServerConfig &serverConfig)
{
	struct sockaddr_storage clientAddress; // Store the address of the connecting client
	socklen_t clientAddressSize = sizeof(clientAddress); // Size of the client address structure

	int clientFd = accept(
		listenFd,
		reinterpret_cast<struct sockaddr *>(&clientAddress),
		&clientAddressSize
	); // Accept a new client connection on the listening socket, returning a new socket file descriptor for the client connection

	if (clientFd == -1)
		return;

	setNonBlocking(clientFd); // Set the new client socket to non-blocking mode to avoid blocking the server when reading/writing data

	addPollFd(clientFd, POLLIN); // Add the new client socket to the poll list, monitoring for incoming data (POLLIN)
	_clientBuffers[clientFd] = ""; // Initialize the buffer for the new client socket to store incoming request data
	_clientServers[clientFd] = &serverConfig; // Associate the new client socket with the corresponding server configuration, allowing the server to handle requests based on the specific server settings
	_clientKeepAlive[clientFd] = true; // Initialize the keep-alive status for the new client socket
	std::cout << "New client connected: fd="
			  << clientFd << " on "
			  << serverConfig.getHost() << ":"
			  << serverConfig.getPort() << std::endl;
}

// HANDLE CLIENT READ
void Server::handleClientRead(std::size_t index)
{
	char buffer[4096]; // Buffer to read incoming data from the client socket
	int clientFd = _pollFds[index].fd;
	ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer), 0); // Read data from the client socket into the buffer, returning the number of bytes read

	if (bytesRead == 0)
	{
		removeClient(index);
		return;
	}

	if (bytesRead < 0)
	{
		removeClient(index);
		return;
	}

	_clientBuffers[clientFd].append(buffer, bytesRead); // Append the received data to the client's buffer, allowing the server to accumulate the request data until a complete request is received
	std::string &requestBuffer = _clientBuffers[clientFd]; // Reference to the client's buffer for easier access and manipulation
	std::size_t headerEnd = requestBuffer.find("\r\n\r\n"); // Find the end of the HTTP headers in the request buffer, which is indicated by a double CRLF ("\r\n\r\n"). If not found, the request is incomplete and we wait for more data.

	if (headerEnd == std::string::npos) // If the end of the headers is not found, the request is incomplete, so we return and wait for more data to arrive
		return;

	HttpRequest request;

	if (!request.parse(requestBuffer)) // If the request parsing fails, we set an error response indicating a bad request (400) and prepare to send it back to the client
	{
		HttpResponse response;
		response.setStatus(400, "Bad Request");
		response.setBody("Bad Request\n");
		response.setContentType("text/plain");

		_clientWriteBuffers[clientFd] = response.toString();
		_pollFds[index].events = POLLOUT;
		requestBuffer.clear();
		return;
	}

	std::string contentLength = request.getHeader("Content-Length");
	const ServerConfig *serverConfig = _clientServers[clientFd];

	/*
		If the request has a Content-Length header, we check if the body size exceeds the maximum allowed size for the server configuration.
		If it does, we set an error response indicating that the payload is too large (413) and prepare to send it back to the client.
		If the body is not fully received yet, we return and wait for more data to arrive.
	*/
	if (!contentLength.empty())
	{
		std::size_t expectedBodyLength = std::atoi(contentLength.c_str());

		if (expectedBodyLength > serverConfig->getClientMaxBodySize())
		{
			HttpResponse response;
			response.setStatus(413, "Payload Too Large");
			response.setBody("Payload Too Large\n");
			response.setContentType("text/plain");

			_clientWriteBuffers[clientFd] = response.toString();
			_pollFds[index].events = POLLOUT;
			requestBuffer.clear();
			return;
		}

		std::size_t bodyStart = headerEnd + 4;
		std::size_t actualBodyLength = requestBuffer.size() - bodyStart;

		if (actualBodyLength < expectedBodyLength)
			return;
	}

	std::cout << "Method:  " << request.getMethod() << std::endl;
	std::cout << "Target:  " << request.getTarget().getPath() << std::endl;
	std::cout << "Version: " << request.getVersion() << std::endl;

	// Handle the request using the RequestHandler and prepare the response to be sent back to the client
	HttpResponse response;
	RequestHandler handler(*serverConfig);
	handler.handleRequest(request, response);

	std::string connection = request.getHeader("Connection");

	if (connection == "close")
	{
		_clientKeepAlive[clientFd] = false;
		response.setHeader("Connection", "close");
	}
	else
	{
		_clientKeepAlive[clientFd] = true;
		response.setHeader("Connection", "keep-alive");
	}

	_clientWriteBuffers[clientFd] = response.toString();
	_pollFds[index].events = POLLOUT; // Switch the poll events for the client socket to POLLOUT, indicating that we are now ready to send data back to the client
	requestBuffer.clear();
}

// HANDLE CLIENT WRITE
void Server::handleClientWrite(std::size_t index)
{
	int clientFd = _pollFds[index].fd;
	std::string &response = _clientWriteBuffers[clientFd];

	if (response.empty())
	{
		if (_clientKeepAlive[clientFd])
		{
			_pollFds[index].events = POLLIN;
		}
		else
		{
			removeClient(index);
		}
	}

	ssize_t bytesSent = send(
		clientFd,
		response.c_str(),
		response.size(),
		0
	);

	if (bytesSent < 0)
	{
		removeClient(index);
		return;
	}

	if (bytesSent == 0)
	{
		return;
	}

	response.erase(0, bytesSent);
	/*
		If the entire response has been sent, we switch back to monitoring for incoming data (POLLIN) on the client socket
		so that clients can send additional requests without needing to reconnect.
		This allows for persistent connections, which is a key feature of HTTP/1.1.
	*/
	if (response.empty())
	{
		_pollFds[index].events = POLLIN;
	}
}

/*
REMOVE CLIENT for "resource lifecycle" management: Close the client socket, remove it from the poll list,
and clean up associated buffers and server configuration mappings.
Acquire -> Use -> Release
*/ 
void Server::removeClient(std::size_t index)
{
	int clientFd = _pollFds[index].fd;

	std::cout << "Client disconnected: fd="
			  << clientFd << std::endl;

	close(clientFd);

	_clientBuffers.erase(clientFd);
	_clientWriteBuffers.erase(clientFd);
	_clientServers.erase(clientFd);
	_clientKeepAlive.erase(clientFd);

	_pollFds.erase(_pollFds.begin() + index);
}
