#include "Server.hpp"

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <sstream>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <dirent.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

Server::Server(const Config &config)
	: _listenFd(-1),
	  _config(config)
{
}

Server::~Server()
{
	if (_listenFd != -1)
		close(_listenFd);
}

void Server::setNonBlocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);

	if (flags == -1)
	{
		std::cerr << "Error: fcntl(F_GETFL) failed: "
				  << strerror(errno) << std::endl;
		close(fd);
		std::exit(1);
	}

	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		std::cerr << "Error: fcntl(F_SETFL) failed: "
				  << strerror(errno) << std::endl;
		close(fd);
		std::exit(1);
	}
}

void Server::createSocket()
{
	_listenFd = socket(AF_INET, SOCK_STREAM, 0);

	if (_listenFd == -1)
	{
		std::cerr << "Error: socket() failed: "
				  << strerror(errno) << std::endl;
		std::exit(1);
	}

	int opt = 1;

	if (setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR,
				   &opt, sizeof(opt)) == -1)
	{
		std::cerr << "Error: setsockopt() failed: "
				  << strerror(errno) << std::endl;
		close(_listenFd);
		std::exit(1);
	}

	setNonBlocking(_listenFd);
}

void Server::bindSocket()
{
	struct addrinfo hints;
	struct addrinfo *result = NULL;

	std::memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	std::ostringstream portStream;
	portStream << _config.getPort();

	std::string port = portStream.str();

	int status = getaddrinfo(
		_config.getHost().c_str(),
		port.c_str(),
		&hints,
		&result
	);

	if (status != 0)
	{
		std::cerr << "Error: getaddrinfo() failed: "
				  << gai_strerror(status) << std::endl;
		close(_listenFd);
		std::exit(1);
	}

	if (bind(_listenFd, result->ai_addr, result->ai_addrlen) == -1)
	{
		std::cerr << "Error: bind() failed: "
				  << strerror(errno) << std::endl;
		freeaddrinfo(result);
		close(_listenFd);
		std::exit(1);
	}

	freeaddrinfo(result);
}

void Server::startListening()
{
	if (listen(_listenFd, SOMAXCONN) == -1)
	{
		std::cerr << "Error: listen() failed: "
				  << strerror(errno) << std::endl;
		close(_listenFd);
		std::exit(1);
	}
}

void Server::run()
{
	createSocket();
	bindSocket();
	startListening();

	addPollFd(_listenFd, POLLIN);

	std::cout << "Server started" << std::endl;
	std::cout << "Listening on "
			  << _config.getHost()
			  << ":"
			  << _config.getPort()
			  << std::endl;

	while (true)
	{
		int ready = poll(
			&_pollFds[0],
			_pollFds.size(),
			-1
		);

		if (ready == -1)
		{
			std::cerr << "Error: poll() failed: "
					  << strerror(errno) << std::endl;
			return;
		}

		std::size_t i = 0;

		while (i < _pollFds.size())
		{
			if (_pollFds[i].fd == _listenFd)
			{
				if (_pollFds[i].revents & POLLIN)
					acceptClient();

				++i;
				continue;
			}

			bool removed = false;

			if (_pollFds[i].revents & POLLIN)
			{
				std::size_t oldSize = _pollFds.size();

				handleClientRead(i);

				if (_pollFds.size() != oldSize)
					removed = true;
			}

			if (!removed && (_pollFds[i].revents & POLLOUT))
			{
				std::size_t oldSize = _pollFds.size();

				handleClientWrite(i);

				if (_pollFds.size() != oldSize)
					removed = true;
			}

			if (!removed)
				++i;
		}
	}
}

void Server::addPollFd(int fd, short events)
{
	struct pollfd pfd;

	pfd.fd = fd;
	pfd.events = events;
	pfd.revents = 0;

	_pollFds.push_back(pfd);
}

void Server::acceptClient()
{
	struct sockaddr_storage clientAddress;
	socklen_t clientAddressSize = sizeof(clientAddress);

	int clientFd = accept(
		_listenFd,
		reinterpret_cast<struct sockaddr *>(&clientAddress),
		&clientAddressSize
	);

	if (clientFd == -1)
		return;

	setNonBlocking(clientFd);

	addPollFd(clientFd, POLLIN);
	_clientBuffers[clientFd] = "";
	std::cout << "New client connected: fd="
			  << clientFd << std::endl;
}

