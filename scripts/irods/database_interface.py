from __future__ import print_function

import contextlib
import logging
import os

from . import database_connect
from . import database_upgrade
from . import lib
from .exceptions import IrodsError, IrodsWarning

#These are the functions that must be implemented in order
#for the iRODS python scripts to communicate with the database

def setup_catalog(irods_config, default_resource_directory=None):
    l = logging.getLogger(__name__)

    with contextlib.closing(database_connect.get_database_connection(irods_config)) as connection:
        connection.autocommit = False
        with contextlib.closing(connection.cursor()) as cursor:
            try:
                database_connect.create_database_tables(irods_config, cursor)
                database_connect.setup_database_values(irods_config, cursor, default_resource_directory=default_resource_directory)
                l.debug('Committing database changes...')
                cursor.commit()
            except:
                l.debug('Rolling back database changes...')
                cursor.rollback()
                raise

def test_catalog(irods_config):
    l = logging.getLogger(__name__)
    # Make sure communications are working.
    #       This simple test issues a few SQL statements
    #       to the database, testing that the connection
    #       works.  iRODS is uninvolved at this point.

    l.info('Testing database communications...');
    lib.execute_command([os.path.join(irods_config.server_test_directory, 'test_cll')])

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

    if irods_config.database_config['catalog_database_type'] == 'oracle':
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
