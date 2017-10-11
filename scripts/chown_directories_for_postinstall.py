import os
import pwd
import grp
from irods import paths

if os.path.exists(paths.service_account_file_path()):
    service_account_uid = paths.irods_uid()
    service_account_gid = paths.irods_gid()
    for dirpath, _, _ in os.walk(paths.irods_directory()):
        os.chown(dirpath, service_account_uid, service_account_gid)
