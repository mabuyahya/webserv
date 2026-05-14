#include "includes/ConfigParser.hpp" 

ConfigParser::ConfigParser(const std::string& path) : _configFile(path) {}

void ConfigParser::parse() {
    std::ifstream file(_configFile.c_str());
    if (!file.is_open()) {
        throw std::runtime_error("Could not open config file: " + _configFile);
    }
    // implement parsing logic here to populate _servers vector
    file.close();
}

std::vector<ServerConfig> ConfigParser::getServers() const {
    return _servers;
}