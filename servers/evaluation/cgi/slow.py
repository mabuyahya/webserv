#!/usr/bin/env python3
import time

time.sleep(1)
print("Content-Type: text/plain; charset=utf-8\r")
print("\r")
print("Slow CGI completed without blocking other clients.")
