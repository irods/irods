from __future__ import print_function
import os
import sys
import json
import psutil

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import replica_status_test
from . import command
from . import session
from .. import lib
from .. import test
from .. import paths
from ..configuration import IrodsConfig
from ..test.command import assert_command

class Test_Irepl(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):

    def setUp(self):
        super(Test_Irepl, self).setUp()
        self.admin = self.admin_sessions[0]
        self.user = self.user_sessions[0]

    def tearDown(self):
        super(Test_Irepl, self).tearDown()

    def do_test_for_issue_5592(self, trigger_checksum_mismatch_error):
        def get_physical_path(data_object, replica_number):
            gql = "select DATA_PATH where COLL_NAME = '{0}' and DATA_NAME = '{1}' and DATA_REPL_NUM = '{2}'"
            path, err, ec = self.admin.run_icommand(['iquest', '%s', gql.format(self.admin.session_collection, data_object, replica_number)])
            self.assertEqual(ec, 0)
            self.assertEqual(len(err), 0)
            self.assertGreater(len(path.strip()), 0)
            return path.strip()

        def get_replica_status(data_object, replica_number):
            gql = "select DATA_REPL_STATUS where COLL_NAME = '{0}' and DATA_NAME = '{1}' and DATA_REPL_NUM = '{2}'"
            status, err, ec = self.admin.run_icommand(['iquest', '%s', gql.format(self.admin.session_collection, data_object, replica_number)])
            self.assertEqual(ec, 0)
            self.assertEqual(len(err), 0)
            self.assertGreater(len(status.strip()), 0)
            return status.strip()

        def get_checksum(data_object, replica_number, expected_length_of_checksum=49):
            gql = "select DATA_CHECKSUM where COLL_NAME = '{0}' and DATA_NAME = '{1}' and DATA_REPL_NUM = '{2}'"
            checksum, err, ec = self.admin.run_icommand(['iquest', '%s', gql.format(self.admin.session_collection, data_object, replica_number)])
            self.assertEqual(ec, 0)
            self.assertEqual(len(err), 0)
            self.assertEqual(len(checksum.strip()), expected_length_of_checksum)
            return checksum.strip()

        # Define our resource names for the "finally" block.
        repl_resc = 'issue_5592_repl_resc'
        ufs_resources = ['issue_5592_ufs1', 'issue_5592_ufs2', 'issue_5592_ufs3']

        # Define the name of the data object for the "finally" block.
        data_object = 'foo'

        try:
            # Create our resource hierarchy.
            lib.create_replication_resource(self.admin, repl_resc)

            for resc_name in ufs_resources:
                lib.create_ufs_resource(self.admin, resc_name)
                self.admin.assert_icommand(['iadmin', 'addchildtoresc', repl_resc, resc_name])

            # Create a data object (this should result in three replicas).
            self.admin.assert_icommand(['istream', 'write', '-R', repl_resc, data_object], input='bar')
            self.admin.assert_icommand(['ils', '-l', data_object], 'STDOUT', [
                ' {};{}            3 '.format(repl_resc, ufs_resources[0]),
                ' {};{}            3 '.format(repl_resc, ufs_resources[1]),
                ' {};{}            3 '.format(repl_resc, ufs_resources[2])
            ])

            # Checksum all replicas.
            self.admin.assert_icommand(['ichksum', '-a', data_object], 'STDOUT', ['sha2:'])

            # Capture the current size of the log file. This will be used as the starting
            # point for searching the log file for a particular string.
            log_offset = lib.get_file_size_by_path(paths.server_log_path())

            if trigger_checksum_mismatch_error:
                # Corrupt the replica by changing the first character inside of it.
                # This will help trigger a checksum mismatch error.
                physical_path = get_physical_path(data_object, 0)
                with open(physical_path, 'w') as f:
                    f.write('far')

                # Trigger the checksum mismatch error.
                self.admin.assert_icommand(['irepl', '-R', 'demoResc', data_object], 'STDERR', ['status = -314000 USER_CHKSUM_MISMATCH'])

                # Show that the replicas under the replication resource are still marked as "good"
                # and that the newest replica is marked as "stale" due to the error.
                self.assertEqual(get_replica_status(data_object, '0'), '1')
                self.assertEqual(get_replica_status(data_object, '1'), '1')
                self.assertEqual(get_replica_status(data_object, '2'), '1')
                self.assertEqual(get_replica_status(data_object, '3'), '0')

                # Show that the newest replica has a different checksum than the first replica.
                self.assertNotEqual(get_checksum(data_object, '0'), get_checksum(data_object, '3'))

                # Show that the log file contains the expected error information.
                msg = 'error finalizing replica; [USER_CHKSUM_MISMATCH:'
                lib.delayAssert(lambda: lib.log_message_occurrences_greater_than_count(msg=msg, count=0, start_index=log_offset))
            else:
                # Append a character to the end of the replica.
                physical_path = get_physical_path(data_object, 0)
                with open(physical_path, 'w') as f:
                    f.write('bars')

                self.admin.assert_icommand(['irepl', '-R', 'demoResc', data_object])

                # Show that all replicas are marked as "good".
                self.assertEqual(get_replica_status(data_object, '0'), '1')
                self.assertEqual(get_replica_status(data_object, '1'), '1')
                self.assertEqual(get_replica_status(data_object, '2'), '1')
                self.assertEqual(get_replica_status(data_object, '3'), '1')

                # Show that the newest replica's checksum matches the first replica's checksum.
                self.assertEqual(get_checksum(data_object, '0'), get_checksum(data_object, '3'))

                # Show that the log file does not contain an error information.
                msg = 'error finalizing replica; [USER_CHKSUM_MISMATCH:'
                lib.delayAssert(lambda: lib.log_message_occurrences_equals_count(msg=msg, count=0, start_index=log_offset))
        finally:
            self.admin.run_icommand(['irm', '-f', data_object])

            for resc_name in ufs_resources:
                self.admin.run_icommand(['iadmin', 'rmchildfromresc', repl_resc, resc_name])
                self.admin.run_icommand(['iadmin', 'rmresc', resc_name])

            self.admin.run_icommand(['iadmin', 'rmresc', repl_resc])

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for Topology Tests on Resource')
    def test_irepl_reports_checksum_mismatch_error_when_bytes_within_the_data_size_are_overwritten__issue_5592(self):
        # Passing "True" causes the test to trigger a checksum mismatch by overwriting a character
        # within a replica. The overwrite will not cause a mismatch in the size. Only the data within
        # the replica will change.
        self.do_test_for_issue_5592(trigger_checksum_mismatch_error=True)

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for Topology Tests on Resource')
    def test_irepl_does_not_return_a_checksum_mismatch_error_after_appending_data_to_a_replica_out_of_band__issue_5592(self):
        # Passing "False" causes the test to append a character to a replica. This will not trigger
        # a checksum mismatch error because the API honors the data size in the catalog when calculating
        # checksums and replicating replicas.
        self.do_test_for_issue_5592(trigger_checksum_mismatch_error=False)


    def test_irepl_a_updates_stale_zero_byte_replicas__issue_6285(self):
        # The data object needs to have a really weird name and exist inside a really weirdly named collection.
        collection = os.path.join(
            self.user.session_collection,
            'sharePoint',
            'mgb',
            '{collectionName=Maize Gene Bank Regeneration}{plantType=Maize-Ear}{imageType=earImage}',
            'algorithmResults',
            'resultObjects')
        logical_path = os.path.join(
            collection,
            '{algorithm=earImage}{computeStatus=complete}'
            '{input=['
            '{accessionNumber=CIMMYTMA~217}'
            '{accessionName=MORE~17}'
            '{fieldData=PV17A}'
            '{experimentId=1903}'
            '{plotId=64}'
            '{dataType=earData}]}'
            '{inputHash=d7b8d47f13839131e805a59091f7c4f12dc553d8}.mat')

        # Construct two hierarchies consisting of a passthru to a unixfilesystem resource.
        roots = ['root_a', 'root_b']
        leaves = ['leaf_a', 'leaf_b']
        self.assertEqual(len(roots), len(leaves))

        lib.create_passthru_resource(self.admin, roots[0])
        lib.create_passthru_resource(self.admin, roots[1])
        lib.create_ufs_resource(self.admin, leaves[0], test.settings.HOSTNAME_2)
        lib.create_ufs_resource(self.admin, leaves[1], test.settings.HOSTNAME_3)
        lib.add_child_resource(self.admin, roots[0], leaves[0])
        lib.add_child_resource(self.admin, roots[1], leaves[1])

        try:
            # itouch a data object in a hierarchy and replicate it such that there are 2 zero-length replicas.
            self.user.assert_icommand(['imkdir', '-p', collection])
            self.user.assert_icommand(['itouch', '-R', leaves[0], logical_path])
            lib.replica_exists_on_resource(self.user, logical_path, leaves[0])
            self.user.assert_icommand(['irepl', '-R', roots[1], logical_path])
            lib.replica_exists_on_resource(self.user, logical_path, leaves[1])

            # Create a large-ish file and overwrite replica 0, which will make replica 1 stale.
            filesize = 24111062
            localfile = os.path.join(self.user.local_session_dir, 'biggishfile.bin')
            lib.make_file(localfile, filesize, contents='arbitrary')
            self.user.assert_icommand(['iput', '-n', '0', '-f', localfile, logical_path])
            self.assertEqual(1, int(lib.get_replica_status(self.user, os.path.basename(logical_path), 0)))
            self.assertEqual(0, int(lib.get_replica_status(self.user, os.path.basename(logical_path), 1)))

            # Run irepl -a -M on that data object and ensure that replica 1 is updated appropriately.
            self.admin.assert_icommand(['irepl', '-a', '-M', logical_path])
            self.assertEqual(1, int(lib.get_replica_status(self.user, os.path.basename(logical_path), 0)))
            self.assertEqual(1, int(lib.get_replica_status(self.user, os.path.basename(logical_path), 1)))

        finally:
            self.user.assert_icommand(['irm', '-f', logical_path])
            lib.remove_child_resource(self.admin, roots[0], leaves[0])
            lib.remove_child_resource(self.admin, roots[1], leaves[1])
            lib.remove_resource(self.admin, leaves[0])
            lib.remove_resource(self.admin, leaves[1])
            lib.remove_resource(self.admin, roots[0])
            lib.remove_resource(self.admin, roots[1])

    def test_server_honors_server_config_option_checksum_read_buffer_size_in_bytes__issue_7947(self):

        object_name = 'test_iput_with_updated_checksum_read_buffer_size_in_bytes__issue_7947'
        server_config_filename = paths.server_config_path()

        # load server_config.json to inject a new rule base
        with open(server_config_filename) as f:
            svr_cfg = json.load(f)

        ufs_resources = ['issue_7947_ufs1', 'issue_7947_ufs2']

        try:
            # create two resources
            for resc_name in ufs_resources:
                lib.create_ufs_resource(self.admin, resc_name)

            # create a 100 MiB file
            file_size = 100*1024*1024
            local_file = os.path.join(self.admin.local_session_dir, object_name)
            lib.make_file(local_file, file_size)
            file_checksum = lib.file_digest(local_file, 'sha256', encoding='base64')

            with lib.file_backed_up(server_config_filename):

                for buffer_size in [10*1024*1024, file_size - 1, file_size, file_size + 1,  2**31-1]:

                    # make sure we have plenty of available virtual memory to run this iteration
                    if psutil.virtual_memory().available < 2 * buffer_size:
                        continue

                    # update the checksum_read_buffer_size_in_bytes
                    svr_cfg['advanced_settings']['checksum_read_buffer_size_in_bytes'] = buffer_size;
                    new_server_config = json.dumps(svr_cfg, sort_keys=True, indent=4, separators=(',', ': '))

                    # repave the existing server_config.json
                    with open(server_config_filename, 'w') as f:
                        f.write(new_server_config)

                    # put the file with the new server configuration and compare checksum
                    self.admin.assert_icommand(['iput', '-R', ufs_resources[0], '-k', local_file, object_name])
                    self.assertEqual(lib.get_replica_checksum(self.admin, object_name, 0), f'sha2:{file_checksum}')

                    # replicate the file and compare the checksum on the replica
                    self.admin.assert_icommand(['irepl', '-R', ufs_resources[1], object_name])
                    self.assertEqual(lib.get_replica_checksum(self.admin, object_name, 1), f'sha2:{file_checksum}')

                    # cleanup for this loop
                    self.admin.assert_icommand(['irm', '-f', object_name])

                # test a buffer size of 1 with a much smaller file
                file_size = 1024
                local_file = os.path.join(self.admin.local_session_dir, object_name)
                lib.make_file(local_file, file_size)
                file_checksum = lib.file_digest(local_file, 'sha256', encoding='base64')

                buffer_size = 1
                svr_cfg['advanced_settings']['checksum_read_buffer_size_in_bytes'] = buffer_size;
                new_server_config = json.dumps(svr_cfg, sort_keys=True, indent=4, separators=(',', ': '))

                # repave the existing server_config.json
                with open(server_config_filename, 'w') as f:
                    f.write(new_server_config)

                # put the file with the new server configuration and compare checksum
                self.admin.assert_icommand(['iput', '-R', ufs_resources[0], '-k', local_file, object_name])
                self.assertEqual(lib.get_replica_checksum(self.admin, object_name, 0), f'sha2:{file_checksum}')

                # replicate the file and compare the checksum on the replica
                self.admin.assert_icommand(['irepl', '-R', ufs_resources[1], object_name])
                self.assertEqual(lib.get_replica_checksum(self.admin, object_name, 1), f'sha2:{file_checksum}')

        finally:
            os.unlink(local_file)
            self.admin.assert_icommand(['irm', '-f', object_name])
            for resc_name in ufs_resources:
                self.admin.run_icommand(['iadmin', 'rmresc', resc_name])

    def test_server_rejects_invalid_server_config_options_for_checksum_read_buffer_size_in_bytes__issue_7947(self):
        object_name = 'test_server_rejects_invalid_server_config_options_for_checksum_read_buffer_size_in_bytes__issue_7947'
        server_config_filename = paths.server_config_path()

        ufs_resources = ['issue_7947_ufs1', 'issue_7947_ufs2']

        # load server_config.json to inject a new rule base
        with open(server_config_filename) as f:
            svr_cfg = json.load(f)

        try:
            # create two resources
            for resc_name in ufs_resources:
                lib.create_ufs_resource(self.admin, resc_name)

            # create a small file
            file_size = 10
            local_file = os.path.join(self.admin.local_session_dir, object_name)
            lib.make_file(local_file, file_size)

            # put the file to first the resource as a source for replication
            self.admin.assert_icommand(['iput', '-R', ufs_resources[0], '-k', local_file, object_name])

            with lib.file_backed_up(server_config_filename):

                for buffer_size in [-2**63-1, -1, 0, 2**63]:  # first and last are outside range of 64 bit signed integers

                    svr_cfg['advanced_settings']['checksum_read_buffer_size_in_bytes'] = buffer_size;
                    new_server_config = json.dumps(svr_cfg, sort_keys=True, indent=4, separators=(',', ': '))

                    # repave the existing server_config.json
                    with open(server_config_filename, 'w') as f:
                        f.write(new_server_config)

                    # irepl the file and verify we get an error due to invalid configuration
                    self.admin.assert_icommand_fail(['irepl', '-R', ufs_resources[1], object_name], 'STDOUT_SINGLELINE', 'CONFIGURATION_ERROR')

        finally:
            os.unlink(local_file)
            self.admin.assert_icommand(['irm', '-f', object_name])
            for resc_name in ufs_resources:
                self.admin.run_icommand(['iadmin', 'rmresc', resc_name])

