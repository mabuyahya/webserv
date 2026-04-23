#include <iostream>
#include <vector>
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <sys/epoll.h>
#include <fstream>

#include "include/config_parser.hpp"
#include "include/server.hpp"

#define MAX_EVENTS 64

int main(int argc, char *argv[])
{
	/* ! the subject says:
		Your program must use a configuration file, provided as an argument on the com-mand line,
		or available in a default path.
		So this check may be unnecessary.
	*/
	if (argc < 2)
	{
		std::cerr << "Usage: " << argv[0] << " <Config File>" << std::endl;
		return 1;
	}
	if (argc == 3)
	{
		std::cout << "INFO: mock info mode" << std::endl;

		// Create a default server config for mock mode
		ServerConfig mock_config;
		mock_config.host = "127.0.0.1:8080";
		mock_config.server_name = "mock_server";
		mock_config.client_max_body_size = 1048576;

		try
		{
			Server server(mock_config);
			server.launch();
			server.run();
		}
		catch (const std::exception &e)
		{
			std::cerr << "Error: " << e.what() << std::endl;
			return 1;
		}
		return 0;
	}

	// * Config parsing and server startup
	try
	{
		ConfigParser parser(argv[1]);
		std::vector<ServerConfig> configs = parser.parseConfig();

		for (size_t i = 0; i < configs.size(); ++i)
		{
			std::cout << "Server " << i + 1 << ":" << std::endl;
			std::cout << "  Host: " << configs[i].host << std::endl;
			std::cout << "  Server Name: " << configs[i].server_name << std::endl;
			std::cout << "  Client Max Body Size: " << configs[i].client_max_body_size << std::endl;
			std::cout << "  Error Pages:" << std::endl;
			for (std::map<int, std::string>::iterator it = configs[i].error_pages.begin(); it != configs[i].error_pages.end(); ++it)
			{
				std::cout << "    " << it->first << ": " << it->second << std::endl;
			}
			std::cout << "  Locations:" << std::endl;
			for (size_t j = 0; j < configs[i].locations.size(); ++j)
			{
				std::cout << "    Path: " << configs[i].locations[j].path << std::endl;
				std::cout << "    Root: " << configs[i].locations[j].root << std::endl;
				std::cout << "    Index: " << configs[i].locations[j].index << std::endl;
				std::cout << "    Allow Methods: ";
				for (size_t k = 0; k < configs[i].locations[j].allow_methods.size(); ++k)
				{
					std::cout << configs[i].locations[j].allow_methods[k] << " ";
				}
				std::cout << std::endl;
				std::cout << "    Autoindex: " << (configs[i].locations[j].autoindex ? "on" : "off") << std::endl;
			}
		}

		// Create and launch server using the first configuration
		Server server(configs[0]);
		server.launch();
		server.run();
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
