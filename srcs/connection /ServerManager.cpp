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
				else if (_cgiToClient.count(currFD))
                handleCgiRead(currFD);
			}
			else if (currEvent & EPOLLOUT)
			{
				if (_clients.count(currFD))
					handleClientWrite(currFD);
				else if (_cgiToClient.count(currFD))
                    handleCgiWrite(currFD);
			}
		}
	}
}

void ServerManager::handleCgiWrite(int pipe_fd) {
    // 1. Find the client associated with this pipe
    int client_fd = _cgiToClient[pipe_fd];
    Client& client = _clients[client_fd];

    // 2. Get the body data and calculate what's left to send
    const std::vector<char>& body = client.getRequest().getBody();
    size_t sentSoFar = client.getResponse().getCgiBytesWritten();
    size_t remaining = body.size() - sentSoFar;

    if (remaining > 0) {
        // C++98 safe way to get the raw char pointer from a vector
        const char* bodyData = &body[0]; 
        
        ssize_t written = write(pipe_fd, bodyData + sentSoFar, remaining);

        if (written > 0) {
            client.getResponse().addCgiBytesWritten(written);
        } 
        else if (written == -1 && (errno != EAGAIN && errno != EWOULDBLOCK)) {
            // FATAL ERROR: The pipe broke
            removeFromEpoll(pipe_fd);
            close(pipe_fd);
            _cgiToClient.erase(pipe_fd);
            
            // Mark response as 500 Internal Server Error and wake up the browser
            client.getResponse().buildErrorPage(500, client.getConfig());
            modifyEpoll(client_fd, EPOLLOUT); 
            return;
        }
    }

    // 3. Check if we are completely done sending the body
    if (client.getResponse().getCgiBytesWritten() >= body.size()) {
        // We pushed everything! 
        // We MUST close this pipe now. Closing the write-end tells the 
        // CGI script "EOF" so it knows to stop waiting for data and start calculating.
        removeFromEpoll(pipe_fd);
        close(pipe_fd);
        _cgiToClient.erase(pipe_fd);
    }
}

#include <sys/wait.h> // Required for waitpid

void ServerManager::handleCgiRead(int pipe_fd) {
    // 1. Find the client associated with this pipe
    int client_fd = _cgiToClient[pipe_fd];
    Client& client = _clients[client_fd];

    char buffer[8192];
    ssize_t bytesRead = read(pipe_fd, buffer, sizeof(buffer));

    if (bytesRead > 0) {
        // 2. The CGI printed something! Append it to our response string
        client.getResponse().appendCgiOutput(buffer, bytesRead);
    } 
    else if (bytesRead == 0) {
        // 3. THE MAGIC MOMENT (EOF)
        // The CGI script has finished executing and closed its stdout.
        
        // Step A: Clean up the pipes
        removeFromEpoll(pipe_fd);
        close(pipe_fd);
        _cgiToClient.erase(pipe_fd);

        // Step B: Clean up the Zombie Process
        // (You need to store the pid returned by fork() in your CgiHandler)
        int status;
        pid_t cgiPid = client.getResponse().getCgiPid();
        waitpid(cgiPid, &status, WNOHANG); 

        // Step C: Wake up the browser!
        // We put the client's network socket BACK into epoll as a WRITE event.
        // On the next loop, handleClientWrite() will trigger and send the 
        // buffered HTML to the user.
        client.getResponse().setCgiFinished(true); 
        
        // We use addToEpoll here because we removed the client earlier to put them on hold
        addToEpoll(client_fd, EPOLLOUT); 
    } 
    else if (bytesRead == -1 && (errno != EAGAIN && errno != EWOULDBLOCK)) {
        // FATAL ERROR: Read failed
        removeFromEpoll(pipe_fd);
        close(pipe_fd);
        _cgiToClient.erase(pipe_fd);
        
        client.getResponse().buildErrorPage(500, client.getConfig());
        addToEpoll(client_fd, EPOLLOUT); 
    }
}

void ServerManager::handleClientWrite(int client_fd) {
	Client& client = _clients[client_fd];
	HttpResponse& response = client.getResponse();

	if (!response.isGenerated()) {
		response.generate(client.getRequest(), client.getServerConfig());

		if (client.getResponse().isCgiRunning()) {
            int cgiReadPipe = client.getResponse().getCgiReadFd();
            int cgiWritePipe = client.getResponse().getCgiWriteFd();

            _cgiToClient[cgiReadPipe] = client_fd;
            addToEpoll(cgiReadPipe, EPOLLIN); // Watch for CGI output
            
            // If it's a POST request, we need to send the body to the CGI
            if (cgiWritePipe != -1) {
                _cgiToClient[cgiWritePipe] = client_fd;
                addToEpoll(cgiWritePipe, EPOLLOUT); 
            }
            
            // CRITICAL: We stop watching the CLIENT for now, 
            // because we are waiting on the CGI to finish.
            removeFromEpoll(client_fd); 
            return; // We exit here! We can't send anything to the client yet.
        }
	}

	int status = response.sendNextChunk(client_fd);
	if (status == -1) {
		removeClient(client_fd);
		return;
	}
	else if (client.getResponse().isComplete()) {
        removeClient(client_fd);
    }
}

void S