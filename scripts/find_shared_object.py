#!/usr/bin/python2
from __future__ import print_function
import itertools
import optparse

import irods.lib

def main():
    parser = optparse.OptionParser()
    parser.add_option('-r', '--regex', dest='regex', action='store_true', default=False,
            help='Enable regular expressions for search string')
    options, args = parser.parse_args()
    for so in args:
        so_paths = irods.lib.find_shared_object(so, regex=options.regex)
        print('\n\t'.join(itertools.chain([''.join([so, ':'])], so_paths)))

if __name__ == '__main__':
    main()
