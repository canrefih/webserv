#include "RequestHandler.hpp"

#include <iostream>
#include <fstream> // For file stream operations
#include <sstream> // For string stream operations
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h> // For file status information S_ISREG
#include <fcntl.h>
#include <dirent.h> // For directory operations

RequestHandler::RequestHandler(const ServerConfig &serverConfig)
	: _serverConfig(serverConfig)
{
}

RequestHandler::~RequestHandler()
{
}

// Handle the incoming HTTP request and generate an appropriate HTTP response based on the request method, target, and server configuration.
void RequestHandler::handleRequest(const HttpRequest &request, HttpResponse &response)
{
	const Location *location = _serverConfig.findLocation(request.getTarget());

	if (location != NULL &&
		!location->isMethodAllowed(request.getMethod()))
	{
		setErrorResponse(response, 405, "Method Not Allowed", "Method Not Allowed\n");
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
				int fd = open(filename.str().c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);

				if (fd == -1)
				{
					response.setStatus(500, "Internal Server Error");
					response.setBody("Internal Server Error\n");
					response.setContentType("text/plain");
				}
				else
				{
					const std::string &body = request.getBody();

					if (!body.empty()) // Write the request body to the file if it is not empty
					{
						ssize_t bytesWritten = write(fd, body.c_str(), body.size());

						if (bytesWritten != static_cast<ssize_t>(body.size()))
						{
							close(fd);
							response.setStatus(500, "Internal Server Error");
							response.setBody("Internal Server Error\n");
							response.setContentType("text/plain");
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
		if (request.getTarget().find("..") != std::string::npos) // Prevent directory traversal attacks by checking for ".." in the target path
		{
			setErrorResponse(response, 403, "Forbidden", "Forbidden\n");
		}
		else
		{
			std::string root = _serverConfig.getRoot();
			std::string path;

			if (location != NULL && !location->getRoot().empty()) // If a location is found and it has a specific root configured, use that root to construct the full path
			{
				root = location->getRoot();
				std::string locationPath = location->getPath();
				std::string relativePath = request.getTarget().substr(locationPath.size());

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
				setErrorResponse(response, 404, "Not Found", "Not Found\n");
			}
			else if (isDirectory(path))
			{
				setErrorResponse(response, 403, "Forbidden", "Forbidden\n");
			}
			else
			{
				if (unlink(path.c_str()) == 0) // If the file is successfully deleted, set the response to indicate success with a 204 No Content status
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
			setErrorResponse(response, 403, "Forbidden", "Forbidden\n");
		}
		else
		{
			std::string root = _serverConfig.getRoot();
			std::string path;

			if (location != NULL && !location->getRoot().empty())
			{
				root = location->getRoot();
				std::string locationPath = location->getPath();
				std::string relativePath = request.getTarget().substr(locationPath.size());

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
				setErrorResponse(response, 404, "Not Found", "Not Found\n");
			}
			else if (isDirectory(path)) // If the target path is a directory, check for an index file or generate a directory listing based on the server configuration and location settings
			{
				std::string directoryPath = path;

				if (directoryPath[directoryPath.size() - 1] != '/')
					directoryPath += "/";

				std::string index = _serverConfig.getIndex();

				if (location != NULL && !location->getIndex().empty())
					index = location->getIndex();

				std::string indexPath = directoryPath + index;

				if (fileExists(indexPath)) // If an index file exists in the directory, read its contents and set it as the response body with a 200 OK status
				{
					std::string body = readFile(indexPath);

					response.setStatus(200, "OK");
					response.setBody(body);
					response.setContentType(getContentType(indexPath));
				}
				else
				{
					bool autoindex = _serverConfig.getAutoIndex();

					if (location != NULL && location->isAutoIndexSet())
						autoindex = location->getAutoIndex();

					if (autoindex) // If autoindex is enabled, generate a directory listing and set it as the response body with a 200 OK status
					{
						std::string body = generateDirectoryListing(directoryPath, request.getTarget());

						response.setStatus(200, "OK");
						response.setBody(body);
						response.setContentType("text/html");
					}
					else
					{
						setErrorResponse(response, 403, "Forbidden", "Forbidden\n");
					}
				}
			}
			else // If the target path is a file, read its contents and set it as the response body with a 200 OK status
			{
				std::string body = readFile(path);

				response.setStatus(200, "OK");
				response.setBody(body);
				response.setContentType(getContentType(path));
			}
		}
	}
	else // If the request method is not supported, set an error response indicating that the method is not allowed (405)
	{
		setErrorResponse(response, 405, "Method Not Allowed", "Method Not Allowed\n");
	}
}

// Read the contents of a file from the filesystem and return it as a string. If the file cannot be opened, return an empty string.
std::string RequestHandler::readFile(const std::string &path)
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

// Check if a file exists at the specified path in the filesystem and return true if it does, false otherwise
bool RequestHandler::fileExists(const std::string &path)
{
	struct stat fileStat;

	if (stat(path.c_str(), &fileStat) == -1)
		return false;

	return S_ISREG(fileStat.st_mode); // Check if the path corresponds to a regular file (not a directory or special file) and return true if it does, false otherwise
}

// Determine the MIME type of a file based on its extension (e.g., .html, .css, .js) and return the corresponding Content-Type string.
// If the extension is not recognized, return "application/octet-stream" as a default.
std::string RequestHandler::getContentType(const std::string &path)
{
	std::size_t dot = path.find_last_of('.'); // Find the last occurence of a dot in the file path

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

// Check if the specified path corresponds to a directory in the filesystem and return true if it does, false otherwise
bool RequestHandler::isDirectory(const std::string &path)
{
	struct stat fileStat;

	if (stat(path.c_str(), &fileStat) == -1) // If the stat call fails (e.g., the path does not exist), return false to indicate that the path is not a directory
		return false;

	return S_ISDIR(fileStat.st_mode);
}

// Generate an HTML page that lists the contents of a directory, including links to files and subdirectories, based on the specified filesystem path and URL.
std::string RequestHandler::generateDirectoryListing(const std::string &path, const std::string &url)
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

	while ((entry = readdir(dir)) != NULL) // Loop through each entry in the directory and generate an HTML list item with a link to the file or subdirectory
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

// Set an error response with the specified status code, status text, and default body. If a custom error page is configured for the status code, it will be used instead of the default body.
void RequestHandler::setErrorResponse(HttpResponse &response, int statusCode, 
									   const std::string &statusText, const std::string &defaultBody)
{
	const std::string *errorPage = _serverConfig.getErrorPage(statusCode);

	if (errorPage != NULL)
	{
		std::string path = _serverConfig.getRoot() + *errorPage;

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
