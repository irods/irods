import os
import re
import stat
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from .resource_suite import ResourceBase
from .. import lib
from .. import test

class Test_iPut_Options(ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_iPut_Options, self).setUp()

    def tearDown(self):
        super(Test_iPut_Options, self).tearDown()

    def test_iput_options(self):
        self.admin.assert_icommand('ichmod read ' + self.user0.username + ' ' + self.admin.session_collection)
        self.admin.assert_icommand('ichmod read ' + self.user1.username + ' ' + self.admin.session_collection)
        zero_filepath = os.path.join(self.admin.local_session_dir, 'zero')
        lib.touch(zero_filepath)
        self.admin.assert_icommand('iput --metadata "a;v;u;a0;v0" --acl "read ' + self.user0.username + ';'
                                   + 'write ' + self.user1.username + ';" -- ' + zero_filepath)
        self.admin.assert_icommand('imeta ls -d zero', 'STDOUT',
                                   '(attribute: *a0?\nvalue: *v0?\nunits: *u?(\n-+ *\n)?){2}', use_regex=True)
        self.admin.assert_icommand('iget -- ' + self.admin.session_collection + '/zero ' + self.admin.local_session_dir + '/newzero')
        self.user0.assert_icommand('iget -- ' + self.admin.session_collection + '/zero ' + self.user0.local_session_dir + '/newzero')
        filepath = os.path.join(self.admin.local_session_dir, 'file')
        lib.make_file(filepath, 1)
        self.admin.assert_icommand('iput --metadata "a;v;u;a2;v2" --acl "read ' + self.user0.username + ';'
                                   + 'write ' + self.user1.username + ';" -- ' + filepath)
        self.admin.assert_icommand('imeta ls -d file', 'STDOUT',
                                   '(attribute: *a2?\nvalue: *v2?\nunits: *u?(\n-+ *\n)?){2}', use_regex=True)
        self.admin.assert_icommand('ils -l', 'STDOUT')
        self.admin.assert_icommand('iget -- ' + self.admin.session_collection + '/file ' + self.admin.local_session_dir + '/newfile')
        self.user0.assert_icommand('iget -- ' + self.admin.session_collection + '/file ' + self.user0.local_session_dir + '/newfile')
        new_filepath = os.path.join(self.user1.local_session_dir, 'file')
        # skip the end until the iput -f of unowned files is resolved
        lib.make_file(new_filepath, 2)
        self.admin.assert_icommand('iput -f -- ' + filepath + ' ' + self.admin.session_collection + '/file')
        self.user1.assert_icommand('iput -f -- ' + new_filepath + ' ' + self.admin.session_collection + '/file')

    def test_iput_with_metadata_overwrite__issue_3114(self):
        filepath = os.path.join(self.admin.local_session_dir, 'file')
        lib.make_file(filepath, 1)
        self.admin.assert_icommand('iput --metadata "a;v;u" ' + filepath)
        self.admin.assert_icommand('imeta ls -d ' + self.admin.session_collection + '/file', 'STDOUT_SINGLELINE', 'value: v')
        self.admin.assert_icommand('imeta ls -d ' + self.admin.session_collection + '/file', 'STDOUT_SINGLELINE', 'units: u')
        self.admin.assert_icommand('iput -f --metadata "a;v;u" ' + filepath)
        self.admin.assert_icommand('imeta ls -d ' + self.admin.session_collection + '/file', 'STDOUT_SINGLELINE', 'value: v')
        self.admin.assert_icommand('imeta ls -d ' + self.admin.session_collection + '/file', 'STDOUT_SINGLELINE', 'units: u')
        self.admin.assert_icommand('iput -f --metadata "a;v1;u1" ' + filepath)
        self.admin.assert_icommand('imeta ls -d ' + self.admin.session_collection + '/file', 'STDOUT_SINGLELINE', 'value: v1')
        self.admin.assert_icommand('imeta ls -d ' + self.admin.session_collection + '/file', 'STDOUT_SINGLELINE', 'units: u1')

    def test_iput_checksum_zero_length_file__issue_3275(self):
        filename = 'test_iput_checksum_zero_length_file__issue_3275'
        lib.touch(filename)
        self.user0.assert_icommand(['iput', '-K', filename])
        self.user0.assert_icommand(['ils', '-L'], 'STDOUT_SINGLELINE', 'sha2:47DEQpj8HBSa+/TImW+5JCeuQeRkm5NMpJWZG3hSuFU=')
        os.unlink(filename)

    def test_iput_with_metadata_same_file_name__issue_3434(self):
        filename = 'test_iput_with_metadata_same_file_name__issue_3434'
        filepath = os.path.join(self.admin.local_session_dir, filename)
        lib.make_file(filepath, 1)
        self.admin.assert_icommand('iput --metadata "a;v;u" ' + filepath)
        self.admin.assert_icommand('imeta ls -d ' + self.admin.session_collection + '/' + filename, 'STDOUT_SINGLELINE', 'value: v')
        self.admin.assert_icommand('imeta ls -d ' + self.admin.session_collection + '/' + filename, 'STDOUT_SINGLELINE', 'units: u')
        self.admin.assert_icommand('imkdir foo')
        self.admin.assert_icommand('icd foo')
        self.admin.assert_icommand('iput --metadata "a;v;u" ' + filepath)
        self.admin.assert_icommand('imeta ls -d ' + self.admin.session_collection + '/foo/' + filename, 'STDOUT_SINGLELINE', 'value: v')
        self.admin.assert_icommand('imeta ls -d ' + self.admin.session_collection + '/foo/' + filename, 'STDOUT_SINGLELINE', 'units: u')

    def test_iput_r_bad_permissions__issue_3583(self):
        test_dir = os.path.join(self.user1.local_session_dir, "test_dir")
        good_sub_dir = os.path.join(self.user1.local_session_dir, "test_dir", "good_sub_dir")
        bad_sub_dir = os.path.join(self.user1.local_session_dir, "test_dir", "bad_sub_dir")

        os.mkdir(test_dir)
        try:
            lib.make_file(os.path.join(test_dir, "a.txt"), 5)
            lib.make_file(os.path.join(test_dir, "b.txt"), 5)
            lib.make_file(os.path.join(test_dir, "c.txt"), 5)
            lib.make_file(os.path.join(test_dir, "bad_file"), 5)
            os.chmod(os.path.join(test_dir, "bad_file"), 0)

            os.mkdir(good_sub_dir)
            lib.make_file(os.path.join(good_sub_dir, "a.txt"), 5)
            lib.make_file(os.path.join(good_sub_dir, "b.txt"), 5)
            lib.make_file(os.path.join(good_sub_dir, "c.txt"), 5)
            lib.make_file(os.path.join(good_sub_dir, "bad_file"), 5)
            os.chmod(os.path.join(good_sub_dir, "bad_file"), 0)

            os.mkdir(bad_sub_dir)
            lib.make_file(os.path.join(bad_sub_dir, "a.txt"), 5)
            lib.make_file(os.path.join(bad_sub_dir, "b.txt"), 5)
            lib.make_file(os.path.join(bad_sub_dir, "c.txt"), 5)
            lib.make_file(os.path.join(bad_sub_dir, "bad_file"), 5)
            os.chmod(os.path.join(bad_sub_dir, "bad_file"), 0)

            os.chmod(os.path.join(bad_sub_dir), 0)

            self.admin.assert_icommand('iput -r ' + test_dir, 'STDERR_SINGLELINE', 'Permission denied')
            self.admin.assert_icommand('ils test_dir', 'STDOUT_SINGLELINE', 'good_sub_dir')
            self.admin.assert_icommand('ils test_dir/good_sub_dir', 'STDOUT_SINGLELINE', 'a.txt')
        finally:
            os.chmod(bad_sub_dir, stat.S_IRWXU)
            os.chmod(os.path.join(test_dir, "bad_file"), stat.S_IWRITE)
            os.chmod(os.path.join(bad_sub_dir, "bad_file"), stat.S_IWRITE)
            os.chmod(os.path.join(good_sub_dir, "bad_file"), stat.S_IWRITE)

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_iput_ignore_symbolic_links__issue_3072(self):
        # The root directory where all files and symlinks will be stored.
        parent_dir = 'issue_3072_parent_dir'
        parent_dir_path = os.path.join(self.admin.local_session_dir, parent_dir)
        os.mkdir(parent_dir_path)

        filename = "test_file.txt"
        filename_path = os.path.join(parent_dir_path, filename)
        lib.make_file(filename_path, 1024)

        # This directory will hold the symlink.
        child_dir_path = os.path.join(parent_dir_path, 'child_dir')
        os.mkdir(child_dir_path)

        # Create a symlink.
        dangling_symlink = 'dangling.test_file.txt'
        dangling_symlink_path = os.path.join(child_dir_path, dangling_symlink)
        os.symlink(filename_path, dangling_symlink_path)

        # Break the symlink by renaming the target file.
        os.rename(filename_path, os.path.join(parent_dir_path, 'renamed.test_file.txt'))

        # Recursively put all directories and files under the
        # parent directory. Symlinks should be completely ignored.
        self.admin.assert_icommand('iput -r --link {0}'.format(parent_dir_path))

        # Fail if the dangling symlink was stored in iRODS.
        self.admin.assert_icommand_fail('ils -lr {0}'.format(parent_dir), 'STDOUT', dangling_symlink)

