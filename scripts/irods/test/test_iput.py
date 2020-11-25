from __future__ import print_function
import os
import sys
if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from .. import test
from .. import lib

class test_iput_with_checksums(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):
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

