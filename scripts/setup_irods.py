#!/usr/bin/python
from __future__ import print_function
import contextlib
import copy
import getpass
import grp
import itertools
import json
import logging
import optparse
import os
import pprint
import pwd
import sys
import time
import tempfile

from irods import six
from irods import pypyodbc

import irods_control
import irods.database_connect
import irods.lib
from irods.configuration import IrodsConfig
from irods.controller import IrodsController
from irods.exceptions import IrodsError, IrodsWarning
import irods.log

class InputFilterError(Exception):
    pass

def setup_server(irods_config):
    setup_service_account(irods_config)

    #Do the rest of the setup as the irods user
    os.seteuid(pwd.getpwnam(irods_config.irods_user).pw_uid)

    setup_server_config(irods_config)
    setup_client_environment(irods_config)

    default_resource_directory = default_prompt(
        'iRODS Vault directory',
        default=[os.path.join(irods_config.irods_directory, 'Vault')])
    if not os.path.exists(default_resource_directory):
        os.makedirs(default_resource_directory, mode=0o700)

    if irods_config.is_catalog:
        setup_catalog(irods_config, default_resource_directory=default_resource_directory)
    else:
        setup_resource(irods_config, default_resource_directory=default_resource_directory)


def setup_service_account(irods_config):
    l = logging.getLogger(__name__)
    irods_user = irods_config.irods_user
    irods_group = irods_config.irods_group
    l.info('The iRODS service account name needs to be defined.')
    if irods_config.binary_installation and pwd.getpwnam(irods_user).pw_uid == 0:
        irods_user = default_prompt('iRODS user', default=['irods'])
        irods_group = default_prompt('iRODS group', default=[irods_user])
    else:
        irods_user = default_prompt('iRODS user', default=[irods_user])
        irods_group = default_prompt('iRODS group', default=[irods_group])

    if irods_group not in [g.gr_name for g in grp.getgrall()]:
        l.info('Creating Service Group: %s', irods_group)
        irods.lib.execute_command(['groupadd', '-r', irods_group])
    else:
        l.info('Existing Group Detected: %s', irods_group)

    if irods_user not in [u.pw_name for u in pwd.getpwall()]:
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

#        try:
#            irods.lib.execute_command(['passwd', '-l', irods_user])
#        except IrodsError:
#            l.warning('Warning: could not lock the service account. The service account may be accessible with no password.')

    with open(irods_config.service_account_file_path, 'wt') as f:
        print('IRODS_SERVICE_ACCOUNT_NAME=%s', irods_user, file=f)
        print('IRODS_SERVICE_GROUP_NAME=%s', irods_group, file=f)

    l.info('Setting owner of %s to %s %s',
            irods_config.top_level_directory, irods_user, irods_group)
    for (root, _, files) in os.walk(irods_config.top_level_directory):
        os.lchown(root,
                pwd.getpwnam(irods_user).pw_uid,
                grp.getgrnam(irods_group).gr_gid)
        for filename in files:
            os.lchown(os.path.join(root, filename),
                    pwd.getpwnam(irods_user).pw_uid,
                    grp.getgrnam(irods_group).gr_gid)

    l.info('Setting owner of %s to %s %s',
            irods_config.config_directory, irods_user, irods_group)
    for (root, _, files) in os.walk(irods_config.config_directory):
        os.lchown(root,
                pwd.getpwnam(irods_user).pw_uid,
                grp.getgrnam(irods_group).gr_gid)
        for filename in files:
            os.lchown(os.path.join(root, filename),
                    pwd.getpwnam(irods_user).pw_uid,
                    grp.getgrnam(irods_group).gr_gid)

    os.lchown(os.path.join(irods_config.server_bin_directory, 'PamAuthCheck'), 0, 0)
    irods_config.clear_cache()

    return (irods_user, irods_group)

