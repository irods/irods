#!/usr/bin/python3
from __future__ import print_function

import inspect
import distro
import platform
import sys

# The following url lists identifying information from several systems
# https://github.com/hpcugent/easybuild/wiki/OS_flavor_name_version

# Invoke module functions directly from the command line by passing the
#  desired function name minus the leading 'get_' as the first argument.

def get_os_distribution_name():
    system = platform.system()
    if system == 'Linux' : return distro.id()
    if system == 'Darwin': return 'MacOSX'

def get_os_distribution_version():
    system = platform.system()
    if system == 'Linux' : return distro.version()
    if system == 'Darwin': return platform.mac_ver()[0]

if __name__ == '__main__':
    def print_error(*args, **kwargs):
        print(*args, file=sys.stderr, **kwargs)

    argument_function_map = dict((fname[4:], f)
                                 for fname, f in globals().items()
                                 if fname.startswith('get_') and inspect.isfunction(f))
    if len(sys.argv) != 2 or sys.argv[1] not in argument_function_map:
        print_error('Call as one of the following:')
        for arg in argument_function_map:
            print_error('  {0} {1}'.format(sys.argv[0], arg))
        print_error('Was called with sys.argv {0}'.format(sys.argv))
        sys.exit(1)

    print(argument_function_map[sys.argv[1]]())
