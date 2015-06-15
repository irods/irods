from __future__ import print_function
import subprocess
import os
import psutil

#
# This script finds iRODS processes owned by the current user,
# confirms their full_path against this installation of iRODS,
# checks their executing state, then pauses and kills each one.
#

def get_fuser_output(fullpath):
   p = subprocess.Popen(['fuser', fullpath], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
   stdout, _ = p.communicate()
   return stdout

def get_pids(fuser_output):
    # get fuser listing of pids
    pid_string = fuser_output.split(':')[-1].strip()
    if not pid_string:
        return []
    pid_strings = pid_string.split()
    # we only want pids in executing state
    def get_pid_if_executing(s):
        if not s[-1] == 'e':
            return None
        try:
            return int(s[:-1])
        except ValueError:
            return False
    pids = []
    for s in pid_strings:
        p = get_pid_if_executing(s)
        if p:
            pids.append(p)
    return pids

top_level_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
for binary in ["irodsServer", "irodsReServer", "irodsXmsgServer", "irodsAgent"]:
    full_path = os.path.join(top_level_dir, "iRODS/server/bin/"+binary)
    for pid in get_pids(get_fuser_output(full_path)):
        #print("killing %d %s" % (pid, full_path))
        p = psutil.Process(pid)
        p.suspend()
        p.terminate()
        p.kill()
