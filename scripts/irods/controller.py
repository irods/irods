from __future__ import print_function
import contextlib
import copy
import glob
import itertools
import json
import logging
import os
import shutil
import socket
import subprocess
import sys
import tempfile
import time

from collections import OrderedDict

import psutil
from . import six

from .configuration import IrodsConfig
from . import lib
from . import paths
from . import upgrade_configuration

from .exceptions import IrodsError, IrodsWarning

class IrodsController(object):
    def __init__(self, irods_config=IrodsConfig()):
        self.config = irods_config

    def check_config(self):
        # load the configuration to ensure it exists
        _ = self.config.server_config
        _ = self.config.version

    @property
    def server_binaries(self):
        return [
            self.config.server_executable,
            self.config.rule_engine_executable
        ]

    def define_log_levels(self, logger):
        config = self.config.server_config

        # If the log levels are not defined, then add them to server config!
        if 'log_level' not in config or 'server' not in config['log_level']:
            logger.debug('Adding log levels to server configuration ...')
            lib.update_json_file_from_dict(paths.server_config_path(), {
                'log_level': {
                    'agent': 'info',
                    'agent_factory': 'info',
                    'api': 'info',
                    'authentication': 'info',
                    'database': 'info',
                    'delay_server': 'info',
                    'legacy': 'info',
                    'microservice': 'info',
                    'network': 'info',
                    'resource': 'info',
                    'rule_engine': 'info',
                    'server': 'info'
                }
            })

    def get_server_proc(self):
        server_pid = lib.get_server_pid()

        # lib.get_server_pid() does not have access to self.config, so cannot
        # check the pid from the pidfile against self.config.server_executable,
        # only that a process with that pid exists. The possibility remains
        # that the pid may have been recycled.
        if server_pid >= 0:
            try:
                server_proc = psutil.Process(server_pid)
                if server_proc.exe() and os.path.samefile(self.config.server_executable, server_proc.exe()):
                    return server_proc
            except (psutil.NoSuchProcess, psutil.AccessDenied):
                return None

        return None

    def start(self, write_to_stdout=False, test_mode=False):
        l = logging.getLogger(__name__)
        l.debug('Calling start on IrodsController')

        if upgrade_configuration.requires_upgrade(self.config):
            upgrade_configuration.upgrade(self.config)

        self.define_log_levels(l)

        try:
            self.config.validate_configuration()
        except IrodsWarning:
            l.warn('Warning encountered in validation:', exc_info=True)

        if self.get_server_proc():
            raise IrodsError('iRODS already running')

        self.config.clear_cache()
        if not os.path.exists(self.config.server_executable):
            raise IrodsError(
                'Configuration problem:\n'
                '\tThe \'%s\' application could not be found.' % (
                    os.path.basename(self.config.server_executable)))

        try:
            (test_file_handle, test_file_name) = tempfile.mkstemp(
                dir=self.config.log_directory)
            os.close(test_file_handle)
            os.unlink(test_file_name)
        except (IOError, OSError):
            six.reraise(IrodsError, IrodsError(
                    'Configuration problem:\n'
                    'The server log directory, \'%s\''
                    'is not writeable.' % (
                        self.config.log_directory)),
                    sys.exc_info()[2])

        for f in ['core.re', 'core.dvm', 'core.fnm']:
            path = os.path.join(self.config.config_directory, f)
            if not os.path.exists(path):
                shutil.copyfile(paths.get_template_filepath(path), path)

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
            l.debug('Socket %s bound and released successfully.', irods_port)

            if self.config.is_catalog:
                from . import database_interface
                database_interface.server_launch_hook(self.config)

            cmd = [self.config.server_executable]

            if write_to_stdout:
                l.info('Starting iRODS server in foreground ...')

                cmd.append('-u')

                env_var_name = 'IRODS_ENABLE_TEST_MODE'
                if test_mode or (env_var_name in os.environ and os.environ[env_var_name] == '1'):
                    cmd.append('-t')

                lib.execute_command(cmd,
                                    stdout=sys.stdout,
                                    stderr=sys.stdout,
                                    cwd=self.config.server_bin_directory,
                                    env=self.config.execution_environment)
            else:
                l.info('Starting iRODS server ...')

                env_var_name = 'IRODS_ENABLE_TEST_MODE'
                if test_mode or (env_var_name in os.environ and os.environ[env_var_name] == '1'):
                    cmd.append('-t')

                lib.execute_command(cmd,
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
                            l.debug('Successfully connected to port %s.', irods_port)
                            if self.get_server_proc is None:
                                raise IrodsError('iRODS port is bound, but server is not started.')
                            s.send(b'\x00\x00\x00\x33<MsgHeader_PI><type>HEARTBEAT</type></MsgHeader_PI>')
                            message = s.recv(256)
                            if message != b'HEARTBEAT':
                                raise IrodsError('iRODS port returned non-heartbeat message:\n{0}'.format(message))
                            break
                    if try_count >= max_retries:
                        raise IrodsError('iRODS server failed to start.')
                    try_count += 1
                    time.sleep(1)

                l.info('Success')

        except IrodsError as e:
            l.info('Failure')
            six.reraise(IrodsError, e, sys.exc_info()[2])

    def irods_graceful_shutdown(self, server_proc, server_descendants, timeout=20):
        start_time = time.time()
        server_proc.terminate()
        while time.time() < start_time + timeout:
            if capture_process_tree(server_proc, server_descendants, self.server_binaries):
                time.sleep(0.3)
            else:
                break
        else:
            capture_process_tree(server_proc, server_descendants, self.server_binaries)
            if server_proc.is_running():
                raise IrodsError('The iRODS server did not stop within {0} seconds of '
                                 'receiving SIGTERM.'.format(timeout))
            elif server_descendants:
                raise IrodsError('iRODS server shut down but left behind child processes')

    def stop(self, timeout=20):
        l = logging.getLogger(__name__)
        self.config.clear_cache()
        l.debug('Calling stop on IrodsController')
        l.info('Stopping iRODS server...')
        try:
            server_proc = self.get_server_proc()
            server_descendants = set()
            if server_proc is None:
                l.warning('Server pidfile missing or stale. No iRODS server running?')
            else:
                try:
                    self.irods_graceful_shutdown(server_proc, server_descendants, timeout=timeout)
                except Exception as e:
                    l.error('Error encountered in graceful shutdown.')
                    l.debug('Exception:', exc_info=True)

            # deal with lingering processes
            binary_to_procs_dict = self.get_binary_to_procs_dict(server_proc, server_descendants)
            if binary_to_procs_dict:
                l.warning('iRODS server processes remain after attempted graceful shutdown.')
                l.warning(format_binary_to_procs_dict(binary_to_procs_dict))
                l.warning('Killing forcefully...')
                for binary, procs in binary_to_procs_dict.items():
                    for proc in procs:
                        l.warning('Killing %s, pid %s', binary, proc.pid)
                        try:
                            proc.kill()
                        except psutil.NoSuchProcess:
                            pass

        except IrodsError as e:
            l.info('Failure')
            six.reraise(IrodsError, e, sys.exc_info()[2])

        l.info('Success')

    def restart(self, write_to_stdout=False, test_mode=False):
        l = logging.getLogger(__name__)
        l.debug('Calling restart on IrodsController')
        self.stop()
        self.start(write_to_stdout, test_mode)

    def status(self):
        l = logging.getLogger(__name__)
        l.debug('Calling status on IrodsController')
        self.config.clear_cache()
        server_proc = self.get_server_proc()
        if server_proc is None:
            l.info('No iRODS servers running.')
        else:
            l.info(format_binary_to_procs_dict(self.get_binary_to_procs_dict(server_proc)))

    def get_binary_to_procs_dict(self, server_proc, server_descendants=None, binaries=None):
        if server_descendants is None and server_proc is not None and server_proc.is_running():
            try:
                server_descendants = server_proc.children(recursive=True)
            except psutil.NoSuchProcess:
                return None
        if server_descendants is None:
            server_descendants = []
        server_descendants = sorted(server_descendants, key=lambda _p: _p.pid)
        if binaries is None:
            binaries = self.server_binaries
        d = OrderedDict()
        for b in binaries:
            procs = list(filter(lambda _p: binary_matches(b, _p), server_descendants))
            if server_proc is not None and binary_matches(b, server_proc):
                procs.insert(0, server_proc)
            if procs:
                d[b] = procs
        return d

def binary_matches(binary_path, proc):
    if proc.is_running():
        try:
            if proc.exe():
                return os.path.samefile(binary_path, proc.exe())
            else:
                return os.path.basename(binary_path) == proc.name()
        except psutil.NoSuchProcess:
            return False

def capture_process_tree(server_proc, server_descendants, candidate_binaries=None):
    # define func to filter to candidate binaries
    if candidate_binaries:
        def should_return_proc(_p):
            for b in candidate_binaries:
                if binary_matches(b, _p):
                    return True
            return False
    else:
        def should_return_proc(_p):
            return True

    if server_proc.is_running():
        try:
            cur_descendants = filter(should_return_proc, server_proc.children(recursive=True))
            orphaned_descendants = server_descendants.difference(cur_descendants)
            server_descendants.update(cur_descendants)
        except (psutil.NoSuchProcess):
            orphaned_descendants = server_descendants.copy()
    else:
        # if server isn't running any more, all previously known descendants are orphaned
        orphaned_descendants = server_descendants.copy()

    # get new descendants of orphans
    for orphaned_descendant in orphaned_descendants:
        if orphaned_descendant.is_running():
            try:
                server_descendants.update(filter(should_return_proc, orphaned_descendant.children(recursive=True)))
            except (psutil.NoSuchProcess):
                server_descendants.discard(orphaned_descendant)
        else:
            # remove descendants that are no longer running
            server_descendants.discard(orphaned_descendant)

    return server_proc.is_running() or server_descendants

def format_binary_to_procs_dict(proc_dict):
    text_list = []
    for binary, procs in proc_dict.items():
        text_list.append('{0} :\n{1}'.format(
            os.path.basename(binary),
            lib.indent(*['Process {0}'.format(proc.pid) for proc in procs])))
    return '\n'.join(text_list)
