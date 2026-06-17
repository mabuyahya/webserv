#!/bin/sh
set -eu

cd "$(dirname "$0")/.."
make

python3 - <<'PY'
import socket

for port in (8080, 8081):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        sock.bind(("127.0.0.1", port))
    except OSError:
        raise SystemExit(
            "port %d is already in use; stop the existing server before running tests" % port
        )
    finally:
        sock.close()
PY

LOG="/tmp/webserv-test-server.log"
./webserv configs_files/evaluation.conf >"$LOG" 2>&1 &
SERVER_PID=$!
export SERVER_PID

cleanup() {
    kill -TERM "$SERVER_PID" 2>/dev/null || true
    wait "$SERVER_PID" 2>/dev/null || true
    find servers/evaluation/uploads -type f ! -name .gitkeep -delete
}
trap cleanup EXIT INT TERM

python3 - <<'PY'
import socket
import time
import os

pid = int(os.environ["SERVER_PID"])
for _ in range(50):
    try:
        os.kill(pid, 0)
    except OSError:
        raise SystemExit("webserv exited early; see /tmp/webserv-test-server.log")
    try:
        socket.create_connection(("127.0.0.1", 8080), 0.1).close()
        break
    except OSError:
        time.sleep(0.05)
else:
    raise SystemExit("webserv did not start; see /tmp/webserv-test-server.log")
PY

python3 tests/http_test.py
kill -TERM "$SERVER_PID"
wait "$SERVER_PID" 2>/dev/null || true
trap - EXIT INT TERM
find servers/evaluation/uploads -type f ! -name .gitkeep -delete

python3 tests/config_test.py
