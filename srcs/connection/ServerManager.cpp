#include "ServerManager.hpp"

#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

static volatile sig_atomic_t gRunning = 1;

static void stopServer(int signalNumber) {
    (void)signalNumber;
    gRunning = 0;
}

ServerManager::ServerManager(const std::vector<ServerConfig>& configs) : _epollFd(-1) {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, stopServer);
    signal(SIGTERM, stopServer);
    _epollFd = epoll_create(1);
    if (_epollFd == -1)
        throw std::runtime_error("Could not create epoll instance");
    fcntl(_epollFd, F_SETFD, FD_CLOEXEC);
    setupServers(configs);
    if (_listeningSockets.empty())
        throw std::runtime_error("Could not bind any configured listening socket");
}

ServerManager::~ServerManager() {
    for (std::map<int, int>::iterator it = _cgiToClient.begin(); it != _cgiToClient.end(); ++it)
        close(it->first);
    for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        if (it->second.getResponse().isCgiRunning())
            kill(it->second.getResponse().getCgiPid(), SIGKILL);
        close(it->first);
    }
    for (std::map<int, ServerConfig>::iterator it = _listeningSockets.begin();
         it != _listeningSockets.end(); ++it)
        close(it->first);
    if (_epollFd != -1)
        close(_epollFd);
}

bool ServerManager::addToEpoll(int fd, uint32_t events) {
    struct epoll_event event;
    event.events = events;
    event.data.fd = fd;
    if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &event) == -1) {
        std::cerr << "webserv: could not monitor descriptor " << fd << std::endl;
        return false;
    }
    return true;
}

void ServerManager::modifyEpoll(int fd, uint32_t events) {
    struct epoll_event event;
    event.events = events;
    event.data.fd = fd;
    if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, fd, &event) == -1)
        removeClient(fd);
}

void ServerManager::removeFromEpoll(int fd) {
    epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL);
}

int ServerManager::createListeningSocket(const std::string& host, int port) {
    std::ostringstream service;
    service << port;
    struct addrinfo hints;
    hints.ai_flags = 0;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_addrlen = 0;
    hints.ai_addr = NULL;
    hints.ai_canonname = NULL;
    hints.ai_next = NULL;
    struct addrinfo* addresses = NULL;
    if (getaddrinfo(host.c_str(), service.str().c_str(), &hints, &addresses) != 0)
        return -1;
    int fd = -1;
    for (struct addrinfo* current = addresses; current != NULL; current = current->ai_next) {
        fd = socket(current->ai_family, current->ai_socktype, current->ai_protocol);
        if (fd == -1)
            continue;
        int enabled = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled)) == -1
            || bind(fd, current->ai_addr, current->ai_addrlen) == -1) {
            close(fd);
            fd = -1;
            continue;
        }
        break;
    }
    freeaddrinfo(addresses);
    return fd;
}

void ServerManager::setupServers(const std::vector<ServerConfig>& configs) {
    for (size_t i = 0; i < configs.size(); ++i) {
        int fd = createListeningSocket(configs[i].getHost(), configs[i].getPort());
        if (fd == -1) {
            std::cerr << "webserv: could not bind " << configs[i].getHost()
                      << ":" << configs[i].getPort() << std::endl;
            continue;
        }
        if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1 || listen(fd, SOMAXCONN) == -1) {
            close(fd);
            continue;
        }
        fcntl(fd, F_SETFD, FD_CLOEXEC);
        _listeningSockets.insert(std::make_pair(fd, configs[i]));
        if (!addToEpoll(fd, EPOLLIN)) {
            close(fd);
            _listeningSockets.erase(fd);
            continue;
        }
        std::cout << "webserv: listening on " << configs[i].getHost()
                  << ":" << configs[i].getPort() << std::endl;
    }
}

void ServerManager::acceptNewConnection(int serverFd) {
    struct sockaddr_storage address;
    socklen_t length = sizeof(address);
    int clientFd = accept(serverFd, reinterpret_cast<struct sockaddr*>(&address), &length);
    if (clientFd == -1)
        return;
    if (fcntl(clientFd, F_SETFL, O_NONBLOCK) == -1) {
        close(clientFd);
        return;
    }
    fcntl(clientFd, F_SETFD, FD_CLOEXEC);
    _clients.insert(std::make_pair(clientFd, Client(clientFd, &_listeningSockets[serverFd])));
    if (!addToEpoll(clientFd, EPOLLIN | EPOLLRDHUP)) {
        close(clientFd);
        _clients.erase(clientFd);
    }
}

