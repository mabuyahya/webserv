*This project has been created as part of the 42 curriculum by mabuyahy, mdarawsh, hassende.*

# webserv

## Description

`webserv` is an HTTP server written in C++98 for the 42 Webserv project. The goal is to reproduce the core behavior of a real web server: parse HTTP requests, serve files, generate correct responses, run CGI scripts, and keep multiple clients active without blocking.

The server always sends HTTP/1.0 responses with closed connections. It also accepts HTTP/1.1 request syntax so standard browsers can be used during evaluation. It uses one `epoll()` event loop for listening sockets, clients, and CGI pipes. The server supports static websites, multiple listening ports, custom error pages, route-level method rules, redirects, directory indexes and listings, uploads, DELETE, request-body limits, chunked request decoding, and CGI execution.

The evaluation setup is intentionally visible: `configs_files/evaluation.conf` serves a browser console on port `8080` and a second site on port `8081`.

## Architecture

- `srcs/config`: configuration parsing, validation, inheritance, and location matching.
- `srcs/connection`: incremental request parsing, response generation, CGI, clients, and the epoll loop.
- `servers/evaluation`: static site, errors, autoindex fixtures, CGI scripts, optional PHP fixture, and upload storage.
- `tests`: socket-level HTTP tests and invalid-configuration tests.

All network sockets and CGI pipe endpoints are non-blocking. Regular files are streamed in bounded chunks.

## Instructions

Compile:

```sh
make
```

Run the complete evaluation configuration:

```sh
./webserv configs_files/evaluation.conf
```

Running without an argument uses the same evaluation configuration:

```sh
./webserv
```

Open:

- `http://127.0.0.1:8080/` for the interactive evaluation console.
- `http://127.0.0.1:8081/` to demonstrate a second listening port and root.

Optional PHP CGI test:

```sh
./webserv configs_files/php_test.conf
```

Then open `http://127.0.0.1:8082/info.php`. This requires `php-cgi` to be installed at `/usr/bin/php-cgi`. The default evaluation configuration uses Python CGI because `/usr/bin/python3` is available on the tested environment.

## Tests

With the evaluation server running in another terminal:

```sh
python3 tests/http_test.py
```

Test configuration validation:

```sh
python3 tests/config_test.py
```

Or build, start, test, stop, and clean the fixtures in one command:

```sh
./tests/run_all.sh
```

The HTTP suite covers the HTTP/1.0 response contract, browser-compatible request parsing, static content and MIME types, custom errors, method rejection, redirects, autoindex, multiple ports, CGI GET/POST/status/error behavior, CGI working directory, raw and chunked uploads, GET/DELETE, body limits, malformed requests, traversal attempts, fragmented requests, concurrency, and non-blocking CGI behavior.

## Evaluation Routes

| Route | Purpose |
|---|---|
| `/` | Interactive frontend and static index |
| `/assets/*` | CSS and JavaScript static files |
| `/files/` | Autoindex enabled |
| `/private/` | Autoindex disabled and no index file |
| `/uploads/<name>` | POST raw body, GET file, DELETE file |
| `/cgi/env.py` | CGI query, headers, body, and working-directory proof |
| `/cgi/status.py` | CGI-provided status code |
| `/cgi/broken.py` | Malformed CGI output and 502 behavior |
| `/cgi/slow.py` | Non-blocking CGI demonstration |
| `/info.php` with `configs_files/php_test.conf` | Optional PHP CGI test if `php-cgi` is installed |
| `/redirect` | Configured 302 response |
| `/get-only/` | Route-level method rejection |

## Resources

- [RFC 1945: HTTP/1.0](https://datatracker.ietf.org/doc/html/rfc1945)
- [RFC 3875: CGI/1.1](https://datatracker.ietf.org/doc/html/rfc3875)
- [Linux epoll documentation](https://man7.org/linux/man-pages/man7/epoll.7.html)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- [PHP CGI/FastCGI manual](https://www.php.net/manual/en/install.unix.commandline.php)
- NGINX was used as a behavioral reference for status codes, headers, and route behavior.

AI was used to help audit integration issues, design repeatable test cases and evaluation fixtures, identify protocol edge cases, improve the README, and review the project against the subject. The project authors remain responsible for understanding, reviewing, testing, and defending every part of the final code.
