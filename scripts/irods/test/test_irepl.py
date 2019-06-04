from __future__ import print_function
import os
import sys
if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from .. import test
from .. import lib
from ..configuration import IrodsConfig

class Test_IRepl(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):

    def setUp(self):
        super(Test_IRepl, self).setUp()
        self.admin = self.admin_sessions[0]

    def tearDown(self):
        super(Test_IRepl, self).tearDown()

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_irepl_with_U_and_R_options__issue_3659(self):
        filename = 'test_file_issue_3659.txt'
        lib.make_file(filename, 1024)

        hostname = IrodsConfig().client_environment['irods_host']
        replicas = 6

        # Create resources [resc_{0..5}].
        for i in range(replicas):
            vault = os.path.join(self.admin.local_session_dir, 'issue_3659_storage_vault_' + str(i))
            path = hostname + ':' + vault
            self.admin.assert_icommand('iadmin mkresc resc_{0} unixfilesystem {1}'.format(i, path), 'STDOUT', 'unixfilesystem')

        try:
            self.admin.assert_icommand('iput -R resc_0 {0}'.format(filename))

            # Replicate the file across the remaining resources.
            for i in range(1, replicas):
                self.admin.assert_icommand('irepl -R resc_{0} {1}'.format(i, filename))

            # Make all replicas stale except one.
            self.admin.assert_icommand('iput -fR resc_0 {0}'.format(filename))

            # Verify that all replicas are stale except the one under [resc_0].
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_0', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_1', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_2', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_3', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_4', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_5', '&'])

            # Main Tests.

            # Test 1: Update the replica under resc_1.
            self.admin.assert_icommand('irepl -R resc_1 {0}'.format(filename))
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_1', '&'])

            # Test 2: Update the replica under resc_2 using any replica under resc_1.
            self.admin.assert_icommand('irepl -S resc_1 -R resc_2 {0}'.format(filename))
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_2', '&'])

            # Test 3: Update the replica under resc_3 using replica 1.
            self.admin.assert_icommand('irepl -n1 -R resc_3 {0}'.format(filename))
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_3', '&'])

            # Test 4: Update the replica under resc_4 using the stale replica under resc_5.
            self.admin.assert_icommand('irepl -S resc_5 -R resc_4 {0}'.format(filename), 'STDERR', 'SYS_NO_GOOD_REPLICA')
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_4', 'X'])

            # Test 5: Update the replica under resc_4 using the stale replica under resc_5.
            self.admin.assert_icommand('irepl -n5 -R resc_4 {0}'.format(filename), 'STDERR', 'SYS_NO_GOOD_REPLICA')
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_4', 'X'])

            # Test 6: Invalid resource name.
            self.admin.assert_icommand('irepl -R invalid_resc {0}'.format(filename), 'STDERR', 'SYS_RESC_DOES_NOT_EXIST')

            # Test 7: Incompatible parameters.
            self.admin.assert_icommand('irepl -S resc_2 -n5 -R resc_5 {0}'.format(filename), 'STDERR', 'USER_INCOMPATIBLE_PARAMS')

            # Test 8: Update a replica that does not exist under the destination resource.
            self.admin.assert_icommand('irepl -R demoResc {0}'.format(filename))
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['demoResc', '&'])

        finally:
            # Clean up.
            self.admin.assert_icommand('irm -rf {0}'.format(filename))

            for i in range(replicas):
                self.admin.assert_icommand('iadmin rmresc resc_{0}'.format(i))

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_irepl_not_honoring_R_option__issue_3885(self):
        filename = 'test_file_issue_3885.txt'
        lib.make_file(filename, 1024)

        hostname = IrodsConfig().client_environment['irods_host']
        replicas = 6

        # Create resources [resc_{0..5}].
        for i in range(replicas):
            vault = os.path.join(self.admin.local_session_dir, 'issue_3885_storage_vault_' + str(i))
            path = hostname + ':' + vault
            self.admin.assert_icommand('iadmin mkresc resc_{0} unixfilesystem {1}'.format(i, path), 'STDOUT', 'unixfilesystem')

        # Create compound resource tree.
        compound_resc_child_names = ['cache', 'archive']

        self.admin.assert_icommand('iadmin mkresc comp_resc compound', 'STDOUT', 'compound')

        for name in compound_resc_child_names:
            vault = os.path.join(self.admin.local_session_dir, 'issue_3885_storage_vault_' + name)
            path = hostname + ':' + vault
            self.admin.assert_icommand('iadmin mkresc {0}_resc unixfilesystem {1}'.format(name, path), 'STDOUT', 'unixfilesystem')
            self.admin.assert_icommand('iadmin addchildtoresc comp_resc {0}_resc {0}'.format(name))

        try:
            self.admin.assert_icommand('iput -R resc_0 {0}'.format(filename))

            # Replicate the file across the remaining resources.
            for i in range(1, replicas):
                self.admin.assert_icommand('irepl -R resc_{0} {1}'.format(i, filename))

            # Replicate the file to the compound resource.
            self.admin.assert_icommand('irepl -R comp_resc {0}'.format(filename))

            # Make all replicas stale except one.
            self.admin.assert_icommand('iput -fR resc_0 {0}'.format(filename))

            # Verify that all replicas are stale except the one under [resc_0].
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_0', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_1', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_2', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_3', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_4', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_5', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['cache_resc', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['archive_resc', '&'])

            # Try to replicate to a child of the compound resource directly.
            # This should fail.
            self.admin.assert_icommand('irepl -R cache_resc {0}'.format(filename), 'STDERR', 'DIRECT_CHILD_ACCESS')

            # Replicate the file in order and verify after each command
            # if the destination resource holds a current replica.
            for i in range(1, replicas):
                self.admin.assert_icommand('irepl -R resc_{0} {1}'.format(i, filename))
                self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_' + str(i), '&'])

            # Replicate to the compound resource and verify the results.
            self.admin.assert_icommand('irepl -R comp_resc {0}'.format(filename))
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['cache_resc', '&'])
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['archive_resc', '&'])

        finally:
            # Clean up.
            self.admin.assert_icommand('irm -rf {0}'.format(filename))

            for i in range(replicas):
                self.admin.assert_icommand('iadmin rmresc resc_{0}'.format(i))

            for name in compound_resc_child_names:
                self.admin.assert_icommand('iadmin rmchildfromresc comp_resc {0}_resc'.format(name))
                self.admin.assert_icommand('iadmin rmresc {0}_resc'.format(name))

            self.admin.assert_icommand('iadmin rmresc comp_resc')

    def test_irepl_data_object_with_no_permission__4479(self):
        user0 = self.user_sessions[0]
        resource_1 = 'resource1'
        resource_1_vault = os.path.join(user0.local_session_dir, resource_1 + 'vault')
        resource_2 = 'resource2'
        resource_2_vault = os.path.join(user0.local_session_dir, resource_2 + 'vault')

        try:
            self.admin.assert_icommand(
                ['iadmin', 'mkresc', resource_1, 'unixfilesystem', ':'.join([test.settings.HOSTNAME_1, resource_1_vault])],
                'STDOUT', 'unixfilesystem')
            self.admin.assert_icommand(
                ['iadmin', 'mkresc', resource_2, 'unixfilesystem', ':'.join([test.settings.HOSTNAME_2, resource_2_vault])],
                'STDOUT', 'unixfilesystem')

            filename = 'test_irepl_data_object_with_no_permission__4479'
            physical_path = os.path.join(user0.local_session_dir, filename)
            lib.make_file(physical_path, 1024)
            logical_path = os.path.join(user0.session_collection, filename)
            #logical_path_sans_zone = os.sep.join(str(logical_path).split(os.sep)[2:])
            #resource_1_repl_path = os.path.join(resource_1_vault, logical_path_sans_zone)
            #resource_2_repl_path = os.path.join(resource_2_vault, logical_path_sans_zone)

            # Put data object to play with...
            user0.assert_icommand(
                ['iput', '-R', resource_1, physical_path, logical_path])
            self.admin.assert_icommand(
                ['iadmin', 'ls', 'logical_path', logical_path, 'resource_hierarchy', resource_1],
                'STDOUT', 'DATA_REPL_STATUS: 1')
            self.admin.assert_icommand(['iscan', '-d', logical_path])

            # Attempt to replicate data object for which admin has no permissions...
            self.admin.assert_icommand(
                ['irepl', '-R', resource_2, logical_path],
                'STDERR', 'CAT_NO_ACCESS_PERMISSION')
            self.admin.assert_icommand(
                ['iadmin', 'ls', 'logical_path', logical_path, 'resource_hierarchy', resource_2],
                'STDOUT', 'No results found.')
            # TODO: #4770 Use test tool to assert that file was not created on resource_2 (i.e. iscan on remote physical file)

            # Try again with admin flag (with success)
            self.admin.assert_icommand(['irepl', '-M', '-R', resource_2, logical_path])
            self.admin.assert_icommand(
                ['iadmin', 'ls', 'logical_path', logical_path, 'resource_hierarchy', resource_2],
                'STDOUT', 'DATA_REPL_STATUS: 1')
            self.admin.assert_icommand(['iscan', '-d', logical_path])

        finally:
            user0.run_icommand(['irm', '-f', logical_path])
            self.admin.assert_icommand(['iadmin', 'rmresc', resource_1])
            self.admin.assert_icommand(['iadmin', 'rmresc', resource_2])

