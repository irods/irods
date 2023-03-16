from __future__ import print_function
import sys
import shutil
import os
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest
import os
import datetime
import socket

from . import resource_suite
from . import session
from . import settings
from .. import lib
from .. import test
from ..configuration import IrodsConfig

@unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Registers files on remote resources')
class Test_Ireg(resource_suite.ResourceBase, unittest.TestCase):
    def setUp(self):
        super(Test_Ireg, self).setUp()
        self.filepaths = [os.path.join(self.admin.local_session_dir, 'file' + str(i)) for i in range(0,4)]
        for p in self.filepaths:
            lib.make_file(p, 200, 'arbitrary')

        self.admin.assert_icommand('iadmin mkresc r_resc passthru', 'STDOUT_SINGLELINE', "Creating")
        self.admin.assert_icommand('iadmin mkresc m_resc passthru', 'STDOUT_SINGLELINE', "Creating")
        hostname = socket.gethostname()
        self.admin.assert_icommand('iadmin mkresc l_resc unixfilesystem ' + hostname + ':/tmp/l_resc', 'STDOUT_SINGLELINE', "Creating")
        self.admin.assert_icommand(['iadmin', 'mkresc', 'another', 'unixfilesystem', hostname + ':/tmp/anothervault'], 'STDOUT_SINGLELINE', "Creating")

        self.admin.assert_icommand("iadmin addchildtoresc r_resc m_resc")
        self.admin.assert_icommand("iadmin addchildtoresc m_resc l_resc")

        # make local test dir
        self.testing_tmp_dir = '/tmp/irods-test-ireg'
        shutil.rmtree(self.testing_tmp_dir, ignore_errors=True)
        os.mkdir(self.testing_tmp_dir)

    def tearDown(self):
        # remove local test dir
        shutil.rmtree(self.testing_tmp_dir)

        self.admin.assert_icommand("irmtrash -M")
        self.admin.assert_icommand("iadmin rmchildfromresc m_resc l_resc")
        self.admin.assert_icommand("iadmin rmchildfromresc r_resc m_resc")

        self.admin.assert_icommand("iadmin rmresc r_resc")
        self.admin.assert_icommand("iadmin rmresc m_resc")
        self.admin.assert_icommand("iadmin rmresc l_resc")
        self.admin.assert_icommand("iadmin rmresc another")

        super(Test_Ireg, self).tearDown()

    def test_ireg_files(self):
        self.admin.assert_icommand("ireg -R l_resc " + self.filepaths[0] + ' ' + self.admin.session_collection + '/file0')
        self.admin.assert_icommand('ils -l ' + self.admin.session_collection + '/file0', 'STDOUT_SINGLELINE', "r_resc;m_resc;l_resc")

        self.admin.assert_icommand("ireg -R l_resc " + self.filepaths[1] + ' ' + self.admin.session_collection + '/file1')
        self.admin.assert_icommand('ils -l ' + self.admin.session_collection + '/file1', 'STDOUT_SINGLELINE', "r_resc;m_resc;l_resc")

        self.admin.assert_icommand("ireg -R m_resc " + self.filepaths[2] +
                                   ' ' + self.admin.session_collection + '/file2', 'STDERR_SINGLELINE', 'ERROR: regUtil: reg error for')

        self.admin.assert_icommand("ireg -R demoResc " + self.filepaths[3] + ' ' + self.admin.session_collection + '/file3')

        self.admin.assert_icommand('irm ' + self.admin.session_collection + '/file0')
        self.admin.assert_icommand('irm ' + self.admin.session_collection + '/file1')
        self.admin.assert_icommand('irm ' + self.admin.session_collection + '/file3')

    def test_ireg_new_replica__2847(self):
        filename = 'regfile.txt'
        filename2 = filename+'2'
        os.system('rm -f {0} {1}'.format(filename, filename2))
        lib.make_file(filename, 234)
        os.system('cp {0} {1}'.format(filename, filename2))
        self.admin.assert_icommand('ireg -Kk -R {0} {1} {2}'.format(self.testresc, os.path.abspath(filename), self.admin.session_collection+'/'+filename))
        self.admin.assert_icommand('ils -L', 'STDOUT_SINGLELINE', [' 0 '+self.testresc, '& '+filename])
        self.admin.assert_icommand('ireg -Kk --repl -R {0} {1} {2}'.format(self.anotherresc, os.path.abspath(filename2), self.admin.session_collection+'/'+filename))
        self.admin.assert_icommand('ils -L', 'STDOUT_SINGLELINE', [' 1 '+self.anotherresc, '& '+filename])
        os.system('rm -f {0} {1}'.format(filename, filename2))

    def test_ireg_inconsistent_replica__3829(self):
        filename = 'regfile.txt'
        filename2 = filename+'2'
        try:
            lib.make_file(filename, 234)
            lib.make_file(filename2, 468)
            self.admin.assert_icommand(['ireg', '-K', '-k', '-R', self.testresc,
                os.path.abspath(filename), os.path.join(self.admin.session_collection, filename)])
            self.admin.assert_icommand(['ils', '-L'], 'STDOUT_SINGLELINE', [' 0 '+self.testresc, '& '+filename])
            self.admin.assert_icommand(['ireg', '-K', '-k', '--repl', '-R', self.anotherresc,
                os.path.abspath(filename2), os.path.join(self.admin.session_collection, filename)])
            self.admin.assert_icommand(['ils', '-L'], 'STDOUT_SINGLELINE', [' 0 '+self.testresc, '234', 'X '+filename])
            self.admin.assert_icommand(['ils', '-L'], 'STDOUT_SINGLELINE', [' 1 '+self.anotherresc, '468', '& '+filename])
        finally:
            if os.path.exists(filename):
                os.unlink(filename)
            if os.path.exists(filename2):
                os.unlink(filename2)

    def test_ireg_dir_exclude_from(self):
        # test settings
        depth = 10
        files_per_level = 10
        file_size = 100

        # make a local test dir under /tmp/
        test_dir_name = 'test_ireg_dir_exclude_from'
        local_dir = os.path.join(self.testing_tmp_dir, test_dir_name)
        local_dirs = lib.make_deep_local_tmp_dir(local_dir, depth, files_per_level, file_size)

        # arbitrary list of files to exclude
        excluded = set(['junk0001', 'junk0002', 'junk0003'])

        # make exclusion file
        exclude_file_path = os.path.join(self.testing_tmp_dir, 'exclude.txt')
        with open(exclude_file_path, 'wt') as exclude_file:
            for filename in excluded:
                print(filename, file=exclude_file)

        # register local dir
        target_collection = self.admin.session_collection + '/' + test_dir_name
        self.admin.assert_icommand("ireg --exclude-from {exclude_file_path} -r {local_dir} {target_collection}".format(**locals()), "EMPTY")

        # compare files at each level
        for dir, files in local_dirs.items():
            subcollection = dir.replace(local_dir, target_collection, 1)

            # run ils on subcollection
            self.admin.assert_icommand(['ils', subcollection], 'STDOUT_SINGLELINE')
            ils_out = self.admin.get_entries_in_collection(subcollection)

            # compare irods objects with local files, minus the excluded ones
            local_files = set(files)
            rods_files = set(lib.get_object_names_from_entries(ils_out))
            self.assertSetEqual(rods_files, local_files - excluded,
                            msg="Files missing:\n" + str(local_files - rods_files) + "\n\n" +
                            "Extra files:\n" + str(rods_files - local_files))

        # remove local test dir and exclusion file
        os.remove(exclude_file_path)
        shutil.rmtree(local_dir)

    def test_ireg_irepl_coordinating_resource__issue_3517(self):
        # this test is no longer relevant due to issue 3844 - allowing voting for registration of replicas
        pass

    def test_ireg_recursively_with_checksums__issue_3662(self):
        thedirname = 'ingestme'
        lib.create_directory_of_small_files(thedirname,3)
        # lowercase k
        self.admin.assert_icommand('ireg -k -r {0} {1}'.format(os.path.abspath(thedirname), self.admin.session_collection+'/'+thedirname))
        self.admin.assert_icommand('ils -L {0}'.format(thedirname), 'STDOUT_SINGLELINE', ['sha2:XAs0B9+Xrk+wuByjAyCOXIyS7QzhM0KpZHwIJeWVOpw=', os.path.abspath(thedirname)])
        self.admin.assert_icommand('iunreg -r {0}'.format(thedirname))
        # uppercase K
        self.admin.assert_icommand('ireg -K -r {0} {1}'.format(os.path.abspath(thedirname), self.admin.session_collection+'/'+thedirname))
        self.admin.assert_icommand('ils -L {0}'.format(thedirname), 'STDOUT_SINGLELINE', ['sha2:IMw+oWsNyQSCaoHslbpnvHCTWE1w3/1Vryz7kcStzKY=', os.path.abspath(thedirname)])
        self.admin.assert_icommand('iunreg -r {0}'.format(thedirname))
        # both
        self.admin.assert_icommand('ireg -Kk -r {0} {1}'.format(os.path.abspath(thedirname), self.admin.session_collection+'/'+thedirname))
        self.admin.assert_icommand('ils -L {0}'.format(thedirname), 'STDOUT_SINGLELINE', ['sha2:k67r3aPVgq6JNOaM8nf/zMi0lBeVjb7g7Ei7cmtM10U=', os.path.abspath(thedirname)])
        self.admin.assert_icommand('iunreg -r {0}'.format(thedirname))
        # cleanup
        os.system('rm -rf {0}'.format(thedirname))

    def test_ireg_repl_invalid_collection__issue_3828(self):
        cmd = 'ireg --repl -R demoResc /tmp/test_file /tempZone/home/invalid_collection_name'
        self.admin.assert_icommand(cmd, 'STDERR', 'status = -814000 CAT_UNKNOWN_COLLECTION')

    def test_ireg_double_slashes__issue_3658(self):
        dirname = 'dir_3658'
        lib.create_directory_of_small_files(dirname,2)
        # This introduces the trailing slash to the end of the physical directory path name
        self.admin.assert_icommand('ireg -R {0} -r {1} {2}'.format(self.testresc, os.path.abspath(dirname)+"/", self.admin.session_collection+"/"+dirname))
        # And this shows the problem (or not, if the bug is fixed)
        self.admin.assert_icommand('iscan {0}'.format(os.path.abspath(dirname)))
        shutil.rmtree(os.path.abspath(dirname), ignore_errors=True)

    def test_ireg_silent_failure_on_invalid_perms__issue_3795(self):
        # Create a directory to hold the test files.
        # The name of the directory should keep it from colliding with other
        # tests even if it is not removed.
        src_dir = os.path.join(self.admin.local_session_dir, 'issue_3795_src_dir')
        lib.make_dir_p(src_dir)

        files = [] # Holds the full physical path of the files.
        for i in range(5):
            filename = 'file_' + str(i) + '.txt'
            filename = os.path.join(src_dir, filename)
            lib.make_file(filename, ord(os.urandom(1)) + 1) # Add 1 to avoid 0 sized file.
            files.append(filename)

        # The indices of the files we will be modifying.
        error_file_idx = [2, 4]

        # This will block iRODS from registering some of the files.
        for i in error_file_idx:
            lib.execute_command('chmod 000 {0}'.format(files[i]))

        dst_dir = self.admin.home_collection + '/issue_3795_dst_dir'

        # We expect two lines of errors to be printed out.
        regex = '^.*Level [01]: dirPathReg: filePathReg failed for.*file_[{0}{1}].txt.*status = -510013$'.format(error_file_idx[0], error_file_idx[1])
        self.admin.assert_icommand('ireg -rK {0} {1}'.format(src_dir, dst_dir), 'STDOUT_MULTILINE', use_regex=True, expected_results=regex)

        # Update permissions so that files can be processed by iRODS
        # and forcefully re-register the files. We should not receive any errors
        # from these commands.
        for i in error_file_idx:
            lib.execute_command('chmod 664 {0}'.format(files[i]))
        self.admin.assert_icommand('ireg -frK {0} {1}'.format(src_dir, dst_dir))

        # Remove the files from iRODS.
        self.admin.assert_icommand('irm -rf {0}'.format(dst_dir))

    def test_ireg_file_with_unresolvable_owner__issue_4040(self):
        filename = '/tmp/irods_unresolvable_uid_testfile__issue_4040'
        fullpath = os.path.abspath(filename)
        # if the file is there (should be for a package installation)
        # attempt the ireg, should pass now that 4040 is fixed
        if os.path.isfile(fullpath):
            self.admin.assert_icommand('ireg {0} {1}'.format(fullpath, self.admin.session_collection+'/'+os.path.basename(filename)))
            self.admin.assert_icommand('iunreg {0}'.format(self.admin.session_collection+'/'+os.path.basename(filename)))
        else:
            print('ireg skipped, file not found [{0}]'.format(filename))

    def test_ireg_with_apostrophe_logical_path__issue_5759(self):
        """Test ireg with apostrophes in the logical path and physical path.

        For each ireg, the logical path will contain an apostrophe in either the collection
        name, data object name, both, or neither. Also tests for the physical path containing an
        apostrophe, both for registering the data object and for registering a replica.
        """

        # There must be unique physical paths for the replicas because registering a replica
        # with the same physical path is not what is being tested here.
        physical_paths = [os.path.join(self.admin.local_session_dir, 'test_ireg_with_apostrophe_logical_path__issue_5759'),
                          os.path.join(self.admin.local_session_dir, 'test_ireg_with_\'_logical_path__issue_5759')]
        for p in physical_paths:
            lib.make_file(p, 1024, 'arbitrary')

        physical_paths_for_repl = [os.path.join(self.admin.local_session_dir, 'test_ireg_with_apostrophe_logical_path__issue_5759_repl'),
                                   os.path.join(self.admin.local_session_dir, 'test_ireg_with_\'_logical_path__issue_5759_repl')]
        for p in physical_paths_for_repl:
            lib.make_file(p, 1024, 'arbitrary')

        data_names = ["data_object", "data'_object"]
        collection_names = ["collection", "collect'ion"]

        for coll in collection_names:
            collection_path = os.path.join(self.admin.session_collection, coll)

            self.admin.assert_icommand(['imkdir', collection_path])

            for name in data_names:
                logical_path = os.path.join(collection_path, name)

                for path in physical_paths:
                    self.admin.assert_icommand(['ireg', path, logical_path])
                    self.admin.assert_icommand(['ils', '-l', logical_path], 'STDOUT', name)

                    for repl_path in physical_paths_for_repl:
                        self.admin.assert_icommand(['ireg', '-R', 'another', '--repl', repl_path, logical_path])

                        self.admin.assert_icommand(['ils', '-l', logical_path], 'STDOUT_MULTILINE', ['demoResc', 'another'])
                        self.admin.assert_icommand(['itrim', '-N1', '-S', 'another', logical_path], 'STDOUT', 'files trimmed = 1')

                    self.admin.assert_icommand(['irm', '-f', logical_path])


