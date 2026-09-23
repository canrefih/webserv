#include "HttpResponse.hpp"

#include <sstream>
#include <exception>

HttpResponse::HttpResponse() // Constructor with default values
	: _statusCode(200),
	  _statusText("OK"),
	  _contentType("text/plain")
{
}

HttpResponse::~HttpResponse()
{
}

HttpResponse::HttpResponse(int statusCode)
	: _statusCode(statusCode),
	  _statusText(reasonPhrase(statusCode)) {}

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

// https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Status
std::string HttpResponse::reasonPhrase(int code)
{
	switch (code)
	{
		case 100: return "Continue";
		case 101: return "Switching Protocols";
		case 103: return "Early Hints";
		case 200: return "OK";
		case 201: return "Created";
		case 202: return "Accepted";
		case 203: return "Non-Authoritative Information";
		case 204: return "No Content";
		case 205: return "Reset Content";
		case 206: return "Partial Content";
		case 207: return "Multi-Status";
		case 208: return "Already Reported";
		case 226: return "IM Used";
		case 300: return "Multiple Choices";
		case 301: return "Moved Permanently";
		case 302: return "Found";
		case 303: return "See Other";
		case 304: return "Not Modified";
		case 307: return "Temporary Redirect";
		case 308: return "Permanent Redirect";
		case 400: return "Bad Request";
		case 401: return "Unauthorized";
		case 402: return "Payment Required";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 406: return "Not Acceptable";
		case 407: return "Proxy Authentication Required";
		case 408: return "Request Timeout";
		case 409: return "Conflict";
		case 410: return "Gone";
		case 411: return "Length Required";
		case 412: return "Precondition Failed";
		case 413: return "Content Too Large";
		case 414: return "URI Too Long";
		case 415: return "Unsupported Media Type";
		case 416: return "Range Not Satisfiable";
		case 417: return "Expectation Failed";
		case 418: return "I'm a teapot";
		case 421: return "Misdirected Request";
		case 422: return "Unprocessable Content";
		case 423: return "Locked";
		case 424: return "Failed Dependency";
		case 425: return "Too Early";
		case 426: return "Upgrade Required";
		case 428: return "Precondition Required";
		case 429: return "Too Many Requests";
		case 431: return "Request Header Fields Too Large";
		case 451: return "Unavailable For Legal Reasons";
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 502: return "Bad Gateway";
		case 503: return "Service Unavailable";
		case 504: return "Gateway Timeout";
		case 505: return "HTTP Version Not Supported";
		case 506: return "Variant Also Negotiates";
		case 507: return "Insufficient Storage";
		case 508: return "Loop Detected";
		case 510: return "Not Extended";
		case 511: return "Network Authentication Required";
		default: throw std::runtime_error("Unreachable: Unknown Status Code");
	}
}

/*Need it because we calculate cookies early  and we need to take it back to put it
in _pendindCgiCookies while is waitinf CGI response because asymcrone*/
const std::string &HttpResponse::getHeader(const std::string &name) const
{
	static const std::string empty;
	std::map<std::string, std::string>::const_iterator it = _customHeaders.find(name);

	if (it == _customHeaders.end())
		return (empty);

	return (it->second);
}