def setup_server_config(irods_config):
    try :
        server_config = copy.deepcopy(irods_config.server_config)
    except (OSError, ValueError):
        server_config = {}

    while True:
        server_config['zone_name'] = default_prompt(
            'iRODS server\'s zone name',
            default=[server_config.get('zone_name', 'tempZone')],
            input_filter=character_count_filter(minimum=1, field='Zone name'))

        server_config['zone_port'] = default_prompt(
            'iRODS server\'s port',
            default=[server_config.get('zone_port', 1247)],
            input_filter=int_filter(field='Port'))

        server_config['server_port_range_start'] = default_prompt(
            'iRODS port range (begin)',
            default=[server_config.get('server_port_range_start', 20000)],
            input_filter=int_filter(field='Port'))

        server_config['server_port_range_end'] = default_prompt(
            'iRODS port range (end)',
            default=[server_config.get('server_port_range_end', 20199)],
            input_filter=int_filter(field='Port'))

        server_config['zone_key'] = prompt(
            'iRODS server\'s zone key',
            input_filter=character_count_filter(minimum=1, field='Zone key'))

        server_config['negotiation_key'] = prompt(
            'iRODS server\'s negotiation key',
            input_filter=character_count_filter(minimum=32, maximum=32, field='Negotiation key'))

        server_config['server_control_plane_port'] = default_prompt(
            'Control Plane port',
            default=[server_config.get('server_control_plane_port', 1248)],
            input_filter=int_filter(field='Port'))

        server_config['server_control_plane_key'] = prompt(
            'Control Plane key',
            input_filter=character_count_filter(minimum=32, maximum=32, field='Control Plane key'))

        server_config['schema_validation_base_uri'] = default_prompt(
            'Schema Validation Base URI (or off)',
            default=[server_config.get('schema_validation_base_uri', 'https://schemas.irods.org/configuration')],
            input_filter=character_count_filter(minimum=1, field='Schema validation base URI'))

        server_config['zone_user'] = default_prompt(
            'iRODS server\'s administrator username',
            default=[server_config.get('zone_user', 'rods')],
            input_filter=character_count_filter(minimum=1, field='iRODS server\'s administrator username'))

        confirmation_message = ''.join([
                '\n',
                '-------------------------------------------\n',
                'Zone name:                  %s\n',
                'iRODS server port:          %d\n',
                'iRODS port range (begin):   %d\n',
                'iRODS port range (end):     %d\n',
                'Zone key:                   %s\n',
                'Negotiation key:            %s\n',
                'Control plane port:         %d\n',
                'Control plane key:          %s\n',
                'Schema validation base URI: %s\n',
                'iRODS server administrator: %s\n',
                '-------------------------------------------\n\n',
                'Please confirm']) % (
                    server_config['zone_name'],
                    server_config['zone_port'],
                    server_config['server_port_range_start'],
                    server_config['server_port_range_end'],
                    server_config['zone_key'],
                    server_config['negotiation_key'],
                    server_config['server_control_plane_port'],
                    server_config['server_control_plane_key'],
                    server_config['schema_validation_base_uri'],
                    server_config['zone_user']
                    )

        if default_prompt(confirmation_message, default=['yes']) in ['', 'y', 'Y', 'yes', 'YES']:
            break

    irods_config.commit(server_config, irods_config.server_config_path)

def setup_resource(irods_config, default_resource_directory=None):
    pass

