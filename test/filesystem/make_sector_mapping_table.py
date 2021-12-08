#!/usr/bin/env python3
from __future__ import print_function
import argparse
import random


# container for valid float choices
class Interval(object):
    def __init__(self, start, end):
        self.start = start
        self.end = end
    def __contains__(self, x):
        return self.start <= x <= self.end
    def __iter__(self):
        return iter(['[{start}, {end}]'.format(**vars(self))])

# arg parsing
parser = argparse.ArgumentParser()
parser.add_argument('-d', '--device', default='/dev/loop0', help='virtual device path')
parser.add_argument('-s', '--sectors', default=1024*1024, type=int, help='total number of disk sectors on device')
parser.add_argument('-p', '--probability', metavar='PROBABILITY', default=.0001, type=float, help='probability of hitting a bad block',
                    choices=Interval(0.0, 1.0))
parser.add_argument('-b', '--bad_block_size', default=10, type=int, help='number of disk sectors per bad block')
args = parser.parse_args()

def bad_block():
    return random.random() < args.probability

# make the mapping
start = 0
good_block_size = 0

while start + good_block_size + args.bad_block_size <= args.sectors:
    if bad_block():
        # print current good block and reset size
        if good_block_size > 0:
            print("{start} {good_block_size} linear {args.device} {start}".format(**locals()))
            start += good_block_size
            good_block_size = 0

        # print a bad block
        print("{start} {args.bad_block_size} error".format(**locals()))
        start += args.bad_block_size

    else:
        # grow current good block
        good_block_size += 1

# any space left becomes a final good block
good_block_size = args.sectors - start
if good_block_size > 0:
    print("{start} {good_block_size} linear {args.device} {start}".format(**locals()))

