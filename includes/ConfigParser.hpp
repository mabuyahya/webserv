#pragma once 
#include "ServerConfig.hpp"
#include  <fstream>

class ConfigParser {
private:
    std::string                         _configFile;
    std::vector<ServerConfig>           _servers;

public:
    ConfigParser(const std::string& path);
    void parse();
    std::vector<ServerConfig> getServers() const;
};