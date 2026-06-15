#pragma once

#include <map>
#include <string>
#include "HttpRequest.hpp"
#include "CgiHandler.hpp"

class HttpResponse {
private:
    int                                 _statusCode;
    std::map<std::string, std::string>  _headers;
    std::string                         _body;
    std::string                         _output;
    size_t                              _bytesSent;
    bool                                _isGenerated;
    bool                                _isComplete;
    int                                 _fileFd;
    size_t                              _fileRemaining;
    std::string                         _fileBuffer;
    size_t                              _fileBufferBytesSent;
    bool                                _isFileResponse;

    bool                                _isCgiRunning;
    CgiHandler                          _cgi;
    int                                 _cgiReadFd;
    int                                 _cgiWriteFd;
    size_t                              _cgiBytesWritten;
    std::string                         _cgiOutput;

    void    resetResponse();
    void    buildErrorPage(int code, const ServerConfig* config);
    bool    buildDirectoryListing(const std::string& uri, const std::string& path);
    void    finalize();
    bool    finalizeFile(const std::string& path, size_t fileSize);
    void    handleCgi(const HttpRequest& req, const std::string& path,
                      const LocationConfig* location, const ServerConfig* config);
    void    handleUpload(const HttpRequest& req, const LocationConfig* location,
                         const ServerConfig* config);

public:
    HttpResponse();
    ~HttpResponse();
    void    generate(const HttpRequest& req, const ServerConfig* config);
    int     sendNextChunk(int clientSocket);

    bool    isGenerated() const;
    bool    isComplete() const;
    bool    isCgiRunning() const;
    int     getCgiReadFd() const;
    int     getCgiWriteFd() const;
    pid_t   getCgiPid() const;
    size_t  getCgiBytesWritten() const;
    void    addCgiBytesWritten(size_t count);
    bool    appendCgiOutput(const char* data, size_t count);
    void    finishCgi(const ServerConfig* config);
    void    failCgi(const ServerConfig* config);
};
