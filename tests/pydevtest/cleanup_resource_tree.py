#!/usr/bin/python

import subprocess
import sys
import json


def main(filename):
    # load tree from file
    with open(filename) as f:
        tree = json.load(f)

    # REMOVE PARENT-CHILD RELATIONSHIPS
    for name, (_, _, children) in tree.items():
        for child in children:
            # test
            print "iadmin rmchildfromresc {0} {1}".format(name, child)

            args = ['/usr/bin/iadmin', 'rmchildfromresc', name, child]
            # print args

            # run command
            subprocess.Popen(args).communicate()

    # REMOVE RESOURCES
    for name in tree:
        # test
        print "iadmin rmresc {0}".format(name)

        args = ['/usr/bin/iadmin', 'rmresc', name]

        # run command
        subprocess.Popen(args).communicate()

    # DONE
    return 0
# def main


# if run from command line
if __name__ == "__main__":
    if len(sys.argv) != 2:
        print 'Usage: cleanup_resource_tree.py <filename>'
        sys.exit(1)

    res = main(sys.argv[1])
    sys.exit(res)
