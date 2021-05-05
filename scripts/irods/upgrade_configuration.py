from __future__ import print_function

import datetime
import json
import logging
import os
import shutil
import stat

from . import six
from . import pyparsing
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
    return os.path.basename(path)[:-len(suffix)]

def requires_upgrade(irods_config):
    if not os.path.exists(paths.version_path()):
        return True
    with open('.'.join([paths.version_path(), 'dist']), 'r') as f:
        new_version = json.load(f)
    new_version_tuple = lib.version_string_to_tuple(new_version['irods_version'])
    old_version_tuple = lib.version_string_to_tuple(irods_config.version['irods_version'])
    return old_version_tuple != new_version_tuple

def upgrade(irods_config):
    l = logging.getLogger(__name__)
    if not os.path.exists('.'.join([paths.version_path(), 'previous'])) and os.path.exists(os.path.join(paths.irods_directory(), 'VERSION.previous')):
        convert_legacy_configuration_to_json(irods_config)

    with open('.'.join([paths.version_path(), 'dist']), 'r') as f:
        new_version = json.load(f)
    new_version['installation_time'] = datetime.datetime.now().isoformat()
    new_version_tuple = lib.version_string_to_tuple(new_version['irods_version'])
    if os.path.exists(paths.version_path()):
        old_version = irods_config.version
        old_version_tuple = lib.version_string_to_tuple(old_version['irods_version'])

    else:
        version_previous_path = '.'.join([paths.version_path(), 'previous'])
        if os.path.exists(version_previous_path):
            with open(version_previous_path, 'r') as f:
                old_version = json.load(f)
        else :
            old_version = {'schema_name': 'VERSION',
                    'schema_version': 'v2',
                    'irods_version': '4.1.0',
                    'commit_id': '0000000000000000000000000000000000000000',
                    'catalog_schema_version': 1,
                    'configuration_schema_version': 2}
        old_version_tuple = lib.version_string_to_tuple(old_version['irods_version'])
        if new_version_tuple == old_version_tuple:
            new_version['previous_version'] = old_version
            irods_config.commit(new_version, paths.version_path())

    if new_version_tuple == old_version_tuple:
        l.debug('Version numbers are the same, no upgrade performed.')
        return
    elif new_version_tuple < old_version_tuple:
        raise IrodsError("Downgrade detected. Downgrading is unsupported, please reinstall iRODS version %s or newer." % (old_version['irods_version']))
    new_version['previous_version'] = old_version
    l.debug('Upgrading from version %s to version %s.', old_version['irods_version'], new_version['irods_version'])

    if old_version_tuple < (4, 2, 0):
        if not os.path.exists(paths.service_account_file_path()):
            with open(paths.service_account_file_path()) as f:
                print('IRODS_SERVICE_ACCOUNT_NAME=%s' % (irods_config.irods_user), file=f)
                print('IRODS_SERVICE_GROUP_NAME=%s' % (irods_config.irods_group), file=f)

        old_dir_to_new_dir_map = {
                os.path.join(paths.irods_directory(), 'iRODS', 'server', 'bin', 'cmd'): os.path.join(paths.irods_directory(), 'msiExecCmd_bin'),
                os.path.join(paths.irods_directory(), 'iRODS', 'server', 'config', 'packedRei'): os.path.join(paths.irods_directory(), 'config', 'packedRei'),
                os.path.join(paths.irods_directory(), 'iRODS', 'server', 'config', 'lockFileDir'): os.path.join(paths.irods_directory(), 'config', 'lockFileDir')
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

    irods_config.clear_cache()

    upgrade_config_file(irods_config, irods_config.client_environment_path, new_version, schema_name='service_account_environment')

    irods_config.commit(new_version, paths.version_path(), make_backup=True)

    os.chmod(paths.genosauth_path(),
             stat.S_ISUID
             | stat.S_IRUSR
             | stat.S_IXUSR
             | stat.S_IRGRP
             | stat.S_IXGRP
             | stat.S_IROTH
             | stat.S_IXOTH)

def upgrade_config_file(irods_config, path, new_version, schema_name=None):
    l = logging.getLogger(__name__)
    with open(path, 'r') as f:
        config_dict = json.load(f)
    if schema_name is None:
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
            config_dict = run_schema_update(config_dict, schema_name, schema_version + 1)
            schema_version = schema_version_as_int(config_dict['schema_version'])
        irods_config.commit(config_dict, path, make_backup=True)


def run_schema_update(config_dict, schema_name, next_schema_version):
    l = logging.getLogger(__name__)
    l.debug('Upgrading %s to schema_version %d...', schema_name, next_schema_version)
    if next_schema_version == 3:
        config_dict['schema_name'] = schema_name
        if schema_name == 'server_config':
            if lib.hostname_resolves_to_local_address(config_dict['icat_host']):
                config_dict['catalog_service_role'] = 'provider'
            else:
                config_dict['catalog_service_role'] = 'consumer'
            config_dict['catalog_provider_hosts'] = [config_dict.pop('icat_host')]
            if 'federation' in config_dict:
                for f in config_dict['federation']:
                    f['catalog_provider_hosts'] = [f.pop('icat_host')]
            config_dict['rule_engine_namespaces'] = [
                    ''
                ]
            config_dict.setdefault('plugin_configuration', {})['rule_engines'] = [
                    {
                        'instance_name': 'irods_rule_engine_plugin-irods_rule_language-instance',
                        'plugin_name': 'irods_rule_engine_plugin-irods_rule_language',
                        'plugin_specific_configuration': dict(
                            [(k, [e['filename'] for e in config_dict.pop(k)]) for k in [
                                're_data_variable_mapping_set',
                                're_function_name_mapping_set',
                                're_rulebase_set']
                            ] + [
                                ('regexes_for_supported_peps', [
                                        'ac[^ ]*',
                                        'msi[^ ]*',
                                        '[^ ]*pep_[^ ]*_(pre|post|except|finally)'
                                    ]
                                )
                            ]
                        ),
                        'shared_memory_instance': 'upgraded_irods_rule_language_rule_engine'
                    }
                ]
            config_dict['plugin_configuration'].setdefault('authentication', {})
            config_dict['plugin_configuration'].setdefault('network', {})
            config_dict['plugin_configuration'].setdefault('resource', {})
            #put pam options in their plugin configuration
            for k, k_prime in [
                    ('pam_no_extend', 'no_extend'),
                    ('pam_password_length', 'password_length'),
                    ('pam_password_max_time', 'password_max_time'),
                    ('pam_password_min_time', 'password_min_time')]:
                if k in config_dict:
                    config_dict['plugin_configuration']['authentication'].setdefault('pam', {})[k_prime] = config_dict.pop(k)
            if 'kerberos_name' in config_dict:
                config_dict['plugin_configuration']['authentication']['krb']['name'] = config_dict.pop('kerberos_name')
            config_dict.setdefault('advanced_settings', {})['rule_engine_server_sleep_time_in_seconds'] = 30
            config_dict['advanced_settings']['rule_engine_server_execution_time_in_seconds'] = 120
            if config_dict['catalog_service_role'] == 'provider':
                with open(paths.database_config_path()) as f:
                    database_config = json.load(f)
                config_dict['plugin_configuration'].setdefault('database', {})[database_config.pop('catalog_database_type')] = database_config

    config_dict['schema_version'] = 'v%d' % (next_schema_version)
    return config_dict

def load_legacy_file(filepath, load_as='dict', token=' '):
    l = logging.getLogger(__name__)
    l.debug('Loading %s for conversion to json...', filepath)
    with open(filepath, 'rt') as f:
        if load_as == 'dict':
            container = {}
        elif load_as == 'list':
            container = []
        else:
            raise IrodsError('load_legacy_file called with unsupported load_as value: "%s".' % load_as)
        for l in f.readlines():
            if line and not line.startswith('#'):
                continue
            if load_as == 'dict':
                key, _, val = l.strip().partition(token)
                legacy_dict[key] = val
            if load_as == 'list':
                container.append(l.strip().split(token))
    return container


#convert a legacy configuration to a v2 json configuration
def convert_legacy_configuration_to_json(irods_config):
    l.debug('Attempting to convert a legacy configuration...')
    if os.path.exists('.'.join(paths.version_path(), 'previous')):
        l.debug('Not a legacy configuration. Skipping conversion...')
        return
    #load legacy files
    legacy_server_config = load_legacy_file(os.path.join(paths.config_directory(), 'server.config'))
    legacy_irods_config = load_legacy_file(os.path.join(paths.config_directory(), 'irods.config'))
    legacy_irods_environment = load_legacy_file(os.getenv('irodsEnvFile',
        os.path.join(paths.home_directory, '.irods', '.irodsEnv')))
    legacy_irods_host = load_legacy_file(os.path.join(paths.config_directory(), 'irodsHost'), load_as='list')
    legacy_version = load_legacy_file(os.path.join(paths.irods_directory(), 'VERSION.previous'), token='=')

    config_dicts = {}

    #Build server_config.json
    server_config_v2 = {
            'schema_name': 'server_config',
            'schema_version': 'v2',
            "advanced_settings": {
                "default_number_of_transfer_threads": 4,
                "default_temporary_password_lifetime_in_seconds": 120,
                "maximum_number_of_concurrent_rule_engine_server_processes": 4,
                "maximum_size_for_single_buffer_in_megabytes": 32,
                "maximum_temporary_password_lifetime_in_seconds": 1000,
                "transfer_buffer_size_for_parallel_transfer_in_megabytes": 4,
                "transfer_chunk_size_for_parallel_transfer_in_megabytes": 40
                },
            "default_dir_mode": legacy_server_config.get('default_dir_mode', "0750"),
            "default_file_mode": legacy_server_config.get('default_file_mode', "0600"),
            "default_hash_scheme": legacy_server_config.get('default_hash_scheme', "SHA256"),
            "environment_variables": {},
            "federation": [],
            "icat_host": legacy_server_config.get('icatHost', None),
            "match_hash_policy": legacy_server_config.get('match_hash_policy', "compatible"),
            "negotiation_key": legacy_server_config.get('agent_key', "TEMPORARY_32byte_negotiation_key"),
            "re_data_variable_mapping_set": [{"filename": f} for f in legacy_server_config.get('reVariableMapSet', "core").split()],
            "re_function_name_mapping_set": [{"filename": f} for f in legacy_server_config.get('reFuncMapSet', "core").split()],
            "re_rulebase_set": [{"filename": f} for f in legacy_server_config.get('reRuleSet', "core").split()],
            "schema_validation_base_uri": "file://{0}/configuration_schemas".format(paths.irods_directory()),
            "server_control_plane_encryption_algorithm": "AES-256-CBC",
            "server_control_plane_encryption_num_hash_rounds": 16,
            "server_control_plane_key": "TEMPORARY__32byte_ctrl_plane_key",
            "server_control_plane_port": 1248,
            "server_control_plane_timeout_milliseconds": 10000,
            "server_port_range_end": legacy_irods_config.get('$SVR_PORT_RANGE_END', 20199),
            "server_port_range_start": legacy_irods_config.get('$SVR_PORT_RANGE_START', 20000),
            "xmsg_port": 1279,
            "zone_auth_scheme": "native",
            "zone_key": legacy_server_config.get('LocalZoneSID', "TEMPORARY_zone_key"),
            "zone_name": legacy_irods_environment.get('irodsZone', 'tempZone'),
            "zone_port": int(legacy_irods_config.get('$IRODS_PORT', 1247)),
            "zone_user": legacy_irods_environment.get('irodsUserName', "rods")
        }

    #optional server config keys
    for old_key, new_key_and_type in {
            "KerberosName": ("kerberos_name", str),
            "pam_password_length": ("pam_password_length", int),
            "pam_no_extend": ("pam_no_extend", str),
            "pam_password_min_time": ("pam_password_min_time", int),
            "pam_password_max_time": ("pam_password_max_time", int)
            }.items():
        if old_key in legacy_server_config:
            new_key, new_type = new_key_and_type
            server_config_v2[new_key] = new_type(legacy_server_config[old_key])

    # server environment variables
    for old_key, new_key in {
            "spLogLevel": "sp_log_level",
            "spLogSql": "sp_log_sql",
            "svrPortRangeStart": "server_port_range_start",
            "svrPortRangeEnd": "server_port_range_end",
            "irodsEnvFile": "IRODS_ENVIRONMENT_FILE"
            }.items():
        if old_key in os.environ:
            server_config_v2['environment_variables'][new_key] = os.environ[old_key]

    config_dicts[paths.server_config_path()] = server_config_v2

    if hostname_resolves_to_local_address(server_config_v2['icat_host']):
        #Build database_config.json
        database_config_v2 = {
                'schema_name': 'database_config',
                'schema_version': 'v2',
                "catalog_database_type": legacy_server_config.get('catalog_database_type', "postgres"),
                "db_host": legacy_irods_config.get('$DATABASE_HOST', "localhost"),
                "db_name": legacy_irods_config.get('$DB_NAME', "ICAT"),
                "db_odbc_type": legacy_irods_config.get('$DATABASE_ODBC_TYPE', "unix"),
                "db_password": password_obfuscation.unscramble(legacy_server_config['DBPassword'], legacy_server_config['DBKey'], ''),
                "db_port": int(legacy_irods_config.get('$DATABASE_PORT', 5432)),
                "db_username": legacy_server_config.get('DBUsername', "irods")
            }

        config_dicts[paths.database_config_path()] = database_config_v2


    #Build irods_environment.json
    irods_environment_v2 = {
            'schema_name': 'service_account_environment',
            'schema_version': 'v2',
            'irods_host': legacy_irods_environment.get('irodsHost', irods.lib.get_hostname()),
            'irods_port': int(legacy_irods_environment.get('irodsPort', server_config_v2['zone_port'])),
            'irods_default_resource': legacy_irods_environment.get('irodsDefResource', server_config_v2['default_resource_name']),
            'irods_home': legacy_irods_environment.get('irodsHome', '/'.join(['', server_config_v2['zone_name'], 'home', server_config_v2['zone_user']])),
            'irods_cwd': legacy_irods_environment.get('irodsCwd', '/'.join(['', server_config_v2['zone_name'], 'home', server_config_v2['zone_user']])),
            'irods_user_name': legacy_irods_environment.get('irodsUserName', server_config_v2['zone_user']),
            'irods_zone_name': legacy_irods_environment.get('irodsZone', server_config_v2['zone_name']),
            'irods_client_server_negotiation': legacy_irods_environment.get('irodsClientServerNegotiation', 'request_server_negotiation'),
            'irods_client_server_policy': legacy_irods_environment.get('irodsClientServerPolicy', 'CS_NEG_REFUSE'),
            'irods_encryption_key_size': int(legacy_irods_environment.get('irodsEncryptionKeySize', 32)),
            'irods_encryption_salt_size': int(legacy_irods_environment.get('irodsEncryptionSaltSize', 8)),
            'irods_encryption_num_hash_rounds': int(legacy_irods_environment.get('irodsEncryptionNumHashRounds', 16)),
            'irods_encryption_algorithm': legacy_irods_environment.get('irodsEncryptionAlgorithm', 'AES-256-CBC'),
            'irods_default_hash_scheme': legacy_irods_environment.get('irodsDefaultHashScheme', server_config_v2['default_hash_scheme']),
            'irods_match_hash_policy': legacy_irods_environment.get('irodsMatchHashPolicy', server_config_v2['match_hash_policy']),
            'irods_server_control_plane_port': server_config_v2['server_control_plane_port'],
            'irods_server_control_plane_key': server_config_v2['server_control_plane_key'],
            'irods_server_control_plane_encryption_num_hash_rounds': server_config_v2['server_control_plane_encryption_num_hash_rounds'],
            'irods_server_control_plane_encryption_algorithm': server_config_v2['server_control_plane_encryption_algorithm'],
            'irods_maximum_size_for_single_buffer_in_megabytes': server_config_v2['advanced_settings']['maximum_size_for_single_buffer_in_megabytes'],
            'irods_default_number_of_transfer_threads': server_config_v2['advanced_settings']['default_number_of_transfer_threads'],
            'irods_transfer_buffer_size_for_parallel_transfer_in_megabytes': server_config_v2['advanced_settings']['transfer_buffer_size_for_parallel_transfer_in_megabytes'],
            "irods_ssl_verify_server": os.getenv("irodsSSLVerifyServer", 'hostname'),
            "irods_ssl_ca_certificate_file": os.getenv("irodsSSLCACertificateFile", ''),
            "irods_ssl_ca_certificate_path": os.getenv("irodsSSLCACertificatePath", '')
            }

    #optional irods environment keys
    for old_key, new_key_and_type in {
            "xmsgHost": ("irods_xmsg_host", str),
            "xmsgPort": ("irods_xmsg_port", int),
            "irodsAuthScheme": ("irods_authentication_scheme", str),
            "irodsServerDn": ("irods_gsi_server_dn", str),
            "irodsLogLevel": ("irods_log_level", str),
            "irodsAuthFileName": ("irods_authentication_file", str),
            "irodsDebug": ("irods_debug", str)
            }.items():
        if old_key in legacy_irods_environment:
            new_key, new_type = new_key_and_type
            irods_environment_v2[new_key] = new_type(legacy_irods_environment[old_key])

    #optional keys from the actual environment
    for old_key, new_key in {
            "irodsProt": "irods_use_xml_protocol",
            "clientUserName": "irods_client_user_name",
            "clientRodsZone": "irods_client_zone_name",
            }.items():
        if old_key in os.environ:
            server_config_v2['environment_variables'][new_key] = os.environ[old_key]

    config_dicts[server_config_v2['environment_variables'].get('IRODS_ENVIRONMENT_FILE', paths.default_client_environment_path())] = irods_environment_v2

    hosts_config_v2 = {
        'schema_name': 'hosts_config',
        'schema_version': 'v2',
        'host_entries': [
            {
                'address_type': 'local' if l[0] == 'localhost' else 'remote',
                'addresses': [{'address': l[i]} for i in range(0, len(l)) if i != 0 or l[i] != 'localhost']
            } for l in legacy_irods_host]
        }

    config_dicts[paths.hosts_config_path()] = hosts_config_v2

    host_access_control_config_v2 = {
        'schema_name': 'host_access_control_config',
        'schema_version': 'v2',
        'access_entries': [{
            'user': l[0],
            'group': l[1],
            'address': l[2],
            'mask': l[3]
            } for l in legacy_host_access_control if len(l) == 4 and l[0] != 'acChkHostAccessControl']
        }

    config_dicts[paths.host_access_control_config_path()] = host_access_control_config_v2

    #new version file
    previous_version = {
        'schema_name': 'VERSION',
        'schema_version': 'v2',
        'irods_version': legacy_version.get('IRODSVERSION', '4.0.0'),
        'catalog_schema_version': int(legacy_version.get('CATALOG_SCHEMA_VERSION', 1)),
        'configuration_schema_version': 0,
        'commit_id': '0000000000000000000000000000000000000000',
        'build_system_information': 'unavailable',
        'compiler_version': 'unavailable'
        }

    for path, config_dict in config_dict.items():
        irods_config.commit(config_dict, path, make_backup=True)

    irods_config.commit(previous_version, '.'.join(paths.version_path(), 'previous'))
