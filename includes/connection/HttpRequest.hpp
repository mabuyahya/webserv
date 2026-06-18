#pragma once

#include <map>
#include <string>
#include <vector>
#include "../../includes/config/ServerConfig.hpp"

enum RequestState {
    REQ_HEADERS,
    REQ_BODY_NORMAL,
    REQ_BODY_CHUNKED,
    REQ_BODY_DISCARD,
    REQ_ERROR,
    REQ_COMPLETE
};

class HttpRequest {
private:
    std::string                         _rawHeaders;
    std::string                         _method;
    std::string                         _uri;
    std::string                         _path;
    std::string                         _queryString;
    std::string                         _version;
    std::map<std::string, std::string>  _headers;
    std::vector<char>                   _body;

    RequestState                        _state;
    int                                 _errorCode;
    size_t                              _contentLength;
    size_t                              _bodyBytesRead;
    bool                                _isChunked;
    const ServerConfig*                 _serverConfig;
    std::string                         _chunkBuffer;
    size_t                              _currentChunkSize;
    bool                                _readingChunkSize;

    void    setError(int code);
    void    parseRequestLine();
    void    parseHeaders();
    void    processNormalBody(const char* data, size_t len);
    void    processChunkedData(const char* data, size_t len);
    void    processDiscardBody(size_t len);
    bool    appendBody(const char* data, size_t len);

public:
    HttpRequest(const ServerConfig* config);
    void    feedData(const char* buffer, size_t length);
    void    finishInput();

    bool    isComplete() const;
    bool    hasError() const;
    std::string getMethod() const;
    std::string getUri() const;
    std::string getPath() const;
    std::string getQueryString() const;
    std::string getVersion() const;
    std::string getHeader(const std::string& name) const;
    const std::map<std::string, std::string>& getHeaders() const;
    const std::vector<char>& getBody() const;
    RequestState getState() const;
    size_t getContentLength() const;
    size_t getBodyBytesRead() const;
    bool getIsChunked() const;
    int getErrorCode() const;
};
