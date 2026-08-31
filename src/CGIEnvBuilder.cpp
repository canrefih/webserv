#include "CGIEnvBuilder.hpp"
#include "HttpRequest.hpp"
#include <sstream>
#include <cctype>
#include <map>

/*
Converts an HTTP header name into its CGI/1.1 env var form,
e.g. "User-Agent" -> "HTTP_USER_AGENT".
*/
static std::string toEnvHeader (const std::string &name)
{
	std::string result = "HTTP_";
	for (std::size_t i = 0; i < name.size(); i++)
	{
		char c = name[i];
		if (c == '-')
			result += '_';
		else
			result += static_cast<char>(std::toupper(static_cast<unsigned char>(c))); /*cast in unSchar because toupper use int*/
	}
	return (result);
}

/*
Builds the full list of CGI/1.1 "NAME=value" env strings for one
HTTP request, ready to be handed to CGIHandler::setup(). Pure
function: reads the request/config, never touches fork/pipe/exec.
*/
std::vector<std::string> buildCGIEnv(const HttpRequest &request, const std::string &serverName, int serverPort)
{
	std::vector<std::string> env;
	std::string scriptName;
	std::string queryString;
	std::ostringstream portStream;
	std::ostringstream lenStream;


	/*split "target" (e.g. "/cgi-bin/hello.py?x=1") into the script
	path (SCRIPT_NAME) and the query string (QUERY_STRING)*/
	std::string target = request.getTarget().getPath();
	queryString = request.getTarget().getQuery(); // contains only the "x=1" part
	portStream << serverPort;
	std::string portStr = portStream.str();
	lenStream << request.getBody().size();
	std::string lenStr = lenStream.str();

	/* base CGI/1.1 meta-variables (see RFC 3875)*/
	env.push_back("REQUEST_METHOD=" + request.getMethod());
	env.push_back("SCRIPT_NAME=" + scriptName);
	env.push_back("QUERY_STRING=" + queryString);
	env.push_back("SERVER_PROTOCOL=" + request.getVersion());
	env.push_back("SERVER_NAME=" + serverName);
	env.push_back("SERVER_PORT=" + portStr);
	env.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env.push_back("CONTENT_LENGTH=" + lenStr);
	env.push_back("CONTENT_TYPE=" + request.getHeader("Content-Type"));

	const std::map<std::string, std::string> &headers = request.getHeaders();
	std::map<std::string, std::string>::const_iterator it = headers.begin();
	for (; it != headers.end(); ++it)
	{
		if (it->first == "content-type" || it->first == "content-length")
			continue;
		env.push_back(toEnvHeader(it->first) + "=" + it->second);
	}
	return (env);
}