def setup_catalog(irods_config, default_resource_directory=None):
    l = logging.getLogger(__name__)
    if os.path.exists(os.path.join(irods_config.top_level_directory, 'plugins', 'database', 'libpostgres.so')):
        db_type = 'postgres'
    elif os.path.exists(os.path.join(irods_config.top_level_directory, 'plugins', 'database', 'libmysql.so')):
        db_type = 'mysql'
    elif os.path.exists(os.path.join(irods_config.top_level_directory, 'plugins', 'database', 'liboracle.so')):
        db_type = 'oracle'
    else:
        raise IrodsError('Database type must be one of postgres, mysql, or oracle.')
    l.debug('setup_catalog has been called with database type \'%s\'.', db_type)

    try :
        db_config = copy.deepcopy(irods_config.database_config)
    except (OSError, ValueError):
        db_config = {}

    try :
        server_config = copy.deepcopy(irods_config.server_config)
    except (OSError, ValueError):
        server_config = {}

    l.info('You are configuring an iRODS database plugin. '
        'The iRODS server cannot be started until its database '
        'has been properly configured.\n'
        )

    db_config['catalog_database_type'] = db_type
    while True:
        if db_config['catalog_database_type'] == 'oracle':
            oracle_home = ''
            if 'environment_variables' not in server_config:
                server_config['environment_variables'] = {}
            if 'ORACLE_HOME' in server_config['environment_variables']:
                oracle_home = server_config['environment_variables']['ORACLE_HOME']
            elif 'ORACLE_HOME' in os.environ:
                oracle_home = os.environ['ORACLE_HOME']
            server_config['environment_variables']['ORACLE_HOME'] = default_prompt('$ORACLE_HOME', default=[oracle_home])

        odbc_drivers = irods.database_connect.get_odbc_drivers_for_db_type(db_config['catalog_database_type'])
        if odbc_drivers:
            db_config['db_odbc_driver'] = default_prompt(
                    'ODBC driver for %s', db_config['catalog_database_type'],
                    default=odbc_drivers)
        else:
            db_config['db_odbc_driver'] = default_prompt(
                    'No default ODBC drivers configured for %s; falling back to bare library paths', db_config['catalog_database_type'],
                    default=irods.database_connect.get_odbc_driver_paths(db_config['catalog_database_type'],
                        oracle_home=server_config['environment_variables']['ORACLE_HOME'] if db_config['catalog_database_type'] == 'oracle' else None))

        db_config['db_host'] = default_prompt(
            'Database server\'s hostname or IP address',
            default=[db_config.get('db_host', 'localhost')])

        db_config['db_port'] = default_prompt(
            'Database server\'s port',
            default=[db_config.get('db_port', 5432)])

        if db_config['catalog_database_type'] == 'oracle':
            db_config['db_name'] = default_prompt(
                'Service name',
                default=[db_config.get('db_name', 'ICAT.example.org')])
        else:
            db_config['db_name'] = default_prompt(
                'Database name',
                default=[db_config.get('db_name', 'ICAT')])

        db_config['db_username'] = default_prompt(
                'Database username',
                default=[db_config.get('db_username', 'irods')])

        confirmation_message = ''.join([
                '\n',
                '-------------------------------------------\n',
                'Database Type: %s\n',
                '$ORACLE_HOME:  %s\n' if db_config['catalog_database_type'] == 'oracle' else '%s',
                'ODBC Driver:   %s\n',
                'Database Host: %s\n',
                'Database Port: %d\n',
                'Database Name: %s\n' if db_config['catalog_database_type'] != 'oracle' else 'Service Name:  %s\n',
                'Database User: %s\n',
                '-------------------------------------------\n\n',
                'Please confirm']) % (
                    db_config['catalog_database_type'],
                    server_config['environment_variables']['ORACLE_HOME'] if db_config['catalog_database_type'] == 'oracle' else '',
                    db_config['db_odbc_driver'],
                    db_config['db_host'],
                    db_config['db_port'],
                    db_config['db_name'],
                    db_config['db_username'])

        if default_prompt(confirmation_message, default=['yes']) in ['', 'y', 'Y', 'yes', 'YES']:
            break

    db_config['db_password'] = prompt(
            'Database password',
            echo=False)

    irods_config.commit(db_config, irods_config.database_config_path)

    if db_config['catalog_database_type'] == 'oracle':
        irods_config.commit(server_config, irods_config.server_config_path)

    controller = IrodsController(irods_config)
    controller.stop()
    with contextlib.closing(irods.database_connect.get_connection(irods_config.database_config)) as connection:
        connection.autocommit = False
        cursor = connection.cursor()
        create_database_tables(irods_config, cursor, default_resource_directory=default_resource_directory)
        update_catalog_schema(irods_config, cursor)

    l.info('Testing database communications...');

    # Make sure communications are working.
    #       This simple test issues a few SQL statements
    #       to the database, testing that the connection
    #       works.  iRODS is uninvolved at this point.

    irods.lib.execute_command([os.path.join(irods_config.server_test_directory, 'test_cll')])

    setup_client_environment(irods_config)

