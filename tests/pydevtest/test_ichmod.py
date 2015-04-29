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


class Test_iChmod(ResourceBase, unittest.TestCase):
    def setUp(self):
        super(Test_iChmod, self).setUp()

    def tearDown(self):
        super(Test_iChmod, self).tearDown()

    def test_ichmod_r(self):
        self.admin.assert_icommand('imkdir -p sub_dir1\\\\%/subdir2/')
        self.admin.assert_icommand('ichmod read ' + self.user0.username + ' -r sub_dir1\\\\%/')
        self.admin.assert_icommand('ichmod inherit -r sub_dir1\\\\%/')
        filepath = os.path.join(self.admin.local_session_dir, 'file')
        lib.make_file( filepath, 1 )
        self.admin.assert_icommand('iput ' + filepath + ' sub_dir1\\\\%/subdir2/')
        self.user0.assert_icommand('iget ' + self.admin.session_collection + '/sub_dir1\\\\%/subdir2/file ' + os.path.join(self.user0.local_session_dir, ''))
