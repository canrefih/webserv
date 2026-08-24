#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>
#include <map>

/**
 * Represents an HTTP request with its method, target, version, headers, and body.
 * Provides functionality to parse a raw HTTP request string into its components.
 * HttpRequest request;
* request.setMethod("GET");
request.setTarget("/index.html");
request.setVersion("HTTP/1.1");
response.setContentType(...);
*/
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

		const std::string &getMethod() const; // Returns the HTTP method (e.g., GET, POST) of the request
		const std::string &getTarget() const; // Returns the target (path) of the request (e.g., /index.html)
		const std::string &getVersion() const; // Returns the HTTP version of the request (e.g., HTTP/1.1)
		const std::string &getBody() const; // Returns the body of the request (if any)
		const std::string &getHeader(const std::string &name) const; // Returns the value of a specific header by name (case-insensitive)
		const std::map<std::string, std::string> &getHeaders() const; // Returns a map of all headers in the request, where the key is the header name and the value is the header value
};

#endif