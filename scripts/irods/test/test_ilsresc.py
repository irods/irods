from __future__ import print_function
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session

admins = [('otherrods', 'rods')]
users  = []

class Test_Ilsresc(session.make_sessions_mixin(admins, users), unittest.TestCase):

    def setUp(self):
        super(Test_Ilsresc, self).setUp()
        self.admin = self.admin_sessions[0]

    def tearDown(self):
        super(Test_Ilsresc, self).tearDown()

    def test_ilsresc_does_not_print_CAT_NO_ROWS_FOUND_at_page_boundary__issue_5155(self):
        # NOTE: This test assumes that the zone only contains two resources (i.e. demoResc and bundleResc).
        #       This test will fail if there are more than two resources.

        # "ilsresc" does not include the bundleResc in its output, so subtract one from
        # the GenQuery integer result.
        number_of_resources_to_add = 49
        resc_name_prefix = 'pt_5155_'

        try:
            # Create enough resources to hit the page boundary of ilsresc.
            for i in range(number_of_resources_to_add):
                self.admin.assert_icommand(['iadmin', 'mkresc', resc_name_prefix + str(i), 'passthru'], 'STDOUT', ['passthru'])

            # Show that iquest does not print an error when the total number of resources
            # is a multiple of the GenQuery page size.
            _, err, ec = self.admin.run_icommand(['ilsresc', '-l'])
            self.assertEqual(ec, 0)
            self.assertNotIn('rcGenQuery failed with error -808000 CAT_NO_ROWS_FOUND', err)

        finally:
            # Remove the resources.
            for i in range(number_of_resources_to_add):
                self.admin.run_icommand(['iadmin', 'rmresc', resc_name_prefix + str(i)])

    def test_ilsresc_reports_an_error_on_unknown_zone__issue_6022(self):
        self.admin.assert_icommand(['ilsresc', '-z', 'unknown_zone_issue_6022'], 'STDERR', ['-26000 SYS_INVALID_ZONE_NAME'])