def list_database_tables(irods_config, cursor=None):
    if cursor is None:
        with contextlib.closing(irods.database_connect.get_connection(irods_config.database_config)) as connection:
            connection.autocommit = False
            with contextlib.closing(connection.cursor()) as cursor:
                return list_database_tables(irods_config, cursor)
    l = logging.getLogger(__name__)
    l.info('Listing database tables...')
    tables = cursor.tables().fetchall()
    table_names = [row.table_name for row in tables]
    l.debug('List of tables:\n%s', pprint.pformat(table_names))
    return table_names

def create_database_tables(irods_config, cursor=None, default_resource_directory=None):
    if cursor is None:
        with contextlib.closing(irods.database_connect.get_connection(irods_config.database_config)) as connection:
            connection.autocommit = False
            with contextlib.closing(connection.cursor()) as cursor:
                create_database_tables(irods_config, cursor)
                return
    l = logging.getLogger(__name__)
    table_names = list_database_tables(irods_config, cursor)
    irods_table_names = [t for t in table_names if t.lower().startswith('r_')]
    if irods_table_names:
        l.info('The following tables already exist in the database, table creation will be skipped:\n%s', pprint.pformat(irods_table_names))
    else:
        l.info('Creating database tables...')
        sql_files = [os.path.join(irods_config.irods_directory, 'server', 'icat', 'src', 'icatSysTables.sql'),
                os.path.join(irods_config.irods_directory, 'server', 'icat', 'src', 'icatSysInserts.sql')
            ]
        for sql_file in sql_files:
            try:
                irods.database_connect.execute_sql_file(sql_file, cursor)
            except pypyodbc.Error as e:
                six.reraise(IrodsError,
                        IrodsError('Database setup failed while running %s' % (sql_file)),
                        sys.exc_info()[2])
        setup_database_values(irods_config, cursor, default_resource_directory=default_resource_directory)

