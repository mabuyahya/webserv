#include "../../includes/config/LocationConfig.hpp"

LocationConfig::LocationConfig() : _autoIndex(false) {
    _path = "";
    _root = "";
    _index = "";
    _allowedMethods.clear();
    _uploadDir = "";
    _hasUploadDir = false;
    _cgiExtension = "";
    _cgiPath = "";
    _return = std::make_pair(0, "");
}
LocationConfig::~LocationConfig() {}

const std::string& LocationConfig::getPath() const {
    return _path;
}
const std::string& LocationConfig::getRoot() const {
    return _root;
}
const std::string& LocationConfig::getIndex() const {
    return _index;
}
bool LocationConfig::getAutoIndex() const {
    return _autoIndex;
}
const std::vector<std::string>& LocationConfig::getAllowedMethods() const {
    return _allowedMethods;
}
const std::string& LocationConfig::getUploadDir() const {
    return _uploadDir;
}
bool LocationConfig::hasUploadDir() const {
    return _hasUploadDir;
}
const std::string& LocationConfig::getCgiExtension() const {
    return _cgiExtension;
}
const std::string& LocationConfig::getCgiPath() const {
    return _cgiPath;
}

const std::pair<int, std::string>& LocationConfig::getReturn() const {
    return _return;
}

void LocationConfig::setPath(const std::string& path) {
    validatePath(path);
    _path = path;
}
void LocationConfig::setRoot(const std::string& root) {
    validateRoot(root);
    _root = root;
}
void LocationConfig::setIndex(const std::string& index) {
    validateIndex(index);
    _index = index;
}
void LocationConfig::setAutoIndex(bool autoIndex) {
    _autoIndex = autoIndex;
}
void LocationConfig::setAllowedMethods(const std::vector<std::string>& methods) {
    validateAllowedMethods(methods);
    _allowedMethods = methods;
}
void LocationConfig::setUploadDir(const std::string& uploadDir) {
    validateUploadDir(uploadDir);
    _uploadDir = uploadDir;
    _hasUploadDir = true;
}
void LocationConfig::setCgiExtension(const std::string& cgiExtension) {
    validateCgiExtension(cgiExtension);
    _cgiExtension = cgiExtension;
}
void LocationConfig::setCgiPath(const std::string& cgiPath) {
    validateCgiPath(cgiPath);
    _cgiPath = cgiPath;
}

void LocationConfig::setReturn(int statusCode, const std::string& url) {
    validateReturn(statusCode, url);
    _return = std::make_pair(statusCode, url);
}

void LocationConfig::validateAllowedMethods(const std::vector<std::string>& methods) {
    for (size_t i = 0; i < methods.size(); ++i) {
        validateAllowedMethod(methods[i]);
    }
    _allowedMethods = methods;
}

void LocationConfig::validateAllowedMethod(const std::string& method) {
    const char* validMethods[] = {"GET", "POST", "DELETE", NULL};
    size_t i = 0;
    while (validMethods[i] != NULL) {
        if (method == std::string(validMethods[i])) {
            return;
        }
        ++i;
    }
    throw std::invalid_argument("Invalid HTTP method: " + method);
}

void LocationConfig::validateCgiExtension(const std::string& extension) {
    if (extension.empty()) {
        throw std::invalid_argument("CGI extension cannot be empty");
    }
    if (extension != ".py") {
        throw std::invalid_argument("CGI extension must be '.py'");
    }
}

void LocationConfig::validateCgiPath(const std::string& path) {
    if (path.empty()) {
        throw std::invalid_argument("CGI path cannot be empty");
    }
    if (path != "/usr/bin/python3") {
        throw std::invalid_argument("CGI path must be '/usr/bin/python3'");
    }
}

void LocationConfig::validateUploadDir(const std::string& uploadDir) {
    if (uploadDir.empty()) {
        throw std::invalid_argument("Upload directory cannot be empty");
    }
}

void LocationConfig::validateRoot(const std::string& root) {

    if (root.empty()) {
        throw std::invalid_argument("Root directory cannot be empty");
    }
}

void LocationConfig::validateIndex(const std::string& index) {
    if (index.empty()) {
        throw std::invalid_argument("Index cannot be empty");
    }
}

void LocationConfig::validatePath(const std::string& path) {
    if (path.empty()) {
        throw std::invalid_argument("Path cannot be empty");
    }
}

void LocationConfig::validateReturn(int statusCode, const std::string& url) {
    if (statusCode < 300 || statusCode > 399) {
        throw std::invalid_argument("Return status code must be a redirection code (300-399)");
    }
    if (url.empty()) {
        throw std::invalid_argument("Return URL cannot be empty");
    }
}
