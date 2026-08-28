#!/usr/bin/env python3
# Used only by tests/test_cgi.cpp to check CGIHandler::tryWait()'s
# WIFSIGNALED path: this script kills itself before writing anything,
# so the parent should see the pipe close (EOF) and report exitCode
# as -SIGTERM instead of a normal 0-255 exit status.
import os
import signal

os.kill(os.getpid(), signal.SIGTERM)
