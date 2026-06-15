
#include "Client.hpp"
Client::Client(int fd, const ServerConfig* config)
    : _socketFd(fd), _serverConfig(config), _idleCycles(0),
      _state(READING_REQUEST), _request(config) {}

bool Client::hasTimedOut() const {
    return _idleCycles > 300;
}

void Client::updateActivity() {
    _idleCycles = 0;
}

void Client::incrementIdle() {
    ++_idleCycles;
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
