from __future__ import print_function
import contextlib
import copy
import glob
import inspect
import itertools
import json
import logging
import optparse
import os
import pprint
import socket
import subprocess
import sys
import tempfile
import time

import irods_six
import psutil

import irods_logging
import get_db_schema_version
import validate_json

class IrodsControllerError(Exception):
    pass

class IrodsController(object):
    def __init__(self,
                 top_level_directory=None,
                 config_directory=None,
                 static_execution_environment={}):

        if top_level_directory:
            self.top_level_directory = top_level_directory
        else:
            self.top_level_directory = get_default_top_level_directory()
        self.manually_set_config_directory = config_directory
        self.static_execution_environment = static_execution_environment

        # load the configuration to ensure it exists
        load_json_config_file(self.get_server_config_path())
        load_json_config_file(self.get_version_path())

    # The default behavior is to (effectively) insert the passed execution
    # environment into the standard execution environment. If you instead
    # wish to replace the environment wholesale, set insert_behavior to False.
    def start(self, execution_environment={}, insert_behavior=True):
        l = logging.getLogger(__name__)
        l.debug('Calling start on IrodsController')
        new_execution_environment = {}
        for key, value in execution_environment.items():
            new_execution_environment[key] = value

        if insert_behavior:
            for key, value in self.static_execution_environment.items():
                if key not in new_execution_environment:
                    new_execution_environment[key] = value
            if 'irodsHomeDir' not in new_execution_environment:
                new_execution_environment['irodsHomeDir'] = self.get_irods_directory()
            if 'irodsConfigDir' not in new_execution_environment:
                new_execution_environment['irodsConfigDir'] = self.get_config_directory()
            if 'PWD' not in new_execution_environment:
                new_execution_environment['PWD'] = self.get_server_bin_directory()

        for key, value in os.environ.items():
            if key not in new_execution_environment:
                new_execution_environment[key] = value

        if not os.path.exists(self.get_server_executable()):
            raise IrodsControllerError('\n\t'.join([
                'Configuration problem:',
                'The \'{0}\' application could not be found.  Have the'.format(
                    os.path.basename(self.get_server_executable())),
                'iRODS servers been compiled?']),
                sys.exc_info()[2])

        try:
            (test_file_handle, test_file_name) = tempfile.mkstemp(
                dir=self.get_log_directory())
            os.close(test_file_handle)
            os.unlink(test_file_name)
        except (IOError, OSError):
            irods_six.reraise(IrodsControllerError, IrodsControllerError('\n\t'.join([
                    'Configuration problem:',
                    'The server log directory, \'{0}\''.format(
                        self.get_log_directory()),
                    'is not writeable.  Please change its permissions',
                    'and retry.'])),
                    sys.exc_info()[2])

        config_dicts = self.load_and_validate_config_files()

        if self.get_database_config_path() in config_dicts:
            schema_version_in_file = config_dicts[self.get_version_path()]['catalog_schema_version']
            if not self.check_database_schema_version(
                    schema_version_in_file=schema_version_in_file):
                raise IrodsControllerError('\n\t'.join([
                    'Preflight Check problem:',
                    'Database Schema in the database is ahead',
                    'of {0} - Please upgrade.'.format(
                        os.path.basename(self.get_version_path()))]))

        try:
            irods_port = int(config_dicts[self.get_server_config_path()]['zone_port'])
            l.debug('Attempting to bind socket %s', irods_port)
            with contextlib.closing(socket.socket(socket.AF_INET, socket.SOCK_STREAM)) as s:
                try:
                    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                    s.bind(('127.0.0.1', irods_port))
                except socket.error:
                    irods_six.reraise(IrodsControllerError,
                            IrodsControllerError('Could not bind port {0}.'.format(irods_port)),
                            sys.exc_info()[2])
            l.debug('Socket %s bound and released successfully.')

            l.info('Starting iRODS server...')
            execute_command(
                [self.get_server_executable()],
                cwd=self.get_server_bin_directory(),
                env=new_execution_environment)

            retry_count = 100
            while True:
                l.debug('Attempting to connect to iRODS server on port %s. Attempt #%s',
                        irods_port, 101 - retry_count)
                with contextlib.closing(socket.socket(
                        socket.AF_INET, socket.SOCK_STREAM)) as s:
                    if s.connect_ex(('127.0.0.1', irods_port)) == 0:
                        if get_pids_executing_binary_file(
                                self.get_server_executable()):
                            l.debug('Successfully connected to port %s.', irods_port)
                            break
                        else:
                            retry_count = 0
                if retry_count <= 0:
                    raise IrodsControllerError('iRODS server failed to start.')
                retry_count = retry_count - 1
                time.sleep(1)
        except IrodsControllerError as e:
            l.info('Failure')
            irods_six.reraise(IrodsControllerError, e, sys.exc_info()[2])

        l.info('Success')

    def irods_grid_shutdown(self, timeout=20):
        l = logging.getLogger(__name__)
        args = ['irods-grid', 'shutdown', '--hosts={0}'.format(get_hostname())]
        kwargs = {'stdout': subprocess.PIPE, 'stderr': subprocess.PIPE}
        p = execute_command_nonblocking(args, **kwargs)
        start_time = time.time()
        while time.time() < start_time + timeout:
            if p.poll() is not None:
                out, err = communicate_and_log(p, args)
                check_command_return(args, out.decode(), err.decode(), p.returncode, **kwargs)
                break
            time.sleep(0.3)
        else:
            try:
                if p.poll() is None:
                    p.kill()
            except OSError:
                pass
            raise IrodsControllerError(
                'The call to "irods-grid shutdown" did not complete within'
                ' {0} seconds.'.format(timeout))

        # "irods-grid shutdown" is non-blocking
        while time.time() < start_time + timeout:
            if self.get_binary_to_pids_dict([self.get_server_executable()]):
                time.sleep(0.3)
            else:
                break
        else:
            raise IrodsControllerError(
                'The iRODS server did not stop within {0} seconds of '
                'receiving the "shutdown" command.'.format(timeout))

    def stop(self, timeout=20):
        l = logging.getLogger(__name__)
        l.debug('Calling stop on IrodsController')
        l.info('Stopping iRODS server...')
        try:
            if self.get_binary_to_pids_dict([self.get_server_executable()]):
                try:
                    self.irods_grid_shutdown(timeout=timeout)
                except Exception as e:
                    l.error('Error encountered in graceful shutdown.', exc_info=True)
            else:
                    l.warning('No iRODS servers running.')

            # kill servers first to stop spawning of other processes
            server_pids_dict = self.get_binary_to_pids_dict([self.get_server_executable()])
            if server_pids_dict:
                l.warning('iRODS server processes remain after "irods-grid shutdown".')
                l.warning(format_binary_to_pids_dict(server_pids_dict))
                l.warning('Killing forcefully...')
                for pid in server_pids_dict[self.get_server_executable()]:
                    l.warning('Killing %s, pid %s', self.get_server_executable(), pid)
                    try:
                        kill_pid(pid)
                    except psutil.NoSuchProcess:
                        pass
                    elete_cache_files_by_pid(pid)

            binary_to_pids_dict = self.get_binary_to_pids_dict()
            if binary_to_pids_dict:
                l.warning('iRODS child processes remain after "irods-grid shutdown".')
                l.warning(format_binary_to_pids_dict(binary_to_pids_dict))
                l.warning('Killing forcefully...')
                for binary, pids in binary_to_pids_dict.items():
                    for pid in pids:
                        l.warning('Killing %s, pid %s', binary, pid)
                        try:
                            kill_pid(pid)
                        except psutil.NoSuchProcess:
                            pass
                        delete_cache_files_by_pid(pid)
        except IrodsControllerError as e:
            l.info('Failure')
            irods_six.reraise(IrodsControllerError, e, sys.exc_info()[2])

        l.info('Success')

    def restart(self, execution_environment=None, insert_behavior=True):
        l = logging.getLogger(__name__)
        l.debug('Calling restart on IrodsController')
        if execution_environment is None:
            execution_environment = {}
        self.stop()
        self.start(execution_environment=execution_environment,
                   insert_behavior=insert_behavior)

    def setup(self):
        l = logging.getLogger(__name__)
        l.debug('Calling setup on IrodsController')

    def status(self):
        l = logging.getLogger(__name__)
        l.debug('Calling status on IrodsController')
        binary_to_pids_dict = self.get_binary_to_pids_dict()
        if not binary_to_pids_dict:
            l.info('No iRODS servers running.')
        else:
            l.info(format_binary_to_pids_dict(binary_to_pids_dict))
        return binary_to_pids_dict

    def load_and_validate_config_files(self):
        l = logging.getLogger(__name__)
        config_files = [
                self.get_server_config_path(),
                self.get_version_path(),
                self.get_hosts_config_path(),
                self.get_host_access_control_config_path(),
                get_irods_environment_path()]
        if os.path.exists(self.get_database_config_path()):
            config_files.append(self.get_database_config_path())
        else:
            l.debug('The database config file, \'%s\', does not exist.', self.get_database_config_path())

        config_dicts = {}
        for path in config_files:
            l.debug('Loading %s into dictionary', path)
            config_dicts[path] = load_json_config_file(path)

        l.debug('Attempting to construct schema URI...')
        try :
            server_config_dict = config_dicts[self.get_server_config_path()]
            key = 'schema_validation_base_uri'
            base_uri = server_config_dict[key]
        except KeyError:
            base_uri = None
            l.warning('%s did not contain \'%s\'',
                self.get_server_config_path(), key)

        try :
            version_dict = config_dicts[self.get_version_path()]
            key = 'configuration_schema_version'
            uri_version = version_dict[key]
        except KeyError:
            uri_version = None
            l.warning('%s did not contain \'%s\'',
                self.get_version_path(), key)

        if base_uri and uri_version:
            validation_uri_prefix = '/'.join([
                    base_uri,
                    'v{0}'.format(uri_version)])
            l.debug('Successfully constructed schema URI.')

            for path, json_dict in config_dicts.items():
                schema_uri = '/'.join([
                    validation_uri_prefix,
                    os.path.basename(path)])
                l.debug('Attempting to validate %s against %s', path, schema_uri)
                try :
                    validate_json.validate_dict(
                            json_dict,
                            schema_uri,
                            name=path)
                except validate_json.ValidationWarning as e:
                    l.warning('Error encountered in validate_json', exc_info=True)
                except validate_json.ValidationError as e:
                    irods_six.reraise(IrodsControllerError, e, sys.exc_info()[2])
        else:
            l.debug('Failed to construct schema URI')
            l.warning('%s\n%s',
                'Preflight Check problem:',
                indent('JSON Configuration Validation failed.'))
        return config_dicts

    def check_database_schema_version(self,
            schema_version_in_db=None,
            schema_version_in_file=None):
        l = logging.getLogger(__name__)

        if schema_version_in_db is None:
            schema_version_in_db = int(
                get_db_schema_version.get_current_schema_version(
                    install_dir=self.get_top_level_directory(),
                    config_dir=self.get_config_directory(),
                    server_config_path=self.get_server_config_path(),
                    database_config_path=self.get_database_config_path()))
        l.debug('Schema version in the database is %s', schema_version_in_db)
        if schema_version_in_file is None:
            version_dict = load_json_config_file(self.get_version_path())
            schema_version_in_file = int(version_dict['catalog_schema_version'])
        l.debug('Schema version in %s is %s',
                self.get_version_path(),
                schema_version_in_file)

        if int(schema_version_in_db) > int(schema_version_in_file):
            l.error('catalog_schema_version in database [%s]', int(schema_version_in_db))
            l.error('catalog_schema_version in %s [%s]',
                    self.get_version_path(),
                    int(schema_version_in_file))
            return False
        l.info('Confirming catalog_schema_version... Success')
        return True

    def check_binary_installation(self):
        return os.path.exists(os.path.join(
            self.get_top_level_directory(),
            'packaging',
            'binary_installation.flag'))

    def get_top_level_directory(self):
        if self.top_level_directory != None:
            return self.top_level_directory
        else:
            return get_default_top_level_directory()

    def get_version_path(self):
        return os.path.join(
            self.get_top_level_directory(),
            'VERSION.json')

    def get_config_directory(self):
        if self.manually_set_config_directory != None:
            return self.manually_set_config_directory
        elif self.check_binary_installation():
            return os.path.join(
                get_root_directory(),
                'etc',
                'irods')
        else:
            return os.path.join(
                self.get_irods_directory(),
                'server',
                'config')

    def get_irods_directory(self):
        return os.path.join(
            self.get_top_level_directory(),
            'iRODS')

    def get_server_config_path(self):
        return os.path.join(
            self.get_config_directory(),
            'server_config.json')

    def get_database_config_path(self):
        return os.path.join(
            self.get_config_directory(),
            'database_config.json')

    def get_hosts_config_path(self):
        return os.path.join(
            self.get_config_directory(),
            'hosts_config.json')

    def get_host_access_control_config_path(self):
        return os.path.join(
            self.get_config_directory(),
            'host_access_control_config.json')

    def get_icommands_test_directory(self):
        return os.path.join(
            self.get_irods_directory(),
            'clients',
            'icommands',
            'test')

    def get_server_test_directory(self):
        return os.path.join(
            self.get_irods_directory(),
            'server',
            'test',
            'bin')

    def get_server_bin_directory(self):
        return os.path.join(
            self.get_irods_directory(),
            'server',
            'bin')

    def get_log_directory(self):
        return os.path.join(
            self.get_irods_directory(),
            'server',
            'log')

    def get_server_executable(self):
        return os.path.join(
            self.get_server_bin_directory(),
            'irodsServer')

    def get_rule_engine_executable(self):
        return os.path.join(
            self.get_server_bin_directory(),
            'irodsReServer')

    def get_xmsg_server_executable(self):
        return os.path.join(
            self.get_server_bin_directory(),
            'irodsXmsgServer')

    def get_agent_executable(self):
        return os.path.join(
            self.get_server_bin_directory(),
            'irodsAgent')

    def get_binary_to_pids_dict(self, binaries=None):
        if binaries is None:
            binaries = [
                self.get_server_executable(),
                self.get_rule_engine_executable(),
                self.get_xmsg_server_executable(),
                self.get_agent_executable()]
        d = {}
        for b in binaries:
            pids = get_pids_executing_binary_file(b)
            if pids:
                d[b] = pids
        return d

