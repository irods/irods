from __future__ import print_function
import contextlib
import copy
import glob
import itertools
import json
import logging
import os
import socket
import subprocess
import sys
import tempfile
import time

import psutil
from . import six

from .configuration import IrodsConfig
from . import database_connect
from . import lib

from .exceptions import IrodsError, IrodsWarning

class IrodsController(object):
    def __init__(self, irods_config=IrodsConfig()):
        self.config = irods_config

    def check_config(self):
        # load the configuration to ensure it exists
        _ = self.config.server_config
        _ = self.config.version

    def start(self):
        l = logging.getLogger(__name__)
        l.debug('Calling start on IrodsController')

        self.config.clear_cache()
        if not os.path.exists(self.config.server_executable):
            raise IrodsError(
                'Configuration problem:\n'
                '\tThe \'%s\' application could not be found.' % (
                    os.path.basename(self.config.server_executable)))

        try:
            (test_file_handle, test_file_name) = tempfile.mkstemp(
                dir=self.config.server_log_directory)
            os.close(test_file_handle)
            os.unlink(test_file_name)
        except (IOError, OSError):
            six.reraise(IrodsError, IrodsError(
                    'Configuration problem:\n'
                    'The server log directory, \'%s\''
                    'is not writeable.' % (
                        self.config.server_log_directory)),
                    sys.exc_info()[2])

        try:
            self.config.validate_configuration()
        except IrodsWarning:
            l.warn('Warning encountered in validation:', exc_info=True)

        try:
            irods_port = int(self.config.server_config['zone_port'])
            l.debug('Attempting to bind socket %s', irods_port)
            with contextlib.closing(socket.socket(socket.AF_INET, socket.SOCK_STREAM)) as s:
                try:
                    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                    s.bind(('127.0.0.1', irods_port))
                except socket.error:
                    six.reraise(IrodsError,
                            IrodsError('Could not bind port {0}.'.format(irods_port)),
                            sys.exc_info()[2])
            l.debug('Socket %s bound and released successfully.')

            if self.config.is_catalog:
                l.debug('Syncing .odbc.ini file...')
                self.config.sync_odbc_ini()

                if self.config.database_config['catalog_database_type'] == 'oracle':
                    two_task = database_connect.get_two_task_for_oracle(self.config.database_config)
                    l.debug('Setting TWO_TASK for oracle...')
                    self.config.execution_environment['TWO_TASK'] = two_task

                with contextlib.closing(self.config.get_database_connection()) as connection:
                    connection.autocommit = False
                    with contextlib.closing(connection.cursor()) as cursor:
                        if self.config.get_schema_version_in_database(cursor) < self.config.version['catalog_schema_version']:
                            try:
                                self.config.update_catalog_schema(cursor)
                                l.debug('Committing database changes...')
                                cursor.commit()
                            except:
                                cursor.rollback()
                                raise
                        if self.config.get_schema_version_in_database(cursor) > self.config.version['catalog_schema_version']:
                            l.error('catalog_schema_version in database [%d], '
                                    'catalog_schema_version in %s [%d]' % (
                                        int(schema_version_in_db),
                                        self.get_version_path(),
                                        int(schema_version_in_file)))
                            raise IrodsError('\n\t'.join([
                                'Preflight Check problem:',
                                'Database Schema in the database is ahead',
                                'of {0} - Please upgrade.'.format(
                                    os.path.basename(self.config.version_path))]))

            l.info('Starting iRODS server...')
            lib.execute_command(
                [self.config.server_executable],
                cwd=self.config.server_bin_directory,
                env=self.config.execution_environment)

            try_count = 1
            max_retries = 100
            while True:
                l.debug('Attempting to connect to iRODS server on port %s. Attempt #%s',
                        irods_port, try_count)
                with contextlib.closing(socket.socket(
                        socket.AF_INET, socket.SOCK_STREAM)) as s:
                    if s.connect_ex(('127.0.0.1', irods_port)) == 0:
                        if lib.get_pids_executing_binary_file(
                                self.config.server_executable):
                            l.debug('Successfully connected to port %s.', irods_port)
                            break
                        else:
                            try_count = max_retries
                if try_count >= max_retries:
                    raise IrodsError('iRODS server failed to start.')
                try_count += 1
                time.sleep(1)
        except IrodsError as e:
            l.info('Failure')
            six.reraise(IrodsError, e, sys.exc_info()[2])

        l.info('Success')

    def irods_grid_shutdown(self, timeout=20, **kwargs):
        l = logging.getLogger(__name__)
        args = ['irods-grid', 'shutdown', '--hosts={0}'.format(lib.get_hostname())]
        if 'IRODS_ENVIRONMENT_FILE' in self.config.execution_environment:
            kwargs['env'] = copy.copy(os.environ)
            kwargs['env']['IRODS_ENVIRONMENT_FILE'] = self.config.execution_environment['IRODS_ENVIRONMENT_FILE']
        p = lib.execute_command_nonblocking(args, **kwargs)
        start_time = time.time()
        while time.time() < start_time + timeout:
            if p.poll() is not None:
                out, err = lib.communicate_and_log(p, args)
                lib.check_command_return(args, out, err, p.returncode, **kwargs)
                break
            time.sleep(0.3)
        else:
            try:
                if p.poll() is None:
                    p.kill()
            except OSError:
                pass
            raise IrodsError(
                'The call to "irods-grid shutdown" did not complete within'
                ' {0} seconds.'.format(timeout))

        # "irods-grid shutdown" is non-blocking
        while time.time() < start_time + timeout:
            if self.get_binary_to_pids_dict([self.config.server_executable]):
                time.sleep(0.3)
            else:
                break
        else:
            raise IrodsError(
                'The iRODS server did not stop within {0} seconds of '
                'receiving the "shutdown" command.'.format(timeout))

    def stop(self, timeout=20):
        l = logging.getLogger(__name__)
        self.config.clear_cache()
        l.debug('Calling stop on IrodsController')
        l.info('Stopping iRODS server...')
        try:
            if self.get_binary_to_pids_dict([self.config.server_executable]):
                try:
                    self.irods_grid_shutdown(timeout=timeout)
                except Exception as e:
                    l.error('Error encountered in graceful shutdown.')
                    l.debug('Exception:', exc_info=True)
            else:
                    l.warning('No iRODS servers running.')

            # kill servers first to stop spawning of other processes
            server_pids_dict = self.get_binary_to_pids_dict([self.config.server_executable])
            if server_pids_dict:
                l.warning('iRODS server processes remain after "irods-grid shutdown".')
                l.warning(format_binary_to_pids_dict(server_pids_dict))
                l.warning('Killing forcefully...')
                for pid in server_pids_dict[self.config.server_executable]:
                    l.warning('Killing %s, pid %s', self.config.server_executable, pid)
                    try:
                        lib.kill_pid(pid)
                    except (psutil.NoSuchProcess, psutil.AccessDenied):
                        pass
                    delete_cache_files_by_pid(pid)

            binary_to_pids_dict = self.get_binary_to_pids_dict()
            if binary_to_pids_dict:
                l.warning('iRODS child processes remain after "irods-grid shutdown".')
                l.warning(format_binary_to_pids_dict(binary_to_pids_dict))
                l.warning('Killing forcefully...')
                for binary, pids in binary_to_pids_dict.items():
                    for pid in pids:
                        l.warning('Killing %s, pid %s', binary, pid)
                        try:
                            lib.kill_pid(pid)
                        except psutil.NoSuchProcess:
                            pass
                        delete_cache_files_by_pid(pid)
        except IrodsError as e:
            l.info('Failure')
            six.reraise(IrodsError, e, sys.exc_info()[2])

        l.info('Success')

    def restart(self):
        l = logging.getLogger(__name__)
        l.debug('Calling restart on IrodsController')
        self.stop()
        self.start()

    def status(self):
        l = logging.getLogger(__name__)
        l.debug('Calling status on IrodsController')
        self.config.clear_cache()
        binary_to_pids_dict = self.get_binary_to_pids_dict()
        if not binary_to_pids_dict:
            l.info('No iRODS servers running.')
        else:
            l.info(format_binary_to_pids_dict(binary_to_pids_dict))
        return binary_to_pids_dict

    def get_binary_to_pids_dict(self, binaries=None):
        if binaries is None:
            binaries = [
                self.config.server_executable,
                self.config.rule_engine_executable,
                self.config.xmsg_server_executable,
                self.config.agent_executable]
        d = {}
        for b in binaries:
            pids = lib.get_pids_executing_binary_file(b)
            if pids:
                d[b] = pids
        return d

def format_binary_to_pids_dict(d):
    text_list = []
    for binary, pids in d.items():
        text_list.append('{0} :\n{1}'.format(
            os.path.basename(binary),
            lib.indent(*['Process {0}'.format(pid) for pid in pids])))
    return '\n'.join(text_list)

def delete_cache_files_by_pid(pid):
    l = logging.getLogger(__name__)
    l.debug('Deleting cache files for pid %s...', pid)
    ubuntu_cache = glob.glob(os.path.join(
        lib.get_root_directory(),
        'var',
        'run',
        'shm',
        '*irods_re_cache*pid{0}_*'.format(pid)))
    delete_cache_files_by_name(*ubuntu_cache)
    other_linux_cache = glob.glob(os.path.join(
        lib.get_root_directory(),
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
            l.warning(lib.indent('Error deleting cache file: %s'), path)


