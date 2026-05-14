#pragma once
#include <string>
#include <vector>
#include <map>
#include "LocationConfig.hpp"

class ServerConfig {
private:
    std::vector<int>                    _ports;
    std::string                         _host;
    std::vector<std::string>            _serverNames;
    size_t                              _clientMaxBodySize;
    std::map<int, std::string>          _errorPages;
    std::vector<LocationConfig>         _locations;

public:
    ServerConfig() : _host("0.0.0.0") {}
    ~ServerConfig() {}
    // Getters
    const std::vector<int>& getPorts() const;
    const std::string& getHost() const;
    const std::vector<std::string>& getServerNames() const;
    size_t getClientMaxBodySize() const;
    const std::map<int, std::string>& getErrorPages() const;
    const std::vector<LocationConfig>& getLocations() const;
    // setters
    void setPorts(const std::vector<int>& ports);
    void setHost(const std::string& host);
    void setServerNames(const std::vector<std::string>& serverNames);
    void setClientMaxBodySize(size_t size);
    void setErrorPages(const std::map<int, std::string>& errorPages);
    void setLocations(const std::vector<LocationConfig>& locations);
    //validation
    void validatePorts(const std::vector<int>& ports);
    // void validateHost(const std::string& host);
    void validateServerNames(const std::vector<std::string>& serverNames);
    void validateClientMaxBodySize(size_t size);
    void validateErrorPages(const std::map<int, std::string>& errorPages);
    // void validateLocations(const std::vector<LocationConfig>& locations);

    const LocationConfig* matchLocation(const std::string& uri) const; // Longest prefix match
};