def format_binary_to_pids_dict(d):
    text_list = []
    for binary, pids in d.items():
        text_list.append('{0} :\n{1}'.format(
            os.path.basename(binary),
            indent(*['Process {0}'.format(pid) for pid in pids])))
    return '\n'.join(text_list)

def load_json_config_file(path):
    if os.path.exists(path):
        with open(path, 'r') as f:
            try :
                return json.load(f)
            except ValueError as e:
                irods_six.reraise(IrodsControllerError,
                        IrodsControllerError('\n\t'.join([
                            'JSON load failed for [{0}]:'.format(
                                path),
                            'Invalid JSON.',
                            '{0}: {1}'.format(
                                e.__class__.__name__, e)])),
                        sys.exc_info()[2])
    else:
        raise IrodsControllerError(
            'File {0} does not exist.'.format(
                path))

def get_default_top_level_directory():
    script_directory = os.path.dirname(os.path.abspath(
        inspect.stack()[0][1]))
    return os.path.dirname(
        os.path.dirname(
            os.path.dirname(
                script_directory)))

def get_log_path():
    return os.path.join(
        get_default_top_level_directory(),
        'log',
        'control_log.txt')

# get the fully qualified domain name
#(no, really, getfqdn() is insufficient)
def get_hostname():
    return socket.getaddrinfo(
        socket.gethostname(), 0, 0, 0, 0, socket.AI_CANONNAME)[0][3]

