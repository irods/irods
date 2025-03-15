import json
import textwrap
import time
import unittest

from . import session
from .. import lib
from ..configuration import IrodsConfig

rodsadmins = [('otherrods', 'rods')]
rodsusers  = [('alice', 'apass')]


class Test_Access_Time_Updates(session.make_sessions_mixin(rodsadmins, rodsusers), unittest.TestCase):

    plugin_name = IrodsConfig().default_rule_engine_plugin

    def setUp(self):
        super(Test_Access_Time_Updates, self).setUp()

        self.admin = self.admin_sessions[0]
        self.user = self.user_sessions[0]

    def tearDown(self):
        super(Test_Access_Time_Updates, self).tearDown()

    def test_GenQuery1_supports_access_time_issue_8260(self):
        data_object = 'issue_8260.txt'
        self.user.assert_icommand(['itouch', data_object])

        query = f"select DATA_ACCESS_TIME where DATA_NAME = '{data_object}'"

        with self.subTest('using GenQuery1'):
            self.user.assert_icommand(['iquest', query], 'STDOUT', r'DATA_ACCESS_TIME = 0\d{10}', use_regex=True)

        with self.subTest('using GenQuery2'):
            self.user.assert_icommand(['iquery', query], 'STDOUT', r'"0\d{10}"', use_regex=True)

    def test_access_time_matches_modification_time_for_new_replicas__issue_8260(self):
        data_object = 'issue_8260.txt'
        self.user.assert_icommand(['itouch', data_object])

        def assert_atime_matches_mtime_for_replica(data_object, replica_number):
            query = f"select DATA_ACCESS_TIME, DATA_MODIFY_TIME where DATA_NAME = '{data_object}' and DATA_REPL_NUM = '{replica_number}'"
            _, out, _ = self.user.assert_icommand(['iquery', query], 'STDOUT')
            rows = json.loads(out.strip())
            self.assertEqual(rows[0][0], rows[0][1])

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

            # TODO Must cover ireg too.

        finally:
            self.user.run_icommand(['irm', '-f', data_object])
            self.admin.run_icommand(['iadmin', 'rmresc', resc_name])

    # TODO
    def test_grid_configuration_values_for_access_time_are_honored__issue_8260(self):
        pass
