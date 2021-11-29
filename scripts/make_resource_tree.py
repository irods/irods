#!/usr/bin/python2
from __future__ import print_function

from irods.test.test_resource_tree import make_resource_tree

if len(sys.argv) != 3:
    print('Usage: make_resource_tree.py <width> <max depth>')
    sys.exit(1)

sys.exit(make_resource_tree(int(sys.argv[1]), int(sys.argv[2])))
