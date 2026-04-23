#!/usr/bin/python3

import argparse
import os
import sys

import irods.setup_options

def parse_arguments():
    parser = argparse.ArgumentParser()
    irods.setup_options.add_options(parser)

    return parser.parse_args()

def get_ld_library_path_list():

    ld_library_path_list = []

    args = parse_arguments()
    if args.ld_library_path:
        ld_library_path_list = [p for p in args.ld_library_path.split(':') if p]

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

from irods.configuration import IrodsConfig
from irods.controller import IrodsController
from irods.exceptions import IrodsError, IrodsWarning
from irods.password_obfuscation import maximum_password_length
import irods.lib
import irods.log
import irods.paths

try:
    import pyodbc
except:
    print('WARNING: pyodbc module could not be imported.', file=sys.stderr)
    print('The pyodbc module is required for setup of catalog service providers.', file=sys.stderr)


def setup_server(irods_config, json_configuration_file=None, test_mode=False, optional_prompts=[]):
    l = logging.getLogger(__name__)

    optional_setup_funcs = {
        "tls": setup_tls
    }

    if json_configuration_file is not None:
        with open(json_configuration_file) as f:
            json_configuration_dict = json.load(f)
    else:
        json_configuration_dict = None
        # Setup the hostname, FQDN, or IP to listen on for client requests.
        setup_server_host(irods_config)

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
        # Go through any optional setup prompted by setup script options.
        for prompt_name in optional_prompts:
            if (setup_func := optional_setup_funcs.get(prompt_name)) is None:
                l.warning(f"No setup function found for optional prompt '{prompt_name}'. Skipping.")
                continue
            setup_func(irods_config)
        # server config
        setup_server_config(irods_config)
        # client environment and password
        setup_client_environment(irods_config)

    if irods_config.is_provider:
        from irods import database_interface
        l.info(irods.lib.get_header('Setting up the database'))
        database_interface.setup_catalog(irods_config, default_resource_directory=default_resource_directory, default_resource_name=default_resource_name)
        l.info(irods.lib.get_header('Applying updates to database'))
        database_interface.run_catalog_update(irods_config, is_upgrade=False)

    # Copy iRODS Rule Language (NREP) files into correct directory if this is a new install.
    for f in ['core.re', 'core.dvm', 'core.fnm']:
        path = os.path.join(irods_config.config_directory, f)
        if not os.path.exists(path):
            shutil.copyfile(irods.paths.get_template_filepath(path), path)

    core_re_path = os.path.join(irods_config.core_re_directory, 'core.re')

    # Update core.re to reflect configured TLS. This is done here because unattended installations will not use the TLS
    # setup function.
    if "CS_NEG_REQUIRE" == irods_config.server_config.get("client_server_policy", "CS_NEG_REFUSE"):
        acPreConnect_rule_to_replace = 'acPreConnect(*OUT) { *OUT="CS_NEG_REFUSE"; }'
        acPreConnect_rule_replacement = 'acPreConnect(*OUT) { *OUT="CS_NEG_REQUIRE"; }'
        replace_in_file(core_re_path, acPreConnect_rule_to_replace, acPreConnect_rule_replacement)

    l.info(irods.lib.get_header('Starting iRODS...'))
    controller = IrodsController(irods_config)
    controller.start()

    # create local storage resource for consumer (provider was configured directly in database_interface.setup_catalog above)
    # If the user answered anything other than "yes" or "y" (case-insensitive) to the "Local storage on this server"
    # prompt, the local storage resource should not be created. This is indicated by default_resource_directory being
    # None, as returned by setup_storage().
    if irods_config.is_consumer and default_resource_directory is not None:
        irods.lib.execute_command(['iadmin', 'mkresc', default_resource_name, 'unixfilesystem', ':'.join([irods.lib.get_hostname(), default_resource_directory]), ''])

    # update core.re with default resource
    replace_in_file(core_re_path, 'demoResc', default_resource_name)

    # Restart the server in case core.re changed.
    # We could use .reload_configuration(), but restarting the server reduces the
    # possibility of errors occurring. 
    controller.restart()

    # test put
    args = parse_arguments()
    if not args.skip_post_install_test:
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
    controller.stop()

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

def setup_server_host(irods_config):
    l = logging.getLogger(__name__)

    irods_config.server_config['host'] = irods.lib.default_prompt(
        'iRODS FQDN, hostname, or IP (253 characters max)',
        default=[irods.lib.get_hostname()],
        input_filter=irods.lib.character_count_filter(minimum=1, maximum=253, field='iRODS server host'))

    irods_config.commit(irods_config.server_config, irods_config.server_config_path, clear_cache=False)

    try:
        irods_config.server_config['host'].index('.')
    except ValueError:
        l.warning('Warning: Hostname `%s` should be a fully qualified domain name.', irods_config.server_config['host'])

    if not irods.lib.hostname_resolves_to_local_address(irods_config.server_config['host']):
        raise IrodsError(f'The hostname [{irods_config.server_config["host"]}] must resolve to the local machine.')

