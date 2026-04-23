#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fstream>
#include <cstdlib>
#include <errno.h>
#include "server.hpp"

Server::Server(const ServerConfig& server_config)
	: server_fd(-1), epoll_fd(-1), config(server_config)
{
	parse_host_port(config.host);
	std::memset(&address, 0, sizeof(address));
}

Server::~Server()
{
	if (epoll_fd != -1)
		close(epoll_fd);
	if (server_fd != -1)
		close(server_fd);
}

Server::Server(const Server& other)
	: server_fd(-1), epoll_fd(-1), address(other.address),
	  host(other.host), port(other.port), config(other.config)
{
}

Server& Server::operator=(const Server& other)
{
	if (this != &other)
	{
		host = other.host;
		port = other.port;
		config = other.config;
		address = other.address;
	}
	return *this;
}

void Server::parse_host_port(const std::string& host_port)
{
	size_t colon_pos = host_port.find(":");
	if (colon_pos == std::string::npos)
	{
		throw std::runtime_error("Invalid host format. Expected 'host:port'");
	}
	host = host_port.substr(0, colon_pos);
	port = std::atoi(host_port.substr(colon_pos + 1).c_str());
}

void Server::create_socket()
{
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1)
	{
		throw std::runtime_error("Failed to create socket");
	}
}

void Server::set_socket_options()
{
	int opt = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
	{
		throw std::runtime_error("Failed to set socket options");
	}
	fcntl(server_fd, F_SETFL, O_NONBLOCK);
}

void Server::bind_socket()
{
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = inet_addr(host.c_str());

	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1)
	{
		std::cerr << "Failed to bind socket: " << strerror(errno) << std::endl;
		std::cerr << "IP: " << host << " Port: " << port << std::endl;
		throw std::runtime_error("Failed to bind socket");
	}
}

void Server::listen_socket()
{
	if (listen(server_fd, 10) == -1)
	{
		throw std::runtime_error("Failed to listen on socket");
	}
}

void Server::create_epoll()
{
	epoll_fd = epoll_create(10);
	if (epoll_fd == -1)
	{
		throw std::runtime_error("Failed to create epoll instance");
	}
}

void Server::add_to_epoll(int fd, uint32_t events)
{
	struct epoll_event event;
	std::memset(&event, 0, sizeof(event));
	event.events = events;
	event.data.fd = fd;

	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1)
	{
		throw std::runtime_error("Failed to add socket to epoll");
	}
}

void Server::remove_from_epoll(int fd)
{
	if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1)
	{
		std::cerr << "Warning: Failed to remove socket from epoll" << std::endl;
	}
}

void Server::handle_new_connection()
{
	struct sockaddr_in client_address;
	socklen_t client_address_len = sizeof(client_address);

	int client_fd = accept(server_fd, (struct sockaddr *)&client_address, &client_address_len);
	if (client_fd == -1)
	{
		std::cerr << "Failed to accept connection" << std::endl;
		return;
	}

	fcntl(client_fd, F_SETFL, O_NONBLOCK);
	add_to_epoll(client_fd, EPOLLIN);
}

void Server::handle_client_data(int client_fd)
{
	char buffer[1024];
	int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

	if (bytes_read <= 0)
	{
		// Client disconnected or error occurred
		remove_from_epoll(client_fd);
		close(client_fd);
	}
	else
	{
		buffer[bytes_read] = '\0';
		std::cout << "Received from client: " << buffer << std::endl;

		// Write data locally
		std::ofstream outfile("client_data.txt", std::ios::app);
		if (outfile.is_open())
		{
			outfile << buffer << std::endl;
			outfile.close();
		}
		else
		{
			std::cerr << "Failed to open file for writing" << std::endl;
		}

		// Send response
		const char* response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!";
		send(client_fd, response, strlen(response), 0);

		// Close connection
		remove_from_epoll(client_fd);
		close(client_fd);
	}
}

void Server::launch()
{
	create_socket();
	set_socket_options();
	bind_socket();
	listen_socket();
	create_epoll();
	add_to_epoll(server_fd, EPOLLIN);
	std::cout << "Server is listening on " << host << ":" << port << std::endl;
}

void Server::run()
{
	struct epoll_event events[MAX_EVENTS];

	while (1)
	{
		int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

		if (num_events == -1)
		{
			throw std::runtime_error("Failed to wait on epoll");
		}

		for (int i = 0; i < num_events; i++)
		{
			int curr_fd = events[i].data.fd;

			if (curr_fd == server_fd)
			{
				// New connection
				handle_new_connection();
			}
			else
			{
				// Client data
				if (events[i].events & EPOLLIN)
				{
					handle_client_data(curr_fd);
				}
			}
		}
	}
}
