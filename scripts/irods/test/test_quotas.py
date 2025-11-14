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
from . import session
from .. import lib
from .. import paths
from ..core_file import temporary_core_file
from ..configuration import IrodsConfig
from ..controller import IrodsController

class Test_Quotas(resource_suite.ResourceBase, unittest.TestCase):
    plugin_name = IrodsConfig().default_rule_engine_plugin
    class_name = 'Test_Quotas'

    def setUp(self):
        super(Test_Quotas, self).setUp()

        self.quota_user = session.mkuser_and_return_session('rodsuser', 'quotauser', 'quotapass', lib.get_hostname())

    def tearDown(self):
        self.quota_user.__exit__()
        self.admin.assert_icommand(['iadmin', 'rmuser', 'quotauser'])
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
        try:
            with temporary_core_file() as core:
                core.add_rule(pep_map[self.plugin_name])
                IrodsController().reload_configuration()

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
                        cmd = 'istream -R {0} write nopes'.format(self.testresc)
                        self.admin.assert_icommand(cmd.split(), 'STDERR', 'Error: Cannot open data object.', input='some data')
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

        finally:
            IrodsController().reload_configuration()

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
    
    # The following test covers case 10, 11, 12
    def test_physical_quota_on_single_resource_with_multiple_groups__issue_8691(self):
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

        try:
            with temporary_core_file() as core:
                core.add_rule(pep_map[self.plugin_name])
                IrodsController().reload_configuration()

                group_name = 'test_group_case_10_11_12'
                file_name = 'test_file_case_10_11_12'
                resc_name = 'ufs0_quota_case_10_11_12'
                lib.make_file(os.path.join(self.quota_user.local_session_dir, file_name), 4096, contents='arbitrary')
                lib.create_ufs_resource(self.admin, resc_name)

                self.admin.assert_icommand(['iadmin', 'mkgroup', f'{group_name}_1'])
                self.admin.assert_icommand(['iadmin', 'mkgroup', f'{group_name}_2'])
                self.admin.assert_icommand(['iadmin', 'sgq', f'{group_name}_1', resc_name, '5000'])
                self.admin.assert_icommand(['iadmin', 'sgq', f'{group_name}_2', resc_name, '10000'])

                self.admin.assert_icommand(['iadmin', 'atg', f'{group_name}_1', self.quota_user.username])
                self.admin.assert_icommand(['iadmin', 'atg', f'{group_name}_2', self.quota_user.username])

                self.quota_user.assert_icommand(['iput', '-R', self.testresc, os.path.join(self.quota_user.local_session_dir, file_name)])
                self.admin.assert_icommand(['iadmin', 'cu'])
                self.admin.assert_icommand(['iadmin', 'lq'], 'STDOUT', ['\nquota_over: -5000\n', '\nquota_over: -10000\n'])

                # Case 10: under quota succeeds
                with self.subTest(testcase=10, desc="Under quota before and after"):
                    self.quota_user.assert_icommand(['icp', '-R', resc_name, file_name, f'{file_name}_case_10'])
                    self.admin.assert_icommand(['iadmin', 'cu'])
                    self.admin.assert_icommand(['iadmin', 'lq'], 'STDOUT', ['\nquota_over: -904\n', '\nquota_over: -5904\n'])

                # Case 11: crossing quota succeeds
                with self.subTest(testcase=11, desc="Crossing quota"):
                    self.quota_user.assert_icommand(['icp', '-R', resc_name, file_name, f'{file_name}_case_11'])
                    self.admin.assert_icommand(['iadmin', 'cu'])
                    self.admin.assert_icommand(['iadmin', 'lq'], 'STDOUT', ['\nquota_over: 3192\n', '\nquota_over: -1808\n'])

                # Case 12: should fail with just a single quota violated
                with self.subTest(testcase=12, desc="Violating quota"):
                    self.quota_user.assert_icommand(['icp', '-R', resc_name, file_name, f'{file_name}_case_12'], 'STDERR_SINGLELINE', 'SYS_RESC_QUOTA_EXCEEDED')
                    self.admin.assert_icommand(['iadmin', 'lq'], 'STDOUT', ['\nquota_over: 3192\n', '\nquota_over: -1808\n'])
        finally:
            self.admin.run_icommand(['irm', '-f', f'{file_name}_case_10', f'{file_name}_case_11', f'{file_name}_case_12'])
            self.admin.run_icommand(['iadmin', 'rmresc', resc_name])
            self.admin.run_icommand(['iadmin', 'rmgroup', f'{group_name}_1'])
            self.admin.run_icommand(['iadmin', 'rmgroup', f'{group_name}_2'])
            IrodsController().reload_configuration()

    # The following test covers case 40, 41, 42
    def test_physical_resource_and_total_quota_simultaneously__issue_4089(self):
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

        try:
            with temporary_core_file() as core:
                core.add_rule(pep_map[self.plugin_name])
                IrodsController().reload_configuration()

                group_name = 'test_group_case_40_41_42'
                file_name = 'test_file_case_40_41_42'
                resc_name = 'ufs0_quota_case_40_41_42'
                lib.make_file(os.path.join(self.quota_user.local_session_dir, file_name), 4096, contents='arbitrary')
                lib.create_ufs_resource(self.admin, resc_name)

                self.admin.assert_icommand(['iadmin', 'mkgroup', f'{group_name}_1'])
                self.admin.assert_icommand(['iadmin', 'sgq', f'{group_name}_1', resc_name, '5000'])
                self.admin.assert_icommand(['iadmin', 'sgq', f'{group_name}_1', 'total', '15000'])

                self.admin.assert_icommand(['iadmin', 'atg', f'{group_name}_1', self.quota_user.username])

                self.quota_user.assert_icommand(['iput', '-R', self.testresc, os.path.join(self.quota_user.local_session_dir, file_name)])
                self.admin.assert_icommand(['iadmin', 'cu'])
                self.admin.assert_icommand(['iadmin', 'lq'], 'STDOUT', ['\nquota_over: -5000\n', '\nquota_over: -10904\n'])

                # Case 40: under quota succeeds
                with self.subTest(testcase=40, desc="Under quota before and after"):
                    self.quota_user.assert_icommand(['icp', '-R', resc_name, file_name, f'{file_name}_case_40'])
                    self.admin.assert_icommand(['iadmin', 'cu'])
                    self.admin.assert_icommand(['iadmin', 'lq'], 'STDOUT', ['\nquota_over: -904\n', '\nquota_over: -6808\n'])

                # Case 41: crossing quota succeeds
                with self.subTest(testcase=41, desc="Crossing quota"):
                    self.quota_user.assert_icommand(['icp', '-R', resc_name, file_name, f'{file_name}_case_41'])
                    self.admin.assert_icommand(['iadmin', 'cu'])
                    self.admin.assert_icommand(['iadmin', 'lq'], 'STDOUT', ['\nquota_over: 3192\n', '\nquota_over: -2712\n'])

                # Case 42: should fail with just a single quota violated, regardless of which
                with self.subTest(testcase=42, desc="Violating resc quota"):
                    self.quota_user.assert_icommand(['icp', '-R', resc_name, file_name, f'{file_name}_case_42'], 'STDERR_SINGLELINE', 'SYS_RESC_QUOTA_EXCEEDED')

                # Swap quotas to check other quota being violated
                with self.subTest(testcase=42, desc="Violating total quota"):
                    self.admin.assert_icommand(['iadmin', 'sgq', f'{group_name}_1', resc_name, '10000'])
                    self.admin.assert_icommand(['iadmin', 'sgq', f'{group_name}_1', 'total', '10000'])
                    self.admin.assert_icommand(['iadmin', 'cu'])
                    self.admin.assert_icommand(['iadmin', 'lq'], 'STDOUT', ['\nquota_over: -1808\n', '\nquota_over: 2288\n'])
                    self.quota_user.assert_icommand(['icp', '-R', resc_name, file_name, f'{file_name}_case_42a'], 'STDERR_SINGLELINE', 'SYS_RESC_QUOTA_EXCEEDED')
                    self.admin.assert_icommand(['iadmin', 'lq'], 'STDOUT', ['\nquota_over: -1808\n', '\nquota_over: 2288\n'])
        finally:
            self.admin.run_icommand(['irm', '-f', f'{file_name}_case_40', f'{file_name}_case_41', f'{file_name}_case_42', f'{file_name}_case_42a'])
            self.admin.run_icommand(['iadmin', 'rmresc', resc_name])
            self.admin.run_icommand(['iadmin', 'rmgroup', f'{group_name}_1'])
            IrodsController().reload_configuration()

    def test_single_resource_quota_violation_does_not_affect_other_resources__issue_8758(self):
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

        try:
            with temporary_core_file() as core:
                core.add_rule(pep_map[self.plugin_name])
                IrodsController().reload_configuration()

                group_name = 'test_group_issue_8758'
                file_name = 'test_file_issue_8758'
                resc_name = 'ufs0_quota_issue_8758'
                other_resc_name = 'ufs1_quota_issue_8758'
                lib.make_file(os.path.join(self.quota_user.local_session_dir, file_name), 4096, contents='arbitrary')
                lib.create_ufs_resource(self.admin, resc_name)
                lib.create_ufs_resource(self.admin, other_resc_name)

                self.admin.assert_icommand(['iadmin', 'mkgroup', f'{group_name}_1'])
                self.admin.assert_icommand(['iadmin', 'sgq', f'{group_name}_1', resc_name, '1000'])

                self.admin.assert_icommand(['iadmin', 'atg', f'{group_name}_1', self.quota_user.username])

                self.quota_user.assert_icommand(['iput', '-R', self.testresc, os.path.join(self.quota_user.local_session_dir, file_name)])

                self.quota_user.assert_icommand(['icp', '-R', resc_name, file_name, f'{file_name}_on_{resc_name}'])
                self.admin.assert_icommand(['iadmin', 'cu'])
                self.admin.assert_icommand(['iadmin', 'lq'], 'STDOUT', ['\nquota_over: 3096\n'])

                # Should pass and be unaffected by the other quota violation
                self.quota_user.assert_icommand(['icp', '-R', other_resc_name, file_name, f'{file_name}_on_{other_resc_name}'])

                self.admin.assert_icommand(['iadmin', 'cu'])
                self.admin.assert_icommand(['iadmin', 'lq'], 'STDOUT', ['\nquota_over: 3096\n'])

        finally:
            self.admin.run_icommand(['irm', '-f', f'{file_name}_on_{other_resc_name}', f'{file_name}_on_{resc_name}'])
            self.admin.run_icommand(['iadmin', 'rmresc', resc_name])
            self.admin.run_icommand(['iadmin', 'rmresc', other_resc_name])
            self.admin.run_icommand(['iadmin', 'rmgroup', f'{group_name}_1'])
            IrodsController().reload_configuration()

    # The following test covers case 3, 4, 5
    def test_physical_resource_with_coordinating_resources__issue_8667(self):
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

        try:
            with temporary_core_file() as core:
                core.add_rule(pep_map[self.plugin_name])
                IrodsController().reload_configuration()

                group_name = 'test_group_case_3_4_5'
                file_name = 'test_file_case_3_4_5'
                resc_name = 'ufs0_quota_case_3_4_5'
                other_resc_name = 'ufs1_quota_case_3_4_5'
                repl_resc_name = 'repl_quota_case_3_4_5'
                passthru_resc_name = 'passthru_quota_case_3_4_5'
                lib.make_file(os.path.join(self.quota_user.local_session_dir, file_name), 4096, contents='arbitrary')
                lib.create_ufs_resource(self.admin, resc_name)
                lib.create_ufs_resource(self.admin, other_resc_name)
                lib.create_replication_resource(self.admin, repl_resc_name)
                lib.create_passthru_resource(self.admin, passthru_resc_name)

                self.admin.assert_icommand(['iadmin', 'addchildtoresc', repl_resc_name, resc_name])
                self.admin.assert_icommand(['iadmin', 'addchildtoresc', repl_resc_name, other_resc_name])
                self.admin.assert_icommand(['iadmin', 'addchildtoresc', passthru_resc_name, repl_resc_name])

                self.admin.assert_icommand(['iadmin', 'mkgroup', f'{group_name}_1'])
                self.admin.assert_icommand(['iadmin', 'sgq', f'{group_name}_1', repl_resc_name, '20000'])
                self.admin.assert_icommand(['iadmin', 'sgq', f'{group_name}_1', passthru_resc_name, '10000'])
                self.admin.assert_icommand(['iadmin', 'atg', f'{group_name}_1', self.quota_user.username])

                self.quota_user.assert_icommand(['iput', '-R', self.testresc, os.path.join(self.quota_user.local_session_dir, file_name)])
                self.admin.assert_icommand(['iadmin', 'cu'])
                self.admin.assert_icommand(['iadmin', 'lq'], 'STDOUT', ['\nquota_over: -10000\n', '\nquota_over: -20000\n'])
                # Case 3: under quota succeeds
                with self.subTest(testcase=3, desc="Under quota before and after"):
                    self.quota_user.assert_icommand(['icp', '-R', passthru_resc_name, file_name, f'{file_name}_case_3'])
                    self.admin.assert_icommand(['iadmin', 'cu'])
                    self.admin.assert_icommand(['iadmin', 'lq'], 'STDOUT', ['\nquota_over: -1808\n', '\nquota_over: -11808\n'])

                # Case 4: crossing quota succeeds
                with self.subTest(testcase=4, desc="Crossing quota"):
                    self.quota_user.assert_icommand(['icp', '-R', passthru_resc_name, file_name, f'{file_name}_case_4'])
                    self.admin.assert_icommand(['iadmin', 'cu'])
                    self.admin.assert_icommand(['iadmin', 'lq'], 'STDOUT', ['\nquota_over: 6384\n', '\nquota_over: -3616\n'])

                # Case 5: violating quota fails
                with self.subTest(testcase=5, desc="Violating resc quota"):
                    self.quota_user.assert_icommand(['icp', '-R', passthru_resc_name, file_name, f'{file_name}_case_5'], 'STDERR_SINGLELINE', 'SYS_RESC_QUOTA_EXCEEDED')
                    self.admin.assert_icommand(['iadmin', 'lq'], 'STDOUT', ['\nquota_over: 6384\n', '\nquota_over: -3616\n'])
        finally:
            self.admin.run_icommand(['irm', '-f', f'{file_name}_case_3', f'{file_name}_case_4', f'{file_name}_case_5'])
            self.admin.run_icommand(['iadmin', 'rmchildfromresc', repl_resc_name, resc_name])
            self.admin.run_icommand(['iadmin', 'rmchildfromresc', repl_resc_name, other_resc_name])
            self.admin.run_icommand(['iadmin', 'rmchildfromresc', passthru_resc_name, repl_resc_name])
            self.admin.run_icommand(['iadmin', 'rmresc', resc_name])
            self.admin.run_icommand(['iadmin', 'rmresc', other_resc_name])
            self.admin.run_icommand(['iadmin', 'rmresc', repl_resc_name])
            self.admin.run_icommand(['iadmin', 'rmresc', passthru_resc_name])
            self.admin.run_icommand(['iadmin', 'rmgroup', f'{group_name}_1'])
            IrodsController().reload_configuration()
