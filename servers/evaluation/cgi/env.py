#!/usr/bin/env python3
import json
import os
import sys

body = sys.stdin.buffer.read().decode("utf-8", "replace")
with open("message.txt", "r", encoding="utf-8") as fixture:
    relative_file = fixture.read().strip()

payload = {
    "method": os.environ.get("REQUEST_METHOD", ""),
    "query": os.environ.get("QUERY_STRING", ""),
    "content_type": os.environ.get("CONTENT_TYPE", ""),
    "content_length": os.environ.get("CONTENT_LENGTH", ""),
    "body": body,
    "relative_file": relative_file,
    "script_name": os.environ.get("SCRIPT_NAME", ""),
    "server_protocol": os.environ.get("SERVER_PROTOCOL", ""),
    "x_test": os.environ.get("HTTP_X_TEST", "")
}
encoded = json.dumps(payload, indent=2)
sys.stdout.write("Content-Type: application/json; charset=utf-8\r\n\r\n")
sys.stdout.write(encoded)
