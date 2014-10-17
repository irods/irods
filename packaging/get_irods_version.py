from __future__ import print_function

import os
import json

irods_top_level = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
with open(os.path.join(irods_top_level, 'VERSION.json')) as fh:
    data = json.load(fh)
print(data['irods_version'])
