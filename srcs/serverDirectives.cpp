#include "includes/serverDirectives.hpp"
serverDirectives::serverDirectives() {}
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
void serverDirectives::setErrorPage(const std::string& errorPage) {
    _errorPage = errorPage;
}
void serverDirectives::setCgiExtension(const std::string& cgiExtension) {
    _cgiExtension = cgiExtension;
}
void serverDirectives::setCgiPath(const std::string& cgiPath) {
    _cgiPath = cgiPath;
}
void serverDirectives::setAutoIndex(const std::string& autoIndex) {
    _autoIndex = autoIndex;
}
void serverDirectives::setAllowedMethods(const std::string& allowedMethods) {
    _allowedMethods = allowedMethods;
}
void serverDirectives::setClientMaxBodySize(const std::string& client_max_body_size) {
    this->client_max_body_size = client_max_body_size;
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
const std::string& serverDirectives::getErrorPage() const {
    return _errorPage;
}
const std::string& serverDirectives::getCgiExtension() const {
    return _cgiExtension;
}
const std::string& serverDirectives::getCgiPath() const {
    return _cgiPath;
}
const std::string& serverDirectives::getAutoIndex() const {
    return _autoIndex;
}
const std::string& serverDirectives::getAllowedMethods() const {
    return _allowedMethods;
}  
const std::string& serverDirectives::getClientMaxBodySize() const {
    return client_max_body_size;
}
const std::vector<locationDirectives>& serverDirectives::getLocations() const {
    return _locations;
}
