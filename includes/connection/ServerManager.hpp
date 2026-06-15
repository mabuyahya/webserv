#pragma once

#include <map>
#include <sys/epoll.h>
#include <vector>
#include "Client.hpp"
#include "ServerConfig.hpp"

#define MAX_EVENTS 128

class ServerManager {
private:
    int                                 _epollFd;
    struct epoll_event                  _events[MAX_EVENTS];
    std::map<int, ServerConfig>         _listeningSockets;
    std::map<int, Client>               _clients;
    std::map<int, int>                  _cgiToClient;

    void    setupServers(const std::vector<ServerConfig>& configs);
    int     createListeningSocket(const std::string& host, int port);
    void    acceptNewConnection(int serverFd);
    void    handleClientRead(int clientFd);
    void    handleClientWrite(int clientFd);
    void    handleCgiRead(int pipeFd);
    void    handleCgiWrite(int pipeFd);
    void    checkTimeouts();
    void    removeClient(int clientFd);
    void    failCgiClient(int clientFd);
    void    closeCgiPipe(int pipeFd);
    bool    addToEpoll(int fd, uint32_t events);
    void    modifyEpoll(int fd, uint32_t events);
    void    removeFromEpoll(int fd);

public:
    ServerManager(const std::vector<ServerConfig>& configs);
    ~ServerManager();
    void    run();
};
