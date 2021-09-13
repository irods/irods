import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session

class Test_Ipwd(session.make_sessions_mixin([('otherrods', 'rods')], []), unittest.TestCase):

    def setUp(self):
        super(Test_Ipwd, self).setUp()
        self.admin = self.admin_sessions[0]

    def tearDown(self):
        super(Test_Ipwd, self).tearDown()

    def test_collection_with_backslash_in_name_does_not_cause_problems__issue_4060(self):
        col = 'issue_4060_test\\folder'
        self.admin.assert_icommand(['imkdir', col])
        self.admin.assert_icommand('ils', 'STDOUT', col)
        self.admin.assert_icommand(['icd', col])
        self.admin.assert_icommand('ipwd', 'STDOUT', col)
        self.admin.assert_icommand('ils', 'STDOUT', col)

