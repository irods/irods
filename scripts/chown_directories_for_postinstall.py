import logging
import grp
import os
import pwd

from irods import paths

if os.path.exists(paths.service_account_file_path()):
    l = logging.getLogger(__name__)
    service_account_uid = paths.irods_uid()
    service_account_gid = paths.irods_gid()
    is_top = True
    for dirpath, dirnames, _ in os.walk(paths.irods_directory()):
        if is_top:
            if 'msiExecCmd_bin' in dirnames:
                dirnames.remove('msiExecCmd_bin')
            is_top = False
        try:
            os.chown(dirpath, service_account_uid, service_account_gid, follow_symlinks=False)
        except Exception as e:
            l.warning("Failed to set ownership of %s: %s", dirpath, e)
