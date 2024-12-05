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
import datetime

from .exceptions import IrodsError, IrodsWarning, IrodsSchemaError
from . import lib
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
    def is_provider(self):
        return self.server_config['catalog_service_role'] == 'provider'

    @property
    def is_catalog(self):
        # compatible with 4.2.x
        return self.is_provider

    @property
    def is_consumer(self):
        return self.server_config['catalog_service_role'] == 'consumer'

    @property
    def is_resource(self):
        # compatible with 4.2.x
        return self.is_consumer

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

    def print_execution_environment(self):
        pprint.pprint(self.execution_environment)

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

    def commit(self, config_dict, path, clear_cache=True, make_backup=False):
        l = logging.getLogger(__name__)
        l.info('Updating %s...', path)
        with tempfile.NamedTemporaryFile(mode='wt', delete=False) as f:
            json.dump(config_dict, f, indent=4, sort_keys=True)
        if os.path.exists(path):
            if filecmp.cmp(f.name, path):
                return
            if make_backup:
                time_suffix = datetime.datetime.utcnow().strftime('%Y%m%dT%H%M%SZ')
                shutil.copyfile(path, '.'.join([path, 'prev', time_suffix]))
        shutil.move(f.name, path)
        if clear_cache:
            self.clear_cache()

    def clear_cache(self):
        self._server_config = None
        self._version = None
        self._hosts_config = None
        self._host_access_control_config = None
        self._client_environment = None
        self._execution_environment = None

    #provide accessors for all the paths
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
    def server_parent_log_path(self):
        return paths.server_parent_log_path()

    @property
    def server_log_path(self):
        return paths.server_log_path()

    @property
    def server_bin_directory(self):
        return paths.server_bin_directory()

    @property
    def server_executable(self):
        return paths.server_executable()

    @property
    def agent_executable(self):
        return paths.agent_executable()

    @property
    def rule_engine_executable(self):
        return paths.rule_engine_executable()

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
        raise IrodsError('%s\n%s' % (
                'JSON load failed for [%s]:' % (path),
                lib.indent('Invalid JSON.',
                    '%s: %s' % (e.__class__.__name__, e))))
