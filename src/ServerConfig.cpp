#include "ServerConfig.hpp"

ServerConfig::ServerConfig() // Constructor with default values
    : _host("127.0.0.1"),
      _port(8080),
      _root("./www"),
      _index("index.html"),
      _autoindex(false),
      _uploadPath(""),
      _clientMaxBodySize(2097152)
{
}

ServerConfig::~ServerConfig()
{
}

const std::string &ServerConfig::getHost() const
{
	return _host;
}

int ServerConfig::getPort() const
{
	return _port;
}

const std::string &ServerConfig::getRoot() const
{
	return _root;
}

const std::string &ServerConfig::getIndex() const
{
	return _index;
}

bool ServerConfig::getAutoIndex() const
{
	return _autoindex;
}

const std::string &ServerConfig::getUploadPath() const
{
	return _uploadPath;
}

std::size_t ServerConfig::getClientMaxBodySize() const
{
	return _clientMaxBodySize;
}

std::size_t ServerConfig::getClientMaxBodySize(const Location *location) const
{
	if (location != NULL && location->isClientMaxBodySizeSet())
		return location->getClientMaxBodySize();
	return _clientMaxBodySize;
}

std::vector<Location> &ServerConfig::getLocations()
{
    return _locations;
}

const std::vector<Location> &ServerConfig::getLocations() const
{
    return _locations;
}

// Find the best matching location for a given path
const Location *ServerConfig::findLocation(const std::string &path) const
{
    const Location *bestMatch = NULL;
    std::size_t bestLength = 0;

    for (std::vector<Location>::const_iterator it = _locations.begin(); it != _locations.end(); ++it)
    {
        const std::string &locationPath = it->getPath();

        // if the request doesnt start with locationPath
        if (path.compare(0, locationPath.size(), locationPath) != 0)
            continue;

        // valid check for paths
        bool isBoundaryMatch = (path.size() == locationPath.size()) ||
                               (locationPath == "/") ||
                               (!locationPath.empty() && locationPath[locationPath.size() - 1] == '/') ||
                               (path.size() > locationPath.size() && path[locationPath.size()] == '/');

        if (isBoundaryMatch)
        {
            if (locationPath.size() > bestLength)
            {
                bestMatch = &(*it);
                bestLength = locationPath.size();
            }
        }
    }

    return bestMatch;
}

void ServerConfig::setHost(const std::string &host)
{
	_host = host;
}

void ServerConfig::setPort(int port)
{
	_port = port;
}

void ServerConfig::setRoot(const std::string &root)
{
	_root = root;
}

void ServerConfig::setIndex(const std::string &index)
{
	_index = index;
}

void ServerConfig::setAutoIndex(bool value)
{
	_autoindex = value;
}

void ServerConfig::setUploadPath(const std::string &path)
{
	_uploadPath = path;
}

void ServerConfig::setClientMaxBodySize(std::size_t size)
{
	_clientMaxBodySize = size;
}

void ServerConfig::addLocation(const Location &location)
{
	_locations.push_back(location);
}

void ServerConfig::setErrorPage(int statusCode, const std::string &path) // Set a custom error page for a specific HTTP status code
{
    _errorPages[statusCode] = path;
}

const std::string *ServerConfig::getErrorPage(int statusCode) const // Retrieve the custom error page for a specific HTTP status code, if set
{
    std::map<int, std::string>::const_iterator it =
        _errorPages.find(statusCode);

    if (it == _errorPages.end())
        return NULL;

    return &it->second;
}

void ServerConfig::addServerName(const std::string &name)
{
    _serverNames.push_back(name);
}

const std::vector<std::string> &ServerConfig::getServerNames() const
{
    return _serverNames;
}
