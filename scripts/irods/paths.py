import grp
import os
import pwd
import pathlib
import logging

from .exceptions import IrodsError

try:
    from . import paths_cmake
    no_paths_cmake = False
except ImportError:
    no_paths_cmake = True

# If making changes to this file, equivalent changes may also be needed in
# lib/core/src/irods_default_paths.cpp

# TODO: consider completely transitioning to pathlib paths.

# Returns a normalized pathlib.PurePath of CMake-provided IRODS_HOME_DIRECTORY
# Not guaranteed to return a usable value. Use _irods_directory() instead.
_irods_install_directory_cache = None
def _irods_install_directory():
    global _irods_install_directory_cache
    if _irods_install_directory_cache is None:
        if no_paths_cmake:
            raise IrodsError('Running from source tree. No install directories.')
        _irods_install_directory_cache = pathlib.PurePath(os.path.normpath(paths_cmake.cmake_dir_irodshome))
    return _irods_install_directory_cache

# Returns a normalized pathlib.PurePath of CMake-provided CMAKE_INSTALL_SYSCONFDIR
# Not guaranteed to return a usable value. Use _sysconf_directory() instead.
_sysconf_install_directory_cache = None
def _sysconf_install_directory():
    global _sysconf_install_directory_cache
    if _sysconf_install_directory_cache is None:
        if no_paths_cmake:
            raise IrodsError('Running from source tree. No install directories.')
        _sysconf_install_directory_cache = pathlib.PurePath(os.path.normpath(paths_cmake.cmake_dir_sysconf))
    return _sysconf_install_directory_cache

# Returns a normalized pathlib.PurePath of CMake-provided IRODS_PLUGINS_DIRECTORY
# Not guaranteed to return a usable value. Use _plugins_directory() instead.
_plugins_install_directory_cache = None
def _plugins_install_directory():
    global _plugins_install_directory_cache
    if _plugins_install_directory_cache is None:
        if no_paths_cmake:
            raise IrodsError('Running from source tree. No install directories.')
        _plugins_install_directory_cache = pathlib.PurePath(os.path.normpath(paths_cmake.cmake_dir_plugins))
    return _plugins_install_directory_cache

# Returns a normalized pathlib.PurePath of CMake-provided CMAKE_INSTALL_SBINDIR
# Not guaranteed to return a usable value. Use _server_bin_directory() instead.
_server_bin_install_directory_cache = None
def _server_bin_install_directory():
    global _server_bin_install_directory_cache
    if _server_bin_install_directory_cache is None:
        if no_paths_cmake:
            raise IrodsError('Running from source tree. No install directories.')
        _server_bin_install_directory_cache = pathlib.PurePath(os.path.normpath(paths_cmake.cmake_dir_sbin))
    return _server_bin_install_directory_cache

# Returns a normalized pathlib.PurePath of CMake-provided CMAKE_INSTALL_LOCALSTATEDIR
# Not guaranteed to return a usable value. Use _localstate_directory() instead.
_localstate_install_directory_cache = None
def _localstate_install_directory():
    global _localstate_install_directory_cache
    if _localstate_install_directory_cache is None:
        if no_paths_cmake:
            raise IrodsError('Running from source tree. No install directories.')
        _localstate_install_directory_cache = pathlib.PurePath(os.path.normpath(paths_cmake.cmake_dir_localstate))
    return _localstate_install_directory_cache

# Returns a normalized pathlib.PurePath of CMake-provided CMAKE_INSTALL_RUNSTATEDIR
# Not guaranteed to return a usable value. Use _runstate_directory() instead.
_runstate_install_directory_cache = None
def _runstate_install_directory():
    global _runstate_install_directory_cache
    if _runstate_install_directory_cache is None:
        if no_paths_cmake:
            raise IrodsError('Running from source tree. No install directories.')
        _runstate_install_directory_cache = pathlib.PurePath(os.path.normpath(paths_cmake.cmake_dir_runstate))
    return _runstate_install_directory_cache

