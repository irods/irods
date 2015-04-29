from __future__ import print_function

import json
import os
import subprocess
import sys
import time

DEBUG = True
DEBUG = False


def print_debug(*args, **kwargs):
    if DEBUG:
        print(*args, **kwargs)


def print_error(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)

def get_current_schema_version():
    # read from version file
    if os.path.isfile('/var/lib/irods/packaging/binary_installation.flag'):
        version_file = os.path.abspath('/var/lib/irods/VERSION.json')
    else:
        version_file = os.path.dirname(os.path.dirname(os.path.realpath(__file__))) + '/VERSION.json'
    with open(version_file) as fh:
        data = json.load(fh)
    current_schema_version = data['previous_version']['configuration_schema_version']
    print_debug('current_schema_version: {0}'.format(current_schema_version))
    return current_schema_version


def get_target_schema_version():
    # read from version file
    if os.path.isfile('/var/lib/irods/packaging/binary_installation.flag'):
        version_file = os.path.abspath('/var/lib/irods/VERSION.json')
    else:
        version_file = os.path.dirname(os.path.dirname(os.path.realpath(__file__))) + '/VERSION.json'
    with open(version_file) as fh:
        data = json.load(fh)
    target_schema_version = data['configuration_schema_version']
    print_debug('target_schema_version: {0}'.format(target_schema_version))
    return target_schema_version


def update_configuration_files(version):
    print('Updating to Configuration Schema... %d' % version)
    # manipulate the configuration files
    print_debug('PLACEHOLDER')
    # success
    print_debug('SUCCESS, updated configuration_schema_version to %d' % version)


def update_configuration_to_latest_version():
    # get current version
    current_schema_version = get_current_schema_version()
    # get target version
    target_schema_version = get_target_schema_version()
    # check if any work to be done
    if current_schema_version > target_schema_version:
        print_error('Configuration Schema Version is from the future (current=%d > target=%d).' % (
            current_schema_version, target_schema_version))
        return
    if current_schema_version == target_schema_version:
        print('Configuration Schema Version is already up to date (version=%d).' % target_schema_version)
        return
    # surgically alter existing version with any new information, with defaults
    update_configuration_files(target_schema_version)
    # done
    print('Done.')


def main():
    print_debug('-------------------- DEBUG IS ON --------------------')
    update_configuration_to_latest_version()
    print_debug('-------------------- DEBUG IS ON --------------------')

if __name__ == '__main__':
    main()
