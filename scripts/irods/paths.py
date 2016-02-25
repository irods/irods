import grp
import inspect
import os
import pwd

class IrodsPaths(object):
    def __init__(self,
                 top_level_directory=None,
                 config_directory=None):

        self._top_level_directory = top_level_directory
        self._config_directory = config_directory
        self.clear_cache()

    @property
    def top_level_directory(self):
        return self._top_level_directory if self._top_level_directory else get_default_top_level_directory()

    @top_level_directory.setter
    def top_level_directory(self, value):
        self.clear_cache()
        self._top_level_directory = value

    @property
    def config_directory(self):
        if self._config_directory:
            return self._config_directory
        elif self.binary_installation:
            return os.path.join(
                get_root_directory(),
                'etc',
                'irods')
        else:
            return os.path.join(
                self.irods_directory,
                'server',
                'config')

    @config_directory.setter
    def config_directory(self, value):
        self.clear_cache()
        self._config_directory = value

    @property
    def home_directory(self):
        return os.path.expanduser(''.join(['~', self.irods_user]))

    @property
    def binary_installation(self):
        if self._binary_installation is None:
            self._binary_installation = os.path.exists(
                    os.path.join(
                        self.top_level_directory,
                        'packaging',
                        'binary_installation.flag'))
        return self._binary_installation

    @property
    def is_catalog(self):
        return os.path.exists(os.path.join(self.top_level_directory,
            'catalog.flag'))

    @property
    def is_resource(self):
        return os.path.exists(os.path.join(self.top_level_directory,
            'resource.flag'))

    @property
    def core_re_directory(self):
        if self.binary_installation:
            return self.config_directory
        else:
            return os.path.join(self.config_directory, 'reConfigs')

    @property
    def scripts_directory(self):
        return os.path.join(self.top_level_directory, 'scripts')

    @property
    def irods_directory(self):
        return os.path.join(
            self.top_level_directory,
            'iRODS')

    @property
    def server_config_path(self):
        return os.path.join(
            self.config_directory,
            'server_config.json')

    @property
    def database_config_path(self):
        return os.path.join(
            self.config_directory,
            'database_config.json')

    @property
    def version_path(self):
        return os.path.join(
            self.top_level_directory,
            'VERSION.json')

    @property
    def hosts_config_path(self):
        return os.path.join(
            self.config_directory,
            'hosts_config.json')

    @property
    def host_access_control_config_path(self):
        return os.path.join(
            self.config_directory,
            'host_access_control_config.json')

    @property
    def password_file_path(self):
        return os.path.join(
            self.home_directory,
            '.irods',
            '.irodsA')

    @property
    def log_directory(self):
        return os.path.join(
            self.top_level_directory,
            'log')

    @property
    def control_log_path(self):
        return os.path.join(
            self.log_directory,
            'control_log.txt')

    @property
    def setup_log_path(self):
        return os.path.join(
            self.log_directory,
            'setup_log.txt')

    @property
    def test_log_path(self):
        return os.path.join(
            self.log_directory,
            'test_log.txt')

    @property
    def icommands_test_directory(self):
        return os.path.join(
            self.irods_directory,
            'clients',
            'icommands',
            'test')

    @property
    def server_test_directory(self):
        return os.path.join(
            self.irods_directory,
            'server',
            'test',
            'bin')

    @property
    def server_log_directory(self):
        return os.path.join(
            self.irods_directory,
            'server',
            'log')

    @property
    def server_log_path(self):
        return sorted([os.path.join(self.server_log_directory, name)
                for name in os.listdir(self.server_log_directory)
                if name.startswith('rodsLog')],
            key=lambda path: os.path.getctime(path))[-1]

    @property
    def re_log_path(self):
        return sorted([os.path.join(self.server_log_directory, name)
                for name in os.listdir(self.server_log_directory)
                if name.startswith('reLog')],
            key=lambda path: os.path.getctime(path))[-1]

    @property
    def server_bin_directory(self):
        return os.path.join(
            self.irods_directory,
            'server',
            'bin')

    @property
    def server_executable(self):
        return os.path.join(
            self.server_bin_directory,
            'irodsServer')

    @property
    def rule_engine_executable(self):
        return os.path.join(
            self.server_bin_directory,
            'irodsReServer')

    @property
    def xmsg_server_executable(self):
        return os.path.join(
            self.server_bin_directory,
            'irodsXmsgServer')

    @property
    def agent_executable(self):
        return os.path.join(
            self.server_bin_directory,
            'irodsAgent')

    @property
    def database_schema_update_directory(self):
        if self.binary_installation:
            return os.path.join(
                    self.top_level_directory,
                    'packaging',
                    'schema_updates')
        else:
            return os.path.join(
                    self.top_level_directory,
                    'plugins',
                    'database',
                    'packaging',
                    'schema_updates')

    @property
    def service_account_file_path(self):
        return os.path.join(self.config_directory, 'service_account.config')

    @property
    def irods_user(self):
        return pwd.getpwuid(os.stat(self.top_level_directory).st_uid).pw_name

    @property
    def irods_uid(self):
        return os.stat(self.top_level_directory).st_uid

    @property
    def irods_group(self):
        return grp.getgrgid(os.stat(self.top_level_directory).st_gid).gr_name

    @property
    def irods_gid(self):
        return os.stat(self.top_level_directory).st_gid

    @property
    def odbc_ini_path(self):
        if 'ODBCINI' in self.execution_environment:
            return self.execution_environment['ODBCINI']
        return os.path.join(self.home_directory, '.odbc.ini')

    def clear_cache(self):
        self._binary_installation = None

def get_default_top_level_directory():
    scripts_directory = os.path.dirname(os.path.dirname(os.path.abspath(
        inspect.stack()[0][1])))
    return os.path.dirname(
            scripts_directory)

def get_root_directory():
    return os.path.abspath(os.sep)
