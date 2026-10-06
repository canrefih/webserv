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
#include <algorithm>
#include <cctype>
#include <cerrno>

RequestHandler::RequestHandler(const ServerConfig &serverConfig)
	: _serverConfig(serverConfig)
{
}

RequestHandler::~RequestHandler()
{
}

static std::string toLower(const std::string &value)
{
    std::string result = value;

    for (std::size_t i = 0; i < result.size(); ++i)
        result[i] = static_cast<char>(
            std::tolower(static_cast<unsigned char>(result[i]))
        );

    return result;
}

// sup function to help to escape characters
static std::string escapeHtml(const std::string &value)
{
    std::string result;

    for (std::size_t i = 0; i < value.size(); ++i)
    {
        switch (value[i])
        {
            case '&':
                result += "&amp;";
                break;
            case '<':
                result += "&lt;";
                break;
            case '>':
                result += "&gt;";
                break;
            case '"':
                result += "&quot;";
                break;
            case '\'':
                result += "&#39;";
                break;
            default:
                result += value[i];
                break;
        }
    }

    return result;
}

// Handle the incoming HTTP request and generate an appropriate HTTP response based on the request method, target, and server configuration.
void RequestHandler::handleRequest(const HttpRequest &request, HttpResponse &response)
{
	const Location *location = _serverConfig.findLocation(request.getTarget().getPath());

	if (location != NULL && location->getReturnCode() != 0) // "return <code> <url>;" in the location: redirect before anything else (like nginx)
	{
		setRedirectResponse(response, location->getReturnCode(), location->getReturnPath());
	}
	else if (location != NULL &&
		!location->isMethodAllowed(request.getMethod()))
	{
		setErrorResponse(response, 405, "Method Not Allowed", "Method Not Allowed\n");
	}
	else if (request.getMethod() == "POST")
	{
		if (location == NULL || !location->getUpload())
		{
			setErrorResponse(response, 405,
							"Method Not Allowed",
							"Method Not Allowed\n");
		}
		else if (request.getBody().size() > _serverConfig.getClientMaxBodySize(location))
		{
			setErrorResponse(response, 413,
							"Payload Too Large",
							"Payload Too Large\n");
		}
		else
		{
			static int uploadCounter = 0;
			++uploadCounter;

			if (location->getUploadStore().empty())
			{
				setErrorResponse(response, 500,
								"Internal Server Error",
								"Upload store is not configured\n");
			}
			else
			{
				std::ostringstream filename;
				filename << location->getUploadStore()
						<< "/upload-"
						<< uploadCounter
						<< ".txt";

				int fd = open(filename.str().c_str(),
							O_WRONLY | O_CREAT | O_TRUNC,
							0644);

				if (fd == -1)
				{
					setErrorResponse(response, 500,
									"Internal Server Error",
									"Internal Server Error\n");
				}
				else
				{
					const std::string &body = request.getBody();
					std::size_t totalWritten = 0;
					bool writeError = false;

					while (totalWritten < body.size())
					{
						ssize_t bytesWritten =
							write(fd,
								body.c_str() + totalWritten,
								body.size() - totalWritten);

						if (bytesWritten <= 0)
						{
							writeError = true;
							break;
						}

						totalWritten +=
							static_cast<std::size_t>(bytesWritten);
					}

					close(fd);

					if (writeError)
					{
						setErrorResponse(response, 500,
										"Internal Server Error",
										"Internal Server Error\n");
					}
					else
					{
						response.setStatus(201, "Created");
						response.setBody("");
						response.setContentType("text/plain");
					}
				}
			}
		}
	}
	else if (request.getMethod() == "DELETE")
	{
		std::string root = _serverConfig.getRoot();
		std::string path;

		if (location != NULL && !location->getRoot().empty()) // If a location is found and it has a specific root configured, use that root to construct the full path
		{
			root = location->getRoot();
			std::string locationPath = location->getPath();
			std::string requestPath = request.getTarget().getPath();

			std::string relativePath = requestPath.substr(locationPath.size());

			if (!relativePath.empty() && relativePath[0] != '/') // Location declared with a trailing slash ("/directory/"): put the separator back
				relativePath = "/" + relativePath;

			path = root + relativePath;
		}
		else
		{
			path = root + request.getTarget().getPath();
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
			else if (errno == EACCES || errno == EPERM)
			{
				setErrorResponse(response, 403, "Forbidden", "Forbidden\n");
			}
			else if (errno == ENOENT)
			{
				setErrorResponse(response, 404, "Not Found", "Not Found\n");
			}
			else
			{
				setErrorResponse(response, 500, "Internal Server Error",
								"Internal Server Error\n");
			}
		}
	}
	else if (request.getMethod() == "GET")
	{
		std::string root = _serverConfig.getRoot();
		std::string path;

		if (location != NULL && !location->getRoot().empty())
		{
			root = location->getRoot();

			std::string locationPath = location->getPath();
			std::string relativePath =
				request.getTarget().getPath().substr(locationPath.size());

			if (relativePath.empty())
				relativePath = "/";
			else if (relativePath[0] != '/')
				relativePath = "/" + relativePath;

			path = root + relativePath;
		}
		else
		{
			path = root + request.getTarget().getPath();
		}

		if (!fileExists(path) && !isDirectory(path))
		{
			setErrorResponse(response, 404, "Not Found", "Not Found\n");
		}
		else if (isDirectory(path))
		{
			std::string directoryPath = path;

			if (directoryPath[directoryPath.size() - 1] != '/')
				directoryPath += "/";

			std::string index = _serverConfig.getIndex();

			if (location != NULL && !location->getIndex().empty())
				index = location->getIndex();

			std::string indexPath = directoryPath + index;

			if (fileExists(indexPath))
			{
				std::string body;

				if (!readFile(indexPath, body))
				{
					setErrorResponse(response, 500,
									"Internal Server Error",
									"Internal Server Error\n");
				}
				else
				{
					response.setStatus(200, "OK");
					response.setBody(body);
					response.setContentType(getContentType(indexPath));
				}
			}
			else
			{
				bool autoindex = _serverConfig.getAutoIndex();

				if (location != NULL && location->isAutoIndexSet())
					autoindex = location->getAutoIndex();

				if (autoindex)
				{
					std::string body =
						generateDirectoryListing(
							directoryPath,
							request.getTarget().getPath());

					response.setStatus(200, "OK");
					response.setBody(body);
					response.setContentType("text/html");
				}
				else
				{
					setErrorResponse(response, 404,
									"Not Found",
									"Not Found\n");
				}
			}
		}
		else
		{
			std::string body;

			if (!readFile(path, body))
			{
				setErrorResponse(response, 500,
								"Internal Server Error",
								"Internal Server Error\n");
			}
			else
			{
				response.setStatus(200, "OK");
				response.setBody(body);
				response.setContentType(getContentType(path));
			}
		}
	}
	else
	{
		setErrorResponse(response, 405,
						"Method Not Allowed",
						"Method Not Allowed\n");
	}
}
// Read the contents of a file from the filesystem and return it as a string. If the file cannot be opened, return an empty string.
bool RequestHandler::readFile(const std::string &path,
                              std::string &content)
{
    int fd = open(path.c_str(), O_RDONLY);

    if (fd == -1)
        return false;

    content.clear();

    char buffer[4096];
    ssize_t bytesRead;

    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0)
        content.append(buffer, bytesRead);

    close(fd);

    return bytesRead == 0;
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

	std::string extension = toLower(path.substr(dot));

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

		std::string escapedName = escapeHtml(name);

		body += "<li><a href=\"";
		body += url;
		if (url[url.size() - 1] != '/')
			body += "/";
		body += escapedName;
		body += "\">";
		body += escapedName;
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

	if (errorPage != NULL) // if there errorpage
	{
		std::string path = _serverConfig.getRoot() + *errorPage;

		if (fileExists(path))
		{
			std::string body;

			if(readFile(path, body))
			{
				response.setStatus(statusCode, statusText);
				response.setBody(body);
				response.setContentType(getContentType(path));
				return;
			}
		}
	}
	//if there is no error page it gives default error response
	response.setStatus(statusCode, statusText);
	response.setBody(defaultBody);
	response.setContentType("text/plain");
}

