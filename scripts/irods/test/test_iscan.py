import os
import re
import sys
import shutil

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from .. import test
from .resource_suite import ResourceBase
from .. import lib


class Test_iScan(ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_iScan, self).setUp()
        self.dirname1 = 'dir_3681-1'
        self.dirname2 = 'dir_3681-2'
        self.dirname3 = 'iscan_4029'
        lib.create_directory_of_small_files(self.dirname1,2)
        lib.create_directory_of_small_files(self.dirname2,2)
        self.admin.assert_icommand(['iadmin', 'mkresc', 'pt', 'passthru'], 'STDOUT_SINGLELINE', 'passthru')
        self.admin.assert_icommand(['iadmin', 'addchildtoresc', 'pt', self.testresc])

    def tearDown(self):
        shutil.rmtree(os.path.abspath(self.dirname1), ignore_errors=True)
        shutil.rmtree(os.path.abspath(self.dirname2), ignore_errors=True)
        shutil.rmtree(os.path.abspath(self.dirname3), ignore_errors=True)
        self.admin.assert_icommand(['iadmin', 'rmchildfromresc', 'pt', self.testresc])
        self.admin.assert_icommand(['iadmin', 'rmresc', 'pt'])
        super(Test_iScan, self).tearDown()

    def test_iscan_filename_longer_than_256__5022(self):
        scandir   = 'please_scan_me'
        subdir    = '1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890'
        subsubdir = '1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890'
        okay_filename = 'short.txt'
        long_filename = 'long_enough_to_trigger_256_bug__issue_5022.txt'
        okayfile = scandir+'/'+subdir+'/'+subsubdir+'/'+okay_filename
        longfile = scandir+'/'+subdir+'/'+subsubdir+'/'+long_filename
        try:
            os.makedirs(os.path.dirname(okayfile))
            lib.touch(okayfile)
            lib.touch(longfile)
            self.admin.assert_icommand('iscan {0}'.format(okayfile),'STDOUT_SINGLELINE','{0} is not registered in iRODS'.format(okay_filename))
            self.admin.assert_icommand('iscan {0}'.format(longfile),'STDOUT_SINGLELINE','{0} is not registered in iRODS'.format(long_filename))
        finally:
            shutil.rmtree(scandir)

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: Registers a collection')
    def test_iscan_4029(self):
        missing_file_regex = re.compile(r'Physical\s+file\b.*\bmissing\b.*corresponding.*\bobject\b',re.I)
        FILES_IN_DIR = 800
        DELETE_AT_ONCE = 20
        max_iter = FILES_IN_DIR // DELETE_AT_ONCE + 1
        test_dir_path = os.path.abspath(self.dirname3)
        test_coll_path = "/" + self.admin.zone_name + "/home/" + self.admin.username + "/" + os.path.split(test_dir_path)[-1]
        if not os.path.isdir(test_dir_path):
          lib.create_directory_of_small_files(test_dir_path,FILES_IN_DIR)
        try:
          self.admin.assert_icommand( ['ireg', '-C', test_dir_path, test_coll_path])
          sorted_files = sorted(os.listdir(test_dir_path),key=int) # sort numerically
          minfile,maxfile = map(int,(sorted_files[0], sorted_files[-1]))
          files_deleted = 0
          delete_N_files_at_end = lambda n,minf,maxf : [os.unlink(os.path.join(test_dir_path,str(x))) for x in
            range(max(minf,maxf-n+1),maxf+1)]
          while maxfile > minfile or max_iter > 0:
            max_iter -= 1
            delete_count = len(delete_N_files_at_end( DELETE_AT_ONCE, minfile, maxfile ))
            maxfile -= delete_count
            files_deleted += delete_count
            out, _, _ = self.admin.run_icommand(["iscan","-rd",test_coll_path])
            printed_lines = out.split('\n')
            number_of_matching_messages = len(
                [m for m in map(lambda line : missing_file_regex.match(line), printed_lines) if m])
            self.assertEqual(number_of_matching_messages, files_deleted)
        finally:
          if os.path.isdir(test_dir_path):
            shutil.rmtree(test_dir_path,  ignore_errors=True)
            self.admin.assert_icommand("irm -fr " + test_coll_path)

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: Reads Vault')
    def test_iscan_local_file(self):
        self.user0.assert_icommand('iscan non_existent_file', 'STDERR_SINGLELINE', 'ERROR: scanObj: non_existent_file does not exist')
        existent_file = os.path.join(self.user0.local_session_dir, 'existent_file')
        lib.touch(existent_file)
        self.user0.assert_icommand('iscan ' + existent_file, 'STDOUT_SINGLELINE', existent_file + ' is not registered in iRODS')
        self.user0.assert_icommand('iput ' + existent_file)
        out, _, _ = self.user0.run_icommand('''iquest "SELECT DATA_PATH WHERE DATA_NAME = 'existent_file'"''' )
        data_path = out.strip().strip('-').strip()[12:]
        self.user0.assert_icommand('iscan ' + data_path)
        self.user0.assert_icommand('irm -f existent_file')

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: Reads Vault')
    def test_iscan_data_object(self):
        # test that rodsusers can't use iscan -d
        self.user0.assert_icommand('iscan -d non_existent_file', 'STDOUT_SINGLELINE', 'Could not find the requested data object or collection in iRODS.')
        existent_file = os.path.join(self.user0.local_session_dir, 'existent_file')
        lib.make_file(existent_file, 1)
        self.user0.assert_icommand('iput ' + existent_file)
        out, _, _ = self.admin.run_icommand('iquest "SELECT DATA_PATH WHERE DATA_NAME = \'existent_file\'"')
        data_path = out.strip().strip('-').strip()[12:]
        self.user0.assert_icommand('iscan -d existent_file', 'STDOUT_SINGLELINE', 'User must be a rodsadmin to scan iRODS data objects.')
        os.remove(data_path)
        self.user0.assert_icommand('iscan -d existent_file', 'STDOUT_SINGLELINE', 'User must be a rodsadmin to scan iRODS data objects.')
        lib.make_file(data_path, 1)
        self.user0.assert_icommand('irm -f existent_file')
        zero_file = os.path.join(self.user0.local_session_dir, 'zero_file')
        lib.touch(zero_file)
        self.user0.assert_icommand('iput ' + zero_file)
        self.user0.assert_icommand('iscan -d zero_file', 'STDOUT_SINGLELINE', 'User must be a rodsadmin to scan iRODS data objects.')
        self.user0.assert_icommand('irm -f zero_file')

        # test that rodsadmins can use iscan -d
        self.admin.assert_icommand('iscan -d non_existent_file', 'STDOUT_SINGLELINE', 'Could not find the requested data object or collection in iRODS.')
        existent_file = os.path.join(self.admin.local_session_dir, 'existent_file')
        lib.make_file(existent_file, 1)
        self.admin.assert_icommand('iput ' + existent_file)
        out, _, _ = self.admin.run_icommand('''iquest "SELECT DATA_PATH WHERE DATA_NAME = 'existent_file'"''' )
        data_path = out.strip().strip('-').strip()[12:]
        self.admin.assert_icommand('iscan -d existent_file')
        os.remove(data_path)
        self.admin.assert_icommand('iscan -d existent_file', 'STDOUT_SINGLELINE', 'is missing, corresponding to iRODS object')
        lib.make_file(data_path, 1)
        self.admin.assert_icommand('irm -f existent_file')
        zero_file = os.path.join(self.admin.local_session_dir, 'zero_file')
        lib.touch(zero_file)
        self.admin.assert_icommand('iput ' + zero_file)
        self.admin.assert_icommand('iscan -d zero_file')
        self.admin.assert_icommand('irm -f zero_file')

    # This is here because assert_icommand (and _fail) do not check
    # the return code (rc below).
    def _util_simple_icmd_assert(self, cmd):
        stdout, stderr, rc = self.admin.run_icommand(cmd)
        self.assertEqual(rc, 0, "{0}: failed rc = {1} (expected 0)".format(cmd, rc))
        self.assertEqual(stdout, "", "{0}: Expected no stdout, got: \"{1}\"".format(cmd, stdout))
        self.assertEqual(stderr, "", "{0}: Expected no stderr, got: \"{1}\"".format(cmd, stderr))

    # Ditto the above. Assumes error string is in stdout (not stderr).
    def _util_simple_icmd_fail_stdout_assert(self, cmd, error_str_in_stdout):
        stdout, stderr, rc = self.admin.run_icommand(cmd)
        self.assertNotEqual(rc, 0, "{0}: failed rc = {1} (expected non-0)".format(cmd, rc))
        self.assertIn(error_str_in_stdout, stdout, "{0}: Expected stdout: \"...{1}...\", got: \"{2}\"".format(cmd, error_str_in_stdout, stdout))
        self.assertEqual(stderr, "", "{0}: Expected no stderr, got: \"{1}\"".format(cmd, stderr))

    # Ditto the above. Assumes error string is in stderr (not stdout).
    def _util_simple_icmd_fail_stderr_assert(self, cmd, error_str_in_stderr):
        stdout, stderr, rc = self.admin.run_icommand(cmd)
        self.assertNotEqual(rc, 0, "{0}: failed rc = {1} (expected non-0)".format(cmd, rc))
        self.assertIn(error_str_in_stderr, stderr, "{0}: Expected stderr: \"...{1}...\", got: \"{2}\"".format(cmd, error_str_in_stderr, stderr))
        self.assertEqual(stdout, "", "{0}: Expected no stdout, got: \"{1}\"".format(cmd, stdout))

    def test_iscan_retcode_and_errmsg__issue_3681(self):
        # Checks that return codes and error messages are correct for
        # missing files and data objects.

        # This is one of the two files created in each directory in the test setup above: make it 0 length.
        lib.execute_command(['truncate', '-s', '0', os.path.abspath(self.dirname1)+"/0"])
        lib.execute_command(['truncate', '-s', '0', os.path.abspath(self.dirname2)+"/0"])
        self.admin.assert_icommand('ireg -R {0} -C {1} {2}/{3}'.format(self.testresc, os.path.abspath(self.dirname1),
                                                                self.admin.session_collection,
                                                                self.dirname1))
        self.admin.assert_icommand('ireg -R {0} -C {1} {2}/{3}'.format(self.testresc, os.path.abspath(self.dirname2),
                                                                self.admin.session_collection,
                                                                self.dirname2))

        # At this point, we have 2 files in each dir, one called "0",
        # which is 0 length, and the other called "1", which is not 0 length.

        # First verify correct behavior with no missing files on the filesystem:
        self._util_simple_icmd_assert('iscan -r {0}'.format(os.path.abspath(self.dirname1)))
        self._util_simple_icmd_assert('iscan -r {0}'.format(os.path.abspath(self.dirname2)))
        self._util_simple_icmd_assert('iscan -rd {0}/{1}'.format(self.admin.session_collection, self.dirname1))
        self._util_simple_icmd_assert('iscan -rd {0}/{1}'.format(self.admin.session_collection, self.dirname2))
        self._util_simple_icmd_assert('iscan {0}/0'.format(os.path.abspath(self.dirname1)))
        self._util_simple_icmd_assert('iscan {0}/1'.format(os.path.abspath(self.dirname1)))
        self._util_simple_icmd_assert('iscan {0}/0'.format(os.path.abspath(self.dirname2)))
        self._util_simple_icmd_assert('iscan {0}/1'.format(os.path.abspath(self.dirname2)))
        self._util_simple_icmd_assert('iscan -d {0}/{1}/0'.format(self.admin.session_collection, self.dirname1))
        self._util_simple_icmd_assert('iscan -d {0}/{1}/1'.format(self.admin.session_collection, self.dirname1))
        self._util_simple_icmd_assert('iscan -d {0}/{1}/0'.format(self.admin.session_collection, self.dirname2))
        self._util_simple_icmd_assert('iscan -d {0}/{1}/1'.format(self.admin.session_collection, self.dirname2))

        # All the way down to here, no icommand failures should be found.
        # From this point, we will be dealing with manually induced errors.

        # Check detection of physical file added manually to the
        # dir, while the dir was previously registered with irods

        lib.make_file( "{0}/{1}".format(os.path.abspath(self.dirname1),"3"), 0)
        self._util_simple_icmd_fail_stdout_assert('iscan -r {0}'.format(os.path.abspath(self.dirname1)), '3 is not registered in iRODS')
        os.unlink( os.path.abspath(self.dirname1)+"/3" )                    # Cleanup


        # Now, manually remove the 0 length file from self.dirname1,
        # and the non-zero length file from self.dirname2
        os.unlink( os.path.abspath(self.dirname1)+"/0" )                    # 0 length file in self.dirname1 is gone
        os.unlink( os.path.abspath(self.dirname2)+"/1" )                    # non-0 length file in self.dirname2 is gone

        # These two should NOT fail - check of physical dirs
        self._util_simple_icmd_assert('iscan -r {0}'.format(os.path.abspath(self.dirname1)))
        self._util_simple_icmd_assert('iscan -r {0}'.format(os.path.abspath(self.dirname2)))

        # This should fail on the missing 0-length file ("0")
        self._util_simple_icmd_fail_stdout_assert('iscan -rd {0}/{1}'.format(self.admin.session_collection,
                                                               self.dirname1),
                                                               " is missing, corresponding to iRODS object ")
        # This should fail on the missing non-0-length file ("1")
        self._util_simple_icmd_fail_stdout_assert('iscan -rd {0}/{1}'.format(self.admin.session_collection,
                                                               self.dirname2),
                                                               " is missing, corresponding to iRODS object ")

        # Checking the four physical files: the missing files are errors, the existing ones are not.
        self._util_simple_icmd_fail_stderr_assert('iscan {0}/0'.format(os.path.abspath(self.dirname1)), "0 does not exist")
        self._util_simple_icmd_assert('iscan {0}/1'.format(os.path.abspath(self.dirname1)))
        self._util_simple_icmd_assert('iscan {0}/0'.format(os.path.abspath(self.dirname2)))
        self._util_simple_icmd_fail_stderr_assert('iscan {0}/1'.format(os.path.abspath(self.dirname2)), "1 does not exist")

        self._util_simple_icmd_fail_stdout_assert('iscan -d {0}/{1}/0'.format(self.admin.session_collection, self.dirname1),
                                                                      " is missing, corresponding to iRODS object /")

        # These two files exist - no error
        self._util_simple_icmd_assert('iscan -d {0}/{1}/1'.format(self.admin.session_collection, self.dirname1))
        self._util_simple_icmd_assert('iscan -d {0}/{1}/0'.format(self.admin.session_collection, self.dirname2))

        self._util_simple_icmd_fail_stdout_assert('iscan -d {0}/{1}/1'.format(self.admin.session_collection, self.dirname2),
                                                                      " is missing, corresponding to iRODS object /")

    def test_iscan_does_not_core_dump_on_insufficient_permissions__issue_4613(self):
        # This test assumes /root has the following permissions: drwx------ N root root
        # "N" is just a placeholder in this example.
        self.admin.assert_icommand(['iscan', '/root'], 'STDERR', ['Permission denied', '"/root"'])

