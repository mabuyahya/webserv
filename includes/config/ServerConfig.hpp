#pragma once
#include <string>
#include <vector>
#include <map>
#include <set>
#include "LocationConfig.hpp"

class ServerConfig {
private:
    int                    _port;
    std::string                         _host;
    std::string             _serverName;
    size_t                              _clientMaxBodySize;
    std::map<int, std::string>          _errorPages;
    std::vector<LocationConfig>         _locations;
    mutable std::set<std::string>       _deletedPaths;


public:
    ServerConfig();
    ~ServerConfig();
    // Getters
    int getPort() const;
    const std::string& getHost() const;
    const std::string& getServerName() const;
    size_t getClientMaxBodySize() const;
    const std::map<int, std::string>& getErrorPages() const;
    const std::string& getErrorPage(int statusCode) const;
    const std::vector<LocationConfig>& getLocations() const;
    bool isDeletedPath(const std::string& path) const;
    // setters
    void setPort(int port);
    void setHost(const std::string& host);
    void setServerName(const std::string& serverName);
    void setClientMaxBodySize(size_t size);
    void setErrorPages(const std::map<int, std::string>& errorPages);
    void addErrorPage(int statusCode, const std::string& page);
    void addLocation(const LocationConfig& location);
    void setLocations(const std::vector<LocationConfig>& locations);
    void markDeletedPath(const std::string& path) const;
    void restoreDeletedPath(const std::string& path) const;
    //validation
    void validatePort(int port);
    // void validateHost(const std::string& host);
    void validateServerName(const std::string& serverName);
    void validateClientMaxBodySize(size_t size);
    void validateErrorPages(const std::map<int, std::string>& errorPages);
    // void validateLocations(const std::vector<LocationConfig>& locations);

    const LocationConfig* matchLocation(const std::string& uri) const; // Longest prefix match
};
