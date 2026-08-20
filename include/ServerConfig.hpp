#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include "Location.hpp"

class ServerConfig
{
	private:
		std::string	_host;
		int			_port;
		std::string	_root;
		std::string	_index;
		bool        _autoindex;
		std::string _uploadPath;
		std::size_t _clientMaxBodySize;
		std::vector<Location> _locations;
		std::map<int, std::string> _errorPages;

	public:
		ServerConfig();
		~ServerConfig();

		const std::string &getHost() const;
		int getPort() const;
		const std::string &getRoot() const;
		const std::string &getIndex() const;
		bool getAutoIndex() const;

		const std::string &getUploadPath() const;

		std::size_t getClientMaxBodySize() const;

		std::vector<Location> &getLocations();
		const std::vector<Location> &getLocations() const;
		const Location *findLocation(const std::string &path) const;

		void setHost(const std::string &host);
		void setPort(int port);
		void setRoot(const std::string &root);
		void setIndex(const std::string &index);
		void setAutoIndex(bool value);
		void setUploadPath(const std::string &path);
		void setClientMaxBodySize(std::size_t size);

		void addLocation(const Location &location);
		void setErrorPage(int statusCode, const std::string &path);
		const std::string *getErrorPage(int statusCode) const;
};

#endif
