from __future__ import print_function

import json
import logging
import os
import shutil

from . import six
from . import lib
from . import paths
from .exceptions import IrodsError, IrodsWarning

def schema_version_as_int(schema_version):
    if isinstance(schema_version, six.string_types) and schema_version[0] == 'v':
        schema_version = schema_version[1:]
    return int(schema_version)

def schema_name_from_path(path):
    suffix = '.json'
    if not path.endswith(suffix):
        raise IrodsError('Configuration file %s must end with "%s"' % (path, suffix))
    return os.path.basename(path)[:len(suffix)]

def requires_upgrade(irods_config):
    l = logging.getLogger(__name__)
    with open('.'.join([irods_config.version_path, 'dist']), 'r') as f:
        new_version = json.load(f)
    new_version_tuple = lib.version_string_to_tuple(new_version['irods_version'])
    old_version_tuple = lib.version_string_to_tuple(irods_config.version['irods_version'])
    if new_version_tuple < old_version_tuple:
        raise IrodsError("Downgrade detected. Downgrading is unsupported, please reinstall iRODS version %s or newer." % (irods_config.version['irods_version']))
    return old_version_tuple < new_version_tuple

def upgrade(irods_config):
    l = logging.getLogger(__name__)
    with open('.'.join([irods_config.version_path, 'dist']), 'r') as f:
        new_version = json.load(f)
    if new_version['irods_version'] == irods_config.version['irods_version']:
        return
    new_version['previous_version'] = irods_config.version
    l.debug('Upgrading from version %s to version %s.', new_version['previous_version']['irods_version'], new_version['irods_version'])
    previous_version_tuple = tuple(map(int, new_version['previous_version']['irods_version'].split('.')))
    if previous_version_tuple < (4, 2, 0):
        old_dir_to_new_dir_map = {
                os.path.join(paths.irods_directory(), 'iRODS', 'server', 'bin', 'cmd'): os.path.join(paths.irods_directory(), 'msiExecCmd_bin'),
                os.path.join(paths.irods_directory(), 'iRODS', 'server', 'config', 'packedRei'): os.path.join(paths.irods_directory(), 'server', 'config', 'packedRei'),
                os.path.join(paths.irods_directory(), 'iRODS', 'server', 'config', 'lockFileDir'): os.path.join(paths.irods_directory(), 'server', 'config', 'lockFileDir'),
            }
        for old_dir, new_dir in old_dir_to_new_dir_map.items():
            if os.path.isdir(old_dir):
                for entry in os.listdir(old_dir):
                    old_path = os.path.join(old_dir, entry)
                    new_path = os.path.join(new_dir, entry)
                    if os.path.exists(new_path):
                        raise IrodsError('File conflict encountered during upgrade: %s could not be moved to %s, as %s already exists. '
                                'Please resolve this naming conflict manually and try again.' % (old_path, new_path, new_path))
                    shutil.move(old_path, new_path)

    configuration_file_list = [
            paths.server_config_path(),
            paths.hosts_config_path(),
            paths.host_access_control_config_path()]

    for path in configuration_file_list:
        upgrade_config_file(irods_config, path, new_version)

    if irods_config.is_catalog:
        upgrade_config_file(irods_config, paths.database_config_path(), new_version)

    upgrade_config_file(irods_config, irods_config.client_environment_path, new_version)

    irods_config.commit(new_version, path, make_backup=True)

def upgrade_config_file(irods_config, path, new_version):
    l = logging.getLogger(__name__)
    with open(path, 'r') as f:
        config_dict = json.load(f)
    schema_name = config_dict.get('schema_name', schema_name_from_path(path))
    if 'schema_version' in config_dict:
        schema_version = schema_version_as_int(config_dict['schema_version'])
    else:
        schema_version = schema_version_as_int(new_version['previous_version']['configuration_schema_version'])
    target_schema_version = schema_version_as_int(new_version['configuration_schema_version'])
    if schema_version > target_schema_version:
        raise IrodsError('Schema version (%d) in %s exceeds the schema version (%d) specified by the new version file %s.'
                'Downgrade of configuration schemas is unsupported.' %
                (schema_version, path, target_schema_version, '.'.join([irods_config.version_path, 'dist'])))
    if schema_version < target_schema_version:
        while schema_version < target_schema_version:
            config_dict = run_schema_update(irods_config, config_dict, schema_name, schema_version + 1)
            schema_version = schema_version_as_int(config_dict['schema_version'])
        irods_config.commit(config_dict, path, make_backup=True)


def run_schema_update(config_dict, schema_name, next_schema_version):
    l.debug('Upgrading %s to schema_version %d...')
    if next_schema_version == 3:
        config_dict['schema_name'] = schema_name
        if schema_name == 'server_config':
            ret, _, _ = lib.execute_command_permissive(os.path.join(
                paths.server_bin_directory, 'hostname_resolves_to_local_address'))
            if ret == 0:
                config_dict['catalog_service_role'] = 'provider'
            elif ret == 1:
                config_dict['catalog_service_role'] = 'consumer'
            else:
                raise IrodsError('Invalid hostname in "icat_host" field in %s.' %
                        (schema_name))
            config_dict['re_plugins'] = [
                    {
                        'instance_name': 're-instance',
                        'plugin_name':'re',
                        'namespaces': [
                            { 'namespace': '' },
                            { 'namespace':'audit_' },
                            { 'namespace':'indexing_' }
                            ]
                    },
                    {
                        'instance_name':'re-irods-instance',
                        'plugin_name': 're-irods'
                    }
                ]


    config_dict['schema_version'] = 'v%d' % (next_schema_version)
    return config_dict
