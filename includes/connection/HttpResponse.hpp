#pragma once
#include <string>
#include <map>
#include "HttpRequest.hpp"
#include "CgiHandler.hpp"

class HttpResponse {
private:
    int                                 _statusCode;
    std::map<std::string, std::string>  _headers;
    std::string                         _headerBuffer;
    bool                                _headersSent;

    // For static files
    int                                 _fileFd;
    bool                                _isFileResponse;
    
    // For strings (Error pages, Dir Listing)
    std::string                         _stringBody;
    size_t                              _stringBytesSent;

    CgiHandler                          _cgi;

    void    buildErrorPage(int code, const ServerConfig* config);
    void    buildDirectoryListing(const std::string& path);
    void    formatHeaders();
 
public:
    HttpResponse();
    void    generate(const HttpRequest& req, const ServerConfig* config);
    
    // Called by ServerManager on WRITE event
    int     sendNextChunk(int client_socket); 
    bool    isComplete() const;
};