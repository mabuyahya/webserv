#include "includes/ServerConfig.hpp"

const std::vector<int>& ServerConfig::getPorts() const {
    return _ports;
}
const std::string& ServerConfig::getHost() const {
    return _host;
}
const std::vector<std::string>& ServerConfig::getServerNames() const {
    return _serverNames;
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

void ServerConfig::setPorts(const std::vector<int>& ports) {
    validatePorts(ports);
    _ports = ports;
}
void ServerConfig::setHost(const std::string& host) {
    // validateHost(host);
    _host = host;
}
void ServerConfig::setServerNames(const std::vector<std::string>& serverNames) {
    validateServerNames(serverNames);
    _serverNames = serverNames;
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


void ServerConfig::validatePorts(const std::vector<int>& ports) {
    for (size_t i = 0; i < ports.size(); ++i) {
        if (ports[i] <= 0 || ports[i] > 65535) {
            throw std::invalid_argument("Invalid port number: " + std::to_string(ports[i]));
        }
    }
}

// void ServerConfig::validateHost(const std::string& host) {
//     if (host.empty()) {
//         throw std::invalid_argument("Host cannot be empty");
//     }
// }


void ServerConfig::validateServerNames(const std::vector<std::string>& serverNames) {
    for (size_t i = 0; i < serverNames.size(); ++i) {
        if (serverNames[i].empty()) {
            throw std::invalid_argument("Server name cannot be empty");
        }
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
            throw std::invalid_argument("Invalid HTTP status code for error page: " + std::to_string(it->first));
        }
        if (it->second.empty()) {
            throw std::invalid_argument("Error page path cannot be empty for status code: " + std::to_string(it->first));
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
// /website/mabuyahy

// / , /website , /website/mabuyahy