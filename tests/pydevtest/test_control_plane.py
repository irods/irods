import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import os
import time

import configuration
import lib


class Test_ControlPlane(unittest.TestCase):
    def test_pause_and_resume(self):
        lib.assert_command('irods-grid pause --all', 'STDOUT_SINGLELINE', 'pausing')

        # need a time-out assert icommand for ils here

        lib.assert_command('irods-grid resume --all', 'STDOUT_SINGLELINE', 'resuming')

        # Make sure server is actually responding
        lib.assert_command('ils', 'STDOUT_SINGLELINE', lib.get_service_account_environment_file_contents()['irods_zone_name'])
        lib.assert_command('irods-grid status --all', 'STDOUT_SINGLELINE', 'hosts')

    def test_status(self):
        lib.assert_command('irods-grid status --all', 'STDOUT_SINGLELINE', 'hosts')

    def test_hosts_separator(self):
        for s in [',', ', ']:
            hosts_string = s.join([lib.get_hostname()]*2)
            lib.assert_command(['irods-grid', 'status', '--hosts', hosts_string], 'STDOUT_SINGLELINE', lib.get_hostname())

    @unittest.skipIf(configuration.RUN_IN_TOPOLOGY, 'Skip for Topology Testing: No way to restart grid')
    def test_shutdown(self):
        assert lib.re_shm_exists()

        lib.assert_command('irods-grid shutdown --all', 'STDOUT_SINGLELINE', 'shutting down')
        time.sleep(2)
        lib.assert_command('ils', 'STDERR_SINGLELINE', 'USER_SOCK_CONNECT_ERR')

        assert not lib.re_shm_exists(), lib.re_shm_exists()

        lib.start_irods_server()

    def test_shutdown_local_server(self):
        initial_size_of_server_log = lib.get_log_size('server')
        lib.assert_command(['irods-grid', 'shutdown', '--hosts', lib.get_hostname()], 'STDOUT_SINGLELINE', 'shutting down')
        time.sleep(10) # server gives control plane the all-clear before printing done message
        assert 1 == lib.count_occurrences_of_string_in_log('server', 'iRODS Server is done', initial_size_of_server_log)
        lib.start_irods_server()