// Build a redirection response: the client is told to request "target" instead (Location header)
void RequestHandler::setRedirectResponse(HttpResponse &response, int statusCode, const std::string &target)
{
	std::string statusText = "Found";

	if (statusCode == 301)
		statusText = "Moved Permanently";
	else if (statusCode == 303)
		statusText = "See Other";
	else if (statusCode == 307)
		statusText = "Temporary Redirect";
	else if (statusCode == 308)
		statusText = "Permanent Redirect";

	response.setStatus(statusCode, statusText);
	response.setHeader("Location", target);
	response.setBody("<html><body><a href=\"" + escapeHtml(target) + "\">" + escapeHtml(target) + "</a></body></html>\n");
	response.setContentType("text/html");
}

bool RequestHandler::resolveCGI(const HttpRequest &request, const Location *location, std::string &scriptPath, std::string &interpreterPath)
{
	if (location == NULL || location->getReturnCode() != 0) // A redirected location never runs a CGI
		return (false);

	std::string target = request.getTarget().getPath();
	std::string::size_type qPos = target.find('?');
	std::string targetPath = (qPos == std::string::npos) ? target : target.substr(0, qPos);

	if (targetPath.find("..") != std::string::npos) // Prevent directory traversal attacks, same check as GET/DELETE
		return (false);

	std::string path;

	if (!location->getRoot().empty())
	{
		std::string locationPath = location->getPath();
		std::string relativePath = targetPath.substr(locationPath.size());

		if (relativePath.empty())
			relativePath = "/";
		else if (relativePath[0] != '/') // Location declared with a trailing slash ("/directory/"): put the separator back
			relativePath = "/" + relativePath;

		path = location->getRoot() + relativePath;
	}
	else
		path = _serverConfig.getRoot() + targetPath;
	std::size_t dot = path.find_last_of('.');
	if (dot == std::string::npos)
		return (false);
	std::string extension = path.substr(dot);
	if (!location->isCgiExtension(extension))
		return (false);
	if (!location->isCgiVirtual(extension) && !fileExists(path)) // Let a missing script fall through to the normal GET/POST/DELETE path so it gets a proper 404 instead of failing execve() later (unless the extension is declared "virtual")
		return (false);
	scriptPath = path;
	interpreterPath = location->getCgiInterpreter(extension);
	return (true);
}