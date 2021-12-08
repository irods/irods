#!/usr/bin/python3
from __future__ import print_function
import sys
import psutil
import time

import irods.lib

if(__name__ == '__main__'):
    irods.lib.kill_pid(int(sys.argv[1]))
    print('done')
