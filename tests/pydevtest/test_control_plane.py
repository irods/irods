import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import pydevtest_common

class TestControlPlane(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        pass

    def test_all_option(self):
        pydevtest_common.assert_command('irods-grid pause --all', 'STDOUT', 'pausing')
        pydevtest_common.assert_command('irods-grid resume --all', 'STDOUT', 'resuming')
        pydevtest_common.assert_command('irods-grid status --all', 'STDOUT', 'hosts')
