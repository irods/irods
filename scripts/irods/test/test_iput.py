from __future__ import print_function
import os
import sys
import tempfile

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from .. import test
from .. import lib
from ..test.command import assert_command

rodsadmins = [('otherrods', 'rods')]
rodsusers  = [('alice', 'apass')]

class Test_Iput(session.make_sessions_mixin(rodsadmins, rodsusers), unittest.TestCase):

    def setUp(self):
        super(Test_Iput, self).setUp()
        self.admin = self.admin_sessions[0]
        self.user = self.user_sessions[0]

    def tearDown(self):
        super(Test_Iput, self).tearDown()

    def test_parallel_put_does_not_generate_SYS_COPY_LEN_ERR_when_overwriting_large_file_with_small_file__issue_5505(self):
        file_40mb = os.path.join(tempfile.gettempdir(), '40mb.txt')
        lib.make_file(file_40mb, 40000000, 'arbitrary')

        file_50mb = os.path.join(tempfile.gettempdir(), '50mb.txt')
        lib.make_file(file_50mb, 50000000, 'arbitrary')

        data_object = 'foo'
        self.admin.assert_icommand(['iput', file_50mb, data_object])

        # The following command should not result in a SYS_COPY_LEN_ERR error.
        self.admin.assert_icommand(['iput', '-f', file_40mb, data_object])

    @unittest.skipIf(False == test.settings.USE_MUNGEFS, "This test requires mungefs")
    def test_failure_in_data_transfer(self):
        munge_mount = os.path.join(self.admin.local_session_dir, 'munge_mount')
        munge_target = os.path.join(self.admin.local_session_dir, 'munge_target')
        munge_resc = 'munge_resc'
        local_file = os.path.join(self.admin.local_session_dir, 'local_file')
        logical_path = os.path.join(self.admin.session_collection, 'foo')
        question = '''"select DATA_REPL_NUM, DATA_RESC_NAME, DATA_REPL_STATUS where DATA_NAME = '{0}' and COLL_NAME = '{1}'"'''.format(
            os.path.basename(logical_path), os.path.dirname(logical_path))

        try:
            lib.make_dir_p(munge_mount)
            lib.make_dir_p(munge_target)
            assert_command(['mungefs', munge_mount, '-omodules=subdir,subdir={}'.format(munge_target)])

            self.admin.assert_icommand(
                ['iadmin', 'mkresc', munge_resc, 'unixfilesystem',
                 ':'.join([test.settings.HOSTNAME_1, munge_mount])], 'STDOUT_SINGLELINE', 'unixfilesystem')

            assert_command(['mungefsctl', '--operations', 'write', '--corrupt_data'])

            if not os.path.exists(local_file):
                lib.make_file(local_file, 1024)

            self.admin.assert_icommand(
                ['iput', '-K', '-R', munge_resc, local_file, logical_path],
                'STDERR', 'USER_CHKSUM_MISMATCH')

            # check on the replicas after the put...
            out, err, ec = self.admin.run_icommand(['iquest', '%s %s %s', question])
            self.assertEqual(0, ec)
            self.assertEqual(len(err), 0)

            # there should be 1 replica...
            out_lines = out.splitlines()
            self.assertEqual(len(out_lines), 1)

            # ensure the new replica is marked stale
            repl_num_0, resc_name_0, repl_status_0 = out_lines[0].split()
            self.assertEqual(int(repl_num_0),    0)          # DATA_REPL_NUM
            self.assertEqual(str(resc_name_0),   munge_resc) # DATA_RESC_NAME
            self.assertEqual(int(repl_status_0), 0)          # DATA_REPL_STATUS

        finally:
            self.admin.run_icommand(['irm', '-f', logical_path])
            assert_command(['mungefsctl', '--operations', 'write'])
            assert_command(['fusermount', '-u', munge_mount])
            self.admin.assert_icommand(['iadmin', 'rmresc', munge_resc])

