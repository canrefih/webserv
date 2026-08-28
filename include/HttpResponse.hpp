#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>
#include <map>

/*
 * Represents an HTTP response with its status code, status text, headers, and body.
 * Provides functionality to construct a raw HTTP response string from its components.
 * HttpResponse response;
response.setStatus(200, "OK");
response.setBody(...);
response.setContentType(...);
*/
class HttpResponse
{
	private:
		int			_statusCode;
		std::string	_statusText;
		std::string	_body;
		std::string	_contentType;
		std::map<std::string, std::string> _customHeaders;

	public:
		HttpResponse();
		~HttpResponse();

		void setStatus(int code, const std::string &text); // Set the HTTP status code and corresponding status text (e.g., 200 OK, 404 Not Found)
		void setBody(const std::string &body); // Set the body of the HTTP response, which contains the actual content to be sent to the client
		void setContentType(const std::string &contentType); // Set the Content-Type header of the HTTP response, indicating the media type of the response body (e.g., text/html, application/json)
		void setHeader(const std::string &name, const std::string &value); // Set a custom header for the HTTP response, allowing the addition of any header not covered by the other methods (e.g., X-Custom-Header)

		std::string toString() const; // Convert the HTTP response object into a raw HTTP response string, including the status line, headers, and body, ready to be sent over the network
};

#endif