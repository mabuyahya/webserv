This project has been created as part of the 42 curriculum by hassende, <login2>, <login3>.


## Description
An HTTP server written in C++98. This project implements a non-blocking, multiplexed server capable of handling multiple client connections via a single `poll()` loop. It serves static websites, handles file uploads, and executes CGI scripts, all configured through an NGINX-style configuration file.

## Instructions
**Compilation:**
Run `make` at the root of the repository to compile the `webserv` executable.

**Execution:**
Start the server by providing a configuration file:
`./webserv [path_to_config_file.conf]`

If no configuration file is provided, the server will default to `./config/default.conf`.

## Resources
* [RFC 1945 (HTTP/1.0)](https://datatracker.ietf.org/doc/html/rfc1945)
* [RFC 2616 (HTTP/1.1)](https://datatracker.ietf.org/doc/html/rfc2616)
* Beej's Guide to Network Programming
* **AI Usage:** AI was used during the initial architecture planning phase to structure the Makefile, outline the README requirements, and generate the boilerplate entry point.
