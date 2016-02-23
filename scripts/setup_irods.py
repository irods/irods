#!/usr/bin/python
from __future__ import print_function

import os, sys

#for oracle, ORACLE_HOME must be in LD_LIBRARY_PATH
if 'ORACLE_HOME' in os.environ:
    oracle_lib_dir = os.path.join(os.environ['ORACLE_HOME'], 'lib')
    if oracle_lib_dir not in os.environ.get('LD_LIBRARY_PATH', ''):
        os.environ['LD_LIBRARY_PATH'] = ':'.join([e for e in [os.environ.get('LD_LIBRARY_PATH', ''), oracle_lib_dir] if e])
        os.execve(sys.argv[0], sys.argv, os.environ)

import contextlib
import copy
import getpass
import grp
import itertools
import json
import logging
import optparse
import pprint
import pwd
import stat
import time
import tempfile

from irods import six

import irods_control
import irods.database_connect
import irods.lib
import irods.password_obfuscation
from irods.configuration import IrodsConfig
from irods.controller import IrodsController
from irods.exceptions import IrodsError, IrodsWarning
import irods.log

class InputFilterError(Exception):
    pass

def setup_server(irods_config):
    l = logging.getLogger(__name__)

    check_hostname()

    l.info(get_header('Stopping iRODS...'))
    IrodsController(irods_config).stop()

    setup_service_account(irods_config)

    #Do the rest of the setup as the irods user
    irods.lib.switch_user(irods_config.irods_user, irods_config.irods_group)

    if irods_config.is_catalog:
        setup_database_config(irods_config)
    setup_server_config(irods_config)
    setup_client_environment(irods_config)

    if irods_config.is_catalog:
        default_resource_directory = get_and_create_default_vault(irods_config)
        setup_catalog(irods_config, default_resource_directory=default_resource_directory)

    l.info(get_header('Starting iRODS...'))
    IrodsController(irods_config).start()

    if irods_config.is_resource:
        default_resource_directory = get_and_create_default_vault(irods_config)
        irods.lib.execute_command(['iadmin', 'mkresc', irods_config.server_config['default_resource_name'], 'unixfilesystem', ':'.join([irods.lib.get_hostname(), default_resource_directory]), ''])

def check_hostname():
    l = logging.getLogger(__name__)

    try:
        irods.lib.get_hostname().index('.')
    except ValueError:
        l.warning('Warning: Hostname `%s` should be a fully qualified domain name.')

def get_and_create_default_vault(irods_config):
    l = logging.getLogger(__name__)
    l.info(get_header('Setting up default vault'))

    default_resource_directory = default_prompt(
        'iRODS Vault directory',
        default=[os.path.join(irods_config.irods_directory, 'Vault')])
    if not os.path.exists(default_resource_directory):
        os.makedirs(default_resource_directory, mode=0o700)

    return default_resource_directory

def setup_service_account(irods_config):
    l = logging.getLogger(__name__)
    l.info(get_header('Setting up the service account'))

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

    if irods.lib.execute_command_permissive(['id', irods_user])[2] != 0:
        l.info('Creating Service Account: %s', irods_group)
        irods.lib.execute_command([
            'useradd',
            '-r',
            '-d', irods_config.top_level_directory,
            '-M',
            '-s', '/bin/bash',
            '-g', irods_group,
            '-c', 'iRODS Administrator',
            '-p', '!',
            irods_user])
    else:
        l.info('Existing Account Detected: %s', irods_user)

    with open(irods_config.service_account_file_path, 'wt') as f:
        print('IRODS_SERVICE_ACCOUNT_NAME=%s', irods_user, file=f)
        print('IRODS_SERVICE_GROUP_NAME=%s', irods_group, file=f)

    l.info('Setting owner of %s to %s:%s',
            irods_config.top_level_directory, irods_user, irods_group)
    for (root, _, files) in os.walk(irods_config.top_level_directory):
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

    os.lchown(os.path.join(irods_config.server_bin_directory, 'PamAuthCheck'), 0, 0)
    os.chmod(os.path.join(irods_config.server_bin_directory, 'PamAuthCheck'), stat.S_ISUID | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)
    irods_config.clear_cache()

    return (irods_user, irods_group)

