from __future__ import print_function
import filecmp
import os
import stat
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from .. import lib
from . import session
from .. import test
from ..test.command import assert_command

class test_iget_general(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):
    def setUp(self):
        super(test_iget_general, self).setUp()
        self.admin = self.admin_sessions[0]
        self.user = self.user_sessions[0]

    def tearDown(self):
        super(test_iget_general, self).tearDown()

    def test_multi_1247_get_retains_replica_selection_and_checksum(self):
        source = os.path.join(self.user.local_session_dir, 'multi_1247_source')
        destination = os.path.join(self.user.local_session_dir, 'multi_1247_destination')
        legacy_destination = os.path.join(self.user.local_session_dir, 'multi_1247_legacy')
        logical_path = os.path.join(self.user.session_collection, 'multi_1247_object')
        # Nonuniform bytes catch incorrect segment offsets and ordering.
        lib.make_file(source, 34 * 1024 * 1024, 'random')
        self.user.assert_icommand(['iput', '-N', '0', '-k', source, logical_path])

        stdout, stderr, status = self.user.run_icommand(
            ['iget', '-P', '-V', '-N', '3', '-K', '-n', '0', logical_path, destination])
        self.assertEqual(status, 0, stderr)
        self.assertIn('multi-1247 transfer', stdout)
        self.assertTrue(filecmp.cmp(source, destination, shallow=False))
        self.user.assert_icommand(['iget', '-N', '0', logical_path, legacy_destination])
        self.assertEqual(stat.S_IMODE(os.stat(legacy_destination).st_mode),
                         stat.S_IMODE(os.stat(destination).st_mode))
        self.assertEqual('1', lib.get_replica_status(self.user,
                                                     os.path.basename(logical_path), 0))

        _, stderr, status = self.user.run_icommand(
            ['iget', '-N', '2', logical_path, destination])
        self.assertNotEqual(status, 0)
        self.assertIn('OVERWRITE_WITHOUT_FORCE_FLAG', stderr)

        stdout, stderr, status = self.user.run_icommand(
            ['iget', '-V', '-f', logical_path, destination])
        self.assertEqual(status, 0, stderr)
        self.assertIn('multi-1247 transfer', stdout)
        self.assertTrue(filecmp.cmp(source, destination, shallow=False))

        stdout, stderr, status = self.user.run_icommand(
            ['iget', '-V', '-f', '-N', '2', '-R', self.user.default_resource,
             logical_path, destination])
        self.assertEqual(status, 0, stderr)
        self.assertIn('multi-1247 transfer', stdout)
        self.assertTrue(filecmp.cmp(source, destination, shallow=False))

        _, output, _ = self.user.assert_icommand(
            ['iticket', 'create', 'read', logical_path], 'STDOUT', 'ticket:')
        ticket = output.rpartition(':')[2].strip()
        try:
            stdout, stderr, status = self.user.run_icommand(
                ['iget', '-V', '-f', '-N', '2', '-t', ticket, logical_path, destination])
            self.assertEqual(status, 0, stderr)
            self.assertIn('multi-1247 transfer', stdout)
            self.assertTrue(filecmp.cmp(source, destination, shallow=False))
        finally:
            self.user.assert_icommand(['iticket', 'delete', ticket])

        stdout, stderr, status = self.user.run_icommand(
            ['iget', '-V', '-f', '-N', '0', logical_path, destination])
        self.assertEqual(status, 0, stderr)
        self.assertNotIn('multi-1247 transfer', stdout)
        self.assertTrue(filecmp.cmp(source, destination, shallow=False))

        self.admin.assert_icommand(['iadmin', 'modrepl', 'logical_path', logical_path,
                                    'replica_number', '0', 'DATA_REPL_STATUS', '0'])
        try:
            # A stale replica may have a catalog size which differs from its
            # physical length. It must be read until EOF, not divided into ranges.
            stdout, stderr, status = self.user.run_icommand(
                ['iget', '-V', '-f', '-R', self.user.default_resource,
                 logical_path, destination])
            self.assertEqual(status, 0, stderr)
            self.assertNotIn('multi-1247 transfer', stdout)
            self.assertTrue(filecmp.cmp(source, destination, shallow=False))
        finally:
            self.admin.assert_icommand(['iadmin', 'modrepl', 'logical_path', logical_path,
                                        'replica_number', '0', 'DATA_REPL_STATUS', '1'])

    def test_iget_R_is_a_directive_not_a_preference__issue_4475(self):
        def do_test_iget_R_is_a_directive_not_a_preference__issue_4475(self, file_size):
            filename = 'test_iget_R_is_a_directive_not_a_preference__issue_4475'
            local_file = os.path.join(self.user.local_session_dir, filename)
            local_iget_file = local_file + '_from_iget'
            logical_path = os.path.join(self.user.session_collection, filename)
            resource_1 = 'resc1'
            resource_2 = 'resc2'
            root_resource = 'pt1'

            try:
                lib.create_passthru_resource(self.admin, root_resource)
                lib.create_ufs_resource(self.admin, resource_1, test.settings.HOSTNAME_2)
                lib.create_ufs_resource(self.admin, resource_2, test.settings.HOSTNAME_3)
                lib.add_child_resource(self.admin, root_resource, resource_1)

                if not os.path.exists(local_file):
                    lib.make_file(local_file, file_size, 'arbitrary')

                if os.path.exists(local_iget_file):
                    os.unlink(local_iget_file)

                self.user.assert_icommand(['iput', '-R', root_resource, local_file, logical_path])

                # resource which does not exist - failure
                self.user.assert_icommand(['iget', '-R', 'nope', logical_path], 'STDERR', 'SYS_RESC_DOES_NOT_EXIST')

                # leaf resource in a hierarchy - failure
                self.user.assert_icommand(['iget', '-R', resource_1, logical_path], 'STDERR', 'DIRECT_CHILD_ACCESS')

                # resource which does not host any replicas - failure
                self.user.assert_icommand(['iget', '-R', resource_2, logical_path], 'STDERR', 'SYS_REPLICA_INACCESSIBLE')

                # specify the resource on which the replica actually resides - success
                self.user.assert_icommand(['iget', '-R', root_resource, logical_path, local_iget_file])
                assert_command(['diff', local_file, local_iget_file])

                # overwrite local file with an unspecified resource - success
                self.user.assert_icommand(['iget', '-f', logical_path, local_iget_file])
                assert_command(['diff', local_file, local_iget_file])

            finally:
                self.user.run_icommand(['irm', '-f', logical_path])
                lib.remove_child_resource(self.admin, root_resource, resource_1)
                lib.remove_resource(self.admin, root_resource)
                lib.remove_resource(self.admin, resource_1)
                lib.remove_resource(self.admin, resource_2)

        do_test_iget_R_is_a_directive_not_a_preference__issue_4475(self, 1024)
        do_test_iget_R_is_a_directive_not_a_preference__issue_4475(self, 40 * 1024 * 1024)

    def test_iget_correctly_handles_zero_length_file__issue_5723(self):
        # Create a new empty data object.
        data_object = 'foo'
        self.user.assert_icommand(['itouch', data_object])
        self.assertEqual(0, int(lib.get_replica_size(self.user, data_object, 0)))

        # Show that "iget" produces zero bytes.
        self.user.assert_icommand(['iget', data_object, '-'])

        # Show that "iget" produces a zero length file.
        file_path = os.path.join(self.user.local_session_dir, 'foo')
        self.user.assert_icommand(['iget', data_object, file_path])
        self.assertEqual(0, os.path.getsize(file_path))

    def test_iget_does_not_change_replica_status__issue_5760(self):
        # Create a new data object.
        filename = 'foo'
        local_file = os.path.join(self.admin.local_session_dir, filename)
        logical_path = os.path.join(self.admin.session_collection, filename)
        lib.create_local_testfile(local_file)
        self.admin.assert_icommand(['iput', local_file, logical_path])

        # Get the physical path for the replica
        out, err, rc = self.admin.run_icommand(['iquest', '%s',
            "select DATA_PATH where COLL_NAME = '{0}' and DATA_NAME = '{1}'".format(
            os.path.dirname(logical_path), os.path.basename(logical_path))])

        self.assertEqual(0, rc)
        self.assertEqual('', err)

        # Write down the physical path and make up a fake physical path
        real_data_path = out
        fake_data_path = os.path.join(self.admin.local_session_dir, 'jimbo')

        try:
            # Modify the data path to something wrong so that the get will fail
            self.admin.assert_icommand(['iadmin', 'modrepl', 'logical_path', logical_path, 'replica_number', '0', 'DATA_PATH', fake_data_path])

            # Assert that the get fails as it should
            self.admin.assert_icommand(['iget', logical_path, '-'], 'STDERR', 'UNIX_FILE_OPEN_ERR')

            # Assert that the status of the replica has not changed
            out, err, rc = self.admin.run_icommand(['iquest', '%s', "select DATA_REPL_STATUS where DATA_PATH = '{}'".format(fake_data_path)])
            self.assertEqual(0, rc)
            self.assertEqual('', err)
            self.assertEqual(1, int(out))

        finally:
            # Make sure the data path is real so that it will get cleaned up
            self.admin.assert_icommand(['iadmin', 'modrepl', 'logical_path', logical_path, 'replica_number', '0', 'DATA_PATH', real_data_path])
