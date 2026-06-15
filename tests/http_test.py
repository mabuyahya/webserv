#!/usr/bin/env python3
"""Socket-level regression tests for configs_files/evaluation.conf."""

import concurrent.futures
import socket
import sys
import time

HOST = "127.0.0.1"
PORT = 8080
HTTP_VERSION = "HTTP/1.0"
passed = 0
failed = 0


def raw_exchange(payload, port=PORT, fragments=None, timeout=5):
    sock = socket.create_connection((HOST, port), timeout)
    sock.settimeout(timeout)
    if fragments:
        for fragment in fragments:
            sock.sendall(fragment)
            time.sleep(0.01)
    else:
        sock.sendall(payload)
    response = b""
    while True:
        chunk = sock.recv(65536)
        if not chunk:
            break
        response += chunk
    sock.close()
    return response


def raw_request(payload, port=PORT, fragments=None, timeout=5):
    response = raw_exchange(payload, port=port, fragments=fragments, timeout=timeout)
    head, _, body = response.partition(b"\r\n\r\n")
    status = int(head.split(b" ", 2)[1])
    headers = {}
    for line in head.split(b"\r\n")[1:]:
        name, _, value = line.partition(b":")
        headers[name.decode().lower()] = value.strip().decode()
    return status, headers, body


def request(method, path, body=b"", headers=None, port=PORT):
    headers = dict(headers or {})
    headers.setdefault("Host", "localhost")
    headers.setdefault("Connection", "close")
    if body:
        headers.setdefault("Content-Length", str(len(body)))
    lines = [f"{method} {path} {HTTP_VERSION}"]
    lines.extend(f"{name}: {value}" for name, value in headers.items())
    payload = ("\r\n".join(lines) + "\r\n\r\n").encode() + body
    return raw_request(payload, port=port)


def check(name, condition, detail=""):
    global passed, failed
    if condition:
        passed += 1
        print(f"\033[32mPASS\033[0m {name}")
    else:
        failed += 1
        print(f"\033[31mFAIL\033[0m {name}: {detail}")


def expect(name, method, path, expected, contains=None, **kwargs):
    status, headers, body = request(method, path, **kwargs)
    valid = status == expected and (contains is None or contains in body)
    check(name, valid, f"status={status}, body={body[:100]!r}")
    return status, headers, body


