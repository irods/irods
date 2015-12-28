import os
import psutil

# This script finds iRODS processes owned by the current user,
# confirms their full_path against this installation of iRODS,
# checks their executing state, then pauses and kills each one.

def get_pids_executing_binary_file(binary_file_path):
    def get_exe(process):
        if psutil.version_info >= (2,0):
            return process.exe()
        return process.exe
    abspath = os.path.abspath(binary_file_path)
    pids = []
    for p in psutil.process_iter():
        try:
            if abspath == get_exe(p):
                pids.append(p.pid)
        except (psutil.NoSuchProcess, psutil.AccessDenied):
            pass
    return pids

def main():
    top_level_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
    for binary in ['irodsServer', 'irodsReServer', 'irodsXmsgServer', 'irodsAgent']:
        full_path = os.path.join(top_level_dir, 'iRODS/server/bin', binary)
        for pid in get_pids_executing_binary_file(full_path):
            try:
                p = psutil.Process(pid)
                p.suspend()
                p.terminate()
                p.kill()
            except (psutil.NoSuchProcess, psutil.AccessDenied):
                pass

if __name__ == '__main__':
    main()
