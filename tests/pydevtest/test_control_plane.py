import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import os
import time
import pydevtest_common

class TestControlPlane(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        pass

    def test_pause_and_resume(self):
        # test pause
        pydevtest_common.assert_command('irods-grid pause --all', 'STDOUT', 'pausing')

        # need a time-out assert icommand for ils here

        # resume the server
        pydevtest_common.assert_command('irods-grid resume --all', 'STDOUT', 'resuming')

        # Make sure server is actually responding
        pydevtest_common.assert_command('ils', 'STDOUT', 'tempZone')
        pydevtest_common.assert_command('irods-grid status --all', 'STDOUT', 'hosts')

    def test_status(self):
        # test grid status
        pydevtest_common.assert_command('irods-grid status --all', 'STDOUT', 'hosts')

    @unittest.skipIf(pydevtest_common.irods_test_constants.RUN_IN_TOPOLOGY, 'Skip for Topology Testing: No way to restart grid')
    def test_shutdown(self):
        # test shutdown
        pydevtest_common.assert_command('irods-grid shutdown --all', 'STDOUT', 'shutting down')
        time.sleep( 2 )
        pydevtest_common.assert_command('ils', 'STDERR', 'USER_SOCK_CONNECT_ERR')

        pydevtest_common.restart_irods_server()
