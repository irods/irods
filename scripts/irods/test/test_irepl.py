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

class Test_IRepl(session.make_sessions_mixin([('otherrods', 'rods')], []), unittest.TestCase):

    def setUp(self):
        super(Test_IRepl, self).setUp()
        self.admin = self.admin_sessions[0]

    def tearDown(self):
        super(Test_IRepl, self).tearDown()

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_irepl_with_U_and_R_options__issue_3659(self):
        self.admin.assert_icommand(['irepl', 'dummyFile', '-U', '-R', 'demoResc'],
                                   'STDERR_SINGLELINE',
                                   'ERROR: irepl: -R and -U cannot be used together')

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

        # Clean up.
        self.admin.assert_icommand('irm -rf {0}'.format(filename))

        for i in range(replicas):
            self.admin.assert_icommand('iadmin rmresc resc_{0}'.format(i))

        for name in compound_resc_child_names:
            self.admin.assert_icommand('iadmin rmchildfromresc comp_resc {0}_resc'.format(name))
            self.admin.assert_icommand('iadmin rmresc {0}_resc'.format(name))

        self.admin.assert_icommand('iadmin rmresc comp_resc')

