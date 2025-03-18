import json
import time
import unittest

from pathlib import Path

from . import session
from .. import lib
from .. import paths
from .. import test
from ..controller import IrodsController
from ..exceptions import IrodsError

rodsadmins = [('otherrods', 'rods')]
rodsusers  = [('alice', 'apass')]


class Test_Access_Time_Updates(session.make_sessions_mixin(rodsadmins, rodsusers), unittest.TestCase):

    def setUp(self):
        super(Test_Access_Time_Updates, self).setUp()

        self.admin = self.admin_sessions[0]
        self.user = self.user_sessions[0]

    def tearDown(self):
        super(Test_Access_Time_Updates, self).tearDown()

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY,
        "Topology testing for a GenQuery column isn't necessary because GenQuery always redirects to the Catalog Service Provider")
    def test_GenQuery_supports_access_time_issue_8260(self):
        data_object = 'test_GenQuery_supports_access_time_issue_8260.txt'
        self.user.assert_icommand(['itouch', data_object])

        query = f"select DATA_ACCESS_TIME where DATA_NAME = '{data_object}'"

        with self.subTest('using GenQuery1'):
            self.user.assert_icommand(['iquest', query], 'STDOUT', r'DATA_ACCESS_TIME = 0\d{10}', use_regex=True)

        with self.subTest('using GenQuery2'):
            self.user.assert_icommand(['iquery', query], 'STDOUT', r'"0\d{10}"', use_regex=True)

    def test_access_time_matches_modification_time_for_new_replicas__issue_8260(self):
        def assert_atime_matches_mtime_for_replica(data_object, replica_number):
            query = f"select DATA_ACCESS_TIME, DATA_MODIFY_TIME where DATA_NAME = '{data_object}' and DATA_REPL_NUM = '{replica_number}'"
            _, out, _ = self.user.assert_icommand(['iquery', query], 'STDOUT')
            rows = json.loads(out.strip())
            self.assertEqual(rows[0][0], rows[0][1])

        data_object = 'test_access_time_matches_modification_time_for_new_replicas__issue_8260.txt'
        self.user.assert_icommand(['itouch', data_object])

        # Creating a new data object must result in the replica's atime matching its mtime.
        assert_atime_matches_mtime_for_replica(data_object, 0)

        try:
            resc_name = 'issue_8260_resc'
            lib.create_ufs_resource(self.admin, resc_name)

            # Creating a new replica for the data object must result in the new replica's
            # atime matching its mtime. We sleep a few seconds so that the timestamps are
            # guaranteed to be different.
            time.sleep(2)
            self.user.assert_icommand(['irepl', '-R', resc_name, data_object])
            assert_atime_matches_mtime_for_replica(data_object, 1)
            self.user.assert_icommand(['itrim', '-N', '1', '-n', '1', data_object], 'STDOUT', ['trimmed = 1'])

            # Give the admin ownership over the data object so that they can register
            # new replicas via ireg.
            self.user.assert_icommand(['ichmod', 'own', self.admin.username, data_object])

            # Create a new file. This file will be registered as a new replica for the
            # data object.
            local_file = f'{self.user.local_session_dir}/{data_object}.ireg'
            Path(local_file).touch()

            # Show that registering a replica for an existing data object results in the
            # new replica's atime matching its mtime.
            logical_path = f'{self.user.session_collection}/{data_object}'
            self.admin.assert_icommand(['ireg', '--repl', '-R', resc_name, local_file, logical_path])
            assert_atime_matches_mtime_for_replica(data_object, 1)

        finally:
            self.user.run_icommand(['irm', '-f', data_object])
            self.admin.run_icommand(['iadmin', 'rmresc', resc_name])

    def test_grid_configuration_value_for_queue_name_prefix_is_honored__issue_8260(self):
        # Capture the original value so it can be restored.
        option_name = 'queue_name_prefix'
        _, out, _ = self.admin.assert_icommand(['iadmin', 'get_grid_configuration', 'access_time', option_name], 'STDOUT')
        original_option_value = out.strip()

        try:
            # Set the queue name prefix to the default value. This requires a
            # server restart to take effect. The grid configuration options for
            # access time are only loaded on server startup.
            option_value = 'irods_access_time_queue_'
            self.admin.assert_icommand(['iadmin', 'set_grid_configuration', 'access_time', option_name, option_value])
            IrodsController().restart(test_mode=True)

            # Show that a shared memory file exists with the default queue name prefix.
            shm_directory = Path('/dev/shm')
            self.assertTrue(any(f.name.startswith(option_value) for f in shm_directory.iterdir()))

            # Now, adjust the queue name prefix.
            option_value = 'irods_access_time_queue_issue_8260_'
            self.admin.assert_icommand(['iadmin', 'set_grid_configuration', 'access_time', option_name, option_value])
            IrodsController().restart(test_mode=True)

            # Show that a new shared memory file exists with the queue name prefix we just set.
            self.assertTrue(any(f.name.startswith(option_value) for f in shm_directory.iterdir()))

        finally:
            # Restore the original grid configuration value.
            self.admin.run_icommand(['iadmin', 'set_grid_configuration', 'access_time', option_name, original_option_value])
            IrodsController().restart(test_mode=True)

    def test_grid_configuration_value_for_queue_size_is_honored__issue_8260(self):
        def get_atime_queue_file_size():
            shm_directory = Path('/dev/shm')
            for f in shm_directory.iterdir():
                if f.name.startswith('irods_access_time_queue_'):
                    return f.stat().st_size
            raise ValueError('Could not get size of shared memory file for access time')

        # Capture the original value so it can be restored.
        option_name = 'queue_size'
        _, out, _ = self.admin.assert_icommand(['iadmin', 'get_grid_configuration', 'access_time', option_name], 'STDOUT')
        original_option_value = out.strip()

        try:
            # Set the queue size to the default value. This requires a server restart
            # to take effect. The grid configuration options for access time are only
            # loaded on server startup.
            option_value = '20000'
            self.admin.assert_icommand(['iadmin', 'set_grid_configuration', 'access_time', option_name, option_value])
            IrodsController().restart(test_mode=True)

            # Show that the shared memory file for the queue has a non-zero file size.
            old_file_size = get_atime_queue_file_size()
            self.assertGreater(old_file_size, 0)

            # Now, adjust the queue size so that the shared memory file's size shrinks.
            option_value = '1000'
            self.admin.assert_icommand(['iadmin', 'set_grid_configuration', 'access_time', option_name, option_value])
            IrodsController().restart(test_mode=True)

            # Show that the new queue size has resulted in a smaller file size.
            self.assertLess(get_atime_queue_file_size(), old_file_size)

        finally:
            # Restore the original grid configuration value.
            self.admin.run_icommand(['iadmin', 'set_grid_configuration', 'access_time', option_name, original_option_value])
            IrodsController().restart(test_mode=True)

    def test_grid_configuration_value_for_resolution_in_seconds_is_honored__issue_8260(self):
        # Capture the original value so it can be restored.
        option_name = 'resolution_in_seconds'
        _, out, _ = self.admin.assert_icommand(['iadmin', 'get_grid_configuration', 'access_time', option_name], 'STDOUT')
        original_option_value = out.strip()

        try:
            # Set the resolution in seconds to the default value. This requires
            # a server restart to take effect. The grid configuration options for
            # access time are only loaded on server startup.
            option_value = '86400'
            self.admin.assert_icommand(['iadmin', 'set_grid_configuration', 'access_time', option_name, option_value])
            IrodsController().restart(test_mode=True)

            # Create a new data object and capture its atime.
            data_object = f'{self.user.session_collection}/test_grid_configuration_value_for_resolution_in_seconds_is_honored__issue_8260.txt'
            self.user.assert_icommand(['itouch', data_object])
            old_atime = lib.get_replica_atime(self.user, data_object, 0)

            # Show that reading from the data object does not update the atime.
            # This is because the atime matches the mtime and the time since the last
            # atime updated hasn't exceeded the resolution in seconds.
            self.user.assert_icommand(['istream', 'read', data_object], 'STDOUT')
            self.assertEqual(lib.get_replica_atime(self.user, data_object, 0), old_atime)

            # Now, adjust the resolution in seconds to a significantly smaller value
            # so that the atime is updated on the next access.
            option_value = 2
            self.admin.assert_icommand(['iadmin', 'set_grid_configuration', 'access_time', option_name, str(option_value)])
            IrodsController().restart(test_mode=True)

            # Wait a few seconds before reading the data object. This guarantees the
            # next read operation triggers an atime update.
            time.sleep(option_value + 1)
            self.user.assert_icommand(['istream', 'read', data_object], 'STDOUT')
            # Updating the atime isn't instantaneous, so give the main server process
            # a moment to process the atime update queue.
            lib.delayAssert(lambda: lib.get_replica_atime(self.user, data_object, 0) > old_atime, maxrep=20)

        finally:
            # Restore the original grid configuration value.
            self.admin.run_icommand(['iadmin', 'set_grid_configuration', 'access_time', option_name, original_option_value])
            IrodsController().restart(test_mode=True)

    def test_invalid_grid_configuration_values_for_access_time_do_not_cause_server_startup_to_fail__issue_8620(self):
        # queue_name_prefix and queue_size are not included because they affect the filename
        # and size of the shared memory. Invalid values for these two properties will result
        # in the server failing to start. This is intentional.
        #
        # This test only applies to the batch_size and resolution_in_seconds options.
        option_value_pairs = [
            ('batch_size', ''),
            ('batch_size', '-1'),
            ('batch_size', str(2**32 + 1)),
            ('batch_size', str(2**129)),
            ('batch_size', 'not a number'),
            ('resolution_in_seconds', ''),
            ('resolution_in_seconds', '-1'),
            ('resolution_in_seconds', str(2**32 + 1)),
            ('resolution_in_seconds', str(2**129)),
            ('resolution_in_seconds', 'not a number'),
        ]

        for opt_name, opt_value in option_value_pairs:
            with self.subTest(f'option_name=[{opt_name}], option_value=[{opt_value}]'):
                # Capture the original value so it can be restored.
                _, out, _ = self.admin.assert_icommand(['iadmin', 'get_grid_configuration', 'access_time', opt_name], 'STDOUT')
                original_option_value = out.strip()

                try:
                    # Set an invalid value for the option.
                    self.admin.assert_icommand(['iadmin', 'set_grid_configuration', 'access_time', opt_name, opt_value])
                    IrodsController().restart(test_mode=True)

                    # Capture the current log file size. This will serve as an offset for when we inspect
                    # the log file for messages.
                    log_offset = lib.get_file_size_by_path(paths.server_log_path())

                    # Check the log file to see if the server is using the default value.
                    expected_msg = 'Using default batch size of 20000.' if 'batch_size' == opt_name else 'Using default resolution of 86400.'
                    lib.delayAssert(lambda: lib.log_message_occurrences_greater_than_count(msg=expected_msg, count=0, start_index=log_offset))

                finally:
                    # Restore the server
                    self.admin.assert_icommand(['iadmin', 'set_grid_configuration', 'access_time', opt_name, original_option_value])
                    IrodsController().restart(test_mode=True)
