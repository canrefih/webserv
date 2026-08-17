#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>
#include <map>

class HttpRequest
{
	private:
		std::string				_method;
		std::string				_target;
		std::string				_version;
		std::map<std::string, std::string> _headers;
		std::string _body;

	public:
		HttpRequest();
		~HttpRequest();

		bool parse(const std::string &rawRequest);

		const std::string &getMethod() const;
		const std::string &getTarget() const;
		const std::string &getVersion() const;
		const std::string &getBody() const;
		const std::string &getHeader(const std::string &name) const;
		const std::map<std::string, std::string> &getHeaders() const;
};

#endif