def get_root_directory():
    return os.path.abspath(os.sep)

def get_home_directory():
    return os.path.expanduser('~')

def get_irods_environment_path():
    return os.path.join(
        get_home_directory(),
        '.irods',
        'irods_environment.json')

def execute_command_nonblocking(args, **kwargs):
    l = logging.getLogger(__name__)
    l.debug('Calling %s with options:', args)
    if 'env' in kwargs:
        kwargs_without_env = copy.copy(kwargs)
        kwargs_without_env['env'] = 'HIDDEN'
        l.debug(pprint.pformat(kwargs_without_env))
    else :
        l.debug(pprint.pformat(kwargs))
    try :
        return subprocess.Popen(args, **kwargs)
    except OSError as e:
        irods_six.reraise(IrodsControllerError,
            IrodsControllerError('\n'.join([
                'Call to open process with {0} failed:'.format(
                    args),
                indent(
                    'Could not find the requested executable \'{0}\'; '
                    'please ensure \'{0}\' is installed and in the path.'.format(
                        args[0]))])),
            sys.exc_info()[2])

def execute_command_permissive(args, **kwargs):
    if 'stdout' not in kwargs:
        kwargs['stdout'] = subprocess.PIPE
    if 'stderr' not in kwargs:
        kwargs['stderr'] = subprocess.PIPE

    p = execute_command_nonblocking(args, **kwargs)

    out, err = communicate_and_log(p, args)
    return (out, err, p.returncode)

