#!/usr/bin/python3
from __future__ import print_function

import os, sys, optparse
import irods.setup_options

def parse_options():
    parser = optparse.OptionParser()
    irods.setup_options.add_options(parser)

    return parser.parse_args()

def get_ld_library_path_list():

    ld_library_path_list = []

    (options, _) = parse_options()
    if options.ld_library_path:
        ld_library_path_list = [p for p in options.ld_library_path.split(':') if p]

    return ld_library_path_list

#wrapper to set up ld_library_path
def wrap_if_necessary():

    ld_library_path_list = get_ld_library_path_list()

    #for oracle, ORACLE_HOME must be in LD_LIBRARY_PATH
    if 'ORACLE_HOME' in os.environ:
        oracle_lib_dir = os.path.join(os.environ['ORACLE_HOME'], 'lib')
        if oracle_lib_dir not in ld_library_path_list:
            ld_library_path_list.append(oracle_lib_dir)

    current_ld_library_path_list = [p for p in os.environ.get('LD_LIBRARY_PATH', '').split(':') if p]
    if ld_library_path_list != current_ld_library_path_list[0:len(ld_library_path_list)]:
        os.environ['LD_LIBRARY_PATH'] = ':'.join(ld_library_path_list + current_ld_library_path_list)
        argv = [sys.executable] + sys.argv
        os.execve(argv[0], argv, os.environ)

wrap_if_necessary()

import contextlib
import datetime
import grp
import itertools
import json
import logging
import pprint
import pwd
import re
import shutil
import stat
import time
import tempfile

from irods import six

import irods.lib
from irods.configuration import IrodsConfig
from irods.controller import IrodsController
from irods.exceptions import IrodsError, IrodsWarning
import irods.log
from irods.password_obfuscation import maximum_password_length

try:
    import pyodbc
except:
    print('WARNING: pyodbc module could not be imported.', file=sys.stderr)
    print('The pyodbc module is required for setup of catalog service providers.', file=sys.stderr)

def setup_server(irods_config, json_configuration_file=None, test_mode=False):
    l = logging.getLogger(__name__)

    check_hostname()

    if json_configuration_file is not None:
        with open(json_configuration_file) as f:
            json_configuration_dict = json.load(f)
    else:
        json_configuration_dict = None

    if IrodsController(irods_config).get_server_proc():
        l.info(irods.lib.get_header('Stopping iRODS...'))
        IrodsController(irods_config).stop()

    if not os.path.exists(irods_config.version_path):
        with open('.'.join([irods_config.version_path, 'dist'])) as f:
            version_json = json.load(f)
        version_json['installation_time'] = datetime.datetime.now().isoformat()
        irods_config.commit(version_json, irods.paths.version_path())

    if json_configuration_dict is not None:
        irods_user = json_configuration_dict['host_system_information']['service_account_user_name']
        irods_group = json_configuration_dict['host_system_information']['service_account_group_name']
    else:
        irods_user, irods_group = get_irods_user_and_group(irods_config)

    setup_service_account(irods_config, irods_user, irods_group)

    # Do the rest of the setup as the irods user
    if os.getuid() == 0:
        irods.lib.switch_user(irods_config.irods_user, irods_config.irods_group)

    if json_configuration_dict is not None:
        # role
        irods_config.commit(json_configuration_dict['server_config'], irods.paths.server_config_path())
        # default resource
        default_resource_name = json_configuration_dict['default_resource_name']
        default_resource_directory = json_configuration_dict.get('default_resource_directory', os.path.join(irods_config.irods_directory, 'Vault'))
        # client environment
        if not os.path.exists(os.path.dirname(irods_config.client_environment_path)):
            os.makedirs(os.path.dirname(irods_config.client_environment_path), mode=0o700)
        irods_config.commit(json_configuration_dict['service_account_environment'], irods_config.client_environment_path)
        # password
        irods_config.admin_password = json_configuration_dict['admin_password']
    else:
        # role
        determine_server_role(irods_config)
        # database
        if irods_config.is_provider:
            from irods import database_interface
            l.info(irods.lib.get_header('Configuring the database communications'))
            database_interface.setup_database_config(irods_config)
        # default resource
        default_resource_name, default_resource_directory = setup_storage(irods_config)
        # server config
        setup_server_config(irods_config)
        # client environment and password
        setup_client_environment(irods_config)

    if irods_config.is_provider:
        from irods import database_interface
        l.info(irods.lib.get_header('Setting up the database'))
        database_interface.setup_catalog(irods_config, default_resource_directory=default_resource_directory, default_resource_name=default_resource_name)

    l.info(irods.lib.get_header('Starting iRODS...'))
    IrodsController(irods_config).start(test_mode=test_mode)

    # create local storage resource for consumer (provider was configured directly in database_interface.setup_catalog above)
    if irods_config.is_consumer:
        irods.lib.execute_command(['iadmin', 'mkresc', default_resource_name, 'unixfilesystem', ':'.join([irods.lib.get_hostname(), default_resource_directory]), ''])

    # update core.re with default resource
    core_re_path = os.path.join(irods_config.core_re_directory, 'core.re')
    replace_in_file(core_re_path, 'demoResc', default_resource_name)

    # test put
    test_put(irods_config)

    # extract the "irods_version" property from the version.json.dist file.
    # this guarantees that setup always uses the correct version information.
    with open('.'.join([irods_config.version_path, 'dist'])) as f:
        irods_version_string = json.load(f)['irods_version']

    l.info(irods.lib.get_header('Log Configuration Notes'))
    l.info(('iRODS uses syslog for logging. If your OS has a running syslog service, messages\n'
            'will appear in the system log file (normally located at /var/log/syslog).\n\n'
            'See the following for more information about configuring an iRODS-specific log file:\n\n'
            f'  https://docs.irods.org/{irods_version_string}/system_overview/troubleshooting/'))

    l.info(irods.lib.get_header('Stopping iRODS...'))
    IrodsController(irods_config).stop()

    l.info(irods.lib.get_header('iRODS is configured and ready to be started'))

