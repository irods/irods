#!/usr/bin/python

import subprocess
import sys
import json
import random
import socket


EMPTY_CTXT_STR = "''"


def main(width, max_depth):

    if max_depth < 1 or width < 1:
        print 'Nothing to do...'
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
            resc_type = random.choice(['replication', 'random', 'roundrobin', 'passthru'])

            # determine if resource should be added to inodes (available as parent)
            if depth < max_depth:
                inodes.append(name)

        # add new resource to tree
        tree[name] = [resc_type, depth, []]

    # remove virtual root node from dictionary
    del(tree['root'])

    # print tree
    for name in tree.keys():
        print name, tree[name]

    # MAKE RESOURCES
    for name in tree.keys():
        # test
        print "iadmin mkresc {0} {1} {2}:/tmp/{0} {3}".format(name, tree[name][0], hostname, EMPTY_CTXT_STR)
        args = ['iadmin', 'mkresc', name, tree[name][0],
                "{0}:/tmp/{1}".format(hostname, name), EMPTY_CTXT_STR]
        # print args

        # run command
        subprocess.Popen(args, stdout=subprocess.PIPE).communicate()

    # ADD PARENT-CHILD RELATIONSHIPS
    for name in tree.keys():
        children = tree[name][2]
        if len(children):
            for child in children:
                # test
                print "iadmin addchildtoresc {0} {1}".format(name, child)
                args = ['iadmin', 'addchildtoresc', name, child]
                # print args

                # run command
                subprocess.Popen(args).communicate()

    # store in file for later teardown
    with open('resourcetree.json', 'w') as f:
        json.dump(tree, f)

    # DONE
    return 0
# def main


# if run from command line
if __name__ == "__main__":
    if len(sys.argv) != 3:
        print 'Usage: make_resource_tree.py <width> <max depth>'
        sys.exit(1)

    res = main(int(sys.argv[1]), int(sys.argv[2]))
    sys.exit(res)