def check_command_return(args, out, err, returncode, **kwargs):
    if returncode is not None and returncode != 0:
        raise IrodsControllerError('\n'.join([
            'Call to open process with {0} returned an error:'.format(
                args),
            indent(
                'Options passed to Popen:',
                indent(['{0}: {1}'.format(k, v) for k, v in kwargs.items()]),
                'Return code: {0}'.format(returncode),
                'Standard output:',
                indent(out),
                'Error output:',
                indent(err))]))

def communicate_and_log(p, args):
    l = logging.getLogger(__name__)
    out, err = [t.decode() for t in p.communicate()]
    l.debug('Command %s returned with code %s.', args, p.returncode)
    if out:
        l.debug('stdout:')
        l.debug(indent(out))
    if err:
        l.debug('stderr:')
        l.debug(indent(err))
    return (out, err)

def execute_command(args, **kwargs):
    if 'stdout' not in kwargs:
        kwargs['stdout'] = subprocess.PIPE
    if 'stderr' not in kwargs:
        kwargs['stderr'] = subprocess.PIPE

    out, err, returncode = execute_command_permissive(args, **kwargs)
    check_command_return(args, out, err, returncode, **kwargs)

    return (out, err)

def get_pids_executing_binary_file(binary_file_path):
    out, err, returncode = execute_command_permissive(
            ['lsof', '-F', 'pf', binary_file_path])
    out = out if returncode == 0 else ''
    parsed_out = parse_formatted_lsof_output(out)
    try:
        # we only want pids in executing state
        return [int(d['p']) for d in parsed_out if d['f'] == 'txt']
    except (ValueError, KeyError):
        irods_six.reraise(IrodsControllerError, IrodsControllerError('\n\t'.join([
            'non-conforming lsof output:',
            '{0}'.format(out),
            '{0}'.format(err)])),
                          sys.exc_info()[2])

