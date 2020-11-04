from __future__ import print_function

import json
import os
import sys


def update_schema_dict(schema_dict, target_directory, filename):
    schema_dict['id'] = 'file://{target_directory}/{filename}#'.format(**vars())

def main(schema_directory, target_directory):
    for filename in os.listdir(schema_directory):
        with open(os.path.join(schema_directory, filename)) as f:
            schema_dict = json.load(f)
        update_schema_dict(schema_dict, target_directory, filename)
        with open(os.path.join(schema_directory, filename), 'w') as f:
            json.dump(schema_dict, f, indent=4, sort_keys=True)

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print('Call as {0} <schema directory> <target directory>'.format(sys.argv[0]), file=sys.stderr)
        sys.exit(1)
    main(os.path.normpath(sys.argv[1]), os.path.normpath(sys.argv[2]))
