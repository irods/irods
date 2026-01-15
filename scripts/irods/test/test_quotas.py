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

        self.pep_to_enable_resc_quota_policy = {
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
        self.quota_user = session.mkuser_and_return_session('rodsuser', 'quotauser', 'quotapass', lib.get_hostname())

    def tearDown(self):
        self.quota_user.__exit__()
        self.admin.assert_icommand(['iadmin', 'rmuser', 'quotauser'])
        super(Test_Quotas, self).tearDown()


    def test_iquota__3044(self):
        filename_1 = 'test_iquota__3044_1'
        filename_2 = 'test_iquota__3044_2'
        with temporary_core_file() as core:
            core.add_rule(self.pep_to_enable_resc_quota_policy[self.plugin_name])
            IrodsController().restart(test_mode=True)
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
        try:
            with temporary_core_file() as core:
                core.add_rule(self.pep_to_enable_resc_quota_policy[self.plugin_name])
                IrodsController().restart(test_mode=True)

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
            IrodsController().restart(test_mode=True)

    # The following test covers case 40, 41, 42
    def test_physical_resource_and_total_quota_simultaneously__issue_4089(self):
        try:
            with temporary_core_file() as core:
                core.add_rule(self.pep_to_enable_resc_quota_policy[self.plugin_name])
                IrodsController().restart(test_mode=True)

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
            IrodsController().restart(test_mode=True)

    def test_single_resource_quota_violation_does_not_affect_other_resources__issue_8758(self):
        try:
            with temporary_core_file() as core:
                core.add_rule(self.pep_to_enable_resc_quota_policy[self.plugin_name])
                IrodsController().restart(test_mode=True)

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
            IrodsController().restart(test_mode=True)

    def test_quota_cares_about_acl_own_permission__issue_8750(self):
        try:
            with temporary_core_file() as core:
                core.add_rule(self.pep_to_enable_resc_quota_policy[self.plugin_name])
                IrodsController().restart(test_mode=True)

                group_name = 'test_group_acl'
                file_name = 'test_file_acl'
                resc_name = 'ufs0_quota_acl'
                user_name = 'test_user_acl'

                self.admin.assert_icommand(['iadmin', 'mkuser', user_name, 'rodsuser'])
                lib.create_ufs_resource(self.admin, resc_name)
                lib.make_file(os.path.join(self.quota_user.local_session_dir, file_name), 4096, contents='arbitrary')

                self.admin.assert_icommand(['iadmin', 'mkgroup', group_name])
                self.admin.assert_icommand(['iadmin', 'sgq', group_name, resc_name, '10000'])
                self.admin.assert_icommand(['iadmin', 'atg', group_name, self.quota_user.username])
                self.admin.assert_icommand(['iadmin', 'atg', group_name, user_name])

                self.quota_user.assert_icommand(['iput', '-R', self.testresc, os.path.join(self.quota_user.local_session_dir, file_name)])
                self.admin.assert_icommand(['iadmin', 'cu'])
                self.admin.assert_icommand(['iadmin', 'lq'], 'STDOUT', ['\nquota_over: -10000\n'])

                # Creator is given "own", so should count against quota
                self.quota_user.assert_icommand(['icp', '-R', resc_name, file_name, f'{file_name}_new'])
                self.admin.assert_icommand(['iadmin', 'cu'])
                self.admin.assert_icommand(['iadmin', 'lq'], 'STDOUT', ['\nquota_over: -5904\n'])

                # Granting another user "own" should count against quota
                self.quota_user.assert_icommand(['ichmod', 'own', user_name, f'{file_name}_new'])
                self.admin.assert_icommand(['iadmin', 'cu'])
                self.admin.assert_icommand(['iadmin', 'lq'], 'STDOUT', ['\nquota_over: -1808\n'])

                # Granting the group "own" should count against quota
                self.quota_user.assert_icommand(['ichmod', 'own', group_name, f'{file_name}_new'])
                self.admin.assert_icommand(['iadmin', 'cu'])
                self.admin.assert_icommand(['iadmin', 'lq'], 'STDOUT', ['\nquota_over: 2288\n'])
                self.quota_user.assert_icommand(['icp', '-R', resc_name, file_name, f'{file_name}_shouldntexist'], 'STDERR_SINGLELINE', 'SYS_RESC_QUOTA_EXCEEDED')

                # Removing owner's "own" permission should no longer count against quota
                self.quota_user.assert_icommand(['ichmod', 'modify_object', self.quota_user.username, f'{file_name}_new'])
                self.admin.assert_icommand(['iadmin', 'cu'])
                self.admin.assert_icommand(['iadmin', 'lq'], 'STDOUT', ['\nquota_over: -1808\n'])
                self.quota_user.assert_icommand(['icp', '-R', resc_name, file_name, f'{file_name}_shouldnowexist'])
        finally:
            self.admin.run_icommand(['ichmod', 'own', self.quota_user.username, f'{self.quota_user.session_collection}/{file_name}_new'])
            self.quota_user.run_icommand(['irm', '-f', file_name, f'{file_name}_new', f'{file_name}_shouldntexist', f'{file_name}_shouldnowexist'])
            self.admin.run_icommand(['iadmin', 'rmresc', resc_name])
            self.admin.run_icommand(['iadmin', 'rmuser', user_name])
            self.admin.run_icommand(['iadmin', 'rmgroup', group_name])
            IrodsController().restart(test_mode=True)

    def test_quota_calculations_match_genquery_calculations__issue_4102(self):
        try:
            with temporary_core_file() as core:
                core.add_rule(self.pep_to_enable_resc_quota_policy[self.plugin_name])
                IrodsController().restart(test_mode=True)

                group_name = 'test_quota_genquery'
                resc_name = 'ufs0_quota_genquery'
                file_prefix = 'quota_genquery_match'
                file_count = 10
                quota_setting = 2048*(file_count+1)+1

                # Capturing groups will capture decimal integers
                # The only valid integer with a preceding 0 is 0 itself
                # Otherwise, it must be 1-9 or 1-9 prefixing any number of 0-9
                quota_re = re.compile(r'quota_limit: (0|-?[1-9][0-9]*)|quota_over: (0|-?[1-9][0-9]*)')
                genquery_re = re.compile(r'DATA_SIZE = (0|-?[1-9][0-9]*)')

                def file_generator(n, filename_prefix):
                    seed = 1031
                    for i in range(n):
                        seed = (5*seed + 131) % 2048
                        yield (f'{filename_prefix}_{i}', seed)

                files = list(file_generator(file_count, file_prefix))
                lib.create_ufs_resource(self.admin, resc_name)
                for file in files:
                    lib.make_file(os.path.join(self.quota_user.local_session_dir, file[0]), file[1], contents='arbitrary')
                    self.quota_user.assert_icommand(['iput', '-R', resc_name, os.path.join(self.quota_user.local_session_dir, file[0])])

                self.admin.assert_icommand(['iadmin', 'mkgroup', group_name])
                self.admin.assert_icommand(['iadmin', 'sgq', group_name, resc_name, str(quota_setting)])
                self.admin.assert_icommand(['iadmin', 'atg', group_name, self.quota_user.username])

                self.admin.assert_icommand(['iadmin', 'cu'])

                total_size = sum(map(lambda file: file[1], files))

                # quota_over should be the total size of the files minus the specified quota amount
                _, quota_out, _ = self.admin.assert_icommand(['iadmin', 'lq'], 'STDOUT', [f'\nquota_over: {total_size - quota_setting}\n'])

                # Sum of all DATA_SIZE from iquest should match total
                _, genquery_out, _ = self.admin.assert_icommand(['iquest', f'select sum(DATA_SIZE) where COLL_NAME like \'{self.quota_user.session_collection}%\''], 'STDOUT', [f'DATA_SIZE = {total_size}\n'])

                # Linking the outputs of the two commands:
                # First, parse out each relevant value
                quota_match = quota_re.findall(quota_out)
                quota_limit = int(quota_match[0][0])
                quota_over = int(quota_match[1][1])
                sum_data_size = int(genquery_re.search(genquery_out).group(1))

                # Next, assert the math is correct
                self.assertEqual(quota_over, sum_data_size - quota_limit)

                lib.make_file(os.path.join(self.quota_user.local_session_dir, 'quota_genquery_bonus_file'), 2048, contents='arbitrary')
                self.quota_user.assert_icommand(['iput', '-R', resc_name, os.path.join(self.quota_user.local_session_dir, 'quota_genquery_bonus_file')])
                self.admin.assert_icommand(['iadmin', 'cu'])

                _, quota_out, _ = self.admin.assert_icommand(['iadmin', 'lq'], 'STDOUT', [f'\nquota_over: {total_size + 2048 - quota_setting}\n'])
                _, genquery_out, _ = self.admin.assert_icommand(['iquest', f'select sum(DATA_SIZE) where COLL_NAME like \'{self.quota_user.session_collection}%\''], 'STDOUT', [f'DATA_SIZE = {total_size + 2048}\n'])

                # Same as above
                quota_match = quota_re.findall(quota_out)
                quota_limit = int(quota_match[0][0])
                quota_over = int(quota_match[1][1])
                sum_data_size = int(genquery_re.search(genquery_out).group(1))

                self.assertEqual(quota_over, sum_data_size - quota_limit)
        finally:
            self.quota_user.run_icommand(['irm', '-f', 'quota_genquery_bonus_file', *map(lambda file: file[0], files)])
            self.admin.run_icommand(['iadmin', 'rmresc', resc_name])
            self.admin.run_icommand(['iadmin', 'rmgroup', group_name])
            IrodsController().restart(test_mode=True)
