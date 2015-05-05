import socket
import os

RUN_IN_TOPOLOGY = False
TOPOLOGY_FROM_RESOURCE_SERVER = False
HOSTNAME_1 = HOSTNAME_2 = HOSTNAME_3 = socket.gethostname()
USE_SSL = False
ICAT_HOSTNAME = socket.gethostname()
PREEXISTING_ADMIN_PASSWORD = 'rods'

# federation settings
LOCAL_IRODS_VERSION = (4,1,0)
CROSS_ZONE_USER_NAME = 'zonehopper'
CROSS_ZONE_USER_AUTH = '53CR37'
CROSS_ZONE_TEST_CONFIG_FILE = os.path.abspath('federation_test_parameters.json')
IRODS_DIR = '/var/lib/irods/iRODS'