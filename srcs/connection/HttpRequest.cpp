#include "HttpRequest.hpp"

#include <algorithm>
#include <exception>
#include <limits>
#include <sstream>

static char asciiLower(char value) {
    if (value >= 'A' && value <= 'Z')
        return static_cast<char>(value + ('a' - 'A'));
    return value;
}

static std::string lowerString(const std::string& value) {
    std::string result(value);
    for (size_t i = 0; i < result.size(); ++i)
        result[i] = asciiLower(result[i]);
    return result;
}

static std::string trim(const std::string& value) {
    size_t first = value.find_first_not_of(" \t");
    if (first == std::string::npos)
        return "";
    size_t last = value.find_last_not_of(" \t");
    return value.substr(first, last - first + 1);
}

static bool parseSize(const std::string& text, int base, size_t& result) {
    if (text.empty() || text[0] == '-')
        return false;
    result = 0;
    for (size_t i = 0; i < text.size(); ++i) {
        size_t digit;
        if (text[i] >= '0' && text[i] <= '9')
            digit = static_cast<size_t>(text[i] - '0');
        else if (base == 16 && asciiLower(text[i]) >= 'a' && asciiLower(text[i]) <= 'f')
            digit = static_cast<size_t>(asciiLower(text[i]) - 'a' + 10);
        else
            return false;
        if (result > (std::numeric_limits<size_t>::max() - digit) / static_cast<size_t>(base))
            return false;
        result = result * static_cast<size_t>(base) + digit;
    }
    return true;
}

static bool decodePath(const std::string& encoded, std::string& decoded) {
    static const std::string digits = "0123456789abcdef";
    decoded.clear();
    for (size_t i = 0; i < encoded.size(); ++i) {
        if (encoded[i] != '%') {
            decoded += encoded[i];
            continue;
        }
        if (i + 2 >= encoded.size())
            return false;
        char highChar = asciiLower(encoded[i + 1]);
        char lowChar = asciiLower(encoded[i + 2]);
        size_t high = digits.find(highChar);
        size_t low = digits.find(lowChar);
        if (high == std::string::npos || low == std::string::npos)
            return false;
        char value = static_cast<char>(high * 16 + low);
        if (value == '\0')
            return false;
        decoded += value;
        i += 2;
    }
    return true;
}

static size_t requestBodyLimit(const ServerConfig* config, const std::string& path) {
    if (config == NULL)
        return 0;
    size_t limit = config->getClientMaxBodySize();
    try {
        const LocationConfig* location = config->matchLocation(path);
        if (location->hasClientMaxBodySize())
            limit = location->getClientMaxBodySize();
    } catch (const std::exception&) {
    }
    return limit;
}

HttpRequest::HttpRequest(const ServerConfig* config)
    : _state(REQ_HEADERS), _errorCode(0), _contentLength(0),
      _bodyBytesRead(0), _isChunked(false), _serverConfig(config),
      _currentChunkSize(0), _readingChunkSize(true) {
}

void HttpRequest::setError(int code) {
    _errorCode = code;
    _state = REQ_ERROR;
}

void HttpRequest::parseRequestLine() {
    size_t lineEnd = _rawHeaders.find("\r\n");
    if (lineEnd == std::string::npos) {
        setError(400);
        return;
    }
    std::istringstream line(_rawHeaders.substr(0, lineEnd));
    std::string extra;
    if (!(line >> _method >> _uri >> _version) || (line >> extra)
        || _uri.empty() || _uri[0] != '/') {
        setError(400);
        return;
    }
    if (_version != "HTTP/1.0" && _version != "HTTP/1.1") {
        setError(505);
        return;
    }
    size_t query = _uri.find('?');
    if (!decodePath(_uri.substr(0, query), _path)) {
        setError(400);
        return;
    }
    if (query != std::string::npos)
        _queryString = _uri.substr(query + 1);
}

