from __future__ import print_function
import sys
if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import os

from . import resource_suite
from .. import lib
from .. import test
from .. import paths
from ..configuration import IrodsConfig

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
        self.admin.assert_icommand_fail(['ichksum', '-K', filename], 'STDOUT_SINGLELINE', 'Failed checksum = 1')
        self.admin.assert_icommand(['ichksum', '-K', filename], 'STDERR_SINGLELINE', 'USER_CHKSUM_MISMATCH')

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for topology testing")
    def test_ichksum_truncating_printed_filename__issue_3085(self):
        filename = 'issue_3085_012345678901234567890123456789.txt'
        filename_path = os.path.join(self.admin.local_session_dir, 'issue_3085_012345678901234567890123456789.txt')
        lib.make_file(filename_path, 1024, 'arbitrary')
        self.admin.assert_icommand(['iput', filename_path])
        self.admin.assert_icommand(['ichksum', filename], 'STDOUT_SINGLELINE', filename)

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for topology testing")
    def test_ichksum_fails_with_dyn_pep__issue_3485(self):
        config = IrodsConfig()

        # Create a file and make iRODS aware of it.
        # If this were done after adding the rule to the rulebase, an
        # error would be returned.
        filename = 'test_file_issue_3485.txt'
        lib.make_file(filename, 1024, 'arbitrary')
        chksum = lib.file_digest(filename, 'sha256', encoding='base64')
        self.admin.assert_icommand(['iput', filename])

        with lib.file_backed_up(config.server_config_path):
            # Create a new rule file that contains a pep with the wrong signature.
            # This particular pep is required in order to show that a more appropriate
            # error code/status is returned instead of [SYS_INTERNAL_ERR].
            rule_filename = 'issue_3485_rule.re'
            rule_filename_path = os.path.join(paths.core_re_directory(), rule_filename)
            with open(rule_filename_path, 'w') as f:
                f.write("pep_database_mod_data_obj_meta_pre() {}\n")

            # Add the newly created rule file to the rule engine rulebase.
            rule_engine = config.server_config['plugin_configuration']['rule_engines'][0]
            rule_engine['plugin_specific_configuration']['re_rulebase_set'][0] = rule_filename[:-3]
            lib.update_json_file_from_dict(config.server_config_path, config.server_config)

            self.admin.assert_icommand(['ichksum', filename], 'STDERR', 'status = -1097000 NO_RULE_OR_MSI_FUNCTION_FOUND_ERR')

            # Update the rule to the correct signature.
            # Running ichksum after this change should work as expected and print
            # the requested information.
            with open(rule_filename_path, 'w') as f:
                f.write("pep_database_mod_data_obj_meta_pre(*a, *b, *c, *d, *e) {}\n")

            self.admin.assert_icommand(['ichksum', filename], 'STDOUT', [filename, chksum])

            os.remove(rule_filename_path)

        self.admin.assert_icommand(['irm', '-f', filename])

