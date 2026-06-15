#include "HttpResponse.hpp"

#include <algorithm>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

static std::string numberToString(size_t value) {
    std::ostringstream stream;
    stream << value;
    return stream.str();
}

static std::string statusText(int code) {
    switch (code) {
        case 200: return "200 OK";
        case 201: return "201 Created";
        case 204: return "204 No Content";
        case 301: return "301 Moved Permanently";
        case 302: return "302 Found";
        case 400: return "400 Bad Request";
        case 403: return "403 Forbidden";
        case 404: return "404 Not Found";
        case 405: return "405 Method Not Allowed";
        case 413: return "413 Payload Too Large";
        case 414: return "414 URI Too Long";
        case 431: return "431 Request Header Fields Too Large";
        case 500: return "500 Internal Server Error";
        case 501: return "501 Not Implemented";
        case 502: return "502 Bad Gateway";
        case 505: return "505 HTTP Version Not Supported";
        default: return numberToString(static_cast<size_t>(code)) + " Unknown";
    }
}

static char asciiLower(char value) {
    if (value >= 'A' && value <= 'Z')
        return static_cast<char>(value + ('a' - 'A'));
    return value;
}

static bool safeUploadCharacter(char value) {
    return (value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z')
        || (value >= '0' && value <= '9') || value == '.' || value == '_'
        || value == '-';
}

static const size_t CGI_OUTPUT_LIMIT = 16 * 1024 * 1024;

static std::string lowerString(const std::string& value) {
    std::string result(value);
    for (size_t i = 0; i < result.size(); ++i)
        result[i] = asciiLower(result[i]);
    return result;
}

static std::string mimeType(const std::string& path) {
    size_t dot = path.rfind('.');
    std::string extension = dot == std::string::npos ? "" : lowerString(path.substr(dot));
    if (extension == ".html" || extension == ".htm") return "text/html; charset=utf-8";
    if (extension == ".css") return "text/css; charset=utf-8";
    if (extension == ".js") return "application/javascript; charset=utf-8";
    if (extension == ".json") return "application/json; charset=utf-8";
    if (extension == ".txt") return "text/plain; charset=utf-8";
    if (extension == ".svg") return "image/svg+xml";
    if (extension == ".png") return "image/png";
    if (extension == ".jpg" || extension == ".jpeg") return "image/jpeg";
    if (extension == ".gif") return "image/gif";
    if (extension == ".ico") return "image/x-icon";
    if (extension == ".pdf") return "application/pdf";
    return "application/octet-stream";
}

static bool readFile(const std::string& path, std::string& output) {
    std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
    if (!file.is_open())
        return false;
    std::ostringstream content;
    content << file.rdbuf();
    if (file.bad())
        return false;
    output = content.str();
    return true;
}

static bool pathIsSafe(const std::string& path) {
    if (path.empty() || path[0] != '/' || path.find('\0') != std::string::npos)
        return false;
    std::istringstream parts(path);
    std::string part;
    while (std::getline(parts, part, '/')) {
        if (part == "..")
            return false;
    }
    return path.find("%2e") == std::string::npos && path.find("%2E") == std::string::npos;
}

static std::string physicalPath(const std::string& uri, const LocationConfig* location) {
    std::string suffix = uri.substr(location->getPath().size());
    while (!suffix.empty() && suffix[0] == '/')
        suffix.erase(0, 1);
    std::string path = location->getRoot();
    if (!suffix.empty()) {
        if (!path.empty() && path[path.size() - 1] != '/')
            path += "/";
        path += suffix;
    }
    return path;
}

static bool methodAllowed(const std::string& method, const LocationConfig* location) {
    const std::vector<std::string>& methods = location->getAllowedMethods();
    return std::find(methods.begin(), methods.end(), method) != methods.end();
}

static bool isCgiTarget(const std::string& path, const LocationConfig* location) {
    const std::string& extension = location->getCgiExtension();
    return !extension.empty() && path.size() >= extension.size()
        && path.compare(path.size() - extension.size(), extension.size(), extension) == 0;
}

HttpResponse::HttpResponse()
    : _statusCode(200), _bytesSent(0), _isGenerated(false), _isComplete(false),
      _fileFd(-1), _fileRemaining(0), _fileBufferBytesSent(0), _isFileResponse(false),
      _isCgiRunning(false), _cgiReadFd(-1), _cgiWriteFd(-1), _cgiBytesWritten(0) {
}

HttpResponse::~HttpResponse() {
    if (_fileFd != -1)
        close(_fileFd);
}

void HttpResponse::resetResponse() {
    if (_fileFd != -1)
        close(_fileFd);
    _statusCode = 200;
    _headers.clear();
    _body.clear();
    _output.clear();
    _bytesSent = 0;
    _isComplete = false;
    _fileFd = -1;
    _fileRemaining = 0;
    _fileBuffer.clear();
    _fileBufferBytesSent = 0;
    _isFileResponse = false;
    _isCgiRunning = false;
}

void HttpResponse::finalize() {
    _headers["Content-Length"] = numberToString(_body.size());
    _headers["Connection"] = "close";
    _headers["Server"] = "webserv/1.0";
    _output = "HTTP/1.0 " + statusText(_statusCode) + "\r\n";
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
         it != _headers.end(); ++it)
        _output += it->first + ": " + it->second + "\r\n";
    _output += "\r\n";
    _output += _body;
    _isGenerated = true;
}

