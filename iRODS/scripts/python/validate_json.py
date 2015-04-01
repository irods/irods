import json
import sys

try:
    from jsonschema import validate
except:
    print 'VALID'
    sys.exit(0)

try:
    with open(sys.argv[1], 'r+') as f:
        cfg_file = json.load(f)
    with open(sys.argv[2], 'r+') as f:
        sch_file = json.load(f)
    validate( cfg_file, sch_file )
except KeyError:
    print 'INVALID'
    sys.exit(1)

else:
    print 'VALID'
    sys.exit(0)

