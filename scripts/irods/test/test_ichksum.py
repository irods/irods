from __future__ import print_function
import sys
if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import os

from . import session
from . import settings
from . import resource_suite
from .. import lib
from .. import test

class Test_Ichksum(resource_suite.ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_Ichksum, self).setUp()

    def tearDown(self):
        super(Test_Ichksum, self).tearDown()

    def test_ichksum_data_obj(self):
        filename = 'test_ichksum_data_obj'
        lib.make_file(filename, 1024, 'arbitrary')
        file_chksum = lib.file_digest(filename, 'sha256', encoding='base64')
        self.admin.assert_icommand(['iput', filename])
        self.admin.assert_icommand(['ichksum', filename], 'STDOUT_SINGLELINE', file_chksum)

    def test_ichksum_coll(self):
        collname = 'test_ichksum_coll'
        filename = 'file'
        lib.make_file(filename, 1024, 'arbitrary')
        file_chksum = lib.file_digest(filename, 'sha256', encoding='base64')
        self.admin.assert_icommand(['imkdir', collname])
        self.admin.assert_icommand(['iput', filename, collname + '/' + filename])
        self.admin.assert_icommand(['ichksum', collname], 'STDOUT_SINGLELINE', file_chksum)

    def test_ichksum_recursive(self):
        collname_1 = 'test_ichksum_recursive'
        filename_1 = 'file1'
        collname_2 = 'subdir'
        filename_2 = 'file2'
        collname_3 = 'subsubdir'
        filename_3 = 'file_3'
        lib.make_file(filename_1, 256, 'arbitrary')
        lib.make_file(filename_2, 512, 'arbitrary')
        lib.make_file(filename_3, 1024, 'arbitrary')
        file_chksum_1 = lib.file_digest(filename_1, 'sha256', encoding='base64')
        file_chksum_2 = lib.file_digest(filename_2, 'sha256', encoding='base64')
        file_chksum_3 = lib.file_digest(filename_3, 'sha256', encoding='base64')
        self.admin.assert_icommand(['imkdir', collname_1])
        self.admin.assert_icommand(['iput', filename_1, collname_1 + '/' + filename_1])
        self.admin.assert_icommand(['imkdir', collname_1 + '/' + collname_2])
        self.admin.assert_icommand(['iput', filename_2, collname_1 + '/' + collname_2 + '/' + filename_2])
        self.admin.assert_icommand(['imkdir', collname_1 + '/' + collname_2 + '/' + collname_3])
        self.admin.assert_icommand(['iput', filename_3, collname_1 + '/' + collname_2 + '/' + collname_3 + '/' + filename_3])
        self.admin.assert_icommand(['ichksum', collname_1], 'STDOUT_MULTILINE', [file_chksum_3, file_chksum_2, file_chksum_1])

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for topology testing: requires Vault")
    def test_ichksum_force(self):
        filename = 'test_ichksum_force'
        lib.make_file(filename, 1024, 'arbitrary')
        file_chksum = lib.file_digest(filename, 'sha256', encoding='base64')
        self.admin.assert_icommand(['iput', filename])
        self.admin.assert_icommand(['ichksum', filename], 'STDOUT_SINGLELINE', file_chksum)
        # Mess with file in vault
        full_path = os.path.join(self.admin.get_vault_session_path(), filename)
        lib.cat(full_path, 'XXXXX')
        new_chksum = lib.file_digest(full_path, 'sha256', encoding='base64')
        self.admin.assert_icommand(['ichksum', '-f', filename], 'STDOUT_SINGLELINE', new_chksum)

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for topology testing: requires Vault")
    def test_ichksum_verify(self):
        filename = 'test_ichksum_verify'
        lib.make_file(filename, 1024, 'arbitrary')
        file_chksum = lib.file_digest(filename, 'sha256', encoding='base64')
        self.admin.assert_icommand(['iput', filename])
        self.admin.assert_icommand(['ichksum', filename], 'STDOUT_SINGLELINE', file_chksum)
        # Mess with file in vault
        full_path = os.path.join(self.admin.get_vault_session_path(), filename)
        lib.cat(full_path, 'XXXXX')
        new_chksum = lib.file_digest(full_path, 'sha256', encoding='base64')
        self.admin.assert_icommand_fail(['ichksum', '-K', filename], 'STDOUT_SINGLELINE', 'Failed checksum = 1')
        self.admin.assert_icommand(['ichksum', '-K', filename], 'STDERR_SINGLELINE', 'USER_CHKSUM_MISMATCH')
