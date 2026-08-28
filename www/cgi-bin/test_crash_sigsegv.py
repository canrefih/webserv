#!/usr/bin/env python3
# Used only by tests/test_cgi.cpp to check CGIHandler::tryWait()'s
# WIFSIGNALED path on a real crash (as opposed to a deliberate kill):
# dereferencing a NULL pointer raises SIGSEGV.
import ctypes

ctypes.string_at(0)