def setup_server_config(irods_config):
    l = logging.getLogger(__name__)
    l.info(get_header('Configuring the server options'))

    try :
        server_config = copy.deepcopy(irods_config.server_config)
    except (OSError, ValueError):
        server_config = {}

    while True:
        server_config['zone_name'] = default_prompt(
            'iRODS server\'s zone name',
            default=[server_config.get('zone_name', 'tempZone')],
            input_filter=character_count_filter(minimum=1, field='Zone name'))

        if irods_config.is_catalog:
            server_config['icat_host'] = irods.lib.get_hostname()
        elif irods_config.is_resource:
            server_config['icat_host'] = prompt(
                'iRODS catalog (ICAT) host',
                input_filter=character_count_filter(minimum=1, field='iRODS catalog hostname'))

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

        server_config['server_control_plane_port'] = default_prompt(
            'Control Plane port',
            default=[server_config.get('server_control_plane_port', 1248)],
            input_filter=int_filter(field='Port'))

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
                'iRODS catalog host:         %s\n' if irods_config.is_resource else '%s',
                'iRODS server port:          %d\n',
                'iRODS port range (begin):   %d\n',
                'iRODS port range (end):     %d\n',
                'Control plane port:         %d\n',
                'Schema validation base URI: %s\n',
                'iRODS server administrator: %s\n',
                '-------------------------------------------\n\n',
                'Please confirm']) % (
                    server_config['zone_name'],
                    server_config['icat_host'] if irods_config.is_resource else '',
                    server_config['zone_port'],
                    server_config['server_port_range_start'],
                    server_config['server_port_range_end'],
                    server_config['server_control_plane_port'],
                    server_config['schema_validation_base_uri'],
                    server_config['zone_user']
                    )

        if default_prompt(confirmation_message, default=['yes']) in ['', 'y', 'Y', 'yes', 'YES']:
            break

    server_config['zone_key'] = prompt(
        'iRODS server\'s zone key',
        input_filter=character_count_filter(minimum=1, field='Zone key'),
        echo=False)

    server_config['negotiation_key'] = prompt(
        'iRODS server\'s negotiation key',
        input_filter=character_count_filter(minimum=32, maximum=32, field='Negotiation key'),
        echo=False)

    server_config['server_control_plane_key'] = prompt(
        'Control Plane key',
        input_filter=character_count_filter(minimum=32, maximum=32, field='Control Plane key'),
        echo=False)

    if irods_config.is_resource:
        server_config['default_resource_name'] = ''.join([irods.lib.get_hostname().split('.')[0], 'Resource'])

    irods_config.commit(server_config, irods_config.server_config_path)

