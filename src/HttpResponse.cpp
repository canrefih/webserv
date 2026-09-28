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

void HttpResponse::setHeader(const std::string &name, const std::string &value)
{
        // Header Injection (CRLF) Engelleme
        if (name.find('\r') != std::string::npos ||
            name.find('\n') != std::string::npos ||
            value.find('\r') != std::string::npos ||
            value.find('\n') != std::string::npos)
        {
                return;
        }

        std::string lowerName = toLower(name);

        // Content-Type senkronizasyonu
        if (lowerName == "content-type")
        {
                _contentType = value;
                return;
        }

        // Büyük/küçük harf duyarsız olarak var olan eski header'ı temizle (Overwrite garantisi)
        for (std::map<std::string, std::string>::iterator it = _customHeaders.begin();
             it != _customHeaders.end(); ++it)
        {
                if (toLower(it->first) == lowerName)
                {
                        _customHeaders.erase(it);
                        break;
                }
        }

        _customHeaders[name] = value;
}

const std::string &HttpResponse::getHeader(const std::string &name) const
{
        static const std::string empty;
        std::string lowerName = toLower(name);

        if (lowerName == "content-type")
        {
                return _contentType;
        }

        for (std::map<std::string, std::string>::const_iterator it = _customHeaders.begin();
             it != _customHeaders.end(); ++it)
        {
                if (toLower(it->first) == lowerName)
                        return it->second;
        }

        return empty;
}

std::string HttpResponse::toString() const
{
        std::ostringstream response;

        // Status Line
        response << "HTTP/1.1 " << _statusCode << " " << _statusText << "\r\n";

        // Content-Length
        response << "Content-Length: " << _body.size() << "\r\n";

        // Content-Type (Boş değilse basılır)
        if (!_contentType.empty())
        {
                response << "Content-Type: " << _contentType << "\r\n";
        }

        // Custom Headers
        for (std::map<std::string, std::string>::const_iterator it = _customHeaders.begin();
             it != _customHeaders.end(); ++it)
        {
                std::string lowerKey = toLower(it->first);

                if (lowerKey == "content-length" || lowerKey == "content-type")
                        continue;

                response << it->first << ": " << it->second << "\r\n";
        }

        response << "\r\n";
        response << _body;

        return response.str();
}