class test_ireg_replica(unittest.TestCase):
    """Test the --repl option of irepl."""
    @classmethod
    def setUpClass(self):
        """Set up test class."""
        self.admin = session.mkuser_and_return_session('rodsadmin', 'otherrods', 'rods', lib.get_hostname())


    @classmethod
    def tearDownClass(self):
        """Tear down test class."""
        with session.make_session_for_existing_admin() as admin_session:
            self.admin.__exit__()
            admin_session.assert_icommand(['iadmin', 'rmuser', self.admin.username])


    def test_ireg_replica_which_does_not_exist__issue_4206(self):
        """Test registering a non-existent file as a replica for an existing data object."""
        resource = 'reghere'
        local_file = 'test_ireg_replica_which_does_not_exist__issue_4206'
        path_to_file = os.path.join(self.admin.local_session_dir, local_file)
        logical_path = os.path.join(self.admin.session_collection, local_file)

        try:
            lib.create_ufs_resource(self.admin, resource, test.settings.HOSTNAME_2)

            lib.make_file(path_to_file, 1)

            self.admin.assert_icommand(['iput', path_to_file, logical_path])

            self.admin.assert_icommand(['ireg', '--repl', '-R', resource, '/nah', logical_path],
                                       'STDERR', 'UNIX_FILE_STAT_ERR')

            self.assertEqual(str(1), lib.get_replica_status(self.admin, local_file, 0))
            self.assertIn('CAT_NO_ROWS_FOUND', lib.get_replica_status(self.admin, local_file, 1))

        finally:
            print(self.admin.run_icommand(['ils', '-L', logical_path])[0])
            self.admin.run_icommand(['irm', '-f', logical_path])
            self.admin.run_icommand(['iadmin', 'rmresc', resource])


    @unittest.skipUnless(test.settings.RUN_IN_TOPOLOGY, 'Depends on resource being somewhere else')
    def test_ireg_replica_which_does_not_exist_on_connected_host__issue_4206(self):
        """Test registering a file as a replica which does not exist on this host."""
        resource = 'reghere'

        old_path_to_file = os.path.join('/var', 'lib', 'irods', 'version.json.dist')
        new_path_to_file = os.path.join('/var', 'lib', 'irods', 'version.json.dist.new')

        logical_path = os.path.join(self.admin.session_collection,
                                    os.path.basename(old_path_to_file))

        try:
            os.rename(old_path_to_file, new_path_to_file)

            lib.create_ufs_resource(self.admin, resource, test.settings.HOSTNAME_2)

            self.admin.assert_icommand(['iput', new_path_to_file, logical_path])

            self.admin.assert_icommand(['ireg', '--repl', '-R', resource, old_path_to_file, logical_path])

            # The expected replica status is 1 because the files should be identical in a
            # properly configured testing environment.
            self.assertEqual(str(1), lib.get_replica_status(self.admin, os.path.basename(logical_path), 0))
            self.assertEqual(str(1), lib.get_replica_status(self.admin, os.path.basename(logical_path), 1))

        finally:
            print(self.admin.run_icommand(['ils', '-L', logical_path])[0])
            self.admin.run_icommand(['iunreg', '-n1', '-N1', logical_path])
            self.admin.run_icommand(['irm', '-f', logical_path])
            self.admin.run_icommand(['iadmin', 'rmresc', resource])
            os.rename(new_path_to_file, old_path_to_file)


    @unittest.skipUnless(test.settings.RUN_IN_TOPOLOGY, 'Depends on resource being somewhere else')
    def test_ireg_replica_which_does_not_exist_on_target_resource_host__issue_4206(self):
        """Test registering a file as a replica which does not exist on the resource host."""
        resource = 'reghere'
        local_file = 'test_ireg_replica_which_does_not_exist_on_target_resource_host__issue_4206'
        path_to_file = os.path.join(self.admin.local_session_dir, local_file)
        logical_path = os.path.join(self.admin.session_collection, local_file)

        try:
            lib.create_ufs_resource(self.admin, resource, test.settings.HOSTNAME_2)

            lib.make_file(path_to_file, 1)
            self.admin.assert_icommand(['iput', path_to_file, logical_path])

            self.admin.assert_icommand(['ireg', '--repl', '-R', resource, path_to_file, logical_path],
                                       'STDERR', 'UNIX_FILE_STAT_ERR')

            self.assertEqual(str(1), lib.get_replica_status(self.admin, local_file, 0))
            self.assertIn('CAT_NO_ROWS_FOUND', lib.get_replica_status(self.admin, local_file, 1))

        finally:
            print(self.admin.run_icommand(['ils', '-L', logical_path])[0])
            self.admin.run_icommand(['irm', '-f', logical_path])
            self.admin.run_icommand(['iadmin', 'rmresc', resource])


    def test_ireg_replica_cannot_put_two_replicas_on_the_same_resource__issue_5265(self):
        """Test registering a replica on a resource which already has a replica (not allowed)."""
        local_file = 'test_ireg_replica_cannot_put_two_replicas_on_the_same_resource__issue_5265'
        path_to_file = os.path.join(self.admin.local_session_dir, local_file)
        logical_path = os.path.join(self.admin.session_collection, local_file)

        try:
            lib.make_file(path_to_file, 1)
            self.admin.assert_icommand(['iput', '-K', path_to_file, logical_path])
            checksum = lib.get_replica_checksum(self.admin, local_file, 0)

            self.admin.assert_icommand(['ireg', '--repl', path_to_file, logical_path],
                                       'STDERR', 'SYS_COPY_ALREADY_IN_RESC')

            self.assertEqual(str(1), lib.get_replica_status(self.admin, local_file, 0))
            self.assertIn('CAT_NO_ROWS_FOUND', lib.get_replica_status(self.admin, local_file, 1))

            new_checksum = lib.get_replica_checksum(self.admin, local_file, 0)
            self.assertEqual(checksum, new_checksum)

        finally:
            print(self.admin.run_icommand(['ils', '-L', logical_path])[0])
            self.admin.run_icommand(['irm', '-f', logical_path])


    def test_reg_replica_size_verification__issue_5265(self):
        """Test registering a replica when the size of the original replica differs."""
        remote_resource = 'puthere'
        local_resource = 'reghere'
        local_file = 'test_reg_replica_size_verification__issue_5265'
        path_to_file = os.path.join(self.admin.local_session_dir, local_file)
        path_to_bigger_file = os.path.join(self.admin.local_session_dir, local_file + '_big')
        logical_path = os.path.join(self.admin.session_collection, local_file)

        try:
            lib.create_ufs_resource(self.admin, remote_resource, test.settings.HOSTNAME_2)
            lib.create_ufs_resource(self.admin, local_resource)

            lib.make_file(path_to_file, 1)
            self.admin.assert_icommand(['iput', '-R', remote_resource, path_to_file, logical_path])
            size = lib.get_replica_size(self.admin, local_file, 0)

            lib.make_file(path_to_bigger_file, 2)
            self.admin.assert_icommand(['ireg', '-R', local_resource, '--repl', path_to_bigger_file, logical_path])
            reg_size = lib.get_replica_checksum(self.admin, local_file, 1)
            self.assertNotEqual(size, reg_size)

            self.assertEqual(str(0), lib.get_replica_status(self.admin, local_file, 0))
            self.assertEqual(str(1), lib.get_replica_status(self.admin, local_file, 1))

            new_size = lib.get_replica_size(self.admin, local_file, 0)
            self.assertEqual(size, new_size)

        finally:
            print(self.admin.run_icommand(['ils', '-L', logical_path])[0])
            self.admin.run_icommand(['irm', '-f', logical_path])
            self.admin.run_icommand(['iadmin', 'rmresc', remote_resource])
            self.admin.run_icommand(['iadmin', 'rmresc', local_resource])


    def test_reg_replica_size_verification_that_matches__issue_5265(self):
        """Test registering a replica when the size of the original replica matches."""
        remote_resource = 'puthere'
        local_resource = 'reghere'
        local_file = 'test_reg_replica_size_verification__issue_5265'
        path_to_file = os.path.join(self.admin.local_session_dir, local_file)
        logical_path = os.path.join(self.admin.session_collection, local_file)

        try:
            lib.create_ufs_resource(self.admin, remote_resource, test.settings.HOSTNAME_2)
            lib.create_ufs_resource(self.admin, local_resource)

            lib.make_file(path_to_file, 1)
            self.admin.assert_icommand(['iput', '-R', remote_resource, path_to_file, logical_path])
            size = lib.get_replica_size(self.admin, local_file, 0)

            self.admin.assert_icommand(['ireg', '-R', local_resource, '--repl', path_to_file, logical_path])
            reg_size = lib.get_replica_size(self.admin, local_file, 1)
            self.assertEqual(size, reg_size)

            self.assertEqual(str(1), lib.get_replica_status(self.admin, local_file, 0))
            self.assertEqual(str(1), lib.get_replica_status(self.admin, local_file, 1))

            new_size = lib.get_replica_size(self.admin, local_file, 0)
            self.assertEqual(size, new_size)

        finally:
            print(self.admin.run_icommand(['ils', '-L', logical_path])[0])
            self.admin.run_icommand(['irm', '-f', logical_path])
            self.admin.run_icommand(['iadmin', 'rmresc', remote_resource])
            self.admin.run_icommand(['iadmin', 'rmresc', local_resource])


    def test_reg_replica_checksum_verification__issue_5265(self):
        """Test registering a replica when the original replica has a checksum and the new one does not match."""
        remote_resource = 'puthere'
        local_resource = 'reghere'
        local_file = 'test_reg_replica_checksum_verification__issue_5265'
        path_to_file = os.path.join(self.admin.local_session_dir, local_file)
        path_to_arbitrary_file = os.path.join(self.admin.local_session_dir, local_file + '_arb')
        logical_path = os.path.join(self.admin.session_collection, local_file)

        try:
            lib.create_ufs_resource(self.admin, remote_resource, test.settings.HOSTNAME_2)
            lib.create_ufs_resource(self.admin, local_resource)

            lib.make_file(path_to_file, 100)
            self.admin.assert_icommand(['iput', '-K', '-R', remote_resource, path_to_file, logical_path])
            checksum = lib.get_replica_checksum(self.admin, local_file, 0)

            lib.make_file(path_to_arbitrary_file, 100, 'random')
            self.admin.assert_icommand(['ireg', '-R', local_resource, '--repl', path_to_arbitrary_file, logical_path])
            reg_checksum = lib.get_replica_checksum(self.admin, local_file, 1)
            self.assertNotEqual(checksum, reg_checksum)

            self.assertEqual(str(0), lib.get_replica_status(self.admin, local_file, 0))
            self.assertEqual(str(1), lib.get_replica_status(self.admin, local_file, 1))

            new_checksum = lib.get_replica_checksum(self.admin, local_file, 0)
            self.assertEqual(checksum, new_checksum)

        finally:
            print(self.admin.run_icommand(['ils', '-L', logical_path])[0])
            self.admin.run_icommand(['irm', '-f', logical_path])
            self.admin.run_icommand(['iadmin', 'rmresc', remote_resource])
            self.admin.run_icommand(['iadmin', 'rmresc', local_resource])


    def test_reg_replica_checksum_verification_that_matches__issue_5265(self):
        """Test registering a replica when the original replica has a checksum and the new one matches."""
        remote_resource = 'puthere'
        local_resource = 'reghere'
        local_file = 'test_reg_replica_checksum_verification_that_matches__issue_5265'
        path_to_file = os.path.join(self.admin.local_session_dir, local_file)
        logical_path = os.path.join(self.admin.session_collection, local_file)

        try:
            lib.create_ufs_resource(self.admin, remote_resource, test.settings.HOSTNAME_2)
            lib.create_ufs_resource(self.admin, local_resource)

            lib.make_file(path_to_file, 100)
            self.admin.assert_icommand(['iput', '-K', '-R', remote_resource, path_to_file, logical_path])
            checksum = lib.get_replica_checksum(self.admin, local_file, 0)

            self.admin.assert_icommand(['ireg', '-R', local_resource, '--repl', path_to_file, logical_path])
            reg_checksum = lib.get_replica_checksum(self.admin, local_file, 1)
            self.assertEqual(checksum, reg_checksum)

            self.assertEqual(str(1), lib.get_replica_status(self.admin, local_file, 0))
            self.assertEqual(str(1), lib.get_replica_status(self.admin, local_file, 1))

            new_checksum = lib.get_replica_checksum(self.admin, local_file, 0)
            self.assertEqual(checksum, new_checksum)

        finally:
            print(self.admin.run_icommand(['ils', '-L', logical_path])[0])
            self.admin.run_icommand(['irm', '-f', logical_path])
            self.admin.run_icommand(['iadmin', 'rmresc', remote_resource])
            self.admin.run_icommand(['iadmin', 'rmresc', local_resource])


