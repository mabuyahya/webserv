
#include "Client.hpp"

Client::Client(int fd, const ServerConfig* config)
    : _socketFd(fd), _serverConfig(config), _lastActivityTime(time(NULL)), _state(READING_REQUEST) {
    memset(&_address, 0, sizeof(_address));
}

bool Client::hasTimedOut() const {
    return (time(NULL) - _lastActivityTime) > 60; // 60 seconds timeout
}

void Client::updateActivity() {
    _lastActivityTime = time(NULL);
}


void Client::setClientState(ClientState newState) {
    _state = newState;
}

void Client::setRequest(const HttpRequest& request) {
    _request = request;
}
void Client::setResponse(const HttpResponse& response) {
    _response = response;
}

int Client::getSocketFd() const {
    return _socketFd;
}

const ServerConfig* Client::getServerConfig() const {
    return _serverConfig;
}

ClientState Client::getState() const {
    return _state;
}

HttpRequest& Client::getRequest() {
    return _request;
}

HttpResponse& Client::getResponse() {
    return _response;
}
