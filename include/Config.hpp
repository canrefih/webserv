#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>
#include "ServerConfig.hpp"

class Config
{
	private:
		std::vector<ServerConfig> _servers;

	public:
		Config();
		~Config();

		bool parse(const std::string &filename);

		const std::vector<ServerConfig> &getServers() const;
		const ServerConfig *getServerByPort(int port) const;
};

#endif