class test_irepl_with_special_resource_configurations(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):
    # In this suite:
    #   - rodsadmin otherrods
    #   - tests in this suite are responsible for their own storage resources
    def setUp(self):
        super(test_irepl_with_special_resource_configurations, self).setUp()
        self.admin = self.admin_sessions[0]

    def tearDown(self):
        super(test_irepl_with_special_resource_configurations, self).tearDown()

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_irepl_with_U_and_R_options__issue_3659(self):
        filename = 'test_file_issue_3659.txt'
        lib.make_file(filename, 1024)

        hostname = IrodsConfig().client_environment['irods_host']
        replicas = 6

        # Create resources [resc_{0..5}].
        for i in range(replicas):
            vault = os.path.join(self.admin.local_session_dir, 'issue_3659_storage_vault_' + str(i))
            path = hostname + ':' + vault
            self.admin.assert_icommand('iadmin mkresc resc_{0} unixfilesystem {1}'.format(i, path), 'STDOUT', 'unixfilesystem')

        try:
            self.admin.assert_icommand('iput -R resc_0 {0}'.format(filename))

            # Replicate the file across the remaining resources.
            for i in range(1, replicas):
                self.admin.assert_icommand('irepl -R resc_{0} {1}'.format(i, filename))

            # Make all replicas stale except one.
            self.admin.assert_icommand('iput -fR resc_0 {0}'.format(filename))

            # Verify that all replicas are stale except the one under [resc_0].
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_0', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_1', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_2', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_3', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_4', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_5', '&'])

            # Main Tests.

            # Test 1: Update the replica under resc_1.
            self.admin.assert_icommand('irepl -R resc_1 {0}'.format(filename))
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_1', '&'])

            # Test 2: Update the replica under resc_2 using any replica under resc_1.
            self.admin.assert_icommand('irepl -S resc_1 -R resc_2 {0}'.format(filename))
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_2', '&'])

            # Test 3: Update the replica under resc_3 using replica 1.
            self.admin.assert_icommand('irepl -n1 -R resc_3 {0}'.format(filename))
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_3', '&'])

            # Test 4: Update the replica under resc_4 using the stale replica under resc_5.
            self.admin.assert_icommand('irepl -S resc_5 -R resc_4 {0}'.format(filename), 'STDERR', 'SYS_NOT_ALLOWED')
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_4', 'X'])

            # Test 5: Update the replica under resc_4 using the stale replica under resc_5.
            self.admin.assert_icommand('irepl -n5 -R resc_4 {0}'.format(filename), 'STDERR', 'SYS_NOT_ALLOWED')
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_4', 'X'])

            # Test 6: Invalid resource name.
            self.admin.assert_icommand('irepl -R invalid_resc {0}'.format(filename), 'STDERR', 'SYS_RESC_DOES_NOT_EXIST')

            # Test 7: Incompatible parameters.
            self.admin.assert_icommand('irepl -S resc_2 -n5 -R resc_5 {0}'.format(filename), 'STDERR', 'USER_INCOMPATIBLE_PARAMS')

            # Test 8: Update a replica that does not exist under the destination resource.
            self.admin.assert_icommand('irepl -R demoResc {0}'.format(filename))
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['demoResc', '&'])

        finally:
            # Clean up.
            self.admin.assert_icommand('irm -rf {0}'.format(filename))

            for i in range(replicas):
                self.admin.assert_icommand('iadmin rmresc resc_{0}'.format(i))

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_irepl_not_honoring_R_option__issue_3885(self):
        filename = 'test_file_issue_3885.txt'
        lib.make_file(filename, 1024)

        hostname = IrodsConfig().client_environment['irods_host']
        replicas = 6

        # Create resources [resc_{0..5}].
        for i in range(replicas):
            vault = os.path.join(self.admin.local_session_dir, 'issue_3885_storage_vault_' + str(i))
            path = hostname + ':' + vault
            self.admin.assert_icommand('iadmin mkresc resc_{0} unixfilesystem {1}'.format(i, path), 'STDOUT', 'unixfilesystem')

        # Create compound resource tree.
        compound_resc_child_names = ['cache', 'archive']

        self.admin.assert_icommand('iadmin mkresc comp_resc compound', 'STDOUT', 'compound')

        for name in compound_resc_child_names:
            vault = os.path.join(self.admin.local_session_dir, 'issue_3885_storage_vault_' + name)
            path = hostname + ':' + vault
            self.admin.assert_icommand('iadmin mkresc {0}_resc unixfilesystem {1}'.format(name, path), 'STDOUT', 'unixfilesystem')
            self.admin.assert_icommand('iadmin addchildtoresc comp_resc {0}_resc {0}'.format(name))

        try:
            self.admin.assert_icommand('iput -R resc_0 {0}'.format(filename))

            # Replicate the file across the remaining resources.
            for i in range(1, replicas):
                self.admin.assert_icommand('irepl -R resc_{0} {1}'.format(i, filename))

            # Replicate the file to the compound resource.
            self.admin.assert_icommand('irepl -R comp_resc {0}'.format(filename))

            # Make all replicas stale except one.
            self.admin.assert_icommand('iput -fR resc_0 {0}'.format(filename))

            # Verify that all replicas are stale except the one under [resc_0].
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_0', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_1', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_2', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_3', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_4', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_5', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['cache_resc', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['archive_resc', '&'])

            # Try to replicate to a child of the compound resource directly.
            # This should fail.
            self.admin.assert_icommand('irepl -R cache_resc {0}'.format(filename), 'STDERR', 'DIRECT_CHILD_ACCESS')

            # Replicate the file in order and verify after each command
            # if the destination resource holds a current replica.
            for i in range(1, replicas):
                self.admin.assert_icommand('irepl -R resc_{0} {1}'.format(i, filename))
                self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_' + str(i), '&'])

            # Replicate to the compound resource and verify the results.
            self.admin.assert_icommand('irepl -R comp_resc {0}'.format(filename))
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['cache_resc', '&'])
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['archive_resc', '&'])

        finally:
            # Clean up.
            self.admin.assert_icommand('irm -rf {0}'.format(filename))

            for i in range(replicas):
                self.admin.assert_icommand('iadmin rmresc resc_{0}'.format(i))

            for name in compound_resc_child_names:
                self.admin.assert_icommand('iadmin rmchildfromresc comp_resc {0}_resc'.format(name))
                self.admin.assert_icommand('iadmin rmresc {0}_resc'.format(name))

            self.admin.assert_icommand('iadmin rmresc comp_resc')

    def assert_two_replicas_where_original_is_good_and_new_is_stale(self, logical_path, original_resc, munge_resc):
        question = '''"select DATA_REPL_NUM, DATA_RESC_NAME, DATA_REPL_STATUS where DATA_NAME = '{0}' and COLL_NAME = '{1}'"'''.format(
            os.path.basename(logical_path), os.path.dirname(logical_path))

        # check on the replicas after the replication...
        out, err, ec = self.admin.run_icommand(['iquest', '%s %s %s', question])
        self.assertEqual(0, ec)
        self.assertEqual(len(err), 0)

        # there should be 2 replicas now...
        out_lines = out.splitlines()
        self.assertEqual(len(out_lines), 2)

        # ensure the original replica remains in tact (important!)
        repl_num_0, resc_name_0, repl_status_0 = out_lines[0].split()
        self.assertEqual(int(repl_num_0),    0)             # DATA_REPL_NUM
        self.assertEqual(str(resc_name_0),   original_resc) # DATA_RESC_NAME
        self.assertEqual(int(repl_status_0), 1)             # DATA_REPL_STATUS

        # ensure the new replica is marked stale
        repl_num_1, resc_name_1, repl_status_1 = out_lines[1].split()
        self.assertEqual(int(repl_num_1),    1)          # DATA_REPL_NUM
        self.assertEqual(str(resc_name_1),   munge_resc) # DATA_RESC_NAME
        self.assertEqual(int(repl_status_1), 0)          # DATA_REPL_STATUS

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

            self.admin.assert_icommand(['iput', '-K', local_file, logical_path])
            self.admin.assert_icommand(['ils', '-l', logical_path], 'STDOUT', self.admin.default_resource)

            self.admin.assert_icommand(
                ['irepl', '-S', self.admin.default_resource, '-R', munge_resc, logical_path],
                'STDERR', 'USER_CHKSUM_MISMATCH')

            self.assert_two_replicas_where_original_is_good_and_new_is_stale(logical_path, self.admin.default_resource, munge_resc)

        finally:
            self.admin.run_icommand(['irm', '-f', logical_path])
            assert_command(['mungefsctl', '--operations', 'write'])
            assert_command(['fusermount', '-u', munge_mount])
            self.admin.assert_icommand(['iadmin', 'rmresc', munge_resc])

class test_irepl_with_two_basic_ufs_resources(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):
    # In this suite:
    #   - rodsadmin otherrods
    #   - rodsuser alice
    #   - 2 unixfilesystem resources
    def setUp(self):
        super(test_irepl_with_two_basic_ufs_resources, self).setUp()

        self.admin = self.admin_sessions[0]

        self.resource_1 = 'resource1'
        resource_1_vault = os.path.join(self.admin.local_session_dir, self.resource_1 + 'vault')
        self.resource_2 = 'resource2'
        resource_2_vault = os.path.join(self.admin.local_session_dir, self.resource_2 + 'vault')

        irods_config = IrodsConfig()
        self.admin.assert_icommand(
                ['iadmin', 'mkresc', self.resource_1, 'unixfilesystem', ':'.join([test.settings.HOSTNAME_1, resource_1_vault])],
                'STDOUT_SINGLELINE', 'unixfilesystem')
        self.admin.assert_icommand(
                ['iadmin', 'mkresc', self.resource_2, 'unixfilesystem', ':'.join([test.settings.HOSTNAME_2, resource_2_vault])],
                'STDOUT_SINGLELINE', 'unixfilesystem')

        self.user = self.user_sessions[0]

    def tearDown(self):
        self.admin.assert_icommand(['iadmin', 'rmresc', self.resource_1])
        self.admin.assert_icommand(['iadmin', 'rmresc', self.resource_2])
        super(test_irepl_with_two_basic_ufs_resources, self).tearDown()

    def test_multithreaded_irepl__issue_5478(self):
        user0 = self.user_sessions[0]

        def do_test(size, threads):
            file_name = 'test_multithreaded_irepl__issue_5478'
            local_file = os.path.join(user0.local_session_dir, file_name)
            dest_path = os.path.join(user0.session_collection, file_name)

            try:
                lib.make_file(local_file, size)

                user0.assert_icommand(['iput', '-R', self.resource_1, local_file, dest_path])

                repl_thread_count = user0.run_icommand(['irepl', '-v', '-R', self.resource_2, dest_path])[0].split('|')[2].split()[0]

                self.assertEqual(int(repl_thread_count), threads)

            finally:
                user0.assert_icommand(['irm', '-f', dest_path])

        server_config_filename = paths.server_config_path()
        with open(server_config_filename) as f:
            svr_cfg = json.load(f)
            default_max_threads = svr_cfg['advanced_settings']['default_number_of_transfer_threads']

        default_buffer_size_in_bytes = 4 * (1024 ** 2)
        cases = [
            {
                'size':     0,
                'threads':  0
            },
            {
                'size':     34603008,
                'threads':  min(default_max_threads, (34603008 / default_buffer_size_in_bytes) + 1)
            }
        ]

        for case in cases:
            do_test(size=case['size'], threads=case['threads'])

    def test_irepl_to_existing_replica_with_checksum__5263(self):
        user0 = self.user_sessions[0]

        try:
            filename = 'test_irepl_to_existing_replica_with_checksum__5263'
            physical_path_1 = os.path.join(user0.local_session_dir, filename + '_1')
            lib.make_file(physical_path_1, 1024, contents='random')
            logical_path = os.path.join(user0.session_collection, filename)

            # Put data object to play with...
            user0.assert_icommand(['iput', '-K', '-R', self.resource_1, physical_path_1, logical_path])
            user0.assert_icommand(['ils', '-L', logical_path], 'STDOUT', 'sha2')

            # Replicate to other resource...
            user0.assert_icommand(['irepl', '-R', self.resource_2, logical_path])
            user0.assert_icommand(['ils', '-L', logical_path], 'STDOUT_MULTILINE', ['1 {}'.format(self.resource_2), 'sha2'])

            physical_path_2 = os.path.join(user0.local_session_dir, filename + '_2')
            lib.make_file(physical_path_2, 512, contents='random')

            # Force overwrite the replica so that replica 0 is good and replica 1 is stale
            user0.assert_icommand(['iput', '-f', '-R', self.resource_1, physical_path_2, logical_path])
            user0.assert_icommand(['ils', '-L', logical_path], 'STDOUT', ['0', '&'])
            user0.assert_icommand(['ils', '-L', logical_path], 'STDOUT', ['1', 'X'])

            # Recalculate the checksum for replica zero to avoid a USER_CHKSUM_MISMATCH error.
            user0.assert_icommand(['ichksum', '-R', self.resource_1, '-f', logical_path], 'STDOUT', [' sha2:'])

            # Replicate to the other resource and ensure it was successful
            user0.assert_icommand(['irepl', '-R', self.resource_2, logical_path])
            user0.assert_icommand(['ils', '-L', logical_path], 'STDOUT', ['0', '&'])
            user0.assert_icommand(['ils', '-L', logical_path], 'STDOUT', ['1', '&'])
            user0.assert_icommand(['ils', '-L', logical_path], 'STDOUT_MULTILINE', ['1 {}'.format(self.resource_2), 'sha2'])

        finally:
            user0.run_icommand(['irm', '-f', logical_path])

    def test_irepl_data_object_with_no_permission__4479(self):
        user0 = self.user_sessions[0]

        try:
            filename = 'test_irepl_data_object_with_no_permission__4479'
            physical_path = os.path.join(user0.local_session_dir, filename)
            lib.make_file(physical_path, 1024)
            logical_path = os.path.join(user0.session_collection, filename)

            # Put data object to play with...
            user0.assert_icommand(
                ['iput', '-R', self.resource_1, physical_path, logical_path])
            self.admin.assert_icommand(
                ['iadmin', 'ls', 'logical_path', logical_path, 'resource_hierarchy', self.resource_1],
                'STDOUT', 'DATA_REPL_STATUS: 1')
            self.admin.assert_icommand(['iscan', '-d', logical_path])

            # Attempt to replicate data object for which admin has no permissions...
            self.admin.assert_icommand(
                ['irepl', '-R', self.resource_2, logical_path],
                'STDERR', 'CAT_NO_ACCESS_PERMISSION')
            self.admin.assert_icommand(
                ['iadmin', 'ls', 'logical_path', logical_path, 'resource_hierarchy', self.resource_2],
                'STDOUT', 'No results found.')
            # TODO: #4770 Use test tool to assert that file was not created on resource_2 (i.e. iscan on remote physical file)

            # Try again with admin flag (with success)
            self.admin.assert_icommand(['irepl', '-M', '-R', self.resource_2, logical_path])
            self.admin.assert_icommand(
                ['iadmin', 'ls', 'logical_path', logical_path, 'resource_hierarchy', self.resource_2],
                'STDOUT', 'DATA_REPL_STATUS: 1')
            self.admin.assert_icommand(['iscan', '-d', logical_path])

        finally:
            user0.run_icommand(['irm', '-f', logical_path])

    def test_irepl_with_apostrophe_logical_path__issue_5759(self):
        """Test irepl with apostrophes in the logical path.

        For each irepl, the logical path will contain an apostrophe in either the collection
        name, data object name, both, or neither.
        """

        local_file = os.path.join(self.user.local_session_dir, 'test_irepl_with_apostrophe_logical_path__issue_5759')
        lib.make_file(local_file, 1024, 'arbitrary')

        collection_names = ["collection", "collect'ion"]

        data_names = ["data_object", "data'_object"]

        for coll in collection_names:
            collection_path = os.path.join(self.user.session_collection, coll)

            self.user.assert_icommand(['imkdir', collection_path])

            try:
                for name in data_names:
                    logical_path = os.path.join(collection_path, name)

                    self.user.assert_icommand(['iput', '-R', self.resource_1, local_file, logical_path])

                    self.user.assert_icommand(['irepl', '-R', self.resource_2, logical_path])

                    self.user.assert_icommand(['ils', '-l', logical_path], 'STDOUT', name)

                self.user.assert_icommand(['ils', '-l', collection_path], 'STDOUT', coll)

            finally:
                self.user.run_icommand(['irm', '-r', '-f', collection_path])


class test_invalid_parameters(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):
    def setUp(self):
        super(test_invalid_parameters, self).setUp()

        self.admin = self.admin_sessions[0]

        self.leaf_rescs = {
            'a' : {
                'name': 'test_irepl_repl_status_a',
                'vault': os.path.join(self.admin.local_session_dir, 'a'),
                'host': test.settings.HOSTNAME_1
            },
            'b' : {
                'name': 'test_irepl_repl_status_b',
                'vault': os.path.join(self.admin.local_session_dir, 'b'),
                'host': test.settings.HOSTNAME_2
            },
            'c' : {
                'name': 'test_irepl_repl_status_c',
                'vault': os.path.join(self.admin.local_session_dir, 'c'),
                'host': test.settings.HOSTNAME_3
            },
            'f' : {
                'name': 'test_irepl_repl_status_f',
                'vault': os.path.join(self.admin.local_session_dir, 'f'),
                'host': test.settings.HOSTNAME_1
            }
        }

        for resc in self.leaf_rescs.values():
            self.admin.assert_icommand(['iadmin', 'mkresc', resc['name'], 'unixfilesystem',
                ':'.join([resc['host'], resc['vault']])],
                'STDOUT', resc['name'])

        self.parent_rescs = {
            'd' : 'test_irepl_repl_status_d',
            'e' : 'test_irepl_repl_status_e',
        }

        for resc in self.parent_rescs.values():
            self.admin.assert_icommand(['iadmin', 'mkresc', resc, 'passthru'], 'STDOUT', resc)

        self.admin.assert_icommand(['iadmin', 'addchildtoresc', self.parent_rescs['d'], self.parent_rescs['e']])
        self.admin.assert_icommand(['iadmin', 'addchildtoresc', self.parent_rescs['e'], self.leaf_rescs['f']['name']])

        #self.env_backup = copy.deepcopy(self.admin.environment_file_contents)
        self.admin.environment_file_contents.update({'irods_default_resource': self.leaf_rescs['a']['name']})

        self.filename = 'foo'
        self.local_path = os.path.join(self.admin.local_session_dir, self.filename)
        self.logical_path = os.path.join(self.admin.session_collection, 'subdir', self.filename)

    def tearDown(self):
        #self.admin.environment_file_contents = self.env_backup

        self.admin.assert_icommand(['iadmin', 'rmchildfromresc', self.parent_rescs['d'], self.parent_rescs['e']])
        self.admin.assert_icommand(['iadmin', 'rmchildfromresc', self.parent_rescs['e'], self.leaf_rescs['f']['name']])

        for resc in self.parent_rescs.values():
            self.admin.assert_icommand(['iadmin', 'rmresc', resc])

        for resc in self.leaf_rescs.values():
            self.admin.assert_icommand(['iadmin', 'rmresc', resc['name']])

        super(test_invalid_parameters, self).tearDown()

    def test_invalid_parameters(self):
        nonexistent_resource_name = 'nope'

        # does not exist
        self.admin.assert_icommand(['irepl', self.logical_path], 'STDERR', 'does not exist')

        # put a file
        if not os.path.exists(self.local_path):
            lib.make_file(self.local_path, 1024)
        self.admin.assert_icommand(['imkdir', os.path.dirname(self.logical_path)])
        self.admin.assert_icommand(['iput', '-R', self.leaf_rescs['a']['name'], self.local_path, self.logical_path])

        try:
            # USER_INCOMPATIBLE_PARAMS
            # irepl -S a -n 0 foo
            self.admin.assert_icommand(['irepl', '-S', self.leaf_rescs['a']['name'], '-n0', self.logical_path],
                'STDERR', 'USER_INCOMPATIBLE_PARAMS')

            # irepl -a -R
            self.admin.assert_icommand(['irepl', '-a', '-R', self.leaf_rescs['a']['name'], '-n0', self.logical_path],
                'STDERR', 'USER_INCOMPATIBLE_PARAMS')

            # DIRECT_CHILD_ACCESS
            # irepl -R f foo
            self.admin.assert_icommand(['irepl', '-R', self.leaf_rescs['f']['name'], self.logical_path],
                'STDERR', 'DIRECT_CHILD_ACCESS')

            # irepl -R e foo
            self.admin.assert_icommand(['irepl', '-R', self.parent_rescs['e'], self.logical_path],
                'STDERR', 'DIRECT_CHILD_ACCESS')

            # irepl -R 'd;e' foo
            hier = ';'.join([self.parent_rescs['d'], self.parent_rescs['e']])
            self.admin.assert_icommand(['irepl', '-R', hier, self.logical_path],
                'STDERR', 'DIRECT_CHILD_ACCESS')

            # irepl -R 'd;e;f' foo
            hier = ';'.join([hier, self.leaf_rescs['f']['name']])
            self.admin.assert_icommand(['irepl', '-R', hier, self.logical_path],
                'STDERR', 'DIRECT_CHILD_ACCESS')

            # irepl -R 'e;f' foo
            hier = ';'.join(hier.split(';')[1:-1])
            self.admin.assert_icommand(['irepl', '-R', hier, self.logical_path],
                'STDERR', 'DIRECT_CHILD_ACCESS')

            # irepl -S f foo
            self.admin.assert_icommand(['irepl', '-S', self.leaf_rescs['f']['name'], self.logical_path],
                'STDERR', 'DIRECT_CHILD_ACCESS')

            # irepl -S e foo
            self.admin.assert_icommand(['irepl', '-S', self.parent_rescs['e'], self.logical_path],
                'STDERR', 'DIRECT_CHILD_ACCESS')

            # irepl -S 'd;e' foo
            hier = ';'.join([self.parent_rescs['d'], self.parent_rescs['e']])
            self.admin.assert_icommand(['irepl', '-S', hier, self.logical_path],
                'STDERR', 'DIRECT_CHILD_ACCESS')

            # irepl -S 'd;e;f' foo
            hier = ';'.join([hier, self.leaf_rescs['f']['name']])
            self.admin.assert_icommand(['irepl', '-S', hier, self.logical_path],
                'STDERR', 'DIRECT_CHILD_ACCESS')

            # irepl -S 'e;f' foo
            hier = ';'.join([self.parent_rescs['e'], self.leaf_rescs['f']['name']])
            self.admin.assert_icommand(['irepl', '-S', hier, self.logical_path],
                'STDERR', 'DIRECT_CHILD_ACCESS')

            # SYS_RESC_DOES_NOT_EXIST
            # irepl -S nope foo
            self.admin.assert_icommand(['irepl', '-S', nonexistent_resource_name, self.logical_path],
                'STDERR', 'SYS_RESC_DOES_NOT_EXIST')

            # irepl -R nope foo
            self.admin.assert_icommand(['irepl', '-R', nonexistent_resource_name, self.logical_path],
                'STDERR', 'SYS_RESC_DOES_NOT_EXIST')

            # irepl -S nope -a foo
            self.admin.assert_icommand(['irepl', '-S', nonexistent_resource_name, '-a', self.logical_path],
                'STDERR', 'SYS_RESC_DOES_NOT_EXIST')

            # ensure no replications took place
            out, _, _ = self.admin.run_icommand(['ils', '-l', self.logical_path])
            print(out)
            # a replica should exist on leaf resource a
            for resc in [self.leaf_rescs.values()][1:]:
                self.assertNotIn(resc['name'], out, msg='found unwanted replica on [{}]'.format(resc['name']))
            for resc in self.parent_rescs.values():
                self.assertNotIn(resc, out, msg='found replica on coordinating resc [{}]'.format(resc))

        finally:
            self.admin.assert_icommand(['irm', '-rf', os.path.dirname(self.logical_path)])