def determine_server_role(irods_config):
    # The list of catalog service roles supported by the server.
    catalog_service_roles = ['provider', 'consumer']

    # Get the service role previously configured if available. Otherwise, set to "provider".
    default_catalog_service_role = irods_config.server_config.get('catalog_service_role', 'provider')

    # Get the new value from user via default_prompt
    irods_config.server_config['catalog_service_role'] = irods.lib.default_prompt(
        'iRODS role',
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
        'iRODS vault directory',
        default=[os.path.join(irods_config.irods_directory, 'Vault')])
    if not os.path.exists(default_resource_directory):
        os.makedirs(default_resource_directory, mode=0o700)

    return default_resource_directory

def get_irods_user_and_group(irods_config):
    l = logging.getLogger(__name__)
    l.info('The iRODS service account name needs to be defined.')
    if pwd.getpwnam(irods_config.irods_user).pw_uid == 0:
        irods_user = irods.lib.default_prompt(
            'iRODS service account username',
            default=['irods'],
            input_filter=irods.lib.character_count_filter(minimum=1, field='iRODS Service Account Username'))
        irods_group = irods.lib.default_prompt(
            'iRODS service account group',
            default=[irods_user],
            input_filter=irods.lib.character_count_filter(minimum=1, field='iRODS Service Account Group'))
    else:
        irods_user = irods.lib.default_prompt(
            'iRODS service account username',
            default=[irods_config.irods_user],
            input_filter=irods.lib.character_count_filter(minimum=1, field='iRODS Service Account Username'))
        irods_group = irods.lib.default_prompt(
            'iRODS service account group',
            default=[irods_config.irods_group],
            input_filter=irods.lib.character_count_filter(minimum=1, field='iRODS Service Account Group'))
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

    os.lchown(os.path.join(irods.paths.runstate_directory(), 'irods'),
              pwd.getpwnam(irods_user).pw_uid,
              grp.getgrnam(irods_group).gr_gid)

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


