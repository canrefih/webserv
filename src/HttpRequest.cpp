#include "HttpRequest.hpp"

#include <sstream>

HttpRequest::HttpRequest()
{
}

HttpRequest::~HttpRequest()
{
}

bool HttpRequest::parse(const std::string &rawRequest)
{
	std::istringstream stream(rawRequest);
	std::string line;

	if (!std::getline(stream, line))
		return false;

	if (!line.empty() && line[line.size() - 1] == '\r')
		line.erase(line.size() - 1);

	std::istringstream requestLine(line);

	if (!(requestLine >> _method >> _target >> _version))
		return false;

	while (std::getline(stream, line))
	{
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);

		if (line.empty())
			break;

		std::size_t colon = line.find(':');

		if (colon == std::string::npos)
			return false;

		std::string name = line.substr(0, colon);
		std::string value = line.substr(colon + 1);

		while (!value.empty() && value[0] == ' ')
			value.erase(0, 1);

		_headers[name] = value;
	}

	std::string remaining;
	std::getline(stream, remaining, '\0');

	_body = remaining;

	return true;
}

const std::string &HttpRequest::getMethod() const
{
	return _method;
}

const std::string &HttpRequest::getTarget() const
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

const std::string &HttpRequest::getHeader(const std::string &name) const
{
	static const std::string empty;

	std::map<std::string, std::string>::const_iterator it =
		_headers.find(name);

	if (it == _headers.end())
		return empty;

	return it->second;
}

const std::string &HttpRequest::getBody() const
{
	return _body;
}