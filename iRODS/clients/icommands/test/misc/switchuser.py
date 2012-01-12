#!/usr/bin/python

'''switchuser.py - A simple utility to switch irods user environment files, offsession (ie. after running iexit).
Asssumes that such files are stored as .irodsEnv_username under ~/.irods
'''

import os
import sys
import shutil


if len(sys.argv) != 2: 
    print 'Usage: switchuser.py <username>' 
    sys.exit(1) 

irods_dir = os.path.join(os.environ['HOME'], '.irods')

# Do we have a file for this user?
if '.irodsEnv_' + sys.argv[1] in os.listdir(irods_dir):
    # Save previous environment file
    if '.irodsEnv' in os.listdir(irods_dir):
        os.rename(os.path.join(irods_dir, '.irodsEnv'), os.path.join(irods_dir, '.irodsEnv~'))

    # Copy user environment file to .irodsEnv
    shutil.copyfile(os.path.join(irods_dir, '.irodsEnv_' + sys.argv[1]), os.path.join(irods_dir, '.irodsEnv'))

else:
    print 'No .irodsEnv file found for user:', sys.argv[1]
    sys.exit(1)
