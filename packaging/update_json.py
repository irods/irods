#!/usr/bin/env python
#
# Updates a json file with passed key and value
#
# New keys will be created
# Existing keys will be updated
#
# update_json.py <file_name> <chain_of_keys> <new_value>

import sys
import json

def print_debug(msg):
    """ Print DEBUG messages """
    DEBUG = False
    if DEBUG == True:
        print("DEBUG: "+msg)

# enforce usage
if len(sys.argv) != 4:
    sys.exit('Usage: %s filename "key,key,key" newvalue' % sys.argv[0])

# parse arguments
file_name = sys.argv[1]
the_keys  = sys.argv[2]
the_value = sys.argv[3]

# take off leading and trailing slashes
key_chain=the_keys.strip(',').split(',')

def insert_value(_data, _keys, _value):
    """ Recursively add a value to a chain of keys """
    working = {}
    k = _keys.pop(0)
    if len(_keys) > 0:
        working[k] = insert_value(_data, _keys, _value)
    else:
        working[k] = _value
    return working

# open and update the specified file
with open(file_name, 'r+') as f:
    data = json.load(f)
    # print the before
    print_debug("before")
    print_debug(json.dumps(data, indent=4, sort_keys=True))
    # insert and merge
    constructed = insert_value(data, key_chain, the_value)
    merged = data.copy()       # copies the original
    merged.update(constructed) # merges into the original
    # write it out
    f.seek(0)
    f.write(json.dumps(merged, indent=4, sort_keys=True))
    f.truncate()

# show the aftermath
print_debug("after")
with open(file_name, 'r') as f:
    print_debug(f.read())

print_debug("done")

