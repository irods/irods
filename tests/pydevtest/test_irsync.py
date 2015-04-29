import os
import re
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import configuration
from resource_suite import ResourceBase
import lib


class Test_iRsync(ResourceBase, unittest.TestCase):
    def setUp(self):
        super(Test_iRsync, self).setUp()

    def tearDown(self):
        super(Test_iRsync, self).tearDown()

    def test_irsync(self):
        filepath = os.path.join(self.admin.local_session_dir, 'file')
        lib.make_file( filepath, 1 )
        self.admin.assert_icommand('iput ' + filepath)
        self.admin.assert_icommand('irsync -l ' + filepath + ' i:file')
