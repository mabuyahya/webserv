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
    size_t                              _headersBytesSent;

    // For static files
    int                                 _fileFd;
    bool                                _isFileResponse;
    
    // For strings (Error pages, Dir Listing)
    std::string                         _stringBody;
    size_t                              _stringBytesSent;

    bool                                _isCgiRunning;
    CgiHandler                          _cgi;
    int                                 _cgiReadFd;
    int                                 _cgiWriteFd;
    bool                                _isGenerated;   
    bool                                _isComplete;


    void    buildErrorPage(int code, const ServerConfig* config);
    void    handleDirectory(const std::string& path, const ServerConfig* config);
    void    buildDirectoryListing(const std::string& path);
    void    formatHeaders();
    bool    isCgiTarget(const std::string& path, const ServerConfig* config);
    void    handleCgi(const HttpRequest& req, const std::string& path, const LocationConfig* loc);
    void    handleDelete(const std::string& path, const ServerConfig* config);
    void    handleStaticFile(const std::string& path, size_t fileSize, const ServerConfig* config);
public:
    HttpResponse();
    void    generate(const HttpRequest& req, const ServerConfig* config);
    
    // Called by ServerManager on WRITE event
    int     sendNextChunk(int client_socket); 
    bool     isGenerated() const;
    bool    isComplete() const;

    // Getters for CGI handling
    bool    isCgiRunning() const;
    int     getCgiReadFd() const;
    int     getCgiWriteFd() const;
};