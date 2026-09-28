import os
import sys

print("Content-Type: text/plain")
print()

print("METHOD=" + os.environ.get("REQUEST_METHOD", ""))
print("SCRIPT_NAME=" + os.environ.get("SCRIPT_NAME", ""))
print("QUERY=" + os.environ.get("QUERY_STRING", ""))
print("CONTENT_LENGTH=" + os.environ.get("CONTENT_LENGTH", ""))
print("CONTENT_TYPE=" + os.environ.get("CONTENT_TYPE", ""))
print("BODY=" + sys.stdin.read())
print("HTTP_HOST=" + os.environ.get("HTTP_HOST", ""))