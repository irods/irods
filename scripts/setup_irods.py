#!/usr/bin/python
from __future__ import print_function
import contextlib
import copy
import getpass
import itertools
import json
import logging
import optparse
import os
import pprint
import sys
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

def setup_resource(irods_config):
    pass

def setup_catalog(db_type, irods_config):
    l = logging.getLogger(__name__)
    if db_type is None:
        raise IrodsError('Database type is required to set up an iRODS catalog (ICAT) server.')
    l.debug('setup_catalog has been called with database type \'%s\'.', db_type)
    if db_type not in ['postgres', 'mysql', 'oracle']:
        raise IrodsError('Database type must be one of postgres, mysql, or oracle.')

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
                    default=irods.database_connect.get_odbc_driver_paths(db_config['catalog_database_type']))

        if db_config['catalog_database_type'] == 'oracle':
            oracle_home = ''
            if 'environment_variables' not in server_config:
                server_config['environment_variables'] = {}
            if 'ORACLE_HOME' in server_config['environment_variables']:
                oracle_home = server_config['environment_variables']['ORACLE_HOME']
            elif 'ORACLE_HOME' in os.environ:
                oracle_home = os.environ['ORACLE_HOME']
            server_config['environment_variables']['ORACLE_HOME'] = default_prompt('$ORACLE_HOME', default=[oracle_home])

        db_config['db_host'] = default_prompt(
                'Database server\'s hostname or IP address',
                default=[db_config['db_host']] if 'db_host' in db_config else ['localhost'])

        while True:
            try:
                db_config['db_port'] = int(default_prompt(
                        'Database server\'s port',
                        default=[db_config['db_port']] if 'db_port' in db_config else ['5432']))
                break
            except ValueError:
                l.warning('Port must be an integer.')

        if db_config['catalog_database_type'] == 'oracle':
            db_config['db_name'] = default_prompt(
                    'Service name',
                    default=[db_config['db_name']] if 'db_name' in db_config else [])
        else:
            db_config['db_name'] = default_prompt(
                    'Database name',
                    default=[db_config['db_name']] if 'db_name' in db_config else ['ICAT'])

        db_config['db_username'] = default_prompt(
                'Database username',
                default=[db_config['db_username']] if 'db_username' in db_config else ['irods'])

        confirmation_message = ''.join([
                '-------------------------------------------\n'
                'Database Type: %s\n',
                'ODBC Driver:   %s\n',
                '$ORACLE_HOME:  %s\n' if db_config['catalog_database_type'] == 'oracle' else '%s',
                'Database Host: %s\n'
                'Database Port: %d\n',
                'Database Name: %s\n' if db_config['catalog_database_type'] != 'oracle' else 'Service Name:  %s\n',
                'Database User: %s\n'
                '-------------------------------------------\n\n'
                'Please confirm']) % (
                    db_config['catalog_database_type'],
                    db_config['db_odbc_driver'],
                    server_config['environment_variables']['ORACLE_HOME'] if db_config['catalog_database_type'] == 'oracle' else '',
                    db_config['db_host'],
                    db_config['db_port'],
                    db_config['db_name'],
                    db_config['db_username'])

        if default_prompt(confirmation_message, default=['yes']) in ['', 'y', 'Y', 'yes', 'YES']:
            break

    db_config['db_password'] = default_prompt(
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
        create_database_tables(irods_config, cursor)
        update_catalog_schema(irods_config, cursor)

    #update the catalog yeeeeeah

def list_database_tables(irods_config, cursor=None):
    if cursor is None:
        with contextlib.closing(irods.database_connect.get_connection(irods_config.database_config)) as connection:
            connection.autocommit = False
            with contextlib.closing(connection.cursor()) as cursor:
                return list_database_tables(irods_config, cursor)
    else:
        l = logging.getLogger(__name__)
        l.info('Listing database tables...')
        tables = cursor.tables().fetchall()
        table_names = [row.table_name for row in tables]
        l.debug('List of tables:\n%s', pprint.pformat(table_names))
        return table_names


def create_database_tables(irods_config, cursor=None):
    if cursor is None:
        with contextlib.closing(irods.database_connect.get_connection(irods_config.database_config)) as connection:
            connection.autocommit = False
            with contextlib.closing(connection.cursor()) as cursor:
                create_database_tables(irods_config, cursor)
    else:
        l = logging.getLogger(__name__)
        table_names = list_database_tables(irods_config, cursor)
        irods_table_names = [t for t in table_names if t.lower().startswith('r_')]
        if irods_table_names:
            l.info('The following tables already exist in the database, table creation will be skipped:\n%s', pprint.pformat(irods_table_names))
        else:
            l.info('Creating database tables...')
            sql_files = [os.path.join(irods_config.irods_directory, 'server', 'icat', 'src', 'icatSysTables.sql'),
                    os.path.join(irods_config.irods_directory, 'server', 'icat', 'src', 'icatSysInserts.sql'),
                    os.path.join(irods_config.irods_directory, 'server', 'icat', 'src', 'icatSetupValues.sql')
                ]
            for sql_file in sql_files:
                try:
                    irods.database_connect.execute_sql_file(sql_file, cursor)
                except pypyodbc.Error as e:
                    six.reraise(IrodsError,
                            IrodsError('Database setup failed while running %s' % (sql_file)),
                            sys.exc_info()[2])



def update_catalog_schema(irods_config, cursor=None):
    if cursor is None:
        with contextlib.closing(irods.database_connect.get_connection(irods_config.database_config)) as connection:
            connection.autocommit = False
            with contextlib.closing(connection.cursor()) as cursor:
                update_catalog_schema(irods_config, cursor)
    else:
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

def default_prompt(*args, **kwargs):
    l = logging.getLogger(__name__)
    if 'default' in kwargs:
        default = kwargs['default']
        del kwargs['default']
    else:
        default = []

    if default:
        if len(default) == 1:
            message = ''.join([
                args[0] % tuple(args[1:]),
                ' [%s]: ' % (default[0])])
            user_input = prompt(message, **kwargs)
            if user_input:
                return user_input
            else:
                return default[0]
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
                return default[i]
            else:
                message = 'New value: '
    else:
        message = ''.join([
            args[0] % tuple(args[1:]),
            ': '])
    return prompt(message, **kwargs)

def prompt(*args, **kwargs):
    l = logging.getLogger(__name__)
    if 'echo' in kwargs:
        echo = kwargs['echo']
    else:
        echo = True
    message = args[0] % tuple(args[1:])
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

    return user_input

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

    operations_dict = {}
    operations_dict['resource'] = lambda: setup_resource(
        irods_config)
    operations_dict['catalog'] = lambda: setup_catalog(
        options.database_type, irods_config)
    operations_dict['update_catalog_schema'] = lambda: update_catalog_schema(
        irods_config)

    (options, arguments) = parse_options()
    if len(arguments) != 1:
        l.error(
            'irods_setup accepts exactly one positional argument, '
            'but {0} were provided.'.format(len(arguments)))
        l.error('Exiting...')
        return 1

    operation = arguments[0]
    if operation not in operations_dict:
        l.error('irods_setup accepts the following positional arguments:')
        l.error(irods.lib.indent(*operations_dict.keys()))
        l.error('but \'%s\' was provided.', operation)
        l.error('Exiting...')
        return 1

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
        operations_dict[operation]()
    except IrodsError as e:
        l.error('Error encountered running setup_irods %s:\n'
                % (operation), exc_info=True)
        l.info('Exiting...')
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
