#!/usr/bin/python2
from __future__ import print_function

from irods.test.test_resource_tree import cleanup_resource_tree

if len(sys.argv) != 2:
    print('Usage: cleanup_resource_tree.py <filename>')
    sys.exit(1)

sys.exit(cleanup_resource_tree(sys.argv[1]))
