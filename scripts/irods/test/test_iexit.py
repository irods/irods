import unittest

from . import session

class Test_iexit(unittest.TestCase):
    def setUp(self):
        super(Test_iexit, self).setUp()

    def tearDown(self):
        super(Test_iexit, self).tearDown()

    def test_iexit(self):
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iexit")

    def test_iexit_full_does_not_delete_auth_file_if_user_is_the_service_account_user__issue_6136(self):
        with session.make_session_for_existing_admin() as admin_session:
            expected_output = 'WARNING: iexit appears to be running as service account. Skipping auth file deletion (pass -f to force).'
            admin_session.assert_icommand("iexit full", 'STDOUT', expected_output)

    def test_iexit_verbose(self):
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iexit -v", 'STDOUT_SINGLELINE', "Deleting (if it exists) session envFile:")

    def test_iexit_with_bad_option(self):
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand_fail("iexit -z")