class test_irepl_repl_status(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):
    def setUp(self):
        super(test_irepl_repl_status, self).setUp()

        self.admin = self.admin_sessions[0]

        self.leaf_rescs = {
            'a' : {
                'name': 'test_irepl_repl_status_a',
                'vault': os.path.join(self.admin.local_session_dir, 'a'),
                'host': test.settings.HOSTNAME_1
            },
            'b' : {
                'name': 'test_irepl_repl_status_b',
                'vault': os.path.join(self.admin.local_session_dir, 'b'),
                'host': test.settings.HOSTNAME_2
            },
            'c' : {
                'name': 'test_irepl_repl_status_c',
                'vault': os.path.join(self.admin.local_session_dir, 'c'),
                'host': test.settings.HOSTNAME_3
            }
        }

        for resc in self.leaf_rescs.values():
            self.admin.assert_icommand(['iadmin', 'mkresc', resc['name'], 'unixfilesystem',
                ':'.join([resc['host'], resc['vault']])],
                'STDOUT', resc['name'])

        self.admin.environment_file_contents.update({'irods_default_resource': self.leaf_rescs['a']['name']})

        self.filename = 'foo'
        self.local_path = os.path.join(self.admin.local_session_dir, self.filename)
        self.logical_path = os.path.join(self.admin.session_collection, 'subdir', self.filename)

    def tearDown(self):
        for resc in self.leaf_rescs.values():
            self.admin.assert_icommand(['iadmin', 'rmresc', resc['name']])

        super(test_irepl_repl_status, self).tearDown()

    def test_irepl_foo_no_arguments(self):
        # irepl foo (irepl -R a foo) (target default resource case)
        scenarios = [
            {'start':{'a':'-', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'-', 'c':'X'}, 'end':{'a':'X', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'-'}, 'end':{'a':'&', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'X'}, 'end':{'a':'&', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'-'}, 'end':{'a':'&', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'X'}, 'end':{'a':'&', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'-'}, 'end':{'a':'X', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'X'}, 'end':{'a':'X', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}}
        ]

        replica_status_test.run_command_against_scenarios(self, ['irepl', self.logical_path], scenarios, 'irepl foo')

    def test_irepl_Rb_foo(self):
        # irepl -R b foo (source unspecified, destination non-default resource)
        scenarios = [
            {'start':{'a':'-', 'b':'-', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'-', 'c':'X'}, 'end':{'a':'-', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'-'}, 'end':{'a':'-', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'X'}, 'end':{'a':'-', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'-'}, 'end':{'a':'-', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'X'}, 'end':{'a':'-', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'&'}, 'end':{'a':'X', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'-'}, 'end':{'a':'X', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'&'}, 'end':{'a':'X', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'X'}, 'end':{'a':'X', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'&'}, 'end':{'a':'X', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}}
        ]

        replica_status_test.run_command_against_scenarios(self,
            ['irepl', '-R', self.leaf_rescs['b']['name'], self.logical_path],
            scenarios, 'irepl -R b foo')

    def test_irepl_Sa_Rb_foo(self):
        # irepl -S a -R b foo (source default resource, destination non-default resource)
        scenarios = [
            {'start':{'a':'-', 'b':'-', 'c':'&'}, 'end':{'a':'-', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'-', 'b':'-', 'c':'X'}, 'end':{'a':'-', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'-'}, 'end':{'a':'-', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'X'}, 'end':{'a':'-', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'-'}, 'end':{'a':'-', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'&'}, 'end':{'a':'-', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'X'}, 'end':{'a':'-', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'&'}, 'end':{'a':'X', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'-'}, 'end':{'a':'X', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'&'}, 'end':{'a':'X', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'X'}, 'end':{'a':'X', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'&'}, 'end':{'a':'X', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}}
        ]

        replica_status_test.run_command_against_scenarios(self,
            ['irepl', '-S', self.leaf_rescs['a']['name'], '-R', self.leaf_rescs['b']['name'], self.logical_path],
            scenarios, 'irepl -S a -R b foo')

    def test_irepl_Sc_Rb_foo(self):
        # irepl -S c -R b foo (source non-default resource, destination non-default resource)
        scenarios = [
            {'start':{'a':'-', 'b':'-', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'-', 'c':'X'}, 'end':{'a':'-', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'-'}, 'end':{'a':'-', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'X'}, 'end':{'a':'-', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'-'}, 'end':{'a':'-', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'X'}, 'end':{'a':'-', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'-'}, 'end':{'a':'&', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'X'}, 'end':{'a':'&', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'-'}, 'end':{'a':'&', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'X'}, 'end':{'a':'&', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'-'}, 'end':{'a':'X', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'&'}, 'end':{'a':'X', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'-'}, 'end':{'a':'X', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'&'}, 'end':{'a':'X', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'X'}, 'end':{'a':'X', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'&'}, 'end':{'a':'X', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}}
        ]

        replica_status_test.run_command_against_scenarios(self,
            ['irepl', '-S', self.leaf_rescs['c']['name'], '-R', self.leaf_rescs['b']['name'], self.logical_path],
            scenarios, 'irepl -S c -R b foo')

    def test_irepl_Sb_foo(self):
        # irepl -S b foo (irepl -S b -R a foo) (source non-default resource, destination unspecified)
        scenarios = [
            {'start':{'a':'-', 'b':'-', 'c':'&'}, 'end':{'a':'-', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'-', 'b':'-', 'c':'X'}, 'end':{'a':'-', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'&'}, 'end':{'a':'X', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'-'}, 'end':{'a':'&', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'X'}, 'end':{'a':'&', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'-'}, 'end':{'a':'&', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'X'}, 'end':{'a':'&', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'-'}, 'end':{'a':'X', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'&'}, 'end':{'a':'X', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'X'}, 'end':{'a':'X', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'&'}, 'end':{'a':'X', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}}
        ]

        replica_status_test.run_command_against_scenarios(self,
            ['irepl', '-S', self.leaf_rescs['b']['name'], self.logical_path],
            scenarios, 'irepl -S b foo')

    def test_irepl_n0_foo(self):
        # irepl -n 0 foo (irepl -n 0 -R a foo) (source replica 0, destination unspecified)
        scenarios = [
            {'start':{'a':'-', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'-', 'c':'X'}, 'end':{'a':'X', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'&'}, 'end':{'a':'X', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'-'}, 'end':{'a':'&', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'X'}, 'end':{'a':'&', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'-'}, 'end':{'a':'&', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'X'}, 'end':{'a':'&', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'-'}, 'end':{'a':'X', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'&'}, 'end':{'a':'X', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'X'}, 'end':{'a':'X', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'-'}, 'end':{'a':'X', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'&'}, 'end':{'a':'X', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'X'}, 'end':{'a':'X', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'&'}, 'end':{'a':'X', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}}
        ]

        replica_status_test.run_command_against_scenarios(self,
                ['irepl', '-n', '0', self.logical_path],
                scenarios, 'irepl -n0 foo')

    def test_irepl_n1_foo(self):
        # irepl -n 1 foo (irepl -n 1 -R a foo) (source replica 1, destination unspecified)
        scenarios = [
            {'start':{'a':'-', 'b':'-', 'c':'&'}, 'end':{'a':'-', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'-', 'b':'-', 'c':'X'}, 'end':{'a':'-', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'-'}, 'end':{'a':'-', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'X'}, 'end':{'a':'X', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'-'}, 'end':{'a':'-', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'-'}, 'end':{'a':'&', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'X'}, 'end':{'a':'&', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'-'}, 'end':{'a':'&', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'X'}, 'end':{'a':'&', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'-'}, 'end':{'a':'X', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'X'}, 'end':{'a':'X', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'&'}, 'end':{'a':'X', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}}
        ]

        replica_status_test.run_command_against_scenarios(self,
                ['irepl', '-n', '1', self.logical_path],
                scenarios, 'irepl -n1 foo')

    def test_irepl_n0_Rb_foo(self):
        # irepl -n 0 -R b foo (source replica 0, destination non-default resource)
        scenarios = [
            {'start':{'a':'-', 'b':'-', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'-', 'c':'X'}, 'end':{'a':'-', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'-'}, 'end':{'a':'-', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'X'}, 'end':{'a':'-', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'-'}, 'end':{'a':'-', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'&'}, 'end':{'a':'-', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'X'}, 'end':{'a':'-', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'&'}, 'end':{'a':'X', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'-'}, 'end':{'a':'X', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'&'}, 'end':{'a':'X', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'X'}, 'end':{'a':'X', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'&'}, 'end':{'a':'X', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}}
        ]

        replica_status_test.run_command_against_scenarios(self,
                ['irepl', '-n', '0', '-R', self.leaf_rescs['b']['name'], self.logical_path],
                scenarios, 'irepl -n0 -Rb foo')

    def test_irepl_n1_Rb_foo(self):
        # irepl -n 1 -R b foo (source replica 1, destination non-default resource)
        scenarios = [
            {'start':{'a':'-', 'b':'-', 'c':'&'}, 'end':{'a':'-', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'-', 'b':'-', 'c':'X'}, 'end':{'a':'-', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'-'}, 'end':{'a':'-', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'X'}, 'end':{'a':'-', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'-'}, 'end':{'a':'-', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'X'}, 'end':{'a':'-', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'-'}, 'end':{'a':'&', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'X'}, 'end':{'a':'&', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'-'}, 'end':{'a':'&', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'X'}, 'end':{'a':'&', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'-'}, 'end':{'a':'X', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'&'}, 'end':{'a':'X', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'-'}, 'end':{'a':'X', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'&'}, 'end':{'a':'X', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'X'}, 'end':{'a':'X', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'&'}, 'end':{'a':'X', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}}
        ]

        replica_status_test.run_command_against_scenarios(self,
                ['irepl', '-n', '1', '-R', self.leaf_rescs['b']['name'], self.logical_path],
                scenarios, 'irepl -n1 -Rb foo')

    def test_irepl_a_foo(self):
        # irepl -a foo (source unspecified, destination ALL; note: does not create replicas)
        scenarios = [
            {'start':{'a':'-', 'b':'-', 'c':'&'}, 'end':{'a':'-', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'-', 'c':'X'}, 'end':{'a':'-', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'-'}, 'end':{'a':'-', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'X'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'-'}, 'end':{'a':'-', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'X'}, 'end':{'a':'-', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'-'}, 'end':{'a':'&', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'X'}, 'end':{'a':'&', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'-'}, 'end':{'a':'X', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'X'}, 'end':{'a':'X', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}}
        ]

        replica_status_test.run_command_against_scenarios(self, ['irepl', '-a', self.logical_path], scenarios, 'irepl -a foo')

    def test_irepl_a_Sa_foo(self):
        # irepl -a -Sa foo (source default resource, destination ALL)
        scenarios = [
            {'start':{'a':'-', 'b':'-', 'c':'&'}, 'end':{'a':'-', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'-', 'b':'-', 'c':'X'}, 'end':{'a':'-', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'-'}, 'end':{'a':'-', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'X'}, 'end':{'a':'-', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'-'}, 'end':{'a':'-', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'&'}, 'end':{'a':'-', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'X'}, 'end':{'a':'-', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'-'}, 'end':{'a':'&', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'X'}, 'end':{'a':'&', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'-'}, 'end':{'a':'X', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'&'}, 'end':{'a':'X', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'X'}, 'end':{'a':'X', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'-'}, 'end':{'a':'X', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'&'}, 'end':{'a':'X', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'X'}, 'end':{'a':'X', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'&'}, 'end':{'a':'X', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}}
        ]

        replica_status_test.run_command_against_scenarios(self,
                ['irepl', '-a', '-S', self.leaf_rescs['a']['name'], self.logical_path],
                scenarios, 'irepl -a -Sa foo')

    def test_irepl_a_Sb_foo(self):
        # irepl -a -Sb foo (source non-default resource, destination ALL)
        scenarios = [
            {'start':{'a':'-', 'b':'-', 'c':'&'}, 'end':{'a':'-', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'-', 'b':'-', 'c':'X'}, 'end':{'a':'-', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'-'}, 'end':{'a':'-', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'X'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'-'}, 'end':{'a':'-', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'&'}, 'end':{'a':'-', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'X'}, 'end':{'a':'-', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'-'}, 'end':{'a':'&', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'X'}, 'end':{'a':'&', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'-'}, 'end':{'a':'&', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'X'}, 'end':{'a':'&', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'-'}, 'end':{'a':'X', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'&'}, 'end':{'a':'X', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'X'}, 'end':{'a':'X', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'&'}, 'end':{'a':'X', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}}
        ]

        replica_status_test.run_command_against_scenarios(self,
                ['irepl', '-a', '-S', self.leaf_rescs['b']['name'], self.logical_path],
                scenarios, 'irepl -a -Sb foo')

    def test_irepl_a_n0_foo(self):
        # irepl -a -n0 foo (source replica 0, destination ALL)
        scenarios = [
            {'start':{'a':'-', 'b':'-', 'c':'&'}, 'end':{'a':'-', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'-', 'c':'X'}, 'end':{'a':'-', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'-'}, 'end':{'a':'-', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'X'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'-'}, 'end':{'a':'-', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'&'}, 'end':{'a':'-', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'X'}, 'end':{'a':'-', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'-'}, 'end':{'a':'&', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'X'}, 'end':{'a':'&', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'-'}, 'end':{'a':'X', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'&'}, 'end':{'a':'X', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'X'}, 'end':{'a':'X', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'-'}, 'end':{'a':'X', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'&'}, 'end':{'a':'X', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'X'}, 'end':{'a':'X', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'&'}, 'end':{'a':'X', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}}
        ]

        replica_status_test.run_command_against_scenarios(self,
                ['irepl', '-a', '-n', '0', self.logical_path],
                scenarios, 'irepl -a -n0 foo')

    def test_irepl_a_n1_foo(self):
        # irepl -a -n1 foo (source replica 1, destination ALL)
        scenarios = [
            {'start':{'a':'-', 'b':'-', 'c':'&'}, 'end':{'a':'-', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'-', 'b':'-', 'c':'X'}, 'end':{'a':'-', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'-'}, 'end':{'a':'-', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'X'}, 'end':{'a':'-', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'-'}, 'end':{'a':'-', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'X'}, 'end':{'a':'-', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'-'}, 'end':{'a':'&', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'X'}, 'end':{'a':'&', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'-'}, 'end':{'a':'&', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'X'}, 'end':{'a':'&', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'-'}, 'end':{'a':'X', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'X'}, 'end':{'a':'X', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'&'}, 'end':{'a':'X', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}}
        ]

        replica_status_test.run_command_against_scenarios(self,
                ['irepl', '-a', '-n', '1', self.logical_path],
                scenarios, 'irepl -a -n1 foo')

