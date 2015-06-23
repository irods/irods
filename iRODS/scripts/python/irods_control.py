from __future__ import print_function
import inspect
import os
import json
import socket
import sys
import imp
import tempfile
import subprocess
import time
import psutil
import glob
import itertools
from optparse import OptionParser
from contextlib import closing
from six import reraise
import get_db_schema_version
import validate_json

class IrodsControllerError(Exception):
    pass

class IrodsController(object):

    def __init__(self,
            top_level_directory=None,
            config_directory=None,
            static_execution_environment={},
            verbose=False):

        self.manually_set_top_level_directory = top_level_directory
        self.manually_set_config_directory = config_directory
        self.static_execution_environment = static_execution_environment
        self.verbose = verbose

        #load the configuration to ensure it exists
        load_json_config_file(self.get_server_config_path())
        load_json_config_file(self.get_version_path())

    #The default behavior is to (effectively) insert the passed execution
    #environment into the standard execution environment. If you instead
    #wish to replace the environment wholesale, set insert_behavior to False.
    def start(self, execution_environment={}, insert_behavior=True):
        server_config_dict = load_json_config_file(
                self.get_server_config_path())
        version_dict = load_json_config_file(self.get_version_path())

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

        try :
            (test_file_handle, test_file_name) = tempfile.mkstemp(
                    dir=self.get_log_directory())
            os.close(test_file_handle)
            os.unlink(test_file_name)
        except (IOError, OSError):
            reraise(IrodsControllerError, IrodsControllerError('\n\t'.join([
                    'Configuration problem:',
                    'The server log directory, \'{0}\''.format(
                        self.get_log_directory()),
                    'is not writeable.  Please change its permissions',
                    'and retry.'])),
                sys.exc_info()[2])

        if (os.path.exists(self.get_database_config_path()) and
                not self.check_database_schema_version()):
            raise IrodsControllerError('\n\t'.join([
                    'Preflight Check problem:',
                    'Database Schema in the database is ahead',
                    'of {0} - Please upgrade.'.format(
                        os.path.basename(self.get_version_path()))]))

        validation_uri_prefix = '/'.join([
            server_config_dict['schema_validation_base_uri'],
            'v{0}'.format(version_dict['configuration_schema_version'])])
        server_config_validation_uri = '/'.join([
                validation_uri_prefix,
                os.path.basename(self.get_server_config_path())])
        if server_config_validation_uri:
            try :
                validate_json.validate_dict(
                        server_config_dict,
                        server_config_validation_uri,
                        name=os.path.basename(self.get_server_config_path()),
                        verbose=self.verbose)
            except validate_json.ValidationWarning as e:
                if self.verbose:
                    print('', e, file=sys.stderr)
            except validate_json.ValidationError as e:
                reraise(IrodsControllerError, e)
        elif self.verbose:
            print(  '\nPreflight Check problem:',
                    'JSON Configuration Validation failed.',
                    sep='\n\t', file=sys.stderr)

        if self.verbose:
            print('Starting iRODS server...', end=' ')

        try :
            irods_port = int(server_config_dict['zone_port'])
            with closing(socket.socket(socket.AF_INET, socket.SOCK_STREAM)) as s:
                try :
                    s.bind(('127.0.0.1', irods_port))
                except socket.error:
                    reraise(IrodsControllerError,
                        IrodsControllerError('Could not bind port {0}.'.format(irods_port)),
                        sys.exc_info()[2])

            p = subprocess.Popen(
                [self.get_server_executable()],
                cwd=self.get_server_bin_directory(),
                env=new_execution_environment,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT)
            out, _ = p.communicate()
            if p.returncode != 0:
                raise IrodsControllerError('\n\t'.join([
                        'iRODS server failed to start.',
                        out.decode()]))

            retry_count = 100;
            while True:
                with closing(socket.socket(
                        socket.AF_INET, socket.SOCK_STREAM)) as s:
                    if s.connect_ex(('127.0.0.1', irods_port)) == 0 :
                        if get_pids_executing_binary_file(
                                self.get_server_executable()):
                            break
                        else :
                            retry_count = 0
                if retry_count <= 0:
                    raise IrodsControllerError('iRODS server failed to start.')
                retry_count = retry_count - 1
                time.sleep(1)
        except IrodsControllerError as e:
            if self.verbose:
                print('Failure')
            reraise(IrodsControllerError, e, sys.exc_info()[2])

        if self.verbose:
            print('Success')

    def stop(self):
        if self.verbose:
            print('Stopping iRODS server...', end=' ')
        try :
            process_map = self.get_processes_by_binary()
            if not [k for k, v in process_map.items() if v]:
                if self.verbose:
                    print('No iRODS servers running.', file=sys.stderr, end=' ')
            else :
                for pid in process_map[self.get_server_executable()]:
                    kill_pid(pid)
                    delete_cache_files_by_pid(pid)
                #irodsServers can spawn new processes while we're killing them
                process_map = self.get_processes_by_binary()
                for binary, pids in process_map.items():
                    for pid in pids:
                        kill_pid(pid)
                        delete_cache_files_by_pid(pid)
        except IrodsControllerError as e:
            if self.verbose:
                print('Failure')
            reraise(IrodsControllerError, e, sys.exc_info()[2])

        if self.verbose:
            print('Success')

    def restart(self, execution_environment={}, insert_behavior=True):
        self.stop()
        self.start(execution_environment=execution_environment,
                insert_behavior=insert_behavior)

    def status(self):
        process_map = self.get_processes_by_binary()
        if self.verbose:
            if not [binary for binary, pids in process_map.items() if pids]:
                    print('No iRODS servers running.')
            else :
                for binary, pids in process_map.items():
                    print(  '{0} :'.format(os.path.basename(binary)),
                            *['Process {0}'.format(pid) for pid in pids],
                            sep='\n\t')
        return process_map

    def check_database_schema_version(self):
        schema_version_in_db = int(
                get_db_schema_version.get_current_schema_version(
                    install_dir=self.get_top_level_directory(),
                    config_dir=self.get_config_directory(),
                    server_config_path=self.get_server_config_path(),
                    database_config_path=self.get_database_config_path()))
        version_dict = load_json_config_file(self.get_version_path())
        schema_version_in_file = int(version_dict['catalog_schema_version'])
        if int(schema_version_in_db) > schema_version_in_file:
            if self.verbose:
                print(
                    'catalog_schema_version in database [{0}]'.format(
                        schema_version_in_db),
                    'catalog_schema_version in {1} [{2}]'.format(
                        os.path.basename(self.get_version_path()),
                        schema_version_in_file),
                    sep='\n', file=sys.stderr)
            return False
        if self.verbose:
            print('Confirming catalog_schema_version... Success')
        return True

    def check_binary_installation(self):
        return os.path.exists(os.path.join(
            self.get_top_level_directory(),
            'packaging',
            'binary_installation.flag'))

    def get_top_level_directory(self):
        if self.manually_set_top_level_directory != None:
            return self.manually_set_top_level_directory
        else :
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
        else :
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

    def get_processes_by_binary(self):
        binaries = [
                self.get_server_executable(),
                self.get_rule_engine_executable(),
                self.get_xmsg_server_executable(),
                self.get_agent_executable()]
        return dict([(b, get_pids_executing_binary_file(b)) for b in binaries])

