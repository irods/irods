from __future__ import print_function
import contextlib
import filecmp
import itertools
import json
import logging
import os
import pprint
import shutil
import sys
import tempfile
import time

from . import six

from .exceptions import IrodsError, IrodsWarning
from . import lib
from . import json_validation
from .password_obfuscation import encode, decode
from . import paths

class IrodsConfig(object):
    def __init__(self,
                 injected_environment={},
                 insert_behavior=True):
        self._injected_environment = lib.callback_on_change_dict(self.clear_cache, injected_environment)
        self._insert_behavior = insert_behavior
        self.clear_cache()

    @property
    def version_tuple(self):
        if os.path.exists(paths.version_path()):
            return lib.version_string_to_tuple(self.version['irods_version'])

        legacy_version_file_path = os.path.join(paths.irods_directory(), 'VERSION')
        if os.path.exists(legacy_version_file_path):
            with open(legacy_version_file_path) as f:
                for line in f:
                    key, _, value = line.strip().partition('=')
                    if key == 'IRODSVERSION':
                        return lib.version_string_to_tuple(value)

        raise IrodsError('Unable to determine iRODS version')

    @property
    def server_config(self):
        if self._server_config is None:
            self._server_config = load_json_config(paths.server_config_path(),
                    template_filepath=paths.get_template_filepath(paths.server_config_path()))
        return self._server_config

    @property
    def is_catalog(self):
        return self.server_config['catalog_service_role'] == 'provider'

    @property
    def is_resource(self):
        return self.server_config['catalog_service_role'] == 'consumer'

    @property
    def default_rule_engine_instance(self):
        return self.server_config['plugin_configuration']['rule_engines'][0]['instance_name']

    @property
    def default_rule_engine_plugin(self):
        return self.server_config['plugin_configuration']['rule_engines'][0]['plugin_name']

    @property
    def configured_rule_engine_plugins(self):
        ret_list = []
        for re in self.server_config['plugin_configuration']['rule_engines']:
            ret_list.append(re['plugin_name'])
        return ret_list

    @property
    def database_config(self):
        try:
            database_config = [e for e in self.server_config['plugin_configuration']['database'].values()][0]
        except (KeyError, IndexError):
            return None
        if not 'db_odbc_driver' in database_config.keys():
            l = logging.getLogger(__name__)
            l.debug('No driver found in the database config, attempting to retrieve the one in the odbc ini file at "%s"...', self.odbc_ini_path)
            if os.path.exists(self.odbc_ini_path):
                from . import database_connect
                with open(self.odbc_ini_path) as f:
                    odbc_ini_contents = database_connect.load_odbc_ini(f)
            else:
                l.debug('No odbc.ini file present')
                odbc_ini_contents = {}
            if self.catalog_database_type in odbc_ini_contents.keys() and 'Driver' in odbc_ini_contents[self.catalog_database_type].keys():
                database_config['db_odbc_driver'] = odbc_ini_contents[self.catalog_database_type]['Driver']
                l.debug('Adding driver "%s" to database_config', database_config['db_odbc_driver'])
                self.commit(self._server_config, paths.server_config_path(), clear_cache=False)
            else:
                l.debug('Unable to retrieve "Driver" field from odbc ini file')

        return database_config

    @property
    def catalog_database_type(self):
        try:
            return [e for e in self.server_config['plugin_configuration']['database'].keys()][0]
        except (KeyError, IndexError):
            return None

    @property
    def odbc_ini_path(self):
        if 'ODBCINI' in self.execution_environment:
            return self.execution_environment['ODBCINI']
        return os.path.join(paths.home_directory(), '.odbc.ini')

    @property
    def version(self):
        if self._version is None:
            self._version = load_json_config(paths.version_path())
        return self._version

    @property
    def hosts_config(self):
        if self._hosts_config is None:
            self._hosts_config = load_json_config(paths.hosts_config_path(),
                    template_filepath=paths.get_template_filepath(paths.hosts_config_path()))
        return self._hosts_config

    @property
    def host_access_control_config(self):
        if self._host_access_control_config is None:
            self._host_access_control_config = load_json_config(paths.host_access_control_config_path(),
                    template_filepath=paths.get_template_filepath(paths.host_access_control_config_path()))
        return self._host_access_control_config

    @property
    def client_environment_path(self):
        if 'IRODS_ENVIRONMENT_FILE' in self.execution_environment:
            return self.execution_environment['IRODS_ENVIRONMENT_FILE']
        else:
            return paths.default_client_environment_path()

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
                self._execution_environment['irodsConfigDir'] = paths.config_directory()
                self._execution_environment['PWD'] = paths.server_bin_directory()
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
                        (paths.server_config_path(), key))

            key = 'configuration_schema_version'
            try:
                uri_version = self.version[key]
            except KeyError:
                uri_version = None
                raise IrodsWarning(
                        '%s did not contain \'%s\'' %
                        (paths.version_path(), key))

            self._schema_uri_prefix = '/'.join([
                    base_uri,
                    'v%s' % (uri_version)])
            l.debug('Successfully constructed schema URI.')
        return self._schema_uri_prefix

    @property
    def admin_password(self):
        if not os.path.exists(os.path.dirname(paths.password_file_path())):
            return None
        with open(paths.password_file_path(), 'rt') as f:
            return decode(f.read())

    @admin_password.setter
    def admin_password(self, value):
        l = logging.getLogger(__name__)
        if not os.path.exists(os.path.dirname(paths.password_file_path())):
            os.makedirs(os.path.dirname(paths.password_file_path()), mode=0o700)
        mtime = int(time.time())
        with open(paths.password_file_path(), 'wt') as f:
            l.debug('Writing password file %s', f.name)
            print(encode(value, mtime=mtime), end='', file=f)
        os.utime(paths.password_file_path(), (mtime, mtime))

    def validate_configuration(self):
        l = logging.getLogger(__name__)

        configuration_schema_mapping = {
                'server_config': {
                    'dict': self.server_config,
                    'path': paths.server_config_path()},
                'VERSION': {
                    'dict': self.version,
                    'path': paths.version_path()},
                'hosts_config': {
                    'dict': self.hosts_config,
                    'path': paths.hosts_config_path()},
                'host_access_control_config': {
                    'dict': self.host_access_control_config,
                    'path': paths.host_access_control_config_path()},
                'service_account_environment': {
                    'dict': self.client_environment,
                    'path': self.client_environment_path}}

        skipped = []

        if self.server_config['schema_validation_base_uri'] == 'off':
            l.warn('Schema validation is disabled; json files will not be validated against schemas. To re-enable schema validation, supply a URL to a set of iRODS schemas in the field "schema_validation_base_uri" and a valid version in the field "schema_version" in the server configuration file (located in %s).', paths.server_config_path())
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

            l.debug('Attempting to validate %s against %s', config_file['path'], schema_uri)
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

    def commit(self, config_dict, path, clear_cache=True, make_backup=False):
        l = logging.getLogger(__name__)
        l.info('Updating %s...', path)
        with tempfile.NamedTemporaryFile(mode='wt', delete=False) as f:
            json.dump(config_dict, f, indent=4, sort_keys=True)
        if os.path.exists(path):
            if filecmp.cmp(f.name, path):
                return
            if make_backup:
                shutil.copyfile(path, '.'.join([path, 'prev', str(time.time())]))
        shutil.move(f.name, path)
        if clear_cache:
            self.clear_cache()

    def clear_cache(self):
        self._server_config = None
        self._version = None
        self._hosts_config = None
        self._host_access_control_config = None
        self._client_environment = None
        self._schema_uri_prefix = None
        self._execution_environment = None

    #provide accessors for all the paths
    @property
    def root_directory(self):
        return paths.root_directory()

    @property
    def irods_directory(self):
        return paths.irods_directory()

    @property
    def config_directory(self):
        return paths.config_directory()

    @property
    def home_directory(self):
        return paths.home_directory()

    @property
    def core_re_directory(self):
        return paths.core_re_directory()

    @property
    def scripts_directory(self):
        return paths.scripts_directory()

    @property
    def server_config_path(self):
        return paths.server_config_path()

    @property
    def database_config_path(self):
        return paths.database_config_path()

    @property
    def version_path(self):
        return paths.version_path()

    @property
    def hosts_config_path(self):
        return paths.hosts_config_path()

    @property
    def host_access_control_config_path(self):
        return paths.host_access_control_config_path()

    @property
    def password_file_path(self):
        return paths.password_file_path()

    @property
    def log_directory(self):
        return paths.log_directory()

    @property
    def control_log_path(self):
        return paths.control_log_path()

    @property
    def setup_log_path(self):
        return paths.setup_log_path()

    @property
    def test_log_path(self):
        return paths.test_log_path()

    @property
    def icommands_test_directory(self):
        return paths.icommands_test_directory()

    @property
    def server_test_directory(self):
        return paths.server_test_directory()

    @property
    def server_log_path(self):
        return paths.server_log_path()

    @property
    def re_log_path(self):
        return paths.re_log_path()

    @property
    def server_bin_directory(self):
        return paths.server_bin_directory()

    @property
    def server_executable(self):
        return paths.server_executable()

    @property
    def rule_engine_executable(self):
        return paths.rule_engine_executable()

    @property
    def xmsg_server_executable(self):
        return paths.xmsg_server_executable()

    @property
    def database_schema_update_directory(self):
        return paths.database_schema_update_directory()

    @property
    def service_account_file_path(self):
        return paths.service_account_file_path()

    @property
    def irods_user(self):
        return paths.irods_user()

    @property
    def irods_uid(self):
        return paths.irods_uid()

    @property
    def irods_group(self):
        return paths.irods_group()

    @property
    def irods_gid(self):
        return paths.irods_gid()

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