_root_directory_cache = None
def _root_directory():
    global _root_directory_cache
    l = logging.getLogger(__name__)
    if _root_directory_cache is None:
        if no_paths_cmake:
            raise IrodsError('Running from source tree. No root directory.')
        irods_dir = _irods_directory()
        irods_dir_install = _irods_install_directory()
        if irods_dir_install.is_absolute():
            irods_dir_install = pathlib.PurePath(*irods_dir_install.parts[1:])
        install_path_len = len(irods_dir_install.parts)
        if irods_dir.parts[-install_path_len:] != irods_dir_install.parts:
            l.warning('iRODS home directory "%s" does not match directory set at build-time "%s".', irods_dir, _irods_install_directory())
        _root_directory_cache = pathlib.Path(*irods_dir.parts[:-install_path_len])
    return _root_directory_cache

# we can tolerate a little re-prefixing if we ignore common base directories.
# For example, if libdir is "usr/lib" and bindir is "usr/bin", we can ignore
# "usr" when traversing from one to the other.
# equivalent to get_irods_directory_impl in irods_default_paths.cpp
# but using IRODS_HOME_DIRECTORY as the traversal start point instead of CMAKE_INSTALL_LIBDIR
def _convert_from_install_dir(install_path):
    # if input path is absolute, do not traverse
    if install_path.is_absolute():
        return pathlib.Path(install_path)

    irods_dir = _irods_directory()
    irods_dir_install = _irods_install_directory()
    len_irods_dir = len(irods_dir_install.parts)

    # if irods_dir_install is absolute, we can't be smart about it
    if irods_dir_install.is_absolute():
        len_irods_dir -= 1
        return pathlib.Path(*irods_dir.parts[:-len_irods_dir], install_path)

    # In get_irods_directory_impl, we use std::filesystem::path::lexically_relative to produce a
    # relative path that can be used to traverse from CMAKE_INSTALL_LIBDIR to install_path.
    # This path usually starts with one or more '..' components.
    # unlike std::filesystem::path::lexically_relative, pathlib.PurePath.relative_to will throw
    # ValueError if the calling PurePath instance (self) is not a subpath of the PurePath passed in.

    len_install_dir = len(install_path.parts)
    for common_part_qty in range(min(len_install_dir, len_irods_dir)):
        if install_path.parts[common_part_qty] != irods_dir.parts[common_part_qty]:
            break
    else:
        common_part_qty += 1

    if common_part_qty == 0:
        # if there is no common base, we can't be smart about it
        return pathlib.Path(*irods_dir.parts[:-len_irods_dir], install_path)

    uncommon_part_qty = len_irods_dir - common_part_qty
    return pathlib.Path(*irods_dir.parts[:-uncommon_part_qty], *install_path.parts[common_part_qty:])

_irods_directory_cache = None
def _irods_directory():
    global _irods_directory_cache
    if _irods_directory_cache is None:
        _irods_directory_cache = _scripts_directory().parent
    return _irods_directory_cache

def irods_directory():
    return str(_irods_directory())

_sysconf_directory_cache = None
def _sysconf_directory():
    global _sysconf_directory_cache
    if _sysconf_directory_cache is None:
        _sysconf_directory_cache = _convert_from_install_dir(_sysconf_install_directory())
    return _sysconf_directory_cache

def sysconf_directory():
    return str(_sysconf_directory())

_config_directory_cache = None
def _config_directory():
    global _config_directory_cache
    if _config_directory_cache is None:
        _config_directory_cache = _sysconf_directory() / 'irods'
    return _config_directory_cache

def config_directory():
    return str(_config_directory())

_plugins_directory_cache = None
def _plugins_directory():
    global _plugins_directory_cache
    if _plugins_directory_cache is None:
        _plugins_directory_cache = _convert_from_install_dir(_plugins_install_directory())
    return _plugins_directory_cache