def replace_in_file(filepath, original, replacement, flags=0):
    with open(filepath, "r+") as f:
        contents = f.read()
        pattern = re.compile(re.escape(original), flags)
        new_contents = pattern.sub(replacement, contents)
        f.seek(0)
        f.truncate()
        f.write(new_contents)

def test_put(irods_config):
    l = logging.getLogger(__name__)
    l.info(irods.lib.get_header('Running Post-Install Test'))

    if 0 != irods.lib.execute_command_permissive(irods.paths.test_put_get_executable())[2]:
        raise IrodsError('Post-install test failed. Please check your configuration.')

    l.info('Success')

def check_hostname():
    l = logging.getLogger(__name__)

    hostname = irods.lib.get_hostname()
    try:
        hostname.index('.')
    except ValueError:
        l.warning('Warning: Hostname `%s` should be a fully qualified domain name.' % (hostname))

    if not irods.lib.hostname_resolves_to_local_address(hostname):
        raise IrodsError('The hostname (%s) must resolve to the local machine.' % (hostname))

def determine_server_role(irods_config):
    # The list of catalog service roles supported by the server.
    catalog_service_roles = ['provider', 'consumer']

    # Get the service role previously configured if available. Otherwise, set to "provider".
    default_catalog_service_role = irods_config.server_config.get('catalog_service_role', 'provider')

    # Get the new value from user via default_prompt
    irods_config.server_config['catalog_service_role'] = irods.lib.default_prompt(
        'iRODS server\'s role',
        default=catalog_service_roles,
        previous=catalog_service_roles.index(default_catalog_service_role) + 1,
        input_filter=irods.lib.set_filter(catalog_service_roles, field='Server role'))

    # Save
    irods_config.commit(irods_config.server_config, irods_config.server_config_path, clear_cache=False)

def determine_local_storage(irods_config):
    l = logging.getLogger(__name__)
    l.info(irods.lib.get_header('Determining local storage'))

    local_storage = irods.lib.default_prompt(
        'Local storage on this server',
        default=['yes'])

    if local_storage.lower() in ('y','yes'):
        l.info('True')
        return True

    l.info('False')
    return False

def get_default_resource_name(irods_config):
    l = logging.getLogger(__name__)
    l.info(irods.lib.get_header('Setting up default resource name'))

    if irods_config.is_provider:
        default_default = irods_config.server_config.get('default_resource_name', 'demoResc')
    else:
        default_default = irods_config.server_config.get('default_resource_name', ''.join([irods.lib.get_hostname().split('.')[0], 'Resource']))

    default_resource_name = irods.lib.default_prompt(
        'Default resource',
        default=[default_default])

    return default_resource_name

def get_and_create_default_resource_vault(irods_config):
    l = logging.getLogger(__name__)
    l.info(irods.lib.get_header('Setting up default vault'))

    default_resource_directory = irods.lib.default_prompt(
        'iRODS Vault directory',
        default=[os.path.join(irods_config.irods_directory, 'Vault')])
    if not os.path.exists(default_resource_directory):
        os.makedirs(default_resource_directory, mode=0o700)

    return default_resource_directory

