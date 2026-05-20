#include "../../includes/config/locationDirectives.hpp"

locationDirectives::locationDirectives() {
    _path = "";
    _root = "";
    _index = "";
    _autoIndex = "";
    _allowedMethods = "";
    _uploadDir = "";
    _cgiExtension = "";
    _cgiPath = "";
}
locationDirectives::~locationDirectives() {}
void locationDirectives::setPath(const std::string& path) {
    _path = path;
}
void locationDirectives::setRoot(const std::string& root) {
    _root = root;
}
void locationDirectives::setIndex(const std::string& index) {
    _index = index;
}
void locationDirectives::setAutoIndex(const std::string& autoIndex) {
    _autoIndex = autoIndex;
}
void locationDirectives::setAllowedMethods(const std::string& allowedMethods) {
    _allowedMethods = allowedMethods;
}
void locationDirectives::setUploadDir(const std::string& uploadDir) {
    _uploadDir = uploadDir;
}
void locationDirectives::setCgiExtension(const std::string& cgiExtension) {
    _cgiExtension = cgiExtension;
}
void locationDirectives::setCgiPath(const std::string& cgiPath) {
    _cgiPath = cgiPath;
}
void locationDirectives::setReturn(const std::string& returnDirective) {
    _return = returnDirective;
}
const std::string& locationDirectives::getPath() const {
    return _path;
}
const std::string& locationDirectives::getRoot() const {
    return _root;
}
const std::string& locationDirectives::getIndex() const {
    return _index;
}
const std::string& locationDirectives::getAutoIndex() const {
    return _autoIndex;
}
const std::string& locationDirectives::getAllowedMethods() const {
    return _allowedMethods;
}
const std::string& locationDirectives::getUploadDir() const {   
    return _uploadDir;
}
const std::string& locationDirectives::getCgiExtension() const {
    return _cgiExtension;
}
const std::string& locationDirectives::getCgiPath() const {
    return _cgiPath;
}
const std::string& locationDirectives::getReturn() const {
    return _return;
}
