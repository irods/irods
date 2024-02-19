import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest
import shutil
import os

from ..configuration import IrodsConfig
from .. import test
from .. import lib
from . import resource_suite

recurse_fail_string = 'Aborting data transfer'


@unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, "Skip for topology testing from resource server")
class Test_Symlink_Operations(resource_suite.ResourceBase, unittest.TestCase):
    plugin_name = IrodsConfig().default_rule_engine_plugin
    class_name = 'Test_Symlink_Operations'

    def setUp(self):
        super(Test_Symlink_Operations, self).setUp()
        self.testing_tmp_dir = '/tmp/irods-test-icommands-recursive'
        shutil.rmtree(self.testing_tmp_dir, ignore_errors=True)
        os.mkdir(self.testing_tmp_dir)

    def tearDown(self):
        shutil.rmtree(self.testing_tmp_dir)
        super(Test_Symlink_Operations, self).tearDown()


    #################################################################
    # Success of both of these 4009_4013 tests proves the fixes
    # for both issue 4009 and 4013
    #############################
    def test_iput_r_symlink_issue_4009_4013_with_no_symlinks(self):

        # Construct the file system version of a directory with a file and symbolic links,
        # and then move them to a collection in irods - with the --link flag.

        base_name = 'test_iput_r_no_symlinks_4009_4013'
        target_collection = 'target_' + base_name
        target_collection_path = '{self.user0.session_collection}/{target_collection}'.format(**locals())

        local_dir = os.path.join(self.testing_tmp_dir, base_name)
        local_anotherdir = os.path.join(self.testing_tmp_dir, 'anotherdir_4009_4013')

        ##################################
        # Create the file system directory hierarchy
        ########
        lib.make_dir_p(local_dir)
        self.user0.run_icommand('imkdir {target_collection_path}'.format(**locals()))

        # make file
        the_file_str = 'the_file'
        file_name = os.path.join(local_dir, the_file_str)
        lib.make_file(file_name, 10)

        # make symlink with relative path
        link1_str = 'link1'
        link_path_1 = os.path.join(local_dir, link1_str)
        lib.execute_command(['ln', '-s', the_file_str, link_path_1])

        # make symlink with fully qualified path
        link2_str = 'link2'
        link_path_2 = os.path.join(local_dir, link2_str)
        lib.execute_command(['ln', '-s', file_name, link_path_2])

        # create the other dir which we will link to
        lib.make_dir_p(local_anotherdir)

        # make symlink with absolute path to anotherdir
        anotherdirsym1 = os.path.join(local_dir, 'anotherdirsym1')
        lib.execute_command(['ln', '-s', local_anotherdir, anotherdirsym1])

        self.user0.assert_icommand('iput -r --link {local_dir} {target_collection_path}'.format(**locals()))

        cmd = 'ils -lr {target_collection_path}'.format(**locals())
        stdout,_,_ = self.user0.run_icommand(cmd)

        ##################################
        # From the ils command above, we  are xpecting output that looks like this:
        #
        # /tempZone/home/alice/2018-07-10Z05:35:12--irods-testing-ZIqHQ6/target_test_iput_r_symlink_4009_4013/test_iput_r_symlink_4009_4013:
        #   alice             0 demoResc           10 2018-07-10.01:35 & the_file
        #
        # and that's it.  What we should not see, is this (because of the --link flag):
        #
        #   alice             0 demoResc           10 2018-07-10.01:35 & link1
        #   alice             0 demoResc           10 2018-07-10.01:35 & link2
        #   C- /tempZone/home/alice/2018-07-10Z05:35:12--irods-testing-ZIqHQ6/target_test_iput_r_symlink_4009_4013/test_iput_r_symlink_4009_4013/anotherdirsym1
        # /tempZone/home/alice/2018-07-10Z05:35:12--irods-testing-ZIqHQ6/target_test_iput_r_symlink_4009_4013/test_iput_r_symlink_4009_4013/anotherdirsym1:
        #
        ########

        needle = '{target_collection_path}:'.format(**locals())
        self.assertIn(needle, stdout, '{0}: Expected stdout: "...{1}...", got: "{2}"'.format(cmd, needle, stdout))

        self.assertIn('& '+the_file_str, stdout,
            '{0}: Expected stdout: "...{1}...", got: "{2}"'.format(cmd, '& '+the_file_str, stdout))

        # Now make sure the links were not copied:

        self.assertNotIn('& '+link1_str, stdout,
            '{0}: Not expected in stdout: "...{1}...", got: "{2}"'.format(cmd, '& '+link1_str, stdout))

        self.assertNotIn('& '+link2_str, stdout,
            '{0}: Not expected in stdout: "...{1}...", got: "{2}"'.format(cmd, '& '+link2_str, stdout))

        needle = 'C- {target_collection_path}/{base_name}/anotherdirsym1'.format(**locals())
        self.assertNotIn(needle, stdout, '{0}: Not expected stdout:\n"{1}"\nGot:\n"{2}"\n'.format(cmd, needle, stdout))

        needle = '{target_collection_path}/{base_name}/anotherdirsym1:'.format(**locals())
        self.assertNotIn(needle, stdout, '{0}: Not expected stdout:\n"{1}"\nGot:\n"{2}"\n'.format(cmd, needle, stdout))


    #################################################################
    # Success of both of this and the previous 4009_4013 tests proves the fixes
    # for both issue 4009 and 4013
    #############################
    def test_iput_r_symlink_issue_4009_4013_with_symlinks(self):

        # Construct the file system version of a directory with a file and symbolic links,
        # and then move them to a collection in irods - with no --link flag.

        base_name = 'test_iput_r_symlink_4009_4013'
        target_collection = 'target_' + base_name
        target_collection_path = '{self.user0.session_collection}/{target_collection}'.format(**locals())

        local_dir = os.path.join(self.testing_tmp_dir, base_name)
        local_anotherdir = os.path.join(self.testing_tmp_dir, 'anotherdir_4009_4013')

        ##################################
        # Create the file system directory hierarchy
        ########
        lib.make_dir_p(local_dir)
        self.user0.run_icommand('imkdir {target_collection_path}'.format(**locals()))

        # make file
        the_file_str = 'the_file'
        file_name = os.path.join(local_dir, the_file_str)
        lib.make_file(file_name, 10)

        # make symlink with relative path
        link1_str = 'link1'
        link_path_1 = os.path.join(local_dir, link1_str)
        lib.execute_command(['ln', '-s', the_file_str, link_path_1])

        # make symlink with fully qualified path
        link2_str = 'link2'
        link_path_2 = os.path.join(local_dir, link2_str)
        lib.execute_command(['ln', '-s', file_name, link_path_2])

        # create the other dir which we will link to
        lib.make_dir_p(local_anotherdir)

        # make symlink with absolute path to anotherdir
        anotherdirsym1 = os.path.join(local_dir, 'anotherdirsym1')
        lib.execute_command(['ln', '-s', local_anotherdir, anotherdirsym1])

        ##################################
        # --link flag OFF:  Move the data into irods, with all valid symbolic links
        ########
        self.user0.assert_icommand('iput -r {local_dir} {target_collection_path}'.format(**locals()))

        cmd = 'ils -lr {target_collection_path}'.format(**locals())
        stdout,_,_ = self.user0.run_icommand(cmd)

        ##################################
        # From the ils command above, we  are xpecting output that looks like this:
        #
        # /tempZone/home/alice/2018-07-10Z05:35:12--irods-testing-ZIqHQ6/target_test_iput_r_symlink_4009_4013/test_iput_r_symlink_4009_4013:
        #   alice             0 demoResc           10 2018-07-10.01:35 & link1
        #   alice             0 demoResc           10 2018-07-10.01:35 & link2
        #   alice             0 demoResc           10 2018-07-10.01:35 & the_file
        #   C- /tempZone/home/alice/2018-07-10Z05:35:12--irods-testing-ZIqHQ6/target_test_iput_r_symlink_4009_4013/test_iput_r_symlink_4009_4013/anotherdirsym1
        # /tempZone/home/alice/2018-07-10Z05:35:12--irods-testing-ZIqHQ6/target_test_iput_r_symlink_4009_4013/test_iput_r_symlink_4009_4013/anotherdirsym1:
        #
        ########

        needle = '{target_collection_path}:'.format(**locals())
        self.assertIn(needle, stdout, '{0}: Expected stdout: "...{1}...", got: "{2}"'.format(cmd, needle, stdout))

        self.assertIn('& '+link1_str, stdout,
            '{0}: Expected stdout: "...{1}...", got: "{2}"'.format(cmd, '& '+link1_str, stdout))

        self.assertIn('& '+link2_str, stdout,
            '{0}: Expected stdout: "...{1}...", got: "{2}"'.format(cmd, '& '+link2_str, stdout))

        self.assertIn('& '+the_file_str, stdout,
            '{0}: Expected stdout: "...{1}...", got: "{2}"'.format(cmd, '& '+the_file_str, stdout))

        needle = 'C- {target_collection_path}/{base_name}/anotherdirsym1'.format(**locals())
        self.assertIn(needle, stdout, '{0}: Expected stdout:\n"{1}"\nGot:\n"{2}"\n'.format(cmd, needle, stdout))

        needle = '{target_collection_path}/{base_name}/anotherdirsym1:'.format(**locals())
        self.assertIn(needle, stdout, '{0}: Expected stdout:\n"{1}"\nGot:\n"{2}"\n'.format(cmd, needle, stdout))


    ##################################
    # Make sure that both irsync and iput detect dangling symlinks,
    # and terminate the icommand before data transfers
    #
    # The scenarios being tested here are enumerated in the list of strings
    # "test_list" below. The non-recursive cases are dealt with in the test case
    # (test_irsync_iput_file_dangling_symlink_issues_3072_3988) below.
    ########
    def test_irsync_iput_recursive_dangling_symlink_issues_3072_3988(self):

        ##################################
        # All of these tests (should) produce the same behavior and results:
        ########
        test_list = [
                        'irsync -r {dirname1} i:{target_collection_path}',
                        'irsync --link -r {dirname1} i:{target_collection_path}',
                        'iput -r {dirname1} {target_collection_path}',
                        'iput --link -r {dirname1} {target_collection_path}',
                        'iput -r {dirname1}',
                        'iput --link -r {dirname1}'
                    ]

        base_name = 'iputrsync_recursive_dangling_symlink'
        local_dir = os.path.join(self.testing_tmp_dir, base_name)
        target_collection = 'target_' + base_name
        target_collection_path = '{self.user0.session_collection}/{target_collection}'.format(**locals())

        try:
            ##################################
            # Create the file system directory hierarchy
            ########
            lib.make_dir_p(local_dir)
            dirname1 = os.path.join(local_dir, 'dir1')

            # This creates dir1 with a single regular files 0 in it
            lib.create_directory_of_small_files(dirname1,2)

            dangling_symlink = os.path.join(dirname1, 'dangling_symlink')
            lib.execute_command(['ln', '-s', dirname1+'/0', dangling_symlink])

            # Make it dangle
            lib.execute_command(['mv', dirname1+'/0', dirname1+'/newname'])

            ##################################
            # Iterate through each of the tests:
            ########
            for teststring in test_list:
                with self.subTest(teststring):
                    # Create the test collection every time because it has to be cleaned up every time.
                    self.user0.run_icommand(['imkdir', target_collection_path])

                    ignore_symlinks = '--link' in teststring

                    cmd = teststring.format(**locals())
                    stdout,stderr,_ = self.user0.run_icommand(cmd)

                    if ignore_symlinks:
                        self.assertEqual('', stdout, '{}: Expected stdout: "", got: "{}"'.format(cmd, stdout))
                        self.assertEqual('', stderr, '{}: Expected stderr: "", got: "{}"'.format(cmd, stderr))

                    else:
                        self.assertIn(recurse_fail_string, stdout,
                                      '{0}: Expected stdout: "...{1}...", got: "{2}"'.format(cmd, recurse_fail_string, stdout))

                        self.assertIn('ERROR: USER_INPUT_PATH_ERR: No such file or directory', stderr,
                                      '{0}: Expected stderr: "...{1}...", got: "{2}"'.format(cmd,
                                            'ERROR: USER_INPUT_PATH_ERR: No such file or directory', stderr)
                        )

                        # Make sure the single valid file under dir1 was not moved into the vault
                        self.user0.assert_icommand(
                            ['ils', '-l', f'{target_collection_path}/newname'], 'STDERR',
                            f'{target_collection_path}/newname does not exist or user lacks access permission'
                        )

                    # Make sure the dangling symlink file under dir1 was not moved into the vault.
                    self.assertFalse(
                        lib.replica_exists(self.user0, os.path.join(target_collection_path, 'dangling_symlink'), 0))

                    # Clean up every time - some of these tests actually upload things and some of them do not expect
                    # there to be things inside the test collection.
                    self.user0.run_icommand('irm -r -f {target_collection_path}'.format(**locals()))

        finally:
            self.user0.assert_icommand(['ils', '-lr', self.user0.session_collection], 'STDOUT', '') # debuggin
            self.user0.run_icommand('irm -r -f {target_collection_path}'.format(**locals()))
            shutil.rmtree(os.path.abspath(dirname1), ignore_errors=True)


    ##################################
    # Make sure that both irsync and iput detect dangling symlinks,
    # and terminate the icommand before data transfers
    #
    # The scenarios being tested here are enumerated in the list of strings
    # "test_list" below. The recursive cases are dealt with in the test case
    # (test_irsync_iput_file_dangling_symlink_issues_3072_3988) above.
    ########

    def test_irsync_iput_file_dangling_symlink_issues_3072_3988(self):

        ##################################
        # All of these tests (should) produce the same behavior and results:
        ########
        test_list = [
                        'iput {dangling_symlink}',
                        'iput --link {dangling_symlink}',
                        'irsync {dangling_symlink} i:{target_collection_path}',
                        'irsync --link {dangling_symlink} i:{target_collection_path}',
                    ]

        base_name = 'iputrsync_file_dangling_symlink'
        local_dir = os.path.join(self.testing_tmp_dir, base_name)
        target_collection = 'target_' + base_name
        target_collection_path = '{self.user0.session_collection}/{target_collection}'.format(**locals())

        try:
            ##################################
            # Create the file system directory hierarchy
            ########
            lib.make_dir_p(local_dir)
            self.user0.run_icommand('imkdir {target_collection_path}'.format(**locals()))
            dirname1 = os.path.join(local_dir, 'dir1')

            # This creates dir1 with a single regular files 0 in it
            lib.create_directory_of_small_files(dirname1,2)

            dangling_symlink = os.path.join(dirname1, 'dangling_symlink')
            lib.execute_command(['ln', '-s', dirname1+'/0', dangling_symlink])

            # Make it dangle
            lib.execute_command(['mv', dirname1+'/0', dirname1+'/newname'])

            ##################################
            # Iterate through each of the tests:
            ########
            for teststring in test_list:
                with self.subTest(teststring):
                    ignore_symlinks = '--link' in teststring

                    cmd = teststring.format(**locals())
                    stdout,stderr,_ = self.user0.run_icommand(cmd)

                    ##################################
                    # Output on stderr should look like this:
                    # remote addresses: 127.0.0.1 ERROR: resolveRodsTarget: source dir1/dangling_symlink does not exist
                    # remote addresses: 127.0.0.1 ERROR: putUtil: resolveRodsTarget error, status = -317000 status = -317000 USER_INPUT_PATH_ERR
                    ########

                    if ignore_symlinks:
                        self.assertEqual('', stdout, '{}: Expected stdout: "", got: "{}"'.format(cmd, stdout))
                        self.assertEqual('', stderr, '{}: Expected stderr: "", got: "{}"'.format(cmd, stderr))
                    else:
                        self.assertIn('/dangling_symlink does not exist', stderr,
                                  '{0}: Expected stderr: "...{1}...", got: "{2}"'.format(cmd, '/dangling_symlink does not exist', stderr))

                        self.assertIn('status = -317000 USER_INPUT_PATH_ERR', stderr,
                                  '{0}: Expected stderr: "...{1}...", got: "{2}"'.format(cmd, 'status = -317000 USER_INPUT_PATH_ERR', stderr))

                    # Make sure the dangling link is nowhere in this collection tree:
                    cmd = 'ils -lr {target_collection_path}'.format(**locals())
                    stdout,_,_ = self.user0.run_icommand(cmd)

                    self.assertNotIn('& dangling_symlink', stdout,
                              '{0}: Not expected in stdout: "...{1}...", got: "{2}"'.format(cmd, '& dangling_symlink', stdout))

        finally:
            self.user0.run_icommand('irm -r -f {target_collection_path}'.format(**locals()))
            shutil.rmtree(os.path.abspath(dirname1), ignore_errors=True)


    ##################################
    # Tests the normal operation of both iput and irsync with directories
    # and files, which inlucde valid symbolic links to files and directories
    # as well.
    ########
    def test_iput_irsync_basic_sanity_with_symlinks(self):

        ##################################
        # Both of these tests (should) produce the same behavior and results:
        ########
        test_list = [
                        'iput -r {dirname1} {dirname2} {target_collection_path}',
                        'irsync -r {dirname1} {dirname2} i:{target_collection_path}'
                    ]

        # Construct the file system version of a directory with a file and symbolic links,
        # and then move them to a collection in irods - with no --link flag.

        base_name = 'iput_irsync_sanity_with_symlinks'
        local_dir = os.path.join(self.testing_tmp_dir, base_name)
        target_collection = 'target_' + base_name
        target_collection_path = '{self.user0.session_collection}/{target_collection}'.format(**locals())

        try:
            ##################################
            # Create the file system directory hierarchy
            ########
            lib.make_dir_p(local_dir)
            self.user0.run_icommand('imkdir {target_collection_path}'.format(**locals()))

            dirname1 = os.path.join(local_dir, 'dir1')
            dirname2 = os.path.join(local_dir, 'dir2')
            dirname3 = os.path.join(local_dir, 'dir3')

            # This creates dir1, 2 and 3, with regular files 0 and 1 in each
            lib.create_directory_of_small_files(dirname1,2)
            lib.create_directory_of_small_files(dirname2,2)
            lib.create_directory_of_small_files(dirname3,2)

            ##################################
            # All symlinks point to dir3 and a file under it - this
            # is not a loop unless we include dir3 itself in the icommand.
            ########

            link_dir3 = os.path.join(dirname1, 'link_dir3')
            lib.execute_command(['ln', '-s', dirname3, link_dir3])

            second_link_dir3 = os.path.join(dirname2, 'second_link_dir3')
            lib.execute_command(['ln', '-s', dirname3, second_link_dir3])

            link_file2 = os.path.join(dirname1, 'link_file2')
            lib.execute_command(['ln', '-s', dirname3+'/1', link_file2])

            ##################################
            # Iterate through each of the tests:
            ########
            for teststring in test_list:

                cmd = teststring.format(**locals())
                self.user0.assert_icommand( cmd )

                ##################################
                # Under the target collection, we expect there to be the following structure.
                # If we ran "ils -lr of {target_collection_path}", this is what we would see:
                #
                # /tempZone/home/alice/2018-07-09Z21:57:41--irods-testing-PU7TAM/target_iput_irsync_with_symlinks:
                #   C- /tempZone/home/alice/2018-07-09Z21:57:41--irods-testing-PU7TAM/target_iput_irsync_with_symlinks/dir1
                # /tempZone/home/alice/2018-07-09Z21:57:41--irods-testing-PU7TAM/target_iput_irsync_with_symlinks/dir1:
                #   alice             0 demoResc           16 2018-07-09.17:57 & 0
                #   alice             0 demoResc           17 2018-07-09.17:57 & 1
                #   alice             0 demoResc           18 2018-07-09.17:57 & link_file2
                #   C- /tempZone/home/alice/2018-07-09Z21:57:41--irods-testing-PU7TAM/target_iput_irsync_with_symlinks/dir1/link_dir3
                # /tempZone/home/alice/2018-07-09Z21:57:41--irods-testing-PU7TAM/target_iput_irsync_with_symlinks/dir1/link_dir3:
                #   alice             0 demoResc           16 2018-07-09.17:57 & 0
                #   alice             0 demoResc           17 2018-07-09.17:57 & 1
                #   C- /tempZone/home/alice/2018-07-09Z21:57:41--irods-testing-PU7TAM/target_iput_irsync_with_symlinks/dir2
                # /tempZone/home/alice/2018-07-09Z21:57:41--irods-testing-PU7TAM/target_iput_irsync_with_symlinks/dir2:
                #   alice             0 demoResc           16 2018-07-09.17:57 & 0
                #   alice             0 demoResc           17 2018-07-09.17:57 & 1
                #   C- /tempZone/home/alice/2018-07-09Z21:57:41--irods-testing-PU7TAM/target_iput_irsync_with_symlinks/dir2/second_link_dir3
                # /tempZone/home/alice/2018-07-09Z21:57:41--irods-testing-PU7TAM/target_iput_irsync_with_symlinks/dir2/second_link_dir3:
                #   alice             0 demoResc           16 2018-07-09.17:57 & 0
                #   alice             0 demoResc           17 2018-07-09.17:57 & 1
                #
                # Instead, below, we will check each collection and subcollection for existence of all entries:
                #
                ########

                ##################################
                # {target_collection_path}:
                ########
                cmd = 'ils -l {target_collection_path}'.format(**locals())
                stdout,_,_ = self.user0.run_icommand(cmd)
                #
                # Should be:
                #
                # /tempZone/home/alice/2018-07-09Z21:57:41--irods-testing-PU7TAM/target_iput_irsync_with_symlinks:
                #   C- /tempZone/home/alice/2018-07-09Z21:57:41--irods-testing-PU7TAM/target_iput_irsync_with_symlinks/dir1
                #   C- /tempZone/home/alice/2018-07-09Z21:57:41--irods-testing-PU7TAM/target_iput_irsync_with_symlinks/dir2
                #
                needle = '{target_collection_path}:'.format(**locals())
                self.assertIn(needle, stdout, '{0}: Expected stdout:\n"{1}"\nGot:\n"{2}"\n'.format(cmd, needle, stdout))

                needle = 'C- {target_collection_path}/dir1'.format(**locals())
                self.assertIn(needle, stdout, '{0}: Expected stdout:\n"{1}"\nGot:\n"{2}"\n'.format(cmd, needle, stdout))

                needle = 'C- {target_collection_path}/dir2'.format(**locals())
                self.assertIn(needle, stdout, '{0}: Expected stdout:\n"{1}"\nGot:\n"{2}"\n'.format(cmd, needle, stdout))


                ##################################
                # {target_collection_path}/dir1:
                ########
                cmd = 'ils -l {target_collection_path}/dir1'.format(**locals())
                stdout,_,_ = self.user0.run_icommand(cmd)
                #
                # Should be:
                #
                # /tempZone/home/alice/2018-07-09Z21:57:41--irods-testing-PU7TAM/target_iput_irsync_with_symlinks/dir1:
                #   alice             0 demoResc           16 2018-07-09.17:57 & 0
                #   alice             0 demoResc           17 2018-07-09.17:57 & 1
                #   alice             0 demoResc           18 2018-07-09.17:57 & link_file2
                #   C- /tempZone/home/alice/2018-07-09Z21:57:41--irods-testing-PU7TAM/target_iput_irsync_with_symlinks/dir1/link_dir3
                #
                needle = '{target_collection_path}/dir1:'.format(**locals())
                self.assertIn(needle, stdout, '{0}: Expected stdout:\n"{1}"\nGot:\n"{2}"\n'.format(cmd, needle, stdout))

                self.assertIn('& 0', stdout, '{0}: Expected stdout: "...{1}...", got: "{2}"'.format(cmd, '& 0', stdout))
                self.assertIn('& 1', stdout, '{0}: Expected stdout: "...{1}...", got: "{2}"'.format(cmd, '& 1', stdout))
                self.assertIn('& link_file2', stdout, '{0}: Expected stdout: "...{1}...", got: "{2}"'.format(cmd, '& link_file2', stdout))

                needle = 'C- {target_collection_path}/dir1/link_dir3'.format(**locals())
                self.assertIn(needle, stdout, '{0}: Expected stdout:\n"{1}"\nGot:\n"{2}"\n'.format(cmd, needle, stdout))


                ##################################
                # {target_collection_path}/dir1/link_dir3:
                ########
                cmd = 'ils -l {target_collection_path}/dir1/link_dir3'.format(**locals())
                stdout,_,_ = self.user0.run_icommand(cmd)
                #
                # Should be:
                #
                # /tempZone/home/alice/2018-07-09Z21:57:41--irods-testing-PU7TAM/target_iput_irsync_with_symlinks/dir1/link_dir3:
                #   alice             0 demoResc           16 2018-07-09.17:57 & 0
                #   alice             0 demoResc           17 2018-07-09.17:57 & 1
                #
                needle = '{target_collection_path}/dir1/link_dir3:'.format(**locals())
                self.assertIn(needle, stdout, '{0}: Expected stdout:\n"{1}"\nGot:\n"{2}"\n'.format(cmd, needle, stdout))

                self.assertIn('& 0', stdout, '{0}: Expected stdout: "...{1}...", got: "{2}"'.format(cmd, '& 0', stdout))
                self.assertIn('& 1', stdout, '{0}: Expected stdout: "...{1}...", got: "{2}"'.format(cmd, '& 1', stdout))


                ##################################
                # {target_collection_path}/dir2
                ########
                cmd = 'ils -l {target_collection_path}/dir2'.format(**locals())
                stdout,_,_ = self.user0.run_icommand(cmd)
                #
                # Should be:
                #
                # /tempZone/home/alice/2018-07-09Z21:57:41--irods-testing-PU7TAM/target_iput_irsync_with_symlinks/dir2:
                #   alice             0 demoResc           16 2018-07-09.17:57 & 0
                #   alice             0 demoResc           17 2018-07-09.17:57 & 1
                #   C- /tempZone/home/alice/2018-07-09Z21:57:41--irods-testing-PU7TAM/target_iput_irsync_with_symlinks/dir2/second_link_dir3
                #
                needle = '{target_collection_path}/dir2:'.format(**locals())
                self.assertIn(needle, stdout, '{0}: Expected stdout:\n"{1}"\nGot:\n"{2}"\n'.format(cmd, needle, stdout))

                self.assertIn('& 0', stdout, '{0}: Expected stdout: "...{1}...", got: "{2}"'.format(cmd, '& 0', stdout))
                self.assertIn('& 1', stdout, '{0}: Expected stdout: "...{1}...", got: "{2}"'.format(cmd, '& 1', stdout))

                needle = 'C- {target_collection_path}/dir2/second_link_dir3'.format(**locals())
                self.assertIn(needle, stdout, '{0}: Expected stdout:\n"{1}"\nGot:\n"{2}"\n'.format(cmd, needle, stdout))


                ##################################
                # {target_collection_path}/dir2/second_link_dir3:
                ########
                cmd = 'ils -l {target_collection_path}/dir2/second_link_dir3'.format(**locals())
                stdout,_,_ = self.user0.run_icommand(cmd)
                #
                # Should be:
                #
                # /tempZone/home/alice/2018-07-09Z21:57:41--irods-testing-PU7TAM/target_iput_irsync_with_symlinks/dir2/second_link_dir3:
                #   alice             0 demoResc           16 2018-07-09.17:57 & 0
                #   alice             0 demoResc           17 2018-07-09.17:57 & 1
                #
                needle = '{target_collection_path}/dir2/second_link_dir3:'.format(**locals())
                self.assertIn(needle, stdout, '{0}: Expected stdout:\n"{1}"\nGot:\n"{2}"\n'.format(cmd, needle, stdout))

                self.assertIn('& 0', stdout, '{0}: Expected stdout: "...{1}...", got: "{2}"'.format(cmd, '& 0', stdout))
                self.assertIn('& 1', stdout, '{0}: Expected stdout: "...{1}...", got: "{2}"'.format(cmd, '& 1', stdout))

        finally:
            self.user0.run_icommand('irm -r -f {target_collection_path}'.format(**locals()))

            shutil.rmtree(os.path.abspath(dirname1), ignore_errors=True)
            shutil.rmtree(os.path.abspath(dirname2), ignore_errors=True)
            shutil.rmtree(os.path.abspath(dirname3), ignore_errors=True)


    ##################################
    # Tests the normal operation of both iput and irsync with directories
    # and files, which inlucde valid symbolic links to files and directories
    # as well, with the --link flag specified.
    #
    # The scenarios being tested here are enumerated in the list of strings
    # "test_list" below.
    ########
    def test_iput_irsync_basic_sanity_with_no_symlinks(self):

        ##################################
        # Both of these tests (should) produce the same behavior and results:
        ########
        test_list = [
                        'iput --link -r {dirname1} {dirname2} {target_collection_path}',
                        'irsync --link -r {dirname1} {dirname2} i:{target_collection_path}'
                    ]

        ##################################
        # Construct the file system version of a directory with a file and symbolic links,
        # and then move them to a collection in irods - with no --link flag.
        ########
        base_name = 'iput_irsync_sanity_no_symlinks'
        local_dir = os.path.join(self.testing_tmp_dir, base_name)
        target_collection = 'target_' + base_name
        target_collection_path = '{self.user0.session_collection}/{target_collection}'.format(**locals())

        try:
            ##################################
            # Create the file system directory hierarchy
            ########
            lib.make_dir_p(local_dir)
            self.user0.run_icommand('imkdir {target_collection_path}'.format(**locals()))

            dirname1 = os.path.join(local_dir, 'dir1')
            dirname2 = os.path.join(local_dir, 'dir2')
            dirname3 = os.path.join(local_dir, 'dir3')

            # This creates dir1, 2 and 3, with regular files 0 and 1 in each
            lib.create_directory_of_small_files(dirname1,2)
            lib.create_directory_of_small_files(dirname2,2)
            lib.create_directory_of_small_files(dirname3,2)

            ##################################
            # All symlinks point to dir3 and a file under it - this
            # is not a loop unless we include dir3 itself in the icommand.
            # But they shouldn't be considered anyways.
            ########

            link_dir3 = os.path.join(dirname1, 'link_dir3')
            lib.execute_command(['ln', '-s', dirname3, link_dir3])

            second_link_dir3 = os.path.join(dirname2, 'second_link_dir3')
            lib.execute_command(['ln', '-s', dirname3, second_link_dir3])

            link_file2 = os.path.join(dirname1, 'link_file2')
            lib.execute_command(['ln', '-s', dirname3+'/1', link_file2])

            ##################################
            # Iterate through each of the tests:
            ########
            for teststring in test_list:

                cmd = teststring.format(**locals())
                self.user0.assert_icommand( cmd)

                ##################################
                # Under the target collection, we expect there to be the following structure:
                # If we ran "ils -lr of {target_collection_path}", this is what we would see:
                #
                # /tempZone/home/alice/2018-07-10Z12:05:45--irods-testing-FRquzH/target_iput_irsync_sanity_no_symlinks:
                #   C- /tempZone/home/alice/2018-07-10Z12:05:45--irods-testing-FRquzH/target_iput_irsync_sanity_no_symlinks/dir1
                # /tempZone/home/alice/2018-07-10Z12:05:45--irods-testing-FRquzH/target_iput_irsync_sanity_no_symlinks/dir1:
                #   alice             0 demoResc           16 2018-07-10.08:05 & 0
                #   alice             0 demoResc           17 2018-07-10.08:05 & 1
                #   C- /tempZone/home/alice/2018-07-10Z12:05:45--irods-testing-FRquzH/target_iput_irsync_sanity_no_symlinks/dir2
                # /tempZone/home/alice/2018-07-10Z12:05:45--irods-testing-FRquzH/target_iput_irsync_sanity_no_symlinks/dir2:
                #   alice             0 demoResc           16 2018-07-10.08:05 & 0
                #   alice             0 demoResc           17 2018-07-10.08:05 & 1
                #
                # What we should not see, is anything to do with the links:
                #
                #   & link_file2
                #   link_dir3
                #   second_link_dir3
                #
                ########

                ##################################
                # {target_collection_path}:
                ########
                cmd = 'ils -l {target_collection_path}'.format(**locals())
                stdout,_,_ = self.user0.run_icommand(cmd)

                needle = '{target_collection_path}:'.format(**locals())
                self.assertIn(needle, stdout, '{0}: Expected stdout:\n"{1}"\nGot:\n"{2}"\n'.format(cmd, needle, stdout))

                needle = 'C- {target_collection_path}/dir1'.format(**locals())
                self.assertIn(needle, stdout, '{0}: Expected stdout:\n"{1}"\nGot:\n"{2}"\n'.format(cmd, needle, stdout))

                needle = 'C- {target_collection_path}/dir2'.format(**locals())
                self.assertIn(needle, stdout, '{0}: Expected stdout:\n"{1}"\nGot:\n"{2}"\n'.format(cmd, needle, stdout))

                ##################################
                # {target_collection_path}/dir1:
                ########
                cmd = 'ils -l {target_collection_path}/dir1'.format(**locals())
                stdout,_,_ = self.user0.run_icommand(cmd)
                #
                # Should be:
                #
                # /tempZone/home/alice/2018-07-09Z21:57:41--irods-testing-PU7TAM/target_iput_irsync_with_symlinks/dir1:
                #   alice             0 demoResc           16 2018-07-09.17:57 & 0
                #   alice             0 demoResc           17 2018-07-09.17:57 & 1
                #
                # And none of this:
                #
                #   alice             0 demoResc           18 2018-07-09.17:57 & link_file2
                #   C- /tempZone/home/alice/2018-07-09Z21:57:41--irods-testing-PU7TAM/target_iput_irsync_with_symlinks/dir1/link_dir3
                #
                needle = '{target_collection_path}/dir1:'.format(**locals())
                self.assertIn(needle, stdout, '{0}: Expected stdout:\n"{1}"\nGot:\n"{2}"\n'.format(cmd, needle, stdout))

                self.assertIn('& 0', stdout, '{0}: Expected stdout: "...{1}...", got: "{2}"'.format(cmd, '& 0', stdout))
                self.assertIn('& 1', stdout, '{0}: Expected stdout: "...{1}...", got: "{2}"'.format(cmd, '& 1', stdout))

                ###

                self.assertNotIn('& link_file2', stdout, '{0}: Not expected stdout: "...{1}...", got: "{2}"'.format(cmd, '& link_file2', stdout))
                needle = 'C- {target_collection_path}/dir1/link_dir3'.format(**locals())
                self.assertNotIn(needle, stdout, '{0}: Not expected stdout:\n"{1}"\nGot:\n"{2}"\n'.format(cmd, needle, stdout))


                ##################################
                # {target_collection_path}/dir2
                ########
                cmd = 'ils -l {target_collection_path}/dir2'.format(**locals())
                stdout,_,_ = self.user0.run_icommand(cmd)
                #
                # Should be:
                #
                # /tempZone/home/alice/2018-07-09Z21:57:41--irods-testing-PU7TAM/target_iput_irsync_with_symlinks/dir2:
                #   alice             0 demoResc           16 2018-07-09.17:57 & 0
                #   alice             0 demoResc           17 2018-07-09.17:57 & 1
                #
                # And not this:
                #
                #   C- /tempZone/home/alice/2018-07-09Z21:57:41--irods-testing-PU7TAM/target_iput_irsync_with_symlinks/dir2/second_link_dir3
                #
                needle = '{target_collection_path}/dir2:'.format(**locals())
                self.assertIn(needle, stdout, '{0}: Expected stdout:\n"{1}"\nGot:\n"{2}"\n'.format(cmd, needle, stdout))

                self.assertIn('& 0', stdout, '{0}: Expected stdout: "...{1}...", got: "{2}"'.format(cmd, '& 0', stdout))
                self.assertIn('& 1', stdout, '{0}: Expected stdout: "...{1}...", got: "{2}"'.format(cmd, '& 1', stdout))

                ###

                needle = 'C- {target_collection_path}/dir2/second_link_dir3'.format(**locals())
                self.assertNotIn(needle, stdout, '{0}: Not expected stdout:\n"{1}"\nGot:\n"{2}"\n'.format(cmd, needle, stdout))

        finally:
            self.user0.run_icommand('irm -r -f {target_collection_path}'.format(**locals()))

            shutil.rmtree(os.path.abspath(dirname1), ignore_errors=True)
            shutil.rmtree(os.path.abspath(dirname2), ignore_errors=True)
            shutil.rmtree(os.path.abspath(dirname3), ignore_errors=True)


    ##################################
    # Tests the operation of both iput and irsync with directories where a
    # directory entry is a symbolic link loop to itself.
    ########
    def test_iput_irsync_recursive_symlink_loop_to_self(self):

        ##################################
        # All of these tests (should) produce the same behavior and results:
        ########
        test_list = [
                        'iput -r {dirname1} {target_collection_path}',
                        'iput --link -r {dirname1} {target_collection_path}',
                        'iput -r {dirname1}',
                        'iput --link -r {dirname1}',
                        'irsync -r {dirname1} i:{target_collection_path}',
                        'irsync --link -r {dirname1} i:{target_collection_path}'
                    ]

        second_test_list = [
                        'iput {dirname1}/symself {target_collection_path}',
                        'iput --link -r {dirname1}/symself {target_collection_path}',
                        'iput -r {dirname1}/symself',
                        'iput --link -r {dirname1}/symself',
                        'irsync -r {dirname1}/symself i:{target_collection_path}',
                        'irsync --link -r {dirname1}/symself i:{target_collection_path}'
                    ]

        # Construct the file system version of a directory with a file and symbolic links,
        # and then move them to a collection in irods - with no --link flag.

        base_name = 'iput_irsync_recursive_symlink_loop_to_self'
        local_dir = os.path.join(self.testing_tmp_dir, base_name)
        target_collection = 'target_' + base_name
        target_collection_path = '{self.user0.session_collection}/{target_collection}'.format(**locals())

        try:
            ##################################
            # Create the file system directory hierarchy
            ########
            dirname1 = os.path.join(local_dir, 'dir1')
            lib.make_dir_p(local_dir)
            lib.create_directory_of_small_files(dirname1,2)

            self.user0.run_icommand('imkdir -p {target_collection_path}'.format(**locals()))

            ##################################
            # Create the file system loop with a synlink in dirname1 which
            # points to itself.
            ########
            symself = os.path.join(dirname1, 'symself')
            lib.execute_command(['ln', '-s', dirname1+'/symself', symself])

            ##################################
            # Iterate through each of the tests:
            ########
            for teststring in test_list:
                with self.subTest(teststring):
                    ignore_symlinks = '--link' in teststring

                    # Run the command:
                    cmd = teststring.format(**locals())
                    stdout,stderr,_ = self.user0.run_icommand(cmd)

                    ##################################
                    # Output on stdout should look like this:
                    #
                    #   Running recursive pre-scan... pre-scan complete... errors found.
                    #   Aborting data transfer.
                    #
                    # Output on stderr should look like this:
                    #
                    #   ERROR: USER_INPUT_PATH_ERR: Too many levels of symbolic links
                    #   Path = dir2/symself
                    #
                    #   ERROR: filesystem::recursive_directory_iterator directory error: Too many levels of symbolic links
                    #
                    ########

                    # Keeping this around - sometimes when another error shows up,
                    # this will display it in the output.
                    # self.assertEqual(stderr, "", "{0}: Expected no stderr, got: \"{1}\"".format(cmd, stderr))

                    if ignore_symlinks:
                        self.assertEqual('', stdout, '{}: Expected stdout: "", got: "{}"'.format(cmd, stdout))
                        self.assertEqual('', stderr, '{}: Expected stderr: "", got: "{}"'.format(cmd, stderr))
                    else:
                        self.assertIn(recurse_fail_string, stdout,
                                         '{0}: Expected stdout: "...{1}...", got: "{2}"'.format(cmd, recurse_fail_string, stdout))

                        self.assertIn('ERROR: USER_INPUT_PATH_ERR: Too many levels of symbolic links', stderr,
                                        '{0}: Expected stderr: "...{1}...", got: "{2}"'.format(cmd,
                                        'ERROR: USER_INPUT_PATH_ERR: Too many levels of symbolic links', stderr)
                        )

                    # Make sure the loop link is nowhere in this collection tree:
                    cmd = 'ils -lr {target_collection_path}'.format(**locals())
                    self.user0.assert_icommand_fail( cmd, 'STDOUT_SINGLELINE', 'symself' );


            ##################################
            # Iterate through the second set of tests:
            ########
            for teststring in second_test_list:
                with self.subTest(teststring):
                    ignore_symlinks = '--link' in teststring

                    # Run the command:
                    cmd = teststring.format(**locals())
                    _,stderr,_ = self.user0.run_icommand(cmd)

                    if ignore_symlinks:
                        self.assertEqual('', stderr, '{}: Expected stderr: "", got: "{}"'.format(cmd, stderr))
                    else:
                        self.assertIn('Too many levels of symbolic links', stderr,
                                      '{0}: Expected stderr: "...{1}...", got: "{2}"'.format(cmd,
                                            'Too many levels of symbolic links', stderr)
                        )

                    # Make sure the loop link is nowhere in this collection tree:
                    cmd = 'ils -lr {target_collection_path}'.format(**locals())
                    self.user0.assert_icommand_fail( cmd, 'STDOUT_SINGLELINE', 'symself' );

        finally:
            self.user0.run_icommand('irm -r -f {target_collection_path}'.format(**locals()))
            shutil.rmtree(os.path.abspath(dirname1), ignore_errors=True)


    ##################################
    # Tests the operation of both iput and irsync with directories where a
    # directory entry is a symbolic link loop to itself.
    ########
    def test_iput_irsync_symlink_loop_general(self):

        ##################################
        # All of these tests (should) produce the same behavior and results:
        ########
        test_list = [
                        'iput -r {dirname1} {dirname2} {dirname3} {target_collection_path}',
                        'irsync -r {dirname1} {dirname2} {dirname3} i:{target_collection_path}'
                    ]

        ##################################
        # Construct the file system version of a directory with a file and symbolic links,
        # and then move them to a collection in irods - with no --link flag.
        ########
        base_name = 'iput_irsync_symlink_loop_general'
        local_dir = os.path.join(self.testing_tmp_dir, base_name)
        target_collection = 'target_' + base_name
        target_collection_path = '{self.user0.session_collection}/{target_collection}'.format(**locals())

        try:
            ##################################
            # Create the file system directory hierarchy
            ########
            lib.make_dir_p(local_dir)

            self.user0.run_icommand('imkdir {target_collection_path}'.format(**locals()))

            dirname1 = os.path.join(local_dir, 'dir1')
            dirname2 = os.path.join(local_dir, 'dir2')
            dirname3 = os.path.join(local_dir, 'dir3')

            ##### This creates dir1, 2 and 3, with regular files 0 and 1 in each
            lib.create_directory_of_small_files(dirname1,2)
            lib.create_directory_of_small_files(dirname2,2)
            lib.create_directory_of_small_files(dirname3,2)

            ##################################
            # Create two crisscrossing symloops between dir1 and dir2,
            # and a dir3 symloop to itself.
            ########

            # symtodir2 resides in dir1, and points to dir2
            symtodir2 = os.path.join(dirname1, 'symtodir2')
            lib.execute_command(['ln', '-s', dirname2, symtodir2])

            # symtodir1 resides in dir2, and points to dir1
            symtodir1 = os.path.join(dirname2, 'symtodir1')
            lib.execute_command(['ln', '-s', dirname1, symtodir1])

            # symtodir3 resides in dir3, and points to dir3 itself
            symtodir3 = os.path.join(dirname3, 'symtodir3')
            lib.execute_command(['ln', '-s', dirname3, symtodir3])

            ##################################
            # Iterate through each of the tests:
            ########
            for teststring in test_list:

                cmd = teststring.format(**locals())
                stdout,stderr,_ = self.user0.run_icommand(cmd)

                errstring = 'Too many levels of symbolic links'
                self.assertIn(errstring, stderr, '{0}: Expected stderr: "...{1}...", got: "{2}"'.format(cmd, errstring, stderr))

                self.assertIn(recurse_fail_string, stdout,
                              '{0}: Expected stdout: "...{1}...", got: "{2}"'.format(cmd, recurse_fail_string, stdout))

                errstring = 'symtodir1/symtodir2/symtodir1/symtodir2/symtodir1/symtodir2/symtodir1/symtodir2'
                self.assertIn(errstring, stderr, '{0}: Expected stderr: "...{1}...", got: "{2}"'.format(cmd, errstring, stderr))

                errstring = 'symtodir3/symtodir3/symtodir3/symtodir3/symtodir3/symtodir3/symtodir3/symtodir3'
                self.assertIn(errstring, stderr, '{0}: Expected stderr: "...{1}...", got: "{2}"'.format(cmd, errstring, stderr))

                # Make sure the loop links are nowhere in this collection tree:
                cmd = 'ils -lr {target_collection_path}'.format(**locals())
                self.user0.assert_icommand_fail( cmd, 'STDOUT_SINGLELINE', 'symtodir' );

        finally:
            self.user0.run_icommand('irm -r -f {target_collection_path}'.format(**locals()))
            shutil.rmtree(os.path.abspath(dirname1), ignore_errors=True)
            shutil.rmtree(os.path.abspath(dirname2), ignore_errors=True)
            shutil.rmtree(os.path.abspath(dirname3), ignore_errors=True)



    ##################################
    # Tests the operation of both iput and irsync with directories and
    # files where one of the directories and one of the files have 000 permissions.
    # One of the scenarios includes a valid symlink to a directory that has 000
    # permissions.
    ########
    def test_iput_irsync_no_permissions(self):

        ##################################
        # All of these tests (should) produce the same behavior and results: FAIL
        ########
        failed_test_list = [
                        'iput -r {dirname1}',
                        'iput -r {dirname1} {target_collection_path}',
                        'iput -r {dirname2}',
                        'iput -r {dirname2} {target_collection_path}',
                        'iput -r {dirname1} {dirname2} {target_collection_path}',
                        'irsync -r {dirname1} i:{target_collection_path}',
                        'irsync -r {dirname2} i:{target_collection_path}',
                        #####            'irsync -r {dirname1} i:{dirname2} {target_collection_path}'
                    ]

        ##################################
        # All of these tests (should) produce the same behavior and results: SUCCEED
        ########
        ok_test_list = [
                        'iput --link -r {dirname1}',
                        'iput --link -r {dirname1} {target_collection_path}',
                        'irsync --link -r {dirname1} i:{target_collection_path}',
                    ]


        # Construct the file system version of a directory with a file and symbolic links,
        # and then move them to a collection in irods - with no --link flag.
        base_name = 'test_iput_irsync_no_permissions'
        local_dir = os.path.join(self.testing_tmp_dir, base_name)
        target_collection = 'target_' + base_name
        target_collection_path = '{self.user0.session_collection}/{target_collection}'.format(**locals())

        try:
            ##################################
            # Create the file system directory hierarchy. Basically, two directories,
            # one of which has no permissions. The other directly is valid, and a symbolic
            # link to the directory that has no permissions, as well as a regular file with
            # no permissions.
            ########
            dirname1 = os.path.join(local_dir, 'dir1')
            dirname2 = os.path.join(local_dir, 'dir2')

            lib.make_dir_p(local_dir)
            lib.create_directory_of_small_files(dirname1,2)
            lib.create_directory_of_small_files(dirname2,2)

            self.user0.run_icommand('imkdir -p {target_collection_path}'.format(**locals()))

            ##################################
            # Create the symlink in dir1 which points to dir1 (which will soon
            # become unreadable/unwritable).
            ########
            validsymlinktodir2 = os.path.join(dirname1, 'validsymlinktodir2')
            lib.execute_command('ln -s {dirname2} {validsymlinktodir2}'.format(**locals()))

            ##################################
            # Make dirname2 and a file in dirname1 inaccessible
            ########
            lib.execute_command('chmod 000 {dirname2}'.format(**locals()))
            lib.execute_command('chmod 000 {dirname1}/0'.format(**locals()))


            ##################################
            # Iterate through each of the tests:
            ########
            for teststring in failed_test_list:

                # Run the command:
                cmd = teststring.format(**locals())
                stdout,stderr,_ = self.user0.run_icommand(cmd)

                # Keeping this around - sometimes when another error shows up,
                # this will display it in the output.
                # self.assertEqual(stderr, "", "{0}: Expected no stderr, got: \"{1}\"".format(cmd, stderr))

                self.assertIn(recurse_fail_string, stdout,
                                 '{0}: Expected stdout: "...{1}...", got: "{2}"'.format(cmd, recurse_fail_string, stdout))

                self.assertIn('Permission denied', stderr,
                                '{0}: Expected stderr: "...{1}...", got: "{2}"'.format(cmd,
                                'Permission denied', stderr)
                )

                # Make sure the loop link is nowhere in this collection tree:
                cmd = 'ils -lr {target_collection_path}'.format(**locals())
                self.user0.assert_icommand_fail( cmd, 'STDOUT_SINGLELINE', 'validsymlinktodir2' );


            ##################################
            # Iterate through the second set of tests.
            # These are the (mostly) successful testcases:
            ########
            for teststring in ok_test_list:

                # Run the command:
                cmd = teststring.format(**locals())
                stdout,stderr,_ = self.user0.run_icommand(cmd)

                self.assertEqual('', stdout, '{}: Expected stdout: "", got: "{}"'.format(cmd, stdout))

                # Files WILL be transferred, with the exception of these errors:
                errstr = 'dir1/0 failed. status = -510013 status = -510013 UNIX_FILE_OPEN_ERR, Permission denied'
                self.assertIn('Permission denied', stderr,
                                '{0}: Expected stderr: "...{1}...", got: "{2}"'.format(cmd, errstr, stderr))

                # Given --link, make sure the loop link is nowhere in this collection tree:
                cmd = 'ils -lr {target_collection_path}'.format(**locals())
                self.user0.assert_icommand_fail( cmd, 'STDOUT_SINGLELINE', 'validsymlinktodir2' );

        finally:
            self.user0.run_icommand('irm -r -f {target_collection_path}'.format(**locals()))
            lib.execute_command('chmod 777 {dirname2}'.format(**locals()))
            lib.execute_command('chmod 777 {dirname1}/0'.format(**locals()))
            shutil.rmtree(os.path.abspath(dirname1), ignore_errors=True)
            shutil.rmtree(os.path.abspath(dirname2), ignore_errors=True)

