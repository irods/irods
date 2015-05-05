from __future__ import print_function

import sys
import os
import json

def usage():
    print('Usage: get_irods_version.py ["string"|"integer"|"major"|"minor"|"patchlevel"]')

# check parameters
if len(sys.argv) != 2:
    usage()
    sys.exit(1)

# read version from VERSION.json
irods_top_level = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.realpath(__file__)))))
with open(os.path.join(irods_top_level, 'VERSION.json')) as fh:
    data = json.load(fh)
version_string = data['irods_version']
(major, minor, patchlevel) = version_string.split('.')

# print it out
if sys.argv[1] == 'string':
    print(version_string)
elif sys.argv[1] == 'integer':
    print(int(major)*1000000 + int(minor)*1000 + int(patchlevel))
elif sys.argv[1] == 'major':
    print(major)
elif sys.argv[1] == 'minor':
    print(minor)
elif sys.argv[1] == 'patchlevel':
    print(patchlevel)
else:
    print('ERROR: unknown format [%s] requested' % sys.argv[1], file=sys.stderr)
    usage()
    sys.exit(1)
