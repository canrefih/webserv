#include "HttpResponse.hpp"

#include <sstream>

HttpResponse::HttpResponse() // Constructor with default values
	: _statusCode(200),
	  _statusText("OK"),
	  _contentType("text/plain")
{
}

HttpResponse::~HttpResponse()
{
}

void HttpResponse::setStatus(int code, const std::string &text) // Set the HTTP status code and corresponding status text (e.g., 200 OK, 404 Not Found)
{
	_statusCode = code;
	_statusText = text;
}

void HttpResponse::setBody(const std::string &body)
{
	_body = body;
}

void HttpResponse::setContentType(const std::string &contentType)
{
	_contentType = contentType;
}

// Convert the HTTP response object into a raw HTTP response string, including the status line, headers, and body, ready to be sent over the network
std::string HttpResponse::toString() const 
{
	std::ostringstream response;

	response << "HTTP/1.1 "
			 << _statusCode
			 << " "
			 << _statusText
			 << "\r\n";

	response << "Content-Length: "
			 << _body.size()
			 << "\r\n";

	response << "Content-Type: "
			 << _contentType
			 << "\r\n";
	for (std::map<std::string, std::string>::const_iterator it = _customHeaders.begin();
		it != _customHeaders.end(); ++it)
	{
		response << it->first << ": " << it->second << "\r\n";
	}

	response << "\r\n";
	response << _body;

	return response.str();
}

// Set a custom header for the HTTP response, allowing the addition of any header not covered by the other methods (e.g., X-Custom-Header)
void HttpResponse::setHeader(const std::string &name, const std::string &value)
{
    _customHeaders[name] = value;
}