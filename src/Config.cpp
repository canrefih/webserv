#include "Config.hpp"

#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>

Config::Config()
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
    ServerConfig *currentServer = NULL;
    Location *currentLocation = NULL;

    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string directive;
        std::string value;

        iss >> directive;

        if (directive.empty() || directive[0] == '#')
            continue;

        if (directive == "server")
        {
            iss >> value;
            if (value != "{")
            {
                std::cerr << "Error: expected '{' after 'server'"
                          << std::endl;
                return false;
            }

            _servers.push_back(ServerConfig());
            currentServer = &_servers.back();
            currentLocation = NULL;
            continue;
        }

        if (currentServer == NULL)
        {
            std::cerr << "Error: directive '" << directive
                      << "' outside server block" << std::endl;
            return false;
        }

        if (directive == "location")
        {
            if (currentLocation != NULL)
            {
                std::cerr << "Error: nested location blocks not allowed"
                          << std::endl;
                return false;
            }

            iss >> value;

            if (value.empty())
            {
                std::cerr << "Error: location path missing" << std::endl;
                return false;
            }

            if (value[value.size() - 1] == '{')
                value.erase(value.size() - 1);

            Location loc(value);
            currentServer->addLocation(loc);
            currentLocation = &currentServer->getLocations().back();

            continue;
        }

        if (directive == "}")
        {
            if (currentLocation != NULL)
            {
                currentLocation = NULL;
                continue;
            }

            if (currentServer != NULL)
            {
                currentServer = NULL;
                continue;
            }

            std::cerr << "Error: unexpected '}'" << std::endl;
            return false;
        }

        if (directive == "listen") // Handle the "listen" directive, which specifies the host and port for the server to listen on. This directive must be inside a server block and not inside a location block.
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
                std::cerr << "Error: invalid listen directive: " << value
                          << std::endl;
                return false;
            }

            std::string host = value.substr(0, colon);
            int port = std::atoi(value.substr(colon + 1).c_str());

            currentServer->setHost(host);
            currentServer->setPort(port);
        }
        else if (directive == "root") // Handle the "root" directive, which specifies the root directory for serving files. This directive can be inside a server or location block.
        {
            iss >> value;

            if (!value.empty() && value[value.size() - 1] == ';')
                value.erase(value.size() - 1);

            if (currentLocation != NULL)
                currentLocation->setRoot(value);
            else
                currentServer->setRoot(value);
        }
        else if (directive == "index") // Handle the "index" directive, which specifies the default file to serve when a directory is requested. This directive must be inside a server or location block.
        {
            iss >> value;

            if (!value.empty() && value[value.size() - 1] == ';')
                value.erase(value.size() - 1);

            if (currentLocation != NULL)
                currentLocation->setIndex(value);
            else
                currentServer->setIndex(value);
        }
        else if (directive == "autoindex") // Handle the "autoindex" directive, which specifies whether to display a list of files in a directory. This directive must be inside a server or location block.
        {
            iss >> value;

            if (!value.empty() && value[value.size() - 1] == ';')
                value.erase(value.size() - 1);

            if (value == "on")
            {
                if (currentLocation != NULL)
                    currentLocation->setAutoIndex(true);
                else
                    currentServer->setAutoIndex(true);
            }
            else if (value == "off")
            {
                if (currentLocation != NULL)
                    currentLocation->setAutoIndex(false);
                else
                    currentServer->setAutoIndex(false);
            }
            else
            {
                std::cerr << "Error: invalid autoindex value"
                          << std::endl;
                return false;
            }
        }
        else if (directive == "client_max_body_size") // Handle the "client_max_body_size" directive, which specifies the maximum allowed size of the request body for a server. This directive must be inside a server block and not inside a location block.
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

            currentServer->setClientMaxBodySize(
                std::atoi(value.c_str()) * multiplier);
        }
        else if (directive == "error_page") // Handle the "error_page" directive, which specifies a custom error page for a specific HTTP status code. This directive must be inside a server block and not inside a location block.
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

            currentServer->setErrorPage(statusCode, path);
        }
        else if (directive == "allow_methods") // Handle the "allow_methods" directive, which specifies the allowed HTTP methods (e.g., GET, POST, DELETE) for a specific location. This directive must be inside a location block.
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
        else if (directive == "upload") // Handle the "upload" directive, which specifies whether file uploads are allowed for a specific location. This directive must be inside a location block.
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

    if (currentLocation != NULL || currentServer != NULL)
    {
        std::cerr << "Error: unclosed configuration block"
                  << std::endl;
        return false;
    }

    if (_servers.empty())
    {
        std::cerr << "Error: no server blocks found"
                  << std::endl;
        return false;
    }

    return true;
}

const std::vector<ServerConfig> &Config::getServers() const
{
    return _servers;
}

// Retrieve a server configuration by its listening port. If no server is found for the specified port, return NULL.
const ServerConfig *Config::getServerByPort(int port) const
{
    std::vector<ServerConfig>::const_iterator it;

    for (it = _servers.begin(); it != _servers.end(); ++it)
    {
        if (it->getPort() == port)
            return &(*it);
    }

    return NULL;
}
