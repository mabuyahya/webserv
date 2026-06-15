# Evaluation Guide

## Quick Start

```sh
make
./webserv configs_files/evaluation.conf
```

Visit `http://127.0.0.1:8080/`, click **Run browser checks**, and use the upload workbench.

## Terminal Demonstration

```sh
curl --http1.0 -i http://127.0.0.1:8080/
curl -i http://127.0.0.1:8080/nested/info.json
curl -i http://127.0.0.1:8080/files/
curl -i http://127.0.0.1:8080/redirect
curl -i http://127.0.0.1:8080/cgi/env.py?name=evaluator
curl -i -d 'hello CGI' http://127.0.0.1:8080/cgi/env.py
curl -i --data-binary @README.md http://127.0.0.1:8080/uploads/readme.txt
curl -i -X DELETE http://127.0.0.1:8080/uploads/readme.txt
curl -i http://127.0.0.1:8081/
```

Every response status line is `HTTP/1.0`, including requests sent by a standard browser.

Run the automated checks:

```sh
python3 tests/http_test.py
python3 tests/config_test.py
```

For a single-command automated demonstration:

```sh
./tests/run_all.sh
```

## Code Walkthrough

1. Start at `main.cpp`: parse configuration, then construct and run `ServerManager`.
2. Show `ServerManager::run`: one `epoll_wait()` dispatches listener, client, and CGI events.
3. Show `HttpRequest::feedData`: requests are parsed incrementally and chunked bodies are decoded.
4. Show `HttpResponse::generate`: longest-prefix location rules select static, upload, DELETE, redirect, autoindex, error, or CGI behavior.
5. Show `CgiHandler::executeCgi`: CGI is the only use of `fork()`, with stdin/stdout connected to non-blocking pipes.

## Cleanup

The automated suite removes its upload fixtures. To reset manual uploads:

```sh
find servers/evaluation/uploads -type f ! -name .gitkeep -delete
```
