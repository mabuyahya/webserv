#include "ServerManager.hpp"

ServerManager::ServerManager(const std::vector<ServerConfig>& configs) {
    _epollFd = epoll_create1(0);
    if (_epollFd == -1) {
        throw std::runtime_error("Failed to create epoll instance");
    }
    setupServers(configs);
}

ServerManager::~ServerManager() {
    for (std::map<int, ServerConfig>::iterator it = _listeningSockets.begin(); it != _listeningSockets.end(); ++it) {
        close(it->first);
    }
    for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        close(it->first);
    }
    close(_epollFd);
}


int ServerManager::createListeningSocket(const std::string& host, int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        throw std::runtime_error("Failed to create socket");
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        close(server_fd);
        throw std::runtime_error("Failed to set socket options");
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(host.c_str());
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == -1) {
        close(server_fd);
        throw std::runtime_error("Failed to bind socket");
    }

    if (listen(server_fd, SOMAXCONN) == -1) {
        close(server_fd);
        throw std::runtime_error("Failed to listen on socket");
    }

    return server_fd;
}


void ServerConfig::setupServers(const std::vector<ServerConfig>& configs) {
    for (size_t i = 0; i < configs.size(); ++i) {
        const ServerConfig& config = configs[i];
        int server_fd = createListeningSocket(config.getHost(), config.getPort());
        _listeningSockets[server_fd] = config;
        addToEpoll(server_fd, EPOLLIN);
    }
}

void ServerManager::addToEpoll(int fd, uint32_t events) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        throw std::runtime_error("Failed to add file descriptor to epoll");
    }
}

void ServerManager::removeFromEpoll(int fd) {
    if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL) == -1) {
        throw std::runtime_error("Failed to remove file descriptor from epoll");
    }
}

void ServerManager::modifyEpoll(int fd, uint32_t events) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, fd, &ev) == -1) {
        throw std::runtime_error("Failed to modify file descriptor in epoll");
    }
}

void ServerManager::removeClient(int client_fd) {
    removeFromEpoll(client_fd);
    close(client_fd);
    _clients.erase(client_fd);
}

void ServerManager::run() {
    while (true) {
        int n = epoll_wait(_epollFd, _events, MAX_EVENTS, -1);
        if (n == -1) {
            if (errno == EINTR)
                continue;
            throw std::runtime_error("Failed to wait on epoll");
        }

        for (int i = 0; i < n; ++i) {
            int fd = _events[i].data.fd;
            uint32_t events = _events[i].events;

            if (_listeningSockets.count(fd)) {
                acceptNewConnection(fd);
            } else if (_clients.count(fd)) {
                if (events & EPOLLIN) {
                    handleClientRead(fd);
                }
                if (events & EPOLLOUT) {
                    handleClientWrite(fd);
                }
            }
        }
        checkTimeouts();
    }
}

void ServerManager::checkTimeouts() {
    time_t now = time(NULL);
    std::vector<int> toRemove;
    for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        if (now - it->second.getLastActiveTime() > 60) {
            toRemove.push_back(it->first);
        }
    }
    for (size_t i = 0; i < toRemove.size(); ++i) {
        removeClient(toRemove[i]);
    }
}

void ServerManager::acceptNewConnection(int server_fd) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            throw std::runtime_error("Failed to accept new connection");
        }
        return;
    }

    fcntl(client_fd, F_SETFL, O_NONBLOCK);
    _clients[client_fd] = Client(client_fd, &(_listeningSockets[server_fd]));
    addToEpoll(client_fd, EPOLLIN | EPOLLOUT);
}

    // void    handleClientRead(int client_fd);//
    // void    handleClientWrite(int client_fd);//

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
}