def plugins_directory():
    return str(_plugins_directory())

# TODO: consider renaming to convey that this is specifically the service account's home directory
# Not cached, as irods_user_and_group_entries return value could change
def _home_directory():
    return pathlib.Path(irods_user_and_group_entries()[0].pw_dir)

# TODO: consider renaming to convey that this is specifically the service account's home directory
# Not cached, as irods_user_and_group_entries return value could change
def home_directory():
    return irods_user_and_group_entries()[0].pw_dir

def core_re_directory():
    return config_directory()

_scripts_directory_cache = None
def _scripts_directory():
    global _scripts_directory_cache
    if _scripts_directory_cache is None:
        _scripts_directory_cache = pathlib.Path(os.path.abspath(__file__)).parent.parent
    return _scripts_directory_cache

def scripts_directory():
    return str(_scripts_directory())

_test_directory_cache = None
def _test_directory():
    global _test_directory_cache
    if _test_directory_cache is None:
        _test_directory_cache = _scripts_directory() / 'irods' / 'test'
    return _test_directory_cache

def test_directory():
    return str(_test_directory())

_server_config_path_cache = None
def _server_config_path():
    global _server_config_path_cache
    if _server_config_path_cache is None:
        _server_config_path_cache = _config_directory() / 'server_config.json'
    return _server_config_path_cache

def server_config_path():
    return str(_server_config_path())

_database_config_path_cache = None
def _database_config_path():
    global _database_config_path_cache
    if _database_config_path_cache is None:
        _database_config_path_cache = _config_directory() / 'database_config.json'
    return _database_config_path_cache

def database_config_path():
    return str(_database_config_path())

_version_path_cache = None
def _version_path():
    global _version_path_cache
    if _version_path_cache is None:
        _version_path_cache = _irods_directory() / 'version.json'
    return _version_path_cache

def version_path():
    return str(_version_path())

_hosts_config_path_cache = None
def _hosts_config_path():
    global _hosts_config_path_cache
    if _hosts_config_path_cache is None:
        _hosts_config_path_cache = _config_directory() / 'hosts_config.json'
    return _hosts_config_path_cache

def hosts_config_path():
    return str(_hosts_config_path())

_host_access_control_config_path_cache = None
def _host_access_control_config_path():
    global _host_access_control_config_path_cache
    if _host_access_control_config_path_cache is None:
        _host_access_control_config_path_cache = _config_directory() / 'host_access_control_config.json'
    return _host_access_control_config_path_cache

def host_access_control_config_path():
    return str(_host_access_control_config_path())

# pathlib.Path of "~/.irods"
# Will change when we start following XDG Base Directory Specification
# TODO: consider renaming to convey that this is specifically the service account's userconf directory
# TODO: cache this if we start caching _home_directory()
def _userconf_directory():
    return _home_directory() / '.irods'

# full path string for "~/.irods"
# Will change when we start following XDG Base Directory Specification
# TODO: consider renaming to convey that this is specifically the service account's userconf directory
def userconf_directory():
    return str(_userconf_directory())

# TODO: consider renaming to convey that this is specifically the service account's password file
# TODO: cache this if we start caching _userconf_directory()
def _password_file_path():
    return _userconf_directory() / '.irodsA'

# TODO: consider renaming to convey that this is specifically the service account's password file
def password_file_path():
    return str(_password_file_path())

# TODO: consider renaming to convey that this is specifically the service account's environment file
# TODO: cache this if we start caching _userconf_directory()
def _default_client_environment_path():
    return _userconf_directory() / 'irods_environment.json'

# TODO: consider renaming to convey that this is specifically the service account's environment file
def default_client_environment_path():
    return str(_default_client_environment_path())

_log_directory_cache = None
def _log_directory():
    global _log_directory_cache
    if _log_directory_cache is None:
        _log_directory_cache = _irods_directory() / 'log'
    return _log_directory_cache

