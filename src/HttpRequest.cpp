#include "HttpRequest.hpp"
#include "lib/StringBuilder.hpp"
#include "lib/StringView.hpp"
#include "utils.hpp"
#include <algorithm>

#include <sstream>
#include <vector>
#include <cstdlib>

HttpRequest::HttpRequest()
{
}

HttpRequest::~HttpRequest()
{
}

// Parse the raw HTTP request string and populate the HttpRequest object's fields (method, target, version, headers, and body)
std::pair<bool, int> HttpRequest::parse(const std::string &rawRequest)
{
	std::istringstream stream(rawRequest);
	std::string line;

	if (!std::getline(stream, line))
		return std::make_pair(false, 400);

	// Remove trailing '\r' from CRLF
	if (!line.empty() && line[line.size() - 1] == '\r')
		line.erase(line.size() - 1);

	// Parse request line
	std::istringstream requestLine(line);

	std::string raw_target;

	if (!(requestLine >> _method >> raw_target >> _version))
		return std::make_pair(false, 400);

	// Parse Http version
	{
		std::size_t slash = _version.find("/");

		std::string ver = _version.substr(slash + 1);
		if (ver.length() < 3 
		    || !std::isdigit(static_cast<unsigned char>(ver[0]))
		    || ver[1] != '.'
		    || !std::isdigit(static_cast<unsigned char>(ver[2])))
			return std::make_pair(false, 400);

		for (size_t i = 3; i < ver.length(); ++i)
		{
		    if (!std::isdigit(static_cast<unsigned char>(ver[i])))
				return std::make_pair(false, 400);
		}
	}

	std::pair<URL, bool> url_parsed = URL::createFromRequestTarget(raw_target);

	// check if URL is malformed
	if (!url_parsed.second)
		return std::make_pair(false, 400);

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
			return std::make_pair(false, 400);

		std::string name = line.substr(0, colon);
		std::string value = line.substr(colon + 1);

		// Remove leading spaces from header value
		while (!value.empty() && value[0] == ' ')
			value.erase(0, 1);

		// HTTP header names are case-insensitive.
		// Store them normalized as lowercase.
		name = utils::toLower(name);

		_headers[name] = value;
	}

	bool is_content_length = _headers.find("content-length") != _headers.end();
	bool is_transfer_encoding = _headers.find("transfer-encoding") != _headers.end();

	if (_method == "DELETE" || _method == "PUT")
	{
		if (!is_content_length && !is_transfer_encoding)
			return std::make_pair(false, 411);

		// case of error is XNOR exists("content-length") with exists("transfer-encoding") 
		// there should be at least one, but not both
		if (is_content_length == is_transfer_encoding)
			return std::make_pair(false, 400);
	}

	// Read body
	std::size_t headerEnd = rawRequest.find("\r\n\r\n");
	if (headerEnd != std::string::npos)
	{
		_body = rawRequest.substr(headerEnd + 4);

		{
			// the body length should be the content-length if provided
			std::map<std::string, std::string>::iterator it = _headers.find("content-length");
			if (it != _headers.end()
				&& std::atoi(it->second.c_str()) != static_cast<int>(_body.size()))
				return std::make_pair(false, 400);
		}
	}

	if (is_transfer_encoding)
	{
		const std::string& transfer_encoding = _headers["transfer-encoding"];
		StringBuilder unchunked_body(_body.length());
		StringView line_v;

		if (transfer_encoding == "chunked")
		{
			std::stringstream body(_body);

			while (1)
			{
				if (!std::getline(body, line))
					return std::make_pair(false, 400);
				line_v = line;

				std::size_t content_len = 0;

				if (line_v.back() == '\r')
					line_v.remove_suffix(1);

				for (StringView::iterator it = line_v.begin(); it != line_v.end(); ++it)
				{
					int char_conv = utils::hex_to_val(*it);
					if (char_conv == -1)
						return std::make_pair(false, 400);
					content_len <<= 4;
					content_len |= char_conv;
				}

				if (content_len == 0)
					break;

				if (!std::getline(body, line))
					return std::make_pair(false, 400);
				line_v = line;

				if (line_v.back() == '\r')
					line_v.remove_suffix(1);

				if (content_len != line_v.length())
					return std::make_pair(false, 400);

				unchunked_body << line_v << '\n';
			}
		}
		else
			return std::make_pair(false, 501);

		_body = unchunked_body.str();
		_headers.erase("transfer-encoding");
		_headers["content-length"] = utils::to_string(unchunked_body.length());
	}

	return std::make_pair(true, 400);
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
	std::string lowerName = utils::toLower(name);

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
