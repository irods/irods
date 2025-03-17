import json
import textwrap
import time
import unittest

from pathlib import Path

from . import session
from .. import lib
from .. import test
from ..configuration import IrodsConfig
from ..controller import IrodsController

rodsadmins = [('otherrods', 'rods')]
rodsusers  = [('alice', 'apass')]


class Test_Access_Time_Updates(session.make_sessions_mixin(rodsadmins, rodsusers), unittest.TestCase):

    def setUp(self):
        super(Test_Access_Time_Updates, self).setUp()

        self.admin = self.admin_sessions[0]
        self.user = self.user_sessions[0]

    def tearDown(self):
        super(Test_Access_Time_Updates, self).tearDown()

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_GenQuery_supports_access_time_issue_8260(self):
        data_object = 'test_GenQuery_supports_access_time_issue_8260.txt'
        self.user.assert_icommand(['itouch', data_object])

        query = f"select DATA_ACCESS_TIME where DATA_NAME = '{data_object}'"

        with self.subTest('using GenQuery1'):
            self.user.assert_icommand(['iquest', query], 'STDOUT', r'DATA_ACCESS_TIME = 0\d{10}', use_regex=True)

        with self.subTest('using GenQuery2'):
            self.user.assert_icommand(['iquery', query], 'STDOUT', r'"0\d{10}"', use_regex=True)

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_access_time_matches_modification_time_for_new_replicas__issue_8260(self):
        def assert_atime_matches_mtime_for_replica(data_object, replica_number):
            query = f"select DATA_ACCESS_TIME, DATA_MODIFY_TIME where DATA_NAME = '{data_object}' and DATA_REPL_NUM = '{replica_number}'"
            _, out, _ = self.user.assert_icommand(['iquery', query], 'STDOUT')
            rows = json.loads(out.strip())
            self.assertEqual(rows[0][0], rows[0][1])

        data_object = 'test_access_time_matches_modification_time_for_new_replicas__issue_8260.txt'
        self.user.assert_icommand(['itouch', data_object])

        # Create a new data object must result in the replica's atime matching its mtime.
        assert_atime_matches_mtime_for_replica(data_object, 0)

        try:
            resc_name = 'issue_8260_resc'
            lib.create_ufs_resource(self.admin, resc_name)

            # Create a new replica for the data object must result in the new replica's
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

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
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

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
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

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_grid_configuration_value_for_resolution_in_seconds_is_honored__issue_8260(self):
        def get_atime(data_object):
            query = f"select DATA_ACCESS_TIME where DATA_NAME = '{data_object}'"
            _, out, _ = self.user.assert_icommand(['iquery', query], 'STDOUT')
            return json.loads(out.strip())[0][0]

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
            data_object = 'test_grid_configuration_value_for_resolution_in_seconds_is_honored__issue_8260.txt'
            self.user.assert_icommand(['itouch', data_object])
            old_atime = get_atime(data_object)

            # Show that reading from the data object does not update the atime.
            # This is because the atime matches the mtime and the time since the last
            # atime updated hasn't exceeded the resolution in seconds.
            self.user.assert_icommand(['istream', 'read', data_object], 'STDOUT')
            self.assertEqual(get_atime(data_object), old_atime)

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
            lib.delayAssert(lambda: get_atime(data_object) > old_atime, maxrep=20)

        finally:
            # Restore the original grid configuration value.
            self.admin.run_icommand(['iadmin', 'set_grid_configuration', 'access_time', option_name, original_option_value])
            IrodsController().restart(test_mode=True)