def log_directory():
    return str(_log_directory())

_proc_directory_cache = None
def _proc_directory():
    global _proc_directory_cache
    if _proc_directory_cache is None:
        _proc_directory_cache = _log_directory() / 'proc'
    return _proc_directory_cache

def proc_directory():
    return str(_proc_directory())

_control_log_path_cache = None
def _control_log_path():
    global _control_log_path_cache
    if _control_log_path_cache is None:
        _control_log_path_cache = _log_directory() / 'control_log.txt'
    return _control_log_path_cache

def control_log_path():
    return str(_control_log_path())

_setup_log_path_cache = None
def _setup_log_path():
    global _setup_log_path_cache
    if _setup_log_path_cache is None:
        _setup_log_path_cache = _log_directory() / 'setup_log.txt'
    return _setup_log_path_cache

def setup_log_path():
    return str(_setup_log_path())

_test_log_path_cache = None
def _test_log_path():
    global _test_log_path_cache
    if _test_log_path_cache is None:
        _test_log_path_cache = _log_directory() / 'test_log.txt'
    return _test_log_path_cache

def test_log_path():
    return str(_test_log_path())

_icommands_test_directory_cache = None
def _icommands_test_directory():
    global _icommands_test_directory_cache
    if _icommands_test_directory_cache is None:
        _icommands_test_directory_cache = _irods_directory() / 'clients' / 'icommands' / 'test'
    return _icommands_test_directory_cache

def icommands_test_directory():
    return str(_icommands_test_directory())

_server_test_directory_cache = None
def _server_test_directory():
    global _server_test_directory_cache
    if _server_test_directory_cache is None:
        _server_test_directory_cache = _irods_directory() / 'test' / 'bin'
    return _server_test_directory_cache

def server_test_directory():
    return str(_server_test_directory())

_localstate_directory_cache = None
def _localstate_directory():
    global _localstate_directory_cache
    if _localstate_directory_cache is None:
        _localstate_directory_cache = _convert_from_install_dir(_localstate_install_directory())
    return _localstate_directory_cache

def localstate_directory():
    return str(_localstate_directory())

_default_server_log_path_cache = None
def _default_server_log_path():
    global _default_server_log_path_cache
    if _default_server_log_path_cache is None:
        _default_server_log_path_cache = _localstate_directory() / 'log' / 'irods' / 'irods.log'
    return _default_server_log_path_cache

def default_server_log_path():
    return str(_default_server_log_path())

_testmode_server_log_path_cache = None
def _testmode_server_log_path():
    global _testmode_server_log_path_cache
    if _testmode_server_log_path_cache is None:
        _testmode_server_log_path_cache = _log_directory() / 'test_mode_output.log'
    return _testmode_server_log_path_cache

def testmode_server_log_path():
    return str(_testmode_server_log_path())

def server_parent_log_path():
    return server_log_path()

def _server_log_path():
    env_var_name = 'IRODS_ENABLE_TEST_MODE'
    if os.environ.get(env_var_name) == '1':
        return _testmode_server_log_path()
    return _default_server_log_path()

def server_log_path():
    return str(_server_log_path())

_server_bin_directory_cache = None
def _server_bin_directory():
    global _server_bin_directory_cache
    if _server_bin_directory_cache is None:
        _server_bin_directory_cache = _convert_from_install_dir(_server_bin_install_directory())
    return _server_bin_directory_cache

def server_bin_directory():
    return str(_server_bin_directory())

_server_executable_cache = None
def _server_executable():
    global _server_executable_cache
    if _server_executable_cache is None:
        _server_executable_cache = _server_bin_directory() / 'irodsServer'
    return _server_executable_cache

def server_executable():
    return str(_server_executable())

_rule_engine_executable_cache = None
def _rule_engine_executable():
    global _rule_engine_executable_cache
    if _rule_engine_executable_cache is None:
        _rule_engine_executable_cache = _server_bin_directory() / 'irodsDelayServer'
    return _rule_engine_executable_cache

