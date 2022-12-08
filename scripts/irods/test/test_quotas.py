import os
import re
import sys
import textwrap
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


class Test_Quotas(resource_suite.ResourceBase, unittest.TestCase):
    plugin_name = IrodsConfig().default_rule_engine_plugin
    class_name = 'Test_Quotas'

    def setUp(self):
        super(Test_Quotas, self).setUp()

    def tearDown(self):
        super(Test_Quotas, self).tearDown()

    def test_iquota__3044(self):
        pep_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                acRescQuotaPolicy {
                    msiSetRescQuotaPolicy("on");
                }
            '''),
            'irods_rule_engine_plugin-python': textwrap.dedent('''
                def acRescQuotaPolicy(rule_args, callback, rei):
                    callback.msiSetRescQuotaPolicy('on')
            ''')
        }

        filename_1 = 'test_iquota__3044_1'
        filename_2 = 'test_iquota__3044_2'
        with temporary_core_file() as core:
            core.add_rule(pep_map[self.plugin_name])
            for quotatype in [['sgq', 'public']]: # group
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

    def test_iquota_empty__3048(self):
        cmd = 'iadmin suq' # no arguments
        self.admin.assert_icommand(cmd.split(), 'STDERR_SINGLELINE', 'ERROR: missing username parameter') # usage information
        cmd = 'iadmin sgq' # no arguments
        self.admin.assert_icommand(cmd.split(), 'STDERR_SINGLELINE', 'ERROR: missing group name parameter') # usage information

    def test_filter_out_groups_when_selecting_user__issue_3507(self):
        self.admin.assert_icommand(['igroupadmin', 'mkgroup', 'test_group_3507'])

        try:
            # Attempt to set user quota passing in the name of a group; should fail
            self.admin.assert_icommand(['iadmin', 'suq', 'test_group_3507', 'demoResc', '0'], 'STDERR_SINGLELINE', 'CAT_INVALID_USER')

        finally:
            self.admin.assert_icommand(['iadmin', 'rmgroup', 'test_group_3507'])
