#include "HttpRequest.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <limits>
#include <unistd.h>

static bool parseChunkSize(const std::string& line, size_t& chunkSize) {
    std::string sizePart = line.substr(0, line.find(';'));
    if (sizePart.empty()) {
        return false;
    }

    errno = 0;
    char* end = NULL;
    unsigned long value = std::strtoul(sizePart.c_str(), &end, 16);
    if (errno != 0 || end == sizePart.c_str() || *end != '\0') {
        return false;
    }
    if (value > std::numeric_limits<size_t>::max()) {
        return false;
    }
    chunkSize = static_cast<size_t>(value);
    return true;
}

static bool parseDecimalSize(const std::string& text, size_t& size) {
    if (text.empty())
        return false;

    errno = 0;
    char* end = NULL;
    unsigned long value = std::strtoul(text.c_str(), &end, 10);
    if (errno != 0 || end == text.c_str() || *end != '\0')
        return false;
    if (value > std::numeric_limits<size_t>::max())
        return false;
    size = static_cast<size_t>(value);
    return true;
}

HttpRequest::HttpRequest(const ServerConfig* config): _state(REQ_HEADERS), _contentLength(0), _bodyBytesRead(0), _uploadFileFd(-1), _isChunked(false), _serverConfig(config) {  
}

void HttpRequest::parseRequestLine() {
    size_t lineEnd = _rawHeaders.find("\r\n");
    if (lineEnd == std::string::npos) {
        _state = REQ_ERROR; 
        return;
    }

    std::string requestLine = _rawHeaders.substr(0, lineEnd);
    size_t methodEnd = requestLine.find(' ');
    if (methodEnd == std::string::npos) {
        _state = REQ_ERROR; 
        return;
    }
    _method = requestLine.substr(0, methodEnd);

    size_t uriStart = methodEnd + 1;
    size_t uriEnd = requestLine.find(' ', uriStart);
    if (uriEnd == std::string::npos) {
        _state = REQ_ERROR; 
        return;
    }
    _uri = requestLine.substr(uriStart, uriEnd - uriStart);

    size_t versionStart = uriEnd + 1;
    if (versionStart >= requestLine.size()) {
        _state = REQ_ERROR; 
        return;
    }
    _version = requestLine.substr(versionStart);
}