bool HttpResponse::finalizeFile(const std::string& path, size_t fileSize) {
    _fileFd = open(path.c_str(), O_RDONLY);
    if (_fileFd == -1)
        return false;
    if (fcntl(_fileFd, F_SETFD, FD_CLOEXEC) == -1) {
        close(_fileFd);
        _fileFd = -1;
        return false;
    }
    _fileRemaining = fileSize;
    _fileBuffer.clear();
    _fileBufferBytesSent = 0;
    _isFileResponse = true;
    _headers["Content-Length"] = numberToString(fileSize);
    _headers["Content-Type"] = mimeType(path);
    _headers["Connection"] = "close";
    _headers["Server"] = "webserv/1.0";
    _output = "HTTP/1.0 " + statusText(_statusCode) + "\r\n";
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
         it != _headers.end(); ++it)
        _output += it->first + ": " + it->second + "\r\n";
    _output += "\r\n";
    _isGenerated = true;
    return true;
}

void HttpResponse::buildErrorPage(int code, const ServerConfig* config) {
    resetResponse();
    _statusCode = code;
    bool loaded = false;
    if (config != NULL) {
        std::string custom = config->getErrorPage(code);
        if (!custom.empty()) {
            loaded = readFile(custom, _body);
            if (!loaded && custom[0] == '/') {
                try {
                    const LocationConfig* location = config->matchLocation(custom);
                    loaded = readFile(physicalPath(custom, location), _body);
                } catch (const std::exception&) {
                    loaded = false;
                }
            }
        }
    }
    if (!loaded) {
        _body = "<!doctype html><html><head><meta charset=\"utf-8\"><title>"
              + statusText(code) + "</title></head><body><h1>" + statusText(code)
              + "</h1><p>Generated by webserv.</p></body></html>";
    }
    _headers["Content-Type"] = "text/html; charset=utf-8";
    finalize();
}

bool HttpResponse::buildDirectoryListing(const std::string& uri, const std::string& path) {
    DIR* directory = opendir(path.c_str());
    if (directory == NULL)
        return false;
    _body = "<!doctype html><html><head><meta charset=\"utf-8\"><title>Index of "
          + uri + "</title></head><body><h1>Index of " + uri + "</h1><ul>";
    struct dirent* entry;
    while ((entry = readdir(directory)) != NULL) {
        std::string name = entry->d_name;
        if (name == ".")
            continue;
        _body += "<li><a href=\"" + name + "\">" + name + "</a></li>";
    }
    closedir(directory);
    _body += "</ul></body></html>";
    _headers["Content-Type"] = "text/html; charset=utf-8";
    return true;
}

void HttpResponse::handleCgi(const HttpRequest& req, const std::string& path,
                             const LocationConfig* location, const ServerConfig* config) {
    _cgi.initEnv(req, path, location);
    if (!_cgi.executeCgi()) {
        buildErrorPage(500, config);
        return;
    }
    _isCgiRunning = true;
    _cgiReadFd = _cgi.getReadFd();
    _cgiWriteFd = _cgi.getWriteFd();
    _cgiBytesWritten = 0;
    _cgiOutput.clear();
    _isGenerated = true;
}