class test_ireg_options(session.make_sessions_mixin([('otherrods', 'rods')], []), unittest.TestCase):
    def setUp(self):
        super(test_ireg_options, self).setUp()
        self.admin = self.admin_sessions[0]


    def tearDown(self):
        super(test_ireg_options, self).tearDown()


    def test_ireg_directory_with_no_option(self):
        reg_path = os.path.join(self.admin.local_session_dir, 'regme')
        coll = os.path.join(self.admin.session_collection, 'reghere')

        self.admin.assert_icommand_fail(['ils', '-l', coll], 'STDOUT', coll)

        lib.make_deep_local_tmp_dir(reg_path)

        out,err,rc = self.admin.run_icommand(['ireg', reg_path, coll])
        self.assertNotEqual(rc, 0)
        self.assertEqual(len(out), 0)
        self.assertTrue('SYS_PATH_IS_NOT_A_FILE' in err or 'UNIX_FILE_STAT_ERR' in err)

        self.admin.assert_icommand_fail(['ils', '-l', coll], 'STDOUT', coll)


    def test_ireg_recursive_C__issue_2919(self):
        reg_path = os.path.join(self.admin.local_session_dir, 'regme')
        coll = os.path.join(self.admin.session_collection, 'reghere')
        local_resource = 'test_ireg_recursive_C__issue_2919_resc'

        try:
            lib.create_ufs_resource(self.admin, local_resource)

            self.admin.assert_icommand_fail(['ils', '-l', coll], 'STDOUT', coll)

            lib.make_deep_local_tmp_dir(reg_path)

            out,err,rc = self.admin.run_icommand(['ireg', '-R', local_resource, '-C', reg_path, coll])
            self.assertEqual(rc, 0)
            self.assertEqual(len(err), 0)
            self.assertIn('-C option is deprecated. Use -r instead.', out)

            _,out,_ = self.admin.assert_icommand(['ils', '-lr', coll], 'STDOUT', coll)

            for f in lib.files_in_dir(reg_path):
                self.assertIn(f, out)

            for d in lib.dirs_in_dir(reg_path):
                self.assertIn(d, out)

        finally:
            self.admin.run_icommand(['iunreg', '-r', coll])
            lib.remove_resource(self.admin, local_resource)


    def test_ireg_recursive_r__issue_2919(self):
        reg_path = os.path.join(self.admin.local_session_dir, 'regme')
        coll = os.path.join(self.admin.session_collection, 'reghere')
        local_resource = 'test_ireg_recursive_C__issue_2919_resc'

        try:
            lib.create_ufs_resource(self.admin, local_resource)

            self.admin.assert_icommand_fail(['ils', '-l', coll], 'STDOUT', coll)

            lib.make_deep_local_tmp_dir(reg_path)

            self.admin.assert_icommand(['ireg', '-R', local_resource, '-r', reg_path, coll])

            _,out,_ = self.admin.assert_icommand(['ils', '-lr', coll], 'STDOUT', coll)

            for f in lib.files_in_dir(reg_path):
                self.assertIn(f, out)

            for d in lib.dirs_in_dir(reg_path):
                self.assertIn(d, out)

        finally:
            self.admin.run_icommand(['iunreg', '-r', coll])
            lib.remove_resource(self.admin, local_resource)
