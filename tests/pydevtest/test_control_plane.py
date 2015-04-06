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
        pydevtest_common.assert_command('ils', 'STDOUT', 'tempZone')

    def test_status(self):
        # test grid status
        pydevtest_common.assert_command('irods-grid status --all', 'STDOUT', 'hosts')

    def test_shutdown(self):
        # test shutdown
        pydevtest_common.assert_command('irods-grid shutdown --all', 'STDOUT', 'shutting down')
        time.sleep( 2 )
        pydevtest_common.assert_command('ils', 'STDERR', 'USER_SOCK_CONNECT_ERR')

        pydevtest_common.restart_irods_server()
