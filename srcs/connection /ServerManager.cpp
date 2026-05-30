#include "ServerManager.hpp"

ServerManager::~ServerManager() { // closes all the open fds
}

ServerManager::ServerManager(const std::vector<ServerConfig>& configs) { // create epoll fd and setupservers
}

void ServerConfig::setupServers(const std::vector<ServerConfig>& configs) { // creates all the listening sockets in the config
}

int ServerManager::createListeningSocket(const std::string& host, int port) { // creates a listening socket according to the config file
}


void ServerManager::addToEpoll(int fd, uint32_t events) {
}

void ServerManager::removeFromEpoll(int fd) {
}

void ServerManager::modifyEpoll(int fd, uint32_t events) {
}

void ServerManager::removeClient(int client_fd) {
}

void ServerManager::run() { // takes the epoll wait result and at according to it eather new connection or read or write
}

void ServerManager::checkTimeouts() {
}

void ServerManager::acceptNewConnection(int server_fd) {
}

void ServerManager::handleClientRead(int client_fd) {
    Client& client = _clients[client_fd];
    char buffer[4096];
    ssize_t bytesRead = recv(client_fd, buffer, sizeof(buffer), 0);
    if (bytesRead == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            removeClient(client_fd);
        }
        return;
    } else if (bytesRead == 0) {
        removeClient(client_fd);
        return;
    }

    client.updateActivity();
    client.getRequest().feedData(buffer, bytesRead);
    if (client.getRequest().isComplete()) {
        modifyEpoll(client_fd, EPOLLOUT);
    } else if (client.getRequest().hasError()) {
        modifyEpoll(client_fd, EPOLLOUT);
    }
}