def parse_formatted_lsof_output(output):
    parsed_output = []
    if output.strip():
        for line in output.split():
            if line[0] == output[0]:
                parsed_output.append({})
            parsed_output[-1][line[0]] = line[1:]
    return parsed_output

def kill_pid(pid):
    p = psutil.Process(pid)
    p.suspend()
    p.terminate()
    p.kill()

def delete_cache_files_by_pid(pid):
    l = logging.getLogger(__name__)
    l.debug('Deleting cache files for pid %s...', pid)
    ubuntu_cache = glob.glob(os.path.join(
        get_root_directory(),
        'var',
        'run',
        'shm',
        '*irods_re_cache*pid{0}_*'.format(pid)))
    delete_cache_files_by_name(*ubuntu_cache)
    other_linux_cache = glob.glob(os.path.join(
        get_root_directory(),
        'dev',
        'shm',
        '*irods_re_cache*pid{0}_*'.format(pid)))
    delete_cache_files_by_name(*other_linux_cache)

def delete_cache_files_by_name(*paths):
    l = logging.getLogger(__name__)
    for path in paths:
        try:
            l.debug('Deleting %s', path)
            os.unlink(path)
        except (IOError, OSError):
            l.warning(indent('Error deleting cache file: %s'), path)

def indent(*text, **kwargs):
    if 'indentation' in kwargs:
        indentation = kwargs['indentation']
    else:
        indentation='  '
    return '\n'.join([
        ''.join([indentation, '\n{0}'.format(indentation).join(lines.splitlines())])
            for lines in text])