def rule_engine_executable():
    return str(_rule_engine_executable())

_test_put_get_executable_cache = None
def _test_put_get_executable():
    global _test_put_get_executable_cache
    if _test_put_get_executable_cache is None:
        _test_put_get_executable_cache = _server_bin_directory() / 'irodsTestPutGet'
    return _test_put_get_executable_cache

def test_put_get_executable():
    return str(_test_put_get_executable())

_service_account_file_path_cache = None
def _service_account_file_path():
    global _service_account_file_path_cache
    if _service_account_file_path_cache is None:
        _service_account_file_path_cache = _config_directory() / 'service_account.config'
    return _service_account_file_path_cache

def service_account_file_path():
    return str(_service_account_file_path())

_genosauth_path_cache = None
def _genosauth_path():
    global _genosauth_path_cache
    if _genosauth_path_cache is None:
        _genosauth_path_cache = _irods_directory() / 'clients' / 'bin' / 'genOSAuth'
    return _genosauth_path_cache

def genosauth_path():
    return str(_genosauth_path())

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
    return irods_user_and_group_entries()[0].pw_name

def irods_uid():
    return irods_user_and_group_entries()[0].pw_uid

def irods_group():
    return irods_user_and_group_entries()[1].gr_name

def irods_gid():
    return irods_user_and_group_entries()[1].gr_gid

def _get_template_filepath(filepath):
    return _irods_directory() / 'packaging' / '.'.join([os.path.basename(filepath), 'template'])

def get_template_filepath(filepath):
    return str(_get_template_filepath(filepath))

_runstate_directory_cache = None
def _runstate_directory():
    global _runstate_directory_cache
    if _runstate_directory_cache is None:
        _runstate_directory_cache = _convert_from_install_dir(_runstate_install_directory())
    return _runstate_directory_cache

def runstate_directory():
    return str(_runstate_directory())

_possible_shm_locations_cache = None
def _possible_shm_locations():
    global _possible_shm_locations_cache
    if _possible_shm_locations_cache is None:
        dirs = [
            pathlib.Path(os.sep, 'dev', 'shm'),
            pathlib.Path(os.sep, 'var', 'run', 'shm'),
            pathlib.Path(os.sep, 'run', 'shm')
        ]
        if not no_paths_cmake:
            run_install_dir = _runstate_install_directory()
            if run_install_dir.is_absolute():
                dirs.append(run_install_dir / 'shm')
            else:
                dirs.append(pathlib.Path(os.sep, run_install_dir / 'shm'))
                dirs.append(_runstate_directory() / 'shm')
            var_install_dir = _localstate_install_directory()
            if var_install_dir.is_absolute():
                dirs.append(var_install_dir / 'run' / 'shm')
            else:
                dirs.append(pathlib.Path(os.sep, var_install_dir / 'run' / 'shm'))
                dirs.append(_localstate_directory() / 'run' / 'shm')
        _possible_shm_locations_cache = frozenset(dirs)
    return _possible_shm_locations_cache

_possible_shm_locations_str_cache = None
def possible_shm_locations():
    global _possible_shm_locations_str_cache
    if _possible_shm_locations_str_cache is None:
        _possible_shm_locations_str_cache = frozenset(str(shm_dir) for shm_dir in _possible_shm_locations())
    return _possible_shm_locations_str_cache

# This is a common mount point used by topology tests when testing detached mode
# behavior for unixfilesystem resources.  In detached mode the servers are expected
# to have a common mount where the vault exists.
_test_mount_directory_cache = None
def _test_mount_directory():
    global _test_mount_directory_cache
    if _test_mount_directory_cache is None:
        _test_mount_directory_cache = pathlib.Path(os.sep, 'irods_testing_environment_mount_dir')
    return _test_mount_directory_cache

def test_mount_directory():
    return str(_test_mount_directory())
