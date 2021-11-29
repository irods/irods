#!/usr/bin/python2
from __future__ import print_function
import sys
import psutil
import time

def get_create_time(process):
    if psutil.version_info >= (2,0):
        return process.create_time()
    return process.create_time

pid = int(sys.argv[1])
p = psutil.Process(pid)
print(time.time() - get_create_time(p))
