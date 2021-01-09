from __future__ import print_function
import sys
if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import os
import shutil
import re

from . import session
from . import settings
from . import resource_suite
from .. import lib
from .. import test
from .. import paths
from ..configuration import IrodsConfig

class Test_Ichksum(resource_suite.ResourceBase, unittest.TestCase):
    plugin_name = IrodsConfig().default_rule_engine_plugin

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

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for topology testing")
    def test_ichksum_truncating_printed_filename__issue_3085(self):
        filename = 'issue_3085_012345678901234567890123456789.txt'
        filename_path = os.path.join(self.admin.local_session_dir, 'issue_3085_012345678901234567890123456789.txt')
        lib.make_file(filename_path, 1024, 'arbitrary')
        self.admin.assert_icommand(['iput', filename_path])
        self.admin.assert_icommand(['ichksum', filename], 'STDOUT_SINGLELINE', filename)

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for topology testing")
    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for irods_rule_language REP')
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

    def test_chksum_catalog_verify_not_exhausting_statements__issue_4732(self):
        Statement_Table_Size = 50
        filecount = Statement_Table_Size + 10
        dirname = 'ichksum_targets_4732'
        lib.create_directory_of_small_files(dirname, filecount)
        try:
            _,err,rcAdm = self.admin.run_icommand(['iput', '-r','-k', dirname])
            _,err,rcUsr = self.user0.run_icommand(['iput', '-r','-k', dirname])
            self.assertTrue(rcAdm == 0 and rcUsr == 0, '**** could not iput -r -k directory/ies in test for #4732')
            tests = [ (self.admin, ['ichksum','--verify','-r','-KM',dirname], 'CAT_STATEMENT_TABLE_FULL' ),
                      (self.admin, ['ichksum','--verify','-r','-K', dirname], 'CAT_NO_ACCESS_PERMISSION' ),
                      (self.user0, ['ichksum','--verify','-r','-K', dirname], 'CAT_NO_ACCESS_PERMISSION' ) ]
            for user,cmd,guard_err in tests:
                _,err,rc = user.run_icommand( cmd )
                self.assertTrue (guard_err not in err, 'ichksum incurred statement table exhaustion #4732')
                self.assertTrue (err == '', 'ichksum produced unwelcome STDERR output')
                self.assertTrue (rc == 0, 'ichksum returned nonzero error code during routine operation')
        finally:
            clean_exit = True
            try:
                self.user0.assert_icommand(['irm', '-rf', dirname])
            except:
                clean_exit = False
            try:
                self.admin.assert_icommand(['irm', '-rf', dirname])
            except:
                clean_exit = False
            shutil.rmtree(dirname, ignore_errors=True)
            self.assertTrue (clean_exit, '**** inadequate clean-up in test for #4732 ***')

    def test_ichksum_reports_incompatible_params__issue_5252(self):
        data_object = 'foo'
        self.admin.assert_icommand(['istream', 'write', data_object], input='some data')
        self.admin.assert_icommand(['ichksum', '-n', '0', '-R', 'demoResc', data_object], 'STDERR', ["the 'n' and 'R' option cannot be used together"])
        self.admin.assert_icommand(['ichksum', '-a', '-n', '0', data_object], 'STDERR', ["the 'n' and 'a' option cannot be used together"])
        self.admin.assert_icommand(['ichksum', '-a', '-R', 'demoResc', data_object], 'STDERR', ["the 'R' and 'a' option cannot be used together"])
        self.admin.assert_icommand(['ichksum', '-K', '-f', data_object], 'STDERR', ["the 'K' and 'f' option cannot be used together"])
        self.admin.assert_icommand(['ichksum', '-K', '-a', '-n', '0', data_object], 'STDERR', ["the 'n' and 'a' option cannot be used together"])
        self.admin.assert_icommand(['ichksum', '-K', '-a', '-R', 'demoResc', data_object], 'STDERR', ["the 'R' and 'a' option cannot be used together"])
        self.admin.assert_icommand(['ichksum', '-K', '--silent', data_object], 'STDERR', ["the 'K' and 'silent' option cannot be used together"])
        self.admin.assert_icommand(['ichksum', '--verify', '--silent', data_object], 'STDERR', ["the 'verify' and 'silent' option cannot be used together"])

    def test_ichksum_reports_number_of_replicas_skipped__issue_5252(self):
        data_object = os.path.join(self.admin.session_collection, 'foo')
        self.admin.assert_icommand(['istream', 'write', data_object], input='some data')
        self.admin.assert_icommand(['irepl', '-R', self.testresc, data_object])

        # Show that ichksum reports about skipping replicas that are not good.
        self.admin.assert_icommand(['iadmin', 'modrepl', 'logical_path', data_object, 'replica_number', '0', 'DATA_REPL_STATUS', '0'])
        self.admin.assert_icommand(['ichksum', '-K', data_object], 'STDOUT', ['INFO: Number of replicas skipped: 1'])

        # Show that ichksum reports information about non-good replicas when they exist.
        self.admin.assert_icommand(['iadmin', 'modrepl', 'logical_path', data_object, 'replica_number', '0', 'DATA_REPL_STATUS', '1'])
        out, _, ec = self.admin.run_icommand(['ichksum', '-K', data_object])
        self.assertTrue(ec == 0 and 'INFO: Number of replicas' not in out)

    def test_ichksum_reports_when_replicas_physical_size_does_not_match_size_in_catalog__issue_5252(self):
        data_object = os.path.join(self.admin.session_collection, 'foo')
        self.admin.assert_icommand(['istream', 'write', data_object], input='some data')
        self.admin.assert_icommand(['iadmin', 'modrepl', 'logical_path', data_object, 'replica_number', '0', 'DATA_SIZE', '1'])
        self.admin.assert_icommand(['ichksum', '-K', data_object], 'STDOUT', ['ERROR: Physical size does not match size in catalog for replica [0].'])

        # Show that --verify is an alias for -K.
        self.admin.assert_icommand(['ichksum', '--verify', data_object], 'STDOUT', ['ERROR: Physical size does not match size in catalog for replica [0].'])

        # Show that the error is printed when targeting a specific replica.
        self.admin.assert_icommand(['ichksum', '-K', '-n0', data_object], 'STDOUT', ['ERROR: Physical size does not match size in catalog for replica [0].'])

    def test_ichksum_reports_when_replicas_are_missing_checksums__issue_5252(self):
        data_object = 'foo'
        self.admin.assert_icommand(['istream', 'write', data_object], input='some data')
        self.admin.assert_icommand(['irepl', '-R', self.testresc, data_object])
        self.admin.assert_icommand(['ichksum', '-n0', data_object], 'STDOUT', [data_object + '    sha2:'])
        self.admin.assert_icommand(['ichksum', '-K', data_object], 'STDOUT', ['WARNING: No checksum available for replica [1].'])

        # Show that --verify is an alias for -K.
        self.admin.assert_icommand(['ichksum', '--verify', data_object], 'STDOUT', ['WARNING: No checksum available for replica [1].'])

        # Show that the warning is printed when targeting a specific replica.
        self.admin.assert_icommand(['ichksum', '-K', '-n1', data_object], 'STDOUT', ['WARNING: No checksum available for replica [1].'])

    def test_ichksum_reports_when_replicas_computed_checksum_does_not_match_checksum_in_catalog__issue_5252(self):
        data_object = os.path.join(self.admin.session_collection, 'foo')
        self.admin.assert_icommand(['istream', 'write', data_object], input='some data')
        self.admin.assert_icommand(['ichksum', data_object], 'STDOUT', [os.path.basename(data_object) + '    sha2:'])
        self.admin.assert_icommand(['iadmin', 'modrepl', 'logical_path', data_object, 'replica_number', '0', 'DATA_CHECKSUM', 'sha2:BAD_CHECKSUM'])
        self.admin.assert_icommand(['ichksum', '-K', data_object], 'STDOUT', ['ERROR: Computed checksum does not match what is in the catalog for replica [0].'])

        # Show that the error is printed when targeting a specific replica.
        self.admin.assert_icommand(['ichksum', '-K', '-n0', data_object], 'STDOUT', ['ERROR: Computed checksum does not match what is in the catalog for replica [0].'])

        # Show that when --no-compute is passed, verification mode will not verify if the checksum in the
        # catalog is correct or not, regardless of whether the client is targeting one or all replicas.
        self.admin.assert_icommand(['ichksum', '-K', '--no-compute', data_object])
        self.admin.assert_icommand(['ichksum', '-K', '--no-compute', '-n0', data_object])

    def test_ichksum_reports_when_replicas_do_not_share_identical_checksums__issue_5252(self):
        data_object = 'foo'
        self.admin.assert_icommand(['istream', 'write', data_object], input='some data')
        self.admin.assert_icommand(['irepl', '-R', self.testresc, data_object])
        
        # Change the contents of replica 0.
        # This will cause ichksum to report a warning.
        gql = "select DATA_PATH where COLL_NAME = '{0}' and DATA_NAME = '{1}' and DATA_REPL_NUM = '0'".format(self.admin.session_collection, data_object)
        out, _, ec = self.admin.run_icommand(['iquest', '%s', gql])
        self.assertEqual(ec, 0)
        out = out.strip()
        self.assertGreater(len(out), 0)
        with open(out, 'r+') as f:
            f.write('test')

        self.admin.assert_icommand(['ichksum', '-a', data_object], 'STDOUT', ['WARNING: Data object has replicas with different checksums.'])

    def test_ichksum_computes_checksum_for_highest_voted_replica_when_no_options_are_present__issue_5252(self):
        data_object = 'foo'
        self.admin.assert_icommand(['istream', 'write', data_object], input='some data')
        self.admin.assert_icommand(['irepl', '-R', self.testresc, data_object])
        self.admin.assert_icommand(['ichksum', data_object], 'STDOUT', [data_object + '    sha2:'])

        # Show that only one of the two replicas has a checksum.
        out1, _, ec1 = self.admin.run_icommand(['ichksum', '-K', '-n0', data_object])
        self.assertTrue(ec1 == 0, 'Unexpected error')

        out2, _, ec2 = self.admin.run_icommand(['ichksum', '-K', '-n1', data_object])
        self.assertTrue(ec2 == 0, 'Unexpected error')

        msg1 = 'WARNING: No checksum available for replica [0].'
        msg2 = 'WARNING: No checksum available for replica [1].'
        self.assertTrue((msg1 in out1 and msg2 not in out2) or (msg1 not in out1 and msg2 in out2))

    def test_silent_option_is_supported__issue_5252(self):
        data_object = 'foo'
        self.admin.assert_icommand(['istream', 'write', data_object], input='some data')
        self.admin.assert_icommand(['irepl', '-R', self.testresc, data_object])
        self.admin.assert_icommand(['ichksum', '-a', '--silent', data_object])

        # Verify that the data object's replicas have checksums.
        gql = "select DATA_CHECKSUM where COLL_NAME = '{0}' and DATA_NAME = '{1}'".format(self.admin.session_collection, data_object)
        self.admin.assert_icommand(['iquest', '%s', gql], 'STDOUT', ['sha2:'])

    def test_recursive_option_is_supported__issue_5252(self):
        coll_a = os.path.join(self.admin.session_collection, 'coll_a.5252')
        self.admin.assert_icommand(['imkdir', coll_a])

        coll_b = os.path.join(self.admin.session_collection, 'coll_b.5252')
        self.admin.assert_icommand(['imkdir', coll_b])

        # Create three data objects with one additional replica.
        # Place one data object in each collection.
        data_objects = [os.path.join(self.admin.session_collection, 'foo'),
                        os.path.join(coll_a, 'bar'),
                        os.path.join(coll_b, 'baz')]
        for data_object in data_objects:
            self.admin.assert_icommand(['istream', 'write', data_object], input='some special sauce!')
            self.admin.assert_icommand(['irepl', '-R', self.testresc, data_object])

        # Checksum all of the data objects (and replicas).
        self.admin.assert_icommand(['ichksum', '-r', '-a', self.admin.session_collection], 'STDOUT', [
            'C- ' + os.path.dirname(data_objects[0]),
            'C- ' + os.path.dirname(data_objects[1]),
            'C- ' + os.path.dirname(data_objects[2]),
            '    ' + os.path.basename(data_objects[0]) + '    sha2:',
            '    ' + os.path.basename(data_objects[1]) + '    sha2:',
            '    ' + os.path.basename(data_objects[2]) + '    sha2:'
        ])

        # Show that using --silent produces no output when all replicas are in a good state.
        self.admin.assert_icommand(['ichksum', '-r', '-a', '--silent', self.admin.session_collection])

    def test_ichksum_ignores_replicas_that_are_not_marked_good_or_stale__issue_5252(self):
        data_object = os.path.join(self.admin.session_collection, 'foo')
        self.admin.assert_icommand(['istream', 'write', data_object], input='some data')

        # Set the replica's status to an unknown value, in this test, 3.
        # If the replica's status was set to intermediate (2), ichksum would return HIERARCHY_ERROR.
        self.admin.assert_icommand(['iadmin', 'modrepl', 'logical_path', data_object, 'replica_number', '0', 'DATA_REPL_STATUS', '3'])

        self.admin.assert_icommand(['ichksum', data_object], 'STDERR', ['SYS_REPLICA_INACCESSIBLE'])
        self.admin.assert_icommand(['ichksum', '-n0', data_object], 'STDERR', ['SYS_REPLICA_INACCESSIBLE'])
        self.admin.assert_icommand(['ichksum', '-a', data_object], 'STDERR', ['SYS_NO_GOOD_REPLICA'])

    def test_ichksum_returns_immediately_if_processing_replicas_in_the_bundle_resource__issue_5252(self):
        data_object = os.path.join(self.admin.session_collection, 'foo')
        self.admin.assert_icommand(['istream', 'write', data_object], input='some data')

        # Capture the replica's resource id.
        gql = "select RESC_ID where COLL_NAME = '{0}' and DATA_NAME = '{1}'".format(self.admin.session_collection, os.path.basename(data_object))
        original_resc_id, _, ec = self.admin.run_icommand(['iquest', '%s', gql])
        self.assertEqual(ec, 0)

        # Get the resource id of the bundle resource and update the replica's resource id column.
        bundle_resc_id, _, ec = self.admin.run_icommand(['iquest', '%s', "select RESC_ID where RESC_NAME = 'bundleResc'"])
        self.assertEqual(ec, 0)
        self.admin.assert_icommand(['iadmin', 'modrepl', 'logical_path', data_object, 'replica_number', '0', 'DATA_RESC_ID', bundle_resc_id.strip()])

        self.admin.assert_icommand(['ichksum', data_object], 'STDERR', ['SYS_CANT_CHKSUM_BUNDLED_DATA'])
        self.admin.assert_icommand(['ichksum', '-n0', data_object], 'STDERR', ['SYS_CANT_CHKSUM_BUNDLED_DATA'])

        # Restore the replica's original resource id.
        self.admin.assert_icommand(['iadmin', 'modrepl', 'logical_path', data_object, 'replica_number', '0', 'DATA_RESC_ID', original_resc_id.strip()])

    def test_ichksum_is_not_allowed_access_to_intermediate_replicas__issue_5252(self):
        data_object = os.path.join(self.admin.session_collection, 'foo')
        self.admin.assert_icommand(['istream', 'write', data_object], input='some data')
        self.admin.assert_icommand(['iadmin', 'modrepl', 'logical_path', data_object, 'replica_number', '0', 'DATA_REPL_STATUS', '2'])
        self.admin.assert_icommand(['ichksum', data_object], 'STDERR', ['HIERARCHY_ERROR'])
        self.admin.assert_icommand(['ichksum', '-n0', data_object], 'STDERR', ['HIERARCHY_ERROR'])
        self.admin.assert_icommand(['ichksum', '-a', data_object], 'STDERR', ['HIERARCHY_ERROR'])

    def test_ichksum_groups_objects_appropriately_when_the_recursive_flag_is_set__issue_5285(self):
        # Create a test collection. This avoids issues with data objects created by
        # the base class appearing in the output.
        root_col = os.path.join(self.admin.session_collection, 'issue_5285')
        self.admin.assert_icommand(['imkdir', root_col])

        # Add a new data object to the root test collection.
        data_object_0 = 'foo'
        self.admin.assert_icommand(['istream', 'write', os.path.join(root_col, data_object_0)], input='X')

        # Create a new collection and add two data objects to it.
        data_object_1 = 'bar'
        data_object_2 = 'baz'
        col_1 = os.path.join(root_col, 'col_1')
        self.admin.assert_icommand(['imkdir', col_1])
        self.admin.assert_icommand(['istream', 'write', os.path.join(col_1, data_object_1)], input='Y')
        self.admin.assert_icommand(['istream', 'write', os.path.join(col_1, data_object_2)], input='Z')

        # Create a third collection inside of the previously created collection and
        # add one data object to it.
        data_object_3 = 'goo'
        col_2 = os.path.join(col_1, 'col_2')
        self.admin.assert_icommand(['imkdir', col_2])
        self.admin.assert_icommand(['istream', 'write', os.path.join(col_2, data_object_3)], input='W')

        # Verify that the output is correct.
        out, _, ec = self.admin.run_icommand(['ichksum', '-r', root_col])
        self.assertEqual(ec, 0)
        pattern = '''C- {0}:
    {1}    sha2:.+
C- {2}:
    {3}    sha2:.+
    {4}    sha2:.+
C- {5}:
    {6}    sha2:.+
'''.format(root_col,
           data_object_0,
           col_1,
           data_object_1,
           data_object_2,
           col_2,
           data_object_3)
        self.assertTrue(re.match(pattern, out))

