import os
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from .. import lib

class Test_ichmod(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):
    def setUp(self):
        super(Test_ichmod, self).setUp()

        self.admin = self.admin_sessions[0]
        self.user = self.user_sessions[0]

    def tearDown(self):
        super(Test_ichmod, self).tearDown()

    def test_ichmod_r_with_garbage_path(self):
        self.admin.assert_icommand('imkdir -p sub_dir1\\\\%/subdir2/')
        self.admin.assert_icommand('ichmod read ' + self.user.username + ' -r sub_dir1\\\\%/')
        self.admin.assert_icommand('ichmod inherit -r sub_dir1\\\\%/')
        filepath = os.path.join(self.admin.local_session_dir, 'file')
        lib.make_file(filepath, 1)
        self.admin.assert_icommand('iput ' + filepath + ' sub_dir1\\\\%/subdir2/')
        self.user.assert_icommand('iget ' + self.admin.session_collection + '/sub_dir1\\\\%/subdir2/file ' + os.path.join(self.user.local_session_dir, ''))


class test_collection_acl_inheritance(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):
    def setUp(self):
        super(test_collection_acl_inheritance, self).setUp()

        self.admin = self.admin_sessions[0]
        self.user = self.user_sessions[0]

        self.collection = os.path.join(self.admin.session_collection, 'sub_dir1', 'subdir2')
        self.admin.assert_icommand(['imkdir', '-p', self.collection])
        self.admin.assert_icommand(['ichmod', 'read', self.user.username, '-r', os.path.dirname(self.collection)])
        self.admin.assert_icommand(['ichmod', 'inherit', '-r', os.path.dirname(self.collection)])

    def tearDown(self):
        super(test_collection_acl_inheritance, self).tearDown()

    def test_collection_acl_inheritance_for_iput__issue_3032(self):
        filepath = os.path.join(self.admin.local_session_dir, 'file')
        lib.make_file(filepath, 1)

        logical_path = os.path.join(self.collection, 'file')

        # Put the data object into the inheritance structure and ensure that the user given
        # permission to read is able to get the data object.
        self.admin.assert_icommand(['iput', filepath, logical_path])
        self.admin.assert_icommand(['ils', '-LA', self.collection], 'STDOUT', 'file')
        self.user.assert_icommand(['iget', logical_path, os.path.join(self.user.local_session_dir, '')])

    def test_collection_acl_inheritance_for_icp__issue_3032(self):
        source_logical_path = os.path.join(self.admin.session_collection, 'file')
        destination_logical_path = os.path.join(self.collection, 'icp_file')
        filepath = os.path.join(self.admin.local_session_dir, 'file')
        lib.make_file(filepath, 1)

        # Copy from a data object outside of the inheritance structure to ensure that the ACL
        # inheritance is not applied to the original object.
        self.admin.assert_icommand(['iput', filepath, source_logical_path])
        self.admin.assert_icommand(['ils', '-LA', source_logical_path], 'STDOUT', 'file')

        # Copy the data object into the inheritance structure and ensure that the user given
        # permission to read is able to get the data object.
        self.admin.assert_icommand(['icp', source_logical_path, destination_logical_path])
        self.admin.assert_icommand(['ils', '-LA', self.collection], 'STDOUT', 'icp_file')
        self.user.assert_icommand(['iget', destination_logical_path, os.path.join(self.user.local_session_dir, '')])