def setup_database_values(irods_config, cursor=None, default_resource_directory=None):
    if cursor is None:
        with contextlib.closing(irods.database_connect.get_connection(irods_config.database_config)) as connection:
            connection.autocommit = False
            with contextlib.closing(connection.cursor()) as cursor:
                create_database_tables(irods_config, cursor)
                return
    l = logging.getLogger(__name__)
    timestamp = '{:011d}'.format(int(time.time()))

    #zone
    database_connect.execute_sql_statement(cursor,
            "insert into R_ZONE_MAIN values (9000,?,'local','','',?,?);",
            irods_config.server_config['zone_name'],
            timestamp,
            timestamp)

    #groups
    admin_group_id = 9001
    database_connect.execute_sql_statement(cursor,
            "insert into R_USER_MAIN values (?,?,?,?,'','',?,?);",
            admin_group_id,
            'rodsadmin',
            'rodsgroup',
            irods_config.server_config['zone_name'],
            timestamp,
            timestamp)

    public_group_id = 9002
    database_connect.execute_sql_statement(cursor,
            "insert into R_USER_MAIN values (?,?,?,?,'','',?,?);",
            admin_group_id,
            'public',
            'rodsgroup',
            irods_config.server_config['zone_name'],
            timestamp,
            timestamp)

    #users
    admin_user_id = 9010
    database_connect.execute_sql_statement(cursor,
            "insert into R_USER_MAIN values (?,?,?,?,'','',?,?);",
            admin_user_id,
            irods_config.server_config['zone_user'],
            'rodsadmin',
            irods_config.server_config['zone_name'],
            timestamp,
            timestamp)

    #group membership
    database_connect.execute_sql_statement(cursor,
            "insert into R_USER_GROUP values (?,?,?,?);",
            admin_group_id,
            admin_user_id,
            timestamp,
            timestamp)
    database_connect.execute_sql_statement(cursor,
            "insert into R_USER_GROUP values (?,?,?,?);",
            admin_user_id,
            admin_user_id,
            timestamp,
            timestamp)
    database_connect.execute_sql_statement(cursor,
            "insert into R_USER_GROUP values (?,?,?,?);",
            public_group_id,
            admin_user_id,
            timestamp,
            timestamp)

    #password
    database_connect.execute_sql_statement(cursor,
            "insert into R_USER_PASSWORD values (?,?,'9999-12-31-23.59.00',?,?);",
            admin_user_id,
            irods_config.admin_password,
            timestamp,
            timestamp,
            log_params=False)

    #collections
    system_collections = [
            '/',
            '/'.join(['', irods_config.server_config['zone_name']]),
            '/'.join(['', irods_config.server_config['zone_name'], 'home']),
            '/'.join(['', irods_config.server_config['zone_name'], 'trash']),
            '/'.join(['', irods_config.server_config['zone_name'], 'trash', 'home'])
        ]
    public_collections = [
            '/'.join(['', irods_config.server_config['zone_name'], 'home', 'public']),
            '/'.join(['', irods_config.server_config['zone_name'], 'trash', 'home', 'public'])
        ]
    admin_collections = [
            '/'.join(['', irods_config.server_config['zone_name'], 'home', irods_config.server_config['zone_user']]),
            '/'.join(['', irods_config.server_config['zone_name'], 'trash', 'home', irods_config.server_config['zone_user']])
        ]
    collection_id = 9020
    for collection in itertools.chain(system_collections, public_collections, admin_collections):
        parent_collection = '/'.join(['', collections[1:].rpartition('/')[0]])
        database_connect.execute_sql_statement(cursor,
                "insert into R_COLL_MAIN values (?,?,?,?,?,0,'','','','','','',?,?);",
                collection_id,
                parent_collection,
                collection,
                irods_config.server_config['zone_user'],
                irods_config.server_config['zone_name'],
                timestamp,
                timestamp)

        database_connect.execute_sql_statement(cursor,
                "insert into R_OBJT_ACCESS values (?,?,1200,?,?);",
                collection_id,
                public_group_id if collection in public_collections else admin_user_id,
                timestamp,
                timestamp)

        collection_id += 1

    #bundle resource
    database_connect.execute_sql_statement(cursor,
            "insert into R_RESC_MAIN (resc_id, resc_name, zone_name, resc_type_name, resc_class_name,  resc_net, resc_def_path, free_space, free_space_ts, resc_info, r_comment, resc_status, create_ts, modify_ts) values (9100, 'bundleResc', ?, 'unixfilesystem', 'bundle', 'localhost', '/bundle', '', '', '', '', '', ?,?);",
            irods_config.server_config['zone_name'],
            timestamp,
            timestamp)

    if default_resource_directory:
        database_connect.execute_sql_statement(cursor,
                "insert into R_RESC_MAIN (resc_id, resc_name, zone_name, resc_type_name, resc_class_name,  resc_net, resc_def_path, free_space, free_space_ts, resc_info, r_comment, resc_status, create_ts, modify_ts) values (9101, 'demoResc', ?, 'unixfilesystem', 'cache', ?, ?, '', '', '', '', '', ?,?);",
                irods_config.server_config['zone_name'],
                irods.lib.get_hostname(),
                default_resource_directory,
                timestamp,
                timestamp)

def update_catalog_schema(irods_config, cursor=None):
    if cursor is None:
        with contextlib.closing(irods.database_connect.get_connection(irods_config.database_config)) as connection:
            connection.autocommit = False
            with contextlib.closing(connection.cursor()) as cursor:
                update_catalog_schema(irods_config, cursor)
                return
    l = logging.getLogger(__name__)
    l.info('Updating schema version...')
    while irods_config.get_schema_version_in_database(cursor) != irods_config.version['catalog_schema_version']:
        schema_update_path = irods_config.get_next_schema_update_path()
        l.info('Running update to schema version %d...', int(os.path.basename(schema_update_path).partition('.')[0]))
        try:
            irods.database_connect.execute_sql_file(schema_update_path, cursor)
        except pypyodbc.Error as e:
            six.reraise(IrodsError,
                    IrodsError('Updating database schema version failed while running %s' % (schema_update_path)),
                    sys.exc_info()[2])

