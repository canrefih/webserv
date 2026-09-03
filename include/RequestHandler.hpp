#ifndef REQUESTHANDLER_HPP
#define REQUESTHANDLER_HPP

#include "ServerConfig.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Location.hpp"
#include <string>

/*
 * Handles HTTP requests and generates appropriate responses.
 * It reads files, determines content types, generates directory listings, and sets error responses based on the server configuration.
 * RequestHandler handler(serverConfig);
handler.handleRequest(request, response);
 */
class RequestHandler
{
	private:
		const ServerConfig &_serverConfig;

		std::string readFile(const std::string &path); // Read the contents of a file from the filesystem and return it as a string
		std::string getContentType(const std::string &path); // Determine the MIME type of a file based on its extension (e.g., .html, .css, .js) and return the corresponding Content-Type string
		std::string generateDirectoryListing(const std::string &path, const std::string &url); // Generate an HTML page that lists the contents of a directory, including links to files and subdirectories, based on the specified filesystem path and URL
		bool fileExists(const std::string &path); // Check if a file exists at the specified path in the filesystem and return true if it does, false otherwise
		bool isFile(const std::string &path); // Check if the specified path corresponds to
		bool isDirectory(const std::string &path); // Check if the specified path corresponds to a directory in the filesystem and return true if it does, false otherwise
		void setErrorResponse(HttpResponse &response, int statusCode,
							  const std::string &statusText, const std::string &defaultBody); // Set an error response with the specified status code, status text, and default body content. If a custom error page is configured for the status code, it will be used instead of the default body.

	public:
		RequestHandler(const ServerConfig &serverConfig);
		~RequestHandler();

		void handleRequest(const HttpRequest &request, HttpResponse &response); // Handle an HTTP request and generate an appropriate response based on the server configuration, request method, target, and other factors. The response is populated with the status code, headers, and body content.
		bool resolveCGI(const HttpRequest &request, const Location *location, std::string &scriptPath, std::string &interpreterPath) const;

};

#endif
