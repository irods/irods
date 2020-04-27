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

class Test_Itrim(session.make_sessions_mixin([('otherrods', 'rods')], []), unittest.TestCase):

    def setUp(self):
        super(Test_Itrim, self).setUp()
        self.admin = self.admin_sessions[0]

    def tearDown(self):
        super(Test_Itrim, self).tearDown()

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_itrim_num_copies_repl_num_conflict__issue_3346(self):
        hostname = IrodsConfig().client_environment['irods_host']
        replicas = 6

        filename = 'test_file_issue_3346.txt'
        lib.make_file(filename, 1024)

        # Create resources [resc_{0..5}].
        for i in range(replicas):
            vault = os.path.join(self.admin.local_session_dir, 'issue_3346_storage_vault_' + str(i))
            path = hostname + ':' + vault
            self.admin.assert_icommand('iadmin mkresc resc_{0} unixfilesystem {1}'.format(i, path), 'STDOUT_SINGLELINE', ['unixfilesystem'])

        self.admin.assert_icommand('iput -R resc_0 {0}'.format(filename))

        # Replicate the file across all resources.
        for i in range(1, replicas):
            self.admin.assert_icommand('irepl -S resc_0 -R resc_{0} {1}'.format(i, filename))

        # Make all replicas stale except two.
        self.admin.assert_icommand('iput -fR resc_0 {0}'.format(filename))
        self.admin.assert_icommand('irepl -R resc_5 {0}'.format(filename))

        # Here are the actual tests.

        # Error cases.
        self.admin.assert_icommand('itrim -S resc_1 -n3 {0}'.format(filename), 'STDERR', 'status = -402000 USER_INCOMPATIBLE_PARAMS')
        self.admin.assert_icommand('itrim -N2 -n0 {0}'.format(filename), 'STDERR', 'status = -402000 USER_INCOMPATIBLE_PARAMS')
        self.admin.assert_icommand('itrim -N9 -n0 {0}'.format(filename), 'STDERR', 'status = -402000 USER_INCOMPATIBLE_PARAMS')
        self.admin.assert_icommand('itrim -n0 {0}'.format(filename), 'STDERR', 'status = -402000 USER_INCOMPATIBLE_PARAMS')

        self.admin.assert_icommand('itrim -S invalid_resc {0}'.format(filename), 'STDERR', 'status = -78000 SYS_RESC_DOES_NOT_EXIST')
        self.admin.assert_icommand('itrim -n999 {0}'.format(filename), 'STDERR', 'status = -164000 SYS_REPLICA_DOES_NOT_EXIST')
        self.admin.assert_icommand('itrim -n-1 {0}'.format(filename), 'STDERR', 'status = -164000 SYS_REPLICA_DOES_NOT_EXIST')
        self.admin.assert_icommand('itrim -nX {0}'.format(filename), 'STDERR', 'status = -403000 USER_INVALID_REPLICA_INPUT')

        # No error cases.
        self.admin.assert_icommand('itrim -N2 -S resc_1 {0}'.format(filename), 'STDOUT', 'files trimmed = 1')
        self.admin.assert_icommand('itrim -S resc_2 {0}'.format(filename), 'STDOUT', 'files trimmed = 1')

        self.admin.assert_icommand('itrim -N4 {0}'.format(filename), 'STDOUT', 'files trimmed = 1')
        self.admin.assert_icommand('itrim -N2 {0}'.format(filename), 'STDOUT', 'files trimmed = 0')
        self.admin.assert_icommand('itrim -N1 {0}'.format(filename), 'STDOUT', 'files trimmed = 1')

        self.admin.assert_icommand('itrim {0}'.format(filename), 'STDOUT', 'files trimmed = 0')

        # Clean up.
        self.admin.assert_icommand('irm -f {0}'.format(filename))
        for i in range(replicas):
            self.admin.assert_icommand('iadmin rmresc resc_{0}'.format(i))

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_itrim_option_N_is_deprecated__issue_4860(self):
        data_object = 'foo'
        filename = os.path.join(self.admin.local_session_dir, data_object)
        lib.make_file(filename, 1)
        self.admin.assert_icommand(['iput', filename])
        self.admin.assert_icommand(['itrim', '-N2', data_object], 'STDOUT', 'Specifying a minimum number of replicas to keep is deprecated.')