def setup_database_config(irods_config):
    l = logging.getLogger(__name__)
    l.info(get_header('Configuring the database communications'))

    if os.path.exists(os.path.join(irods_config.top_level_directory, 'plugins', 'database', 'libpostgres.so')):
        db_type = 'postgres'
    elif os.path.exists(os.path.join(irods_config.top_level_directory, 'plugins', 'database', 'libmysql.so')):
        db_type = 'mysql'
    elif os.path.exists(os.path.join(irods_config.top_level_directory, 'plugins', 'database', 'liboracle.so')):
        db_type = 'oracle'
    else:
        raise IrodsError('Database type must be one of postgres, mysql, or oracle.')
    l.debug('setup_database_config has been called with database type \'%s\'.', db_type)

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
        odbc_drivers = irods.database_connect.get_odbc_drivers_for_db_type(db_config['catalog_database_type'])
        if odbc_drivers:
            db_config['db_odbc_driver'] = default_prompt(
                'ODBC driver for %s', db_config['catalog_database_type'],
                default=odbc_drivers)
        else:
            db_config['db_odbc_driver'] = default_prompt(
                'No default ODBC drivers configured for %s; falling back to bare library paths', db_config['catalog_database_type'],
                default=irods.database_connect.get_odbc_driver_paths(db_config['catalog_database_type'],
                    oracle_home=os.getenv('ORACLE_HOME', None)))

        db_config['db_host'] = default_prompt(
            'Database server\'s hostname or IP address',
            default=[db_config.get('db_host', 'localhost')])

        db_config['db_port'] = default_prompt(
            'Database server\'s port',
            default=[db_config.get('db_port', 5432)],
            input_filter=int_filter(field='Port'))

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
                'ODBC Driver:   %s\n',
                'Database Host: %s\n',
                'Database Port: %d\n',
                'Database Name: %s\n' if db_config['catalog_database_type'] != 'oracle' else 'Service Name:  %s\n',
                'Database User: %s\n',
                '-------------------------------------------\n\n',
                'Please confirm']) % (
                    db_config['catalog_database_type'],
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

    if irods_config.irods_tables_in_database():
        l.warning(get_header(
            'WARNING:\n'
            'The database specified is an already-configured\n'
            'iRODS database, so first-time database setup will\n'
            'not be performed. Providing different inputs from\n'
            'those provided the first time this script was run\n'
            'will result in unspecified behavior, and may put\n'
            'a previously working grid in a broken state. It\'s\n'
            'recommended that you exit this script now if you\n'
            'are running it manually. If you wish to wipe out\n'
            'your current iRODS installation and associated data\n'
            'catalog, drop the database and recreate it before\n'
            're-running this script.'))

    db_password_salt = prompt(
            'Salt for passwords stored in the database',
            echo=False)
    if db_password_salt:
        if 'environment_variables' not in server_config:
            server_config['environment_variables'] = {}
        server_config['environment_variables']['IRODS_DATABASE_USER_PASSWORD_SALT'] = db_password_salt

    irods_config.commit(server_config, irods_config.server_config_path)

def setup_catalog(irods_config, default_resource_directory=None):
    l = logging.getLogger(__name__)
    l.info(get_header('Setting up the database'))

    with contextlib.closing(irods_config.get_database_connection()) as connection:
        connection.autocommit = False
        with contextlib.closing(connection.cursor()) as cursor:
            try:
                create_database_tables(irods_config, cursor, default_resource_directory=default_resource_directory)
                irods_config.update_catalog_schema(cursor)
                l.debug('Committing database changes...')
                cursor.commit()
            finally:
                cursor.rollback()

    # Make sure communications are working.
    #       This simple test issues a few SQL statements
    #       to the database, testing that the connection
    #       works.  iRODS is uninvolved at this point.

    l.info('Testing database communications...');
    irods.lib.execute_command([os.path.join(irods_config.server_test_directory, 'test_cll')])

def create_database_tables(irods_config, cursor=None, default_resource_directory=None):
    if cursor is None:
        with contextlib.closing(irods_config.get_database_connection()) as connection:
            with contextlib.closing(connection.cursor()) as cursor:
                create_database_tables(irods_config, cursor)
                return
    l = logging.getLogger(__name__)
    irods_table_names = irods_config.irods_tables_in_database()
    if irods_table_names:
        l.info('The following tables already exist in the database, table creation will be skipped:\n%s', pprint.pformat(irods_table_names))
    else:
        if irods_config.database_config['catalog_database_type'] == 'mysql':
            l.info('Defining mysql functions...')
            with tempfile.NamedTemporaryFile() as f:
                f.write('\n'.join([
                        '[client]',
                        '='.join(['user', irods_config.database_config['db_username']]),
                        '='.join(['password', irods_config.database_config['db_password']]),
                        '='.join(['port', str(irods_config.database_config['db_port'])]),
                        '='.join(['host', irods_config.database_config['db_host']])
                    ]))
                f.flush()
                with open(os.path.join(irods_config.irods_directory, 'server', 'icat', 'src', 'mysql_functions.sql'), 'r') as sql_file:
                    irods.lib.execute_command(
                        ['mysql', '='.join(['--defaults-file', f.name]), irods_config.database_config['db_name']],
                        stdin=sql_file)
        l.info('Creating database tables...')
        sql_files = [os.path.join(irods_config.irods_directory, 'server', 'icat', 'src', 'icatSysTables.sql'),
                os.path.join(irods_config.irods_directory, 'server', 'icat', 'src', 'icatSysInserts.sql')
            ]
        for sql_file in sql_files:
            try:
                irods.database_connect.execute_sql_file(sql_file, cursor, by_line=True)
            except IrodsError:
                six.reraise(IrodsError,
                        IrodsError('Database setup failed while running %s' % (sql_file)),
                        sys.exc_info()[2])
        setup_database_values(irods_config, cursor, default_resource_directory=default_resource_directory)

def setup_database_values(irods_config, cursor=None, default_resource_directory=None):
    if cursor is None:
        with contextlib.closing(irods_config.get_database_connection()) as connection:
            with contextlib.closing(connection.cursor()) as cursor:
                create_database_tables(irods_config, cursor)
                return
    l = logging.getLogger(__name__)
    timestamp = '{0:011d}'.format(int(time.time()))

    def get_next_object_id():
        if irods_config.database_config['catalog_database_type'] == 'postgres':
            return irods.database_connect.execute_sql_statement(cursor, "select nextval('R_OBJECTID');").fetchone()[0]
        elif irods_config.database_config['catalog_database_type'] == 'mysql':
            return irods.database_connect.execute_sql_statement(cursor, "select R_OBJECTID_nextval();").fetchone()[0]
        elif irods_config.database_config['catalog_database_type'] == 'oracle':
            return irods.database_connect.execute_sql_statement(cursor, "select R_OBJECTID.nextval from DUAL;").fetchone()[0]

    #zone
    zone_id = get_next_object_id()
    irods.database_connect.execute_sql_statement(cursor,
            "insert into R_ZONE_MAIN values (?,?,'local','','',?,?);",
            zone_id,
            irods_config.server_config['zone_name'],
            timestamp,
            timestamp)

    #groups
    admin_group_id = get_next_object_id()
    irods.database_connect.execute_sql_statement(cursor,
            "insert into R_USER_MAIN values (?,?,?,?,'','',?,?);",
            admin_group_id,
            'rodsadmin',
            'rodsgroup',
            irods_config.server_config['zone_name'],
            timestamp,
            timestamp)

    public_group_id = get_next_object_id()
    irods.database_connect.execute_sql_statement(cursor,
            "insert into R_USER_MAIN values (?,?,?,?,'','',?,?);",
            public_group_id,
            'public',
            'rodsgroup',
            irods_config.server_config['zone_name'],
            timestamp,
            timestamp)

    #users
    admin_user_id = get_next_object_id()
    irods.database_connect.execute_sql_statement(cursor,
            "insert into R_USER_MAIN values (?,?,?,?,'','',?,?);",
            admin_user_id,
            irods_config.server_config['zone_user'],
            'rodsadmin',
            irods_config.server_config['zone_name'],
            timestamp,
            timestamp)

    #group membership
    irods.database_connect.execute_sql_statement(cursor,
            "insert into R_USER_GROUP values (?,?,?,?);",
            admin_group_id,
            admin_user_id,
            timestamp,
            timestamp)
    irods.database_connect.execute_sql_statement(cursor,
            "insert into R_USER_GROUP values (?,?,?,?);",
            admin_user_id,
            admin_user_id,
            timestamp,
            timestamp)
    irods.database_connect.execute_sql_statement(cursor,
            "insert into R_USER_GROUP values (?,?,?,?);",
            public_group_id,
            admin_user_id,
            timestamp,
            timestamp)

    #password
    scrambled_password = irods.password_obfuscation.scramble(irods_config.admin_password,
            key=irods_config.server_config.get('environment_variables', {}).get('IRODS_DATABASE_USER_PASSWORD_SALT', None))
    irods.database_connect.execute_sql_statement(cursor,
            "insert into R_USER_PASSWORD values (?,?,'9999-12-31-23.59.00',?,?);",
            admin_user_id,
            scrambled_password,
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
    for collection in itertools.chain(system_collections, public_collections, admin_collections):
        parent_collection = '/'.join(['', collection[1:].rpartition('/')[0]])
        collection_id = get_next_object_id()
        irods.database_connect.execute_sql_statement(cursor,
                "insert into R_COLL_MAIN values (?,?,?,?,?,0,'','','','','','',?,?);",
                collection_id,
                parent_collection,
                collection,
                irods_config.server_config['zone_user'],
                irods_config.server_config['zone_name'],
                timestamp,
                timestamp)

        irods.database_connect.execute_sql_statement(cursor,
                "insert into R_OBJT_ACCESS values (?,?,1200,?,?);",
                collection_id,
                public_group_id if collection in public_collections else admin_user_id,
                timestamp,
                timestamp)

    #bundle resource
    bundle_resc_id = get_next_object_id()
    irods.database_connect.execute_sql_statement(cursor,
            "insert into R_RESC_MAIN (resc_id,resc_name,zone_name,resc_type_name,resc_class_name,resc_net,resc_def_path,free_space,free_space_ts,resc_info,r_comment,resc_status,create_ts,modify_ts) values (?,'bundleResc',?,'unixfilesystem','bundle','localhost','/bundle','','','','','',?,?);",
            bundle_resc_id,
            irods_config.server_config['zone_name'],
            timestamp,
            timestamp)

    if default_resource_directory:
        default_resc_id = get_next_object_id()
        irods.database_connect.execute_sql_statement(cursor,
                "insert into R_RESC_MAIN (resc_id,resc_name,zone_name,resc_type_name,resc_class_name,resc_net,resc_def_path,free_space,free_space_ts,resc_info,r_comment,resc_status,create_ts,modify_ts) values (?,?,?,'unixfilesystem','cache',?,?,'','','','','',?,?);",
                default_resc_id,
                'demoResc',
                irods_config.server_config['zone_name'],
                irods.lib.get_hostname(),
                default_resource_directory,
                timestamp,
                timestamp)

def setup_client_environment(irods_config):
    l = logging.getLogger(__name__)
    l.info(get_header('Setting up the client environment'))

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

def get_header(message):
    lines = [l.strip() for l in message.splitlines()]
    length = 0
    for line in lines:
        length = max(length, len(line))
    edge = '+' + '-' * (length + 2) + '+'
    format_string = '{0:<' + str(length) + '}'
    header_lines = ['', edge]
    for line in lines:
        header_lines.append('| ' + format_string.format(line) + ' |')
    header_lines.append(edge)
    header_lines.append('')
    return '\n'.join(header_lines)

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
            user_input = prompt(*args, **kwargs)
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
        if not sys.stdin.isatty():
            print('\n', end='')
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

    irods.log.register_file_handler(irods_config.setup_log_path)
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
