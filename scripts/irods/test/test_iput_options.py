import os
import re
import stat
import sys
import shutil
import ustrings

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from .resource_suite import ResourceBase
from ..configuration import IrodsConfig
from .. import lib

class Test_iPut_Options(ResourceBase, unittest.TestCase):

    def setUp(self):
        self.new_paths = []
        super(Test_iPut_Options, self).setUp()

    def tearDown(self):
        for abs_path in self.new_paths:
            shutil.rmtree(abs_path, ignore_errors = True)
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

    def test_iput_recursive_with_period__issue_2010(self):
        try:
            new_dir = 'dir_for_test_iput_recursive_with_period_2010'
            home_dir = IrodsConfig().irods_directory
            save_dir = os.getcwd()
            os.chdir(home_dir)
            lib.make_deep_local_tmp_dir(new_dir, depth=5, files_per_level=30, file_size=57)
            self.new_paths.append(os.path.abspath(new_dir))
            os.chdir(new_dir)
            self.user0.assert_icommand(['iput', '-r', './'], 'STDOUT_SINGLELINE', ustrings.recurse_ok_string())
            self.user0.assert_icommand_fail('ils -l', 'STDOUT_SINGLELINE', '/.')
        finally:
            os.chdir(save_dir)

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

            # Issue 3988: the iput -r will fail during prescan, which means
            # that no data transfer will occur at all. The ils command below
            # should therefore fail.
            cmd = 'iput -r ' + test_dir
            _,stderr,_ = self.admin.run_icommand( cmd )
            self.assertIn( 'directory error: Permission denied',
                           stderr,
                           "{0}: Expected stderr: \"...{1}...\", got: \"{2}\"".format(cmd, 'directory error: Permission denied', stderr))

            self.admin.assert_icommand('ils test_dir', 'STDOUT_SINGLELINE', 'test_dir')
            self.admin.assert_icommand_fail('ils test_dir/good_sub_dir', 'STDOUT_SINGLELINE', 'a.txt')

        finally:
            os.chmod(bad_sub_dir, stat.S_IRWXU)
            os.chmod(os.path.join(test_dir, "bad_file"), stat.S_IWRITE)
            os.chmod(os.path.join(bad_sub_dir, "bad_file"), stat.S_IWRITE)
            os.chmod(os.path.join(good_sub_dir, "bad_file"), stat.S_IWRITE)

    #################################################################
    # This iput involves a commandline parameter which is a symbolic
    # link to itself.
    #############################
    def test_iput_symlink_to_self_on_commandline_4016(self):

        ##################################
        # All of these tests (should) produce the same behavior and results:
        ########
        test_list = [
                        'iput -r {symtoself_path}',
                        'iput -r {symtoself_path} {target_collection_path}',
                        'iput -r {goodfile_path} {symtoself_path} {target_collection_path}',
                    ]

        goodfile = 'goodfile'
        goodfile_path = os.path.join(self.user0.local_session_dir, goodfile)
        lib.make_file(goodfile_path, 1)

        symtoself = 'symtoself';
        symtoself_path = os.path.join(self.user0.local_session_dir, symtoself)
        lib.execute_command(['ln', '-s', symtoself, symtoself_path])

        base_name = 'iput_symlink_to_self_on_commandline_4016'
        target_collection = 'target_' + base_name
        target_collection_path = '{self.user0.session_collection}/{target_collection}'.format(**locals())
        self.user0.run_icommand('imkdir {target_collection_path}'.format(**locals()))

        ##################################
        # Iterate through each of the tests:
        ########
        for teststring in test_list:

            # Run the command:
            cmd = teststring.format(**locals())
            stdout,stderr,_ = self.user0.run_icommand(cmd)

            estr = 'Too many levels of symbolic links:'
            self.assertIn(estr, stderr, '{cmd}: Expected stderr: "...{estr}...", got: "{stderr}"'.format(**locals()))

            # Make sure the usage message is not present in the output
            self.assertNotIn('Usage:', stdout, '{cmd}: Not expected in stdout: "Usage:", got: "{stdout}"'.format(**locals()))

            # Make sure neither the loop link nor regular file is anywhere in this collection tree:
            cmd = 'ils -lr {self.user0.session_collection}'.format(**locals())
            self.user0.assert_icommand_fail( cmd, 'STDOUT_SINGLELINE', symtoself );
            self.user0.assert_icommand_fail( cmd, 'STDOUT_SINGLELINE', goodfile );

