import grp
import os
import pwd

def _root_directory():
    return os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(scripts_directory()))))

def irods_directory():
    return os.path.dirname(scripts_directory())

def config_directory():
    return os.path.join(_root_directory(), 'etc', 'irods')

def plugins_directory():
    return os.path.join(_root_directory(), 'usr', 'lib', 'irods', 'plugins')

def home_directory():
    return irods_user_and_group_entries()[0][5]

def core_re_directory():
    return config_directory()

def scripts_directory():
    return os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

def test_directory():
    return os.path.join(scripts_directory(), 'irods', 'test')

def server_config_path():
    return os.path.join(
        config_directory(),
        'server_config.json')

def database_config_path():
    return os.path.join(
        config_directory(),
        'database_config.json')

def version_path():
    return os.path.join(
        irods_directory(),
        'version.json')

def hosts_config_path():
    return os.path.join(
        config_directory(),
        'hosts_config.json')

def host_access_control_config_path():
    return os.path.join(
        config_directory(),
        'host_access_control_config.json')

def password_file_path():
    return os.path.join(
        home_directory(),
        '.irods',
        '.irodsA')

def default_client_environment_path():
    return os.path.join(
        home_directory(),
        '.irods',
        'irods_environment.json')

def log_directory():
    return os.path.join(
        irods_directory(),
        'log')

def proc_directory():
    return os.path.join(log_directory(), 'proc')

def control_log_path():
    return os.path.join(
        log_directory(),
        'control_log.txt')

def setup_log_path():
    return os.path.join(
        log_directory(),
        'setup_log.txt')

def test_log_path():
    return os.path.join(
        log_directory(),
        'test_log.txt')

def icommands_test_directory():
    return os.path.join(
        irods_directory(),
        'clients',
        'icommands',
        'test')

def server_test_directory():
    return os.path.join(
        irods_directory(),
        'test',
        'bin')

def server_parent_log_path():
    return server_log_path()

def server_log_path():
    env_var_name = 'IRODS_ENABLE_TEST_MODE'
    if env_var_name in os.environ and os.environ[env_var_name] == '1':
        return os.path.join(
            log_directory(),
            'test_mode_output.log')
    return '/var/log/irods/irods.log'

def server_bin_directory():
    return os.path.join(
        _root_directory(),
        'usr',
        'sbin')

def server_executable():
    return os.path.join(
        server_bin_directory(),
        'irodsServer')

def rule_engine_executable():
    return os.path.join(
        server_bin_directory(),
        'irodsDelayServer')

def test_put_get_executable():
    return os.path.join(server_bin_directory(), 'irodsTestPutGet')

def service_account_file_path():
    return os.path.join(config_directory(), 'service_account.config')

def genosauth_path():
    return os.path.join(irods_directory(), 'clients', 'bin', 'genOSAuth')

def irods_user_and_group_entries():
    try:
        with open(service_account_file_path()) as f:
            service_account_dict = dict([(l.partition('=')[0].strip(), l.partition('=')[2].strip()) for l in f.readlines()])
        user = pwd.getpwnam(service_account_dict['IRODS_SERVICE_ACCOUNT_NAME'])
        group = grp.getgrnam(service_account_dict['IRODS_SERVICE_GROUP_NAME'])
    except IOError:
        user = pwd.getpwuid(os.stat(irods_directory()).st_uid)
        group = grp.getgrgid(os.stat(irods_directory()).st_gid)
    return (user, group)

def irods_user():
    return irods_user_and_group_entries()[0][0]

def irods_uid():
    return irods_user_and_group_entries()[0][2]

def irods_group():
    return irods_user_and_group_entries()[1][0]

def irods_gid():
    return irods_user_and_group_entries()[1][2]

def get_template_filepath(filepath):
    return os.path.join(irods_directory(), 'packaging', '.'.join([os.path.basename(filepath), 'template']))

def possible_shm_locations():
    return {
        os.path.join(os.sep, 'dev', 'shm'),
        os.path.join(os.sep, 'var', 'run', 'shm'),
        os.path.join(os.sep, 'run', 'shm'),
    }
