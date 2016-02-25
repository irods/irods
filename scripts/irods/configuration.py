from __future__ import print_function
import contextlib
import itertools
import json
import logging
import os
import pprint
import sys
import time

from . import six

from . import database_connect
from .exceptions import IrodsError, IrodsWarning
from . import lib
from . import json_validation
from .password_obfuscation import encode, decode
from .paths import IrodsPaths

class IrodsConfig(IrodsPaths):
    def __init__(self,
                 top_level_directory=None,
                 config_directory=None,
                 injected_environment={},
                 insert_behavior=True):
        super(IrodsConfig, self).__init__(top_level_directory, config_directory)

        self._injected_environment = lib.callback_on_change_dict(self.clear_cache, injected_environment)
        self._insert_behavior = insert_behavior
        self.clear_cache()

    @property
    def version_tuple(self):
        if os.path.exists(self.version_path):
            return tuple(map(int, self.version['irods_version'].split('.')))

        legacy_version_file_path = os.path.join(self.top_level_directory, 'VERSION')
        if os.path.exists(legacy_version_file_path):
            with open(legacy_version_file_path) as f:
                for line in f:
                    key, _, value = line.strip().partition('=')
                    if key == 'IRODSVERSION':
                        return tuple(map(int, value.split('.')))

        raise IrodsError('Unable to determine iRODS version')

    @property
    def server_config(self):
        if self._server_config is None:
            self._server_config = load_json_config(self.server_config_path)
        return self._server_config

    @property
    def database_config(self):
        if self._database_config is None:
            self._database_config = load_json_config(self.database_config_path)
            if not 'db_odbc_driver' in self._database_config.keys():
                l = logging.getLogger(__name__)
                l.debug('No driver found in the database config, attempting to retrieve the one in the odbc ini file at "%s"...', self.odbc_ini_path)
                if os.path.exists(self.odbc_ini_path):
                    with open(self.odbc_ini_path) as f:
                        odbc_ini_contents = database_connect.load_odbc_ini(f)
                else:
                    l.debug('No odbc.ini file present')
                    odbc_ini_contents = {}
                if self._database_config['catalog_database_type'] in odbc_ini_contents.keys() and 'Driver' in odbc_ini_contents[self._database_config['catalog_database_type']].keys():
                    self._database_config['db_odbc_driver'] = odbc_ini_contents[self._database_config['catalog_database_type']]['Driver']
                    l.debug('Adding driver "%s" to database_config', self._database_config['db_odbc_driver'])
                    self.commit(self._database_config, self.database_config_path, clear_cache=False)
                else:
                    l.debug('Unable to retrieve "Driver" field from odbc ini file')

        return self._database_config

    @property
    def version(self):
        if self._version is None:
            self._version = load_json_config(self.version_path)
        return self._version

    @property
    def hosts_config(self):
        if self._hosts_config is None:
            self._hosts_config = load_json_config(self.hosts_config_path)
        return self._hosts_config

    @property
    def host_access_control_config(self):
        if self._host_access_control_config is None:
            self._host_access_control_config = load_json_config(self.host_access_control_config_path)
        return self._host_access_control_config

    @property
    def client_environment_path(self):
        if 'IRODS_ENVIRONMENT_FILE' in self.execution_environment:
            return self.execution_environment['IRODS_ENVIRONMENT_FILE']
        else:
            return os.path.join(
                self.home_directory,
                '.irods',
                'irods_environment.json')

    @property
    def client_environment(self):
        if self._client_environment is None:
            self._client_environment = load_json_config(self.client_environment_path)
        return self._client_environment

    @property
    def server_environment(self):
        return self.server_config.get('environment_variables', {})

    @property
    def execution_environment(self):
        if self._execution_environment is None:
            if self.insert_behavior:
                self._execution_environment = dict(self.server_environment)
                self._execution_environment.update(os.environ)
                self._execution_environment['irodsConfigDir'] = self.config_directory
                self._execution_environment['PWD'] = self.server_bin_directory
                self._execution_environment.update(self.injected_environment)
            else:
                self._execution_environment = dict(self.injected_environment)
        return self._execution_environment

    @property
    def insert_behavior(self):
        return self._insert_behavior

    @insert_behavior.setter
    def insert_behavior(self, value):
        self._insert_behavior = value
        self.clear_cache()

    @property
    def injected_environment(self):
        return self._injected_environment

    @injected_environment.setter
    def injected_environment(self, value):
        self._injected_environment = lib.callback_on_change_dict(self.clear_cache, value if value is not None else {})
        self.clear_cache()

    @property
    def schema_uri_prefix(self):
        if self._schema_uri_prefix is None:
            l = logging.getLogger(__name__)
            l.debug('Attempting to construct schema URI...')
            try:
                key = 'schema_validation_base_uri'
                base_uri = self.server_config[key]
            except KeyError:
                base_uri = None
                raise IrodsWarning(
                        '%s did not contain \'%s\'' %
                        (self.server_config_path, key))

            try:
                key = 'configuration_schema_version'
                uri_version = self.version[key]
            except KeyError:
                uri_version = None
                raise IrodsWarning(
                        '%s did not contain \'%s\'' %
                        (self.version_path, key))

            self._schema_uri_prefix = '/'.join([
                    base_uri,
                    'v%s' % (uri_version)])
            l.debug('Successfully constructed schema URI.')
        return self._schema_uri_prefix

    def list_database_tables(self, cursor=None):
        if cursor is None:
            with contextlib.closing(self.get_database_connection()) as connection:
                with contextlib.closing(connection.cursor()) as cursor:
                    return self.list_database_tables(cursor)
        l = logging.getLogger(__name__)
        l.info('Listing database tables...')
        table_names = [row[2] for row in cursor.tables()]
        l.debug('List of tables:\n%s', pprint.pformat(table_names))
        return table_names

    def irods_tables_in_database(self, cursor=None):
        with open(os.path.join(self.irods_directory, 'server', 'icat', 'src', 'icatSysTables.sql')) as f:
            irods_tables = [l.split()[2].lower() for l in f.readlines() if l.lower().startswith('create table')]
        table_names = self.list_database_tables(cursor)
        return [t for t in table_names if t.lower() in irods_tables]

    def update_catalog_schema(self, cursor=None):
        if cursor is None:
            with contextlib.closing(self.get_database_connection()) as connection:
                connection.autocommit = False
                with contextlib.closing(connection.cursor()) as cursor:
                    self.update_catalog_schema(cursor)
                    return
        l = logging.getLogger(__name__)
        l.info('Updating schema version...')
        while self.get_schema_version_in_database(cursor) != self.version['catalog_schema_version']:
            schema_update_path = self.get_next_schema_update_path(cursor)
            l.info('Running update to schema version %d...', int(os.path.basename(schema_update_path).partition('.')[0]))
            try:
                database_connect.execute_sql_file(schema_update_path, cursor, by_line=True)
            except IrodsError:
                six.reraise(IrodsError,
                        IrodsError('Updating database schema version failed while running %s' % (schema_update_path)),
                        sys.exc_info()[2])

    @property
    def admin_password(self):
        if not os.path.exists(os.path.dirname(self.password_file_path)):
            return None
        with open(self.password_file_path, 'rt') as f:
            return decode(f.read())

    @admin_password.setter
    def admin_password(self, value):
        if not os.path.exists(os.path.dirname(self.password_file_path)):
            os.makedirs(os.path.dirname(self.password_file_path), mode=0o700)
        mtime = int(time.time())
        with open(self.password_file_path, 'wt') as f:
            print(encode(value, mtime=mtime), end='', file=f)
        os.utime(self.password_file_path, (mtime, mtime))

    def get_database_connection(self):
        return database_connect.get_database_connection(self)

    def sync_odbc_ini(self):
        odbc_dict = database_connect.get_odbc_entry(self.database_config)

        #The 'Driver' keyword must be first
        keys = [k for k in odbc_dict.keys()]
        keys[keys.index('Driver')] = keys[0]
        keys[0] = 'Driver'

        template = '\n'.join(itertools.chain(['[iRODS Catalog]'], ['%s=%s' % (k, odbc_dict[k]) for k in keys]))
        lib.execute_command(['odbcinst', '-i', '-s', '-h', '-r'],
                input=template,
                env={'ODBCINI': self.odbc_ini_path, 'ODBCSYSINI': '/etc'})

    def get_next_schema_update_path(self, cursor=None):
        if not self.is_catalog:
            return None

        l = logging.getLogger(__name__)
        database_schema_version = self.get_schema_version_in_database(cursor)
        if database_schema_version is not None:
            return os.path.join(
                    self.database_schema_update_directory,
                    '%d.%s.sql' % (
                        database_schema_version + 1,
                        self.database_config['catalog_database_type']))
        return None

    def get_schema_version_in_database(self, cursor=None):
        if not self.is_catalog:
            return None

        if cursor is None:
            with contextlib.closing(self.get_database_connection()) as connection:
                with contextlib.closing(connection.cursor()) as cursor:
                    return self.get_schema_version_in_database(cursor)
        l = logging.getLogger(__name__)
        query = "select option_value from R_GRID_CONFIGURATION where namespace='database' and option_name='schema_version';"
        try :
            rows = database_connect.execute_sql_statement(cursor, query).fetchall()
        except IrodsError:
            six.reraise(IrodsError,
                IrodsError('pypyodbc encountered an error executing '
                    'the query:\n\t%s' % (query)),
                sys.exc_info()[2])
        if len(rows) == 0:
            raise IrodsError('No schema version present, unable to upgrade. '
                    'If this is an upgrade from a pre-4.0 installation, '
                    'a manual upgrade is required.')
        if len(rows) > 1:
            raise IrodsError('Expected one row when querying '
                'for database schema version, received %d rows' % (len(rows)))

        try:
            schema_version = int(rows[0][0])
        except ValueError:
            raise RuntimeError(
                'Failed to convert [%s] to an int for database schema version' % (rows[0][0]))
        l.debug('Schema_version in database: %s' % (schema_version))

        return schema_version

    def validate_configuration(self):
        l = logging.getLogger(__name__)

        configuration_schema_mapping = {
                'server_config': {
                    'dict': self.server_config,
                    'path': self.server_config_path},
                'VERSION': {
                    'dict': self.version,
                    'path': self.version_path},
                'hosts_config': {
                    'dict': self.hosts_config,
                    'path': self.hosts_config_path},
                'host_access_control_config': {
                    'dict': self.host_access_control_config,
                    'path': self.host_access_control_config_path},
                'irods_environment': {
                    'dict': self.client_environment,
                    'path': self.client_environment_path}}

        if os.path.exists(self.database_config_path):
            configuration_schema_mapping['database_config'] = {
                    'dict': self.database_config,
                    'path': self.database_config_path}
        else:
            l.debug('The database config file, \'%s\', does not exist.', self.database_config_path)

        skipped = []

        for schema_uri_suffix, config_file in configuration_schema_mapping.items():
            try:
                schema_uri = '%s/%s.json' % (
                        self.schema_uri_prefix,
                        schema_uri_suffix)
            except IrodsError as e:
                l.debug('Failed to construct schema URI')
                six.reraise(IrodsWarning, IrodsWarning('%s\n%s' % (
                        'Preflight Check problem:',
                        lib.indent('JSON Configuration Validation failed.'))),
                    sys.exc_info()[2])

            l.debug('Attempting to validate against %s against %s', config_file['path'], schema_uri)
            try:
                json_validation.validate_dict(
                        config_file['dict'],
                        schema_uri,
                        name=config_file['path'])
            except IrodsWarning as e:
                l.warning(e)
                l.warning('Warning encountered in json_validation for %s, skipping validation...',
                        config_file['path'])
                l.debug('Exception:', exc_info=True)
                skipped.append(config_file['path'])
        if skipped:
            raise IrodsWarning('%s\n%s' % (
                'Skipped validation for the following files:',
                lib.indent(*skipped)))

    def commit(self, config_dict, path, clear_cache=True):
        l = logging.getLogger(__name__)
        l.info('Updating %s...', path)
        with open(path, mode='w') as f:
            json.dump(config_dict, f, indent=4, sort_keys=True)
        if clear_cache:
            self.clear_cache()

    def clear_cache(self):
        super(IrodsConfig, self).clear_cache()
        self._database_config = None
        self._server_config = None
        self._version = None
        self._hosts_config = None
        self._host_access_control_config = None
        self._client_environment = None
        self._schema_uri_prefix = None
        self._execution_environment = None

def load_json_config(path):
    l = logging.getLogger(__name__)
    if os.path.exists(path):
        l.debug('Loading %s into dictionary', path)
        try :
            return lib.open_and_load_json(path)
        except ValueError as e:
            six.reraise(IrodsError,
                    IrodsError('%s\n%s' % (
                        'JSON load failed for [%s]:' % (path),
                        lib.indent('Invalid JSON.',
                            '%s: %s' % (e.__class__.__name__, e)))),
                    sys.exc_info()[2])
    else:
        raise IrodsError(
            'File %s does not exist.' % (path))
