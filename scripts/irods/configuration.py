from __future__ import print_function
import contextlib
import itertools
import json
import logging
import os
import pprint
import shutil
import sys
import time

from . import six

from . import database_connect
from .exceptions import IrodsError, IrodsWarning
from . import lib
from . import json_validation
from .password_obfuscation import encode, decode
from . import paths

class IrodsConfig(paths.IrodsPaths):
    def __init__(self,
                 top_level_directory=None,
                 injected_environment={},
                 insert_behavior=True):
        super(IrodsConfig, self).__init__(top_level_directory)

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
            self._server_config = load_json_config(self.server_config_path,
                    template_filepath=self.get_template_filepath(self.server_config_path))
        return self._server_config

    @property
    def is_catalog(self):
        return self.server_config['catalog_service_role'] == 'provider'

    @property
    def is_resource(self):
        return self.server_config['catalog_service_role'] == 'consumer'

    @property
    def database_config(self):
        if self._database_config is None:
            self._database_config = load_json_config(self.database_config_path,
                    template_filepath=self.get_template_filepath(self.database_config_path))
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
            self._hosts_config = load_json_config(self.hosts_config_path,
                    template_filepath=self.get_template_filepath(self.hosts_config_path))
        return self._hosts_config

    @property
    def host_access_control_config(self):
        if self._host_access_control_config is None:
            self._host_access_control_config = load_json_config(self.host_access_control_config_path,
                    template_filepath=self.get_template_filepath(self.host_access_control_config_path))
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

            key = 'schema_validation_base_uri'
            try:
                base_uri = self.server_config[key]
            except KeyError:
                base_uri = None
                raise IrodsWarning(
                        '%s did not contain \'%s\'' %
                        (self.server_config_path, key))

            key = 'configuration_schema_version'
            try:
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

        if self.server_config['schema_validation_base_uri'] == 'off':
            l.warn('Schema validation is disabled; json files will not be validated against schemas. To re-enable schema validation, supply a URL to a set of iRODS schemas in the field "schema_validation_base_uri" and a valid version in the field "schema_version" in the server configuration file (located in %s).', self.server_config_path)
            return
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

def load_json_config(path, template_filepath=None):
    l = logging.getLogger(__name__)
    if not os.path.exists(path) and template_filepath is not None:
        l.debug('%s does not exist, copying from template file %s', path, template_filepath)
        shutil.copyfile(template_filepath, path)
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