void HttpRequest::parseHeaders() {
    size_t headerEnd = _rawHeaders.find("\r\n\r\n");
    size_t lineStart = _rawHeaders.find("\r\n") + 2;
    if (headerEnd == std::string::npos || lineStart == 1) {
        setError(400);
        return;
    }
    while (lineStart < headerEnd) {
        size_t lineEnd = _rawHeaders.find("\r\n", lineStart);
        if (lineEnd == std::string::npos || lineEnd > headerEnd) {
            setError(400);
            return;
        }
        std::string line = _rawHeaders.substr(lineStart, lineEnd - lineStart);
        size_t colon = line.find(':');
        if (colon == std::string::npos || colon == 0) {
            setError(400);
            return;
        }
        std::string name = lowerString(trim(line.substr(0, colon)));
        std::string value = trim(line.substr(colon + 1));
        if (name.empty() || line.substr(0, colon).find_first_of(" \t") != std::string::npos
            || _headers.count(name)) {
            setError(400);
            return;
        }
        _headers[name] = value;
        lineStart = lineEnd + 2;
    }

    if (_version == "HTTP/1.1" && getHeader("host").empty()) {
        setError(400);
        return;
    }
    std::string contentLength = getHeader("content-length");
    std::string transferEncoding = lowerString(getHeader("transfer-encoding"));
    if (!contentLength.empty() && !transferEncoding.empty()) {
        setError(400);
        return;
    }
    if (!transferEncoding.empty()) {
        if (transferEncoding != "chunked") {
            setError(501);
            return;
        }
        _isChunked = true;
    } else if (!contentLength.empty() && !parseSize(contentLength, 10, _contentLength)) {
        setError(400);
        return;
    }
    if (_serverConfig != NULL && _contentLength > requestBodyLimit(_serverConfig, _path)) {
        _errorCode = 413;
        _state = REQ_BODY_DISCARD;
    }
}

bool HttpRequest::appendBody(const char* data, size_t len) {
    if (_serverConfig != NULL
        && _bodyBytesRead + len > requestBodyLimit(_serverConfig, _path)) {
        setError(413);
        return false;
    }
    _body.insert(_body.end(), data, data + len);
    _bodyBytesRead += len;
    return true;
}

void HttpRequest::processNormalBody(const char* data, size_t len) {
    size_t remaining = _contentLength - _bodyBytesRead;
    size_t count = std::min(remaining, len);
    if (count > 0 && !appendBody(data, count))
        return;
    if (_bodyBytesRead == _contentLength)
        _state = REQ_COMPLETE;
}

void HttpRequest::processDiscardBody(size_t len) {
    size_t remaining = _contentLength - _bodyBytesRead;
    size_t count = std::min(remaining, len);
    _bodyBytesRead += count;
    if (_bodyBytesRead == _contentLength)
        _state = REQ_ERROR;
}

void HttpRequest::processChunkedData(const char* data, size_t len) {
    _chunkBuffer.append(data, len);
    while (_state == REQ_BODY_CHUNKED) {
        if (_readingChunkSize) {
            size_t lineEnd = _chunkBuffer.find("\r\n");
            if (lineEnd == std::string::npos)
                return;
            std::string sizeText = _chunkBuffer.substr(0, lineEnd);
            size_t extension = sizeText.find(';');
            if (extension != std::string::npos)
                sizeText.erase(extension);
            if (!parseSize(trim(sizeText), 16, _currentChunkSize)) {
                setError(400);
                return;
            }
            if (_serverConfig != NULL
                && _currentChunkSize > requestBodyLimit(_serverConfig, _path) - _bodyBytesRead) {
                setError(413);
                return;
            }
            _chunkBuffer.erase(0, lineEnd + 2);
            _readingChunkSize = false;
            if (_currentChunkSize == 0) {
                if (_chunkBuffer.size() >= 2 && _chunkBuffer.compare(0, 2, "\r\n") == 0) {
                    _state = REQ_COMPLETE;
                    return;
                }
                size_t trailerEnd = _chunkBuffer.find("\r\n\r\n");
                if (trailerEnd == std::string::npos)
                    return;
                _state = REQ_COMPLETE;
                return;
            }
        }
        if (_currentChunkSize == 0) {
            if (_chunkBuffer.size() >= 2 && _chunkBuffer.compare(0, 2, "\r\n") == 0) {
                _state = REQ_COMPLETE;
                return;
            }
            size_t trailerEnd = _chunkBuffer.find("\r\n\r\n");
            if (trailerEnd == std::string::npos)
                return;
            _state = REQ_COMPLETE;
            return;
        }
        if (_chunkBuffer.size() < 2 || _currentChunkSize > _chunkBuffer.size() - 2)
            return;
        if (_chunkBuffer.compare(_currentChunkSize, 2, "\r\n") != 0) {
            setError(400);
            return;
        }
        if (!appendBody(_chunkBuffer.data(), _currentChunkSize))
            return;
        _chunkBuffer.erase(0, _currentChunkSize + 2);
        _currentChunkSize = 0;
        _readingChunkSize = true;
    }
}

