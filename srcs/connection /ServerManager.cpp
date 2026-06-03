#include "ServerManager.hpp"
#include <fcntl.h>

ServerManager::~ServerManager() { // closes all the open fds
	for (std::map<int, ServerConfig>::iterator it = _listeningSockets.begin(); it != _listeningSockets.end(); it++) {
		close(it->first);
	}
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		close(it->first);
	}
	if (_epollFd != -1)
		close(_epollFd);
}

ServerManager::ServerManager(const std::vector<ServerConfig>& configs) { // create epoll fd and setupservers
	_epollFd = epoll_create(1);
	if (_epollFd == -1) {
		std::cerr << "Failed to create epoll instance" << std::endl;
		exit(EXIT_FAILURE);
	}
	setupServers(configs);
}

void ServerManager::setupServers(const std::vector<ServerConfig>& configs) { // creates all the listening sockets in the config
	for (size_t i = 0; i < configs.size(); i++)
	{
		int server_fd = createListeningSocket(configs[i].getHost(), configs[i].getPort());
		if (server_fd == -1) {
			std::cerr << "Failed to create listening socket for " << configs[i].getHost() << ":" << configs[i].getPort() << std::endl;
			continue;
		}

		fcntl(server_fd, F_SETFL, O_NONBLOCK);

		if (listen(server_fd, SOMAXCONN) == -1) {
			std::cerr << "Failed to listen on socket for " << configs[i].getHost() << ":" << configs[i].getPort() << std::endl;
			close(server_fd);
			continue;
		}

		_listeningSockets[server_fd] = configs[i];
		addToEpoll(server_fd, EPOLLIN);
	}
}

int ServerManager::createListeningSocket(const std::string& host, int port) { // creates a listening socket according to the config file
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) {
		std::cerr << "Failed to create socket" << std::endl;
		return -1;
	}

	int opt = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
		std::cerr << "Failed to set socket options: Error occured" << std::endl;
		close(fd);
		return -1;
	}

	struct sockaddr_in address;
	std::memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(host.c_str());
	address.sin_port = htons(port);

	if (bind(fd, (struct sockaddr*)&address, sizeof(address)) == -1) {
		std::cerr << "Failed to bind socket to " << host << ":" << port << std::endl;
		close(fd);
		return -1;
	}

	// We don't listen here
	return fd;
}


void ServerManager::addToEpoll(int fd, uint32_t events) {
	struct epoll_event event;
	event.events = events;
	event.data.fd = fd;
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &event) == -1) {
		std::cerr << "Failed to add fd to epoll" << std::endl;
		close(fd);
	}
}

void ServerManager::removeFromEpoll(int fd) {
	struct epoll_event event; // since epoll_ctl requires a non-null pointer we use a dummy one
	if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, &event) == -1) {
		std::cerr << "Failed to remove fd from epoll" << std::endl;
	}
}

void ServerManager::modifyEpoll(int fd, uint32_t events) {
	struct epoll_event event;
	event.events = events;
	event.data.fd = fd;
	if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, fd, &event) == -1) {
		std::cerr << "Failed to modify fd in epoll" << std::endl;
	}
}

void ServerManager::removeClient(int client_fd) {
	if (_clients.count(client_fd)) {
		removeFromEpoll(client_fd);
		close(client_fd);
		_clients.erase(client_fd);
	}
}

void ServerManager::run() { // takes the epoll wait result and at according to it eather new connection or read or write
	while (true)
	{
		checkTimeouts();
		int numEvents = epoll_wait(_epollFd, _events, MAX_EVENTS, 1000);
		if (numEvents == -1)
		{
			std::cerr << "Failed to wait on epoll" << std::endl;
			continue;
		}

		for (int i = 0; i < numEvents; i++)
		{
			int currFD = _events[i].data.fd;
			uint32_t currEvent = _events[i].events;

			if (currEvent & EPOLLERR || currEvent & EPOLLHUP) {
				removeClient(currFD);
				continue;
			}

			if (currEvent & EPOLLIN)
			{
				if (_listeningSockets.count(currFD))
					acceptNewConnection(currFD);
				else if (_clients.count(currFD))
					handleClientRead(currFD);
			}
			else if (currEvent & EPOLLOUT)
			{
				if (_clients.count(currFD))
					handleClientWrite(currFD);
			}
		}
	}
}

void ServerManager::checkTimeouts() {
	time_t now = time(NULL);
	std::vector<int> timedOutClients;

	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		if (it->second.hasTimedOut()) {
			timedOutClients.push_back(it->first);
		}
	}

	for (size_t i = 0; i < timedOutClients.size(); i++) {
		removeClient(timedOutClients[i]);
	}
}

void ServerManager::acceptNewConnection(int server_fd) {
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
	if (client_fd == -1) {
		std::cerr << "Failed to accept new connection" << std::endl;
		return;
	}

	if (fcntl(client_fd, F_SETFL, O_NONBLOCK) == -1)
	{
		std::cerr << "Failed to set client socket to non-blocking" << std::endl;
		close(client_fd);
		return;
	}
	_clients[client_fd] = Client(client_fd, &_listeningSockets[server_fd]);
	addToEpoll(client_fd, EPOLLIN);
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
