#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "common.hpp"

// change the struct as the index.html can be changed in the config file
struct ServerConfig
{
	std::string host;
	int port;
	std::string server_name;

	std::string root_static;
	std::vector<HTTPMethod> allowed_methods_static;

	std::string upload;
	std::vector<HTTPMethod> allowed_methods_upload;
};

class ConfigParser
{
	private:
		std::string filename;
		ServerConfig config;

	public:
		ConfigParser(const std::string& filename);
		~ConfigParser();
		ConfigParser(const ConfigParser& other);
		ConfigParser& operator=(const ConfigParser& other);

		ServerConfig parseConfig();
};