void HttpRequest::parseHeaders() {
    size_t pos = _rawHeaders.find("\r\n\r\n");
    if (pos == std::string::npos) {
        _state = REQ_ERROR; 
        return;
    }

    std::string headersPart = _rawHeaders.substr(0, pos);
    size_t lineStart = headersPart.find("\r\n") + 2; // Skip request line
    while (lineStart < headersPart.size()) {
        size_t lineEnd = headersPart.find("\r\n", lineStart);
        if (lineEnd == std::string::npos) {
            lineEnd = headersPart.size();
        }
        std::string headerLine = headersPart.substr(lineStart, lineEnd - lineStart);
        size_t colonPos = headerLine.find(':');
        if (colonPos != std::string::npos) {
            std::string name = headerLine.substr(0, colonPos);
            std::string value = headerLine.substr(colonPos + 1);
            // Trim whitespace
            name.erase(name.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            _headers[name] = value;

            // Check for Content-Length and Transfer-Encoding
            if (name == "Content-Length") {
                if (!parseDecimalSize(value, _contentLength)) {
                    _state = REQ_ERROR;
                    return;
                }
            } else if (name == "Transfer-Encoding" && value == "chunked") {
                _isChunked = true;
            }
        } else {
            _state = REQ_ERROR; 
            return;
        }
        lineStart = lineEnd + 2; // Move to next line
    }
}

// Helper method to keep things clean
void HttpRequest::processNormalBody(const char* data, size_t len) {
    size_t bodyBytesToRead = std::min(_contentLength - _bodyBytesRead, len);
    
    if (bodyBytesToRead == 0) {
        return; // No more body data needed
    }

    if (!writeBodyData(data, bodyBytesToRead))
        return;

    _bodyBytesRead += bodyBytesToRead;
    if (_bodyBytesRead >= _contentLength) {
        finishBody();
    }
}



// 5\r\n
// Hellodssddfs\r\n
// 5\r\n
// World\r\n
// 0\r\n
// \r\n

void HttpRequest::processChunkedData(const char* buffer, size_t length) {
    _chunkBuffer.append(buffer, length);

    while (_state == REQ_BODY_CHUNKED) {
        size_t lineEnd = _chunkBuffer.find("\r\n");
        if (lineEnd == std::string::npos)
            return;

        size_t chunkSize = 0;
        if (!parseChunkSize(_chunkBuffer.substr(0, lineEnd), chunkSize)) {
            _state = REQ_ERROR;
            return;
        }

        size_t dataStart = lineEnd + 2;
        if (chunkSize == 0) {
            if (_chunkBuffer.size() < dataStart + 2)
                return;
            if (_chunkBuffer.compare(dataStart, 2, "\r\n") != 0) {
                _state = REQ_ERROR;
                return;
            }
            _chunkBuffer.erase(0, dataStart + 2);
            finishBody();
            return;
        }

        if (_chunkBuffer.size() < dataStart + chunkSize + 2)
            return;
        if (_chunkBuffer.compare(dataStart + chunkSize, 2, "\r\n") != 0) {
            _state = REQ_ERROR;
            return;
        }

        if (!writeBodyData(_chunkBuffer.data() + dataStart, chunkSize))
            return;

        _bodyBytesRead += chunkSize;
        _chunkBuffer.erase(0, dataStart + chunkSize + 2);
    }
}

bool HttpRequest::shouldUploadBody() const {
    if (_serverConfig == NULL)
        return false;
    try {
        const LocationConfig* location = _serverConfig->matchLocation(_uri);
        return location != NULL && location->hasUploadDir();
    } catch (const std::exception&) {
        return false;
    }
}

bool HttpRequest::openUploadFile() {
    if (_uploadFileFd != -1)
        return true;
    if (_serverConfig == NULL) {
        _state = REQ_ERROR;
        return false;
    }

    const LocationConfig* location = NULL;
    try {
        location = _serverConfig->matchLocation(_uri);
    } catch (const std::exception&) {
        _state = REQ_ERROR;
        return false;
    }

    std::string suffix = _uri.substr(location->getPath().size());
    while (!suffix.empty() && suffix[0] == '/')
        suffix.erase(0, 1);
    if (suffix.empty())
        suffix = "upload_body";

    std::string path = location->getUploadDir();
    if (!path.empty() && path[path.size() - 1] != '/')
        path += "/";
    path += suffix;

    _uploadFileFd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (_uploadFileFd < 0) {
        _state = REQ_ERROR;
        return false;
    }
    return true;
}

bool HttpRequest::writeBodyData(const char* data, size_t len) {
    if (!shouldUploadBody()) {
        _bodyBuffer.insert(_bodyBuffer.end(), data, data + len);
        return true;
    }

    if (!openUploadFile())
        return false;

    size_t writtenTotal = 0;
    while (writtenTotal < len) {
        ssize_t written = write(_uploadFileFd, data + writtenTotal, len - writtenTotal);
        if (written <= 0) {
            _state = REQ_ERROR;
            return false;
        }
        writtenTotal += static_cast<size_t>(written);
    }
    return true;
}

void HttpRequest::finishBody() {
    if (_uploadFileFd != -1) {
        close(_uploadFileFd);
        _uploadFileFd = -1;
    }
    _state = REQ_COMPLETE;
}

void HttpRequest::feedData(const char* buffer, size_t length) {
    if (_state == REQ_ERROR || _state == REQ_COMPLETE) {
        return; 
    }

    // --- PHASE 1: STILL READING HEADERS ---
    if (_state == REQ_HEADERS) {
        _rawHeaders.append(buffer, length);
        size_t pos = _rawHeaders.find("\r\n\r\n");
        
        if (pos != std::string::npos) {
            // 1. Headers are fully received! Parse them.
            parseRequestLine();
            if (_state == REQ_ERROR) return;
            parseHeaders();
            if (_state == REQ_ERROR) return;

            // 2. Change state based on headers
            if (_isChunked) {
                _state = REQ_BODY_CHUNKED;
            } else if (_contentLength > 0) {
                _state = REQ_BODY_NORMAL;
            } else {
                _state = REQ_COMPLETE;
                return; // We are done!
            }

            // 3. THE PIVOT: Handle any body data that arrived in this exact buffer
            size_t headerEnd = pos + 4;
            if (_rawHeaders.size() > headerEnd) {
                size_t leftoverLength = _rawHeaders.size() - headerEnd;
                const char* leftoverData = _rawHeaders.c_str() + headerEnd;
                
                if (_state == REQ_BODY_CHUNKED) {
                    processChunkedData(leftoverData, leftoverLength);
                } else if (_state == REQ_BODY_NORMAL) {
                    processNormalBody(leftoverData, leftoverLength);
                }
            }
            
            // Optional: Shrink _rawHeaders to save memory since we don't need the body parts attached to it
            _rawHeaders.resize(headerEnd); 
        }
    } 
    // --- PHASE 2: READING BODY (Subsequent calls to feedData) ---
    else if (_state == REQ_BODY_NORMAL) {
        processNormalBody(buffer, length); 
    } 
    else if (_state == REQ_BODY_CHUNKED) {
        processChunkedData(buffer, length);
    }
}

bool HttpRequest::isComplete() const {
    return _state == REQ_COMPLETE;
}

bool HttpRequest::hasError() const {
    return _state == REQ_ERROR;
}

std::string HttpRequest::getrawHeaders() const {
    return _rawHeaders;
}
std::string HttpRequest::getMethod() const {
    return _method;
}
std::string HttpRequest::getUri() const {
    return _uri;
}
std::string HttpRequest::getVersion() const {
    return _version;
}
std::map<std::string, std::string> HttpRequest::getHeaders() const {
    return _headers;
}
RequestState HttpRequest::getState() const {
    return _state;
}
size_t HttpRequest::getContentLength() const {
    return _contentLength;
}
size_t HttpRequest::getBodyBytesRead() const {
    return _bodyBytesRead;
}
int HttpRequest::getuploadFileFd() const {
    return _uploadFileFd;
}
bool HttpRequest::getisChunked() const {
    return _isChunked;
}
