from __future__ import print_function

import json
import random
import socket
import subprocess
import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest

from .. import lib
from . import session


class Test_ilsresc(session.make_sessions_mixin([('otherrods', 'pass')], []), unittest.TestCase):
    width = 300
    max_depth = 100
    filename = 'resourcetree.json'

    def setUp(self):
        super(Test_ilsresc, self).setUp()
        make_resource_tree(self.width, self.max_depth)

    def tearDown(self):
        cleanup_resource_tree(self.filename)
        super(Test_ilsresc, self).tearDown()

    def test_ilsresc_tree(self):
        self.admin_sessions[0].assert_icommand('ilsresc --tree', 'STDOUT_SINGLELINE', 'resc')

    def test_ilsresc_tree_with_ascii_output(self):
        self.admin_sessions[0].assert_icommand('ilsresc --tree --ascii', 'STDOUT_SINGLELINE', 'resc')


def cleanup_resource_tree(filename):
    # load tree from file
    tree = lib.open_and_load_json(filename)

    # REMOVE PARENT-CHILD RELATIONSHIPS
    for name, (_, _, children) in tree.items():
        for child in children:
            # test
            print("iadmin rmchildfromresc {0} {1}".format(name, child))

            args = ['iadmin', 'rmchildfromresc', name, child]
            # print(args)

            # run command
            subprocess.Popen(args).communicate()

    # REMOVE RESOURCES
    for name in tree:
        # test
        print("iadmin rmresc {0}".format(name))

        args = ['iadmin', 'rmresc', name]

        # run command
        subprocess.Popen(args).communicate()

    # DONE
    return 0


EMPTY_CTXT_STR = "''"
def make_resource_tree(width, max_depth):

    if max_depth < 1 or width < 1:
        print('Nothing to do...')
        return 0

    hostname = socket.gethostname()

    # BUILD TREE MODEL

    # create dictionary that will contain everything
    # tree = {'resc_name' : ['resc_type', depth, ['child_name', ...]], ...}
    #
    # tree[resc_name][0] for type
    # tree[resc_name][1] for depth
    # tree[resc_name][2] for [children]

    tree = {}

    # make virtual root node with depth of 0
    tree['root'] = ['', 0, []]

    # create list of available inodes for random parent selection
    inodes = ['root']

    # make nodes
    for index in range(0, width):
        # name resource
        name = 'resc{0}'.format(index)

        # determine parent:
        parent = random.choice(inodes)

        # add resource to parent's children
        tree[parent][2].append(name)

        # if parent is a passthru, remove it from inodes now that it has a child
        if tree[parent][0] == 'passthru':
            inodes.remove(parent)

        # determine depth:
        depth = tree[parent][1] + 1

        # determine if inode or leaf (with 25% and 75% probability)
        node_type = random.choice(['inode'] + ['leaf'] * 3)

        # IF LEAF
        if node_type == 'leaf':
            # determine storage resource type (80% probability of UFS, 10% nonblocking, 10% mockarchive)
            resc_type = random.choice(['unixfilesystem'] * 8 + ['nonblocking', 'mockarchive'])

        # IF INODE
        if node_type == 'inode':
            # determine coordinating resource type
            resc_type = random.choice(['replication', 'random', 'passthru'])

            # determine if resource should be added to inodes (available as parent)
            if depth < max_depth:
                inodes.append(name)

        # add new resource to tree
        tree[name] = [resc_type, depth, []]

    # remove virtual root node from dictionary
    del(tree['root'])

    # print(tree)
    for name in tree.keys():
        print(name, tree[name])

    # MAKE RESOURCES
    for name in tree.keys():
        # test
        print("iadmin mkresc {0} {1} {2}:/tmp/{0} {3}".format(name, tree[name][0], hostname, EMPTY_CTXT_STR))
        args = ['iadmin', 'mkresc', name, tree[name][0],
                "{0}:/tmp/{1}".format(hostname, name), EMPTY_CTXT_STR]
        # print(args)

        # run command
        subprocess.Popen(args, stdout=subprocess.PIPE).communicate()

    # ADD PARENT-CHILD RELATIONSHIPS
    for name in tree.keys():
        children = tree[name][2]
        if len(children):
            for child in children:
                # test
                print("iadmin addchildtoresc {0} {1}".format(name, child))
                args = ['iadmin', 'addchildtoresc', name, child]
                # print(args)

                # run command
                subprocess.Popen(args).communicate()

    # store in file for later teardown
    with open('resourcetree.json', 'wt') as f:
        json.dump(tree, f)

    # DONE
    return 0