void HttpRequest::feedData(const char* buffer, size_t length) {
    if (_state == REQ_ERROR || _state == REQ_COMPLETE || length == 0)
        return;
    if (_state == REQ_HEADERS) {
        _rawHeaders.append(buffer, length);
        size_t requestLineEnd = _rawHeaders.find("\r\n");
        if ((requestLineEnd == std::string::npos && _rawHeaders.size() > 8192)
            || (requestLineEnd != std::string::npos && requestLineEnd > 8192)) {
            setError(414);
            return;
        }
        size_t headerEnd = _rawHeaders.find("\r\n\r\n");
        if (headerEnd == std::string::npos) {
            if (_rawHeaders.size() > 65536)
                setError(431);
            return;
        }
        if (headerEnd + 4 > 65536) {
            setError(431);
            return;
        }
        parseRequestLine();
        if (_state == REQ_ERROR)
            return;
        parseHeaders();
        if (_state == REQ_ERROR)
            return;
        size_t bodyStart = headerEnd + 4;
        std::string leftover = _rawHeaders.substr(bodyStart);
        _rawHeaders.resize(bodyStart);
        if (_state == REQ_BODY_DISCARD) {
            if (!leftover.empty())
                processDiscardBody(leftover.size());
            return;
        } else if (_isChunked)
            _state = REQ_BODY_CHUNKED;
        else if (_contentLength > 0)
            _state = REQ_BODY_NORMAL;
        else {
            _state = REQ_COMPLETE;
            return;
        }
        if (!leftover.empty()) {
            if (_state == REQ_BODY_CHUNKED)
                processChunkedData(leftover.data(), leftover.size());
            else
                processNormalBody(leftover.data(), leftover.size());
        }
    } else if (_state == REQ_BODY_CHUNKED)
        processChunkedData(buffer, length);
    else if (_state == REQ_BODY_DISCARD)
        processDiscardBody(length);
    else if (_state == REQ_BODY_NORMAL)
        processNormalBody(buffer, length);
}

void HttpRequest::finishInput() {
    if (_state != REQ_COMPLETE && _state != REQ_ERROR)
        setError(400);
}

bool HttpRequest::isComplete() const { return _state == REQ_COMPLETE; }
bool HttpRequest::hasError() const { return _state == REQ_ERROR; }
std::string HttpRequest::getMethod() const { return _method; }
std::string HttpRequest::getUri() const { return _uri; }
std::string HttpRequest::getPath() const { return _path; }
std::string HttpRequest::getQueryString() const { return _queryString; }
std::string HttpRequest::getVersion() const { return _version; }

std::string HttpRequest::getHeader(const std::string& name) const {
    std::map<std::string, std::string>::const_iterator it = _headers.find(lowerString(name));
    if (it == _headers.end())
        return "";
    return it->second;
}

const std::map<std::string, std::string>& HttpRequest::getHeaders() const { return _headers; }
const std::vector<char>& HttpRequest::getBody() const { return _body; }
RequestState HttpRequest::getState() const { return _state; }
size_t HttpRequest::getContentLength() const { return _contentLength; }
size_t HttpRequest::getBodyBytesRead() const { return _bodyBytesRead; }
bool HttpRequest::getIsChunked() const { return _isChunked; }
int HttpRequest::getErrorCode() const { return _errorCode; }
