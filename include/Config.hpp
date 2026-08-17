#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include "Location.hpp"

class Config
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
		Config();
		~Config();

		bool parse(const std::string &filename);

		const std::string &getHost() const;
		int getPort() const;
		const std::string &getRoot() const;
		const std::string &getIndex() const;
		bool getAutoIndex() const;

		const std::string &getUploadPath() const;
		
		std::size_t getClientMaxBodySize() const;

		const std::vector<Location> &getLocations() const;
		const Location *findLocation(const std::string &path) const;

		void setErrorPage(int statusCode, const std::string &path);
		const std::string *getErrorPage(int statusCode) const;
};

#endif