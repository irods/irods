from __future__ import print_function
import sys
import psutil
import time

pid = int(sys.argv[1])
p = psutil.Process(pid)
p.terminate()
p.kill()
print('done')
