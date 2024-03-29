from __future__ import print_function
import os
import sys
import textwrap

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from .. import lib
from .. import test
from ..configuration import IrodsConfig
from ..test.command import assert_command

class test_targeting_specific_replica_number__issue_6896(
    session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]),
    unittest.TestCase):

    global plugin_name
    plugin_name = IrodsConfig().default_rule_engine_plugin

    def setUp(self):
        super(test_targeting_specific_replica_number__issue_6896, self).setUp()
        self.admin = self.admin_sessions[0]
        self.user = self.user_sessions[0]

        self.pt1 = 'pt1'
        self.pt2 = 'pt2'
        self.ufs1 = 'ufs1'
        self.ufs2 = 'ufs2'

        # Create two resources with passthrus as parents in order to manipulate voting.
        lib.create_passthru_resource(self.admin, self.pt1)
        lib.create_passthru_resource(self.admin, self.pt2)
        lib.create_ufs_resource(self.admin, self.ufs1, hostname=test.settings.HOSTNAME_2)
        lib.create_ufs_resource(self.admin, self.ufs2, hostname=test.settings.HOSTNAME_3)

        for parent, child in [(self.pt1, self.ufs1), (self.pt2, self.ufs2)]:
            lib.add_child_resource(self.admin, parent, child)

        self.admin.assert_icommand(['ilsresc', '--ascii'], 'STDOUT') # debugging

        # Set up some local files to put for reading and writing in the tests which follow...
        self.file_size = 1000
        self.other_file_size = 2000
        self.local_file = os.path.join(self.user.local_session_dir, 'smallerfile')
        self.other_local_file = os.path.join(self.user.local_session_dir, 'biggerfile')
        self.logical_path = os.path.join(self.user.session_collection, 'whateveritsatest')

        if not os.path.exists(self.other_local_file):
            lib.make_file(self.other_local_file, self.other_file_size, 'arbitrary')

        if not os.path.exists(self.local_file):
            lib.make_file(self.local_file, self.file_size, 'arbitrary')

        # Create a data object and ensure that each hierarchy has a good replica of the expected size.
        self.user.assert_icommand(['iput', '-R', self.pt1, self.local_file, self.logical_path])
        self.user.assert_icommand(['irepl', '-R', self.pt2, self.logical_path])
        self.assertEqual(
            str(self.file_size), lib.get_replica_size(self.user, os.path.basename(self.logical_path), 0))
        self.assertEqual(
            str(self.file_size), lib.get_replica_size(self.user, os.path.basename(self.logical_path), 1))
        self.assertEqual(str(1), lib.get_replica_status(self.user, os.path.basename(self.logical_path), 0))
        self.assertEqual(str(1), lib.get_replica_status(self.user, os.path.basename(self.logical_path), 1))
        self.user.assert_icommand(['ils', '-L', self.logical_path], 'STDOUT') # debugging


    def tearDown(self):
        self.user.run_icommand(['irm', '-f', self.logical_path])

        for parent, child in [(self.pt1, self.ufs1), (self.pt2, self.ufs2)]:
            if (lib.get_resource_parent_name(self.admin, child) == parent):
                lib.remove_child_resource(self.admin, parent, child)

        lib.remove_resource(self.admin, self.ufs2)
        lib.remove_resource(self.admin, self.ufs1)
        lib.remove_resource(self.admin, self.pt2)
        lib.remove_resource(self.admin, self.pt1)

        super(test_targeting_specific_replica_number__issue_6896, self).tearDown()


    def test_iget_n_returns_error_when_requested_replica_votes_0(self):
        local_iget_file = self.local_file + '_from_iget'

        if os.path.exists(local_iget_file):
            os.unlink(local_iget_file)

        # Overwrite the first replica with a different file so that the replicas can be differentiated.
        self.user.assert_icommand(['iput', '-f', '-n0', self.other_local_file, self.logical_path])
        self.assertEqual(
            str(self.other_file_size), lib.get_replica_size(self.user, os.path.basename(self.logical_path), 0))
        self.assertEqual(str(self.file_size), lib.get_replica_size(self.user, os.path.basename(self.logical_path), 1))
        self.assertEqual(str(1), lib.get_replica_status(self.user, os.path.basename(self.logical_path), 0))
        self.assertEqual(str(0), lib.get_replica_status(self.user, os.path.basename(self.logical_path), 1))

        # set read weight to 0.0 on passthru with replica 1
        self.admin.assert_icommand(['iadmin', 'modresc', self.pt2, 'context', 'read=0.0'])

        self.user.assert_icommand(['ils', '-L', self.logical_path], 'STDOUT') # debugging

        # execute iget -n 1, assert that an error is returned because the requested replica voted 0.
        self.user.assert_icommand(
            ['iget', '-n1', self.logical_path, local_iget_file], 'STDERR', 'SYS_REPLICA_INACCESSIBLE')
        self.assertFalse(os.path.exists(local_iget_file))


    def test_iget_n_always_returns_requested_replica(self):
        local_iget_file = self.local_file + '_from_iget'

        if os.path.exists(local_iget_file):
            os.unlink(local_iget_file)

        # Overwrite the first replica with a different file so that the replicas can be differentiated.
        self.user.assert_icommand(['iput', '-f', '-n0', self.other_local_file, self.logical_path])
        self.assertEqual(
            str(self.other_file_size), lib.get_replica_size(self.user, os.path.basename(self.logical_path), 0))
        self.assertEqual(str(self.file_size), lib.get_replica_size(self.user, os.path.basename(self.logical_path), 1))
        self.assertEqual(str(1), lib.get_replica_status(self.user, os.path.basename(self.logical_path), 0))
        self.assertEqual(str(0), lib.get_replica_status(self.user, os.path.basename(self.logical_path), 1))

        self.user.assert_icommand(['ils', '-L', self.logical_path], 'STDOUT') # debugging

        # set read weight to 0.1 (very low) on passthru with replica 1
        self.admin.assert_icommand(['iadmin', 'modresc', self.pt2, 'context', 'read=0.1'])

        # execute iget -n 1, assert that replica 1 is returned even though it will vote extremely low.
        self.user.assert_icommand(['iget', '-n1', self.logical_path, local_iget_file])
        assert_command(['diff', self.local_file, local_iget_file])


    def test_iput_n_returns_error_when_requested_replica_votes_0(self):
        try:
            # Set write weight to 0.0 on passthru with replica 1
            self.admin.assert_icommand(['iadmin', 'modresc', self.pt2, 'context', 'write=0.0'])

            # Execute iput -n 1 and assert that an error is returned because the requested replica voted 0.
            self.user.assert_icommand(
                ['iput', '-f', '-n1', self.other_local_file, self.logical_path], 'STDERR', 'SYS_REPLICA_INACCESSIBLE')
            self.assertEqual(
                str(self.file_size), lib.get_replica_size(self.user, os.path.basename(self.logical_path), 0))
            self.assertEqual(
                str(self.file_size), lib.get_replica_size(self.user, os.path.basename(self.logical_path), 1))
            self.assertEqual(str(1), lib.get_replica_status(self.user, os.path.basename(self.logical_path), 0))
            self.assertEqual(str(1), lib.get_replica_status(self.user, os.path.basename(self.logical_path), 1))

        finally:
            self.user.assert_icommand(['ils', '-L', self.logical_path], 'STDOUT') # debugging


    def test_iput_n_always_overwrites_requested_replica(self):
        try:
            # Set write weight to 0.1 on passthru with replica 1 so that it will vote the lowest
            self.admin.assert_icommand(['iadmin', 'modresc', self.pt2, 'context', 'write=0.1'])

            # Execute iput -n 1 and assert that replica 1 was overwritten in spite of voting lower than replica 0.
            self.user.assert_icommand(['iput', '-f', '-n1', self.other_local_file, self.logical_path])
            self.assertEqual(
                str(self.file_size), lib.get_replica_size(self.user, os.path.basename(self.logical_path), 0))
            self.assertEqual(
                str(self.other_file_size), lib.get_replica_size(self.user, os.path.basename(self.logical_path), 1))
            self.assertEqual(str(0), lib.get_replica_status(self.user, os.path.basename(self.logical_path), 0))
            self.assertEqual(str(1), lib.get_replica_status(self.user, os.path.basename(self.logical_path), 1))

        finally:
            self.user.assert_icommand(['ils', '-L', self.logical_path], 'STDOUT') # debugging


    def test_irepl_n_returns_error_when_requested_replica_votes_0(self):
        other_resc = 'other_resc'

        try:
            lib.create_ufs_resource(self.admin, other_resc, hostname=test.settings.HOSTNAME_1)

            # Modify replica status for the target so it can be differentiated.
            self.admin.assert_icommand([
                'iadmin', 'modrepl',
                'logical_path', self.logical_path,
                'replica_number', str(1),
                'DATA_REPL_STATUS', str(0)])
            self.assertEqual(str(1), lib.get_replica_status(self.user, os.path.basename(self.logical_path), 0))
            self.assertEqual(str(0), lib.get_replica_status(self.user, os.path.basename(self.logical_path), 1))

            # Set read weight to 0.0 on passthru with replica 1
            self.admin.assert_icommand(['iadmin', 'modresc', self.pt2, 'context', 'read=0.0'])

            # Replicate with replica 1 as the source replica. irepl should return an error and no replication occurs.
            self.user.assert_icommand(
                ['irepl', '-n1', '-R', other_resc, self.logical_path], 'STDERR', 'SYS_REPLICA_INACCESSIBLE')
            self.assertEqual(str(1), lib.get_replica_status(self.user, os.path.basename(self.logical_path), 0))
            self.assertEqual(str(0), lib.get_replica_status(self.user, os.path.basename(self.logical_path), 1))
            self.assertFalse(lib.replica_exists(self.user, self.logical_path, 2))

        finally:
            self.user.assert_icommand(['ils', '-L', self.logical_path], 'STDOUT') # debugging

            self.user.assert_icommand(['irm', '-f', self.logical_path])

            lib.remove_resource(self.admin, other_resc)


    def test_irepl_n_always_uses_requested_replica_as_source(self):
        other_resc = 'other_resc'

        try:
            lib.create_ufs_resource(self.admin, other_resc, hostname=test.settings.HOSTNAME_1)

            # Modify replica status for the target so it can be differentiated.
            self.admin.assert_icommand([
                'iadmin', 'modrepl',
                'logical_path', self.logical_path,
                'replica_number', str(1),
                'DATA_REPL_STATUS', str(0)])
            self.assertEqual(str(1), lib.get_replica_status(self.user, os.path.basename(self.logical_path), 0))
            self.assertEqual(str(0), lib.get_replica_status(self.user, os.path.basename(self.logical_path), 1))

            # Set read weight to 0.1 on passthru with replica 1 so that it will vote the lowest
            self.admin.assert_icommand(['iadmin', 'modresc', self.pt2, 'context', 'read=0.1'])

            # Replicate with replica 1 as the source replica. Replica status should match.
            self.user.assert_icommand(['irepl', '-n1', '-R', other_resc, self.logical_path])
            self.assertEqual(str(1), lib.get_replica_status(self.user, os.path.basename(self.logical_path), 0))
            self.assertEqual(str(0), lib.get_replica_status(self.user, os.path.basename(self.logical_path), 1))
            self.assertEqual(str(0), lib.get_replica_status(self.user, os.path.basename(self.logical_path), 2))

        finally:
            self.user.assert_icommand(['ils', '-L', self.logical_path], 'STDOUT') # debugging

            self.user.assert_icommand(['irm', '-f', self.logical_path])

            lib.remove_resource(self.admin, other_resc)


    def test_iphymv_n_returns_error_when_requested_replica_votes_0(self):
        other_resc = 'other_resc'

        try:
            lib.create_ufs_resource(self.admin, other_resc, hostname=test.settings.HOSTNAME_1)

            # Modify replica status for the target so it can be differentiated.
            self.admin.assert_icommand([
                'iadmin', 'modrepl',
                'logical_path', self.logical_path,
                'replica_number', str(1),
                'DATA_REPL_STATUS', str(0)])
            self.assertEqual(str(1), lib.get_replica_status(self.user, os.path.basename(self.logical_path), 0))
            self.assertEqual(str(0), lib.get_replica_status(self.user, os.path.basename(self.logical_path), 1))

            # Set read weight to 0.0 on passthru with replica 1
            self.admin.assert_icommand(['iadmin', 'modresc', self.pt2, 'context', 'read=0.0'])

            # Phymv with replica 1 as the source replica. iphymv should return an error and no phymv occurs.
            self.user.assert_icommand(
                ['iphymv', '-n1', '-R', other_resc, self.logical_path], 'STDERR', 'SYS_REPLICA_INACCESSIBLE')
            self.assertTrue(lib.replica_exists_on_resource(self.user, self.logical_path, self.ufs1))
            self.assertTrue(lib.replica_exists_on_resource(self.user, self.logical_path, self.ufs2))
            self.assertFalse(lib.replica_exists_on_resource(self.user, self.logical_path, other_resc))

        finally:
            self.user.assert_icommand(['ils', '-L', self.logical_path], 'STDOUT') # debugging

            self.user.assert_icommand(['irm', '-f', self.logical_path])

            lib.remove_resource(self.admin, other_resc)


    def test_iphymv_n_always_moves_requested_replica(self):
        other_resc = 'other_resc'

        try:
            lib.create_ufs_resource(self.admin, other_resc, hostname=test.settings.HOSTNAME_1)

            # Modify replica status for the target so it can be differentiated.
            self.admin.assert_icommand([
                'iadmin', 'modrepl',
                'logical_path', self.logical_path,
                'replica_number', str(1),
                'DATA_REPL_STATUS', str(0)])
            self.assertEqual(str(1), lib.get_replica_status(self.user, os.path.basename(self.logical_path), 0))
            self.assertEqual(str(0), lib.get_replica_status(self.user, os.path.basename(self.logical_path), 1))

            # Set read weight to 0.1 on passthru with replica 1 so that it will vote the lowest
            self.admin.assert_icommand(['iadmin', 'modresc', self.pt2, 'context', 'read=0.1'])

            # Replicate with replica 1 as the source replica. Replica status should match.
            self.user.assert_icommand(['iphymv', '-n1', '-R', other_resc, self.logical_path])
            self.assertEqual(str(1), lib.get_replica_status(self.user, os.path.basename(self.logical_path), 0))
            self.assertEqual(str(0), lib.get_replica_status(self.user, os.path.basename(self.logical_path), 1))
            self.assertTrue(lib.replica_exists_on_resource(self.user, self.logical_path, self.ufs1))
            self.assertFalse(lib.replica_exists_on_resource(self.user, self.logical_path, self.ufs2))
            self.assertTrue(lib.replica_exists_on_resource(self.user, self.logical_path, other_resc))

        finally:
            self.user.assert_icommand(['ils', '-L', self.logical_path], 'STDOUT') # debugging

            self.user.assert_icommand(['irm', '-f', self.logical_path])

            lib.remove_resource(self.admin, other_resc)


    def test_istream_n_returns_error_when_requested_replica_votes_0(self):
        content = 'test_istream_n_returns_error_when_requested_replica_votes_0'
        stream_error_msg = 'Error: Cannot open data object.'

        try:
            # Set write weight to 0.0 on passthru with replica 1
            self.admin.assert_icommand(['iadmin', 'modresc', self.pt2, 'context', 'write=0.0'])

            # Stream to the existing replica and confirm that it fails to update the contents.
            self.user.assert_icommand(
                ['istream', '-n1', 'write', self.logical_path], 'STDERR', stream_error_msg, input=content)
            self.assertEqual(
                str(self.file_size), lib.get_replica_size(self.user, os.path.basename(self.logical_path), 0))
            self.assertEqual(
                str(self.file_size), lib.get_replica_size(self.user, os.path.basename(self.logical_path), 1))

            # Set read weight to 0.0 on passthru with replica 1
            self.admin.assert_icommand(['iadmin', 'modresc', self.pt2, 'context', 'read=0.0'])

            # Stream to the existing replica and confirm that the contents are updated.
            self.user.assert_icommand(['istream', '-n1', 'write', self.logical_path], input=content)
            self.assertEqual(
                str(self.file_size), lib.get_replica_size(self.user, os.path.basename(self.logical_path), 0))
            self.assertEqual(
                str(len(content)), lib.get_replica_size(self.user, os.path.basename(self.logical_path), 1))
            self.assertEqual(str(0), lib.get_replica_status(self.user, os.path.basename(self.logical_path), 0))
            self.assertEqual(str(1), lib.get_replica_status(self.user, os.path.basename(self.logical_path), 1))

            # Read from the existing replica and confirm that it fails to be opened due to the read weight.
            self.user.assert_icommand(['istream', '-n1', 'read', self.logical_path], 'STDERR', stream_error_msg)

        finally:
            self.user.assert_icommand(['ils', '-L', self.logical_path], 'STDOUT') # debugging


    def test_istream_n_always_overwrites_and_reads_requested_replica(self):
        content = 'test_istream_n_returns_error_when_requested_replica_votes_0'

        try:
            # Set write weight to 0.1 on passthru with replica 1 so that it will vote the lowest
            self.admin.assert_icommand(['iadmin', 'modresc', self.pt2, 'context', 'write=0.1'])

            # Stream to the existing replica and confirm that the contents are updated.
            self.user.assert_icommand(['istream', '-n1', 'write', self.logical_path], input=content)
            self.assertEqual(
                str(self.file_size), lib.get_replica_size(self.user, os.path.basename(self.logical_path), 0))
            self.assertEqual(
                str(len(content)), lib.get_replica_size(self.user, os.path.basename(self.logical_path), 1))
            self.assertEqual(str(0), lib.get_replica_status(self.user, os.path.basename(self.logical_path), 0))
            self.assertEqual(str(1), lib.get_replica_status(self.user, os.path.basename(self.logical_path), 1))

            # Set read weight to 0.1 on passthru with replica 1 so that it will vote the lowest
            self.admin.assert_icommand(['iadmin', 'modresc', self.pt2, 'context', 'read=0.1'])

            # Read from the existing replica and confirm that it opens the correct replica.
            self.user.assert_icommand(['istream', '-n1', 'read', self.logical_path], 'STDOUT', content)

        finally:
            self.user.assert_icommand(['ils', '-L', self.logical_path], 'STDOUT') # debugging


    def test_ichksum_n_returns_error_when_requested_replica_votes_0__issue_7133(self):
        data_name = os.path.basename(self.logical_path)

        # Assert that no checksum is registered in the catalog for any replica.
        self.assertEqual(len(lib.get_replica_checksum(self.user, data_name, 0)), 0)
        self.assertEqual(len(lib.get_replica_checksum(self.user, data_name, 1)), 0)

        # set read weight to 0.0 on passthru with replica 1
        self.admin.assert_icommand(['iadmin', 'modresc', self.pt2, 'context', 'read=0.0'])

        self.user.assert_icommand(['ils', '-L', self.logical_path], 'STDOUT') # debugging

        # execute ichksum -n 1, assert that an error is returned because the requested replica voted 0.
        self.user.assert_icommand(
            ['ichksum', '-n1', self.logical_path], 'STDERR', 'SYS_REPLICA_INACCESSIBLE')

        # Assert that no checksum was registered in the catalog for the target or any other replica.
        self.assertEqual(len(lib.get_replica_checksum(self.user, data_name, 0)), 0)
        self.assertEqual(len(lib.get_replica_checksum(self.user, data_name, 1)), 0)


    def test_ichksum_n_always_calculates_checksum_for_requested_replica__issue_7133(self):
        data_name = os.path.basename(self.logical_path)

        # Assert that no checksum is registered in the catalog for this replica.
        self.assertEqual(len(lib.get_replica_checksum(self.user, data_name, 0)), 0)
        self.assertEqual(len(lib.get_replica_checksum(self.user, data_name, 1)), 0)

        # set read weight to 0.1 (very low) on passthru with replica 1
        self.admin.assert_icommand(['iadmin', 'modresc', self.pt2, 'context', 'read=0.1'])

        self.user.assert_icommand(['ils', '-L', self.logical_path], 'STDOUT') # debugging

        # execute ichksum -n 1, assert that replica 1 has a checksum even though it will vote extremely low.
        self.user.assert_icommand(['ichksum', '-n1', self.logical_path], 'STDOUT', '    sha2:')

        # Assert that replica 0 has no checksum and replica 1 has a checksum.
        self.assertEqual(len(lib.get_replica_checksum(self.user, data_name, 0)), 0)
        self.assertGreater(len(lib.get_replica_checksum(self.user, data_name, 1)), 0)


    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'Only implemented for NREP.')
    def test_msiDataObjChksum_with_replNum_keyword__issue_7133(self):
        # Note: If this is ever implemented for PREP, this will have to be modified to add the rule to the configured
        # rulebase and execute the rule by name rather than directly executing rule text as is being done here.
        rule_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                msiDataObjChksum(*file, "replNum=*repl_num++++forceChksum=", *checksum);
                writeLine("stdout", "file: [*file], checksum: [*checksum]");
            ''')
        }

        rep_intance = plugin_name + '-instance'

        rule_text = rule_map[plugin_name]
        partial_expected_output = 'file: [{}], checksum:'.format(self.logical_path) # Not testing checksum accuracy

        data_name = os.path.basename(self.logical_path)

        self.user.assert_icommand(['ils', '-L', self.logical_path], 'STDOUT') # debugging

        # Assert that no checksum is registered in the catalog for this replica.
        self.assertEqual(len(lib.get_replica_checksum(self.user, data_name, 0)), 0)
        self.assertEqual(len(lib.get_replica_checksum(self.user, data_name, 1)), 0)

        # set read weight to 0.1 (very low) on passthru with replica 1
        self.admin.assert_icommand(['iadmin', 'modresc', self.pt2, 'context', 'read=0.1'])

        self.user.assert_icommand(
            ['irule', '-r', rep_intance, rule_text, '*file={}%*repl_num={}'.format(self.logical_path, 1), 'ruleExecOut'],
            'STDOUT', partial_expected_output)

        self.user.assert_icommand(['ils', '-L', self.logical_path], 'STDOUT') # debugging

        # Assert that replica 0 has no checksum and replica 1 has a checksum.
        self.assertEqual(len(lib.get_replica_checksum(self.user, data_name, 0)), 0)
        self.assertGreater(len(lib.get_replica_checksum(self.user, data_name, 1)), 0)

        # set read weight to 0.0 on passthru with replica 0
        self.admin.assert_icommand(['iadmin', 'modresc', self.pt1, 'context', 'read=0.0'])

        # Try generating a checksum on replica 0 and ensure that this fails.
        self.user.assert_icommand(
            ['irule', '-r', rep_intance, rule_text, '*file={}%*repl_num={}'.format(self.logical_path, 0), 'ruleExecOut'],
            'STDERR', 'SYS_REPLICA_INACCESSIBLE')

        # Assert that replica 0 has no checksum and replica 1 has a checksum.
        self.assertEqual(len(lib.get_replica_checksum(self.user, data_name, 0)), 0)
        self.assertGreater(len(lib.get_replica_checksum(self.user, data_name, 1)), 0)
