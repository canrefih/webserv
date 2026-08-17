#include "Config.hpp"

#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>

Config::Config()
    : _host("127.0.0.1"),
      _port(8080),
      _root("./www"),
      _index("index.html"),
      _autoindex(false),
      _clientMaxBodySize(2097152)
{
}

Config::~Config()
{
}

bool Config::parse(const std::string &filename)
{
    std::ifstream file(filename.c_str());

    if (!file.is_open())
    {
        std::cerr << "Error: cannot open configuration file: "
                  << filename << std::endl;
        return false;
    }

    std::string line;
    Location *currentLocation = NULL;
    bool inServer = false;

    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string directive;
        std::string value;

        iss >> directive;

        if (directive.empty())
            continue;

        if (directive == "server")
        {
            inServer = true;
            continue;
        }

        if (directive == "location")
        {
            if (!inServer || currentLocation != NULL)
            {
                std::cerr << "Error: invalid location block"
                          << std::endl;
                return false;
            }

            iss >> value;

            if (value.empty())
            {
                std::cerr << "Error: location path missing"
                          << std::endl;
                return false;
            }

            _locations.push_back(Location(value));
            currentLocation = &_locations.back();

            continue;
        }

        if (directive == "}")
        {
            if (currentLocation != NULL)
            {
                currentLocation = NULL;
                continue;
            }

            if (inServer)
            {
                inServer = false;
                continue;
            }

            std::cerr << "Error: unexpected '}'" << std::endl;
            return false;
        }

        if (directive == "listen")
        {
            if (currentLocation != NULL)
            {
                std::cerr << "Error: listen is not allowed "
                          << "inside location" << std::endl;
                return false;
            }

            iss >> value;

            if (!value.empty() && value[value.size() - 1] == ';')
                value.erase(value.size() - 1);

            std::size_t colon = value.find(':');

            if (colon == std::string::npos)
            {
                std::cerr << "Error: invalid listen directive"
                          << std::endl;
                return false;
            }

            _host = value.substr(0, colon);
            _port = std::atoi(value.substr(colon + 1).c_str());
        }
        else if (directive == "root")
        {
            iss >> value;

            if (!value.empty() && value[value.size() - 1] == ';')
                value.erase(value.size() - 1);

            if (currentLocation != NULL)
                currentLocation->setRoot(value);
            else
                _root = value;
        }
        else if (directive == "index")
        {
            iss >> value;

            if (!value.empty() && value[value.size() - 1] == ';')
                value.erase(value.size() - 1);

            if (currentLocation != NULL)
                currentLocation->setIndex(value);
            else
                _index = value;
        }
        else if (directive == "autoindex")
        {
            iss >> value;

            if (!value.empty() && value[value.size() - 1] == ';')
                value.erase(value.size() - 1);

            if (value == "on")
            {
                if (currentLocation != NULL)
                    currentLocation->setAutoIndex(true);
                else
                    _autoindex = true;
            }
            else if (value == "off")
            {
                if (currentLocation != NULL)
                    currentLocation->setAutoIndex(false);
                else
                    _autoindex = false;
            }
            else
            {
                std::cerr << "Error: invalid autoindex value"
                          << std::endl;
                return false;
            }
        }
        else if (directive == "client_max_body_size")
        {
            if (currentLocation != NULL)
            {
                std::cerr << "Error: client_max_body_size "
                          << "must be in server block"
                          << std::endl;
                return false;
            }

            iss >> value;

            if (!value.empty() && value[value.size() - 1] == ';')
                value.erase(value.size() - 1);

            std::size_t multiplier = 1;

            if (!value.empty())
            {
                char unit = value[value.size() - 1];

                if (unit == 'M')
                {
                    multiplier = 1024 * 1024;
                    value.erase(value.size() - 1);
                }
                else if (unit == 'K')
                {
                    multiplier = 1024;
                    value.erase(value.size() - 1);
                }
            }

            _clientMaxBodySize =
                std::atoi(value.c_str()) * multiplier;
        }
        else if (directive == "error_page")
        {
            if (currentLocation != NULL)
            {
                std::cerr << "Error: error_page must be "
                        << "inside server block"
                        << std::endl;
                return false;
            }

            std::string codeString;
            std::string path;

            iss >> codeString;
            iss >> path;

            if (!codeString.empty() &&
                codeString[codeString.size() - 1] == ';')
                codeString.erase(codeString.size() - 1);

            if (!path.empty() &&
                path[path.size() - 1] == ';')
                path.erase(path.size() - 1);

            if (codeString.empty() || path.empty())
            {
                std::cerr << "Error: invalid error_page directive"
                        << std::endl;
                return false;
            }

            int statusCode = std::atoi(codeString.c_str());

            if (statusCode < 400 || statusCode > 599)
            {
                std::cerr << "Error: invalid error page status code"
                        << std::endl;
                return false;
            }

            _errorPages[statusCode] = path;
        }
        else if (directive == "allow_methods")
        {
            if (currentLocation == NULL)
            {
                std::cerr << "Error: allow_methods must be "
                          << "inside location"
                          << std::endl;
                return false;
            }

            while (iss >> value)
            {
                if (!value.empty() && value[value.size() - 1] == ';')
                    value.erase(value.size() - 1);

                if (!value.empty())
                    currentLocation->addMethod(value);
            }
        }
        else if (directive == "upload")
        {
            if (currentLocation == NULL)
            {
                std::cerr << "Error: upload must be "
                          << "inside location"
                          << std::endl;
                return false;
            }

            iss >> value;

            if (!value.empty() && value[value.size() - 1] == ';')
                value.erase(value.size() - 1);

            if (value == "on")
                currentLocation->setUpload(true);
            else if (value == "off")
                currentLocation->setUpload(false);
            else
            {
                std::cerr << "Error: invalid upload value"
                          << std::endl;
                return false;
            }
        }
        else if (directive == "upload_store")
        {
            if (currentLocation == NULL)
            {
                std::cerr << "Error: upload_store must be "
                          << "inside location"
                          << std::endl;
                return false;
            }

            iss >> value;

            if (!value.empty() && value[value.size() - 1] == ';')
                value.erase(value.size() - 1);

            currentLocation->setUploadStore(value);
        }
        else
        {
            std::cerr << "Error: unknown directive: "
                      << directive << std::endl;
            return false;
        }
    }

    if (currentLocation != NULL || inServer)
    {
        std::cerr << "Error: unclosed configuration block"
                  << std::endl;
        return false;
    }

    return true;
}