def load_json_config_file(filepath, permissive=False):
    if os.path.exists(filepath):
        with open(filepath) as f:
            return json.load(f)
    elif permissive:
        return {}
    else:
        raise IrodsControllerError(
            'Configuration file {0} does not exist.'.format(
                filepath))

def get_default_top_level_directory():
    script_directory = os.path.dirname(os.path.abspath(
            inspect.stack()[0][1]))
    return os.path.dirname(
            os.path.dirname(
                os.path.dirname(
                    script_directory)))

#get the fully qualified domain name
#(no, really, getfqdn() is insufficient)
def get_hostname():
    return socket.getaddrinfo(
            socket.gethostname(), 0, 0, 0, 0, socket.AI_CANONNAME)[0][3]

def get_root_directory():
    return os.path.abspath(os.sep)

def get_pids_executing_binary_file(binary_file_path):
    # get lsof listing of pids
    p = subprocess.Popen(['lsof', '-F', 'pf', binary_file_path],
            stdout=subprocess.PIPE)
    out, _ = p.communicate()
    out = out.decode() if p.returncode == 0 else ''
    parsed_out = parse_formatted_lsof_output(out)
    try:
        # we only want pids in executing state
        return [int(d['p']) for d in parsed_out if d['f'] == 'txt']
    except (ValueError, KeyError):
        reraise(IrodsControllerError, IrodsControllerError('\n\t'.join([
                'non-conforming lsof output:',
                '{0}'.format(out)])),
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

def delete_cache_files_by_pid(pid, verbose=False):
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
        for path in paths:
            try:
                os.unlink(path)
            except (IOError, OSError):
                if verbose:
                    print('\tError deleting cache file: {0}'.format(path),
                            file=sys.stderr)

def parse_options():

    parser = OptionParser()

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
            dest='rule_engine_server_options', metavar='"OPTIONS..."',
            help='Options to be passed to the rule engine server')

    parser.add_option('--reconnect',
            dest='server_reconnect_flag', action='store_true', default=False,
            help='Causes the server to attempt a reconnect after '
            'timeout (ten minutes)')

    return parser.parse_args()


if __name__ == '__main__':

    if ( os.geteuid() == 0 ) :
        print(  'irodsctl should not be run by the root user.',
                'Exiting...',
                sep='\n', file=sys.stderr)
        sys.exit(1)

    operations_dict = {}
    operations_dict['start'] = lambda : irods_controller.start(
            execution_environment=execution_environment)
    operations_dict['graceful_start'] = lambda : irods_controller.start(
            execution_environment=execution_environment)
    operations_dict['stop'] = lambda : irods_controller.stop()
    operations_dict['restart'] = lambda : irods_controller.restart(
            execution_environment=execution_environment)
    operations_dict['graceful_restart'] = lambda : irods_controller.restart(
            execution_environment=execution_environment)
    operations_dict['status'] = lambda : irods_controller.status()

    (options, arguments) = parse_options()
    if len(arguments) != 1:
        print(  'irodsctl accepts exactly one positional argument, '
                'but {0} were provided.'.format(len(arguments)),
                'Exiting...',
                sep='\n', file=sys.stderr)
        sys.exit(1)

    operation = arguments[0]
    if operation not in operations_dict:
        print(  'irodsctl accepts the following positional arguments:',
                '\n\t'.join(operations_dict.keys()),
                'but \'{0}\' was provided.'.format(operation),
                'Exiting...',
                sep='\n', file=sys.stderr)
        sys.exit(1)

    try :
        irods_controller = IrodsController(
                top_level_directory=options.top_level_directory,
                config_directory=options.config_directory,
                verbose=options.verbose)
    except IrodsControllerError as e:
        print('', e, 'Exiting...', sep='\n', file=sys.stderr)
        sys.exit(1)

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

    try :
        operations_dict[operation]()
    except IrodsControllerError as e:
        print('', e, 'Exiting...', sep='\n', file=sys.stderr)
        sys.exit(1)
