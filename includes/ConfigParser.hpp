#pragma once 
#include "ServerConfig.hpp"
#include  <fstream>

enum LINE_KIND {
    IDENTIFIER,
    IDENTIFIER_CLOSER,
    COMMENT,
    NORMAL,
    EMPTY,
    UNKNOWN
}


class ConfigParser {
private:
    std::string                         _configFile;
    std::vector<ServerConfig>           _servers;

public:
    ConfigParser(const std::string& path);
    void parse();
    std::vector<ServerConfig> getServers() const;
};