def main():
    response = raw_exchange(b"GET / HTTP/1.0\r\nConnection: close\r\n\r\n")
    check("HTTP/1.0 request and response", response.startswith(b"HTTP/1.0 200"), response[:100])
    response = raw_exchange(b"GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n")
    check("browser-style request receives HTTP/1.0", response.startswith(b"HTTP/1.0 200"), response[:100])

    expect("static index", "GET", "/", 200, b"evaluation console")
    _, headers, _ = expect("nested static JSON", "GET", "/nested/info.json", 200, b'"served"')
    check("JSON MIME type", headers.get("content-type", "").startswith("application/json"), str(headers))
    expect("percent-decoded static path", "GET", "/nested/space%20file.txt", 200, b"percent-decoding")
    expect("custom 404", "GET", "/missing", 404, b"custom missing")
    expect("GET-only method rejection", "POST", "/get-only/", 405, b"custom method")
    expect("configured static POST accepted", "POST", "/static-post/", 200, b"evaluation console",
           body=b"accepted body")
    _, headers, _ = expect("configured redirect", "GET", "/redirect", 302)
    check("redirect Location", headers.get("location") == "/", str(headers))
    expect("autoindex", "GET", "/files/", 200, b"alpha.txt")
    expect("nested autoindex", "GET", "/files/nested/", 200, b"charlie.txt")
    expect("autoindex disabled", "GET", "/private/", 403, b"custom forbidden")
    expect("second listening port", "GET", "/", 200, b"Port 8081", port=8081)

    expect("CGI GET query and HTTP header", "GET", "/cgi/env.py?name=peer", 200, b'"x_test": "available"',
           headers={"X-Test": "available"})
    expect("CGI receives HTTP/1.0 protocol", "GET", "/cgi/env.py", 200,
           b'"server_protocol": "HTTP/1.0"')
    expect("CGI directory index executes", "GET", "/cgi/", 200, b'"method": "GET"')
    expect("CGI POST body", "POST", "/cgi/env.py?mode=post", 200, b'"body": "hello cgi"',
           body=b"hello cgi", headers={"Content-Type": "text/plain"})
    chunked_cgi = (
        b"POST /cgi/env.py?mode=chunked HTTP/1.0\r\nHost: localhost\r\n"
        b"Transfer-Encoding: chunked\r\nConnection: close\r\n\r\n"
        b"5\r\nhello\r\n6\r\n world\r\n0\r\n\r\n"
    )
    status, _, body = raw_request(chunked_cgi)
    check("chunked CGI body is un-chunked", status == 200 and b'"body": "hello world"' in body,
          f"status={status}, body={body[:100]!r}")
    expect("CGI working directory", "GET", "/cgi/env.py", 200, b"script directory")
    expect("CGI-selected status", "GET", "/cgi/status.py", 201, b"CGI-selected")
    expect("malformed CGI output", "GET", "/cgi/broken.py", 502, b"502 Bad Gateway")
    start = time.time()
    expect("CGI closing output cannot linger", "GET", "/cgi/close_then_hang.py", 200, b"output closed")
    check("closed-output CGI returns promptly", time.time() - start < 2)

    expect("raw upload", "POST", "/uploads/regression.txt", 201, b'"bytes":17',
           body=b"regression upload")
    expect("uploaded file GET", "GET", "/uploads/regression.txt", 200, b"regression upload")
    expect("uploaded file DELETE", "DELETE", "/uploads/regression.txt", 204)
    expect("deleted file is gone", "GET", "/uploads/regression.txt", 404)

    chunked = (
        b"POST /uploads/chunked.txt HTTP/1.0\r\nHost: localhost\r\n"
        b"Transfer-Encoding: chunked\r\nConnection: close\r\n\r\n"
        b"5\r\nhello\r\n6\r\n world\r\n0\r\n\r\n"
    )
    status, _, _ = raw_request(chunked)
    check("chunked upload", status == 201, f"status={status}")
    expect("chunked body decoded", "GET", "/uploads/chunked.txt", 200, b"hello world")
    expect("chunked cleanup", "DELETE", "/uploads/chunked.txt", 204)

    malformed = b"GET / HTTP/1.0\r\nBroken header\r\n\r\n"
    check("malformed header", raw_request(malformed)[0] == 400)
    no_host = b"GET / HTTP/1.0\r\nConnection: close\r\n\r\n"
    check("HTTP/1.0 Host is optional", raw_request(no_host)[0] == 200)
    expect("unsupported method", "PATCH", "/", 405)
    expect("path traversal rejected", "GET", "/../Makefile", 400)
    expect("encoded traversal rejected", "GET", "/%2e%2e/Makefile", 400)
    expect("invalid percent encoding", "GET", "/%Q0", 400)
    expect("oversized URI", "GET", "/" + ("a" * 9000), 414)
    expect("oversized Content-Length", "POST", "/uploads/large.bin", 413,
           headers={"Content-Length": "1048577"})

    huge_chunk = (
        b"POST /uploads/huge.bin HTTP/1.0\r\nHost: localhost\r\n"
        b"Transfer-Encoding: chunked\r\nConnection: close\r\n\r\n"
        b"FFFFFFFFFFFFFFFF\r\n"
    )
    check("oversized chunk rejected", raw_request(huge_chunk)[0] == 413)

    fragmented = [
        b"GET /nested/info.json HTTP/1.0\r\nHo",
        b"st: localhost\r\nConnection: close\r\n\r\n",
    ]
    status, _, body = raw_request(b"", fragments=fragmented)
    check("fragmented request", status == 200 and b'"served"' in body, f"status={status}")

    incomplete = b"POST /uploads/incomplete.txt HTTP/1.0\r\nHost: localhost\r\nContent-Length: 20\r\n\r\nshort"
    sock = socket.create_connection((HOST, PORT), 2)
    sock.settimeout(3)
    sock.sendall(incomplete)
    sock.shutdown(socket.SHUT_WR)
    response = b""
    while True:
        part = sock.recv(4096)
        if not part:
            break
        response += part
    sock.close()
    check("incomplete body gets 400", response.startswith(b"HTTP/1.0 400"), response[:100].decode(errors="replace"))

    start = time.time()
    with concurrent.futures.ThreadPoolExecutor(max_workers=24) as pool:
        results = list(pool.map(lambda _: request("GET", "/nested/info.json")[0], range(120)))
    elapsed = time.time() - start
    check("120 concurrent requests", results == [200] * 120, f"statuses={set(results)}")
    print(f"     concurrency elapsed: {elapsed:.3f}s")

    with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
        slow = pool.submit(request, "GET", "/cgi/slow.py")
        time.sleep(0.1)
        static_start = time.time()
        static = pool.submit(request, "GET", "/nested/info.json")
        static_result = static.result()
        static_elapsed = time.time() - static_start
        slow_result = slow.result()
    check("slow CGI completes", slow_result[0] == 200 and b"completed" in slow_result[2])
    check("slow CGI does not block static response", static_result[0] == 200 and static_elapsed < 0.8,
          f"static elapsed={static_elapsed:.3f}s")

    print(f"\n{passed} passed, {failed} failed")
    return 1 if failed else 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (ConnectionError, OSError, ValueError) as error:
        print(f"\033[31mTEST HARNESS ERROR\033[0m {error}")
        sys.exit(2)
