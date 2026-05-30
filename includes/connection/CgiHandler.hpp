#pragma once
#include <string>
#include <map>
#include <sys/types.h>
#include "HttpRequest.hpp"
#include "LocationConfig.hpp"

class CgiHandler {
private:
    std::map<std::string, std::string>  _env;
    std::string                         _scriptPath;
    int                                 _pipeIn[2];
    int                                 _pipeOut[2];
    pid_t                               _pid;

public:
    CgiHandler();
    void    initEnv(const HttpRequest& req, const LocationConfig* loc);
    bool    executeCgi();
    int     getReadFd() const; // To give to poll()
    int     getWriteFd() const; // To give to poll()
};
