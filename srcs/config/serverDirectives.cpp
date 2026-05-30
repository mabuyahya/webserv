#include "../../includes/config/serverDirectives.hpp"

serverDirectives::serverDirectives() {
    _listen = "";
    _serverName = "";
    _root = "";
    _index = "";
    _autoIndex = "off";
    _allowedMethods = "GET";
    _clientMaxBodySize = "";
}
serverDirectives::~serverDirectives() {}
void serverDirectives::setListen(const std::string& listen) {
    _listen = listen;
}
void serverDirectives::setServerName(const std::string& serverName) {
    _serverName = serverName;
}
void serverDirectives::setRoot(const std::string& root) {
    _root = root;
}
void serverDirectives::setIndex(const std::string& index) {
    _index = index;
}
void serverDirectives::addErrorPage(const std::string& errorPage) {
    _errorPages.push_back(errorPage);
}
void serverDirectives::setAutoIndex(const std::string& autoIndex) {
    _autoIndex = autoIndex;
}
void serverDirectives::setAllowedMethods(const std::string& allowedMethods) {
    _allowedMethods = allowedMethods;
}
void serverDirectives::setClientMaxBodySize(const std::string& clientMaxBodySize) {
    _clientMaxBodySize = clientMaxBodySize;
}
void serverDirectives::addLocation(const locationDirectives& location) {
    _locations.push_back(location);
}
const std::string& serverDirectives::getListen() const {
    return _listen;
}
const std::string& serverDirectives::getServerName() const {
    return _serverName;
}
const std::string& serverDirectives::getRoot() const {
    return _root;
}
const std::string& serverDirectives::getIndex() const {
    return _index;
}
const std::vector<std::string>& serverDirectives::getErrorPages() const {
    return _errorPages;
}
const std::string& serverDirectives::getAutoIndex() const {
    return _autoIndex;
}
const std::string& serverDirectives::getAllowedMethods() const {
    return _allowedMethods;
}
const std::string& serverDirectives::getClientMaxBodySize() const {
    return _clientMaxBodySize;
}
const std::vector<locationDirectives>& serverDirectives::getLocations() const {
    return _locations;
}
