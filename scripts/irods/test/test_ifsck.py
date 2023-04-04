import os
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from .. import test
from .. import lib

class Test_Ifsck(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):

    def setUp(self):
        super(Test_Ifsck, self).setUp()
        self.admin = self.admin_sessions[0]
        self.user = self.user_sessions[0]

    def tearDown(self):
        super(Test_Ifsck, self).tearDown()

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_ifsck_has_integer_overflow__issue_4391(self):
        # Create a file that is exactly 2G in size.
        file_2g = '2g_issue_4391.txt'
        lib.make_file(file_2g, 2147483648, 'arbitrary')

        # Create another file that is one byte smaller than 2G in size.
        file_just_under_2g = 'just_under_2g_issue_4391.txt'
        lib.make_file(file_just_under_2g, 2147483647, 'arbitrary')

        # Create a collection and put the large files into it.
        col = 'ifsck_issue_4391'
        self.admin.assert_icommand(['imkdir', col])
        self.admin.assert_icommand(['iput', file_2g, os.path.join(col, file_2g)])
        self.admin.assert_icommand(['iput', file_just_under_2g, os.path.join(col, file_just_under_2g)])

        # Verify that the data objects exist in iRODS under the correct location.
        physical_path_to_col = os.path.join(self.admin.get_vault_session_path(), col)
        self.admin.assert_icommand(['ils', '-L', col], 'STDOUT_MULTILINE', [os.path.join(physical_path_to_col, file_2g),
                                                                            os.path.join(physical_path_to_col, file_just_under_2g)])

        # TEST: This should not produce any messages.
        self.admin.assert_icommand(['ifsck', '-r', physical_path_to_col])

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_ifsck_continues_when_user_cannot_access_subdirectory__issue_5358(self):
        try:
            #
            # Create a directory tree with inaccessible subdirectories.
            #

            # 1. Create the root directory.
            root_dir = os.path.join(self.user.local_session_dir, 'issue_5358')
            os.mkdir(root_dir)

            # 2. Create subdirectories and adjust their permissions so that an error
            #    is generated when "ifsck" attempts to iterate over their contents.
            dirs = [os.path.join(root_dir, 'dir_1'),
                    os.path.join(root_dir, 'dir_2'),
                    os.path.join(root_dir, 'dir_3')]
            for d in dirs:
                os.mkdir(d, 0o111)

            # 3. Create one more subdirectory with normal permissions. This directory
            #    will be used to verify that the user does not run into permission
            #    issues and that "ifsck" continues without terminating.
            dirs.append(os.path.join(root_dir, 'dir_4'))
            os.mkdir(dirs[3])

            #
            # TEST
            #
            ec, _, err = self.admin.assert_icommand(['ifsck', '-r', root_dir], 'STDERR', [
                'Permission denied: "' + dirs[0] + '"',
                'Permission denied: "' + dirs[1] + '"',
                'Permission denied: "' + dirs[2] + '"'
            ])
            self.assertEqual(ec, 3)
            self.assertNotIn('Permission denied: "' + dirs[3] + '"', err)
        finally:
            for d in dirs:
                os.chmod(d, 0o777)

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing, checks vault")
    def test_ifsck_returns_error_when_an_unregistered_file_is_detected__issue_6367(self):
        directory_name = 'test_ifsck_returns_error_when_an_unregistered_file_is_detected__issue_6367'
        filename1 = 'test_ifsck_returns_error_when_an_unregistered_file_is_detected__issue_6367_f1'
        filename2 = 'test_ifsck_returns_error_when_an_unregistered_file_is_detected__issue_6367_f2'
        full_directory_path = '%s/%s' % (self.admin.local_session_dir, directory_name)
        full_directory_logical_path = '%s/%s' % (self.admin.session_collection, directory_name)
        full_path_filename1 = '%s/%s' % (full_directory_path, filename1)
        full_path_filename2 = '%s/%s' % (full_directory_path, filename2)
        # create directory with file and register it
        os.mkdir(full_directory_path)
        filesize = 100
        lib.make_file(full_path_filename1, filesize)
        self.admin.assert_icommand(['ireg', '-C', full_directory_path, full_directory_logical_path])
        # verify ifsck returns no error
        self.admin.assert_icommand(['ifsck', '-r', full_directory_path])
        # create a new unregistered file
        lib.make_file(full_path_filename2, filesize)
        # verify ifsck errors
        self.admin.assert_icommand(['ifsck', '-r', full_directory_path], 'STDOUT_SINGLELINE', 'WARNING: local file [{0}] is not registered in iRODS.'.format(full_path_filename2), desired_rc=3)

