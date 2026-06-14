#pragma once
#include <sys/epoll.h>
#include <sys/socket.h>
#include <map>
#include <vector>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include "ServerConfig.hpp"
#include "Client.hpp"

#define MAX_EVENTS 64

class ServerManager {
private:
    int                                 _epollFd;
    struct epoll_event                  _events[MAX_EVENTS];

    std::map<int, ServerConfig>         _listeningSockets;
    std::map<int, Client>               _clients;


    void    setupServers(const std::vector<ServerConfig>& configs);
    void    acceptNewConnection(int server_fd);
    void    handleClientRead(int client_fd);//
    void    handleClientWrite(int client_fd);//
    void    checkTimeouts();
    void    removeClient(int client_fd);



    void    addToEpoll(int fd, uint32_t events);
    void    modifyEpoll(int fd, uint32_t events);
    void    removeFromEpoll(int fd);
    int     createListeningSocket(const std::string& host, int port);

public:
    ServerManager(const std::vector<ServerConfig>& configs);
    ~ServerManager();

    void    run();
};
