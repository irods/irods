from __future__ import print_function

import contextlib
import logging
import os

from . import database_connect
from . import database_upgrade
from . import lib
from . import paths
from .exceptions import IrodsError, IrodsWarning

#These are the functions that must be implemented in order
#for the iRODS python scripts to communicate with the database

def setup_catalog(irods_config, default_resource_directory=None):
    l = logging.getLogger(__name__)

    with contextlib.closing(database_connect.get_database_connection(irods_config)) as connection:
        if irods_config.catalog_database_type == "cockroachdb":
            connection.autocommit = True
        else:
            connection.autocommit = False
        with contextlib.closing(connection.cursor()) as cursor:
            try:
                if database_connect.create_database_tables(irods_config, cursor) != 'skipped':
                    database_connect.setup_database_values(irods_config, cursor, default_resource_directory=default_resource_directory)
                    l.debug('Committing database changes...')
                cursor.commit()
            except:
                l.debug('Rolling back database changes...')
                cursor.rollback()
                raise

def server_launch_hook(irods_config):
    l = logging.getLogger(__name__)
    l.debug('Syncing .odbc.ini file...')
    def update_catalog_schema(irods_config, cursor):
        l = logging.getLogger(__name__)
        l.info('Ensuring catalog schema is up-to-date...')
        while True:
            schema_version_in_database = database_connect.get_schema_version_in_database(cursor)
            if schema_version_in_database == irods_config.version['catalog_schema_version']:
                l.info('Catalog schema is up-to-date.')
                return
            elif schema_version_in_database < irods_config.version['catalog_schema_version']:
                try:
                    database_upgrade.run_update(irods_config, cursor)
                    l.debug('Committing database changes...')
                    cursor.commit()
                except:
                    l.debug('Rolling back database changes...')
                    cursor.rollback()
                    raise
            elif schema_version_in_database > irods_config.version['catalog_schema_version']:
                raise IrodsError('Schema version in catalog (%d) is newer than schema version in the version file (%d); '
                                 'downgrading of catalog schema risks losing data and is unsupported.',
                                 schema_version_in_database, irods_config.version['catalog_schema_version'])

    database_connect.sync_odbc_ini(irods_config)

    if irods_config.catalog_database_type == 'oracle':
        two_task = database_connect.get_two_task_for_oracle(irods_config.database_config)
        l.debug('Setting TWO_TASK for oracle...')
        irods_config.execution_environment['TWO_TASK'] = two_task

    with contextlib.closing(database_connect.get_database_connection(irods_config)) as connection:
        connection.autocommit = False
        with contextlib.closing(connection.cursor()) as cursor:
            update_catalog_schema(irods_config, cursor)

def database_already_in_use_by_irods(irods_config):
    with contextlib.closing(database_connect.get_database_connection(irods_config)) as connection:
        with contextlib.closing(connection.cursor()) as cursor:
            if database_connect.irods_tables_in_database(irods_config, cursor):
                return True
            else:
                return False

def get_database_type():
    if os.path.exists(os.path.join(paths.plugins_directory(), 'database', 'libpostgres.so')):
        db_type = 'postgres'
    elif os.path.exists(os.path.join(paths.plugins_directory(), 'database', 'libcockroachdb.so')):
        db_type = 'cockroachdb'
    elif os.path.exists(os.path.join(paths.plugins_directory(), 'database', 'libmysql.so')):
        db_type = 'mysql'
    elif os.path.exists(os.path.join(paths.plugins_directory(), 'database', 'liboracle.so')):
        db_type = 'oracle'
    else:
        raise IrodsError('Database type must be one of postgres, cockroachdb, mysql, or oracle.')
    return db_type

