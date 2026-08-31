#include "HttpRequest.hpp"

#include <sstream>

HttpRequest::HttpRequest()
{
}

HttpRequest::~HttpRequest()
{
}

// Helper function to convert a string to lowercase for case-insensitive header name comparisons
static std::string toLower(const std::string &str)
{
	std::string result = str;

	for (std::size_t i = 0; i < result.size(); ++i)
	{
		if (result[i] >= 'A' && result[i] <= 'Z')
			result[i] = result[i] - 'A' + 'a';
	}

	return result;
}

// Parse the raw HTTP request string and populate the HttpRequest object's fields (method, target, version, headers, and body)
bool HttpRequest::parse(const std::string &rawRequest)
{
	std::istringstream stream(rawRequest);
	std::string line;

	if (!std::getline(stream, line))
		return false;

	// Remove trailing '\r' from CRLF
	if (!line.empty() && line[line.size() - 1] == '\r')
		line.erase(line.size() - 1);

	// Parse request line
	std::istringstream requestLine(line);

	std::string raw_target;

	if (!(requestLine >> _method >> raw_target >> _version))
		return false;

	std::pair<URL, bool> url_parsed = URL::createFromRequestTarget(raw_target);

	// check if URL is malformed
	if (!url_parsed.second)
		return false;

	_target = url_parsed.first;

	// Parse headers
	while (std::getline(stream, line))
	{
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);

		// Empty line = end of headers
		if (line.empty())
			break;

		std::size_t colon = line.find(':');

		if (colon == std::string::npos)
			return false;

		std::string name = line.substr(0, colon);
		std::string value = line.substr(colon + 1);

		// Remove leading spaces from header value
		while (!value.empty() && value[0] == ' ')
			value.erase(0, 1);

		// HTTP header names are case-insensitive.
		// Store them normalized as lowercase.
		name = toLower(name);

		_headers[name] = value;
	}

	// Read body
	std::string remaining;
	std::getline(stream, remaining, '\0');

	_body = remaining;

	return true;
}

const std::string &HttpRequest::getMethod() const
{
	return _method;
}

const URL &HttpRequest::getTarget() const
{
	return _target;
}

const std::string &HttpRequest::getVersion() const
{
	return _version;
}

const std::map<std::string, std::string> &HttpRequest::getHeaders() const
{
	return _headers;
}

// Retrieve the value of a specific header by name (case-insensitive). If the header is not found, return an empty string.
const std::string &HttpRequest::getHeader(const std::string &name) const
{
	static const std::string empty;

	// Normalize the requested header name as well.
	std::string lowerName = toLower(name);

	std::map<std::string, std::string>::const_iterator it =
		_headers.find(lowerName);

	if (it == _headers.end())
		return empty;

	return it->second;
}

const std::string &HttpRequest::getBody() const
{
	return _body;
}