def get_irods_user_and_group(irods_config):
    l = logging.getLogger(__name__)
    l.info('The iRODS service account name needs to be defined.')
    if pwd.getpwnam(irods_config.irods_user).pw_uid == 0:
        irods_user = irods.lib.default_prompt('iRODS user', default=['irods'])
        irods_group = irods.lib.default_prompt('iRODS group', default=[irods_user])
    else:
        irods_user = irods.lib.default_prompt('iRODS user', default=[irods_config.irods_user])
        irods_group = irods.lib.default_prompt('iRODS group', default=[irods_config.irods_group])
    return (irods_user, irods_group)

def setup_service_account(irods_config, irods_user, irods_group):
    l = logging.getLogger(__name__)
    l.info(irods.lib.get_header('Setting up the service account'))

    if irods_group not in [g.gr_name for g in grp.getgrall()]:
        l.info('Creating Service Group: %s', irods_group)
        cmd = ['groupadd', '-r', irods_group]
        out, err, returncode = irods.lib.execute_command_permissive(cmd)
        if returncode == 9:
            l.info('Existing non-local Group Detected: %s', irods_group)
        else:
            irods.lib.check_command_return(cmd, out, err, returncode, input=None)
    else:
        l.info('Existing Group Detected: %s', irods_group)

    if irods.lib.execute_command_permissive(['id', irods_user])[2] != 0:
        l.info('Creating Service Account: %s', irods_group)
        irods.lib.execute_command([
            'useradd',
            '-r',
            '-d', irods_config.irods_directory,
            '-M',
            '-s', '/bin/bash',
            '-g', irods_group,
            '-c', 'iRODS Administrator',
            '-p', '!',
            irods_user])
    else:
        l.info('Existing Account Detected: %s', irods_user)

    with open(irods_config.service_account_file_path, 'wt') as f:
        print('IRODS_SERVICE_ACCOUNT_NAME=%s' % (irods_user), file=f)
        print('IRODS_SERVICE_GROUP_NAME=%s' % (irods_group), file=f)

    l.info('Setting owner of %s to %s:%s',
            irods_config.irods_directory, irods_user, irods_group)
    for (root, _, files) in os.walk(irods_config.irods_directory):
        os.lchown(root,
                pwd.getpwnam(irods_user).pw_uid,
                grp.getgrnam(irods_group).gr_gid)
        for filename in files:
            os.lchown(os.path.join(root, filename),
                    pwd.getpwnam(irods_user).pw_uid,
                    grp.getgrnam(irods_group).gr_gid)

    l.info('Setting owner of %s to %s:%s',
            irods_config.config_directory, irods_user, irods_group)
    for (root, _, files) in os.walk(irods_config.config_directory):
        os.lchown(root,
                pwd.getpwnam(irods_user).pw_uid,
                grp.getgrnam(irods_group).gr_gid)
        for filename in files:
            os.lchown(os.path.join(root, filename),
                    pwd.getpwnam(irods_user).pw_uid,
                    grp.getgrnam(irods_group).gr_gid)

    l.debug('Setting uid bit on %s', irods.paths.genosauth_path())
    os.chmod(irods.paths.genosauth_path(),
            stat.S_ISUID
            | stat.S_IRUSR
            | stat.S_IXUSR
            | stat.S_IRGRP
            | stat.S_IXGRP
            | stat.S_IROTH
            | stat.S_IXOTH
            )

    #owner of top-level directory changed, clear the cache
    irods_config.clear_cache()

def setup_storage(irods_config):
    create_local_storage = determine_local_storage(irods_config)

    if create_local_storage:
        # prompt for default resource information
        resource_name = get_default_resource_name(irods_config)
        resource_vault_path = get_and_create_default_resource_vault(irods_config)
    else:
        # prompt and set default resource to one hosted elsewhere - the test_put will catch any typos
        resource_name = get_default_resource_name(irods_config)
        resource_vault_path = None # passed to 'setting up the database'

    irods_config.server_config['default_resource_name'] = resource_name

    return (resource_name, resource_vault_path)