def setup_database_config(irods_config):
    l = logging.getLogger(__name__)

    db_type = get_database_type()
    l.debug('setup_database_config has been called with database type \'%s\'.', db_type)

    l.info('You are configuring an iRODS database plugin. '
        'The iRODS server cannot be started until its database '
        'has been properly configured.\n'
        )

    for k in list(irods_config.server_config.setdefault('plugin_configuration', {}).setdefault('database', {})):
        if k != db_type:
            del irods_config.server_config['plugin_configuration']['database'][k]
    irods_config.server_config['plugin_configuration']['database'].setdefault(db_type, {})
    while True:
        odbc_drivers = database_connect.get_odbc_drivers_for_db_type(irods_config.catalog_database_type)
        if odbc_drivers:
            irods_config.database_config['db_odbc_driver'] = lib.default_prompt(
                'ODBC driver for %s', irods_config.catalog_database_type,
                default=odbc_drivers)
        else:
            irods_config.database_config['db_odbc_driver'] = lib.default_prompt(
                'No default ODBC drivers configured for %s; falling back to bare library paths', irods_config.catalog_database_type,
                default=database_connect.get_odbc_driver_paths(irods_config.catalog_database_type,
                    oracle_home=os.getenv('ORACLE_HOME', None)))

        irods_config.database_config['db_host'] = lib.default_prompt(
            'Database server\'s hostname or IP address',
            default=[irods_config.database_config.get('db_host', 'localhost')])

        irods_config.database_config['db_port'] = lib.default_prompt(
            'Database server\'s port',
            default=[irods_config.database_config.get('db_port', database_connect.get_default_port_for_database_type(irods_config.catalog_database_type))],
            input_filter=lib.int_filter(field='Port'))

        if irods_config.catalog_database_type == 'oracle':
            irods_config.database_config['db_name'] = lib.default_prompt(
                'Service name',
                default=[irods_config.database_config.get('db_name', 'ICAT.example.org')])
        else:
            irods_config.database_config['db_name'] = lib.default_prompt(
                'Database name',
                default=[irods_config.database_config.get('db_name', 'ICAT')])

        irods_config.database_config['db_username'] = lib.default_prompt(
                'Database username',
                default=[irods_config.database_config.get('db_username', 'irods')])

        if db_type == 'cockroachdb':
            irods_config.database_config['sslrootcert'] = lib.default_prompt(
                'Database Root SSL (CA) Cert file',
                 default=[irods_config.database_config.get('sslrootcert', '')])

        confirmation_message = ''.join([
                '\n',
                '-------------------------------------------\n',
                'Database Type: %s\n',
                'ODBC Driver:   %s\n',
                'Database Host: %s\n',
                'Database Port: %d\n',
                'Database Name: %s\n' if irods_config.catalog_database_type != 'oracle' else 'Service Name:  %s\n',
                'Database User: %s\n',
                'Database Cert: %s\n' if db_type == 'cockroachdb' else '%s',
                '-------------------------------------------\n\n',
                'Please confirm']) % (
                    irods_config.catalog_database_type,
                    irods_config.database_config['db_odbc_driver'],
                    irods_config.database_config['db_host'],
                    irods_config.database_config['db_port'],
                    irods_config.database_config['db_name'],
                    irods_config.database_config['db_username'],
                    irods_config.database_config['sslrootcert'] if db_type == 'cockroachdb' else '')

        if lib.default_prompt(confirmation_message, default=['yes']) in ['', 'y', 'Y', 'yes', 'YES']:
            break

    irods_config.database_config['db_password'] = lib.prompt(
            'Database password',
            echo=False)

    irods_config.commit(irods_config.server_config, irods_config.server_config_path)

    if database_already_in_use_by_irods(irods_config):
        l.warning(lib.get_header(
            'WARNING:\n'
            'The database specified is an already-configured\n'
            'iRODS database, so first-time database setup will\n'
            'not be performed. Providing different inputs from\n'
            'those provided the first time this script was run\n'
            'will result in unspecified behavior, and may put\n'
            'a previously working zone in a broken state. It\'s\n'
            'recommended that you exit this script now if you\n'
            'are running it manually. If you wish to wipe out\n'
            'your current iRODS installation and associated data\n'
            'catalog, drop the database and recreate it before\n'
            're-running this script.'))

    db_password_salt = lib.prompt(
            'Salt for passwords stored in the database',
            echo=False)
    if db_password_salt:
        if 'environment_variables' not in irods_config.server_config:
            irods_config.server_config['environment_variables'] = {}
        irods_config.server_config['environment_variables']['IRODS_DATABASE_USER_PASSWORD_SALT'] = db_password_salt

    irods_config.commit(irods_config.server_config, irods_config.server_config_path)
