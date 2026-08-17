#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>

class HttpResponse
{
	private:
		int			_statusCode;
		std::string	_statusText;
		std::string	_body;
		std::string	_contentType;

	public:
		HttpResponse();
		~HttpResponse();

		void setStatus(int code, const std::string &text);
		void setBody(const std::string &body);
		void setContentType(const std::string &contentType);

		std::string toString() const;
};

#endif