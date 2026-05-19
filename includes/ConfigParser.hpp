#pragma once 
#include "ServerConfig.hpp"
#include "serverDirectives.hpp"
#include  <fstream>

enum LINE_KIND {
    IDENTIFIER,
    IDENTIFIER_CLOSER,
    COMMENT,
    NORMAL,
    EMPTY,
    UNKNOWN
};

class ConfigParser {
private:
    std::string                         _configFile;
    std::vector<ServerConfig>           _servers;

public:
    ConfigParser(const std::string& path);
    void parse();
    std::vector<ServerConfig> getServers() const;
     ServerConfig parse_server_directives(const serverDirectives& s_directives);
};