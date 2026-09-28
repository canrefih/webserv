#include "HttpRequest.hpp"

#include <sstream>

HttpRequest::HttpRequest()
    : _duplicateContentLength(false)
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

// Parse the raw HTTP request string and populate the HttpRequest object's fields
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

        // Check for extra tokens in request line
        std::string extraToken;
        if (requestLine >> extraToken)
                return false;

        // Validate HTTP version
        if (_version != "HTTP/1.1" && _version != "HTTP/1.0")
                return false;

        std::pair<URL, bool> url_parsed = URL::createFromRequestTarget(raw_target);

        // Check if URL is malformed
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

                // Reject whitespace before colon (RFC 7230 §3.2.4)
                if (!name.empty() && (name[name.size() - 1] == ' ' || name[name.size() - 1] == '\t'))
                        return false;

                std::string value = line.substr(colon + 1);

                // Trim leading whitespace
                while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
                        value.erase(0, 1);

                // Trim trailing whitespace
                while (!value.empty() && (value[value.size() - 1] == ' ' || value[value.size() - 1] == '\t'))
                        value.erase(value.size() - 1);

                // HTTP header names are case-insensitive
                name = toLower(name);

                if (name == "content-length" &&
                        _headers.find(name) != _headers.end())
                {
                        _duplicateContentLength = true;
                }

                _headers[name] = value;
        }

        // Read body without treating '\0' as a delimiter
        std::size_t bodyStart = rawRequest.find("\r\n\r\n");

        if (bodyStart != std::string::npos)
        {
                _body = rawRequest.substr(bodyStart + 4);
        }
        else
        {
                bodyStart = rawRequest.find("\n\n");
                if (bodyStart != std::string::npos)
                        _body = rawRequest.substr(bodyStart + 2);
                else
                        _body.clear();
        }

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

// Retrieve the value of a specific header by name (case-insensitive)
const std::string &HttpRequest::getHeader(const std::string &name) const
{
        static const std::string empty;

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

void HttpRequest::setBody(const std::string &body)
{
    _body = body;
}

bool HttpRequest::hasDuplicateContentLength() const
{
    return _duplicateContentLength;
}