class test_iput_with_checksums(session.make_sessions_mixin(rodsadmins, rodsusers), unittest.TestCase):

    def setUp(self):
        super(test_iput_with_checksums, self).setUp()
        self.admin = self.admin_sessions[0]

        self.resource = 'test_iput_with_checksums_resource'
        self.vault_path = os.path.join(self.admin.local_session_dir, self.resource + '_storage_vault')
        self.admin.assert_icommand(
            ['iadmin', 'mkresc', self.resource, 'unixfilesystem', ':'.join([test.settings.HOSTNAME_2, self.vault_path])],
            'STDOUT', test.settings.HOSTNAME_2)

    def tearDown(self):
        self.admin.run_icommand(['irm', '-f', self.object_name])
        self.admin.assert_icommand(['iadmin', 'rmresc', self.resource])
        super(test_iput_with_checksums, self).tearDown()

    def test_zero_length_put(self):
        self.object_name = 'test_zero_length_put'
        self.run_tests(self.object_name, 0)

    def test_small_put(self):
        self.object_name = 'test_small_put'
        self.run_tests(self.object_name, 512)

    @unittest.skip('parallel transfer not yet working')
    def test_large_put(self):
        self.object_name = 'test_large_put'
        self.run_tests(self.object_name, 5120000)

    def get_checksum(self, object_name):
        iquest_result,_,ec = self.admin.run_icommand(['iquest', '%s', '"select DATA_CHECKSUM where DATA_NAME = \'{}\'"'.format(object_name)])
        self.assertEqual(0, ec)

        print(iquest_result)
        self.assertEqual(1, len(iquest_result.splitlines()))

        return '' if u'\n' == iquest_result else iquest_result

    def run_tests(self, filename, file_size):
        lib.make_file(filename, file_size, 'arbitrary')
        filename_tmp = filename + '_tmp'
        self.admin.assert_icommand(['iput', '-K', filename, filename_tmp])
        expected_checksum = self.get_checksum(filename_tmp)

        try:
            self.new_register(filename, expected_checksum)
            self.new_verify(filename, expected_checksum)
            self.overwrite_register_good_no_checksum(filename, expected_checksum)
            self.overwrite_register_good_with_checksum(filename, expected_checksum)
            self.overwrite_register_stale_no_checksum(filename, expected_checksum)
            self.overwrite_register_stale_with_checksum(filename, expected_checksum)
            self.overwrite_verify_good_no_checksum(filename, expected_checksum)
            self.overwrite_verify_good_with_checksum(filename, expected_checksum)
            self.overwrite_verify_stale_no_checksum(filename, expected_checksum)
            self.overwrite_verify_stale_with_checksum(filename, expected_checksum)
        finally:
            os.unlink(filename)
            self.admin.run_icommand(['irm', '-f', filename_tmp])

    def new_register(self, filename, expected_checksum):
        try:
            self.admin.assert_icommand(['iput', '-R', self.resource, '-k', filename, self.object_name])
            self.assertEqual(self.get_checksum(self.object_name), expected_checksum)
        finally:
            self.admin.assert_icommand(['irm', '-f', self.object_name])

    def new_verify(self, filename, expected_checksum):
        try:
            self.admin.assert_icommand(['iput', '-R', self.resource, '-K', filename, self.object_name])
            self.assertEqual(self.get_checksum(self.object_name), expected_checksum)
        finally:
            self.admin.assert_icommand(['irm', '-f', self.object_name])

    def overwrite_register_good_no_checksum(self, filename, expected_checksum):
        filename_tmp = filename + '_for_overwrite'
        lib.make_file(filename_tmp, 4444, 'arbitrary')

        try:
            self.admin.assert_icommand(['iput', '-R', self.resource, filename_tmp, self.object_name])
            self.assertEqual(self.get_checksum(self.object_name), '')

            self.admin.assert_icommand(['iput', '-R', self.resource, '-f', '-k', filename, self.object_name])
            self.assertEqual(self.get_checksum(self.object_name), expected_checksum)
        finally:
            os.unlink(filename_tmp)
            self.admin.assert_icommand(['irm', '-f', self.object_name])

    def overwrite_register_good_with_checksum(self, filename, expected_checksum):
        filename_tmp = filename + '_for_overwrite'
        lib.make_file(filename_tmp, 4444, 'arbitrary')
        self.admin.assert_icommand(['iput', '-R', self.resource, '-K', filename_tmp])
        tmp_checksum = self.get_checksum(filename_tmp)

        try:
            self.admin.assert_icommand(['iput', '-R', self.resource, '-k', filename_tmp, self.object_name])
            self.assertEqual(self.get_checksum(self.object_name), tmp_checksum)

            self.admin.assert_icommand(['iput', '-R', self.resource, '-f', '-k', filename, self.object_name])
            self.assertEqual(self.get_checksum(self.object_name), expected_checksum)
        finally:
            os.unlink(filename_tmp)
            self.admin.assert_icommand(['irm', '-f', self.object_name])
            self.admin.assert_icommand(['irm', '-f', filename_tmp])

    def overwrite_no_checksum_flag_good_with_checksum(self, filename, expected_checksum):
        filename_tmp = filename + '_for_overwrite'
        lib.make_file(filename_tmp, 4444, 'arbitrary')
        self.admin.assert_icommand(['iput', '-R', self.resource, '-K', filename_tmp])
        tmp_checksum = self.get_checksum(filename_tmp)

        try:
            self.admin.assert_icommand(['iput', '-R', self.resource, '-k', filename_tmp, self.object_name])
            self.assertEqual(self.get_checksum(self.object_name), tmp_checksum)

            self.admin.assert_icommand(['iput', '-R', self.resource, '-f', filename, self.object_name])
            self.assertEqual(self.get_checksum(self.object_name), expected_checksum)
        finally:
            os.unlink(filename_tmp)
            self.admin.assert_icommand(['irm', '-f', self.object_name])
            self.admin.assert_icommand(['irm', '-f', filename_tmp])

    def overwrite_register_stale_no_checksum(self, filename, expected_checksum):
        pass

    def overwrite_register_stale_with_checksum(self, filename, expected_checksum):
        pass

    def overwrite_verify_good_no_checksum(self, filename, expected_checksum):
        filename_tmp = filename + '_for_overwrite'
        lib.make_file(filename_tmp, 4444, 'arbitrary')

        try:
            self.admin.assert_icommand(['iput', '-R', self.resource, filename_tmp, self.object_name])
            self.assertEqual(self.get_checksum(self.object_name), '')

            self.admin.assert_icommand(['iput', '-R', self.resource, '-f', '-K', filename, self.object_name])
            self.assertEqual(self.get_checksum(self.object_name), expected_checksum)
        finally:
            os.unlink(filename_tmp)
            self.admin.assert_icommand(['irm', '-f', self.object_name])

    def overwrite_verify_good_with_checksum(self, filename, expected_checksum):
        filename_tmp = filename + '_for_overwrite'
        lib.make_file(filename_tmp, 4444, 'arbitrary')
        self.admin.assert_icommand(['iput', '-R', self.resource, '-K', filename_tmp])
        tmp_checksum = self.get_checksum(filename_tmp)

        try:
            self.admin.assert_icommand(['iput', '-R', self.resource, '-k', filename_tmp, self.object_name])
            self.assertEqual(self.get_checksum(self.object_name), tmp_checksum)

            self.admin.assert_icommand(['iput', '-R', self.resource, '-f', '-K', filename, self.object_name])
            self.assertEqual(self.get_checksum(self.object_name), expected_checksum)
        finally:
            os.unlink(filename_tmp)
            self.admin.assert_icommand(['irm', '-f', self.object_name])
            self.admin.assert_icommand(['irm', '-f', filename_tmp])

    def overwrite_verify_stale_no_checksum(self, filename, expected_checksum):
        pass

    def overwrite_verify_stale_with_checksum(self, filename, expected_checksum):
        pass

