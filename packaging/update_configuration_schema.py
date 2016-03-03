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
    if os.path.isfile('/var/lib/irods/packaging/binary_installation.flag'):
        version_file = os.path.abspath('/etc/irods/configuration_version.json')
    else:
        version_file = os.path.dirname(os.path.dirname(os.path.realpath(__file__))) + '/iRODS/server/config/configuration_version.json'

    if(os.path.isfile(version_file)):
        with open(version_file) as fh:
            data = json.load(fh)
        current_schema_version = data['version']
    else:
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

def update_configuration_to_version_3():
    # read server config file
    if os.path.isfile('/var/lib/irods/packaging/binary_installation.flag'):
        server_config = os.path.abspath('/etc/irods/server_config.json')
        database_config = os.path.abspath('/etc/irods/database_config.json')
    else:
        server_config = os.path.dirname(os.path.dirname(os.path.realpath(__file__))) + '/iRODS/server/config/server_config.json'
        database_config = os.path.dirname(os.path.dirname(os.path.realpath(__file__))) + '/iRODS/server/config/database_config.json'

    with open(server_config) as fh:
        data = json.load(fh)

    role = "consumer"
    if(os.path.isfile(database_config)):
        role = "provider"

    data['catalog_service_role'] = role

    re_vals = """[
        {
            \"instance_name\": \"re-instance\",
            \"plugin_name\":\"re\",
            \"namespaces\": [
                {
                    \"namespace\": \"\"
                }, 
                {
                    \"namespace\":\"audit_\"
                }, 
                {
                    \"namespace\":\"indexing_\"
                }
            ]
        },
        {
            \"instance_name\":\"re-irods-instance\",
            \"plugin_name\": \"re-irods\"
        }
    ]"""

    data["re_plugins"] = json.loads(re_vals);

    with open(server_config, 'w') as fh:
        data = json.dump(data, fh, indent=4)

    return 3

def update_configuration_version(version):
    if os.path.isfile('/var/lib/irods/packaging/binary_installation.flag'):
        version_config = os.path.abspath('/etc/irods/configuration_version.json')
    else:
        version_config = os.path.dirname(os.path.dirname(os.path.realpath(__file__))) + '/iRODS/server/config/configuration_version.json'

    version_string = '{ "version": %d }' % (version)
    
    data = json.loads( version_string )

    with open(version_config, 'w') as fh:
        data = json.dump(data, fh, indent=4)

def update_configuration_files(current_schema_version,target_schema_version):
    # map of update logic allowing iteration
    update_stages = { 3: update_configuration_to_version_3 }

    start_stage = current_schema_version+1
    end_stage   = target_schema_version+1

    # iterate over update stages to bring schema up to date
    for stage in range(start_stage,end_stage):
        print('Updating Configuration Schema Version to %d' % (stage))

        ret = update_stages[stage]()
        if ret:
            update_configuration_version(stage)
        else:
            print('FAILED to update to Configuration Schema Version %d' % (stage))
            break

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
    update_configuration_files(current_schema_version,target_schema_version)
    # done
    print('Done.')


def main():
    print_debug('-------------------- DEBUG IS ON --------------------')
    update_configuration_to_latest_version()
    print_debug('-------------------- DEBUG IS ON --------------------')

if __name__ == '__main__':
    main()
