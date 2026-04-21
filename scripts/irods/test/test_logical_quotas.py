import os
import sys
import unittest

from . import session
from .. import lib
from .. import paths
from .resource_suite import ResourceBase
from ..configuration import IrodsConfig
from ..controller import IrodsController

class Test_Logical_Quotas(session.make_sessions_mixin([('otherrods', 'rods')], []), unittest.TestCase):

    def setUp(self):
        super(Test_Logical_Quotas, self).setUp()

        self.admin = self.admin_sessions[0]

        self.quota_user = session.mkuser_and_return_session('rodsuser', 'lquotauser', 'lquotapass', lib.get_hostname())
        self.llq_output_template = 'Collection name: %s\nMaximum bytes: %s\nMaximum objects: %s\nBytes over: %s\nObjects over: %s'

    def tearDown(self):
        self.quota_user.__exit__()
        self.admin.assert_icommand(['iadmin', 'rmuser', 'lquotauser'])
        super(Test_Logical_Quotas, self).tearDown()

    def test_logical_quota_set_nonexistent(self):
        self.admin.assert_icommand(['iadmin', 'set_logical_quota', f'{self.quota_user.session_collection}/blah', 'bytes', '10000'], 'STDERR_SINGLELINE', 'CAT_INVALID_ARGUMENT' )

    def test_logical_quota_list_nonexistent(self):
        self.admin.assert_icommand(['iadmin', 'list_logical_quotas', f'{self.quota_user.session_collection}/blah'], 'STDERR_SINGLELINE', 'CAT_UNKNOWN_COLLECTION' )

    def test_logical_quota_set_bad_values(self):
        try:
            # Nonexistent keyword
            self.admin.assert_icommand(['iadmin', 'set_logical_quota', f'{self.quota_user.session_collection}', 'potatoes', '10000'], 'STDERR_SINGLELINE', 'SYS_INVALID_INPUT_PARAM' )

            # Negative values
            self.admin.assert_icommand(['iadmin', 'set_logical_quota', f'{self.quota_user.session_collection}', 'bytes', '--', '-10000'], 'STDERR_SINGLELINE', 'USER_INPUT_FORMAT_ERR' )
            self.admin.assert_icommand(['iadmin', 'set_logical_quota', self.quota_user.session_collection, 'objects', '--', '-10000'], 'STDERR_SINGLELINE', 'USER_INPUT_FORMAT_ERR' )
            self.admin.assert_icommand(['iadmin', 'set_logical_quota', self.quota_user.session_collection, '0', '--', '-10000'], 'STDERR_SINGLELINE', 'USER_INPUT_FORMAT_ERR' )
            self.admin.assert_icommand(['iadmin', 'set_logical_quota', self.quota_user.session_collection, '--', '-10000', '10000'], 'STDERR_SINGLELINE', 'USER_INPUT_FORMAT_ERR' )
            self.admin.assert_icommand(['iadmin', 'set_logical_quota', self.quota_user.session_collection, '--', '-10000', '-10000'], 'STDERR_SINGLELINE', 'USER_INPUT_FORMAT_ERR' )

            # Invalid final parameter
            self.admin.assert_icommand(['iadmin', 'set_logical_quota', self.quota_user.session_collection, 'bytes', 'tomatoes'], 'STDERR_SINGLELINE', 'SYS_INVALID_INPUT_PARAM' )

            # Numeric overflow
            self.admin.assert_icommand(['iadmin', 'set_logical_quota', self.quota_user.session_collection, 'bytes', '218128128218374562312312412412'], 'STDERR_SINGLELINE', 'SYS_INVALID_INPUT_PARAM' )
        finally:
            self.admin.assert_icommand(['iadmin', 'set_logical_quota', self.quota_user.session_collection, '0', '0'])

    def test_logical_quota_set(self):
        try:
            self.admin.assert_icommand(['iadmin', 'set_logical_quota', self.quota_user.session_collection, 'bytes', '10000'])
            _, out, _ = self.admin.assert_icommand(['iadmin', 'list_logical_quotas'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '10000', '<unset>', '-10000', '<unenforced>')) in out)
            self.admin.assert_icommand(['iadmin', 'set_logical_quota', self.quota_user.session_collection, 'objects', '10000'])
            _, out, _ = self.admin.assert_icommand(['iadmin', 'list_logical_quotas'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '10000', '10000', '-10000', '0')) in out)
        finally:
            self.admin.assert_icommand(['iadmin', 'set_logical_quota', self.quota_user.session_collection, '0', '0'])

    def test_logical_quota_enforcement(self):
        file_name = 'test_logical_quota_enforcement'
        lib.make_file(os.path.join(self.quota_user.local_session_dir, file_name), 4096, contents='arbitrary')

        try:
            self.admin.assert_icommand(['iadmin', 'set_grid_configuration', 'logical_quotas', 'enabled', '1'])

            self.admin.assert_icommand(['iadmin', 'set_logical_quota', self.quota_user.session_collection, 'bytes', '10000'])
            _, out, _ = self.admin.assert_icommand(['iadmin', 'list_logical_quotas'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '10000', '<unset>', '-10000', '<unenforced>')) in out)

            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), f'{self.quota_user.session_collection}/{file_name}'])
            self.admin.assert_icommand(['iadmin', 'calculate_logical_usage'])

            _, out, _ = self.admin.assert_icommand(['iadmin', 'list_logical_quotas'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '10000', '<unset>', str(4096 - 10000), '<unenforced>')) in out)

            self.admin.assert_icommand(['iadmin', 'set_logical_quota', self.quota_user.session_collection, 'objects', '50'])
            self.admin.assert_icommand(['iadmin', 'calculate_logical_usage'])

            _, out, _ = self.admin.assert_icommand(['iadmin', 'list_logical_quotas'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '10000', '50', str(4096 - 10000), str(1 - 50))) in out)

            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), f'{self.quota_user.session_collection}/{file_name}_2'])
            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), f'{self.quota_user.session_collection}/{file_name}_3'])
            self.admin.assert_icommand(['iadmin', 'calculate_logical_usage'])

            _, out, _ = self.admin.assert_icommand(['iadmin', 'list_logical_quotas'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '10000', '50', str(4096*3 - 10000), str(3 - 50))) in out)

            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), f'{file_name}_4'],'STDERR_SINGLELINE', 'LOGICAL_QUOTA_EXCEEDED')
            self.quota_user.assert_icommand(['itouch', f'{file_name}_4'], 'STDOUT_SINGLELINE', 'CAT_NOT_A_DATAOBJ_AND_NOT_A_COLLECTION', desired_rc=1)
            self.quota_user.assert_icommand(['istream', 'write', f'{file_name}_4'], 'STDERR', ['Error: Cannot open data object'], input='some data')

            self.admin.assert_icommand(['iadmin', 'set_logical_quota', self.quota_user.session_collection, 'bytes', '100000'])
            self.admin.assert_icommand(['iadmin', 'set_logical_quota', self.quota_user.session_collection, 'objects', '2'])
            self.admin.assert_icommand(['iadmin', 'calculate_logical_usage'])

            _, out, _ = self.admin.assert_icommand(['iadmin', 'list_logical_quotas'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '100000', '2', str(4096*3 - 100000), str(3 - 2))) in out)
            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), f'{file_name}_4'],'STDERR_SINGLELINE', 'LOGICAL_QUOTA_EXCEEDED')
            self.quota_user.assert_icommand(['itouch', f'{file_name}_4'], 'STDOUT_SINGLELINE', 'CAT_NOT_A_DATAOBJ_AND_NOT_A_COLLECTION', desired_rc=1)
            self.quota_user.assert_icommand(['istream', 'write', f'{file_name}_4'], 'STDERR', ['Error: Cannot open data object'], input='some data')

            self.admin.assert_icommand(['iadmin', 'set_logical_quota', self.quota_user.session_collection, 'objects', '10'])
            self.admin.assert_icommand(['iadmin', 'calculate_logical_usage'])
            _, out, _ = self.admin.assert_icommand(['iadmin', 'list_logical_quotas'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '100000', '10', str(4096*3 - 100000), str(3 - 10))) in out)

            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), f'{file_name}_4'])

        finally:
            self.admin.assert_icommand(['iadmin', 'set_logical_quota', self.quota_user.session_collection, '0', '0'])
            self.admin.assert_icommand(['iadmin', 'set_grid_configuration', 'logical_quotas', 'enabled', '0'])


    def test_nested_logical_quota_enforcement(self):
        nesting_depth = 5
        self.quota_user.assert_icommand(['imkdir', f'{self.quota_user.session_collection}/test_nested_logical_quota_coll'])
        def subcoll_path_generator():
            path = 'test_nested_logical_quota_coll'
            count = 0
            while True:
                path = f'{path}/{count}'
                count = count + 1
                yield path

        file_name = 'test_nested_logical_quota_enforcement'
        # Violate quota in one shot
        lib.make_file(os.path.join(self.quota_user.local_session_dir, file_name), 10001, contents='arbitrary')

        try:
            self.admin.assert_icommand(['iadmin', 'set_grid_configuration', 'logical_quotas', 'enabled', '1'])

            path_gen = subcoll_path_generator()
            innermost_subcoll = ''

            max_bytes = 10000
            # Set nested quotas
            # Make outermost quota most restrictive
            for i in range(0, nesting_depth):
                subcoll = next(path_gen)
                self.quota_user.assert_icommand(['imkdir', f'{self.quota_user.session_collection}/{subcoll}'])
                self.admin.assert_icommand(['iadmin', 'set_logical_quota', f'{self.quota_user.session_collection}/{subcoll}', 'bytes', str(max_bytes) ])
                _, out, _ = self.admin.assert_icommand(['iadmin', 'list_logical_quotas'], 'STDOUT_SINGLELINE', f'{self.quota_user.session_collection}/{subcoll}')
                self.assertTrue((self.llq_output_template % (f'{self.quota_user.session_collection}/{subcoll}', str(max_bytes), '<unset>', str(-max_bytes), '<unenforced>')) in out)
                max_bytes = max_bytes * 2
                innermost_subcoll = subcoll

            path_gen = subcoll_path_generator()
            subcoll = next(path_gen)

            # Put to innermost subcoll
            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), f'{self.quota_user.session_collection}/{innermost_subcoll}/{file_name}'])
            self.admin.assert_icommand(['iadmin', 'calculate_logical_usage'])

            _, out, _ = self.admin.assert_icommand(['iadmin', 'list_logical_quotas'], 'STDOUT_SINGLELINE', f'{self.quota_user.session_collection}/{subcoll}')
            self.assertTrue((self.llq_output_template % (f'{self.quota_user.session_collection}/{subcoll}', '10000', '<unset>', str(10001 - 10000), '<unenforced>')) in out)

            # Putting to innermost subcoll should violate the outermost subcoll's quota
            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), f'{self.quota_user.session_collection}/{innermost_subcoll}/{file_name}_2'],'STDERR_SINGLELINE', 'LOGICAL_QUOTA_EXCEEDED')

            # Lift byte restriction on outermost subcoll
            # Add 2-object quota on outermost
            self.admin.assert_icommand(['iadmin', 'set_logical_quota', f'{self.quota_user.session_collection}/{subcoll}', '0', '2'])
            self.admin.assert_icommand(['iadmin', 'calculate_logical_usage'])

            # Should succeed now
            # Should violate byte quota one level deep after recalculation
            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), f'{self.quota_user.session_collection}/{innermost_subcoll}/{file_name}_2'])
            self.admin.assert_icommand(['iadmin', 'calculate_logical_usage'])

            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), f'{self.quota_user.session_collection}/{innermost_subcoll}/{file_name}_3'],'STDERR_SINGLELINE', 'LOGICAL_QUOTA_EXCEEDED')

            # Lift byte restriction on one-level-deep subcoll
            self.admin.assert_icommand(['iadmin', 'set_logical_quota', f'{self.quota_user.session_collection}/{next(path_gen)}', 'bytes', '0'])
            self.admin.assert_icommand(['iadmin', 'calculate_logical_usage'])

            # Should succeed now
            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), f'{self.quota_user.session_collection}/{innermost_subcoll}/{file_name}_3'])
            self.admin.assert_icommand(['iadmin', 'calculate_logical_usage'])

            # Should now violate object quota on outermost collection
            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), f'{self.quota_user.session_collection}/{innermost_subcoll}/{file_name}_4'],'STDERR_SINGLELINE', 'LOGICAL_QUOTA_EXCEEDED')

        finally:
            self.admin.assert_icommand(['iadmin', 'set_logical_quota', self.quota_user.session_collection, '0', '0'])
            self.admin.assert_icommand(['iadmin', 'set_grid_configuration', 'logical_quotas', 'enabled', '0'])

    @unittest.skipIf(IrodsConfig().default_rule_engine_plugin == 'irods_rule_engine_plugin-python', 'Only implemented with NREP')
    def test_logical_quota_msi(self):
        file_name = 'test_logical_quota_msi'
        lib.make_file(os.path.join(self.quota_user.local_session_dir, file_name), 4096, contents='arbitrary')
        self.admin.assert_icommand(['iadmin', 'set_grid_configuration', 'logical_quotas', 'enabled', '1'])

        try:
            self.admin.assert_icommand(['irule', f'msi_set_logical_quota("{self.quota_user.session_collection}", "bytes", "4000")', 'null', 'null'])
            _, out, _ = self.admin.assert_icommand(['iadmin', 'list_logical_quotas'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '4000', '<unset>', '-4000', '<unenforced>')) in out)

            # Check that the alternate invocation works too
            self.admin.assert_icommand(['irule', f'msi_set_logical_quota("{self.quota_user.session_collection}", "3000", "10")', 'null', 'null'])
            _, out, _ = self.admin.assert_icommand(['iadmin', 'list_logical_quotas'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            # Bytes over still at -4000 since no recalculation done yet
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '3000', '10', '-4000', '0')) in out)
            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), f'{self.quota_user.session_collection}/{file_name}'])
            self.admin.assert_icommand(['irule', f'msi_calc_logical_usage', 'null', 'null'])
            _, out, _ = self.admin.assert_icommand(['iadmin', 'list_logical_quotas'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '3000', '10', str(4096 - 3000), str(1 - 10))) in out)
            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), f'{file_name}_2'],'STDERR_SINGLELINE', 'LOGICAL_QUOTA_EXCEEDED')
        finally:
            self.admin.assert_icommand(['iadmin', 'set_logical_quota', self.quota_user.session_collection, '0', '0'])
            self.admin.assert_icommand(['iadmin', 'set_grid_configuration', 'logical_quotas', 'enabled', '0'])

    def test_logical_quota_grid_configuration_settings(self):
        file_name = 'test_logical_quota_grid'

        lib.make_file(os.path.join(self.quota_user.local_session_dir, file_name), 4096, contents='arbitrary')

        try:
            # No enforcement when disabled.
            self.admin.assert_icommand(['iadmin', 'set_grid_configuration', 'logical_quotas', 'enabled', '0'])

            # Byte quota is low enough to be violated in one upload.
            self.admin.assert_icommand(['iadmin', 'set_logical_quota', self.quota_user.session_collection, 'bytes', '4000'])
            _, out, _ = self.admin.assert_icommand(['iadmin', 'list_logical_quotas'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '4000', '<unset>', '-4000', '<unenforced>')) in out)

            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), f'{self.quota_user.session_collection}/{file_name}'])
            self.admin.assert_icommand(['iadmin', 'calculate_logical_usage'])

            # Check to ensure numbers are correct.
            _, out, _ = self.admin.assert_icommand(['iadmin', 'list_logical_quotas'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '4000', '<unset>', str(4096-4000), '<unenforced>')) in out)

            # No failure, because no enforcement.
            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), f'{self.quota_user.session_collection}/{file_name}_2'])
            self.admin.assert_icommand(['iadmin', 'calculate_logical_usage'])

            # No enforcement when set to strange value.
            self.admin.assert_icommand(['iadmin', 'set_grid_configuration', 'logical_quotas', 'enabled', 'potato'])

            # No failure, because no enforcement.
            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), f'{self.quota_user.session_collection}/{file_name}_3'])
            self.admin.assert_icommand(['iadmin', 'calculate_logical_usage'])

            # Enforcement when explicitly enabled.
            self.admin.assert_icommand(['iadmin', 'set_grid_configuration', 'logical_quotas', 'enabled', '1'])

            # Should fail due to enabled enforcement.
            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), f'{file_name}_4'],'STDERR_SINGLELINE', 'LOGICAL_QUOTA_EXCEEDED')

            # No enforcement when value is completely missing.
            self.admin.assert_icommand(['iadmin', 'asq', "DELETE FROM R_GRID_CONFIGURATION WHERE namespace='logical_quotas' AND option_name='enabled'", 'rmlqgridopt'])
            self.admin.assert_icommand(['iquest', '--sql', 'rmlqgridopt'], 'STDOUT_SINGLELINE', 'CAT_NO_ROWS_FOUND', desired_rc=1)

            # No failure, because no enforcement.
            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), f'{self.quota_user.session_collection}/{file_name}_4'])
        finally:
            # Run and don't assert these-- if failure happens midway
            # we can't guarantee what state the catalog is in.
            self.admin.run_icommand(['iadmin', 'asq', "INSERT INTO R_GRID_CONFIGURATION VALUES('logical_quotas','enabled','0')", 'addlqgridopt'])
            self.admin.run_icommand(['iquest', '--sql', 'addlqgridopt'])
            self.admin.run_icommand(['iadmin', 'rsq', 'rmlqgridopt'])
            self.admin.run_icommand(['iadmin', 'rsq', 'addlqgridopt'])

            self.admin.assert_icommand(['iadmin', 'set_logical_quota', self.quota_user.session_collection, '0', '0'])

            # This will fail if the previous "restore the catalog" cleanup failed.
            # We don't want to progress with the catalog in a bad state.
            self.admin.assert_icommand(['iadmin', 'set_grid_configuration', 'logical_quotas', 'enabled', '0'])

    def test_logical_quota_with_replicas(self):
        dataobj_name = 'test_logical_quota_replica'
        small_file_name = 'test_logical_quota_smallfile'
        big_file_name = 'test_logical_quota_bigfile'
        resc_name = 'ufs0_logical_quota_replica'
        other_resc_name = 'ufs1_logical_quota_replica'

        lib.make_file(os.path.join(self.quota_user.local_session_dir, small_file_name), 2048, contents='arbitrary')
        lib.make_file(os.path.join(self.quota_user.local_session_dir, big_file_name), 4096, contents='arbitrary')
        lib.create_ufs_resource(self.admin, resc_name)
        lib.create_ufs_resource(self.admin, other_resc_name)

        try:
            # Enable enforcement.
            self.admin.assert_icommand(['iadmin', 'set_grid_configuration', 'logical_quotas', 'enabled', '1'])

            # Set to 10000 bytes and 5 objects
            self.admin.assert_icommand(['iadmin', 'set_logical_quota', self.quota_user.session_collection, '10000', '5'])
            _, out, _ = self.admin.assert_icommand(['iadmin', 'list_logical_quotas'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '10000', '5', '-10000', '-5')) in out)

            # Replica 0 is now large file.
            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, big_file_name), f'{self.quota_user.session_collection}/{dataobj_name}'])
            # Replica 1 is now large file.
            self.quota_user.assert_icommand(['irepl', '-R', resc_name, f'{self.quota_user.session_collection}/{dataobj_name}'])

            # Replica 0 is now small file and good.
            # Replica 1 is stale.
            self.quota_user.assert_icommand(['iput', '-f', os.path.join(self.quota_user.local_session_dir, small_file_name), f'{self.quota_user.session_collection}/{dataobj_name}'])
            self.admin.assert_icommand(['iadmin', 'calculate_logical_usage'])

            # Check: Replica 1 is stale and replica 0 is good,
            # so logical quotas should calculate using replica 0.
            # Replica 0 has smaller size, so verify smaller size is counted against
            # the byte limit. Additionally, verify the data object
            # counts against the object limit only once.
            _, out, _ = self.admin.assert_icommand(['iadmin', 'list_logical_quotas'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '10000', '5', str(2048-10000), str(1-5))) in out)

            # Replica 0 is now small file.
            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, small_file_name), f'{self.quota_user.session_collection}/{dataobj_name}_1'])

            # Replica 1 is now small file.
            self.quota_user.assert_icommand(['irepl', '-R', resc_name, f'{self.quota_user.session_collection}/{dataobj_name}_1'])

            # Replica 2 is now small file.
            self.quota_user.assert_icommand(['irepl', '-R', other_resc_name, f'{self.quota_user.session_collection}/{dataobj_name}_1'])

            # Replica 0 is now big file.
            # Replicas 1, 2 are now stale.
            self.quota_user.assert_icommand(['iput', '-f', os.path.join(self.quota_user.local_session_dir, big_file_name), f'{self.quota_user.session_collection}/{dataobj_name}_1'])

            # Force-stale replica 0.
            self.admin.assert_icommand(['iadmin', 'modrepl', 'logical_path', f'{self.quota_user.session_collection}/{dataobj_name}_1', 'replica_number', '0', 'DATA_REPL_STATUS', '0'])
            self.admin.assert_icommand(['iadmin', 'calculate_logical_usage'])

            # Check: All replicas are stale, so logical quotas
            # should calculate using the largest replica (i.e. 0)
            # for byte_limit calculations.
            # Object should still count only once against object_limit.
            _, out, _ = self.admin.assert_icommand(['iadmin', 'list_logical_quotas'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '10000', '5', str(2048+4096-10000), str(2-5))) in out)

            # Replica 0 is big file.
            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, big_file_name), f'{self.quota_user.session_collection}/{dataobj_name}_2'])
            # Set to 3 (read-locked).
            # Read-locked should be ignored for byte calculations.
            self.admin.assert_icommand(['iadmin', 'modrepl', 'logical_path', f'{self.quota_user.session_collection}/{dataobj_name}_2', 'replica_number', '0', 'DATA_REPL_STATUS', '3'])
            self.admin.assert_icommand(['iadmin', 'calculate_logical_usage'])

            # Read-locked should not count against byte_limit.
            # It will count against object_limit, however.
            _, out, _ = self.admin.assert_icommand(['iadmin', 'list_logical_quotas'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '10000', '5', str(2048+4096-10000), str(3-5))) in out)

            # Replica 0 is big file.
            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, big_file_name), f'{self.quota_user.session_collection}/{dataobj_name}_3'])
            # Set to 4 (write-locked).
            # Write-locked should be counted for byte calculations.
            self.admin.assert_icommand(['iadmin', 'modrepl', 'logical_path', f'{self.quota_user.session_collection}/{dataobj_name}_3', 'replica_number', '0', 'DATA_REPL_STATUS', '4'])
            self.admin.assert_icommand(['iadmin', 'calculate_logical_usage'])

            # Write-locked should count against byte_limit.
            # It will also count against object_limit.
            _, out, _ = self.admin.assert_icommand(['iadmin', 'list_logical_quotas'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '10000', '5', str(2048+4096+4096-10000), str(4-5))) in out)

            # Byte limit is now in violation, so should fail.
            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, big_file_name), f'{self.quota_user.session_collection}/{dataobj_name}_4'],'STDERR_SINGLELINE', 'LOGICAL_QUOTA_EXCEEDED')

            # Return to the "3 stale replicas" case.
            # Set a small-file replica to "good."
            self.admin.assert_icommand(['iadmin', 'modrepl', 'logical_path', f'{self.quota_user.session_collection}/{dataobj_name}_1', 'replica_number', '1', 'DATA_REPL_STATUS', '1'])
            self.admin.assert_icommand(['iadmin', 'calculate_logical_usage'])

            # Since the "3 stale replicas" now has a good replica
            # in the form of a small file, that size now counts
            # instead of the large file size.
            _, out, _ = self.admin.assert_icommand(['iadmin', 'list_logical_quotas'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '10000', '5', str(2048+2048+4096-10000), str(4-5))) in out)

            # Replica 0 is big file.
            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, big_file_name), f'{self.quota_user.session_collection}/{dataobj_name}_4'])
            # Set to 2 (intermediate).
            # Intermediate should be ignored for byte calculations,
            # and counted for object calculations.
            self.admin.assert_icommand(['iadmin', 'modrepl', 'logical_path', f'{self.quota_user.session_collection}/{dataobj_name}_4', 'replica_number', '0', 'DATA_REPL_STATUS', '2'])
            self.admin.assert_icommand(['iadmin', 'calculate_logical_usage'])

            _, out, _ = self.admin.assert_icommand(['iadmin', 'list_logical_quotas'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '10000', '5', str(2048+2048+4096-10000), str(5-5))) in out)

            # Replica 0 is big file.
            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, big_file_name), f'{self.quota_user.session_collection}/{dataobj_name}_6'])

            # Set to 7 (???).
            # Unused/unknown values should be ignored for byte calculations,
            # and counted for object calculations.
            self.admin.assert_icommand(['iadmin', 'modrepl', 'logical_path', f'{self.quota_user.session_collection}/{dataobj_name}_6', 'replica_number', '0', 'DATA_REPL_STATUS', '7'])
            self.admin.assert_icommand(['iadmin', 'calculate_logical_usage'])
            _, out, _ = self.admin.assert_icommand(['iadmin', 'list_logical_quotas'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '10000', '5', str(2048+2048+4096-10000), str(6-5))) in out)

            # Object limit is violated, so should fail.
            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, big_file_name), f'{self.quota_user.session_collection}/{dataobj_name}_7'],'STDERR_SINGLELINE', 'LOGICAL_QUOTA_EXCEEDED')


        finally:
            # Reset all the objects set to locked/intermediate;
            # deletes will fail otherwise.
            self.admin.assert_icommand(['iadmin', 'modrepl', 'logical_path', f'{self.quota_user.session_collection}/{dataobj_name}_4', 'replica_number', '0', 'DATA_REPL_STATUS', '1'])
            self.admin.assert_icommand(['iadmin', 'modrepl', 'logical_path', f'{self.quota_user.session_collection}/{dataobj_name}_3', 'replica_number', '0', 'DATA_REPL_STATUS', '1'])
            self.admin.assert_icommand(['iadmin', 'modrepl', 'logical_path', f'{self.quota_user.session_collection}/{dataobj_name}_2', 'replica_number', '0', 'DATA_REPL_STATUS', '1'])

            self.admin.assert_icommand(['iadmin', 'set_logical_quota', self.quota_user.session_collection, '0', '0'])
            self.admin.assert_icommand(['iadmin', 'set_grid_configuration', 'logical_quotas', 'enabled', '0'])