class test_irepl_replication_hierarchy(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):
    repl_statuses = ['X', '&', '?']
    def setUp(self):
        super(test_irepl_replication_hierarchy, self).setUp()

        self.admin = self.admin_sessions[0]

        self.leaf_rescs = {
            'a' : {
                'name': 'test_irepl_replication_hierarchy_a',
                'vault': os.path.join(self.admin.local_session_dir, 'a'),
                'host': test.settings.HOSTNAME_1
            },
            'b' : {
                'name': 'test_irepl_replication_hierarchy_b',
                'vault': os.path.join(self.admin.local_session_dir, 'b'),
                'host': test.settings.HOSTNAME_2
            },
            'c' : {
                'name': 'test_irepl_replication_hierarchy_c',
                'vault': os.path.join(self.admin.local_session_dir, 'c'),
                'host': test.settings.HOSTNAME_3
            },
            'f' : {
                'name': 'test_irepl_replication_hierarchy_f',
                'vault': os.path.join(self.admin.local_session_dir, 'f'),
                'host': test.settings.HOSTNAME_1
            }
        }

        for resc in self.leaf_rescs.values():
            self.admin.assert_icommand(['iadmin', 'mkresc', resc['name'], 'unixfilesystem',
                ':'.join([resc['host'], resc['vault']])],
                'STDOUT', resc['name'])

        self.parent_rescs = {
            'd' : 'test_irepl_replication_hierarchy_d',
            'e' : 'test_irepl_replication_hierarchy_e',
        }

        self.admin.assert_icommand(['iadmin', 'mkresc', self.parent_rescs['d'], 'passthru'], 'STDOUT', self.parent_rescs['d'])
        self.admin.assert_icommand(['iadmin', 'mkresc', self.parent_rescs['e'], 'replication'], 'STDOUT', self.parent_rescs['e'])

        self.admin.assert_icommand(['iadmin', 'addchildtoresc', self.parent_rescs['d'], self.parent_rescs['e']])

        #self.env_backup = copy.deepcopy(self.admin.environment_file_contents)
        self.admin.environment_file_contents.update({'irods_default_resource': self.parent_rescs['d']})

        self.filename = 'foo'
        self.local_path = os.path.join(self.admin.local_session_dir, self.filename)
        self.logical_path = os.path.join(self.admin.session_collection, 'subdir', self.filename)

    def tearDown(self):
        #self.admin.environment_file_contents = self.env_backup

        self.admin.assert_icommand(['iadmin', 'rmchildfromresc', self.parent_rescs['d'], self.parent_rescs['e']])

        for resc in self.parent_rescs.values():
            self.admin.assert_icommand(['iadmin', 'rmresc', resc])

        for resc in self.leaf_rescs.values():
            self.admin.assert_icommand(['iadmin', 'rmresc', resc['name']])

        super(test_irepl_replication_hierarchy, self).tearDown()

    def setup_replicas(self, scenario):
        initial_replicas = scenario['start']
        ordered_keys = sorted(initial_replicas)

        replica_zero_resource = None

        # create the new data object
        for repl in ordered_keys:
            if initial_replicas[repl] != '-':
                replica_zero_resource = self.leaf_rescs[repl]['name']
                self.admin.assert_icommand(['iput', '-R', replica_zero_resource, self.local_path, self.logical_path])
                break

        # stage replicas and set replica status for scenario
        for repl in ordered_keys:
            if initial_replicas[repl] != '-':
                if self.leaf_rescs[repl]['name'] != replica_zero_resource:
                    self.admin.assert_icommand(['irepl', '-n0', '-R', self.leaf_rescs[repl]['name'], self.logical_path])

                self.admin.assert_icommand(
                    ['iadmin', 'modrepl', 'logical_path', self.logical_path, 'resource_hierarchy', self.leaf_rescs[repl]['name'],
                        'DATA_REPL_STATUS', str(self.repl_statuses.index(initial_replicas[repl]))])

        # construct the replication hierarchy after everything is prepared
        for resc in self.leaf_rescs.values():
            if resc['name'] is not self.leaf_rescs['f']['name']:
                self.admin.assert_icommand(['iadmin', 'addchildtoresc', self.parent_rescs['e'], resc['name']])

    def run_command_against_scenarios(self, command, scenarios, name, file_size=1024):
        i = 0
        for scenario in scenarios:
            i = i + 1
            subtest_name = name.replace(' ', '_').replace('-', '') + f'_{i}'
            with self.subTest(subtest_name):
                print("============= (" + name + "): [" + str(i) + "] =============")
                print(scenario)
                try:
                    if not os.path.exists(self.local_path):
                        lib.make_file(self.local_path, file_size)
                    self.admin.assert_icommand(['imkdir', os.path.dirname(self.logical_path)])

                    self.setup_replicas(scenario)

                    out,err,_ = self.admin.run_icommand(command)
                    print(out)
                    print(err)
                    replica_status_test.assert_result(self, scenario, err=err)

                finally:
                    self.admin.assert_icommand(['irm', '-rf', os.path.dirname(self.logical_path)])

                    # take the tree apart
                    for resc in self.leaf_rescs.values():
                        if resc['name'] is not self.leaf_rescs['f']['name']:
                            self.admin.run_icommand(['iadmin', 'rmchildfromresc', self.parent_rescs['e'], resc['name']])

    # Each of these matrix tests contains 29 unique scenarios:
    #   - there are 81 scenarios total (Cartesian product)
    #   - the no-replica scenario is invalid
    #   - 51 of these are duplicates
    def test_irepl_foo_no_arguments(self):
        # irepl foo (irepl -R d foo) (source unspecified, target default resource (root of replication hierarchy))
        scenarios = [
            #{'start':{'a':'-', 'b':'-', 'c':'-', 'f':'-'}, 'end':{'a':'-', 'b':'-', 'c':'-', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},              # -
            {'start':{'a':'-', 'b':'-', 'c':'-', 'f':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d-f&
            {'start':{'a':'-', 'b':'-', 'c':'-', 'f':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d-fX
            {'start':{'a':'-', 'b':'-', 'c':'&', 'f':'-'}, 'end':{'a':'-', 'b':'-', 'c':'&', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&f-
            {'start':{'a':'-', 'b':'-', 'c':'&', 'f':'&'}, 'end':{'a':'-', 'b':'-', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&f&
            {'start':{'a':'-', 'b':'-', 'c':'&', 'f':'X'}, 'end':{'a':'-', 'b':'-', 'c':'&', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&fX
            {'start':{'a':'-', 'b':'-', 'c':'X', 'f':'-'}, 'end':{'a':'-', 'b':'-', 'c':'X', 'f':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # dXf-
            {'start':{'a':'-', 'b':'-', 'c':'X', 'f':'&'}, 'end':{'a':'-', 'b':'-', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # dXf&
            {'start':{'a':'-', 'b':'-', 'c':'X', 'f':'X'}, 'end':{'a':'-', 'b':'-', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # dXfX
#           {'start':{'a':'-', 'b':'&', 'c':'-', 'f':'-'}, 'end':{'a':'-', 'b':'&', 'c':'-', 'f':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&f- (duplicate)
#           {'start':{'a':'-', 'b':'&', 'c':'-', 'f':'&'}, 'end':{'a':'-', 'b':'&', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&f& (duplicate)
#           {'start':{'a':'-', 'b':'&', 'c':'-', 'f':'X'}, 'end':{'a':'-', 'b':'&', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&fX (duplicate)
            {'start':{'a':'-', 'b':'&', 'c':'&', 'f':'-'}, 'end':{'a':'-', 'b':'&', 'c':'&', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&&f-
            {'start':{'a':'-', 'b':'&', 'c':'&', 'f':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&&f&
            {'start':{'a':'-', 'b':'&', 'c':'&', 'f':'X'}, 'end':{'a':'-', 'b':'&', 'c':'&', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&&fX
            {'start':{'a':'-', 'b':'&', 'c':'X', 'f':'-'}, 'end':{'a':'-', 'b':'&', 'c':'&', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&Xf-
            {'start':{'a':'-', 'b':'&', 'c':'X', 'f':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&Xf&
            {'start':{'a':'-', 'b':'&', 'c':'X', 'f':'X'}, 'end':{'a':'-', 'b':'&', 'c':'&', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&XfX
#           {'start':{'a':'-', 'b':'X', 'c':'-', 'f':'-'}, 'end':{'a':'-', 'b':'X', 'c':'-', 'f':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # dXf- (duplicate)
#           {'start':{'a':'-', 'b':'X', 'c':'-', 'f':'&'}, 'end':{'a':'-', 'b':'&', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # dXf& (duplicate)
#           {'start':{'a':'-', 'b':'X', 'c':'-', 'f':'X'}, 'end':{'a':'-', 'b':'X', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # dXfX (duplicate)
#           {'start':{'a':'-', 'b':'X', 'c':'&', 'f':'-'}, 'end':{'a':'-', 'b':'X', 'c':'&', 'f':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&Xf- (duplicate)
#           {'start':{'a':'-', 'b':'X', 'c':'&', 'f':'&'}, 'end':{'a':'-', 'b':'X', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&Xf& (duplicate)
#           {'start':{'a':'-', 'b':'X', 'c':'&', 'f':'X'}, 'end':{'a':'-', 'b':'X', 'c':'&', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&XfX (duplicate)
            {'start':{'a':'-', 'b':'X', 'c':'X', 'f':'-'}, 'end':{'a':'-', 'b':'X', 'c':'X', 'f':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # dXXf-
            {'start':{'a':'-', 'b':'X', 'c':'X', 'f':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # dXXf&
            {'start':{'a':'-', 'b':'X', 'c':'X', 'f':'X'}, 'end':{'a':'-', 'b':'X', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # dXXfX
#           {'start':{'a':'&', 'b':'-', 'c':'-', 'f':'-'}, 'end':{'a':'&', 'b':'-', 'c':'-', 'f':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&f- (duplicate)
#           {'start':{'a':'&', 'b':'-', 'c':'-', 'f':'&'}, 'end':{'a':'&', 'b':'-', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&f& (duplicate)
#           {'start':{'a':'&', 'b':'-', 'c':'-', 'f':'X'}, 'end':{'a':'&', 'b':'-', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&fX (duplicate)
#           {'start':{'a':'&', 'b':'-', 'c':'&', 'f':'-'}, 'end':{'a':'&', 'b':'-', 'c':'&', 'f':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&&f- (duplicate)
#           {'start':{'a':'&', 'b':'-', 'c':'&', 'f':'&'}, 'end':{'a':'&', 'b':'-', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&&f& (duplicate)
#           {'start':{'a':'&', 'b':'-', 'c':'&', 'f':'X'}, 'end':{'a':'&', 'b':'-', 'c':'&', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&&fX (duplicate)
#           {'start':{'a':'&', 'b':'-', 'c':'X', 'f':'-'}, 'end':{'a':'&', 'b':'-', 'c':'X', 'f':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&Xf- (duplicate)
#           {'start':{'a':'&', 'b':'-', 'c':'X', 'f':'&'}, 'end':{'a':'&', 'b':'-', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&Xf& (duplicate)
#           {'start':{'a':'&', 'b':'-', 'c':'X', 'f':'X'}, 'end':{'a':'&', 'b':'-', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&XfX (duplicate)
#           {'start':{'a':'&', 'b':'&', 'c':'-', 'f':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-', 'f':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&&f- (duplicate)
#           {'start':{'a':'&', 'b':'&', 'c':'-', 'f':'&'}, 'end':{'a':'&', 'b':'&', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&&f& (duplicate)
#           {'start':{'a':'&', 'b':'&', 'c':'-', 'f':'X'}, 'end':{'a':'&', 'b':'&', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&&fX (duplicate)
            {'start':{'a':'&', 'b':'&', 'c':'&', 'f':'-'}, 'end':{'a':'&', 'b':'&', 'c':'&', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&&&f-
            {'start':{'a':'&', 'b':'&', 'c':'&', 'f':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&&&f&
            {'start':{'a':'&', 'b':'&', 'c':'&', 'f':'X'}, 'end':{'a':'&', 'b':'&', 'c':'&', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&&&fX
            {'start':{'a':'&', 'b':'&', 'c':'X', 'f':'-'}, 'end':{'a':'&', 'b':'&', 'c':'&', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&&Xf-
            {'start':{'a':'&', 'b':'&', 'c':'X', 'f':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&&Xf&
            {'start':{'a':'&', 'b':'&', 'c':'X', 'f':'X'}, 'end':{'a':'&', 'b':'&', 'c':'&', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&&XfX
#           {'start':{'a':'&', 'b':'X', 'c':'-', 'f':'-'}, 'end':{'a':'&', 'b':'X', 'c':'-', 'f':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&Xf- (duplicate)
#           {'start':{'a':'&', 'b':'X', 'c':'-', 'f':'&'}, 'end':{'a':'&', 'b':'X', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&Xf& (duplicate)
#           {'start':{'a':'&', 'b':'X', 'c':'-', 'f':'X'}, 'end':{'a':'&', 'b':'X', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&XfX (duplicate)
#           {'start':{'a':'&', 'b':'X', 'c':'&', 'f':'-'}, 'end':{'a':'&', 'b':'X', 'c':'&', 'f':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&&Xf- (duplicate)
#           {'start':{'a':'&', 'b':'X', 'c':'&', 'f':'&'}, 'end':{'a':'&', 'b':'X', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&&Xf& (duplicate)
#           {'start':{'a':'&', 'b':'X', 'c':'&', 'f':'X'}, 'end':{'a':'&', 'b':'X', 'c':'&', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&&XfX (duplicate)
            {'start':{'a':'&', 'b':'X', 'c':'X', 'f':'-'}, 'end':{'a':'&', 'b':'&', 'c':'&', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&XXf-
            {'start':{'a':'&', 'b':'X', 'c':'X', 'f':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&XXf&
            {'start':{'a':'&', 'b':'X', 'c':'X', 'f':'X'}, 'end':{'a':'&', 'b':'&', 'c':'&', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&XXfX
#           {'start':{'a':'X', 'b':'-', 'c':'-', 'f':'-'}, 'end':{'a':'X', 'b':'-', 'c':'-', 'f':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # dXf- (duplicate)
#           {'start':{'a':'X', 'b':'-', 'c':'-', 'f':'&'}, 'end':{'a':'&', 'b':'-', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # dXf& (duplicate)
#           {'start':{'a':'X', 'b':'-', 'c':'-', 'f':'X'}, 'end':{'a':'X', 'b':'-', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # dXfX (duplicate)
#           {'start':{'a':'X', 'b':'-', 'c':'&', 'f':'-'}, 'end':{'a':'X', 'b':'-', 'c':'&', 'f':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&Xf- (duplicate)
#           {'start':{'a':'X', 'b':'-', 'c':'&', 'f':'&'}, 'end':{'a':'X', 'b':'-', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&Xf& (duplicate)
#           {'start':{'a':'X', 'b':'-', 'c':'&', 'f':'X'}, 'end':{'a':'X', 'b':'-', 'c':'&', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&XfX (duplicate)
#           {'start':{'a':'X', 'b':'-', 'c':'X', 'f':'-'}, 'end':{'a':'X', 'b':'-', 'c':'X', 'f':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # dXf- (duplicate)
#           {'start':{'a':'X', 'b':'-', 'c':'X', 'f':'&'}, 'end':{'a':'X', 'b':'-', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # dXf& (duplicate)
#           {'start':{'a':'X', 'b':'-', 'c':'X', 'f':'X'}, 'end':{'a':'X', 'b':'-', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # dXfX (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'-', 'f':'-'}, 'end':{'a':'X', 'b':'&', 'c':'-', 'f':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&Xf- (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'-', 'f':'&'}, 'end':{'a':'X', 'b':'&', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&Xf& (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'-', 'f':'X'}, 'end':{'a':'X', 'b':'&', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&XfX (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'&', 'f':'-'}, 'end':{'a':'X', 'b':'&', 'c':'&', 'f':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&&Xf- (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'&', 'f':'&'}, 'end':{'a':'X', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&&Xf& (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'&', 'f':'X'}, 'end':{'a':'X', 'b':'&', 'c':'&', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&&XfX (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'X', 'f':'-'}, 'end':{'a':'X', 'b':'&', 'c':'X', 'f':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&XXf- (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'X', 'f':'&'}, 'end':{'a':'X', 'b':'&', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&XXf& (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'X', 'f':'X'}, 'end':{'a':'X', 'b':'&', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&XXfX (duplicate)
#           {'start':{'a':'X', 'b':'X', 'c':'-', 'f':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-', 'f':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # dXXf- (duplicate)
#           {'start':{'a':'X', 'b':'X', 'c':'-', 'f':'&'}, 'end':{'a':'&', 'b':'&', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # dXXf& (duplicate)
#           {'start':{'a':'X', 'b':'X', 'c':'-', 'f':'X'}, 'end':{'a':'X', 'b':'X', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # dXXfX (duplicate)
#           {'start':{'a':'X', 'b':'X', 'c':'&', 'f':'-'}, 'end':{'a':'X', 'b':'X', 'c':'&', 'f':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&XXf- (duplicate)
#           {'start':{'a':'X', 'b':'X', 'c':'&', 'f':'&'}, 'end':{'a':'X', 'b':'X', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&XXf& (duplicate)
#           {'start':{'a':'X', 'b':'X', 'c':'&', 'f':'X'}, 'end':{'a':'X', 'b':'X', 'c':'&', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&XXfX (duplicate)
            {'start':{'a':'X', 'b':'X', 'c':'X', 'f':'-'}, 'end':{'a':'X', 'b':'X', 'c':'X', 'f':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # dXXXf-
            {'start':{'a':'X', 'b':'X', 'c':'X', 'f':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # dXXXf&
            {'start':{'a':'X', 'b':'X', 'c':'X', 'f':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}}   # dXXXfX
        ]

        self.run_command_against_scenarios(['irepl', self.logical_path], scenarios, 'irepl foo')
        self.run_command_against_scenarios(['irepl', self.logical_path], scenarios, 'irepl foo', file_size=0)

    def test_irepl_Rf_foo(self):
        # irepl -R f foo (source unspecified, destination f (standalone resource))
        scenarios = [
            #{'start':{'a':'-', 'b':'-', 'c':'-', 'f':'-'}, 'end':{'a':'-', 'b':'-', 'c':'-', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},              # -
            {'start':{'a':'-', 'b':'-', 'c':'-', 'f':'&'}, 'end':{'a':'-', 'b':'-', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},  # d-f&
            {'start':{'a':'-', 'b':'-', 'c':'-', 'f':'X'}, 'end':{'a':'-', 'b':'-', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d-fX
            {'start':{'a':'-', 'b':'-', 'c':'&', 'f':'-'}, 'end':{'a':'-', 'b':'-', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&f-
            {'start':{'a':'-', 'b':'-', 'c':'&', 'f':'&'}, 'end':{'a':'-', 'b':'-', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&f&
            {'start':{'a':'-', 'b':'-', 'c':'&', 'f':'X'}, 'end':{'a':'-', 'b':'-', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&fX
            {'start':{'a':'-', 'b':'-', 'c':'X', 'f':'-'}, 'end':{'a':'-', 'b':'-', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},               # dXf-
            {'start':{'a':'-', 'b':'-', 'c':'X', 'f':'&'}, 'end':{'a':'-', 'b':'-', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # dXf&
            {'start':{'a':'-', 'b':'-', 'c':'X', 'f':'X'}, 'end':{'a':'-', 'b':'-', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # dXfX
#           {'start':{'a':'-', 'b':'&', 'c':'-', 'f':'-'}, 'end':{'a':'-', 'b':'&', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&f- (duplicate)
#           {'start':{'a':'-', 'b':'&', 'c':'-', 'f':'&'}, 'end':{'a':'-', 'b':'&', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&f& (duplicate)
#           {'start':{'a':'-', 'b':'&', 'c':'-', 'f':'X'}, 'end':{'a':'-', 'b':'&', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&fX (duplicate)
            {'start':{'a':'-', 'b':'&', 'c':'&', 'f':'-'}, 'end':{'a':'-', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&&f-
            {'start':{'a':'-', 'b':'&', 'c':'&', 'f':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&&f&
            {'start':{'a':'-', 'b':'&', 'c':'&', 'f':'X'}, 'end':{'a':'-', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&&fX
            {'start':{'a':'-', 'b':'&', 'c':'X', 'f':'-'}, 'end':{'a':'-', 'b':'&', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&Xf-
            {'start':{'a':'-', 'b':'&', 'c':'X', 'f':'&'}, 'end':{'a':'-', 'b':'&', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&Xf&
            {'start':{'a':'-', 'b':'&', 'c':'X', 'f':'X'}, 'end':{'a':'-', 'b':'&', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&XfX
#           {'start':{'a':'-', 'b':'X', 'c':'-', 'f':'-'}, 'end':{'a':'-', 'b':'X', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},               # dXf- (duplicate)
#           {'start':{'a':'-', 'b':'X', 'c':'-', 'f':'&'}, 'end':{'a':'-', 'b':'X', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # dXf& (duplicate)
#           {'start':{'a':'-', 'b':'X', 'c':'-', 'f':'X'}, 'end':{'a':'-', 'b':'X', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # dXfX (duplicate)
#           {'start':{'a':'-', 'b':'X', 'c':'&', 'f':'-'}, 'end':{'a':'-', 'b':'X', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&Xf- (duplicate)
#           {'start':{'a':'-', 'b':'X', 'c':'&', 'f':'&'}, 'end':{'a':'-', 'b':'X', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&Xf& (duplicate)
#           {'start':{'a':'-', 'b':'X', 'c':'&', 'f':'X'}, 'end':{'a':'-', 'b':'X', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&XfX (duplicate)
            {'start':{'a':'-', 'b':'X', 'c':'X', 'f':'-'}, 'end':{'a':'-', 'b':'X', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},               # dXXf-
            {'start':{'a':'-', 'b':'X', 'c':'X', 'f':'&'}, 'end':{'a':'-', 'b':'X', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # dXXf&
            {'start':{'a':'-', 'b':'X', 'c':'X', 'f':'X'}, 'end':{'a':'-', 'b':'X', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # dXXfX
#           {'start':{'a':'&', 'b':'-', 'c':'-', 'f':'-'}, 'end':{'a':'&', 'b':'-', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&f- (duplicate)
#           {'start':{'a':'&', 'b':'-', 'c':'-', 'f':'&'}, 'end':{'a':'&', 'b':'-', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&f& (duplicate)
#           {'start':{'a':'&', 'b':'-', 'c':'-', 'f':'X'}, 'end':{'a':'&', 'b':'-', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&fX (duplicate)
#           {'start':{'a':'&', 'b':'-', 'c':'&', 'f':'-'}, 'end':{'a':'&', 'b':'-', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&&f- (duplicate)
#           {'start':{'a':'&', 'b':'-', 'c':'&', 'f':'&'}, 'end':{'a':'&', 'b':'-', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&&f& (duplicate)
#           {'start':{'a':'&', 'b':'-', 'c':'&', 'f':'X'}, 'end':{'a':'&', 'b':'-', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&&fX (duplicate)
#           {'start':{'a':'&', 'b':'-', 'c':'X', 'f':'-'}, 'end':{'a':'&', 'b':'-', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&Xf- (duplicate)
#           {'start':{'a':'&', 'b':'-', 'c':'X', 'f':'&'}, 'end':{'a':'&', 'b':'-', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&Xf& (duplicate)
#           {'start':{'a':'&', 'b':'-', 'c':'X', 'f':'X'}, 'end':{'a':'&', 'b':'-', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&XfX (duplicate)
#           {'start':{'a':'&', 'b':'&', 'c':'-', 'f':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&&f- (duplicate)
#           {'start':{'a':'&', 'b':'&', 'c':'-', 'f':'&'}, 'end':{'a':'&', 'b':'&', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&&f& (duplicate)
#           {'start':{'a':'&', 'b':'&', 'c':'-', 'f':'X'}, 'end':{'a':'&', 'b':'&', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&&fX (duplicate)
            {'start':{'a':'&', 'b':'&', 'c':'&', 'f':'-'}, 'end':{'a':'&', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&&&f-
            {'start':{'a':'&', 'b':'&', 'c':'&', 'f':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&&&f&
            {'start':{'a':'&', 'b':'&', 'c':'&', 'f':'X'}, 'end':{'a':'&', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&&&fX
            {'start':{'a':'&', 'b':'&', 'c':'X', 'f':'-'}, 'end':{'a':'&', 'b':'&', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&&Xf-
            {'start':{'a':'&', 'b':'&', 'c':'X', 'f':'&'}, 'end':{'a':'&', 'b':'&', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&&Xf&
            {'start':{'a':'&', 'b':'&', 'c':'X', 'f':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&&XfX
#           {'start':{'a':'&', 'b':'X', 'c':'-', 'f':'-'}, 'end':{'a':'&', 'b':'X', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&Xf- (duplicate)
#           {'start':{'a':'&', 'b':'X', 'c':'-', 'f':'&'}, 'end':{'a':'&', 'b':'X', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&Xf& (duplicate)
#           {'start':{'a':'&', 'b':'X', 'c':'-', 'f':'X'}, 'end':{'a':'&', 'b':'X', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&XfX (duplicate)
#           {'start':{'a':'&', 'b':'X', 'c':'&', 'f':'-'}, 'end':{'a':'&', 'b':'X', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&&Xf- (duplicate)
#           {'start':{'a':'&', 'b':'X', 'c':'&', 'f':'&'}, 'end':{'a':'&', 'b':'X', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&&Xf& (duplicate)
#           {'start':{'a':'&', 'b':'X', 'c':'&', 'f':'X'}, 'end':{'a':'&', 'b':'X', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&&XfX (duplicate)
            {'start':{'a':'&', 'b':'X', 'c':'X', 'f':'-'}, 'end':{'a':'&', 'b':'X', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&XXf-
            {'start':{'a':'&', 'b':'X', 'c':'X', 'f':'&'}, 'end':{'a':'&', 'b':'X', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&XXf&
            {'start':{'a':'&', 'b':'X', 'c':'X', 'f':'X'}, 'end':{'a':'&', 'b':'X', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&XXfX
#           {'start':{'a':'X', 'b':'-', 'c':'-', 'f':'-'}, 'end':{'a':'X', 'b':'-', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},               # dXf- (duplicate)
#           {'start':{'a':'X', 'b':'-', 'c':'-', 'f':'&'}, 'end':{'a':'X', 'b':'-', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # dXf& (duplicate)
#           {'start':{'a':'X', 'b':'-', 'c':'-', 'f':'X'}, 'end':{'a':'X', 'b':'-', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # dXfX (duplicate)
#           {'start':{'a':'X', 'b':'-', 'c':'&', 'f':'-'}, 'end':{'a':'X', 'b':'-', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&Xf- (duplicate)
#           {'start':{'a':'X', 'b':'-', 'c':'&', 'f':'&'}, 'end':{'a':'X', 'b':'-', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&Xf& (duplicate)
#           {'start':{'a':'X', 'b':'-', 'c':'&', 'f':'X'}, 'end':{'a':'X', 'b':'-', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&XfX (duplicate)
#           {'start':{'a':'X', 'b':'-', 'c':'X', 'f':'-'}, 'end':{'a':'X', 'b':'-', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},               # dXf- (duplicate)
#           {'start':{'a':'X', 'b':'-', 'c':'X', 'f':'&'}, 'end':{'a':'X', 'b':'-', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # dXf& (duplicate)
#           {'start':{'a':'X', 'b':'-', 'c':'X', 'f':'X'}, 'end':{'a':'X', 'b':'-', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # dXfX (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'-', 'f':'-'}, 'end':{'a':'X', 'b':'&', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&Xf- (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'-', 'f':'&'}, 'end':{'a':'X', 'b':'&', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&Xf& (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'-', 'f':'X'}, 'end':{'a':'X', 'b':'&', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&XfX (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'&', 'f':'-'}, 'end':{'a':'X', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&&Xf- (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'&', 'f':'&'}, 'end':{'a':'X', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&&Xf& (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'&', 'f':'X'}, 'end':{'a':'X', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&&XfX (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'X', 'f':'-'}, 'end':{'a':'X', 'b':'&', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&XXf- (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'X', 'f':'&'}, 'end':{'a':'X', 'b':'&', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&XXf& (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'X', 'f':'X'}, 'end':{'a':'X', 'b':'&', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&XXfX (duplicate)
#           {'start':{'a':'X', 'b':'X', 'c':'-', 'f':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},               # dXXf- (duplicate)
#           {'start':{'a':'X', 'b':'X', 'c':'-', 'f':'&'}, 'end':{'a':'X', 'b':'X', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # dXXf& (duplicate)
#           {'start':{'a':'X', 'b':'X', 'c':'-', 'f':'X'}, 'end':{'a':'X', 'b':'X', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # dXXfX (duplicate)
#           {'start':{'a':'X', 'b':'X', 'c':'&', 'f':'-'}, 'end':{'a':'X', 'b':'X', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&XXf- (duplicate)
#           {'start':{'a':'X', 'b':'X', 'c':'&', 'f':'&'}, 'end':{'a':'X', 'b':'X', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},  # d&XXf& (duplicate)
#           {'start':{'a':'X', 'b':'X', 'c':'&', 'f':'X'}, 'end':{'a':'X', 'b':'X', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # d&XXfX (duplicate)
            {'start':{'a':'X', 'b':'X', 'c':'X', 'f':'-'}, 'end':{'a':'X', 'b':'X', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},               # dXXXf-
            {'start':{'a':'X', 'b':'X', 'c':'X', 'f':'&'}, 'end':{'a':'X', 'b':'X', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},               # dXXXf&
            {'start':{'a':'X', 'b':'X', 'c':'X', 'f':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}}   # dXXXfX
        ]

        self.run_command_against_scenarios(
            ['irepl', '-R', self.leaf_rescs['f']['name'], self.logical_path],
            scenarios, 'irepl -R f foo')

        self.run_command_against_scenarios(
            ['irepl', '-R', self.leaf_rescs['f']['name'], self.logical_path],
            scenarios, 'irepl -R f foo',
            file_size=0)

    def test_irepl_Sd_Rf_foo(self):
        # irepl -S d -R f foo (source replication hierarchy, destination f (standalone, non-default resource))
        scenarios = [
            #{'start':{'a':'-', 'b':'-', 'c':'-', 'f':'-'}, 'end':{'a':'-', 'b':'-', 'c':'-', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},                       # -
            {'start':{'a':'-', 'b':'-', 'c':'-', 'f':'&'}, 'end':{'a':'-', 'b':'-', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},  # d-f&
            {'start':{'a':'-', 'b':'-', 'c':'-', 'f':'X'}, 'end':{'a':'-', 'b':'-', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},  # d-fX
            {'start':{'a':'-', 'b':'-', 'c':'&', 'f':'-'}, 'end':{'a':'-', 'b':'-', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&f-
            {'start':{'a':'-', 'b':'-', 'c':'&', 'f':'&'}, 'end':{'a':'-', 'b':'-', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&f&
            {'start':{'a':'-', 'b':'-', 'c':'&', 'f':'X'}, 'end':{'a':'-', 'b':'-', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&fX
            {'start':{'a':'-', 'b':'-', 'c':'X', 'f':'-'}, 'end':{'a':'-', 'b':'-', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # dXf-
            {'start':{'a':'-', 'b':'-', 'c':'X', 'f':'&'}, 'end':{'a':'-', 'b':'-', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # dXf&
            {'start':{'a':'-', 'b':'-', 'c':'X', 'f':'X'}, 'end':{'a':'-', 'b':'-', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # dXfX
#           {'start':{'a':'-', 'b':'&', 'c':'-', 'f':'-'}, 'end':{'a':'-', 'b':'&', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&f- (duplicate)
#           {'start':{'a':'-', 'b':'&', 'c':'-', 'f':'&'}, 'end':{'a':'-', 'b':'&', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&f& (duplicate)
#           {'start':{'a':'-', 'b':'&', 'c':'-', 'f':'X'}, 'end':{'a':'-', 'b':'&', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&fX (duplicate)
            {'start':{'a':'-', 'b':'&', 'c':'&', 'f':'-'}, 'end':{'a':'-', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&&f-
            {'start':{'a':'-', 'b':'&', 'c':'&', 'f':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&&f&
            {'start':{'a':'-', 'b':'&', 'c':'&', 'f':'X'}, 'end':{'a':'-', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&&fX
            {'start':{'a':'-', 'b':'&', 'c':'X', 'f':'-'}, 'end':{'a':'-', 'b':'&', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&Xf-
            {'start':{'a':'-', 'b':'&', 'c':'X', 'f':'&'}, 'end':{'a':'-', 'b':'&', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&Xf&
            {'start':{'a':'-', 'b':'&', 'c':'X', 'f':'X'}, 'end':{'a':'-', 'b':'&', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&XfX
#           {'start':{'a':'-', 'b':'X', 'c':'-', 'f':'-'}, 'end':{'a':'-', 'b':'X', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # dXf- (duplicate)
#           {'start':{'a':'-', 'b':'X', 'c':'-', 'f':'&'}, 'end':{'a':'-', 'b':'X', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # dXf& (duplicate)
#           {'start':{'a':'-', 'b':'X', 'c':'-', 'f':'X'}, 'end':{'a':'-', 'b':'X', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # dXfX (duplicate)
#           {'start':{'a':'-', 'b':'X', 'c':'&', 'f':'-'}, 'end':{'a':'-', 'b':'X', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&Xf- (duplicate)
#           {'start':{'a':'-', 'b':'X', 'c':'&', 'f':'&'}, 'end':{'a':'-', 'b':'X', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&Xf& (duplicate)
#           {'start':{'a':'-', 'b':'X', 'c':'&', 'f':'X'}, 'end':{'a':'-', 'b':'X', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&XfX (duplicate)
            {'start':{'a':'-', 'b':'X', 'c':'X', 'f':'-'}, 'end':{'a':'-', 'b':'X', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # dXXf-
            {'start':{'a':'-', 'b':'X', 'c':'X', 'f':'&'}, 'end':{'a':'-', 'b':'X', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # dXXf&
            {'start':{'a':'-', 'b':'X', 'c':'X', 'f':'X'}, 'end':{'a':'-', 'b':'X', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # dXXfX
#           {'start':{'a':'&', 'b':'-', 'c':'-', 'f':'-'}, 'end':{'a':'&', 'b':'-', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&f- (duplicate)
#           {'start':{'a':'&', 'b':'-', 'c':'-', 'f':'&'}, 'end':{'a':'&', 'b':'-', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&f& (duplicate)
#           {'start':{'a':'&', 'b':'-', 'c':'-', 'f':'X'}, 'end':{'a':'&', 'b':'-', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&fX (duplicate)
#           {'start':{'a':'&', 'b':'-', 'c':'&', 'f':'-'}, 'end':{'a':'&', 'b':'-', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&&f- (duplicate)
#           {'start':{'a':'&', 'b':'-', 'c':'&', 'f':'&'}, 'end':{'a':'&', 'b':'-', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&&f& (duplicate)
#           {'start':{'a':'&', 'b':'-', 'c':'&', 'f':'X'}, 'end':{'a':'&', 'b':'-', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&&fX (duplicate)
#           {'start':{'a':'&', 'b':'-', 'c':'X', 'f':'-'}, 'end':{'a':'&', 'b':'-', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&Xf- (duplicate)
#           {'start':{'a':'&', 'b':'-', 'c':'X', 'f':'&'}, 'end':{'a':'&', 'b':'-', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&Xf& (duplicate)
#           {'start':{'a':'&', 'b':'-', 'c':'X', 'f':'X'}, 'end':{'a':'&', 'b':'-', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&XfX (duplicate)
#           {'start':{'a':'&', 'b':'&', 'c':'-', 'f':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&&f- (duplicate)
#           {'start':{'a':'&', 'b':'&', 'c':'-', 'f':'&'}, 'end':{'a':'&', 'b':'&', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&&f& (duplicate)
#           {'start':{'a':'&', 'b':'&', 'c':'-', 'f':'X'}, 'end':{'a':'&', 'b':'&', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&&fX (duplicate)
            {'start':{'a':'&', 'b':'&', 'c':'&', 'f':'-'}, 'end':{'a':'&', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&&&f-
            {'start':{'a':'&', 'b':'&', 'c':'&', 'f':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&&&f&
            {'start':{'a':'&', 'b':'&', 'c':'&', 'f':'X'}, 'end':{'a':'&', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&&&fX
            {'start':{'a':'&', 'b':'&', 'c':'X', 'f':'-'}, 'end':{'a':'&', 'b':'&', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&&Xf-
            {'start':{'a':'&', 'b':'&', 'c':'X', 'f':'&'}, 'end':{'a':'&', 'b':'&', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&&Xf&
            {'start':{'a':'&', 'b':'&', 'c':'X', 'f':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&&XfX
#           {'start':{'a':'&', 'b':'X', 'c':'-', 'f':'-'}, 'end':{'a':'&', 'b':'X', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&Xf- (duplicate)
#           {'start':{'a':'&', 'b':'X', 'c':'-', 'f':'&'}, 'end':{'a':'&', 'b':'X', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&Xf& (duplicate)
#           {'start':{'a':'&', 'b':'X', 'c':'-', 'f':'X'}, 'end':{'a':'&', 'b':'X', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&XfX (duplicate)
#           {'start':{'a':'&', 'b':'X', 'c':'&', 'f':'-'}, 'end':{'a':'&', 'b':'X', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&&Xf- (duplicate)
#           {'start':{'a':'&', 'b':'X', 'c':'&', 'f':'&'}, 'end':{'a':'&', 'b':'X', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&&Xf& (duplicate)
#           {'start':{'a':'&', 'b':'X', 'c':'&', 'f':'X'}, 'end':{'a':'&', 'b':'X', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&&XfX (duplicate)
            {'start':{'a':'&', 'b':'X', 'c':'X', 'f':'-'}, 'end':{'a':'&', 'b':'X', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&XXf-
            {'start':{'a':'&', 'b':'X', 'c':'X', 'f':'&'}, 'end':{'a':'&', 'b':'X', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&XXf&
            {'start':{'a':'&', 'b':'X', 'c':'X', 'f':'X'}, 'end':{'a':'&', 'b':'X', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&XXfX
#           {'start':{'a':'X', 'b':'-', 'c':'-', 'f':'-'}, 'end':{'a':'X', 'b':'-', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # dXf- (duplicate)
#           {'start':{'a':'X', 'b':'-', 'c':'-', 'f':'&'}, 'end':{'a':'X', 'b':'-', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # dXf& (duplicate)
#           {'start':{'a':'X', 'b':'-', 'c':'-', 'f':'X'}, 'end':{'a':'X', 'b':'-', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # dXfX (duplicate)
#           {'start':{'a':'X', 'b':'-', 'c':'&', 'f':'-'}, 'end':{'a':'X', 'b':'-', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&Xf- (duplicate)
#           {'start':{'a':'X', 'b':'-', 'c':'&', 'f':'&'}, 'end':{'a':'X', 'b':'-', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&Xf& (duplicate)
#           {'start':{'a':'X', 'b':'-', 'c':'&', 'f':'X'}, 'end':{'a':'X', 'b':'-', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&XfX (duplicate)
#           {'start':{'a':'X', 'b':'-', 'c':'X', 'f':'-'}, 'end':{'a':'X', 'b':'-', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # dXf- (duplicate)
#           {'start':{'a':'X', 'b':'-', 'c':'X', 'f':'&'}, 'end':{'a':'X', 'b':'-', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # dXf& (duplicate)
#           {'start':{'a':'X', 'b':'-', 'c':'X', 'f':'X'}, 'end':{'a':'X', 'b':'-', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # dXfX (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'-', 'f':'-'}, 'end':{'a':'X', 'b':'&', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&Xf- (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'-', 'f':'&'}, 'end':{'a':'X', 'b':'&', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&Xf& (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'-', 'f':'X'}, 'end':{'a':'X', 'b':'&', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&XfX (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'&', 'f':'-'}, 'end':{'a':'X', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&&Xf- (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'&', 'f':'&'}, 'end':{'a':'X', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&&Xf& (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'&', 'f':'X'}, 'end':{'a':'X', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&&XfX (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'X', 'f':'-'}, 'end':{'a':'X', 'b':'&', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&XXf- (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'X', 'f':'&'}, 'end':{'a':'X', 'b':'&', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&XXf& (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'X', 'f':'X'}, 'end':{'a':'X', 'b':'&', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&XXfX (duplicate)
#           {'start':{'a':'X', 'b':'X', 'c':'-', 'f':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # dXXf- (duplicate)
#           {'start':{'a':'X', 'b':'X', 'c':'-', 'f':'&'}, 'end':{'a':'X', 'b':'X', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # dXXf& (duplicate)
#           {'start':{'a':'X', 'b':'X', 'c':'-', 'f':'X'}, 'end':{'a':'X', 'b':'X', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # dXXfX (duplicate)
#           {'start':{'a':'X', 'b':'X', 'c':'&', 'f':'-'}, 'end':{'a':'X', 'b':'X', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&XXf- (duplicate)
#           {'start':{'a':'X', 'b':'X', 'c':'&', 'f':'&'}, 'end':{'a':'X', 'b':'X', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&XXf& (duplicate)
#           {'start':{'a':'X', 'b':'X', 'c':'&', 'f':'X'}, 'end':{'a':'X', 'b':'X', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&XXfX (duplicate)
            {'start':{'a':'X', 'b':'X', 'c':'X', 'f':'-'}, 'end':{'a':'X', 'b':'X', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # dXXXf-
            {'start':{'a':'X', 'b':'X', 'c':'X', 'f':'&'}, 'end':{'a':'X', 'b':'X', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # dXXXf&
            {'start':{'a':'X', 'b':'X', 'c':'X', 'f':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}}            # dXXXfX
        ]

        self.run_command_against_scenarios(
            ['irepl', '-S', self.parent_rescs['d'], '-R', self.leaf_rescs['f']['name'], self.logical_path],
            scenarios, 'irepl -S d -R f foo')

        self.run_command_against_scenarios(
            ['irepl', '-S', self.parent_rescs['d'], '-R', self.leaf_rescs['f']['name'], self.logical_path],
            scenarios, 'irepl -S d -R f foo', file_size=0)

    def test_irepl_Sf_Rd_foo(self):
        # irepl -S f -R d foo (irepl -S f -R d foo) (source standalone resource, target default resource (root of replication hierarchy))
        scenarios = [
            #{'start':{'a':'-', 'b':'-', 'c':'-', 'f':'-'}, 'end':{'a':'-', 'b':'-', 'c':'-', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},                       # -
            {'start':{'a':'-', 'b':'-', 'c':'-', 'f':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d-f&
            {'start':{'a':'-', 'b':'-', 'c':'-', 'f':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d-fX
            {'start':{'a':'-', 'b':'-', 'c':'&', 'f':'-'}, 'end':{'a':'-', 'b':'-', 'c':'&', 'f':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},  # d&f-
            {'start':{'a':'-', 'b':'-', 'c':'&', 'f':'&'}, 'end':{'a':'-', 'b':'-', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&f&
            {'start':{'a':'-', 'b':'-', 'c':'&', 'f':'X'}, 'end':{'a':'-', 'b':'-', 'c':'&', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # d&fX
            {'start':{'a':'-', 'b':'-', 'c':'X', 'f':'-'}, 'end':{'a':'-', 'b':'-', 'c':'X', 'f':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},  # dXf-
            {'start':{'a':'-', 'b':'-', 'c':'X', 'f':'&'}, 'end':{'a':'-', 'b':'-', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # dXf&
            {'start':{'a':'-', 'b':'-', 'c':'X', 'f':'X'}, 'end':{'a':'-', 'b':'-', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # dXfX
#           {'start':{'a':'-', 'b':'&', 'c':'-', 'f':'-'}, 'end':{'a':'-', 'b':'&', 'c':'-', 'f':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},  # d&f- (duplicate)
#           {'start':{'a':'-', 'b':'&', 'c':'-', 'f':'&'}, 'end':{'a':'-', 'b':'&', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&f& (duplicate)
#           {'start':{'a':'-', 'b':'&', 'c':'-', 'f':'X'}, 'end':{'a':'-', 'b':'&', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # d&fX (duplicate)
            {'start':{'a':'-', 'b':'&', 'c':'&', 'f':'-'}, 'end':{'a':'-', 'b':'&', 'c':'&', 'f':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},  # d&&f-
            {'start':{'a':'-', 'b':'&', 'c':'&', 'f':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&&f&
            {'start':{'a':'-', 'b':'&', 'c':'&', 'f':'X'}, 'end':{'a':'-', 'b':'&', 'c':'&', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # d&&fX
            {'start':{'a':'-', 'b':'&', 'c':'X', 'f':'-'}, 'end':{'a':'-', 'b':'&', 'c':'X', 'f':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},  # d&Xf-
            {'start':{'a':'-', 'b':'&', 'c':'X', 'f':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&Xf&
            {'start':{'a':'-', 'b':'&', 'c':'X', 'f':'X'}, 'end':{'a':'-', 'b':'&', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # d&XfX
#           {'start':{'a':'-', 'b':'X', 'c':'-', 'f':'-'}, 'end':{'a':'-', 'b':'X', 'c':'-', 'f':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},  # dXf- (duplicate)
#           {'start':{'a':'-', 'b':'X', 'c':'-', 'f':'&'}, 'end':{'a':'-', 'b':'&', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # dXf& (duplicate)
#           {'start':{'a':'-', 'b':'X', 'c':'-', 'f':'X'}, 'end':{'a':'-', 'b':'X', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # dXfX (duplicate)
#           {'start':{'a':'-', 'b':'X', 'c':'&', 'f':'-'}, 'end':{'a':'-', 'b':'X', 'c':'&', 'f':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},  # d&Xf- (duplicate)
#           {'start':{'a':'-', 'b':'X', 'c':'&', 'f':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&Xf& (duplicate)
#           {'start':{'a':'-', 'b':'X', 'c':'&', 'f':'X'}, 'end':{'a':'-', 'b':'X', 'c':'&', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # d&XfX (duplicate)
            {'start':{'a':'-', 'b':'X', 'c':'X', 'f':'-'}, 'end':{'a':'-', 'b':'X', 'c':'X', 'f':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},  # dXXf-
            {'start':{'a':'-', 'b':'X', 'c':'X', 'f':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # dXXf&
            {'start':{'a':'-', 'b':'X', 'c':'X', 'f':'X'}, 'end':{'a':'-', 'b':'X', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # dXXfX
#           {'start':{'a':'&', 'b':'-', 'c':'-', 'f':'-'}, 'end':{'a':'&', 'b':'-', 'c':'-', 'f':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},  # d&f- (duplicate)
#           {'start':{'a':'&', 'b':'-', 'c':'-', 'f':'&'}, 'end':{'a':'&', 'b':'-', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&f& (duplicate)
#           {'start':{'a':'&', 'b':'-', 'c':'-', 'f':'X'}, 'end':{'a':'&', 'b':'-', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # d&fX (duplicate)
#           {'start':{'a':'&', 'b':'-', 'c':'&', 'f':'-'}, 'end':{'a':'&', 'b':'-', 'c':'&', 'f':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},  # d&&f- (duplicate)
#           {'start':{'a':'&', 'b':'-', 'c':'&', 'f':'&'}, 'end':{'a':'&', 'b':'-', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&&f& (duplicate)
#           {'start':{'a':'&', 'b':'-', 'c':'&', 'f':'X'}, 'end':{'a':'&', 'b':'-', 'c':'&', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # d&&fX (duplicate)
#           {'start':{'a':'&', 'b':'-', 'c':'X', 'f':'-'}, 'end':{'a':'&', 'b':'-', 'c':'X', 'f':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},  # d&Xf- (duplicate)
#           {'start':{'a':'&', 'b':'-', 'c':'X', 'f':'&'}, 'end':{'a':'&', 'b':'-', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&Xf& (duplicate)
#           {'start':{'a':'&', 'b':'-', 'c':'X', 'f':'X'}, 'end':{'a':'&', 'b':'-', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # d&XfX (duplicate)
#           {'start':{'a':'&', 'b':'&', 'c':'-', 'f':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-', 'f':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},  # d&&f- (duplicate)
#           {'start':{'a':'&', 'b':'&', 'c':'-', 'f':'&'}, 'end':{'a':'&', 'b':'&', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&&f& (duplicate)
#           {'start':{'a':'&', 'b':'&', 'c':'-', 'f':'X'}, 'end':{'a':'&', 'b':'&', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # d&&fX (duplicate)
            {'start':{'a':'&', 'b':'&', 'c':'&', 'f':'-'}, 'end':{'a':'&', 'b':'&', 'c':'&', 'f':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},  # d&&&f-
            {'start':{'a':'&', 'b':'&', 'c':'&', 'f':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&&&f&
            {'start':{'a':'&', 'b':'&', 'c':'&', 'f':'X'}, 'end':{'a':'&', 'b':'&', 'c':'&', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # d&&&fX
            {'start':{'a':'&', 'b':'&', 'c':'X', 'f':'-'}, 'end':{'a':'&', 'b':'&', 'c':'X', 'f':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},  # d&&Xf-
            {'start':{'a':'&', 'b':'&', 'c':'X', 'f':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&&Xf&
            {'start':{'a':'&', 'b':'&', 'c':'X', 'f':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # d&&XfX
#           {'start':{'a':'&', 'b':'X', 'c':'-', 'f':'-'}, 'end':{'a':'&', 'b':'X', 'c':'-', 'f':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},  # d&Xf- (duplicate)
#           {'start':{'a':'&', 'b':'X', 'c':'-', 'f':'&'}, 'end':{'a':'&', 'b':'&', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&Xf& (duplicate)
#           {'start':{'a':'&', 'b':'X', 'c':'-', 'f':'X'}, 'end':{'a':'&', 'b':'X', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # d&XfX (duplicate)
#           {'start':{'a':'&', 'b':'X', 'c':'&', 'f':'-'}, 'end':{'a':'&', 'b':'X', 'c':'&', 'f':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},  # d&&Xf- (duplicate)
#           {'start':{'a':'&', 'b':'X', 'c':'&', 'f':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&&Xf& (duplicate)
#           {'start':{'a':'&', 'b':'X', 'c':'&', 'f':'X'}, 'end':{'a':'&', 'b':'X', 'c':'&', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # d&&XfX (duplicate)
            {'start':{'a':'&', 'b':'X', 'c':'X', 'f':'-'}, 'end':{'a':'&', 'b':'X', 'c':'X', 'f':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},  # d&XXf-
            {'start':{'a':'&', 'b':'X', 'c':'X', 'f':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&XXf&
            {'start':{'a':'&', 'b':'X', 'c':'X', 'f':'X'}, 'end':{'a':'&', 'b':'X', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # d&XXfX
            {'start':{'a':'X', 'b':'-', 'c':'-', 'f':'-'}, 'end':{'a':'X', 'b':'-', 'c':'-', 'f':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},  # dXf- (duplicate)
#           {'start':{'a':'X', 'b':'-', 'c':'-', 'f':'&'}, 'end':{'a':'&', 'b':'-', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # dXf& (duplicate)
#           {'start':{'a':'X', 'b':'-', 'c':'-', 'f':'X'}, 'end':{'a':'X', 'b':'-', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # dXfX (duplicate)
#           {'start':{'a':'X', 'b':'-', 'c':'&', 'f':'-'}, 'end':{'a':'X', 'b':'-', 'c':'&', 'f':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},  # d&Xf- (duplicate)
#           {'start':{'a':'X', 'b':'-', 'c':'&', 'f':'&'}, 'end':{'a':'&', 'b':'-', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&Xf& (duplicate)
#           {'start':{'a':'X', 'b':'-', 'c':'&', 'f':'X'}, 'end':{'a':'X', 'b':'-', 'c':'&', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # d&XfX (duplicate)
#           {'start':{'a':'X', 'b':'-', 'c':'X', 'f':'-'}, 'end':{'a':'X', 'b':'-', 'c':'X', 'f':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},  # dXf- (duplicate)
#           {'start':{'a':'X', 'b':'-', 'c':'X', 'f':'&'}, 'end':{'a':'&', 'b':'-', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # dXf& (duplicate)
#           {'start':{'a':'X', 'b':'-', 'c':'X', 'f':'X'}, 'end':{'a':'X', 'b':'-', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # dXfX (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'-', 'f':'-'}, 'end':{'a':'X', 'b':'&', 'c':'-', 'f':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},  # d&Xf- (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'-', 'f':'&'}, 'end':{'a':'&', 'b':'&', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&Xf& (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'-', 'f':'X'}, 'end':{'a':'X', 'b':'&', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # d&XfX (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'&', 'f':'-'}, 'end':{'a':'X', 'b':'&', 'c':'&', 'f':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},  # d&&Xf- (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'&', 'f':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&&Xf& (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'&', 'f':'X'}, 'end':{'a':'X', 'b':'&', 'c':'&', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # d&&XfX (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'X', 'f':'-'}, 'end':{'a':'X', 'b':'&', 'c':'X', 'f':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},  # d&XXf- (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'X', 'f':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&XXf& (duplicate)
#           {'start':{'a':'X', 'b':'&', 'c':'X', 'f':'X'}, 'end':{'a':'X', 'b':'&', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # d&XXfX (duplicate)
#           {'start':{'a':'X', 'b':'X', 'c':'-', 'f':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-', 'f':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},  # dXXf- (duplicate)
#           {'start':{'a':'X', 'b':'X', 'c':'-', 'f':'&'}, 'end':{'a':'&', 'b':'&', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # dXXf& (duplicate)
#           {'start':{'a':'X', 'b':'X', 'c':'-', 'f':'X'}, 'end':{'a':'X', 'b':'X', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # dXXfX (duplicate)
#           {'start':{'a':'X', 'b':'X', 'c':'&', 'f':'-'}, 'end':{'a':'X', 'b':'X', 'c':'&', 'f':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},  # d&XXf- (duplicate)
#           {'start':{'a':'X', 'b':'X', 'c':'&', 'f':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # d&XXf& (duplicate)
#           {'start':{'a':'X', 'b':'X', 'c':'&', 'f':'X'}, 'end':{'a':'X', 'b':'X', 'c':'&', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},           # d&XXfX (duplicate)
            {'start':{'a':'X', 'b':'X', 'c':'X', 'f':'-'}, 'end':{'a':'X', 'b':'X', 'c':'X', 'f':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_INACCESSIBLE', 'rc':None}},  # dXXXf-
            {'start':{'a':'X', 'b':'X', 'c':'X', 'f':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},                        # dXXXf&
            {'start':{'a':'X', 'b':'X', 'c':'X', 'f':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}}            # dXXXfX
        ]

        self.run_command_against_scenarios(
            ['irepl', '-S', self.leaf_rescs['f']['name'], '-R', self.parent_rescs['d'], self.logical_path],
            scenarios, 'irepl -S f -R d foo')

        self.run_command_against_scenarios(
            ['irepl', '-S', self.leaf_rescs['f']['name'], '-R', self.parent_rescs['d'], self.logical_path],
            scenarios, 'irepl -S f -R d foo', file_size=0)


           #{'start':{'a':'-', 'b':'-', 'c':'-', 'f':'-'}, 'end':{'a':'-', 'b':'-', 'c':'-', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # -
           #{'start':{'a':'-', 'b':'-', 'c':'-', 'f':'&'}, 'end':{'a':'-', 'b':'-', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d-f&
           #{'start':{'a':'-', 'b':'-', 'c':'-', 'f':'X'}, 'end':{'a':'-', 'b':'-', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d-fX
           #{'start':{'a':'-', 'b':'-', 'c':'&', 'f':'-'}, 'end':{'a':'-', 'b':'-', 'c':'&', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&f-
           #{'start':{'a':'-', 'b':'-', 'c':'&', 'f':'&'}, 'end':{'a':'-', 'b':'-', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&f&
           #{'start':{'a':'-', 'b':'-', 'c':'&', 'f':'X'}, 'end':{'a':'-', 'b':'-', 'c':'&', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&fX
           #{'start':{'a':'-', 'b':'-', 'c':'X', 'f':'-'}, 'end':{'a':'-', 'b':'-', 'c':'X', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # dXf-
           #{'start':{'a':'-', 'b':'-', 'c':'X', 'f':'&'}, 'end':{'a':'-', 'b':'-', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # dXf&
           #{'start':{'a':'-', 'b':'-', 'c':'X', 'f':'X'}, 'end':{'a':'-', 'b':'-', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # dXfX
           #{'start':{'a':'-', 'b':'&', 'c':'-', 'f':'-'}, 'end':{'a':'-', 'b':'&', 'c':'-', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&f- (duplicate)
           #{'start':{'a':'-', 'b':'&', 'c':'-', 'f':'&'}, 'end':{'a':'-', 'b':'&', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&f& (duplicate)
           #{'start':{'a':'-', 'b':'&', 'c':'-', 'f':'X'}, 'end':{'a':'-', 'b':'&', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&fX (duplicate)
           #{'start':{'a':'-', 'b':'&', 'c':'&', 'f':'-'}, 'end':{'a':'-', 'b':'&', 'c':'&', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&&f-
           #{'start':{'a':'-', 'b':'&', 'c':'&', 'f':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&&f&
           #{'start':{'a':'-', 'b':'&', 'c':'&', 'f':'X'}, 'end':{'a':'-', 'b':'&', 'c':'&', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&&fX
           #{'start':{'a':'-', 'b':'&', 'c':'X', 'f':'-'}, 'end':{'a':'-', 'b':'&', 'c':'X', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&Xf-
           #{'start':{'a':'-', 'b':'&', 'c':'X', 'f':'&'}, 'end':{'a':'-', 'b':'&', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&Xf& - non-deterministic due to lack of specified source resource
           #{'start':{'a':'-', 'b':'&', 'c':'X', 'f':'X'}, 'end':{'a':'-', 'b':'&', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&XfX
           #{'start':{'a':'-', 'b':'X', 'c':'-', 'f':'-'}, 'end':{'a':'-', 'b':'X', 'c':'-', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # dXf- (duplicate)
           #{'start':{'a':'-', 'b':'X', 'c':'-', 'f':'&'}, 'end':{'a':'-', 'b':'X', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # dXf& (duplicate)
           #{'start':{'a':'-', 'b':'X', 'c':'-', 'f':'X'}, 'end':{'a':'-', 'b':'X', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # dXfX (duplicate)
           #{'start':{'a':'-', 'b':'X', 'c':'&', 'f':'-'}, 'end':{'a':'-', 'b':'X', 'c':'&', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&Xf- (duplicate)
           #{'start':{'a':'-', 'b':'X', 'c':'&', 'f':'&'}, 'end':{'a':'-', 'b':'X', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&Xf& (duplicate) - non-deterministic due to lack of specified source resource
           #{'start':{'a':'-', 'b':'X', 'c':'&', 'f':'X'}, 'end':{'a':'-', 'b':'X', 'c':'&', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&XfX (duplicate)
           #{'start':{'a':'-', 'b':'X', 'c':'X', 'f':'-'}, 'end':{'a':'-', 'b':'X', 'c':'X', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # dXXf-
           #{'start':{'a':'-', 'b':'X', 'c':'X', 'f':'&'}, 'end':{'a':'-', 'b':'X', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # dXXf&
           #{'start':{'a':'-', 'b':'X', 'c':'X', 'f':'X'}, 'end':{'a':'-', 'b':'X', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # dXXfX
           #{'start':{'a':'&', 'b':'-', 'c':'-', 'f':'-'}, 'end':{'a':'&', 'b':'-', 'c':'-', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&f- (duplicate)
           #{'start':{'a':'&', 'b':'-', 'c':'-', 'f':'&'}, 'end':{'a':'&', 'b':'-', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&f& (duplicate)
           #{'start':{'a':'&', 'b':'-', 'c':'-', 'f':'X'}, 'end':{'a':'&', 'b':'-', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&fX (duplicate)
           #{'start':{'a':'&', 'b':'-', 'c':'&', 'f':'-'}, 'end':{'a':'&', 'b':'-', 'c':'&', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&&f- (duplicate)
           #{'start':{'a':'&', 'b':'-', 'c':'&', 'f':'&'}, 'end':{'a':'&', 'b':'-', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&&f& (duplicate)
           #{'start':{'a':'&', 'b':'-', 'c':'&', 'f':'X'}, 'end':{'a':'&', 'b':'-', 'c':'&', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&&fX (duplicate)
           #{'start':{'a':'&', 'b':'-', 'c':'X', 'f':'-'}, 'end':{'a':'&', 'b':'-', 'c':'X', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&Xf- (duplicate)
           #{'start':{'a':'&', 'b':'-', 'c':'X', 'f':'&'}, 'end':{'a':'&', 'b':'-', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&Xf& (duplicate) - non-deterministic due to lack of specified source resource
           #{'start':{'a':'&', 'b':'-', 'c':'X', 'f':'X'}, 'end':{'a':'&', 'b':'-', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&XfX (duplicate)
           #{'start':{'a':'&', 'b':'&', 'c':'-', 'f':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&&f- (duplicate)
           #{'start':{'a':'&', 'b':'&', 'c':'-', 'f':'&'}, 'end':{'a':'&', 'b':'&', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&&f& (duplicate)
           #{'start':{'a':'&', 'b':'&', 'c':'-', 'f':'X'}, 'end':{'a':'&', 'b':'&', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&&fX (duplicate)
           #{'start':{'a':'&', 'b':'&', 'c':'&', 'f':'-'}, 'end':{'a':'&', 'b':'&', 'c':'&', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&&&f-
           #{'start':{'a':'&', 'b':'&', 'c':'&', 'f':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&&&f&
           #{'start':{'a':'&', 'b':'&', 'c':'&', 'f':'X'}, 'end':{'a':'&', 'b':'&', 'c':'&', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&&&fX
           #{'start':{'a':'&', 'b':'&', 'c':'X', 'f':'-'}, 'end':{'a':'&', 'b':'&', 'c':'X', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&&Xf-
           #{'start':{'a':'&', 'b':'&', 'c':'X', 'f':'&'}, 'end':{'a':'&', 'b':'&', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&&Xf& - non-deterministic due to lack of specified source resource
           #{'start':{'a':'&', 'b':'&', 'c':'X', 'f':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&&XfX
           #{'start':{'a':'&', 'b':'X', 'c':'-', 'f':'-'}, 'end':{'a':'&', 'b':'X', 'c':'-', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&Xf- (duplicate)
           #{'start':{'a':'&', 'b':'X', 'c':'-', 'f':'&'}, 'end':{'a':'&', 'b':'X', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&Xf& (duplicate) - non-deterministic due to lack of specified source resource
           #{'start':{'a':'&', 'b':'X', 'c':'-', 'f':'X'}, 'end':{'a':'&', 'b':'X', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&XfX (duplicate)
           #{'start':{'a':'&', 'b':'X', 'c':'&', 'f':'-'}, 'end':{'a':'&', 'b':'X', 'c':'&', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&&Xf- (duplicate)
           #{'start':{'a':'&', 'b':'X', 'c':'&', 'f':'&'}, 'end':{'a':'&', 'b':'X', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&&Xf& (duplicate)
           #{'start':{'a':'&', 'b':'X', 'c':'&', 'f':'X'}, 'end':{'a':'&', 'b':'X', 'c':'&', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&&XfX (duplicate)
           #{'start':{'a':'&', 'b':'X', 'c':'X', 'f':'-'}, 'end':{'a':'&', 'b':'X', 'c':'X', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&XXf-
           #{'start':{'a':'&', 'b':'X', 'c':'X', 'f':'&'}, 'end':{'a':'&', 'b':'X', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&XXf& - non-deterministic due to lack of specified source resource
           #{'start':{'a':'&', 'b':'X', 'c':'X', 'f':'X'}, 'end':{'a':'&', 'b':'X', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&XXfX
           #{'start':{'a':'X', 'b':'-', 'c':'-', 'f':'-'}, 'end':{'a':'X', 'b':'-', 'c':'-', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # dXf- (duplicate)
           #{'start':{'a':'X', 'b':'-', 'c':'-', 'f':'&'}, 'end':{'a':'X', 'b':'-', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # dXf& (duplicate)
           #{'start':{'a':'X', 'b':'-', 'c':'-', 'f':'X'}, 'end':{'a':'X', 'b':'-', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # dXfX (duplicate)
           #{'start':{'a':'X', 'b':'-', 'c':'&', 'f':'-'}, 'end':{'a':'X', 'b':'-', 'c':'&', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&Xf- (duplicate)
           #{'start':{'a':'X', 'b':'-', 'c':'&', 'f':'&'}, 'end':{'a':'X', 'b':'-', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&Xf& (duplicate) - non-deterministic due to lack of specified source resource
           #{'start':{'a':'X', 'b':'-', 'c':'&', 'f':'X'}, 'end':{'a':'X', 'b':'-', 'c':'&', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&XfX (duplicate)
           #{'start':{'a':'X', 'b':'-', 'c':'X', 'f':'-'}, 'end':{'a':'X', 'b':'-', 'c':'X', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # dXf- (duplicate)
           #{'start':{'a':'X', 'b':'-', 'c':'X', 'f':'&'}, 'end':{'a':'X', 'b':'-', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # dXf& (duplicate)
           #{'start':{'a':'X', 'b':'-', 'c':'X', 'f':'X'}, 'end':{'a':'X', 'b':'-', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # dXfX (duplicate)
           #{'start':{'a':'X', 'b':'&', 'c':'-', 'f':'-'}, 'end':{'a':'X', 'b':'&', 'c':'-', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&Xf- (duplicate)
           #{'start':{'a':'X', 'b':'&', 'c':'-', 'f':'&'}, 'end':{'a':'X', 'b':'&', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&Xf& (duplicate) - non-deterministic due to lack of specified source resource
           #{'start':{'a':'X', 'b':'&', 'c':'-', 'f':'X'}, 'end':{'a':'X', 'b':'&', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&XfX (duplicate)
           #{'start':{'a':'X', 'b':'&', 'c':'&', 'f':'-'}, 'end':{'a':'X', 'b':'&', 'c':'&', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&&Xf- (duplicate)
           #{'start':{'a':'X', 'b':'&', 'c':'&', 'f':'&'}, 'end':{'a':'X', 'b':'&', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&&Xf& (duplicate)
           #{'start':{'a':'X', 'b':'&', 'c':'&', 'f':'X'}, 'end':{'a':'X', 'b':'&', 'c':'&', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&&XfX (duplicate)
           #{'start':{'a':'X', 'b':'&', 'c':'X', 'f':'-'}, 'end':{'a':'X', 'b':'&', 'c':'X', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&XXf- (duplicate)
           #{'start':{'a':'X', 'b':'&', 'c':'X', 'f':'&'}, 'end':{'a':'X', 'b':'&', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&XXf& (duplicate)
           #{'start':{'a':'X', 'b':'&', 'c':'X', 'f':'X'}, 'end':{'a':'X', 'b':'&', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&XXfX (duplicate)
           #{'start':{'a':'X', 'b':'X', 'c':'-', 'f':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # dXXf- (duplicate)
           #{'start':{'a':'X', 'b':'X', 'c':'-', 'f':'&'}, 'end':{'a':'X', 'b':'X', 'c':'-', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # dXXf& (duplicate)
           #{'start':{'a':'X', 'b':'X', 'c':'-', 'f':'X'}, 'end':{'a':'X', 'b':'X', 'c':'-', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # dXXfX (duplicate) - non-deterministic due to lack of specified source resource
           #{'start':{'a':'X', 'b':'X', 'c':'&', 'f':'-'}, 'end':{'a':'X', 'b':'X', 'c':'&', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&XXf- (duplicate)
           #{'start':{'a':'X', 'b':'X', 'c':'&', 'f':'&'}, 'end':{'a':'X', 'b':'X', 'c':'&', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&XXf& (duplicate)
           #{'start':{'a':'X', 'b':'X', 'c':'&', 'f':'X'}, 'end':{'a':'X', 'b':'X', 'c':'&', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # d&XXfX (duplicate)
           #{'start':{'a':'X', 'b':'X', 'c':'X', 'f':'-'}, 'end':{'a':'X', 'b':'X', 'c':'X', 'f':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # dXXXf-
           #{'start':{'a':'X', 'b':'X', 'c':'X', 'f':'&'}, 'end':{'a':'X', 'b':'X', 'c':'X', 'f':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # dXXXf&
           #{'start':{'a':'X', 'b':'X', 'c':'X', 'f':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X', 'f':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}  # dXXXfX


class test_all_permission_levels__issue_7444_7465_7816(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.user0 = session.mkuser_and_return_session('rodsuser', 'smeagol', 'spass', lib.get_hostname())
        self.user1 = session.mkuser_and_return_session('rodsuser', 'bilbo', 'bpass', lib.get_hostname())

        # Give other user ownership of the session collection so we can focus on object permissions.
        self.user0.assert_icommand(['ichmod', 'own', self.user1.username, self.user0.session_collection])

        self.target_resource = 'target_resource'
        self.other_resource = 'other_resource'

        with session.make_session_for_existing_admin() as admin_session:
            lib.create_ufs_resource(admin_session, self.target_resource, hostname=test.settings.HOSTNAME_2)
            lib.create_ufs_resource(admin_session, self.other_resource, hostname=test.settings.HOSTNAME_3)

    @classmethod
    def tearDownClass(self):
        self.user0.__exit__()
        self.user1.__exit__()

        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand(['iadmin', 'rmuser', self.user0.username])
            admin_session.assert_icommand(['iadmin', 'rmuser', self.user1.username])
            lib.remove_resource(admin_session, self.target_resource)
            lib.remove_resource(admin_session, self.other_resource)

    def permissions_that_do_not_allow_seeing_object_results_in_no_replication_and_an_error_impl(self, user_or_group):
        logical_path = os.path.join(self.user0.session_collection, 'this_object_is_invisible')
        permissions = [None, 'null', 'read_metadata']

        for permission in permissions:
            with self.subTest(str(permission)):
                try:
                    # Create a data object and make sure it is good.
                    self.user0.assert_icommand(['itouch', '-R', self.target_resource, logical_path])
                    self.assertTrue(lib.replica_exists_on_resource(self.user0, logical_path, self.target_resource))
                    self.assertEqual(str(1), lib.get_replica_status(self.user0, os.path.basename(logical_path), 0))

                    if permission is not None:
                        self.user0.assert_icommand(['ichmod', permission, user_or_group, logical_path])

                    # Try to replicate the data object and fail because user1 cannot even see the data object.
                    self.user1.assert_icommand(
                        ['irepl', '-R', self.other_resource, logical_path], 'STDERR', 'does not exist')
                    self.assertTrue(lib.replica_exists_on_resource(self.user0, logical_path, self.target_resource))
                    self.assertEqual(str(1), lib.get_replica_status(self.user0, os.path.basename(logical_path), 0))
                    self.assertFalse(lib.replica_exists_on_resource(self.user0, logical_path, self.other_resource))

                    # Now replicate the data object and set it to stale so that it is possible to be updated.
                    self.user0.assert_icommand(['irepl', '-R', self.other_resource, logical_path])
                    self.assertTrue(lib.replica_exists_on_resource(self.user0, logical_path, self.other_resource))
                    self.assertEqual(str(1), lib.get_replica_status(self.user0, os.path.basename(logical_path), 1))
                    with session.make_session_for_existing_admin() as admin_session:
                        lib.set_replica_status(admin_session, logical_path, 1, 0)
                    self.assertEqual(str(0), lib.get_replica_status(self.user0, os.path.basename(logical_path), 1))

                    # Try to update the stale replicas and fail because user1 cannot even see the data object.
                    self.user1.assert_icommand(['irepl', '-a', logical_path], 'STDERR', 'does not exist')
                    self.assertTrue(lib.replica_exists_on_resource(self.user0, logical_path, self.target_resource))
                    self.assertEqual(str(1), lib.get_replica_status(self.user0, os.path.basename(logical_path), 0))
                    self.assertTrue(lib.replica_exists_on_resource(self.user0, logical_path, self.other_resource))
                    self.assertEqual(str(0), lib.get_replica_status(self.user0, os.path.basename(logical_path), 1))

                finally:
                    # In the case where this test does not pass (i.e. REGRESSION) it is possible for the replicas to be
                    # stuck in the intermediate or write-locked status. We set the status at the end here to ensure
                    # that the object can be removed.
                    with session.make_session_for_existing_admin() as admin_session:
                        admin_session.assert_icommand(['ils', '-L', os.path.dirname(logical_path)], 'STDOUT') # debug
                        for replica_number in range(2):
                            admin_session.run_icommand([
                                'iadmin', 'modrepl',
                                'logical_path', logical_path,
                                'replica_number', str(replica_number),
                                'DATA_REPL_STATUS', str(0)
                            ])
                    self.user0.assert_icommand(['irm', '-f', logical_path])

    def test_permissions_that_do_not_allow_user_to_see_object_results_in_no_replication_and_an_error(self):
        user = self.user1.username
        self.permissions_that_do_not_allow_seeing_object_results_in_no_replication_and_an_error_impl(user)

    def test_permissions_that_do_not_allow_group_to_see_object_results_in_no_replication_and_an_error(self):
        user = self.user1.username
        group = 'testgroup'

        try:
            # Create a test group and add the test user to the group. The test group will be checked for permissions on
            # the data object while the user itself will have no permissions on the data object.
            with session.make_session_for_existing_admin() as admin_session:
                admin_session.assert_icommand(['iadmin', 'mkgroup', group])
                admin_session.assert_icommand(['iadmin', 'atg', group, user])

            self.permissions_that_do_not_allow_seeing_object_results_in_no_replication_and_an_error_impl(group)

        finally:
            # Clean up the test group.
            with session.make_session_for_existing_admin() as admin_session:
                admin_session.run_icommand(['iadmin', 'rfg', group, user])
                admin_session.run_icommand(['iadmin', 'rmgroup', group])

    def insufficient_permissions_results_in_no_replication_and_an_error_impl(self, user_or_group):
        logical_path = os.path.join(self.user0.session_collection, 'this_object_will_not_be_replicated')
        permissions = ['read_object', 'create_metadata', 'modify_metadata', 'delete_metadata', 'create_object']

        for permission in permissions:
            with self.subTest(permission):
                try:
                    # Create a data object and make sure it is good.
                    self.user0.assert_icommand(['itouch', '-R', self.target_resource, logical_path])
                    self.assertTrue(lib.replica_exists_on_resource(self.user0, logical_path, self.target_resource))
                    self.assertEqual(str(1), lib.get_replica_status(self.user0, os.path.basename(logical_path), 0))

                    self.user0.assert_icommand(['ichmod', permission, user_or_group, logical_path])

                    # Try to replicate the data object and ensure that it fails with an error.
                    self.user1.assert_icommand(
                        ['irepl', '-R', self.other_resource, logical_path], 'STDERR', 'SYS_USER_NO_PERMISSION')
                    self.assertTrue(lib.replica_exists_on_resource(self.user0, logical_path, self.target_resource))
                    self.assertEqual(str(1), lib.get_replica_status(self.user0, os.path.basename(logical_path), 0))
                    self.assertFalse(lib.replica_exists_on_resource(self.user0, logical_path, self.other_resource))

                    # Now replicate the data object and set it to stale so that it is possible to be updated.
                    self.user0.assert_icommand(['irepl', '-R', self.other_resource, logical_path])
                    self.assertTrue(lib.replica_exists_on_resource(self.user0, logical_path, self.other_resource))
                    self.assertEqual(str(1), lib.get_replica_status(self.user0, os.path.basename(logical_path), 1))
                    with session.make_session_for_existing_admin() as admin_session:
                        lib.set_replica_status(admin_session, logical_path, 1, 0)
                    self.assertEqual(str(0), lib.get_replica_status(self.user0, os.path.basename(logical_path), 1))

                    # Try to update the stale replicas and ensure that it fails with an error.
                    self.user1.assert_icommand(['irepl', '-a', logical_path], 'STDERR', 'SYS_USER_NO_PERMISSION')
                    self.assertTrue(lib.replica_exists_on_resource(self.user0, logical_path, self.target_resource))
                    self.assertEqual(str(1), lib.get_replica_status(self.user0, os.path.basename(logical_path), 0))
                    self.assertTrue(lib.replica_exists_on_resource(self.user0, logical_path, self.other_resource))
                    self.assertEqual(str(0), lib.get_replica_status(self.user0, os.path.basename(logical_path), 1))

                finally:
                    # In the case where this test does not pass (i.e. REGRESSION) it is possible for the replicas to be
                    # stuck in the intermediate or write-locked status. We set the status at the end here to ensure
                    # that the object can be removed.
                    with session.make_session_for_existing_admin() as admin_session:
                        admin_session.assert_icommand(['ils', '-L', os.path.dirname(logical_path)], 'STDOUT') # debug
                        for replica_number in range(2):
                            admin_session.run_icommand([
                                'iadmin', 'modrepl',
                                'logical_path', logical_path,
                                'replica_number', str(replica_number),
                                'DATA_REPL_STATUS', str(0)
                            ])
                    self.user0.assert_icommand(['irm', '-f', logical_path])

    def test_insufficient_permissions_for_user_results_in_no_replication_and_an_error(self):
        user = self.user1.username
        self.insufficient_permissions_results_in_no_replication_and_an_error_impl(user)

    def test_insufficient_permissions_for_group_results_in_no_replication_and_an_error(self):
        user = self.user1.username
        group = 'testgroup'

        try:
            # Create a test group and add the test user to the group. The test group will be checked for permissions on
            # the data object while the user itself will have no permissions on the data object.
            with session.make_session_for_existing_admin() as admin_session:
                admin_session.assert_icommand(['iadmin', 'mkgroup', group])
                admin_session.assert_icommand(['iadmin', 'atg', group, user])

            self.insufficient_permissions_results_in_no_replication_and_an_error_impl(group)

        finally:
            # Clean up the test group.
            with session.make_session_for_existing_admin() as admin_session:
                admin_session.run_icommand(['iadmin', 'rfg', group, user])
                admin_session.run_icommand(['iadmin', 'rmgroup', group])

    def sufficient_permissions_results_in_replication_and_no_error_impl(self, user_or_group):
        logical_path = os.path.join(self.user0.session_collection, 'this_object_will_be_replicated')
        permissions = ['modify_object', 'delete_object', 'own']

        for permission in permissions:
            with self.subTest(permission):
                try:
                    # Create a data object and make sure it is good.
                    self.user0.assert_icommand(['itouch', '-R', self.target_resource, logical_path])
                    self.assertTrue(lib.replica_exists_on_resource(self.user0, logical_path, self.target_resource))
                    self.assertEqual(str(1), lib.get_replica_status(self.user0, os.path.basename(logical_path), 0))

                    self.user0.assert_icommand(['ichmod', permission, user_or_group, logical_path])

                    # Try to replicate the data object and ensure that it completes successfully.
                    self.user1.assert_icommand(['irepl', '-R', self.other_resource, logical_path])
                    self.assertTrue(lib.replica_exists_on_resource(self.user0, logical_path, self.target_resource))
                    self.assertEqual(str(1), lib.get_replica_status(self.user0, os.path.basename(logical_path), 0))
                    self.assertTrue(lib.replica_exists_on_resource(self.user0, logical_path, self.other_resource))
                    self.assertEqual(str(1), lib.get_replica_status(self.user0, os.path.basename(logical_path), 1))

                    # Set a replica to stale so that it is possible to be updated.
                    with session.make_session_for_existing_admin() as admin_session:
                        lib.set_replica_status(admin_session, logical_path, 1, 0)
                    self.assertEqual(str(0), lib.get_replica_status(self.user0, os.path.basename(logical_path), 1))

                    # Try to update the stale replicas and ensure that it completes successfully.
                    self.user1.assert_icommand(['irepl', '-a', logical_path])
                    self.assertTrue(lib.replica_exists_on_resource(self.user0, logical_path, self.target_resource))
                    self.assertEqual(str(1), lib.get_replica_status(self.user0, os.path.basename(logical_path), 0))
                    self.assertTrue(lib.replica_exists_on_resource(self.user0, logical_path, self.other_resource))
                    self.assertEqual(str(1), lib.get_replica_status(self.user0, os.path.basename(logical_path), 1))

                finally:
                    # In the case where this test does not pass (i.e. REGRESSION) it is possible for the replicas to be
                    # stuck in the intermediate or write-locked status. We set the status at the end here to ensure
                    # that the object can be removed.
                    with session.make_session_for_existing_admin() as admin_session:
                        admin_session.assert_icommand(['ils', '-L', os.path.dirname(logical_path)], 'STDOUT') # debug
                        for replica_number in range(2):
                            admin_session.run_icommand([
                                'iadmin', 'modrepl',
                                'logical_path', logical_path,
                                'replica_number', str(replica_number),
                                'DATA_REPL_STATUS', str(0)
                            ])
                    self.user0.assert_icommand(['irm', '-f', logical_path])

    def test_sufficient_permissions_for_user_results_in_replication_and_no_error(self):
        user = self.user1.username
        self.sufficient_permissions_results_in_replication_and_no_error_impl(user)

    def test_sufficient_permissions_for_group_results_in_replication_and_no_error(self):
        user = self.user1.username
        group = 'testgroup'

        try:
            # Create a test group and add the test user to the group. The test group will be checked for permissions on
            # the data object while the user itself will have no permissions on the data object.
            with session.make_session_for_existing_admin() as admin_session:
                admin_session.assert_icommand(['iadmin', 'mkgroup', group])
                admin_session.assert_icommand(['iadmin', 'atg', group, user])

            self.sufficient_permissions_results_in_replication_and_no_error_impl(group)

        finally:
            # Clean up the test group.
            with session.make_session_for_existing_admin() as admin_session:
                admin_session.run_icommand(['iadmin', 'rfg', group, user])
                admin_session.run_icommand(['iadmin', 'rmgroup', group])

    def test_groups_with_test_user_have_no_permission_to_replicate_but_group_without_test_user_does__issue_7816(self):
        group_count = 10
        group_names = [f'group_{i}' for i in range(group_count)]
        group_without_test_user = 'group_without_test_user'

        logical_path = os.path.join(self.user0.session_collection, 'test_user_member_groups_cannot_replicate_this')
        permission_for_groups_with_test_user = 'read_object'
        permission_for_group_without_test_user = 'modify_object'

        try:
            with session.make_session_for_existing_admin() as admin_session:
                # Create another group which exists but does not have the test user as a member so that we can test
                # whether the permission check query properly excludes groups of which the test user is not a member.
                admin_session.assert_icommand(['iadmin', 'mkgroup', group_without_test_user])

                group_user = self.user1.username
                for group in group_names:
                    admin_session.assert_icommand(['iadmin', 'mkgroup', group])
                    admin_session.assert_icommand(['iadmin', 'atg', group, group_user])

            # Create a data object and make sure it is good.
            self.user0.assert_icommand(['itouch', '-R', self.target_resource, logical_path])
            self.assertTrue(lib.replica_exists_on_resource(self.user0, logical_path, self.target_resource))
            self.assertEqual(str(1), lib.get_replica_status(self.user0, os.path.basename(logical_path), 0))

            for group in group_names:
                self.user0.assert_icommand(['ichmod', permission_for_groups_with_test_user, group, logical_path])

            # Grant permissions to a group of which the test user is not a member.
            self.user0.assert_icommand(
                ['ichmod', permission_for_group_without_test_user, group_without_test_user, logical_path])

            # Try to replicate the data object and ensure that it fails with an error.
            self.user1.assert_icommand(
                ['irepl', '-R', self.other_resource, logical_path], 'STDERR', 'SYS_USER_NO_PERMISSION')
            self.assertTrue(lib.replica_exists_on_resource(self.user0, logical_path, self.target_resource))
            self.assertEqual(str(1), lib.get_replica_status(self.user0, os.path.basename(logical_path), 0))
            self.assertFalse(lib.replica_exists_on_resource(self.user0, logical_path, self.other_resource))

            # Now replicate the data object and set it to stale so that it is possible to be updated.
            self.user0.assert_icommand(['irepl', '-R', self.other_resource, logical_path])
            self.assertTrue(lib.replica_exists_on_resource(self.user0, logical_path, self.other_resource))
            self.assertEqual(str(1), lib.get_replica_status(self.user0, os.path.basename(logical_path), 1))
            with session.make_session_for_existing_admin() as admin_session:
                lib.set_replica_status(admin_session, logical_path, 1, 0)
            self.assertEqual(str(0), lib.get_replica_status(self.user0, os.path.basename(logical_path), 1))

            # Try to update the stale replicas and ensure that it fails with an error.
            self.user1.assert_icommand(['irepl', '-a', logical_path], 'STDERR', 'SYS_USER_NO_PERMISSION')
            self.assertTrue(lib.replica_exists_on_resource(self.user0, logical_path, self.target_resource))
            self.assertEqual(str(1), lib.get_replica_status(self.user0, os.path.basename(logical_path), 0))
            self.assertTrue(lib.replica_exists_on_resource(self.user0, logical_path, self.other_resource))
            self.assertEqual(str(0), lib.get_replica_status(self.user0, os.path.basename(logical_path), 1))

        finally:
            # In the case where this test does not pass (i.e. REGRESSION) it is possible for the replicas to be
            # stuck in the intermediate or write-locked status. We set the status at the end here to ensure
            # that the object can be removed.
            with session.make_session_for_existing_admin() as admin_session:
                admin_session.assert_icommand(['ils', '-L', os.path.dirname(logical_path)], 'STDOUT') # debug
                for replica_number in range(2):
                    admin_session.run_icommand([
                        'iadmin', 'modrepl',
                        'logical_path', logical_path,
                        'replica_number', str(replica_number),
                        'DATA_REPL_STATUS', str(0)
                    ])

                admin_session.run_icommand(['iadmin', 'rmgroup', group_without_test_user])

                for group in group_names:
                    admin_session.run_icommand(['iadmin', 'rfg', group, self.user1.username])
                    admin_session.run_icommand(['iadmin', 'rmgroup', group])

            self.user0.assert_icommand(['irm', '-f', logical_path])

    def test_multiple_groups_with_test_user_have_sufficient_permission_to_replicate__issue_7816(self):
        group_count = 10
        group_names = [f'group_{i}' for i in range(group_count)]
        group_without_test_user = 'group_without_test_user'

        logical_path = os.path.join(self.user0.session_collection, 'test_user_member_groups_can_replicate_this')
        permission_for_groups_with_test_user = 'modify_object'
        permission_for_group_without_test_user = 'read_object'

        try:
            with session.make_session_for_existing_admin() as admin_session:
                # Create another group which exists but does not have the test user as a member so that we can test
                # whether the permission check query properly excludes groups of which the test user is not a member.
                admin_session.assert_icommand(['iadmin', 'mkgroup', group_without_test_user])

                group_user = self.user1.username
                for group in group_names:
                    admin_session.assert_icommand(['iadmin', 'mkgroup', group])
                    admin_session.assert_icommand(['iadmin', 'atg', group, group_user])

            # Create a data object and make sure it is good.
            self.user0.assert_icommand(['itouch', '-R', self.target_resource, logical_path])
            self.assertTrue(lib.replica_exists_on_resource(self.user0, logical_path, self.target_resource))
            self.assertEqual(str(1), lib.get_replica_status(self.user0, os.path.basename(logical_path), 0))

            for group in group_names:
                self.user0.assert_icommand(['ichmod', permission_for_groups_with_test_user, group, logical_path])

            # Grant permissions to a group of which the test user is not a member.
            self.user0.assert_icommand(
                ['ichmod', permission_for_group_without_test_user, group_without_test_user, logical_path])

            # Try to replicate the data object and ensure that it completes successfully.
            self.user1.assert_icommand(['irepl', '-R', self.other_resource, logical_path])
            self.assertTrue(lib.replica_exists_on_resource(self.user0, logical_path, self.target_resource))
            self.assertEqual(str(1), lib.get_replica_status(self.user0, os.path.basename(logical_path), 0))
            self.assertTrue(lib.replica_exists_on_resource(self.user0, logical_path, self.other_resource))
            self.assertEqual(str(1), lib.get_replica_status(self.user0, os.path.basename(logical_path), 1))

            # Set a replica to stale so that it is possible to be updated.
            with session.make_session_for_existing_admin() as admin_session:
                lib.set_replica_status(admin_session, logical_path, 1, 0)
            self.assertEqual(str(0), lib.get_replica_status(self.user0, os.path.basename(logical_path), 1))

            # Try to update the stale replicas and ensure that it completes successfully.
            self.user1.assert_icommand(['irepl', '-a', logical_path])
            self.assertTrue(lib.replica_exists_on_resource(self.user0, logical_path, self.target_resource))
            self.assertEqual(str(1), lib.get_replica_status(self.user0, os.path.basename(logical_path), 0))
            self.assertTrue(lib.replica_exists_on_resource(self.user0, logical_path, self.other_resource))
            self.assertEqual(str(1), lib.get_replica_status(self.user0, os.path.basename(logical_path), 1))

        finally:
            # In the case where this test does not pass (i.e. REGRESSION) it is possible for the replicas to be
            # stuck in the intermediate or write-locked status. We set the status at the end here to ensure
            # that the object can be removed.
            with session.make_session_for_existing_admin() as admin_session:
                admin_session.assert_icommand(['ils', '-L', os.path.dirname(logical_path)], 'STDOUT') # debug
                for replica_number in range(2):
                    admin_session.run_icommand([
                        'iadmin', 'modrepl',
                        'logical_path', logical_path,
                        'replica_number', str(replica_number),
                        'DATA_REPL_STATUS', str(0)
                    ])

                admin_session.run_icommand(['iadmin', 'rmgroup', group_without_test_user])

                for group in group_names:
                    admin_session.run_icommand(['iadmin', 'rfg', group, self.user1.username])
                    admin_session.run_icommand(['iadmin', 'rmgroup', group])

            self.user0.assert_icommand(['irm', '-f', logical_path])
