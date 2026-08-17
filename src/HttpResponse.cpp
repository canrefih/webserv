#include "HttpResponse.hpp"

#include <sstream>

HttpResponse::HttpResponse()
	: _statusCode(200),
	  _statusText("OK"),
	  _contentType("text/plain")
{
}

HttpResponse::~HttpResponse()
{
}

void HttpResponse::setStatus(int code, const std::string &text)
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

	response << "Connection: close\r\n";
	response << "\r\n";
	response << _body;

	return response.str();
}