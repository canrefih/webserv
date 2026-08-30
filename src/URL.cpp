#include "URL.hpp"
#include <sstream>

#include <cctype>
#include <cctype>
#include <exception>
#include <iostream>

URL::URL() {}
URL::~URL() {}
URL::URL(const URL& other)
	: _content(other._content),
	_host(other._host),
	_path(other._path),
	_query(other._query) {}

URL& URL::operator=(const URL& other)
{
	if (this != &other)
	{
		_content = other._content;
		_path = other._path;
		_host = other._host;
		_query = other._query;
	}
	return *this;
}

const std::string& URL::getContent() const { return _content; }
const std::string& URL::getHost() const { return _host; }
const std::string& URL::getPath() const { return _path; }
const std::string& URL::getQuery() const { return _query; }

std::pair<URL, bool> URL::createFromRequestTarget(const std::string& s)
{
	URL url;

	url._content = s;

	if (s.find('#') != std::string::npos)
		return std::make_pair(url, false);

	std::string::const_iterator start = s.begin();
	std::string::const_iterator path_start = start;

	if (s.substr(0, 4) == "http")
	{
		std::string::const_iterator it = start + 4;
		if (it != s.end() && *it == 's')
			++it;
		if (s.end() - it < 3 || s.substr(it - start, 3) != "://")
			return std::make_pair(url, false);
		it += 3;
		std::string::const_iterator auth_end = std::find(it, s.end(), '/');
		url._host = s.substr(it - start, auth_end - it);
		path_start = auth_end;
	}
	else if (s.empty() || s[0] != '/')
		return std::make_pair(url, false);

	std::string::const_iterator path_end = std::find(path_start, s.end(), '?');

	if (path_end != s.end())
		url._query = s.substr(path_end - start + 1, s.end() - path_end - 1);

	std::string raw_path = s.substr(path_start - start, path_end - path_start);
	if (raw_path.empty())
		raw_path = "/";

	std::pair<std::string, bool> decoded = URL::decode(raw_path);
	if (!decoded.second)
		return std::make_pair(url, false);

	std::pair<std::string, bool> normalized = URL::normalize(decoded.first);
	if (!normalized.second)
		return std::make_pair(url, false);

	url._path = normalized.first;

	return std::make_pair(url, true);
}

std::string URL::encode(const std::string& s)
{
	static const char hex_base[] = "0123456789ABCDEF";

	std::string res;
	res.reserve(s.length() * 3);

	char	buf[4] = {0};
	unsigned char c;

	for (std::string::const_iterator it = s.begin(); it != s.end(); ++it)
	{
		c = *it;
		if (std::isalnum(c) || c == '-' || c == '_'
			|| c == '.' || c == '~') {
			buf[0] = c;
			buf[1] = '\0';
		} else {
			buf[0] = '%';
			buf[1] = hex_base[c >> 4];
			buf[2] = hex_base[c & 0x0F];
		}
		res += buf;
	}
	return res;
}

static int hex_to_val(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

std::pair<std::string, bool> URL::decode(const std::string& s)
{
	std::string res;
	res.reserve(s.length());

	int percent_code;
	int tmp;
	unsigned char c;

	for (std::string::const_iterator it = s.begin(); it != s.end(); ++it)
	{
		c = *it;
		if (std::isspace(c))
			return std::make_pair("", false);
		if (c == '%')
		{
			if (++it == s.end())
				return std::make_pair("", false);
			tmp = hex_to_val(*it);
			if (tmp == -1)
				return std::make_pair("", false);
			percent_code = tmp << 4;
			if (++it == s.end())
				return std::make_pair("", false);
			tmp = hex_to_val(*it);
			if (tmp == -1)
				return std::make_pair("", false);
			percent_code |= tmp;

			c = static_cast<char>(percent_code);
		}
		res += c;
	}
	return std::make_pair(res, true);
}

std::pair<std::string, bool> URL::normalize(const std::string& s)
{
	if (s.find("..") == std::string::npos)
		return std::make_pair(s, true);

	std::vector<std::string> segments;

	{
		std::size_t pos = 0;
		std::size_t end = 0;

		while (true)
		{
			if (s[pos] == '/')
				pos++;
			end = s.find("/", pos);
			if (pos != end) 
				segments.push_back(s.substr(pos, end - pos));
			if (end == std::string::npos)
				break;
			pos = end;
		}

	}

	{
		std::vector<std::string>::iterator it = segments.begin();
		while (it != segments.end())
		{
			if (*it == "." || *it == "")
				it = segments.erase(it);
			else
				++it;
		}
	}

	if (segments.empty())
		return std::make_pair("/", true);

	{
		std::vector<std::string>::iterator it = segments.begin();
		while (it != segments.end())
		{
			if (*it == "..")
			{
				if (it == segments.begin())
					return std::make_pair("", false);
				it = segments.erase(it);
				if (it == segments.begin())
					return std::make_pair("", false);
				--it;
				it = segments.erase(it);
			}
			else
				++it;
		}
	}

	if (segments.empty())
		return std::make_pair("/", true);

	std::string res;
	res.reserve(s.length());

	for (std::vector<std::string>::iterator it = segments.begin(); it != segments.end(); ++it)
		res += "/" + *it;

	return std::make_pair(res, true);
}
