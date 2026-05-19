#include "../includes/ServerConfig.hpp"

ServerConfig::ServerConfig() : _port(80), _host("0.0.0.0"), _serverName("default"), _clientMaxBodySize(1024 * 1024) {}
#include <sstream>

static std::string int_to_string(int value) {
    std::ostringstream ss;
    ss << value;
    return ss.str();
}
ServerConfig::~ServerConfig() {}
int ServerConfig::getPort() const {
    return _port;
}
const std::string& ServerConfig::getHost() const {
    return _host;
}
const std::string& ServerConfig::getServerName() const {
    return _serverName;
}
size_t ServerConfig::getClientMaxBodySize() const {
    return _clientMaxBodySize;
}
const std::map<int, std::string>& ServerConfig::getErrorPages() const {
    return _errorPages;
}
const std::vector<LocationConfig>& ServerConfig::getLocations() const {
    return _locations;
}

void ServerConfig::setPort(int port) {
    validatePort(port);
    _port = port;
}
void ServerConfig::setHost(const std::string& host) {
    // validateHost(host);
    _host = host;
}
void ServerConfig::setServerName(const std::string& serverName) {
    validateServerName(serverName);
    _serverName = serverName;
}
void ServerConfig::setClientMaxBodySize(size_t size) {
    validateClientMaxBodySize(size);
    _clientMaxBodySize = size;
}
void ServerConfig::setErrorPages(const std::map<int, std::string>& errorPages) {
    validateErrorPages(errorPages);
    _errorPages = errorPages;
}
void ServerConfig::setLocations(const std::vector<LocationConfig>& locations) {
    _locations = locations;
}


void ServerConfig::validatePort(int port) {
    if (port <= 0 || port > 65535) {
        throw std::invalid_argument("Invalid port number: " + int_to_string(port));
    }
}

// void ServerConfig::validateHost(const std::string& host) {
//     if (host.empty()) {
//         throw std::invalid_argument("Host cannot be empty");
//     }
// }


void ServerConfig::validateServerName(const std::string& serverName) {
    if (serverName.empty()) {
        throw std::invalid_argument("Server name cannot be empty");
    }
}


void ServerConfig::validateClientMaxBodySize(size_t size) {
    if (size == 0) {
        throw std::invalid_argument("Client max body size must be greater than 0");
    }
}

void ServerConfig::validateErrorPages(const std::map<int, std::string>& errorPages) {
    for (std::map<int, std::string>::const_iterator it = errorPages.begin(); it != errorPages.end(); ++it) {
        if (it->first < 400 || it->first > 599) {
            throw std::invalid_argument("Invalid HTTP status code for error page: " + int_to_string(it->first));
        }
        if (it->second.empty()) {
            throw std::invalid_argument("Error page path cannot be empty for status code: " + int_to_string(it->first));
        }
    }
}

// void ServerConfig::validateLocations(const std::vector<LocationConfig>& locations) {
//     // No specific validation for locations in this implementation
// }

const LocationConfig* ServerConfig::matchLocation(const std::string& uri) const {
    const LocationConfig* bestMatch = NULL;
    size_t longestMatchLength = 0;

    for (size_t i = 0; i < _locations.size(); ++i) {
        const std::string& locationPath = _locations[i].getPath();
        if (uri.compare(0, locationPath.length(), locationPath) == 0) {
            if (locationPath.length() > longestMatchLength) {
                longestMatchLength = locationPath.length();
                bestMatch = &_locations[i];
            }
        }
    }
    if (bestMatch == NULL) {
        throw std::runtime_error("No matching location found for URI: " + uri);
    }
    return bestMatch;
}

void ServerConfig::addErrorPage(int statusCode, const std::string& page) {
    if (statusCode < 400 || statusCode > 599) {
        throw std::invalid_argument("Invalid HTTP status code for error page: " + int_to_string(statusCode));
    }
    if (page.empty()) {
        throw std::invalid_argument("Error page path cannot be empty for status code: " + int_to_string(statusCode));
    }
    _errorPages[statusCode] = page;
}

void ServerConfig::addLocation(const LocationConfig& location) {
    _locations.push_back(location);
}
// /website/mabuyahy

// / , /website , /website/mabuyahy