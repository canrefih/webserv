import sys

sys.stdout.write("Content-Type: text/plain\r\n")
sys.stdout.write("\r\n")
sys.stdout.write("X" * (128 * 1024))
sys.stdout.flush()