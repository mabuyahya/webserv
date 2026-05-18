#include <iostream>
#include "includes/ConfigParser.hpp"

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return 1;
    }

    try {
        ConfigParser parser(argv[1]);
        parser.parse();
        
        std::vector<ServerConfig> servers = parser.getServers();
        std::cout << "Successfully parsed " << servers.size() << " server(s)" << std::endl;
        
        for (size_t i = 0; i < servers.size(); ++i) {
            std::cout << "\nServer " << (i + 1) << ":" << std::endl;
            std::cout << "  Listen: " << servers[i].getHost() << ":" << servers[i].getPort() << std::endl;
            std::cout << "  Server Name: " << servers[i].getServerName() << std::endl;
            std::cout << "  Client Max Body Size: " << servers[i].getClientMaxBodySize() << std::endl;
            std::cout << "  Locations: " << servers[i].getLocations().size() << std::endl;
            
            for (size_t j = 0; j < servers[i].getLocations().size(); ++j) {
                const LocationConfig& loc = servers[i].getLocations()[j];
                std::cout << "    Location " << (j + 1) << ": " << loc.getPath() << std::endl;
                std::cout << "      Root: " << loc.getRoot() << std::endl;
                std::cout << "      Index: " << loc.getIndex() << std::endl;
                std::cout << "      AutoIndex: " << (loc.getAutoIndex() ? "on" : "off") << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

