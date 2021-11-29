#!/usr/bin/python2
from __future__ import print_function

from functools import reduce
import sys

from irods.configuration import IrodsConfig

def usage():
    print('Usage: get_irods_version.py ["string"|"integer"|"major"|"minor"|"patchlevel"]')

# check parameters
if len(sys.argv) != 2:
    usage()
    sys.exit(1)

operations = {}
operations['string'] = lambda x: x
operations['integer'] = lambda x: reduce(lambda y, z: y*1000 + int(z), x.split('.'), 0)
operations['major'] = lambda x: int(x.split('.')[0])
operations['minor'] = lambda x: int(x.split('.')[1])
operations['patchlevel'] = lambda x: int(x.split('.')[2])

# read version from VERSION.json
version_string = IrodsConfig().version['irods_version']
try :
    value = operations[sys.argv[1]](version_string)
except KeyError:
    print('ERROR: unknown format [%s] requested' % sys.argv[1], file=sys.stderr)
    usage()
    sys.exit(1)

print(value)
