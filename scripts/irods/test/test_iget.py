from __future__ import print_function
import os
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from .. import lib
from . import session
from .. import test
from ..test.command import assert_command

class test_iget_general(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):
    def setUp(self):
        super(test_iget_general, self).setUp()
        self.admin = self.admin_sessions[0]
        self.user = self.user_sessions[0]

    def tearDown(self):
        super(test_iget_general, self).tearDown()

    def test_iget_R_is_a_directive_not_a_preference__issue_4475(self):
        def do_test_iget_R_is_a_directive_not_a_preference__issue_4475(self, file_size):
            filename = 'test_iget_R_is_a_directive_not_a_preference__issue_4475'
            local_file = os.path.join(self.user.local_session_dir, filename)
            local_iget_file = local_file + '_from_iget'
            logical_path = os.path.join(self.user.session_collection, filename)
            resource_1 = 'resc1'
            resource_2 = 'resc2'
            root_resource = 'pt1'

            try:
                lib.create_passthru_resource(root_resource, self.admin)
                lib.create_ufs_resource(resource_1, self.admin, test.settings.HOSTNAME_2)
                lib.create_ufs_resource(resource_2, self.admin, test.settings.HOSTNAME_3)
                lib.add_child_resource(root_resource, resource_1, self.admin)

                if not os.path.exists(local_file):
                    lib.make_file(local_file, file_size, 'arbitrary')

                if os.path.exists(local_iget_file):
                    os.unlink(local_iget_file)

                self.user.assert_icommand(['iput', '-R', root_resource, local_file, logical_path])

                # resource which does not exist - failure
                self.user.assert_icommand(['iget', '-R', 'nope', logical_path], 'STDERR', 'SYS_RESC_DOES_NOT_EXIST')

                # leaf resource in a hierarchy - failure
                self.user.assert_icommand(['iget', '-R', resource_1, logical_path], 'STDERR', 'DIRECT_CHILD_ACCESS')

                # resource which does not host any replicas - failure
                self.user.assert_icommand(['iget', '-R', resource_2, logical_path], 'STDERR', 'SYS_REPLICA_INACCESSIBLE')

                # specify the resource on which the replica actually resides - success
                self.user.assert_icommand(['iget', '-R', root_resource, logical_path, local_iget_file])
                assert_command(['diff', local_file, local_iget_file])

                # overwrite local file with an unspecified resource - success
                self.user.assert_icommand(['iget', '-f', logical_path, local_iget_file])
                assert_command(['diff', local_file, local_iget_file])

            finally:
                self.user.run_icommand(['irm', '-f', logical_path])
                lib.remove_child_resource(root_resource, resource_1, self.admin)
                lib.remove_resource(root_resource, self.admin)
                lib.remove_resource(resource_1, self.admin)
                lib.remove_resource(resource_2, self.admin)

        do_test_iget_R_is_a_directive_not_a_preference__issue_4475(self, 1024)
        do_test_iget_R_is_a_directive_not_a_preference__issue_4475(self, 40 * 1024 * 1024)
