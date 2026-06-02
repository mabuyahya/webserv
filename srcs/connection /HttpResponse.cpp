#include "HttpResponse.hpp"

HttpResponse::HttpResponse() {
    _statusCode = 200;
    _headersSent = false;
    _fileFd = -1;
    _isFileResponse = false;
    _stringBytesSent = 0;
}

void HttpResponse::generate(const HttpRequest& req, const ServerConfig* config) {
    // For simplicity, we will just handle a few cases here.
    // In a real implementation, this would be much more complex.

    std::string path = req.getPath();
    if (path == "/") {
        buildDirectoryListing(config->getRoot());
    } else {
        std::string fullPath = config->getRoot() + path;
        _fileFd = open(fullPath.c_str(), O_RDONLY);
        if (_fileFd == -1) {
            buildErrorPage(404, config);
        } else {
            _isFileResponse = true;
            _headers["Content-Type"] = "text/html"; // Simplification
            _headers["Content-Length"] = std::to_string(lseek(_fileFd, 0, SEEK_END));
            lseek(_fileFd, 0, SEEK_SET);
        }
    }
    formatHeaders();
}

void HttpResponse::formatHeaders() {
    _headerBuffer = "HTTP/1.1 " + std::to_string(_statusCode) + " OK\r\n";
    for (std::map<std::string, std::string>::iterator it = _headers.begin(); it != _headers.end(); ++it) {
        _headerBuffer += it->first + ": " + it->second + "\r\n";
    }
    _headerBuffer += "\r\n";
}

void HttpResponse::buildErrorPage(int code, const ServerConfig* config) {
    std::string errorPagePath;
    if(config != NULL) {
        errorPagePath = config->getErrorPage(code);
        if (!errorPagePath.empty()) {
            _fileFd = open(errorPagePath.c_str(), O_RDONLY);
            if (_fileFd != -1) {
                _isFileResponse = true;
                _headers["Content-Type"] = "text/html";
                _headers["Content-Length"] = std::to_string(lseek(_fileFd, 0, SEEK_END));
                lseek(_fileFd, 0, SEEK_SET);
                return;
            }
        }
        _isFileResponse = false;
    }
    _statusCode = code;
    // Simple default error page 404, 500, etc.
    _stringBody = "<html><body><h1>" + std::to_string(code) + " Error</h1></body></html>";
    _headers["Content-Type"] = "text/html";
    _headers["Content-Length"] = std::to_string(_stringBody.size());
}