def setup_client_environment(irods_config):
    l = logging.getLogger(__name__)
    l.info('Setting up client environment...')

    print('\n', end='')

    irods_config.admin_password = prompt(
        'iRODS server\'s administrator password',
        input_filter=character_count_filter(minimum=1, field='Admin password'),
        echo=False)

    print('\n', end='')

    service_account_dict = {
            'irods_host': irods.lib.get_hostname(),
            'irods_port': irods_config.server_config['zone_port'],
            'irods_default_resource': irods_config.server_config['default_resource_name'],
            'irods_home': '/'.join(['', irods_config.server_config['zone_name'], 'home', irods_config.server_config['zone_user']]),
            'irods_cwd': '/'.join(['', irods_config.server_config['zone_name'], 'home', irods_config.server_config['zone_user']]),
            'irods_user_name': irods_config.server_config['zone_user'],
            'irods_zone_name': irods_config.server_config['zone_name'],
            'irods_client_server_negotiation': 'request_server_negotiation', #irods_config.server_config['negotiation_key'],
            'irods_client_server_policy': 'CS_NEG_REFUSE', #irods_config.server_config['match_hash_policy'],
            'irods_encryption_key_size': 32, #irods_config.server_config['encryption_key_size'],
            'irods_encryption_salt_size': 8, #irods_config.server_config['encryption_salt_size'],
            'irods_encryption_num_hash_rounds': 16, #irods_config.server_config['encryption_num_hash_rounds'],
            'irods_encryption_algorithm': 'AES-256-CBC', #irods_config.server_config['encryption_algorithm'],
            'irods_default_hash_scheme': irods_config.server_config['default_hash_scheme'],
            'irods_match_hash_policy': irods_config.server_config['match_hash_policy'],
            'irods_server_control_plane_port': irods_config.server_config['server_control_plane_port'],
            'irods_server_control_plane_key': irods_config.server_config['server_control_plane_key'],
            'irods_server_control_plane_encryption_num_hash_rounds': irods_config.server_config['server_control_plane_encryption_num_hash_rounds'],
            'irods_server_control_plane_encryption_algorithm': irods_config.server_config['server_control_plane_encryption_algorithm'],
            'irods_maximum_size_for_single_buffer_in_megabytes': irods_config.server_config['advanced_settings']['maximum_size_for_single_buffer_in_megabytes'],
            'irods_default_number_of_transfer_threads': irods_config.server_config['advanced_settings']['default_number_of_transfer_threads'],
            'irods_transfer_buffer_size_for_parallel_transfer_in_megabytes': irods_config.server_config['advanced_settings']['transfer_buffer_size_for_parallel_transfer_in_megabytes'],
        }
    if not os.path.exists(os.path.dirname(irods_config.client_environment_path)):
        os.makedirs(os.path.dirname(irods_config.client_environment_path), mode=0o700)
    irods_config.commit(service_account_dict, irods_config.client_environment_path)

def default_prompt(*args, **kwargs):
    l = logging.getLogger(__name__)
    default = kwargs.pop('default', [])
    input_filter = kwargs.pop('input_filter', lambda x: x)

    while True:
        if default:
            if len(default) == 1:
                message = ''.join([
                    args[0] % tuple(args[1:]),
                    ' [%s]' % (default[0])])
                user_input = prompt(message, **kwargs)
                if not user_input:
                    user_input = default[0]
            else:
                message = ''.join([
                    args[0] % tuple(args[1:]), ':\n',
                    '\n'.join(['%d. %s' % (i + 1, default[i]) for i in range(0, len(default))]),
                    '\nPlease select a number or choose 0 to enter a new value'])
                user_input = default_prompt(message, default=[1], **kwargs)
                try:
                    i = int(user_input) - 1
                except (TypeError, ValueError):
                    i = -1
                if i in range(0, len(default)):
                    user_input = default[i]
                else:
                    user_input = prompt('New value', **kwargs)
        else:
            message = args[0] % tuple(args[1:])
        try :
            return input_filter(user_input)
        except InputFilterError as e:
            l.debug(e)
            l.warning(e.args[0] if len(e.args) else "User input error.")
            user_input = prompt(message, **kwargs)