void HttpResponse::handleUpload(const HttpRequest& req, const LocationConfig* location,
                                const ServerConfig* config) {
    std::string suffix = req.getPath().substr(location->getPath().size());
    while (!suffix.empty() && suffix[0] == '/')
        suffix.erase(0, 1);
    if (suffix.empty())
        suffix = "upload_body";
    bool safeName = suffix != "." && suffix != ".." && suffix.find('/') == std::string::npos;
    for (size_t i = 0; i < suffix.size() && safeName; ++i) {
        safeName = safeUploadCharacter(suffix[i]);
    }
    if (!safeName) {
        buildErrorPage(400, config);
        return;
    }
    std::string destination = location->getUploadDir();
    if (!destination.empty() && destination[destination.size() - 1] != '/')
        destination += "/";
    destination += suffix;
    int fd = open(destination.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        buildErrorPage(500, config);
        return;
    }
    const std::vector<char>& body = req.getBody();
    size_t offset = 0;
    while (offset < body.size()) {
        ssize_t count = write(fd, &body[offset], body.size() - offset);
        if (count <= 0) {
            close(fd);
            buildErrorPage(500, config);
            return;
        }
        offset += static_cast<size_t>(count);
    }
    close(fd);
    _statusCode = 201;
    _headers["Content-Type"] = "application/json; charset=utf-8";
    _body = "{\"created\":\"" + suffix + "\",\"bytes\":" + numberToString(body.size()) + "}";
    finalize();
}

void HttpResponse::generate(const HttpRequest& req, const ServerConfig* config) {
    resetResponse();
    if (req.hasError()) {
        buildErrorPage(req.getErrorCode() == 0 ? 400 : req.getErrorCode(), config);
        return;
    }
    if (config == NULL || !pathIsSafe(req.getPath())) {
        buildErrorPage(400, config);
        return;
    }
    const LocationConfig* location = NULL;
    try {
        location = config->matchLocation(req.getPath());
    } catch (const std::exception&) {
        buildErrorPage(404, config);
        return;
    }
    if (!methodAllowed(req.getMethod(), location)) {
        buildErrorPage(405, config);
        std::string allow;
        const std::vector<std::string>& methods = location->getAllowedMethods();
        for (size_t i = 0; i < methods.size(); ++i) {
            if (!allow.empty())
                allow += ", ";
            allow += methods[i];
        }
        _headers["Allow"] = allow;
        finalize();
        return;
    }
    if (location->getReturn().first != 0) {
        _statusCode = location->getReturn().first;
        _headers["Location"] = location->getReturn().second;
        _headers["Content-Type"] = "text/html; charset=utf-8";
        _body = "<!doctype html><html><body><a href=\"" + location->getReturn().second
              + "\">Redirect</a></body></html>";
        finalize();
        return;
    }

    std::string path = physicalPath(req.getPath(), location);
    struct stat info;
    bool exists = stat(path.c_str(), &info) == 0;
    if (req.getMethod() == "POST" && location->hasUploadDir()) {
        handleUpload(req, location, config);
        return;
    }
    if (!exists) {
        buildErrorPage(404, config);
        return;
    }
    if (isCgiTarget(path, location)) {
        handleCgi(req, path, location, config);
        return;
    }
    if (req.getMethod() == "DELETE") {
        if (!S_ISREG(info.st_mode)) {
            buildErrorPage(403, config);
            return;
        }
        if (unlink(path.c_str()) != 0) {
            buildErrorPage(500, config);
            return;
        }
        _statusCode = 204;
        finalize();
        return;
    }
    if (S_ISDIR(info.st_mode)) {
        if (req.getPath()[req.getPath().size() - 1] != '/') {
            _statusCode = 301;
            _headers["Location"] = req.getPath() + "/";
            finalize();
            return;
        }
        std::istringstream indexes(location->getIndex());
        std::string index;
        while (indexes >> index) {
            std::string indexPath = path;
            if (!indexPath.empty() && indexPath[indexPath.size() - 1] != '/')
                indexPath += "/";
            indexPath += index;
            struct stat indexInfo;
            if (stat(indexPath.c_str(), &indexInfo) == 0 && S_ISREG(indexInfo.st_mode)) {
                if (isCgiTarget(indexPath, location)) {
                    handleCgi(req, indexPath, location, config);
                    return;
                }
                if (!finalizeFile(indexPath, static_cast<size_t>(indexInfo.st_size))) {
                    buildErrorPage(500, config);
                    return;
                }
                return;
            }
        }
        if (!location->getAutoIndex()) {
            buildErrorPage(403, config);
            return;
        }
        if (!buildDirectoryListing(req.getPath(), path)) {
            buildErrorPage(403, config);
            return;
        }
        finalize();
        return;
    }
    if (!S_ISREG(info.st_mode)) {
        buildErrorPage(403, config);
        return;
    }
    if (!finalizeFile(path, static_cast<size_t>(info.st_size)))
        buildErrorPage(500, config);
}

