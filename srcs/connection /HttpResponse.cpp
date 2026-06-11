#include "HttpResponse.hpp"
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
HttpResponse::HttpResponse() {
    _statusCode = 200;
    _headersSent = false;
    _fileFd = -1;
    _isFileResponse = false;
    _stringBytesSent = 0;

}
void HttpResponse::generate(const HttpRequest& req, const ServerConfig* config) {
    if (req.hasError())
    {
        if (req.getErrorCode() == 400) {
        buildErrorPage(400, config);
        } else if (req.getErrorCode() == 413) {
            buildErrorPage(413, config);
        } else {
            buildErrorPage(500, config);
        }
        return;
    }
    if (methodIsNotAllowed(req.getMethod(),req ,config)) {
        buildErrorPage(405, config);
    }
    std::string uri = req.getUri();
    std::string physicalPath = physicalPathForUri(uri, config);
    if (physicalPath.empty()) {
        buildErrorPage(404, config);
        return;
    }
    struct stat fileInfo;
    if (stat(physicalPath.c_str(), &fileInfo) != 0) {
        return buildErrorPage(404, config);
    }
    if (S_ISDIR(fileInfo.st_mode)) {
        handleDirectory(physicalPath, config);
    }
    formatHeaders();
}


static std::string  physicalPathForUri(const std::string& uri, const ServerConfig* config) {
    if (config == NULL) {
        return "." + uri; // Fallback to current directory
    }
    const LocationConfig* location = NULL;
    try {
        location = config->matchLocation(uri);
        std::string suffix = uri.substr(location->getPath().size());
        while (!suffix.empty() && suffix[0] == '/')
            suffix.erase(0, 1);
        if (suffix.empty())
            suffix = location->getIndex(); // Default file for directories

        std::string path = location->getRoot();
        if (!path.empty() && path[path.size() - 1] != '/')
            path += "/";
        path += suffix;
        return path;
    } catch (const std::exception&) {
        return "." + uri; // Fallback if no matching location
    }
}

bool methodIsNotAllowed(const std::string& method, const HttpRequest& req, const ServerConfig* config) {
    if (method != "GET" && method != "POST" && method != "DELETE") {
         return true;
    }

    if (config == NULL) {
        return false; // No config means we can't determine allowed methods, so we won't block it here
    }
    const LocationConfig* location = NULL;
    try {
        location = config->matchLocation(req.getUri());
        const std::vector<std::string>& allowedMethods = location->getAllowedMethods();
        return std::find(allowedMethods.begin(), allowedMethods.end(), method) == allowedMethods.end();
    } catch (const std::exception&) {
        return false;
    }

    return false;
}


std::string getStatusCodeString(int code) {
    switch (code) {
        case 200: return "200 OK";
        case 405: return "405 Method Not Allowed";
        case 400: return "400 Bad Request";
        case 404: return "404 Not Found";
        case 500: return "500 Internal Server Error";
        default: return std::to_string(code) + " Unknown";
    }
}

void HttpResponse::formatHeaders() {
    _headerBuffer = "HTTP/1.1 " + getStatusCodeString(_statusCode) + "\r\n";
    for (std::map<std::string, std::string>::iterator it = _headers.begin(); it != _headers.end(); ++it) {
        _headerBuffer += it->first + ": " + it->second + "\r\n";
    }
    _headerBuffer += "\r\n";
}

void HttpResponse::buildErrorPage(int code, const ServerConfig* config) {
    std::string errorPagePath = config->getErrorPage(code);
    if (!errorPagePath.empty())
    {
        _fileFd = open(errorPagePath.c_str(), O_RDONLY);
        if (_fileFd == -1) {
            // If the custom error page cannot be opened, fall back to a simple error message
            _statusCode = code;
            _stringBody = "<html><body><h1>" + getStatusCodeString(code) + "</h1></body></html>";
            _headers["Content-Type"] = "text/html";
            _headers["Content-Length"] = std::to_string(_stringBody.size());
        } else {
            _isFileResponse = true;
            _headers["Content-Type"] = "text/html"; // Simplification
            _headers["Content-Length"] = std::to_string(lseek(_fileFd, 0, SEEK_END));
            lseek(_fileFd, 0, SEEK_SET);
        }
    }
    else
    {
        _statusCode = code;
        _stringBody = "<html><body><h1>" + getStatusCodeString(code) + "</h1></body></html>";
        _headers["Content-Type"] = "text/html";
        _headers["Content-Length"] = std::to_string(_stringBody.size());

    }
    formatHeaders();
}