const std::string &Config::getHost() const // getter for the host
{
	return _host;
}

int Config::getPort() const // getter for the port
{
	return _port;
}

const std::string &Config::getRoot() const // getter for the root directory
{
	return _root;
}

const std::string &Config::getIndex() const // getter for the index file
{
	return _index;
}

bool Config::getAutoIndex() const
{
	return _autoindex;
}

const std::string &Config::getUploadPath() const
{
	return _uploadPath;
}

std::size_t Config::getClientMaxBodySize() const
{
	return _clientMaxBodySize;
}

const std::vector<Location> &Config::getLocations() const
{
    return _locations;
}

const Location *Config::findLocation(const std::string &path) const
{
    const Location *bestMatch = NULL;
    std::size_t bestLength = 0;

    std::vector<Location>::const_iterator it;

    for (it = _locations.begin(); it != _locations.end(); ++it)
    {
        const std::string &locationPath = it->getPath();

        if (path.compare(0, locationPath.size(), locationPath) != 0)
            continue;

        if (path.size() == locationPath.size())
        {
            if (locationPath.size() > bestLength)
            {
                bestMatch = &(*it);
                bestLength = locationPath.size();
            }
        }
        else if (locationPath == "/" || path[locationPath.size()] == '/')
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

void Config::setErrorPage(int statusCode, const std::string &path)
{
    _errorPages[statusCode] = path;
}

const std::string *Config::getErrorPage(int statusCode) const
{
    std::map<int, std::string>::const_iterator it =
        _errorPages.find(statusCode);

    if (it == _errorPages.end())
        return NULL;

    return &it->second;
}