import os
import re
import stat
import sys
import shutil
import ustrings
import commands
import tempfile

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from .resource_suite import ResourceBase
from ..configuration import IrodsConfig
from .. import lib
from .. import test
from . import session

class Test_iPut_Options(ResourceBase, unittest.TestCase):

    def setUp(self):
        self.new_paths = []
        super(Test_iPut_Options, self).setUp()

    def tearDown(self):
        for abs_path in self.new_paths:
            shutil.rmtree(abs_path, ignore_errors = True)
        super(Test_iPut_Options, self).tearDown()

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'not working - see comment below')
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

    def test_iput_recursive_bulk_upload__4657(self):
        bulk_str = 'Bulk upload'
        source_path = tempfile.mkdtemp()
        tempfile.mkstemp(dir=source_path)
        try:
            _,out,_ = self.admin.assert_icommand(
                ['iput', '-v', '-r', source_path, 'v-r'],
                'STDOUT', ustrings.recurse_ok_string())
            self.assertNotIn(bulk_str, out, 'Bulk upload performed with no bulk flag')

            _,out,_ = self.admin.assert_icommand(
                ['iput', '-v', '-r', '-f', source_path, 'v-r-f'],
                'STDOUT', ustrings.recurse_ok_string())
            self.assertNotIn(bulk_str, out, 'Bulk upload performed with no bulk flag')

            _,out,_ = self.admin.assert_icommand(
                ['iput', '-v', '-r', '-b', source_path, 'v-r-b'],
                'STDOUT', ustrings.recurse_ok_string())
            self.assertIn(bulk_str, out, 'Bulk upload not performed when requested')

            _,out,_ = self.admin.assert_icommand(
                ['iput', '-v', '-r', '-b', '-f', source_path, 'v-r-b-f'],
                'STDOUT', ustrings.recurse_ok_string())
            self.assertIn(bulk_str, out, 'Bulk upload not performed when requested')
        finally:
            shutil.rmtree(source_path, ignore_errors=True)

    def test_verify_checksum_flag_is_honored_on_bulk_upload__issue_5288(self):
        dir_name = tempfile.mkdtemp(prefix='issue_5288_')
        files = [
            {'name': os.path.join(dir_name, 'foo'), 'data': "abc"},
            {'name': os.path.join(dir_name, 'bar'), 'data': "defghi"},
            {'name': os.path.join(dir_name, 'baz'), 'data': "jklmnopqrs"}
        ]
        for info in files:
            with open(info['name'], 'w') as f:
                f.write(info['data'])

        try:
            coll_name = os.path.join(self.admin.session_collection, 'issue_5288.d')
            self.admin.assert_icommand(['iput', '-rbK', dir_name, coll_name], 'STDOUT', [' '])

            # Show that each data object in the new collection has a checksum.
            for f in files:
                filename = os.path.basename(f['name'])
                gql = "select DATA_CHECKSUM where COLL_NAME = '{0}' and DATA_NAME = '{1}'".format(coll_name, filename)
                print('Executing GenQuery: {0}'.format(gql))
                out, _, _ = self.admin.run_icommand(['iquest', gql])
                self.assertIn('sha2:', out, "'sha2:' should be in " + out)
                self.assertGreater(len(out), 32)
        finally:
            shutil.rmtree(dir_name, ignore_errors=True)

    def test_no_checksums_are_stored_when_the_verify_checksum_flag_is_not_passed_on_bulk_upload__issue_5288(self):
        dir_name = tempfile.mkdtemp(prefix='issue_5288_')
        files = [
            {'name': os.path.join(dir_name, 'foo'), 'data': "abc"},
            {'name': os.path.join(dir_name, 'bar'), 'data': "defghi"},
            {'name': os.path.join(dir_name, 'baz'), 'data': "jklmnopqrs"}
        ]
        for info in files:
            with open(info['name'], 'w') as f:
                f.write(info['data'])

        try:
            coll_name = os.path.join(self.admin.session_collection, 'issue_5288.d')
            self.admin.assert_icommand(['iput', '-rb', dir_name, coll_name], 'STDOUT', [' '])

            # Show that each data object in the new collection does not have a checksum.
            for f in files:
                filename = os.path.basename(f['name'])
                gql = "select DATA_CHECKSUM where COLL_NAME = '{0}' and DATA_NAME = '{1}'".format(coll_name, filename)
                print('Executing GenQuery: {0}'.format(gql))
                out, _, _ = self.admin.run_icommand(['iquest', gql])
                self.assertNotIn('sha2:', out, "'sha2:' should not be in " + out)
        finally:
            shutil.rmtree(dir_name, ignore_errors=True)

class Test_iPut_Options_Issue_3883(ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_iPut_Options_Issue_3883, self).setUp()

        output = commands.getstatusoutput("hostname")
        hostname = output[1]

        # create a compound resource tree
        self.admin.assert_icommand('iadmin mkresc compoundresc3883 compound', 'STDOUT_SINGLELINE', 'Creating')
        self.admin.assert_icommand('iadmin mkresc cacheresc3883 unixfilesystem ' + hostname +
                                   ':/tmp/irods/cacheresc3883', 'STDOUT_SINGLELINE', 'Creating')
        self.admin.assert_icommand('iadmin mkresc archiveresc3883 unixfilesystem ' + hostname +
                                   ':/tmp/irods/archiveresc3883', 'STDOUT_SINGLELINE', 'Creating')

        # now connect the tree
        self.admin.assert_icommand(['iadmin', 'addchildtoresc', 'compoundresc3883', 'cacheresc3883', 'cache'])
        self.admin.assert_icommand(['iadmin', 'addchildtoresc', 'compoundresc3883', 'archiveresc3883', 'archive'])

    def tearDown(self):

        super(Test_iPut_Options_Issue_3883, self).tearDown()

        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand(['iadmin', 'rmchildfromresc', 'compoundresc3883', 'cacheresc3883'])
            admin_session.assert_icommand(['iadmin', 'rmchildfromresc', 'compoundresc3883', 'archiveresc3883'])

            admin_session.assert_icommand(['iadmin', 'rmresc', 'compoundresc3883'])
            admin_session.assert_icommand(['iadmin', 'rmresc', 'cacheresc3883'])
            admin_session.assert_icommand(['iadmin', 'rmresc', 'archiveresc3883'])


    def test_iput_zero_length_file_with_purge_and_checksum_3883(self):
        filename = 'test_iput_zero_length_file_with_purge_and_checksum_3883'
        lib.touch(filename)
        logical_path = os.path.join(self.user0.session_collection, filename)

        # Replicas on archive resources do not have checksums applied as of #6089 because the
        # operation might be expensive or unfeasible depending on the underlying storage
        # technology. It is incorrect to assume that the checksum matches that of the source
        # replica, which has been done historically in the event of a DIRECT_ARCHIVE_ACCESS
        # error. So, it is enough to see that an error did not occur here and that the replica
        # on the archive resource exists.
        self.user0.assert_icommand(['iput', '-R', 'compoundresc3883', '--purgec', '-k',
                                    filename, logical_path])
        self.user0.assert_icommand_fail(['ils', '-L', logical_path],
                                        'STDOUT_SINGLELINE', 'cacheresc3883')
        self.user0.assert_icommand(['ils', '-L', logical_path],
                                   'STDOUT_SINGLELINE', 'archiveresc3883')
        os.unlink(filename)
