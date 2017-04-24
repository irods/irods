from __future__ import print_function
import sys
if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import os

from . import session
from . import settings
from . import resource_suite
from .. import lib

class Test_Ils(resource_suite.ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_Ils, self).setUp()

    def tearDown(self):
        super(Test_Ils, self).tearDown()

    def test_ils_of_multiple_data_objects__issue_3520(self):
        filename_1 = 'test_ils_of_multiple_data_objects__issue_3520_1'
        filename_2 = 'test_ils_of_multiple_data_objects__issue_3520_2'
        lib.make_file(filename_1, 1024, 'arbitrary')
        lib.make_file(filename_2, 1024, 'arbitrary')
        rods_filename_1 = self.admin.session_collection + '/' + filename_1
        rods_filename_2 = self.admin.session_collection + '/' + filename_2
        self.admin.assert_icommand(['iput', filename_1, rods_filename_1])
        self.admin.assert_icommand(['iput', filename_2, rods_filename_2])
        self.admin.assert_icommand(['ils', '-l', rods_filename_1, rods_filename_2], 'STDOUT_MULTILINE', filename_1, filename_2)
        os.system('rm -f ' + filename_1)
        os.system('rm -f ' + filename_2)