def setup_server_config(irods_config):
    l = logging.getLogger(__name__)
    l.info(irods.lib.get_header('Configuring the server options'))

    ld_library_path_list = get_ld_library_path_list()
    if ld_library_path_list:
        if 'environment_variables' not in irods_config.server_config:
            irods_config.server_config['environment_variables'] = {}
        irods_config.server_config['environment_variables']['LD_LIBRARY_PATH'] = ':'.join(ld_library_path_list)

    while True:
        irods_config.server_config['zone_name'] = irods.lib.default_prompt(
            'iRODS server\'s zone name',
            default=[irods_config.server_config.get('zone_name', 'tempZone')],
            input_filter=irods.lib.character_count_filter(minimum=1, field='Zone name'))

        if irods_config.is_provider:
            irods_config.server_config['catalog_provider_hosts'] = [irods.lib.get_hostname()]
        elif irods_config.is_consumer:
            irods_config.server_config['catalog_provider_hosts'] = [irods.lib.prompt(
                'iRODS catalog (ICAT) host',
                input_filter=irods.lib.character_count_filter(minimum=1, field='iRODS catalog hostname'))]

        irods_config.server_config['zone_port'] = irods.lib.default_prompt(
            'iRODS server\'s port',
            default=[irods_config.server_config.get('zone_port', 1247)],
            input_filter=irods.lib.int_filter(field='Port'))

        irods_config.server_config['server_port_range_start'] = irods.lib.default_prompt(
            'iRODS port range (begin)',
            default=[irods_config.server_config.get('server_port_range_start', 20000)],
            input_filter=irods.lib.int_filter(field='Port'))

        irods_config.server_config['server_port_range_end'] = irods.lib.default_prompt(
            'iRODS port range (end)',
            default=[irods_config.server_config.get('server_port_range_end', 20199)],
            input_filter=irods.lib.int_filter(field='Port'))

        irods_config.server_config['server_control_plane_port'] = irods.lib.default_prompt(
            'Control Plane port',
            default=[irods_config.server_config.get('server_control_plane_port', 1248)],
            input_filter=irods.lib.int_filter(field='Port'))

        irods_config.server_config['schema_validation_base_uri'] = irods.lib.default_prompt(
            'Schema Validation Base URI (or off)',
            default=[irods_config.server_config.get('schema_validation_base_uri', 'file://{0}/configuration_schemas'.format(irods.paths.irods_directory()))],
            input_filter=irods.lib.character_count_filter(minimum=1, field='Schema validation base URI'))

        irods_config.server_config['zone_user'] = irods.lib.default_prompt(
            'iRODS server\'s administrator username',
            default=[irods_config.server_config.get('zone_user', 'rods')],
            input_filter=irods.lib.character_count_filter(minimum=1, field='iRODS server\'s administrator username'))

        confirmation_message = ''.join([
                '\n',
                '-------------------------------------------\n',
                'Zone name:                  %s\n',
                'iRODS catalog host:         %s\n' if irods_config.is_consumer else '%s',
                'iRODS server port:          %d\n',
                'iRODS port range (begin):   %d\n',
                'iRODS port range (end):     %d\n',
                'Control plane port:         %d\n',
                'Schema validation base URI: %s\n',
                'iRODS server administrator: %s\n',
                '-------------------------------------------\n\n',
                'Please confirm']) % (
                    irods_config.server_config['zone_name'],
                    irods_config.server_config['catalog_provider_hosts'][0] if irods_config.is_consumer else '',
                    irods_config.server_config['zone_port'],
                    irods_config.server_config['server_port_range_start'],
                    irods_config.server_config['server_port_range_end'],
                    irods_config.server_config['server_control_plane_port'],
                    irods_config.server_config['schema_validation_base_uri'],
                    irods_config.server_config['zone_user']
                    )

        if irods.lib.default_prompt(confirmation_message, default=['yes']) in ['', 'y', 'Y', 'yes', 'YES']:
            break

    irods_config.server_config['zone_key'] = irods.lib.prompt(
        'iRODS server\'s zone key',
        input_filter=irods.lib.character_count_filter(minimum=1, field='Zone key'),
        echo=False)

    irods_config.server_config['negotiation_key'] = irods.lib.prompt(
        'iRODS server\'s negotiation key (32 characters)',
        input_filter=irods.lib.character_count_filter(minimum=32, maximum=32, field='Negotiation key'),
        echo=False)

    irods_config.server_config['server_control_plane_key'] = irods.lib.prompt(
        'Control Plane key (32 characters)',
        input_filter=irods.lib.character_count_filter(minimum=32, maximum=32, field='Control Plane key'),
        echo=False)

    irods_config.commit(irods_config.server_config, irods_config.server_config_path, clear_cache=False)

