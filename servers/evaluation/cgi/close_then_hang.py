#!/usr/bin/env python3
import os
import sys
import time

sys.stdout.write("Content-Type: text/plain; charset=utf-8\r\n\r\nCGI output closed.\n")
sys.stdout.flush()
os.close(1)
time.sleep(10)
