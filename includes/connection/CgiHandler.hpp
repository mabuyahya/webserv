#pragma once

#include <map>
#include <string>
#include <sys/types.h>
#include "HttpRequest.hpp"
#include "LocationConfig.hpp"

class CgiHandler {
private:
    std::map<std::string, std::string>  _env;
    std::string                         _scriptPath;
    std::string                         _interpreter;
    int                                 _pipeIn[2];
    int                                 _pipeOut[2];
    pid_t                               _pid;

public:
    CgiHandler();
    void    initEnv(const HttpRequest& req, const std::string& scriptPath,
                    const LocationConfig* location);
    bool    executeCgi();
    int     getReadFd() const;
    int     getWriteFd() const;
    pid_t   getPid() const;
};
