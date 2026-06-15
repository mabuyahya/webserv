*This project has been created as part of the 42 curriculum by hassende, mabuyahy, mdarawsh.*

# webserv

## Description

`webserv` is an HTTP/1.0 server written in C++98. It always sends HTTP/1.0 responses with closed connections. It also accepts HTTP/1.1 request syntax so the evaluation frontend works in standard browsers, while preserving HTTP/1.0 response semantics. It uses one `epoll()` event loop for listening sockets, clients, and CGI pipes. The server supports static sites, multiple ports, custom error pages, route-level method rules, redirects, directory indexes and listings, uploads, DELETE, request-body limits, chunked request decoding, and Python CGI.

The evaluation setup is intentionally visible: `configs_files/evaluation.conf` serves a browser console on port `8080` and a second site on port `8081`.

## Architecture

- `srcs/config`: configuration parsing, validation, inheritance, and location matching.
- `srcs/connection`: incremental request parsing, response generation, CGI, clients, and the epoll loop.
- `servers/evaluation`: static site, errors, autoindex fixtures, CGI scripts, and upload storage.
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
| `/redirect` | Configured 302 response |
| `/get-only/` | Route-level method rejection |

## Resources

- [RFC 1945: HTTP/1.0](https://datatracker.ietf.org/doc/html/rfc1945)
- [RFC 3875: CGI/1.1](https://datatracker.ietf.org/doc/html/rfc3875)
- [Linux epoll documentation](https://man7.org/linux/man-pages/man7/epoll.7.html)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- NGINX was used as a behavioral reference for status codes, headers, and route behavior.

AI was used to help audit the unfinished integration, design repeatable test cases and evaluation fixtures, identify protocol edge cases, and improve documentation. Every generated change must still be understood, reviewed, and defended by the project authors.
