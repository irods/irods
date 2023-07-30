from __future__ import print_function
import sys
if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from . import settings
from . import resource_suite

class Test_Ipasswd(resource_suite.ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_Ipasswd, self).setUp()

    def tearDown(self):
        # Fix up bad password attempts in the tests
        self.user0.assert_icommand('iinit', 'STDOUT_SINGLELINE', 'Enter your current iRODS password:', input='apass\n')
        self.admin.assert_icommand('iinit', 'STDOUT_SINGLELINE', 'Enter your current iRODS password:', input='rods\n')
        super(Test_Ipasswd, self).tearDown()

    # The tests are divided by "should succeed" and "should fail"
    # order to help manage the passwords for otherrods (self.admin)
    # and alice (self.user0).  All tests are repeated - once for
    # rodsadmin, and again for rodsuser.

    # ==========================================================================================================
    # Section dealing with RODSUSER (successful ipasswd tests)
    # ==========================================================================================================

    def test_ipasswd_rodsuser_should_succeed(self):

        # self.admin is otherrods - rodsadmin - passwd: rods
        # self.user0 is alice - rodsuser - passwd: apass

        # Note:  Beware of leaving otherrods or alice with unknown (to the framework) passwords.


        ####################################
        # Current password on command line, new password and reentry are valid.
        #
        expected_lines = [
                            'WARNING: Error 25 disabling echo mode. Password will be displayed in plaintext.',
                            'Enter your new iRODS password:',
                            'Reenter your new iRODS password:'
                         ]
        self.user0.assert_icommand('ipasswd apass', 'STDOUT_MULTILINE', expected_lines,  input='newapass\nnewapass\n', desired_rc=0)


        ####################################
        # No current password on the command line, ipasswd challenges for current password, new password, and reentry. all valid entries
        #
        expected_lines = [
                            'WARNING: Error 25 disabling echo mode. Password will be displayed in plaintext.',
                            'Enter your current iRODS password:',
                            'WARNING: Error 25 disabling echo mode. Password will be displayed in plaintext.',
                            'Enter your new iRODS password:',
                            'Reenter your new iRODS password:'
                         ]
        self.user0.assert_icommand('ipasswd', 'STDOUT_MULTILINE', expected_lines,  input='newapass\naapass\naapass\n', desired_rc=0)


        ####################################
        # Good password on third retry, and identical password on the reentry. Should be ok.
        #
        expected_lines = [
                            'WARNING: Error 25 disabling echo mode. Password will be displayed in plaintext.',
                            'Enter your current iRODS password:',
                            'WARNING: Error 25 disabling echo mode. Password will be displayed in plaintext.',
                            'Enter your new iRODS password:',
                            'Your password must be between 3 and 42 characters long.',
                            'Enter your new iRODS password:',
                            'Your password must be between 3 and 42 characters long.',
                            'Enter your new iRODS password:',
                            'Reenter your new iRODS password:'
                         ]
        self.user0.assert_icommand('ipasswd', 'STDOUT_MULTILINE', expected_lines,  input='aapass\nxx\nxx\napass\napass\n', desired_rc=0)


    # ==========================================================================================================
    # Section dealing with RODSUSER (failed ipasswd tests)
    # ==========================================================================================================

    def test_ipasswd_rodsuser_should_fail(self):

        ####################################
        # End of file instead of the password reentry.
        #
        expected_lines = [
                            'WARNING: Error 25 disabling echo mode. Password will be displayed in plaintext.',
                            'Enter your current iRODS password:',
                            'WARNING: Error 25 disabling echo mode. Password will be displayed in plaintext.',
                            'Enter your new iRODS password:',
                            'Reenter your new iRODS password:End of file encountered.'
                         ]
        self.user0.assert_icommand('ipasswd', 'STDOUT_MULTILINE', expected_lines,  input='apass\napass\n', desired_rc=8)


        ####################################
        # Three retries of short password, then abort
        #
        expected_lines = [
                            'WARNING: Error 25 disabling echo mode. Password will be displayed in plaintext.',
                            'Enter your current iRODS password:',
                            'WARNING: Error 25 disabling echo mode. Password will be displayed in plaintext.',
                            'Enter your new iRODS password:',
                            'Your password must be between 3 and 42 characters long.',
                            'Enter your new iRODS password:',
                            'Your password must be between 3 and 42 characters long.',
                            'Enter your new iRODS password:',
                            'Your password must be between 3 and 42 characters long.',
                            'Invalid password. Aborting...'
                         ]
        self.user0.assert_icommand('ipasswd', 'STDOUT_MULTILINE', expected_lines,  input='apass\nxx\nxx\nxx\nxx\n', desired_rc=8)


        ####################################
        # Three retries of long password, then abort
        #
        expected_lines = [
                            'WARNING: Error 25 disabling echo mode. Password will be displayed in plaintext.',
                            'Enter your current iRODS password:',
                            'WARNING: Error 25 disabling echo mode. Password will be displayed in plaintext.',
                            'Enter your new iRODS password:',
                            'Your password must be between 3 and 42 characters long.',
                            'Enter your new iRODS password:',
                            'Your password must be between 3 and 42 characters long.',
                            'Enter your new iRODS password:',
                            'Your password must be between 3 and 42 characters long.',
                            'Invalid password. Aborting...'
                         ]
        password = "1234567890123456789012345678901234567890123"
        self.user0.assert_icommand('ipasswd', 'STDOUT_MULTILINE', expected_lines,  input='apass\n' + password + '\n' + password + '\n' + password + '\n', desired_rc=8)


        ####################################
        # Bad password on the command line
        #
        expected_lines = [
                            'CAT_INVALID_AUTHENTICATION',
                            '826000'
                         ]
        self.user0.assert_icommand('ipasswd BAD_PASSWD', 'STDERR_MULTILINE', expected_lines, input='apass\napass\n', desired_rc=7)

        # Bad passwords that come from the database destroy the session.  So correct.
        self.user0.assert_icommand('iinit', 'STDOUT_SINGLELINE', 'Enter your current iRODS password:', input='apass\n')


        ####################################
        # password too short, and only three chances total to get it right.
        # Make sure you don't get more than three chances.
        #
        expected_lines = [
                            'WARNING: Error 25 disabling echo mode. Password will be displayed in plaintext.',
                            'Enter your current iRODS password:',
                            'WARNING: Error 25 disabling echo mode. Password will be displayed in plaintext.',
                            'Enter your new iRODS password:',
                            'Your password must be between 3 and 42 characters long.',
                            'Enter your new iRODS password:',
                            'Your password must be between 3 and 42 characters long.',
                            'Enter your new iRODS password:',
                            'Your password must be between 3 and 42 characters long.',
                            'Invalid password. Aborting...'
                         ]
        self.user0.assert_icommand('ipasswd', 'STDOUT_MULTILINE', expected_lines,  input='apass\nxx\nxx\nxx\nnewapass\n', desired_rc=8)


        ####################################
        # Good password on third retry, but empty (end-of-file) response on the reentry.
        #
        expected_lines = [
                            'WARNING: Error 25 disabling echo mode. Password will be displayed in plaintext.',
                            'Enter your current iRODS password:',
                            'WARNING: Error 25 disabling echo mode. Password will be displayed in plaintext.',
                            'Enter your new iRODS password:',
                            'Your password must be between 3 and 42 characters long.',
                            'Enter your new iRODS password:',
                            'Your password must be between 3 and 42 characters long.',
                            'Enter your new iRODS password:',
                            'Reenter your new iRODS password:End of file encountered.'
                         ]
        self.user0.assert_icommand('ipasswd', 'STDOUT_MULTILINE', expected_lines,  input='apass\nxx\nxx\nnewapass\n', desired_rc=8)


        ####################################
        # Good password on third retry, but different password on the reentry to cause a mismatch.
        #
        expected_lines = [
                            'WARNING: Error 25 disabling echo mode. Password will be displayed in plaintext.',
                            'Enter your current iRODS password:',
                            'WARNING: Error 25 disabling echo mode. Password will be displayed in plaintext.',
                            'Enter your new iRODS password:',
                            'Your password must be between 3 and 42 characters long.',
                            'Enter your new iRODS password:',
                            'Your password must be between 3 and 42 characters long.',
                            'Enter your new iRODS password:',
                            'Reenter your new iRODS password:',
                            'Entered passwords do not match'
                         ]
        self.user0.assert_icommand('ipasswd', 'STDOUT_MULTILINE', expected_lines,  input='apass\nxx\nxx\nnewapass\nBAD_REENTRY\n', desired_rc=8)


        ####################################
        # No password on command line; bad password on challenge
        #
        # Have to use a different mechanism here, since we get output on both
        # stdout and stderr.

        stdout, stderr, rc = self.user0.run_icommand("ipasswd", input='BAD_PASSWD\napass\n')

        self.assertEqual(rc, 7, "ipasswd: failed rc = {0} (expected 7)".format(rc))

        str1 = "WARNING: Error 25 disabling echo mode. Password will be displayed in plaintext."
        self.assertIn(str1, stdout, "ipasswd: Expected stdout: \"...{0}...\", got: \"{1}\"".format(str1, stdout))

        # and...

        str1 = "Enter your current iRODS password:"
        self.assertIn(str1, stdout, "ipasswd: Expected stdout: \"...{0}...\", got: \"{1}\"".format(str1, stdout))

        # and...

        str1 = "CAT_INVALID_AUTHENTICATION: failed to perform request"
        self.assertIn(str1, stderr, "ipasswd: Expected stderr: \"...{0}...\", got: \"{1}\"".format(str1, stderr))

    # ==========================================================================================================
    # Section dealing with RODSADMIN (successful ipasswd tests)
    # ==========================================================================================================

    def test_ipasswd_rodsadmin_should_succeed(self):

        # Note:  Beware of leaving otherrods or alice with unknown (to the framework) passwords.


        ####################################
        # Current password on command line, new password and reentry are valid.
        #
        expected_lines = [
                            'WARNING: Error 25 disabling echo mode. Password will be displayed in plaintext.',
                            'Enter your new iRODS password:',
                            'Reenter your new iRODS password:'
                         ]
        self.admin.assert_icommand('ipasswd rods', 'STDOUT_MULTILINE', expected_lines,  input='newrods\nnewrods\n', desired_rc=0)


        ####################################
        # No current password on the command line, ipasswd challenges for current password, new password, and reentry. all valid entries
        #
        expected_lines = [
                            'WARNING: Error 25 disabling echo mode. Password will be displayed in plaintext.',
                            'Enter your current iRODS password:',
                            'WARNING: Error 25 disabling echo mode. Password will be displayed in plaintext.',
                            'Enter your new iRODS password:',
                            'Reenter your new iRODS password:'
                         ]
        self.admin.assert_icommand('ipasswd', 'STDOUT_MULTILINE', expected_lines,  input='newrods\narods\narods\n', desired_rc=0)


        ####################################
        # Good password on third retry, and identical password on the reentry. Should be ok.
        #
        expected_lines = [
                            'WARNING: Error 25 disabling echo mode. Password will be displayed in plaintext.',
                            'Enter your current iRODS password:',
                            'WARNING: Error 25 disabling echo mode. Password will be displayed in plaintext.',
                            'Enter your new iRODS password:',
                            'Your password must be between 3 and 42 characters long.',
                            'Enter your new iRODS password:',
                            'Your password must be between 3 and 42 characters long.',
                            'Enter your new iRODS password:',
                            'Reenter your new iRODS password:'
                         ]
        self.admin.assert_icommand('ipasswd', 'STDOUT_MULTILINE', expected_lines,  input='arods\nxx\nxx\nrods\nrods\n', desired_rc=0)


    # ==========================================================================================================
    # Section dealing with RODSADMIN (failed ipasswd tests)
    # ==========================================================================================================

    def test_ipasswd_rodsadmin_should_fail(self):

        ####################################
        # End of file instead of the password reentry.
        #
        expected_lines = [
                            'WARNING: Error 25 disabling echo mode. Password will be displayed in plaintext.',
                            'Enter your current iRODS password:',
                            'WARNING: Error 25 disabling echo mode. Password will be displayed in plaintext.',
                            'Enter your new iRODS password:',
                            'Reenter your new iRODS password:End of file encountered.'
                         ]
        self.admin.assert_icommand('ipasswd', 'STDOUT_MULTILINE', expected_lines,  input='rods\nrods\n', desired_rc=8)


        ####################################
        # Three retries of short password, then abort
        #
        expected_lines = [
                            'WARNING: Error 25 disabling echo mode. Password will be displayed in plaintext.',
                            'Enter your current iRODS password:',
                            'WARNING: Error 25 disabling echo mode. Password will be displayed in plaintext.',
                            'Enter your new iRODS password:',
                            'Your password must be between 3 and 42 characters long.',
                            'Enter your new iRODS password:',
                            'Your password must be between 3 and 42 characters long.',
                            'Enter your new iRODS password:',
                            'Your password must be between 3 and 42 characters long.',
                            'Invalid password. Aborting...'
                         ]
        self.admin.assert_icommand('ipasswd', 'STDOUT_MULTILINE', expected_lines,  input='rods\nxx\nxx\nxx\nxx\n', desired_rc=8)


        ####################################
        # Bad password on the command line
        #
        expected_lines = [
                            'CAT_INVALID_AUTHENTICATION',
                            '826000'
                         ]
        self.admin.assert_icommand('ipasswd BAD_PASSWD', 'STDERR_MULTILINE', expected_lines, input='rods\nrods\n', desired_rc=7)

        # Bad passwords that come from the database destroy the session.  So correct.
        self.admin.assert_icommand('iinit', 'STDOUT_SINGLELINE', 'Enter your current iRODS password:', input='rods\n')


        ####################################
        # password too short, and only three chances total to get it right.
        # Make sure you don't get more than three chances.
        #
        expected_lines = [
                            'WARNING: Error 25 disabling echo mode. Password will be displayed in plaintext.',
                            'Enter your current iRODS password:',
                            'WARNING: Error 25 disabling echo mode. Password will be displayed in plaintext.',
                            'Enter your new iRODS password:',
                            'Your password must be between 3 and 42 characters long.',
                            'Enter your new iRODS password:',
                            'Your password must be between 3 and 42 characters long.',
                            'Enter your new iRODS password:',
                            'Your password must be between 3 and 42 characters long.',
                            'Invalid password. Aborting...'
                         ]
        self.admin.assert_icommand('ipasswd', 'STDOUT_MULTILINE', expected_lines,  input='rods\nxx\nxx\nxx\nnewrods\n', desired_rc=8)


        ####################################
        # Good password on third retry, but empty (end-of-file) response on the reentry.
        #
        expected_lines = [
                            'WARNING: Error 25 disabling echo mode. Password will be displayed in plaintext.',
                            'Enter your current iRODS password:',
                            'WARNING: Error 25 disabling echo mode. Password will be displayed in plaintext.',
                            'Enter your new iRODS password:',
                            'Your password must be between 3 and 42 characters long.',
                            'Enter your new iRODS password:',
                            'Your password must be between 3 and 42 characters long.',
                            'Enter your new iRODS password:',
                            'Reenter your new iRODS password:End of file encountered.'
                         ]
        self.admin.assert_icommand('ipasswd', 'STDOUT_MULTILINE', expected_lines,  input='rods\nxx\nxx\nnewrods\n', desired_rc=8)


        ####################################
        # Good password on third retry, but different password on the reentry to cause a mismatch.
        #
        expected_lines = [
                            'WARNING: Error 25 disabling echo mode. Password will be displayed in plaintext.',
                            'Enter your current iRODS password:',
                            'WARNING: Error 25 disabling echo mode. Password will be displayed in plaintext.',
                            'Enter your new iRODS password:',
                            'Your password must be between 3 and 42 characters long.',
                            'Enter your new iRODS password:',
                            'Your password must be between 3 and 42 characters long.',
                            'Enter your new iRODS password:',
                            'Reenter your new iRODS password:',
                            'Entered passwords do not match'
                         ]
        self.admin.assert_icommand('ipasswd', 'STDOUT_MULTILINE', expected_lines,  input='rods\nxx\nxx\nnewrods\nBAD_REENTRY\n', desired_rc=8)


        ####################################
        # No password on command line; bad password on challenge
        #
        # Have to use a different mechanism here, since we get output on both
        # stdout and stderr.

        stdout, stderr, rc = self.admin.run_icommand("ipasswd", input='BAD_PASSWD\nrods\n')

        self.assertEqual(rc, 7, "ipasswd: failed rc = {0} (expected 7)".format(rc))

        str1 = "WARNING: Error 25 disabling echo mode. Password will be displayed in plaintext."
        self.assertIn(str1, stdout, "ipasswd: Expected stdout: \"...{0}...\", got: \"{1}\"".format(str1, stdout))

        # and...

        str1 = "Enter your current iRODS password:"
        self.assertIn(str1, stdout, "ipasswd: Expected stdout: \"...{0}...\", got: \"{1}\"".format(str1, stdout))

        # and...

        str1 = "CAT_INVALID_AUTHENTICATION: failed to perform request"
        self.assertIn(str1, stderr, "ipasswd: Expected stderr: \"...{0}...\", got: \"{1}\"".format(str1, stderr))
