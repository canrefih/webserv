import os
import time
import sys

sys.stdout.flush()
os.close(1)
time.sleep(1)
sys.exit(42)