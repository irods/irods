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
        os.system('rm -f ' + filename_1)
        os.system('rm -f ' + filename_2)

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