def setup_tls(irods_config):
    l = logging.getLogger(__name__)
    l.info(irods.lib.get_header('Setting up TLS'))

    while True:
        # TLS configuration for incoming connections.
        if "tls_server" not in irods_config.server_config:
            irods_config.server_config["tls_server"] = {}

        default_certificate_chain_file = irods_config.server_config["tls_server"].get("certificate_chain_file")
        irods_config.server_config["tls_server"]['certificate_chain_file'] = irods.lib.default_prompt(
            "Path to server's certificate chain file",
            default=[default_certificate_chain_file] if default_certificate_chain_file else None,
            input_filter=irods.lib.character_count_filter(minimum=1, field='Certificate chain file'))

        default_certificate_key_file = irods_config.server_config["tls_server"].get("certificate_key_file")
        irods_config.server_config["tls_server"]['certificate_key_file'] = irods.lib.default_prompt(
            "Path to server's certificate key file",
            default=[default_certificate_key_file] if default_certificate_key_file else None,
            input_filter=irods.lib.character_count_filter(minimum=1, field='Certificate key file'))

        default_dh_params_file = irods_config.server_config["tls_server"].get("dh_params_file")
        irods_config.server_config["tls_server"]['dh_params_file'] = irods.lib.default_prompt(
            "Path to Diffie-Hellman parameter file",
            default=[default_dh_params_file] if default_dh_params_file else None,
            input_filter=irods.lib.character_count_filter(minimum=1, field='Diffie-Hellman params file'))

        # TLS configuration for outgoing connections (server-to-server).
        if "tls_client" not in irods_config.server_config:
            irods_config.server_config["tls_client"] = {}

        default_ca_certificate_file = irods_config.server_config["tls_client"].get('ca_certificate_file')
        client_ca_certificate_file = irods.lib.default_prompt(
            "Path to CA certificate file",
            default=[default_ca_certificate_file] if default_ca_certificate_file else None)

        default_ca_certificate_path = irods_config.server_config["tls_client"].get('ca_certificate_path')
        client_ca_certificate_path = irods.lib.default_prompt(
            "Path to CA certificates directory",
            default=[default_ca_certificate_path] if default_ca_certificate_path else None)

        default_verify_server = irods_config.server_config["tls_client"].get('verify_server') or "cert"
        verify_server_levels = ["hostname", "cert", "none"]
        client_verify_server = irods.lib.default_prompt(
            "Certificate verification level",
            default=verify_server_levels,
            previous=verify_server_levels.index(default_verify_server) + 1,
            input_filter=irods.lib.set_filter(verify_server_levels, field='Verify server'))

        if client_ca_certificate_file:
            irods_config.server_config["tls_client"]['ca_certificate_file'] = client_ca_certificate_file
        if client_ca_certificate_path:
            irods_config.server_config["tls_client"]['ca_certificate_path'] = client_ca_certificate_path
        irods_config.server_config["tls_client"]['verify_server'] = client_verify_server

        confirmation_message = (
            '\n'
            '-------------------------------------------------------------\n'
            f'Certificate chain file:         {irods_config.server_config["tls_server"].get("certificate_chain_file")}\n'
            f'Certificate key file:           {irods_config.server_config["tls_server"].get("certificate_key_file")}\n'
            f'Diffie-Hellman parameters file: {irods_config.server_config["tls_server"].get("dh_params_file")}\n'
            f'CA certificate file:            {irods_config.server_config["tls_client"].get("ca_certificate_file", "")}\n'
            f'CA certificate path:            {irods_config.server_config["tls_client"].get("ca_certificate_path", "")}\n'
            f'Client verify server level:     {irods_config.server_config["tls_client"].get("verify_server")}\n'
            '-------------------------------------------------------------\n\n'
            'Please confirm'
        )

        if irods.lib.default_prompt(confirmation_message, default=['yes']) in ['', 'y', 'Y', 'yes', 'YES']:
            break

    irods_config.server_config["client_server_policy"] = "CS_NEG_REQUIRE"

    irods_config.commit(irods_config.server_config, irods_config.server_config_path, clear_cache=False)


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
            'iRODS zone name',
            default=[irods_config.server_config.get('zone_name', 'tempZone')],
            input_filter=irods.lib.character_count_filter(minimum=1, field='Zone name'))

        if irods_config.is_provider:
            irods_config.server_config['catalog_provider_hosts'] = [irods_config.server_config['host']]
        elif irods_config.is_consumer:
            irods_config.server_config['catalog_provider_hosts'] = [irods.lib.prompt(
                'iRODS Catalog Provider FQDN, hostname, or IP (253 characters max)',
                input_filter=irods.lib.character_count_filter(minimum=1, maximum=253, field='iRODS catalog provider host'))]

        irods_config.server_config['zone_port'] = irods.lib.default_prompt(
            'iRODS zone port',
            default=[irods_config.server_config.get('zone_port', 1247)],
            input_filter=irods.lib.int_filter(field='Zone port'))

        irods_config.server_config['server_port_range_start'] = irods.lib.default_prompt(
            'iRODS parallel transfer port range (begin)',
            default=[irods_config.server_config.get('server_port_range_start', 20000)],
            input_filter=irods.lib.int_filter(field='Parallel transfer port'))

        irods_config.server_config['server_port_range_end'] = irods.lib.default_prompt(
            'iRODS parallel transfer port range (end)',
            default=[irods_config.server_config.get('server_port_range_end', 20199)],
            input_filter=irods.lib.int_filter(field='Parallel transfer port'))

        irods_config.server_config['zone_user'] = irods.lib.default_prompt(
            'iRODS administrator username',
            default=[irods_config.server_config.get('zone_user', 'rods')],
            input_filter=irods.lib.character_count_filter(minimum=1, field='iRODS administrator username'))

        confirmation_message = ''.join([
                '\n',
                '-------------------------------------------------------------\n',
                'iRODS Zone Name:                            %s\n',
                'iRODS Zone Port:                            %d\n',
                'iRODS Catalog Provider Host:                %s\n' if irods_config.is_consumer else '%s',
                'iRODS Parallel Transfer Port Range (begin): %d\n',
                'iRODS Parallel Transfer Port Range (end):   %d\n',
                'iRODS Administrator:                        %s\n',
                '-------------------------------------------------------------\n\n',
                'Please confirm']) % (
                    irods_config.server_config['zone_name'],
                    irods_config.server_config['zone_port'],
                    irods_config.server_config['catalog_provider_hosts'][0] if irods_config.is_consumer else '',
                    irods_config.server_config['server_port_range_start'],
                    irods_config.server_config['server_port_range_end'],
                    irods_config.server_config['zone_user']
                    )

        if irods.lib.default_prompt(confirmation_message, default=['yes']) in ['', 'y', 'Y', 'yes', 'YES']:
            break

    irods_config.server_config['zone_key'] = irods.lib.prompt(
        'iRODS zone key',
        input_filter=irods.lib.character_count_filter(minimum=1, field='Zone key'),
        echo=False)

    irods_config.server_config['negotiation_key'] = irods.lib.prompt(
        'iRODS negotiation key (32 characters)',
        input_filter=irods.lib.character_count_filter(minimum=32, maximum=32, field='Negotiation key'),
        echo=False)

    irods_config.commit(irods_config.server_config, irods_config.server_config_path, clear_cache=False)

