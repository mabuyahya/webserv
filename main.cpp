#include <exception>
#include <iostream>
#include "includes/config/ConfigParser.hpp"
#include "includes/connection/ServerManager.hpp"

int main(int argc, char** argv) {
    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " [config_file]" << std::endl;
        return 1;
    }
    const std::string configPath = argc == 2
        ? argv[1] : "configs_files/evaluation.conf";
    try {
        ConfigParser parser(configPath);
        parser.parse();
        std::vector<ServerConfig> servers = parser.getServers();
        if (servers.empty())
            throw std::runtime_error("Configuration contains no server blocks");
        ServerManager manager(servers);
        manager.run();
    } catch (const std::exception& error) {
        std::cerr << "webserv: " << error.what() << std::endl;
        return 1;
    }
    return 0;
}
