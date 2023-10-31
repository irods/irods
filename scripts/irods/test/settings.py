import socket
import os

RUN_IN_TOPOLOGY = False
TOPOLOGY_FROM_RESOURCE_SERVER = False
HOSTNAME_1 = HOSTNAME_2 = HOSTNAME_3 = socket.gethostname()
USE_SSL = False
USE_MUNGEFS = False
ICAT_HOSTNAME = socket.gethostname()
PREEXISTING_ADMIN_USERNAME = 'rods'
PREEXISTING_ADMIN_PASSWORD = 'rods'

class FEDERATION(object):
    REMOTE_IRODS_VERSION = (4, 2, 0)
    RODSUSER_NAME_PASSWORD_LIST = [('zonehopper', '53CR37')]
    RODSADMIN_NAME_PASSWORD_LIST = [('admin', PREEXISTING_ADMIN_PASSWORD)]
    IRODS_DIR = '/var/lib/irods'
    REMOTE_ZONE = 'buntest'
    REMOTE_HOST = 'buntest'
    REMOTE_VAULT = '/var/lib/irods/Vault'
    TEST_FILE_SIZE = 4*1024*1024
    LARGE_FILE_SIZE = 64*1024*1024
    TEST_FILE_COUNT = 300
    MAX_THREADS = 4

    # resource hierarchies
    REMOTE_PT_RESC_HIER = 'federation_remote_passthrough;federation_remote_unixfilesystem_leaf'
    LOCAL_PT_RESC_HIER = 'pt;leaf'
    REMOTE_DEF_RESOURCE = 'demoResc'
