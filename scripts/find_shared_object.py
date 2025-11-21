#!/usr/bin/python3
import argparse
import itertools

import irods.lib

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("shared_objects", nargs="+", help="List of shared objects to find.")
    parser.add_argument('-r', '--regex', dest='regex', action='store_true', default=False,
            help='Enable regular expressions for search string')
    args = parser.parse_args()
    for so in args.shared_objects:
        so_paths = irods.lib.find_shared_object(so, regex=args.regex)
        print('\n\t'.join(itertools.chain([''.join([so, ':'])], so_paths)))

if __name__ == '__main__':
    main()