def prompt(*args, **kwargs):
    echo = kwargs.get('echo', True)
    end = kwargs.get('end', ': ')
    input_filter = kwargs.get('input_filter', lambda x: x)

    l = logging.getLogger(__name__)
    message = ''.join([args[0] % tuple(args[1:]), end])
    while True:
        l.debug(message)
        if echo:
            print(message, end='')
            sys.stdout.flush()
            user_input = sys.stdin.readline().rstrip('\n')
            l.debug('User input: %s', user_input)
        else:
            if sys.stdin.isatty():
                user_input = getpass.getpass(message)
            else:
                print('Warning: Cannot control echo output on the terminal (stdin is not a tty). Input may be echoed.', file=sys.stderr)
                user_input = sys.stdin.readline().rstrip('\n')
        try :
            return input_filter(user_input)
        except InputFilterError as e:
            l.debug(e)
            l.warning(e.args[0] if len(e.args) else "User input error.")

def int_filter(field='Input'):
    def f(x):
        try:
            return int(x)
        except ValueError as e:
            irods.six.reraise(InputFilterError, InputFilterError('%s must be an integer.' % (field)), sys.exc_info()[2])
    return f

def character_count_filter(minimum=None, maximum=None, field='Input'):
    def f(x):
        if (minimum is None or len(x) >= minimum) and (maximum is None or len(x) <= maximum):
            return x
        if minimum is not None and minimum < 0:
            new_minimum = 0
        else:
            new_minimum = minimum
        if new_minimum is not None and maximum is not None:
            if new_minimum == maximum:
                raise InputFilterError('%s must be exactly %s character%s in length.' % (field, maximum, '' if maximum == 1 else 's'))
            if new_minimum < maximum:
                raise InputFilterError('%s must be between %s and %s characters in length.' % (field, new_minimum, maximum))
            raise IrodsError('Minimum character count %s must not be greater than maximum character count %s.' % (new_minimum, maximum))
        if new_minimum is not None:
            raise InputFilterError('%s must be at least %s character%s in length.' % (field, new_minimum, '' if maximum == 1 else 's'))
        raise InputFilterError('%s may be at most %s character%s in length.' % (field, maximum, '' if maximum == 1 else 's'))
    return f

def add_options(parser):
    parser.add_option('-d', '--database_type',
                      dest='database_type', metavar='DB_TYPE',
                      help='The type of database to be used by the iRODS Catalog (ICAT) server. '
                      'Valid values are \'postgres\', \'mysql\', and \'oracle\'. '
                      'This option is required to set up an ICAT server and ignored for a resource server.')

    irods_control.add_options(parser)

def parse_options():
    parser = optparse.OptionParser()
    add_options(parser)

    return parser.parse_args()

def main():
    l = logging.getLogger(__name__)
    logging.getLogger().setLevel(logging.NOTSET)

    irods.log.register_tty_handler(sys.stderr, logging.WARNING, None)

    (options, _) = parse_options()

    irods_config = IrodsConfig(
        top_level_directory=options.top_level_directory,
        config_directory=options.config_directory)

    irods.log.register_file_handler(irods_config.control_log_path)
    if options.verbose:
        irods.log.register_tty_handler(sys.stdout, logging.INFO, logging.WARNING)

    if options.server_log_level != None:
        irods_config.injected_environment['spLogLevel'] = options.server_log_level
    if options.sql_log_level != None:
        irods_config.injected_environment['spLogSql'] = options.sql_log_level
    if options.days_per_log != None:
        irods_config.injected_environment['logfileInt'] = options.days_per_log
    if options.rule_engine_server_options != None:
        irods_config.injected_environment['reServerOption'] = options.rule_engine_server_options
    if options.server_reconnect_flag:
        irods_config.injected_environment['irodsReconnect'] = ''

    try:
        setup_server(irods_config)
    except IrodsError as e:
        l.error('Error encountered running setup_irods:\n', exc_info=True)
        l.info('Exiting...')
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
