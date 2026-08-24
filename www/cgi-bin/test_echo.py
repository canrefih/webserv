#!/usr/bin/env python3
# Minimal CGI script used only by tests/test_cgi.cpp to check that
# CGIHandler/buildCGIEnv pass the right env vars and body to the child.
import os
import sys

try:
    length = int(os.environ.get("CONTENT_LENGTH", "0") or "0")
except ValueError:
    length = 0

body = sys.stdin.read(length) if length > 0 else ""

print("Content-Type: text/plain")
print()
print("METHOD=" + os.environ.get("REQUEST_METHOD", ""))
print("SCRIPT_NAME=" + os.environ.get("SCRIPT_NAME", ""))
print("QUERY=" + os.environ.get("QUERY_STRING", ""))
print("BODY=" + body)
for key in sorted(os.environ):
    if key.startswith("HTTP_"):
        print(key + "=" + os.environ[key])