def setup_client_environment(irods_config):
    l = logging.getLogger(__name__)
    l.info(irods.lib.get_header('Setting up the client environment'))

    print('\n', end='')

    irods_config.admin_password = irods.lib.prompt(
        'iRODS server\'s administrator password',
        input_filter=irods.lib.character_count_filter(minimum=3, maximum=maximum_password_length, field='Admin password'),
        echo=False)

    print('\n', end='')

    service_account_dict = {
            'schema_name': 'service_account_environment',
            'schema_version': 'v4',
            'irods_host': irods.lib.get_hostname(),
            'irods_port': irods_config.server_config['zone_port'],
            'irods_default_resource': irods_config.server_config['default_resource_name'],
            'irods_home': '/'.join(['', irods_config.server_config['zone_name'], 'home', irods_config.server_config['zone_user']]),
            'irods_cwd': '/'.join(['', irods_config.server_config['zone_name'], 'home', irods_config.server_config['zone_user']]),
            'irods_user_name': irods_config.server_config['zone_user'],
            'irods_zone_name': irods_config.server_config['zone_name'],
            'irods_client_server_negotiation': 'request_server_negotiation',
            'irods_client_server_policy': 'CS_NEG_REFUSE',
            'irods_encryption_key_size': 32,
            'irods_encryption_salt_size': 8,
            'irods_encryption_num_hash_rounds': 16,
            'irods_encryption_algorithm': 'AES-256-CBC',
            'irods_default_hash_scheme': irods_config.server_config['default_hash_scheme'],
            'irods_match_hash_policy': irods_config.server_config['match_hash_policy'],
            'irods_server_control_plane_port': irods_config.server_config['server_control_plane_port'],
            'irods_server_control_plane_key': irods_config.server_config['server_control_plane_key'],
            'irods_server_control_plane_encryption_num_hash_rounds': irods_config.server_config['server_control_plane_encryption_num_hash_rounds'],
            'irods_server_control_plane_encryption_algorithm': irods_config.server_config['server_control_plane_encryption_algorithm'],
            'irods_maximum_size_for_single_buffer_in_megabytes': irods_config.server_config['advanced_settings']['maximum_size_for_single_buffer_in_megabytes'],
            'irods_default_number_of_transfer_threads': irods_config.server_config['advanced_settings']['default_number_of_transfer_threads'],
            'irods_transfer_buffer_size_for_parallel_transfer_in_megabytes': irods_config.server_config['advanced_settings']['transfer_buffer_size_for_parallel_transfer_in_megabytes'],
            'irods_connection_pool_refresh_time_in_seconds': 300,
        }
    if not os.path.exists(os.path.dirname(irods_config.client_environment_path)):
        os.makedirs(os.path.dirname(irods_config.client_environment_path), mode=0o700)
    irods_config.commit(service_account_dict, irods_config.client_environment_path, clear_cache=False)

def main():
    l = logging.getLogger(__name__)
    logging.getLogger().setLevel(logging.NOTSET)

    irods.log.register_tty_handler(sys.stderr, logging.WARNING, None)

    (options, _) = parse_options()

    irods_config = IrodsConfig()

    irods.log.register_file_handler(irods_config.setup_log_path)
    if options.verbose > 0:
        llevel = logging.NOTSET
        if options.verbose == 1:
            llevel = logging.INFO
        elif options.verbose == 2:
            llevel = logging.DEBUG
        irods.log.register_tty_handler(sys.stdout, llevel, logging.WARNING)

    if options.server_log_level != None:
        irods_config.injected_environment['spLogLevel'] = str(options.server_log_level)
    if options.sql_log_level != None:
        irods_config.injected_environment['spLogSql'] = str(options.sql_log_level)
    if options.days_per_log != None:
        irods_config.injected_environment['logfileInt'] = str(options.days_per_log)
    if options.rule_engine_server_options != None:
        irods_config.injected_environment['reServerOption'] = options.rule_engine_server_options
    if options.server_reconnect_flag:
        irods_config.injected_environment['irodsReconnect'] = ''

    try:
        setup_server(irods_config,
                     json_configuration_file=options.json_configuration_file,
                     test_mode=options.test_mode)
    except IrodsError:
        l.error('Error encountered running setup_irods:\n', exc_info=True)
        l.info('Exiting...')
        return 1
    except KeyboardInterrupt:
        l.info('Script terminated by user.')
        l.debug('KeyboardInterrupt encountered:\n', exc_info=True)
    return 0


if __name__ == '__main__':
    sys.exit(main())
