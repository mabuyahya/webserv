#include "HttpRequest.hpp"

HttpRequest::HttpRequest()
    : _state(REQ_HEADERS), _contentLength(0), _bodyBytesRead(0), _uploadFileFd(-1), _isChunked(false) {}

void HttpRequest::feedData(const char* buffer, size_t length) 
{
    if (_state == REQ_ERROR || _state == REQ_COMPLETE) {
        return; // No more processing if already in error or complete state
    }

    _rawHeaders.append(buffer, length);

    if (_state == REQ_HEADERS) {
        size_t pos = _rawHeaders.find("\r\n\r\n");
        if (pos != std::string::npos) {
            parseRequestLine();
            parseHeaders();
            if (_state != REQ_ERROR) {
                if (_isChunked) {
                    _state = REQ_BODY_CHUNKED;
                    processChunkedData(_rawHeaders.c_str() + pos + 4, _rawHeaders.size() - pos - 4);
                } else if (_contentLength > 0) {
                    _state = REQ_BODY_NORMAL;
                    size_t bodyStart = pos + 4;
                    size_t bodyBytes = std::min(_contentLength, _rawHeaders.size() - bodyStart);
                    _bodyBytesRead += bodyBytes;
                    // Handle body data (e.g., write to file if upload)
                    // ...
                    if (_bodyBytesRead >= _contentLength) {
                        _state = REQ_COMPLETE;
                    }
                } else {
                    _state = REQ_COMPLETE; // No body
                }
            }
        }
    } else if (_state == REQ_BODY_NORMAL) {
        size_t bodyBytes = std::min(_contentLength - _bodyBytesRead, length);
        _bodyBytesRead += bodyBytes;
        // Handle body data (e.g., write to file if upload)
        // ...
        if (_bodyBytesRead >= _contentLength) {
            _state = REQ_COMPLETE;
        }
    } else if (_state == REQ_BODY_CHUNKED) {
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