int HttpResponse::sendNextChunk(int clientSocket) {
    if (_bytesSent < _output.size()) {
        size_t remaining = _output.size() - _bytesSent;
        size_t count = std::min(remaining, static_cast<size_t>(16384));
        ssize_t sent = send(clientSocket, _output.data() + _bytesSent, count, 0);
        if (sent < 0)
            return -1;
        if (sent == 0)
            return -1;
        _bytesSent += static_cast<size_t>(sent);
        if (_bytesSent < _output.size())
            return static_cast<int>(sent);
    }
    if (!_isFileResponse) {
        _isComplete = true;
        return 0;
    }
    if (_fileBufferBytesSent == _fileBuffer.size()) {
        _fileBuffer.clear();
        _fileBufferBytesSent = 0;
        if (_fileRemaining == 0) {
            close(_fileFd);
            _fileFd = -1;
            _isComplete = true;
            return 0;
        }
        char buffer[16384];
        size_t toRead = std::min(_fileRemaining, sizeof(buffer));
        ssize_t readCount = read(_fileFd, buffer, toRead);
        if (readCount <= 0)
            return -1;
        _fileBuffer.assign(buffer, static_cast<size_t>(readCount));
        _fileRemaining -= static_cast<size_t>(readCount);
    }
    ssize_t sent = send(clientSocket, _fileBuffer.data() + _fileBufferBytesSent,
                        _fileBuffer.size() - _fileBufferBytesSent, 0);
    if (sent < 0)
        return -1;
    if (sent == 0)
        return -1;
    _fileBufferBytesSent += static_cast<size_t>(sent);
    return static_cast<int>(sent);
}

bool HttpResponse::isGenerated() const { return _isGenerated; }
bool HttpResponse::isComplete() const { return _isComplete; }
bool HttpResponse::isCgiRunning() const { return _isCgiRunning; }
int HttpResponse::getCgiReadFd() const { return _cgiReadFd; }
int HttpResponse::getCgiWriteFd() const { return _cgiWriteFd; }
pid_t HttpResponse::getCgiPid() const { return _cgi.getPid(); }
size_t HttpResponse::getCgiBytesWritten() const { return _cgiBytesWritten; }
void HttpResponse::addCgiBytesWritten(size_t count) { _cgiBytesWritten += count; }
bool HttpResponse::appendCgiOutput(const char* data, size_t count) {
    if (count > CGI_OUTPUT_LIMIT - _cgiOutput.size())
        return false;
    _cgiOutput.append(data, count);
    return true;
}

void HttpResponse::finishCgi(const ServerConfig* config) {
    resetResponse();
    size_t separator = _cgiOutput.find("\r\n\r\n");
    size_t separatorSize = 4;
    if (separator == std::string::npos) {
        separator = _cgiOutput.find("\n\n");
        separatorSize = 2;
    }
    if (separator == std::string::npos) {
        buildErrorPage(502, config);
        return;
    }
    std::string headerBlock = _cgiOutput.substr(0, separator);
    _body = _cgiOutput.substr(separator + separatorSize);
    std::istringstream headers(headerBlock);
    std::string line;
    while (std::getline(headers, line)) {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        size_t colon = line.find(':');
        if (colon == std::string::npos)
            continue;
        std::string name = line.substr(0, colon);
        std::string value = line.substr(colon + 1);
        while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
            value.erase(0, 1);
        if (lowerString(name) == "status") {
            std::istringstream status(value);
            status >> _statusCode;
        } else if (lowerString(name) != "content-length"
                   && lowerString(name) != "connection")
            _headers[name] = value;
    }
    if (_headers.count("Content-Type") == 0)
        _headers["Content-Type"] = "text/plain; charset=utf-8";
    finalize();
}

void HttpResponse::failCgi(const ServerConfig* config) {
    buildErrorPage(500, config);
}
