from __future__ import print_function
import os
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session

class Test_Imkdir(session.make_sessions_mixin([('otherrods', 'rods')], []), unittest.TestCase):

    def setUp(self):
        super(Test_Imkdir, self).setUp()
        self.admin = self.admin_sessions[0]

    def tearDown(self):
        super(Test_Imkdir, self).tearDown()

    def test_imkdir_does_not_create_collections_under_root_collection_due_to_substring_bug__issue_4621(self):
        rods = session.make_session_for_existing_admin()
        rods.assert_icommand_fail(['imkdir', os.path.join("/", rods.zone_name + 'Nopes')], 'STDOUT', [
            'SYS_INVALID_INPUT_PARAM',
            'a valid zone name does not appear at the root of the object path'
        ])
