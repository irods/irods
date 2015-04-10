import sys
import psutil
import time
pid = int(sys.argv[1])
p = psutil.Process(pid)
p.terminate()
p.kill()
print "done"