void ServerManager::handleClientRead(int clientFd) {
    char buffer[16384];
    ssize_t count = recv(clientFd, buffer, sizeof(buffer), 0);
    if (count < 0)
        return;
    if (count == 0) {
        std::map<int, Client>::iterator found = _clients.find(clientFd);
        if (found != _clients.end()) {
            found->second.getRequest().finishInput();
            modifyEpoll(clientFd, EPOLLOUT);
        }
        return;
    }
    if (!_clients.count(clientFd)) {
        removeClient(clientFd);
        return;
    }
    Client& client = _clients.find(clientFd)->second;
    client.updateActivity();
    client.getRequest().feedData(buffer, static_cast<size_t>(count));
    if (client.getRequest().isComplete() || client.getRequest().hasError())
        modifyEpoll(clientFd, EPOLLOUT | EPOLLRDHUP);
}

void ServerManager::handleClientWrite(int clientFd) {
    std::map<int, Client>::iterator found = _clients.find(clientFd);
    if (found == _clients.end())
        return;
    Client& client = found->second;
    HttpResponse& response = client.getResponse();
    client.updateActivity();
    if (!response.isGenerated()) {
        response.generate(client.getRequest(), client.getServerConfig());
        if (response.isCgiRunning()) {
            int readFd = response.getCgiReadFd();
            int writeFd = response.getCgiWriteFd();
            removeFromEpoll(clientFd);
            _cgiToClient[readFd] = clientFd;
            if (!addToEpoll(readFd, EPOLLIN | EPOLLHUP)) {
                failCgiClient(clientFd);
                return;
            }
            if (client.getRequest().getBody().empty())
                close(writeFd);
            else {
                _cgiToClient[writeFd] = clientFd;
                if (!addToEpoll(writeFd, EPOLLOUT | EPOLLHUP)) {
                    failCgiClient(clientFd);
                    return;
                }
            }
            return;
        }
    }
    if (response.sendNextChunk(clientFd) == -1 || response.isComplete())
        removeClient(clientFd);
}

void ServerManager::closeCgiPipe(int pipeFd) {
    removeFromEpoll(pipeFd);
    close(pipeFd);
    _cgiToClient.erase(pipeFd);
}

void ServerManager::handleCgiWrite(int pipeFd) {
    std::map<int, int>::iterator mapping = _cgiToClient.find(pipeFd);
    if (mapping == _cgiToClient.end())
        return;
    std::map<int, Client>::iterator found = _clients.find(mapping->second);
    if (found == _clients.end()) {
        closeCgiPipe(pipeFd);
        return;
    }
    HttpResponse& response = found->second.getResponse();
    const std::vector<char>& body = found->second.getRequest().getBody();
    size_t offset = response.getCgiBytesWritten();
    size_t count = std::min(body.size() - offset, static_cast<size_t>(4096));
    ssize_t written = write(pipeFd, &body[offset], count);
    if (written < 0) {
        failCgiClient(found->first);
        return;
    }
    if (written > 0)
        response.addCgiBytesWritten(static_cast<size_t>(written));
    if (response.getCgiBytesWritten() == body.size())
        closeCgiPipe(pipeFd);
}

void ServerManager::handleCgiRead(int pipeFd) {
    std::map<int, int>::iterator mapping = _cgiToClient.find(pipeFd);
    if (mapping == _cgiToClient.end())
        return;
    int clientFd = mapping->second;
    std::map<int, Client>::iterator found = _clients.find(clientFd);
    if (found == _clients.end()) {
        closeCgiPipe(pipeFd);
        return;
    }
    char buffer[16384];
    ssize_t count = read(pipeFd, buffer, sizeof(buffer));
    if (count > 0) {
        if (!found->second.getResponse().appendCgiOutput(buffer, static_cast<size_t>(count))) {
            failCgiClient(clientFd);
            return;
        }
        found->second.updateActivity();
        return;
    }
    if (count < 0) {
        failCgiClient(clientFd);
        return;
    }
    closeCgiPipe(pipeFd);
    pid_t cgiPid = found->second.getResponse().getCgiPid();
    if (waitpid(cgiPid, NULL, WNOHANG) == 0) {
        kill(cgiPid, SIGKILL);
        waitpid(cgiPid, NULL, WNOHANG);
    }
    found->second.getResponse().finishCgi(found->second.getServerConfig());
    if (!addToEpoll(clientFd, EPOLLOUT | EPOLLRDHUP))
        removeClient(clientFd);
}

