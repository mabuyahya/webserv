#include <iostream>
#include <vector>
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <sys/epoll.h>

#include "include/config_parser.hpp"

#define MAX_EVENTS 64

int main(int argc, char *argv[])
{
	/* ! the subject says:
		Your program must use a configuration file, provided as an argument on the com-mand line,
		or available in a default path.
		So this check may be unnecessary.
	*/
	if (argc < 2)
	{
		std::cerr << "Usage: " << argv[0] << " <Config File>" << std::endl;
		return 1;
	}
	if (argc == 3)
	{
		std::cout << "INFO: mock info mode" << std::endl;

		// hardcoded socket creation and binding for now

		// hardcoded socket creation and binding for now

		int server_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (server_fd == -1)
		{
			throw std::runtime_error("Failed to create socket");
		}

		int opt = 1;
		if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
		{
			throw std::runtime_error("Failed to set socket options");
		}
		fcntl(server_fd, F_SETFL, O_NONBLOCK);

		struct sockaddr_in address;
		std::memset(&address, 0, sizeof(address));
		address.sin_family = AF_INET;

		address.sin_port = htons(8080);
		address.sin_addr.s_addr = inet_addr("127.0.0.1");

		if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1)
		{
			// hardcoded error managment
			std::cerr << "Failed to bind socket: " << strerror(errno) << std::endl;
			throw std::runtime_error("Failed to bind socket");
		}

		// the magic number "10" is the backlog, essentially how many connections can be waiting at once, any more will be rejected
		if (listen(server_fd, 10) == -1)
		{
			throw std::runtime_error("Failed to listen on socket");
		}

		// the size is ignored in modern linux but should be above 0
		int epoll_fd = epoll_create(10);
		if (epoll_fd == -1)
		{
			throw std::runtime_error("Failed to create epoll instance");
		}

		struct epoll_event event;
		std::memset(&event, 0, sizeof(event));

		event.events = EPOLLIN;
		event.data.fd = server_fd;

		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1)
		{
			throw std::runtime_error("Failed to add server socket to epoll");
		}

		std::cout << "Server is listening on 127.0.0.1:8080" << std::endl;
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
					// new conn
					struct sockaddr_in client_address;
					socklen_t client_address_len = sizeof(client_address);

					int client_fd = accept(server_fd, (struct sockaddr *)&client_address, &client_address_len);
					if (client_fd == -1)
					{
						// TODO: remove this later, its just means another thread accepted it or the client dropped
						std::cerr << "Failed to accept connection" << std::endl;
						continue;
					}
					fcntl(client_fd, F_SETFL, O_NONBLOCK);

					struct epoll_event client_event;
					std::memset(&client_event, 0, sizeof(client_event));

					client_event.events = EPOLLIN;
					client_event.data.fd = client_fd;

					if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event) == -1)
					{
						throw std::runtime_error("Failed to add client socket to epoll");
					}
				}
				// its the client talking
				else
				{
					// --[READING DATA]--
					if (events[i].events & EPOLLIN)
					{
						char buffer[1024]; // a kb

						int bytes_read = recv(curr_fd, buffer, sizeof(buffer) - 1, 0);
						if (bytes_read <= 0)
						{
							// 0 means client disconnected, -1 means error
							epoll_ctl(epoll_fd, EPOLL_CTL_DEL, curr_fd, NULL);
							close(curr_fd);
						}
						else
						{
							buffer[bytes_read] = '\0';
							std::cout << "Received from client: " << buffer << std::endl;

							// write the data locally for the شباب
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
						}
						//simple response for no hanging
						const char* response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!";
						send(curr_fd, response, strlen(response), 0);
						// close the connection after responding for simplicity, can be changed later for keep-alive
						epoll_ctl(epoll_fd, EPOLL_CTL_DEL, curr_fd, NULL);
						close(curr_fd);
					}
				}
			}
		}
	}

	// * Config parsing
	try
	{
		ConfigParser parser(argv[1]);
		std::vector<ServerConfig> configs = parser.parseConfig();

		for (size_t i = 0; i < configs.size(); ++i)
		{
			std::cout << "Server " << i + 1 << ":" << std::endl;
			std::cout << "  Host: " << configs[i].host << std::endl;
			std::cout << "  Server Name: " << configs[i].server_name << std::endl;
			std::cout << "  Client Max Body Size: " << configs[i].client_max_body_size << std::endl;
			std::cout << "  Error Pages:" << std::endl;
			for (std::map<int, std::string>::iterator it = configs[i].error_pages.begin(); it != configs[i].error_pages.end(); ++it)
			{
				std::cout << "    " << it->first << ": " << it->second << std::endl;
			}
			std::cout << "  Locations:" << std::endl;
			for (size_t j = 0; j < configs[i].locations.size(); ++j)
			{
				std::cout << "    Path: " << configs[i].locations[j].path << std::endl;
				std::cout << "    Root: " << configs[i].locations[j].root << std::endl;
				std::cout << "    Index: " << configs[i].locations[j].index << std::endl;
				std::cout << "    Allow Methods: ";
				for (size_t k = 0; k < configs[i].locations[j].allow_methods.size(); ++k)
				{
					std::cout << configs[i].locations[j].allow_methods[k] << " ";
				}
				std::cout << std::endl;
				std::cout << "    Autoindex: " << (configs[i].locations[j].autoindex ? "on" : "off") << std::endl;
			}
		}

		// hardcoded socket creation and binding for now

		int server_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (server_fd == -1)
		{
			throw std::runtime_error("Failed to create socket");
		}

		int opt = 1;
		if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
		{
			throw std::runtime_error("Failed to set socket options");
		}
		fcntl(server_fd, F_SETFL, O_NONBLOCK);

		struct sockaddr_in address;
		std::memset(&address, 0, sizeof(address));
		address.sin_family = AF_INET;

		std::string ip = configs[0].host;
		std::string s_port = ip.substr(ip.find(":") + 1);
		int port = std::atoi(s_port.c_str());

		address.sin_port = htons(port);
		address.sin_addr.s_addr = inet_addr(ip.substr(0, ip.find(":")).c_str());

		if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1)
		{
			// hardcoded error managment
			std::cerr << "Failed to bind socket: " << strerror(errno) << std::endl;
			std::cerr << "ip: " << ip.substr(0, ip.find(":")) << " port: " << port << std::endl;
			throw std::runtime_error("Failed to bind socket");
		}

		// the magic number "10" is the backlog, essentially how many connections can be waiting at once, any more will be rejected
		if (listen(server_fd, 10) == -1)
		{
			throw std::runtime_error("Failed to listen on socket");
		}

		// the size is ignored in modern linux but should be above 0
		int epoll_fd = epoll_create(10);
		if (epoll_fd == -1)
		{
			throw std::runtime_error("Failed to create epoll instance");
		}

		struct epoll_event event;
		std::memset(&event, 0, sizeof(event));

		event.events = EPOLLIN;
		event.data.fd = server_fd;

		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1)
		{
			throw std::runtime_error("Failed to add server socket to epoll");
		}

		std::cout << "Server is listening on " << ip.substr(0, ip.find(":")) << ":" << port << std::endl;
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
					// new conn
					struct sockaddr_in client_address;
					socklen_t client_address_len = sizeof(client_address);

					int client_fd = accept(server_fd, (struct sockaddr *)&client_address, &client_address_len);
					if (client_fd == -1)
					{
						// TODO: remove this later, its just means another thread accepted it or the client dropped
						std::cerr << "Failed to accept connection" << std::endl;
						continue;
					}
					fcntl(client_fd, F_SETFL, O_NONBLOCK);

					struct epoll_event client_event;
					std::memset(&client_event, 0, sizeof(client_event));

					client_event.events = EPOLLIN;
					client_event.data.fd = client_fd;

					if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event) == -1)
					{
						throw std::runtime_error("Failed to add client socket to epoll");
					}
				}
				// its the client talking
				else
				{
					// --[READING DATA]--
					if (events[i].events & EPOLLIN)
					{
						char buffer[1024]; // a kb

						int bytes_read = recv(curr_fd, buffer, sizeof(buffer) - 1, 0);
						if (bytes_read <= 0)
						{
							// 0 means client disconnected, -1 means error
							epoll_ctl(epoll_fd, EPOLL_CTL_DEL, curr_fd, NULL);
							close(curr_fd);
						}
						else
						{
							buffer[bytes_read] = '\0';
							std::cout << "Received from client: " << buffer << std::endl;

							// write the data locally for the شباب
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
						}

						//simple response for no hanging
						const char* response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!";
						send(curr_fd, response, strlen(response), 0);
						// close the connection after responding for simplicity, can be changed later for keep-alive
						epoll_ctl(epoll_fd, EPOLL_CTL_DEL, curr_fd, NULL);
						close(curr_fd);
					}
				}
			}
		}
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	// * Start the server and listen

	return 0;
}
