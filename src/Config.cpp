#include "Config.hpp"
#include "Location.hpp"

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
    _servers.clear();

    std::string line;
    ServerConfig *currentServer = NULL;
    Location *currentLocation = NULL;
    bool listenSet = false;
    bool serverRootSet = false;
    bool locationRootSet = false;

    std::ifstream file(filename.c_str());

    if (!file.is_open())
    {
        std::cerr << "Error: cannot open configuration file: "
                  << filename << std::endl;
        return false;
    }

    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string directive;
        std::string value;

        iss >> directive;

        if (directive.empty() || directive[0] == '#')
            continue;

        if (!directive.empty() &&
            directive[directive.size() - 1] == ';')
        {
            std::cerr << "Error: missing value for directive '"
                    << directive.substr(0, directive.size() - 1)
                    << "'" << std::endl;
            return false;
        }

        if (directive == "server")
        {
            if (currentServer != NULL)
            {
                std::cerr << "Error: nested server blocks not allowed"
                        << std::endl;
                return false;
            }

            iss >> value;

            // Eğer 'server' kelimesinden sonra aynı satırda token yoksa (yeni satıra geçildiyse),
            // '{' işaretini bulana kadar alt satırları okur.
            if (value.empty())
            {
                while (std::getline(file, line))
                {
                    std::size_t commentPos = line.find('#');
                    if (commentPos != std::string::npos)
                    {
                        line = line.substr(0, commentPos);
                    }

                    iss.str(line);
                    iss.clear();

                    if (iss >> value)
                    {
                        break;
                    }
                }
            }

            if (value != "{")
            {
                std::cerr << "Error: expected '{' after 'server'"
                        << std::endl;
                return false;
            }

            std::string extra;
            if (iss >> extra)
            {
                std::cerr << "Error: unexpected token after 'server {'"
                        << std::endl;
                return false;
            }

            _servers.push_back(ServerConfig());
            currentServer = &_servers.back();
            listenSet = false;
            serverRootSet = false;
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
            {
                value.erase(value.size() - 1);

                std::string extra;
                if (iss >> extra)
                {
                    std::cerr << "Error: extra token after location '{'"
                            << std::endl;
                    return false;
                }
            }
            else
            {
                std::string brace;
                iss >> brace;

                if (brace != "{")
                {
                    std::cerr << "Error: expected '{' after location path"
                            << std::endl;
                    return false;
                }

                std::string extra;
                if (iss >> extra)
                {
                    std::cerr << "Error: extra token after location '{'"
                            << std::endl;
                    return false;
                }
            }

            if (value.empty())
            {
                std::cerr << "Error: location path missing" << std::endl;
                return false;
            }

            Location loc(value);
            currentServer->addLocation(loc);
            currentLocation = &currentServer->getLocations().back();
            locationRootSet = false;

            continue;
        }

        if (directive == "}")
        {
            std::string extra;
            if (iss >> extra)
            {
                std::cerr << "Error: extra token after '}'"
                        << std::endl;
                return false;
            }

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
            if (listenSet)
            {
                std::cerr << "Error: duplicate listen directive"
                        << std::endl;
                return false;
            }
            if (currentLocation != NULL)
            {
                std::cerr << "Error: listen is not allowed "
                        << "inside location" << std::endl;
                return false;
            }

            iss >> value;

            if (value.empty())
            {
                std::cerr << "Error: listen value missing"
                        << std::endl;
                return false;
            }

            if (value[value.size() - 1] != ';')
            {
                std::string extra;
                if (iss >> extra)
                {
                    std::cerr << "Error: extra token in listen directive"
                            << std::endl;
                }
                else
                {
                    std::cerr << "Error: listen directive must end with ';'"
                            << std::endl;
                }
                return false;
            }

            value.erase(value.size() - 1);

            std::string extra;
            if (iss >> extra)
            {
                std::cerr << "Error: extra token in listen directive"
                        << std::endl;
                return false;
            }

            std::size_t colon = value.find(':');
            std::string host;
            std::string portString;

            if (colon == std::string::npos)
            {
                // Sadece port girildiğinde (örneğin: listen 8080;) varsayılan IP olarak 0.0.0.0 atanır.
                host = "0.0.0.0";
                portString = value;
            }
            else
            {
                // IP:PORT şeklinde girildiğinde (örneğin: listen 127.0.0.1:8080;)
                host = value.substr(0, colon);
                portString = value.substr(colon + 1);
            }

            if (host.empty() || portString.empty())
            {
                std::cerr << "Error: invalid listen directive: "
                        << value << std::endl;
                return false;
            }

            for (std::size_t i = 0; i < portString.size(); ++i)
            {
                if (portString[i] < '0' || portString[i] > '9')
                {
                    std::cerr << "Error: invalid listen port: "
                            << portString << std::endl;
                    return false;
                }
            }

            int port = std::atoi(portString.c_str());

            if (port < 1 || port > 65535)
            {
                std::cerr << "Error: invalid listen port: "
                        << portString << std::endl;
                return false;
            }

            currentServer->setHost(host);
            currentServer->setPort(port);
            listenSet = true;
        }
        else if (directive == "server_name") // Handle the "server_name" directive
        {
            if (currentLocation != NULL)
            {
                std::cerr << "Error: server_name is not allowed inside location"
                        << std::endl;
                return false;
            }

            std::string token;
            bool semicolonFound = false;

            while (iss >> token)
            {
                if (!token.empty() && token[token.size() - 1] == ';')
                {
                    token.erase(token.size() - 1);
                    semicolonFound = true;
                    if (!token.empty())
                    {
                        currentServer->addServerName(token);
                    }
                    break;
                }

                if (!token.empty())
                {
                    currentServer->addServerName(token);
                }
            }

            if (!semicolonFound)
            {
                std::cerr << "Error: server_name directive must end with ';'"
                        << std::endl;
                return false;
            }
        }
        else if (directive == "return") // Handle HTTP redirection (e.g. return 301 /new_path;)
        {
            if (currentLocation == NULL)
            {
                std::cerr << "Error: return directive outside location block"
                        << std::endl;
                return false;
            }

            std::string token1, token2;
            iss >> token1;

            if (token1.empty())
            {
                std::cerr << "Error: return directive missing parameters"
                        << std::endl;
                return false;
            }

            int redirectCode = 302;
            std::string redirectPath = "";

            if (token1[token1.size() - 1] == ';')
            {
                token1.erase(token1.size() - 1);
                redirectPath = token1;
            }
            else
            {
                iss >> token2;
                if (token2.empty() || token2[token2.size() - 1] != ';')
                {
                    std::cerr << "Error: return directive must end with ';'"
                            << std::endl;
                    return false;
                }
                token2.erase(token2.size() - 1);

                bool isNumber = true;
                for (std::size_t i = 0; i < token1.size(); ++i)
                {
                    if (token1[i] < '0' || token1[i] > '9')
                    {
                        isNumber = false;
                        break;
                    }
                }

                if (isNumber)
                {
                    redirectCode = std::atoi(token1.c_str());
                    redirectPath = token2;

                    if (redirectCode != 301 && redirectCode != 302 && redirectCode != 303
                        && redirectCode != 307 && redirectCode != 308)
                    {
                        std::cerr << "Error: return code must be a redirection (301, 302, 303, 307 or 308)"
                                << std::endl;
                        return false;
                    }
                }
                else
                {
                    std::cerr << "Error: invalid status code in return directive"
                            << std::endl;
                    return false;
                }
            }

            std::string extra;
            if (iss >> extra)
            {
                std::cerr << "Error: extra token in return directive"
                        << std::endl;
                return false;
            }

            currentLocation->setRedirection(redirectCode, redirectPath);
        }
        else if (directive == "root") // Handle the "root" directive, which specifies the root directory for serving files. This directive can be inside a server or location block.
        {
            iss >> value;

            if (value.empty())
            {
                std::cerr << "Error: root value missing"
                        << std::endl;
                return false;
            }

            if (value[value.size() - 1] != ';')
            {
                std::string extra;

                if (iss >> extra)
                {
                    std::cerr << "Error: extra token in root directive"
                            << std::endl;
                }
                else
                {
                    std::cerr << "Error: root directive must end with ';'"
                            << std::endl;
                }

                return false;
            }

            value.erase(value.size() - 1);

            std::string extra;
            if (iss >> extra)
            {
                std::cerr << "Error: extra token in root directive"
                        << std::endl;
                return false;
            }

            if (value.empty())
            {
                std::cerr << "Error: root value missing"
                        << std::endl;
                return false;
            }

            if (currentLocation != NULL)
            {
                if (locationRootSet)
                {
                    std::cerr << "Error: duplicate root directive"
                            << std::endl;
                    return false;
                }

                currentLocation->setRoot(value);
                locationRootSet = true;
            }
            else
            {
                if (serverRootSet)
                {
                    std::cerr << "Error: duplicate root directive"
                            << std::endl;
                    return false;
                }

                currentServer->setRoot(value);
                serverRootSet = true;
            }
        }
        else if (directive == "index") // Handle the "index" directive, which specifies the default file to serve when a directory is requested. This directive must be inside a server or location block.
        {
            iss >> value;

            if (value.empty())
            {
                std::cerr << "Error: index value missing"
                        << std::endl;
                return false;
            }

            if (value[value.size() - 1] != ';')
            {
                std::string extra;
                if (iss >> extra)
                {
                    std::cerr << "Error: extra token in index directive"
                            << std::endl;
                }
                else
                {
                    std::cerr << "Error: index directive must end with ';'"
                            << std::endl;
                }
                return false;
            }

            value.erase(value.size() - 1);

            if (value.empty())
            {
                std::cerr << "Error: index value missing"
                        << std::endl;
                return false;
            }

            std::string extra;
            if (iss >> extra)
            {
                std::cerr << "Error: extra token in index directive"
                        << std::endl;
                return false;
            }

            if (currentLocation != NULL)
                currentLocation->setIndex(value);
            else
                currentServer->setIndex(value);
        }
        else if (directive == "autoindex") // Handle the "autoindex" directive, which specifies whether to display a list of files in a directory. This directive must be inside a server or location block.
        {
            iss >> value;

            if (value.empty())
            {
                std::cerr << "Error: autoindex value missing"
                        << std::endl;
                return false;
            }

            if (value[value.size() - 1] != ';')
            {
                std::string extra;
                if (iss >> extra)
                {
                    std::cerr << "Error: extra token in autoindex directive"
                            << std::endl;
                }
                else
                {
                    std::cerr << "Error: autoindex directive must end with ';'"
                            << std::endl;
                }
                return false;
            }

            value.erase(value.size() - 1);

            if (value.empty())
            {
                std::cerr << "Error: autoindex value missing"
                        << std::endl;
                return false;
            }

            std::string extra;
            if (iss >> extra)
            {
                std::cerr << "Error: extra token in autoindex directive"
                        << std::endl;
                return false;
            }

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
        else if (directive == "client_max_body_size") // Handle the "client_max_body_size" directive, which specifies the maximum allowed size of the request body. In a server block it is the default; in a location block it overrides the server value for that location (as in nginx).
        {
            iss >> value;

            if (value.empty())
            {
                std::cerr << "Error: invalid client_max_body_size"
                        << std::endl;
                return false;
            }

            if (value[value.size() - 1] != ';')
            {
                std::string extra;
                if (iss >> extra)
                {
                    std::cerr << "Error: extra token in "
                            << "client_max_body_size directive"
                            << std::endl;
                }
                else
                {
                    std::cerr << "Error: client_max_body_size "
                            << "directive must end with ';'"
                            << std::endl;
                }
                return false;
            }

            value.erase(value.size() - 1);

            if (value.empty())
            {
                std::cerr << "Error: invalid client_max_body_size"
                        << std::endl;
                return false;
            }

            std::size_t multiplier = 1;
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

            if (value.empty())
            {
                std::cerr << "Error: invalid client_max_body_size"
                        << std::endl;
                return false;
            }

            for (std::size_t i = 0; i < value.size(); ++i)
            {
                if (value[i] < '0' || value[i] > '9')
                {
                    std::cerr << "Error: invalid client_max_body_size"
                            << std::endl;
                    return false;
                }
            }

            std::size_t size = static_cast<std::size_t>(
                std::atol(value.c_str()));

            if (size == 0)
            {
                std::cerr << "Error: invalid client_max_body_size"
                        << std::endl;
                return false;
            }

            if (size > static_cast<std::size_t>(-1) / multiplier)
            {
                std::cerr << "Error: client_max_body_size too large"
                        << std::endl;
                return false;
            }

            std::string extra;
            if (iss >> extra)
            {
                std::cerr << "Error: extra token in "
                        << "client_max_body_size directive"
                        << std::endl;
                return false;
            }

            if (currentLocation != NULL)
                currentLocation->setClientMaxBodySize(size * multiplier);
            else
                currentServer->setClientMaxBodySize(size * multiplier);
        }
        else if (directive == "error_page") // Handle the "error_page" directive, which specifies a custom error page for specific HTTP status codes. This directive must be inside a server block and not inside a location block.
        {
            if (currentLocation != NULL)
            {
                std::cerr << "Error: error_page must be "
                        << "inside server block"
                        << std::endl;
                return false;
            }

            std::vector<std::string> tokens;
            std::string token;

            while (iss >> token)
            {
                tokens.push_back(token);
            }

            if (tokens.size() < 2)
            {
                std::cerr << "Error: error_page status code or path missing"
                        << std::endl;
                return false;
            }

            std::string path = tokens.back();

            if (path[path.size() - 1] != ';')
            {
                std::cerr << "Error: error_page directive must end with ';'"
                        << std::endl;
                return false;
            }

            path.erase(path.size() - 1);

            if (path.empty())
            {
                std::cerr << "Error: invalid error_page path"
                        << std::endl;
                return false;
            }

            // Son eleman (path) haricindeki tum token'lar status code'lardir
            for (std::size_t k = 0; k < tokens.size() - 1; ++k)
            {
                std::string codeString = tokens[k];

                for (std::size_t i = 0; i < codeString.size(); ++i)
                {
                    if (codeString[i] < '0' || codeString[i] > '9')
                    {
                        std::cerr << "Error: invalid error page status code"
                                << std::endl;
                        return false;
                    }
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

            bool methodFound = false;
            bool semicolonFound = false;

            while (iss >> value)
            {
                if (!value.empty() && value[value.size() - 1] == ';')
                {
                    value.erase(value.size() - 1);
                    semicolonFound = true;
                }

                if (value.empty())
                    continue;

                if (value != "GET" &&
                    value != "POST" &&
                    value != "DELETE" &&
                    value != "HEAD")
                {
                    std::cerr << "Error: invalid allow_methods value: "
                            << value << std::endl;
                    return false;
                }

                currentLocation->addMethod(value);
                methodFound = true;
            }

            if (!methodFound)
            {
                std::cerr << "Error: allow_methods value missing"
                        << std::endl;
                return false;
            }

            if (!semicolonFound)
            {
                std::cerr << "Error: allow_methods directive "
                        << "must end with ';'"
                        << std::endl;
                return false;
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

            if (value.empty())
            {
                std::cerr << "Error: upload value missing"
                        << std::endl;
                return false;
            }

            if (value[value.size() - 1] != ';')
            {
                std::string extra;
                if (iss >> extra)
                {
                    std::cerr << "Error: extra token in upload directive"
                            << std::endl;
                }
                else
                {
                    std::cerr << "Error: upload directive must end with ';'"
                            << std::endl;
                }
                return false;
            }

            value.erase(value.size() - 1);

            if (value.empty())
            {
                std::cerr << "Error: upload value missing"
                        << std::endl;
                return false;
            }

            std::string extra;
            if (iss >> extra)
            {
                std::cerr << "Error: extra token in upload directive"
                        << std::endl;
                return false;
            }

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
        else if (directive == "cgi_extension")
        {
            if (currentLocation == NULL)
            {
                std::cerr << "Error: cgi_extension must be "
                        << "inside location"
                        << std::endl;
                return false;
            }

            std::string extension;
            std::string interpreterPath;

            iss >> extension;
            iss >> interpreterPath;

            if (extension.empty() || extension == ";")
            {
                std::cerr << "Error: CGI extension missing"
                        << std::endl;
                return false;
            }

            if (interpreterPath.empty())
            {
                std::cerr << "Error: CGI interpreter missing"
                        << std::endl;
                return false;
            }

            /*
            * Optional "virtual" flag (same idea as Apache's "Action ... virtual"):
            * the CGI program is run even if the requested file does not exist,
            * for programs that don't need the script file (e.g. the 42 cgi_tester).
            *
            *   cgi_extension .bla ./cgi_tester virtual;
            */
            bool isVirtual = false;

            if (interpreterPath[interpreterPath.size() - 1] == ';')
                interpreterPath.erase(interpreterPath.size() - 1);
            else
            {
                std::string flag;

                if (!(iss >> flag) || flag == "virtual")
                {
                    std::cerr << "Error: cgi_extension directive must end with ';'"
                            << std::endl;
                    return false;
                }

                if (flag != "virtual;")
                {
                    std::cerr << "Error: extra token in cgi_extension directive"
                            << std::endl;
                    return false;
                }

                isVirtual = true;
            }

            if (interpreterPath.empty())
            {
                std::cerr << "Error: CGI interpreter missing"
                        << std::endl;
                return false;
            }

            std::string extra;
            if (iss >> extra)
            {
                std::cerr << "Error: extra token in cgi_extension directive"
                        << std::endl;
                return false;
            }

            currentLocation->addCgiExtension(extension, interpreterPath);

            if (isVirtual)
                currentLocation->setCgiVirtual(extension);
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

            if (value.empty())
            {
                std::cerr << "Error: upload_store value missing"
                        << std::endl;
                return false;
            }

            if (value[value.size() - 1] != ';')
            {
                std::string extra;
                if (iss >> extra)
                {
                    std::cerr << "Error: extra token in upload_store directive"
                            << std::endl;
                }
                else
                {
                    std::cerr << "Error: upload_store directive "
                            << "must end with ';'"
                            << std::endl;
                }
                return false;
            }

            value.erase(value.size() - 1);

            if (value.empty())
            {
                std::cerr << "Error: upload_store value missing"
                        << std::endl;
                return false;
            }

            std::string extra;
            if (iss >> extra)
            {
                std::cerr << "Error: extra token in upload_store directive"
                        << std::endl;
                return false;
            }

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