void ServerManager::removeClient(int clientFd) {
    std::map<int, Client>::iterator client = _clients.find(clientFd);
    if (client == _clients.end())
        return;
    pid_t cgiPid = client->second.getResponse().getCgiPid();
    for (std::map<int, int>::iterator it = _cgiToClient.begin(); it != _cgiToClient.end();) {
        if (it->second == clientFd) {
            int fd = it->first;
            ++it;
            closeCgiPipe(fd);
        } else
            ++it;
    }
    if (cgiPid > 0 && client->second.getResponse().isCgiRunning())
        kill(cgiPid, SIGKILL);
    removeFromEpoll(clientFd);
    close(clientFd);
    _clients.erase(client);
}

void ServerManager::failCgiClient(int clientFd) {
    std::map<int, Client>::iterator client = _clients.find(clientFd);
    if (client == _clients.end())
        return;
    pid_t cgiPid = client->second.getResponse().getCgiPid();
    for (std::map<int, int>::iterator it = _cgiToClient.begin(); it != _cgiToClient.end();) {
        if (it->second == clientFd) {
            int fd = it->first;
            ++it;
            closeCgiPipe(fd);
        } else
            ++it;
    }
    if (cgiPid > 0)
        kill(cgiPid, SIGKILL);
    waitpid(cgiPid, NULL, WNOHANG);
    client->second.getResponse().failCgi(client->second.getServerConfig());
    if (!addToEpoll(clientFd, EPOLLOUT | EPOLLRDHUP))
        removeClient(clientFd);
}

void ServerManager::checkTimeouts() {
    for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end();) {
        int fd = it->first;
        it->second.incrementIdle();
        ++it;
        if (_clients.find(fd) != _clients.end() && _clients.find(fd)->second.hasTimedOut())
            removeClient(fd);
    }
    while (waitpid(-1, NULL, WNOHANG) > 0) {
    }
}

void ServerManager::run() {
    while (gRunning) {
        int count = epoll_wait(_epollFd, _events, MAX_EVENTS, 1000);
        if (count < 0) {
            throw std::runtime_error("epoll_wait failed");
        }
        for (int i = 0; i < count; ++i) {
            int fd = _events[i].data.fd;
            uint32_t events = _events[i].events;
            if (_listeningSockets.count(fd)) {
                if (events & EPOLLIN)
                    acceptNewConnection(fd);
                continue;
            }
            if (_cgiToClient.count(fd)) {
                int clientFd = _cgiToClient[fd];
                std::map<int, Client>::iterator client = _clients.find(clientFd);
                bool isReadPipe = client != _clients.end()
                    && client->second.getResponse().getCgiReadFd() == fd;
                if (isReadPipe && (events & (EPOLLIN | EPOLLHUP)))
                    handleCgiRead(fd);
                else if (events & (EPOLLHUP | EPOLLERR)) {
                    failCgiClient(clientFd);
                } else if (events & EPOLLOUT)
                    handleCgiWrite(fd);
                continue;
            }
            if (!_clients.count(fd))
                continue;
            if (events & (EPOLLERR | EPOLLHUP)) {
                removeClient(fd);
                continue;
            }
            if (events & EPOLLIN)
                handleClientRead(fd);
            if (_clients.count(fd) && (events & EPOLLOUT))
                handleClientWrite(fd);
            if (_clients.count(fd) && (events & EPOLLRDHUP) && !(events & EPOLLIN)) {
                _clients.find(fd)->second.getRequest().finishInput();
                modifyEpoll(fd, EPOLLOUT);
            }
        }
        checkTimeouts();
    }
}
