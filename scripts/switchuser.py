#!/usr/bin/python2
from __future__ import print_function

'''switchuser.py - A simple utility to switch irods user environment files, offsession (ie. after running iexit).
Assumes that such files are stored as irods_environment_username.json under ~/.irods
'''

import os
import sys
import shutil


if len(sys.argv) != 2:
    print('Usage: switchuser.py <username>')
    sys.exit(1)

irods_dir = os.path.join(os.environ['HOME'], '.irods')

# Do we have a file for this user?
if 'irods_environment_' + sys.argv[1] + ".json" in os.listdir(irods_dir):
    # Save previous environment file
    if 'irods_environment.json' in os.listdir(irods_dir):
        os.rename(os.path.join(irods_dir, 'irods_environment.json'),
                  os.path.join(irods_dir, 'irods_environment.json~'))

    # Copy user environment file to irods_environemnt.json
    shutil.copyfile(os.path.join(
        irods_dir, 'irods_environment_' + sys.argv[1] + ".json"), os.path.join(irods_dir, 'irods_environment.json'))

else:
    print('No irods_environment.json file found for user: %s' % (sys.argv[1]))
    sys.exit(1)
