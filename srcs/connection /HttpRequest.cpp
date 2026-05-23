#include "HttpRequest.hpp"

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
                _contentLength = std::stoul(value);
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
    if(!_serverConfig->getLocationConfigs()[i]._hasUploadDir) 
    {
        _bodyBuffer.insert(_bodyBuffer.end(), data, data + bodyBytesToRead);
    }
    else if (_uploadFileFd == -1) {
        for (size_t  i = 0; i < _serverConfig->getLocationConfigs().size(); i++) {
            if (_uri.find(_serverConfig->getLocationConfigs()[i].getPath()) == 0) {
                if (_serverConfig->getLocationConfigs()[i].getUploadDir() != "") {
                    _uri = _serverConfig->getLocationConfigs()[i].getUploadDir() + "/" + _uri.substr(_serverConfig->getLocationConfigs()[i].getPath().size());
                }
                break;
            }
        }

        _uploadFileFd = open((_uri).c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (_uploadFileFd < 0) {
            _state = REQ_ERROR; 
            return;
        }
    } else {
        ssize_t written = write(_uploadFileFd, data, bodyBytesToRead);
        if (written < 0) {
            _state = REQ_ERROR; 
            return;
        }
    }

    _bodyBytesRead += bodyBytesToRead;
    if (_bodyBytesRead >= _contentLength) {
        _state = REQ_COMPLETE;
    }
}



// 5\r\n
// Hellodssddfs\r\n
// 5\r\n
// World\r\n
// 0\r\n
// \r\n

void HttpRequest::processChunkedData(const char* buffer, size_t length) {
    // This is a simplified implementation. A real implementation would need to handle chunk extensions and trailers.
    size_t pos = 0;
    while (pos < length) {
        size_t lineEnd = std::string(buffer + pos).find("\r\n");
        if (lineEnd == std::string::npos) {
            break; // Wait for more data
        }
        // this function create a string from first param (string) and second param (size_t) is the length of the string to create
        // std::string str(std::string, size_t length);
        std::string chunkSizeStr(buffer + pos, lineEnd);
        size_t chunkSize = std::stoul(chunkSizeStr, nullptr, 16);
        if (chunkSize == 0) {
            _state = REQ_COMPLETE; // Last chunk
            return;
        }
        pos += lineEnd + 2; // Move past chunk size line

        if (pos + chunkSize > length) {
            break; // Wait for more data
        }

        if (_serverConfig.getLocationConfigs()[i]._hasUploadDir) {
            if (_uploadFileFd == -1) {
                for (size_t  i = 0; i < _serverConfig->getLocationConfigs().size(); i++) {
                    if (_uri.find(_serverConfig->getLocationConfigs()[i].getPath()) == 0) {
                        if (_serverConfig->getLocationConfigs()[i].getUploadDir() != "") {
                            _uri = _serverConfig->getLocationConfigs()[i].getUploadDir() + "/" + _uri.substr(_serverConfig->getLocationConfigs()[i].getPath().size());
                        }
                        break;
                    }
                }

                _uploadFileFd = open((_uri).c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
                if (_uploadFileFd < 0) {
                    _state = REQ_ERROR; 
                    return;
                }
            }
            ssize_t written = write(_uploadFileFd, buffer + pos, chunkSize);
            if (written < 0) {
                _state = REQ_ERROR; 
                return;
            }
        } else {
             _
        _bodyBuffer.insert(_bodyBuffer.end(), buffer + pos, buffer + pos + chunkSize);
        pos += chunkSize + 2; // Move past chunk data and trailing \r\n
    }
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