void Server::handleClientRead(std::size_t index)
{
	char buffer[4096];

	int clientFd = _pollFds[index].fd;

	ssize_t bytesRead = recv(
		clientFd,
		buffer,
		sizeof(buffer),
		0
	);

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

	_clientBuffers[clientFd].append(buffer, bytesRead);

	std::string &requestBuffer = _clientBuffers[clientFd];

	std::size_t headerEnd = requestBuffer.find("\r\n\r\n");

	if (headerEnd == std::string::npos)
		return;

	HttpRequest request;

	if (!request.parse(requestBuffer))
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

	if (!contentLength.empty())
	{
		std::size_t expectedBodyLength =
			std::atoi(contentLength.c_str());

		if (expectedBodyLength > _config.getClientMaxBodySize())
		{
			HttpResponse response;

			setErrorResponse(
					response,
					413,
					"Payload Too Large",
					"Payload Too Large\n"
			);

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
	std::cout << "Target:  " << request.getTarget() << std::endl;
	std::cout << "Version: " << request.getVersion() << std::endl;

	const Location *location = _config.findLocation(request.getTarget());
	HttpResponse response;

	if (location != NULL &&
		!location->isMethodAllowed(request.getMethod()))
	{
		setErrorResponse(
				response,
				405,
				"Method Not Allowed",
				"Method Not Allowed\n"
		);
	}
	else if (request.getMethod() == "POST")
	{
		if (location == NULL || !location->getUpload())
		{
			response.setStatus(405, "Method Not Allowed");
			response.setBody("Method Not Allowed\n");
			response.setContentType("text/plain");
		}
		else
		{
			static int uploadCounter = 0;

			++uploadCounter;

			std::ostringstream filename;
			filename << location->getUploadStore()
					<< "/upload-"
					<< uploadCounter
					<< ".txt";

			if (location->getUploadStore().empty())
			{
				response.setStatus(500, "Internal Server Error");
				response.setBody("Upload store is not configured\n");
				response.setContentType("text/plain");
			}
			else
			{
				int fd = open(
					filename.str().c_str(),
					O_WRONLY | O_CREAT | O_TRUNC,
					0644
				);

				if (fd == -1)
				{
					response.setStatus(500, "Internal Server Error");
					response.setBody("Internal Server Error\n");
					response.setContentType("text/plain");
				}
				else
				{
					const std::string &body = request.getBody();

					if (!body.empty())
					{
						ssize_t bytesWritten = write(
							fd,
							body.c_str(),
							body.size()
						);

						if (bytesWritten != static_cast<ssize_t>(body.size()))
						{
							close(fd);

							response.setStatus(500, "Internal Server Error");
							response.setBody("Internal Server Error\n");
							response.setContentType("text/plain");

							_clientWriteBuffers[clientFd] =
								response.toString();

							_pollFds[index].events = POLLOUT;
							requestBuffer.clear();
							return;
						}
					}

					close(fd);

					response.setStatus(201, "Created");
					response.setBody("File uploaded\n");
					response.setContentType("text/plain");
				}
			}
		}
	}
	else if (request.getMethod() == "DELETE")
	{
		if (request.getTarget().find("..") != std::string::npos)
		{
			setErrorResponse(
					response,
					403,
					"Forbidden",
					"Forbidden\n"
			);
		}
		else
		{
			std::string root = _config.getRoot();
			std::string path;

			if (location != NULL && !location->getRoot().empty())
			{
				root = location->getRoot();

				std::string locationPath = location->getPath();
				std::string relativePath =
					request.getTarget().substr(locationPath.size());

				if (relativePath.empty())
					relativePath = "/";

				path = root + relativePath;
			}
			else
			{
				path = root + request.getTarget();
			}

			if (!fileExists(path) && !isDirectory(path))
			{
				setErrorResponse(
						response,
						404,
						"Not Found",
						"Not Found\n"
				);
			}
			else if (isDirectory(path))
			{
				setErrorResponse(
						response,
						403,
						"Forbidden",
						"Forbidden\n"
				);
			}
			else
			{
					if (unlink(path.c_str()) == 0)
					{
							response.setStatus(204, "No Content");
							response.setBody("");
							response.setContentType("text/plain");
					}
					else
					{
							response.setStatus(500, "Internal Server Error");
							response.setBody("Internal Server Error\n");
							response.setContentType("text/plain");
					}
			}
		}
	}
	else if (request.getMethod() == "GET")
	{
		if (request.getTarget().find("..") != std::string::npos)
		{
			setErrorResponse(
					response,
					403,
					"Forbidden",
					"Forbidden\n"
			);
		}
		else
		{
			std::string root = _config.getRoot();
			std::string path;

			if (location != NULL && !location->getRoot().empty())
			{
				root = location->getRoot();

				std::string locationPath = location->getPath();
				std::string relativePath =
					request.getTarget().substr(locationPath.size());

				if (relativePath.empty())
					relativePath = "/";

				path = root + relativePath;
			}
			else
			{
				path = root + request.getTarget();
			}

			if (!fileExists(path) && !isDirectory(path))
			{
				setErrorResponse(
						response,
						404,
						"Not Found",
						"Not Found\n"
				);
			}
			else if (isDirectory(path))
			{
				std::string directoryPath = path;

				if (directoryPath[directoryPath.size() - 1] != '/')
					directoryPath += "/";

				std::string index = _config.getIndex();

				if (location != NULL && !location->getIndex().empty())
					index = location->getIndex();

				std::string indexPath =
						directoryPath + index;

				if (fileExists(indexPath))
				{
					std::string body = readFile(indexPath);

					response.setStatus(200, "OK");
					response.setBody(body);
					response.setContentType(getContentType(indexPath));
				}
				else
				{
					bool autoindex = _config.getAutoIndex();

					if (location != NULL && location->isAutoIndexSet())
						autoindex = location->getAutoIndex();

					if (autoindex)
					{
						std::string body =
								generateDirectoryListing(
										directoryPath,
										request.getTarget()
								);

						response.setStatus(200, "OK");
						response.setBody(body);
						response.setContentType("text/html");
					}
					else
					{
						setErrorResponse(
								response,
								403,
								"Forbidden",
								"Forbidden\n"
						);
					}
				}
			}
			else
			{
				std::string body = readFile(path);

				response.setStatus(200, "OK");
				response.setBody(body);
				response.setContentType(getContentType(path));
			}
		}
	}
	else
	{
		setErrorResponse(
				response,
				405,
				"Method Not Allowed",
				"Method Not Allowed\n"
		);
	}
	_clientWriteBuffers[clientFd] = response.toString();

	_pollFds[index].events = POLLOUT;

	requestBuffer.clear();
}

void Server::handleClientWrite(std::size_t index)
{
	int clientFd = _pollFds[index].fd;

	std::string &response = _clientWriteBuffers[clientFd];

	if (response.empty())
	{
		removeClient(index);
		return;
	}

	ssize_t bytesSent = send(
		clientFd,
		response.c_str(),
		response.size(),
		0
	);

	if (bytesSent <= 0)
	{
		removeClient(index);
		return;
	}

	response.erase(0, bytesSent);

	if (response.empty())
		removeClient(index);
}

void Server::removeClient(std::size_t index)
{
	int clientFd = _pollFds[index].fd;

	std::cout << "Client disconnected: fd="
			  << clientFd << std::endl;

	close(clientFd);

	_clientBuffers.erase(clientFd);
	_clientWriteBuffers.erase(clientFd);

	_pollFds.erase(_pollFds.begin() + index);
}

std::string Server::readFile(const std::string &path)
{
	int fd = open(path.c_str(), O_RDONLY);

	if (fd == -1)
		return "";

	std::string content;
	char buffer[4096];

	ssize_t bytesRead;

	while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0)
		content.append(buffer, bytesRead);

	close(fd);

	return content;
}

bool Server::fileExists(const std::string &path)
{
	struct stat fileStat;

	if (stat(path.c_str(), &fileStat) == -1)
		return false;

	return S_ISREG(fileStat.st_mode);
}

std::string Server::getContentType(const std::string &path)
{
	std::size_t dot = path.find_last_of('.');

	if (dot == std::string::npos)
		return "application/octet-stream";

	std::string extension = path.substr(dot);

	if (extension == ".html" || extension == ".htm")
		return "text/html";

	if (extension == ".css")
		return "text/css";

	if (extension == ".js")
		return "application/javascript";

	if (extension == ".txt")
		return "text/plain";

	if (extension == ".json")
		return "application/json";

	if (extension == ".png")
		return "image/png";

	if (extension == ".jpg" || extension == ".jpeg")
		return "image/jpeg";

	if (extension == ".gif")
		return "image/gif";

	if (extension == ".svg")
		return "image/svg+xml";

	return "application/octet-stream";
}

bool Server::isDirectory(const std::string &path)
{
	struct stat fileStat;

	if (stat(path.c_str(), &fileStat) == -1)
		return false;

	return S_ISDIR(fileStat.st_mode);
}

std::string Server::generateDirectoryListing(
	const std::string &path,
	const std::string &url
)
{
	DIR *dir = opendir(path.c_str());

	if (dir == NULL)
		return "";

	std::string body;

	body += "<!DOCTYPE html>\n";
	body += "<html>\n";
	body += "<head><title>Index of " + url + "</title></head>\n";
	body += "<body>\n";
	body += "<h1>Index of " + url + "</h1>\n";
	body += "<ul>\n";

	struct dirent *entry;

	while ((entry = readdir(dir)) != NULL)
	{
		std::string name = entry->d_name;

		if (name == "." || name == "..")
			continue;

		body += "<li><a href=\"";
		body += url;

		if (url[url.size() - 1] != '/')
			body += "/";

		body += name;
		body += "\">";
		body += name;
		body += "</a></li>\n";
	}

	closedir(dir);

	body += "</ul>\n";
	body += "</body>\n";
	body += "</html>\n";

	return body;
}

void Server::setErrorResponse(
        HttpResponse &response,
        int statusCode,
        const std::string &statusText,
        const std::string &defaultBody
)
{
    const std::string *errorPage =
        _config.getErrorPage(statusCode);

    if (errorPage != NULL)
    {
        std::string path = _config.getRoot() + *errorPage;

        if (fileExists(path))
        {
            std::string body = readFile(path);

            response.setStatus(statusCode, statusText);
            response.setBody(body);
            response.setContentType(getContentType(path));
            return;
        }
    }

    response.setStatus(statusCode, statusText);
    response.setBody(defaultBody);
    response.setContentType("text/plain");
}