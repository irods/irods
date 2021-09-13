import unittest

from . import session

class Test_ihelp(unittest.TestCase):
    def setUp(self):
        super(Test_ihelp, self).setUp()

    def tearDown(self):
        super(Test_ihelp, self).tearDown()

    def test_local_ihelp(self):
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand('ihelp', 'STDOUT_SINGLELINE', 'The iCommands and a brief description of each:')

    def test_local_ihelp_with_help(self):
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("ihelp -h", 'STDOUT_SINGLELINE', "Display iCommands synopsis")

    def test_local_ihelp_all(self):
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("ihelp -a", 'STDOUT_SINGLELINE', "Usage")

    def test_local_ihelp_with_good_icommand(self):
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("ihelp ils", 'STDOUT_SINGLELINE', "Usage")

    def test_local_ihelp_with_bad_icommand(self):
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand_fail("ihelp idoesnotexist")

    def test_local_ihelp_with_bad_option(self):
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand_fail("ihelp -z")
