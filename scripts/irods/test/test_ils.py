from __future__ import print_function

import sys
import os

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from . import settings
from . import resource_suite
from .. import lib

class Test_Ils(resource_suite.ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_Ils, self).setUp()

    def tearDown(self):
        super(Test_Ils, self).tearDown()

    def test_ilsresc_long_confirm_class_not_printed__issue_5115(self):
        self.admin.assert_icommand_fail(['ilsresc', '-l', 'demoResc'], 'STDOUT_SINGLELINE', 'class: ')

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

    def test_genquery_fallback_for_collection_permissions__issue_4636(self):
        # Remove the specific query. This will cause the fallback to be used.
        self.admin.assert_icommand(['iadmin', 'rsq', 'ShowCollAcls'])

        self.admin.assert_icommand(['ils', '-A'], 'STDOUT', 'own')

        # Restore the specific query.
        specific_query = ("select distinct"
                          " R_USER_MAIN.user_name,"
                          " R_USER_MAIN.zone_name,"
                          " R_TOKN_MAIN.token_name,"
                          " R_USER_MAIN.user_type_name "
                          "from"
                          " R_USER_MAIN,"
                          " R_TOKN_MAIN,"
                          " R_OBJT_ACCESS,"
                          " R_COLL_MAIN "
                          "where"
                          " R_OBJT_ACCESS.object_id = R_COLL_MAIN.coll_id AND"
                          " R_COLL_MAIN.coll_name = ? AND"
                          " R_TOKN_MAIN.token_namespace = 'access_type' AND"
                          " R_USER_MAIN.user_id = R_OBJT_ACCESS.user_id AND"
                          " R_OBJT_ACCESS.access_type_id = R_TOKN_MAIN.token_id")
        self.admin.assert_icommand(['iadmin', 'asq', specific_query, 'ShowCollAcls'])

    def test_ils_lists_collections_without_listing_their_contents__issue_5506(self):
        # Create some data objects.
        self.admin.assert_icommand(['istream', 'write', 'foo'], input='foo')
        self.admin.assert_icommand(['istream', 'write', 'bar'], input='bar')
        self.admin.assert_icommand(['istream', 'write', 'baz'], input='baz')

        # Show that the data objects exist.
        # The leading whitespace and trailing newline help to guarantee we find
        # the correct information.
        self.admin.assert_icommand(['ils'], 'STDOUT', ['  foo\n', '  bar\n', '  baz\n'])

        # Show that "ils -d" only prints the collection.
        ec, out, err = self.admin.assert_icommand(['ils', '-d'], 'STDOUT_SINGLELINE', [self.admin.session_collection + ':'])
        self.assertEqual(ec, 0)
        self.assertEqual(out, self.admin.session_collection + ':\n')
        self.assertEqual(len(err), 0)

    def test_option_d_with_collections__issue_5506(self):
        # Show that the -l option is ignored.
        ec, out, err = self.admin.assert_icommand(['ils', '-d', '-l'], 'STDOUT', [self.admin.session_collection + ':'])
        self.assertEqual(ec, 0)
        self.assertEqual(out, self.admin.session_collection + ':\n')
        self.assertEqual(len(err), 0)

        # Show that the -L option is ignored.
        ec, out, err = self.admin.assert_icommand(['ils', '-d', '-L'], 'STDOUT', [self.admin.session_collection + ':'])
        self.assertEqual(ec, 0)
        self.assertEqual(out, self.admin.session_collection + ':\n')
        self.assertEqual(len(err), 0)

        # Show that the -A option is honored.
        expected_output = [self.admin.session_collection, ' ACL - ' + self.admin.username + '#' + self.admin.zone_name + ':own']
        self.admin.assert_icommand(['ils', '-d', '-A'], 'STDOUT', expected_output)

    def test_option_d_with_data_objects__issue_5506(self):
        data_object = os.path.join(self.admin.session_collection, 'foo')
        self.admin.assert_icommand(['istream', 'write', data_object], input='foo')

        # Show that only the data object's path is printed.
        ec, out, err = self.admin.assert_icommand(['ils', '-d', data_object], 'STDOUT', ['  ' + data_object])
        self.assertEqual(ec, 0)
        self.assertEqual(out, '  ' + data_object + '\n')
        self.assertEqual(len(err), 0)

        # Show that only the data object's path is printed.
        ec, out, err = self.admin.assert_icommand(['ils', '-d', '-r', data_object], 'STDOUT', ['  ' + data_object])
        self.assertEqual(ec, 0)
        self.assertEqual(out, '  ' + data_object + '\n')
        self.assertEqual(len(err), 0)

        # Show that the -L option is honored.
        expected_output = [os.path.basename(data_object), self.admin.username, ' & ', ' generic ']
        self.admin.assert_icommand(['ils', '-d', '-L', data_object], 'STDOUT', expected_output)

        # Show that the -A option is honored.
        expected_output = ['  ' + data_object, ' ACL - ' + self.admin.username + '#' + self.admin.zone_name + ':own']
        self.admin.assert_icommand(['ils', '-d', '-A', data_object], 'STDOUT', expected_output)

    def test_option_l_or_L_with_three_or_more_data_objects_does_not_cause_the_agent_to_crash__issue_5502(self):
        data_object = 'issue_5502'
        self.admin.assert_icommand(['itouch', data_object])
        self.admin.assert_icommand(['ils', '-l', data_object, data_object, data_object], 'STDOUT', [data_object])
        self.admin.assert_icommand(['ils', '-L', data_object, data_object, data_object], 'STDOUT', [data_object])

