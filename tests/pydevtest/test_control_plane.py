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
        lib.assert_command('irods-grid pause --all', 'STDOUT', 'pausing')

        # need a time-out assert icommand for ils here

        # resume the server
        lib.assert_command('irods-grid resume --all', 'STDOUT', 'resuming')

        # Make sure server is actually responding
        lib.assert_command('ils', 'STDOUT', lib.get_service_account_environment_file_contents()['irods_zone_name'])
        lib.assert_command('irods-grid status --all', 'STDOUT', 'hosts')

    def test_status(self):
        # test grid status
        lib.assert_command('irods-grid status --all', 'STDOUT', 'hosts')

    @unittest.skipIf(configuration.RUN_IN_TOPOLOGY, 'Skip for Topology Testing: No way to restart grid')
    def test_shutdown(self):
        # test shutdown
        lib.assert_command('irods-grid shutdown --all', 'STDOUT', 'shutting down')
        time.sleep( 2 )
        lib.assert_command('ils', 'STDERR', 'USER_SOCK_CONNECT_ERR')

        lib.start_irods_server()
