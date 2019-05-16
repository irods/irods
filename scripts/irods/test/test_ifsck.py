import os
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from .. import test
from .. import lib

class Test_Ifsck(session.make_sessions_mixin([('otherrods', 'rods')], []), unittest.TestCase):

    def setUp(self):
        super(Test_Ifsck, self).setUp()
        self.admin = self.admin_sessions[0]

    def tearDown(self):
        super(Test_Ifsck, self).tearDown()

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_ifsck_has_integer_overflow__issue_4391(self):
        # Create a file that is exactly 2G in size.
        file_2g = '2g_issue_4391.txt'
        lib.make_file(file_2g, 2147483648, 'arbitrary')

        # Create another file that is one byte smaller than 2G in size.
        file_just_under_2g = 'just_under_2g_issue_4391.txt'
        lib.make_file(file_just_under_2g, 2147483647, 'arbitrary')

        # Create a collection and put the large files into it.
        col = 'ifsck_issue_4391'
        self.admin.assert_icommand(['imkdir', col])
        self.admin.assert_icommand(['iput', file_2g, os.path.join(col, file_2g)])
        self.admin.assert_icommand(['iput', file_just_under_2g, os.path.join(col, file_just_under_2g)])

        # Verify that the data objects exist in iRODS under the correct location.
        physical_path_to_col = os.path.join(self.admin.get_vault_session_path(), col)
        self.admin.assert_icommand(['ils', '-L', col], 'STDOUT_MULTILINE', [os.path.join(physical_path_to_col, file_2g),
                                                                            os.path.join(physical_path_to_col, file_just_under_2g)])

        # TEST: This should not produce any messages.
        self.admin.assert_icommand(['ifsck', '-r', physical_path_to_col])

