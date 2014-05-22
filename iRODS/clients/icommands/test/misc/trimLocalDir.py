'''trimLocalDir.py - Compares a local directory with an iRODS collection
    and deletes extraneous files from the local directory.
    Both inputs need to be absolute paths.
    For use during a client session (iinit) to cleanup a local directory after running irsync .
    AdT, 10/13/2011
'''

import os
import sys
import subprocess

# Location of the icommand binaries
ICMDS_DIR = '/Users/antoine/iRODS/clients/icommands/bin'


# Usage
if len(sys.argv) != 3:
    print 'Usage: trimLocalDir.py <directory> <collection>'
    sys.exit(1)


# Prompt for icommands location if not found
if not os.path.isfile(os.path.join(ICMDS_DIR, 'iquest')):
    ICMDS_DIR = raw_input('Icommands bin directory: ')


# Crawl our local directory
basedir, basecoll = sys.argv[1:3]
for dirpath, dirnames, filenames in os.walk(basedir):
    for filename in filenames:
        # Local file path
        filepath = os.path.join(dirpath, filename)

        # Build corresponding iRODS object path
        objpath = filepath.replace(basedir, basecoll, 1)

        # Make iquest query: Is this a valid object path?
        query = "select count(DATA_ID) where COLL_NAME = '%s' and DATA_NAME = '%s'" % \
            (objpath.rpartition('/')[0], filename)

        # Run that query through iquest
        proc = subprocess.Popen([os.path.join(ICMDS_DIR, 'iquest'), '%s', query],
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, err = proc.communicate()

        # Error?
        if len(err) > 0:
            print err
            sys.exit(1)

        # Delete local file if no corresponding iRODS object was found
        if int(out) == 0:
            print 'Removing ' + filepath
            os.unlink(filepath)


sys.exit(0)
