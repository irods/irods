#!/usr/bin/env python
#
# Sorts existing json file for consistency

import json
import sys

if len(sys.argv) != 2:
    sys.exit('Usage: {0} filename'.format(sys.argv[0]))

filename = sys.argv[1]

with open(filename, 'r+') as f:
    data = json.load(f)
    f.seek(0)
    json.dump(data, f, indent=4, sort_keys=True)
    f.truncate()