def setup_client_environment(irods_config):
    l = logging.getLogger(__name__)
    l.info(irods.lib.get_header('Setting up the client environment'))

    print('\n', end='')

    irods_config.admin_password = irods.lib.prompt(
        'iRODS administrator password',
        input_filter=irods.lib.character_count_filter(minimum=3, maximum=maximum_password_length, field='Admin password'),
        echo=False)

    print('\n', end='')

    service_account_dict = {
        'schema_name': 'service_account_environment',
        'schema_version': 'v5',
        'irods_host': irods_config.server_config['host'],
        'irods_port': irods_config.server_config['zone_port'],
        'irods_default_resource': irods_config.server_config['default_resource_name'],
        'irods_home': '/'.join(['', irods_config.server_config['zone_name'], 'home', irods_config.server_config['zone_user']]),
        'irods_cwd': '/'.join(['', irods_config.server_config['zone_name'], 'home', irods_config.server_config['zone_user']]),
        'irods_user_name': irods_config.server_config['zone_user'],
        'irods_zone_name': irods_config.server_config['zone_name'],
        'irods_client_server_policy': irods_config.server_config['client_server_policy'],
        'irods_encryption_algorithm': irods_config.server_config['encryption']['algorithm'],
        'irods_encryption_key_size': irods_config.server_config['encryption']['key_size'],
        'irods_encryption_num_hash_rounds': irods_config.server_config['encryption']['num_hash_rounds'],
        'irods_encryption_salt_size': irods_config.server_config['encryption']['salt_size'],
        'irods_default_hash_scheme': irods_config.server_config['default_hash_scheme'],
        'irods_match_hash_policy': irods_config.server_config['match_hash_policy'],
        'irods_maximum_size_for_single_buffer_in_megabytes': irods_config.server_config['advanced_settings']['maximum_size_for_single_buffer_in_megabytes'],
        'irods_default_number_of_transfer_threads': irods_config.server_config['advanced_settings']['default_number_of_transfer_threads'],
        'irods_transfer_buffer_size_for_parallel_transfer_in_megabytes': irods_config.server_config['advanced_settings']['transfer_buffer_size_for_parallel_transfer_in_megabytes'],
        'irods_connection_pool_refresh_time_in_seconds': irods_config.server_config['connection_pool_refresh_time_in_seconds']
    }
    if tls_client_config := irods_config.server_config.get("tls_client"):
        service_account_dict["irods_ssl_ca_certificate_file"] = tls_client_config["ca_certificate_file"]
        service_account_dict["irods_ssl_verify_server"] = tls_client_config["verify_server"]

    if not os.path.exists(os.path.dirname(irods_config.client_environment_path)):
        os.makedirs(os.path.dirname(irods_config.client_environment_path), mode=0o700)
    irods_config.commit(service_account_dict, irods_config.client_environment_path, clear_cache=False)

def main():
    l = logging.getLogger(__name__)
    logging.getLogger().setLevel(logging.NOTSET)

    irods.log.register_tty_handler(sys.stderr, logging.WARNING, None)

    args = parse_arguments()

    irods_config = IrodsConfig()
    optional_prompts = []

    irods.log.register_file_handler(irods_config.setup_log_path)
    if None != args.verbose and args.verbose > 0:
        llevel = logging.NOTSET
        if args.verbose == 1:
            llevel = logging.INFO
        elif args.verbose == 2:
            llevel = logging.DEBUG
        irods.log.register_tty_handler(sys.stdout, llevel, logging.WARNING)

    if args.server_log_level != None:
        irods_config.injected_environment['spLogLevel'] = str(args.server_log_level)
    if args.days_per_log != None:
        irods_config.injected_environment['logfileInt'] = str(args.days_per_log)
    if args.rule_engine_server_options != None:
        irods_config.injected_environment['reServerOption'] = args.rule_engine_server_options
    if args.server_reconnect_flag:
        irods_config.injected_environment['irodsReconnect'] = ''
    if args.prompt_tls:
        optional_prompts.append("tls")

    try:
        setup_server(irods_config,
                     json_configuration_file=args.json_configuration_file,
                     test_mode=args.test_mode,
                     optional_prompts=optional_prompts)
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
