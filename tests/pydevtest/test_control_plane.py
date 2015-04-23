import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import os
import time

import configuration
import lib

class TestControlPlane(unittest.TestCase):
    def test_pause_and_resume(self):
        # test pause
        lib.assert_command('irods-grid pause --all', 'STDOUT_SINGLELINE', 'pausing')

        # need a time-out assert icommand for ils here

        # resume the server
        lib.assert_command('irods-grid resume --all', 'STDOUT_SINGLELINE', 'resuming')

        # Make sure server is actually responding
        lib.assert_command('ils', 'STDOUT_SINGLELINE', lib.get_service_account_environment_file_contents()['irods_zone_name'])
        lib.assert_command('irods-grid status --all', 'STDOUT_SINGLELINE', 'hosts')

    def test_status(self):
        # test grid status
        lib.assert_command('irods-grid status --all', 'STDOUT_SINGLELINE', 'hosts')

    @unittest.skipIf(configuration.RUN_IN_TOPOLOGY, 'Skip for Topology Testing: No way to restart grid')
    def test_shutdown(self):
        # test shutdown
        lib.assert_command('irods-grid shutdown --all', 'STDOUT_SINGLELINE', 'shutting down')
        time.sleep( 2 )
        lib.assert_command('ils', 'STDERR_SINGLELINE', 'USER_SOCK_CONNECT_ERR')

        lib.start_irods_server()
