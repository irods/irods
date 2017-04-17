import inspect
import os
import re
import sys
import time

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import resource_suite
from .. import lib
from .. import paths
from ..core_file import temporary_core_file
from ..configuration import IrodsConfig
from .rule_texts_for_tests import rule_texts


class Test_Quotas(resource_suite.ResourceBase, unittest.TestCase):
    plugin_name = IrodsConfig().default_rule_engine_plugin
    class_name = 'Test_Quotas'

    def setUp(self):
        super(Test_Quotas, self).setUp()

    def tearDown(self):
        super(Test_Quotas, self).tearDown()

    def test_iquota__3044(self):
        filename_1 = 'test_iquota__3044_1'
        filename_2 = 'test_iquota__3044_2'
        with temporary_core_file() as core:
            time.sleep(1)  # remove once file hash fix is committed #2279
            core.add_rule(rule_texts[self.plugin_name][self.class_name][inspect.currentframe().f_code.co_name])
            time.sleep(1)  # remove once file hash fix is committed #2279
            for quotatype in [['suq',self.admin.username], ['sgq','public']]: # user and group
                for quotaresc in [self.testresc, 'total']: # resc and total
                    cmd = 'iadmin {0} {1} {2} 10000000'.format(quotatype[0], quotatype[1], quotaresc) # set high quota
                    self.admin.assert_icommand(cmd.split())
                    cmd = 'iquota'
                    self.admin.assert_icommand(cmd.split(), 'STDOUT_SINGLELINE', 'Nearing quota') # not over yet
                    lib.make_file(filename_1, 1024, contents='arbitrary')
                    cmd = 'iput -R {0} {1}'.format(self.testresc, filename_1) # should succeed
                    self.admin.assert_icommand(cmd.split())
                    cmd = 'iadmin cu' # calculate, update db
                    self.admin.assert_icommand(cmd.split())
                    cmd = 'iquota'
                    self.admin.assert_icommand(cmd.split(), 'STDOUT_SINGLELINE', 'Nearing quota') # not over yet
                    cmd = 'iadmin {0} {1} {2} 40'.format(quotatype[0], quotatype[1], quotaresc) # set low quota
                    self.admin.assert_icommand(cmd.split())
                    cmd = 'iquota'
                    self.admin.assert_icommand(cmd.split(), 'STDOUT_SINGLELINE', 'OVER QUOTA') # confirm it's over
                    lib.make_file(filename_2, 1024, contents='arbitrary')
                    cmd = 'iput -R {0} {1}'.format(self.testresc, filename_2) # should fail
                    self.admin.assert_icommand(cmd.split(), 'STDERR_SINGLELINE', 'SYS_RESC_QUOTA_EXCEEDED')
                    cmd = 'iadmin {0} {1} {2} 0'.format(quotatype[0], quotatype[1], quotaresc) # remove quota
                    self.admin.assert_icommand(cmd.split())
                    cmd = 'iadmin cu' # update db
                    self.admin.assert_icommand(cmd.split())
                    cmd = 'iput -R {0} {1}'.format(self.testresc, filename_2) # should succeed again
                    self.admin.assert_icommand(cmd.split())
                    cmd = 'irm -rf {0}'.format(filename_1) # clean up
                    self.admin.assert_icommand(cmd.split())
                    cmd = 'irm -rf {0}'.format(filename_2) # clean up
                    self.admin.assert_icommand(cmd.split())
            time.sleep(2)  # remove once file hash fix is commited #2279
 
    def test_iquota_empty__3048(self):
        cmd = 'iadmin suq' # no arguments
        self.admin.assert_icommand(cmd.split(), 'STDERR_SINGLELINE', 'ERROR: missing username parameter') # usage information
        cmd = 'iadmin sgq' # no arguments
        self.admin.assert_icommand(cmd.split(), 'STDERR_SINGLELINE', 'ERROR: missing group name parameter') # usage information

    def test_iquota_u_updates_usage__issue_3508(self):
        filename = 'test_quota_u_updates_usage__issue_3508'
        lib.make_file(filename, 1024, contents='arbitrary')
        self.admin.assert_icommand(['iadmin', 'suq', self.admin.username, 'demoResc', '10000000'])
        self.admin.assert_icommand(['iput', filename])
        self.admin.assert_icommand(['iadmin', 'cu'])
        # In 4.2.0, iadmin cu does not actually update the r_quota_usage table, so will remain at -10,000,000
        # When fixed, will show the actual value, and so the string below will not match and the assert will fail
        self.admin.assert_icommand_fail(['iquota', '-u', self.admin.username], 'STDOUT_SINGLELINE', 'Over:  -10,000,000 (-10 million) bytes')

    def test_filter_out_groups_when_selecting_user__issue_3507(self):
        self.admin.assert_icommand(['igroupadmin', 'mkgroup', 'test_group_3507'])
        # Attempt to set user quota passing in the name of a group; should fail
        self.admin.assert_icommand(['iadmin', 'suq', 'test_group_3507', 'demoResc', '10000000'], 'STDERR_SINGLELINE', 'CAT_INVALID_USER')

