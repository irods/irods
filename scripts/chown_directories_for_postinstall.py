import os
import pwd
import grp
from irods import paths

if os.path.exists(paths.service_account_file_path()):
    service_account_uid = paths.irods_uid()
    service_account_gid = paths.irods_gid()
    is_top = True
    for dirpath, dirnames, _ in os.walk(paths.irods_directory()):
        if is_top:
            if 'msiExecCmd_bin' in dirnames:
                dirnames.remove('msiExecCmd_bin')
            is_top = False
        os.chown(dirpath, service_account_uid, service_account_gid)
