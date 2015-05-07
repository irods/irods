import socket
import os

RUN_IN_TOPOLOGY = False
TOPOLOGY_FROM_RESOURCE_SERVER = False
HOSTNAME_1 = HOSTNAME_2 = HOSTNAME_3 = socket.gethostname()
USE_SSL = False
ICAT_HOSTNAME = socket.gethostname()
PREEXISTING_ADMIN_PASSWORD = 'rods'

class FEDERATION(object):
    LOCAL_IRODS_VERSION = (4,1,0)
    RODSUSER_NAME_PASSWORD_LIST = [('zonehopper', '53CR37')]
    RODSADMIN_NAME_PASSWORD_LIST = []
    IRODS_DIR = '/var/lib/irods/iRODS'
    LOCAL_ZONE = 'dev'
    REMOTE_ZONE = '403'
    REMOTE_RESOURCE = 'demoResc'
    REMOTE_VAULT = '/var/lib/irods/iRODS/Vault'
    TEST_FILE_SIZE = 4096   # 4MB
    LARGE_FILE_SIZE = 67108864  # 64MB
    TEST_FILE_COUNT = 300
    MAX_THREADS = 16
