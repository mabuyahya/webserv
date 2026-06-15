#include "CgiHandler.hpp"

#include <algorithm>
#include <fcntl.h>
#include <signal.h>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

static char asciiUpper(char value) {
    if (value >= 'a' && value <= 'z')
        return static_cast<char>(value - ('a' - 'A'));
    return value;
}

static std::string cgiHeaderName(const std::string& name) {
    std::string result = "HTTP_";
    for (size_t i = 0; i < name.size(); ++i) {
        char value = name[i] == '-' ? '_' : name[i];
        result += asciiUpper(value);
    }
    return result;
}

CgiHandler::CgiHandler() : _pid(-1) {
    _pipeIn[0] = -1;
    _pipeIn[1] = -1;
    _pipeOut[0] = -1;
    _pipeOut[1] = -1;
}

void CgiHandler::initEnv(const HttpRequest& req, const std::string& scriptPath,
                         const LocationConfig* location) {
    _scriptPath = scriptPath;
    _interpreter = location->getCgiPath();
    _env.clear();
    _env["REQUEST_METHOD"] = req.getMethod();
    _env["QUERY_STRING"] = req.getQueryString();
    _env["REQUEST_URI"] = req.getUri();
    _env["CONTENT_TYPE"] = req.getHeader("content-type");
    _env["CONTENT_LENGTH"] = req.getIsChunked() ? "" : req.getHeader("content-length");
    _env["SCRIPT_FILENAME"] = scriptPath;
    _env["SCRIPT_NAME"] = req.getPath();
    _env["SERVER_PROTOCOL"] = "HTTP/1.0";
    _env["GATEWAY_INTERFACE"] = "CGI/1.1";
    _env["SERVER_SOFTWARE"] = "webserv/1.0";
    _env["REDIRECT_STATUS"] = "200";
    const std::map<std::string, std::string>& headers = req.getHeaders();
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
         it != headers.end(); ++it) {
        if (it->first != "content-type" && it->first != "content-length")
            _env[cgiHeaderName(it->first)] = it->second;
    }
}

bool CgiHandler::executeCgi() {
    if (pipe(_pipeIn) == -1)
        return false;
    if (pipe(_pipeOut) == -1) {
        close(_pipeIn[0]);
        close(_pipeIn[1]);
        return false;
    }
    _pid = fork();
    if (_pid == -1) {
        close(_pipeIn[0]);
        close(_pipeIn[1]);
        close(_pipeOut[0]);
        close(_pipeOut[1]);
        return false;
    }
    if (_pid == 0) {
        dup2(_pipeIn[0], STDIN_FILENO);
        dup2(_pipeOut[1], STDOUT_FILENO);
        close(_pipeIn[0]);
        close(_pipeIn[1]);
        close(_pipeOut[0]);
        close(_pipeOut[1]);

        std::string directory = ".";
        std::string script = _scriptPath;
        size_t slash = _scriptPath.rfind('/');
        if (slash != std::string::npos) {
            directory = _scriptPath.substr(0, slash);
            script = _scriptPath.substr(slash + 1);
        }
        if (chdir(directory.c_str()) == -1)
            throw std::runtime_error("CGI could not enter script directory");
        std::vector<char*> envp;
        for (std::map<std::string, std::string>::const_iterator it = _env.begin();
             it != _env.end(); ++it) {
            std::string entry = it->first + "=" + it->second;
            char* value = new char[entry.size() + 1];
            std::copy(entry.begin(), entry.end(), value);
            value[entry.size()] = '\0';
            envp.push_back(value);
        }
        envp.push_back(NULL);
        char* argv[3];
        argv[0] = const_cast<char*>(_interpreter.c_str());
        argv[1] = const_cast<char*>(script.c_str());
        argv[2] = NULL;
        execve(_interpreter.c_str(), argv, &envp[0]);
        throw std::runtime_error("CGI execve failed");
    }
    close(_pipeIn[0]);
    close(_pipeOut[1]);
    if (fcntl(_pipeIn[1], F_SETFD, FD_CLOEXEC) == -1
        || fcntl(_pipeOut[0], F_SETFD, FD_CLOEXEC) == -1
        || fcntl(_pipeIn[1], F_SETFL, O_NONBLOCK) == -1
        || fcntl(_pipeOut[0], F_SETFL, O_NONBLOCK) == -1) {
        close(_pipeIn[1]);
        close(_pipeOut[0]);
        kill(_pid, SIGKILL);
        waitpid(_pid, NULL, WNOHANG);
        return false;
    }
    return true;
}

int CgiHandler::getReadFd() const { return _pipeOut[0]; }
int CgiHandler::getWriteFd() const { return _pipeIn[1]; }
pid_t CgiHandler::getPid() const { return _pid; }
