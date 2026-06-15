#!/bin/sh
set -eu

cd "$(dirname "$0")/.."
make

LOG="/tmp/webserv-test-server.log"
./webserv configs_files/evaluation.conf >"$LOG" 2>&1 &
SERVER_PID=$!

cleanup() {
    kill -TERM "$SERVER_PID" 2>/dev/null || true
    wait "$SERVER_PID" 2>/dev/null || true
    find servers/evaluation/uploads -type f ! -name .gitkeep -delete
}
trap cleanup EXIT INT TERM

python3 - <<'PY'
import socket
import time

for _ in range(50):
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
wait "$SERVER_PID"
trap - EXIT INT TERM
find servers/evaluation/uploads -type f ! -name .gitkeep -delete

python3 tests/config_test.py
