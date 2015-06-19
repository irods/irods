from __future__ import print_function
from irods_control import IrodsController
import sys

#
# This script finds iRODS processes owned by the current user,
# confirms their full_path against this installation of iRODS,
# checks their executing state, then pauses and kills each one.
#

if __name__ == '__main__':
    try:
        irods_controller = IrodsController()
        irods_controller.stop()
    except BaseException as e:
        print(e, file=sys.stderr)
        sys.exit(1)
