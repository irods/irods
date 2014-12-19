#!/usr/bin/env python
#
# Merges json file with a nested json object created from a list of comma
#  separated keys. New value inserted as a string.
#
# New keys will be created
# Existing keys will be updated


import collections
import json
import sys

if len(sys.argv) != 4:
    sys.exit('Usage: {1} filename <comma separated key list> new_value'.format(sys.argv[0]))

filename = sys.argv[1]
key_list = sys.argv[2].split(',')
the_value = sys.argv[3]


def make_nested_dict_from_key_list(keys, value):
    constructed = {keys[-1]: value}
    for k in keys[-2::-1]:
        constructed = {k: constructed}
    return constructed


def update_recursive(orig_dict, new_dict):
    for key, val in new_dict.items():
        if isinstance(val, collections.Mapping):
            tmp = update_recursive(orig_dict.get(key, {}), val)
            orig_dict[key] = tmp
        elif isinstance(val, list):
            orig_dict[key] = (orig_dict[key] + val)
        else:
            orig_dict[key] = new_dict[key]
    return orig_dict

with open(filename, 'r+') as f:
    data = json.load(f)
    update_recursive(data, make_nested_dict_from_key_list(key_list, the_value))
    f.seek(0)
    json.dump(data, f, indent=4, sort_keys=True)
    f.truncate()
