#!/usr/bin/python3
from __future__ import print_function

import sys

from irods.controller import IrodsController

#
# This script finds iRODS processes owned by the current user,
# confirms their full_path against this installation of iRODS,
# checks their executing state, then pauses and kills each one.
#

if __name__ == '__main__':
    try:
        control = IrodsController()
        control.stop()
    except BaseException as e:
        print(e, file=sys.stderr)
        sys.exit(1)
