#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <cstdlib>

#include "common.hpp"

struct LocationBlock {
	std::string path;
	std::string root;
	std::string index;
	std::vector<std::string> allow_methods;
	bool autoindex;
	// add cgi later
};

// change the struct as the index.html can be changed in the config file
struct ServerConfig
{
	std::string host;
	std::string server_name;
	int			client_max_body_size;
	std::map<int, std::string> error_pages;
	std::vector<LocationBlock> locations;
};

class ConfigParser
{
	private:
		std::string filename;
		std::vector<ServerConfig> configs;
		std::vector<std::string> tokens;

	public:
		ConfigParser(const std::string& filename);
		~ConfigParser();
		ConfigParser(const ConfigParser& other);
		ConfigParser& operator=(const ConfigParser& other);

		std::vector<ServerConfig> parseConfig();
};

#endif
