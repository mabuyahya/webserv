#ifndef SERVER_HPP
#define SERVER_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <vector>
#include <string>
#include "config_parser.hpp"

#define MAX_EVENTS 64

class Server
{
private:
	int server_fd;
	int epoll_fd;
	struct sockaddr_in address;
	std::string host;
	int port;
	ServerConfig config;

	void create_socket();
	void set_socket_options();
	void bind_socket();
	void listen_socket();
	void create_epoll();
	void add_to_epoll(int fd, uint32_t events);
	void remove_from_epoll(int fd);
	void handle_new_connection();
	void handle_client_data(int client_fd);
	void parse_host_port(const std::string& host_port);

public:
	Server(const ServerConfig& server_config);
	~Server();

	Server(const Server& other);
	Server& operator=(const Server& other);

	void launch();
	void run();
};

#endif
