#include "CgiHandler.hpp"

#include <unistd.h>
#include <cstring>
CgiHandler::CgiHandler() : _pid(-1) {
    _pipeIn[0] = -1;
    _pipeIn[1] = -1;
    _pipeOut[0] = -1;
    _pipeOut[1] = -1;
}

void CgiHandler::initEnv(const HttpRequest& req, const std::string& scriptPath, const LocationConfig* loc) {
    _scriptPath = scriptPath;
    _env.clear();
    _env["REQUEST_METHOD"] = req.getMethod();
    // _env["QUERY_STRING"] = req.getQueryString();
    _env["CONTENT_TYPE"] = req.getHeaders()["Content-Type"];
    _env["CONTENT_LENGTH"] = req.getHeaders()["Content-Length"];
    _env["SCRIPT_NAME"] = scriptPath;
    _env["SERVER_PROTOCOL"] = "HTTP/1.1"; // Simplification
    _env["GATEWAY_INTERFACE"] = "CGI/1.1"; // Simplification
    // Add more environment variables as needed
}

bool CgiHandler::executeCgi() {
    // This function should fork and execute the CGI script, setting up pipes for communication
    // For simplicity, we will not implement the full CGI execution logic here
    if (pipe(_pipeIn) == -1 || pipe(_pipeOut) == -1) {
        return false;
    }
    _pid = fork();
    if (_pid == -1) {
        return false;
    }
    if(_pid == 0) {
        // Child process
        dup2(_pipeIn[0], STDIN_FILENO);
        dup2(_pipeOut[1], STDOUT_FILENO);
        close(_pipeIn[1]);
        close(_pipeOut[0]);
        // Set up environment variables
        char* envp[_env.size() + 1];
        size_t i = 0;
        for (std::map<std::string, std::string>::const_iterator it = _env.begin(); it != _env.end(); ++it) {
            std::string envVar = it->first + "=" + it->second;
            envp[i] = new char[envVar.size() + 1];
            std::strcpy(envp[i], envVar.c_str());
            ++i;
        }
        envp[i] = NULL;
        execl(_scriptPath.c_str(), _scriptPath.c_str(), NULL, envp);
        // If execl fails
        exit(1);
    } else {
        // Parent process
        close(_pipeIn[0]);
        close(_pipeOut[1]);
    }
    return true;
}

int CgiHandler::getReadFd() const {
    return _pipeOut[0];
}

int CgiHandler::getWriteFd() const {
    return _pipeIn[1];
}