def parse_options():
    parser = optparse.OptionParser()

    parser.add_option('-q', '--quiet',
                      dest='verbose', action='store_false',
                      help='Silence verbose output')

    parser.add_option('-v', '--verbose',
                      dest='verbose', action='store_true', default=True,
                      help='Enable verbose output')

    parser.add_option('--irods-home-directory',
                      dest='top_level_directory',
                      default=get_default_top_level_directory(), metavar='DIR',
                      help='The directory in which the iRODS install is located; '
                      'this is the home directory of the service account in '
                      'vanilla binary installs and the top-level directory of '
                      'the build in run-in-place')

    parser.add_option('--config-directory',
                      dest='config_directory', metavar='DIR',
                      help='The directory in which the iRODS configuration files '
                      'are located; this will be /etc/irods in vanilla binary '
                      'installs and the \'config\' subdirectory of the iRODS '
                      'home directory in run-in-place')

    parser.add_option('--server-log-level',
                      dest='server_log_level', type='int', metavar='INT',
                      help='The logging level of the iRODS server')

    parser.add_option('--sql-log-level',
                      dest='sql_log_level', type='int', metavar='INT',
                      help='The database logging level')

    parser.add_option('--days-per-log',
                      dest='days_per_log', type='int', metavar='INT',
                      help='Number of days to use the same log file')

    parser.add_option('--rule-engine-server-options',
                      dest='rule_engine_server_options', metavar='OPTIONS...',
                      help='Options to be passed to the rule engine server')

    parser.add_option('--reconnect',
                      dest='server_reconnect_flag', action='store_true', default=False,
                      help='Causes the server to attempt a reconnect after '
                      'timeout (ten minutes)')

    return parser.parse_args()

def main():
    l = logging.getLogger(__name__)
    logging.getLogger().setLevel(logging.DEBUG)

    irods_logging.register_tty_handler(sys.stderr, logging.WARNING, None)
    irods_logging.register_file_handler(get_log_path())

    if (os.geteuid() == 0):
        l.error('irodsctl should not be run by the root user.')
        l.error('Exiting...')
        return 1

    operations_dict = {}
    operations_dict['start'] = lambda: irods_controller.start(
        execution_environment=execution_environment)
    operations_dict['graceful_start'] = lambda: irods_controller.start(
        execution_environment=execution_environment)
    operations_dict['stop'] = lambda: irods_controller.stop()
    operations_dict['restart'] = lambda: irods_controller.restart(
        execution_environment=execution_environment)
    operations_dict['graceful_restart'] = lambda: irods_controller.restart(
        execution_environment=execution_environment)
    operations_dict['status'] = lambda: irods_controller.status()

    (options, arguments) = parse_options()
    if len(arguments) != 1:
        l.error(
            'irodsctl accepts exactly one positional argument, '
            'but {0} were provided.'.format(len(arguments)))
        l.error('Exiting...')
        return 1

    operation = arguments[0]
    if operation not in operations_dict:
        l.error('irodsctl accepts the following positional arguments:')
        l.error(indent(operations_dict.keys()))
        l.error('but \'%s\' was provided.', operation)
        l.error('Exiting...')
        return 1

    if options.verbose:
        irods_logging.register_tty_handler(sys.stdout, logging.INFO, logging.WARNING)


    try:
        irods_controller = IrodsController(
            top_level_directory=options.top_level_directory,
            config_directory=options.config_directory)
    except IrodsControllerError:
        l.error('Error encountered creating IrodsController.', exc_info=True)
        l.error('Exiting...')
        return 1

    execution_environment = {}
    if options.server_log_level != None:
        execution_environment['spLogLevel'] = options.server_log_level
    if options.sql_log_level != None:
        execution_environment['spLogSql'] = options.sql_log_level
    if options.days_per_log != None:
        execution_environment['logfileInt'] = options.days_per_log
    if options.rule_engine_server_options != None:
        execution_environment['reServerOption'] = options.rule_engine_server_options
    if options.server_reconnect_flag:
        execution_environment['irodsReconnect'] = ''

    try:
        operations_dict[operation]()
    except IrodsControllerError:
        l.error('Error encountered running %s on irods_control.', operation, exc_info=True)
        l.error('Exiting...')
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
