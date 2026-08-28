#ifndef CGIENVBUILDER_HPP
# define CGIENVBUILDER_HPP

#include <string>
#include <vector>

class HttpRequest;

std::vector<std::string> buildCGIEnv(const HttpRequest &request, const std::string &serverName, int serverPort);

#endif

