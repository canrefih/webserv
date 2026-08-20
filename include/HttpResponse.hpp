#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>
#include <map>

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

		void setStatus(int code, const std::string &text);
		void setBody(const std::string &body);
		void setContentType(const std::string &contentType);
		void setHeader(const std::string &name, const std::string &value);

		std::string toString() const;
};

#endif