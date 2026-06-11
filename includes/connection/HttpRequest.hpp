#pragma once
#include <string>
#include <map>
#include <vector>
#include "../../includes/config/ServerConfig.hpp"

enum RequestState {
    REQ_HEADERS,
    REQ_BODY_NORMAL,
    REQ_BODY_CHUNKED,
    REQ_ERROR,
    REQ_COMPLETE
};

class HttpRequest {
private:
    std::string                         _rawHeaders;
    std::string                         _method;       // GET, POST, DELETE
    std::string                         _uri;          // e.g., /images/cat.png
    std::string                         _version;      // HTTP/1.1
    std::map<std::string, std::string>  _headers; // e.g., "Host" -> "example.com"
    
    RequestState                        _state;
    int                                 _errorCode; // For storing specific error codes if needed (e.g., 400 for bad request)
    size_t                              _contentLength;
    size_t                              _bodyBytesRead;
    
    // For file uploads streaming to disk
    int                                 _uploadFileFd; 
    bool                                _isChunked;
    const ServerConfig*                 _serverConfig;
    std::string                         _chunkBuffer;

    std::vector<char>                      _bodyBuffer; // For storing body data if needed (e.g., for small requests or chunked data assembly)

    void    parseRequestLine();
    void    parseHeaders();
    void    processNormalBody(const char* data, size_t len);
    void    processChunkedData(const char* buffer, size_t length);
    bool    writeBodyData(const char* data, size_t len);
    bool    openUploadFile();
    bool    shouldUploadBody() const;
    void    finishBody();

public:
    HttpRequest(const ServerConfig* config);
    void    feedData(const char* buffer, size_t length); // Called by ServerManager on READ event
    
    bool    isComplete() const;
    bool    hasError() const;
    // Getters for method, uri, headers...

    std::string getrawHeaders() const;
    std::string getMethod() const;
    std::string getUri() const;
    std::string getVersion() const;
    std::map<std::string, std::string> getHeaders() const;
    RequestState getState() const;
    size_t getContentLength() const;
    size_t getBodyBytesRead() const;
    int getuploadFileFd() const;
    bool getisChunked() const;
    int getErrorCode() const;
};
