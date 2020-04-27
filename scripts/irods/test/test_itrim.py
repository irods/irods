from __future__ import print_function

import os
import sys
import shutil

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from .. import test
from .. import lib
from ..configuration import IrodsConfig

class Test_Itrim(session.make_sessions_mixin([('otherrods', 'rods')], []), unittest.TestCase):

    def setUp(self):
        super(Test_Itrim, self).setUp()
        self.admin = self.admin_sessions[0]

    def tearDown(self):
        super(Test_Itrim, self).tearDown()

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_itrim_num_copies_repl_num_conflict__issue_3346(self):
        hostname = IrodsConfig().client_environment['irods_host']
        replicas = 6

        filename = 'test_file_issue_3346.txt'
        lib.make_file(filename, 1024)

        # Create resources [resc_{0..5}].
        for i in range(replicas):
            vault = os.path.join(self.admin.local_session_dir, 'issue_3346_storage_vault_' + str(i))
            path = hostname + ':' + vault
            self.admin.assert_icommand('iadmin mkresc resc_{0} unixfilesystem {1}'.format(i, path), 'STDOUT_SINGLELINE', ['unixfilesystem'])

        try:
            self.admin.assert_icommand('iput -R resc_0 {0}'.format(filename))

            # Replicate the file across all resources.
            for i in range(1, replicas):
                self.admin.assert_icommand('irepl -S resc_0 -R resc_{0} {1}'.format(i, filename))

            # Make all replicas stale except two.
            self.admin.assert_icommand('iput -fR resc_0 {0}'.format(filename))
            self.admin.assert_icommand('irepl -R resc_5 {0}'.format(filename))

            # Here are the actual tests.

            # Error cases.
            self.admin.assert_icommand('itrim -S resc_1 -n3 {0}'.format(filename), 'STDERR', 'status = -402000 USER_INCOMPATIBLE_PARAMS')
            self.admin.assert_icommand('itrim -N2 -n0 {0}'.format(filename), 'STDERR', 'status = -402000 USER_INCOMPATIBLE_PARAMS')
            self.admin.assert_icommand('itrim -N9 -n0 {0}'.format(filename), 'STDERR', 'status = -402000 USER_INCOMPATIBLE_PARAMS')
            self.admin.assert_icommand('itrim -n0 {0}'.format(filename), 'STDERR', 'status = -402000 USER_INCOMPATIBLE_PARAMS')

            self.admin.assert_icommand('itrim -S invalid_resc {0}'.format(filename), 'STDERR', 'status = -78000 SYS_RESC_DOES_NOT_EXIST')
            self.admin.assert_icommand('itrim -n999 {0}'.format(filename), 'STDERR', 'status = -164000 SYS_REPLICA_DOES_NOT_EXIST')
            self.admin.assert_icommand('itrim -n-1 {0}'.format(filename), 'STDERR', 'status = -164000 SYS_REPLICA_DOES_NOT_EXIST')
            self.admin.assert_icommand('itrim -nX {0}'.format(filename), 'STDERR', 'status = -403000 USER_INVALID_REPLICA_INPUT')

            # No error cases.
            self.admin.assert_icommand('itrim -N2 -S resc_1 {0}'.format(filename), 'STDOUT', 'files trimmed = 1')
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT', filename)
            self.admin.assert_icommand('itrim -S resc_2 {0}'.format(filename), 'STDOUT', 'files trimmed = 1')
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT', filename)

            self.admin.assert_icommand('itrim -N4 {0}'.format(filename), 'STDOUT', 'files trimmed = 1')
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT', filename)
            self.admin.assert_icommand('itrim -N2 {0}'.format(filename), 'STDOUT', 'files trimmed = 0')
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT', filename)
            self.admin.assert_icommand('itrim -N1 {0}'.format(filename), 'STDOUT', 'files trimmed = 1')
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT', filename)

            self.admin.assert_icommand('itrim {0}'.format(filename), 'STDOUT', 'files trimmed = 0')
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT', filename)

        finally:
            # Clean up.
            self.admin.assert_icommand('irm -f {0}'.format(filename))
            for i in range(replicas):
                self.admin.assert_icommand('iadmin rmresc resc_{0}'.format(i))

    def test_itrim_option_N_is_deprecated__issue_4860(self):
        data_object = 'foo'
        filename = os.path.join(self.admin.local_session_dir, data_object)
        lib.make_file(filename, 1)
        self.admin.assert_icommand(['iput', filename])
        self.admin.assert_icommand(['itrim', '-N2', data_object], 'STDOUT', 'Specifying a minimum number of replicas to keep is deprecated.')

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_itrim_removes_in_vault_registered_replicas_from_disk__issue_5362(self):
        resc_name = 'issue_5362_resc'
        logical_path = os.path.join(self.admin.session_collection, 'issue_5362')

        try:
            # Create a resource.
            hostname = IrodsConfig().client_environment['irods_host']
            vault_path = '/tmp/issue_5362_vault'
            os.mkdir(vault_path)
            # The important part here is the trailing slash on the vault path. The trailing slash
            # causes the server to think the replica is not in a vault.
            self.admin.assert_icommand(['iadmin', 'mkresc', resc_name, 'unixfilesystem', hostname + ':' + vault_path + '/'], 'STDOUT', ['unixfilesystem'])

            # Create a tree of files.
            dir_1 = os.path.join(vault_path, 'dir_1')
            os.mkdir(dir_1)
            file_1 = os.path.join(dir_1, 'file_1')
            open(file_1, 'w').close()

            dir_2 = os.path.join(vault_path, 'dir_2')
            os.mkdir(dir_2)
            file_2 = os.path.join(dir_2, 'file_2')
            open(file_2, 'w').close()

            # Register the tree into iRODS.
            self.admin.assert_icommand(['ireg', '-r', '-R', resc_name, vault_path, logical_path])

            # Show that the tree has been registered.
            dir_1_logical_path = os.path.join(logical_path, 'dir_1')
            self.admin.assert_icommand(['ils', '-Lr', logical_path], 'STDOUT', [
                logical_path,
                dir_1_logical_path,
                os.path.join(logical_path, 'dir_1'),
                '& file_1',
                '& file_2',
                file_1,
                file_2
            ])

            # Replicate to demoResc.
            data_object = os.path.join(logical_path, 'dir_1', 'file_1')
            self.admin.assert_icommand(['irepl', '-R', 'demoResc', data_object])

            # Show that the data object now has two replicas.
            self.admin.assert_icommand(['ils', '-l', data_object], 'STDOUT', ['0 ' + resc_name, '1 demoResc'])

            # Trim the original replica.
            self.admin.assert_icommand(['itrim', '-N', '1', '-n', '0', data_object], 'STDOUT', ['files trimmed = 1'])

            # Show that replica 0 has been unregistered from the catalog.
            gql = "select DATA_REPL_NUM where COLL_NAME = '{0}' and DATA_NAME = 'file_1' and DATA_REPL_NUM = '0'".format(dir_1_logical_path)
            self.admin.assert_icommand(['iquest', gql], 'STDOUT', ['CAT_NO_ROWS_FOUND'])

            # Show that replica 1 still exists.
            gql = "select DATA_REPL_NUM where COLL_NAME = '{0}' and DATA_NAME = 'file_1' and DATA_REPL_NUM = '1'".format(dir_1_logical_path)
            self.admin.assert_icommand(['iquest', gql], 'STDOUT', ['DATA_REPL_NUM = 1'])

            # Show that the original replica has been removed from the disk.
            self.assertFalse(os.path.exists(file_1))
        finally:
            self.admin.run_icommand(['irm', '-rf', logical_path])
            self.admin.run_icommand(['iadmin', 'rmresc', resc_name])
            shutil.rmtree(vault_path)

    def test_itrim_returns_on_negative_status__issue_3531(self):
        resc1 = 'issue_3531_1'
        resc2 = 'issue_3531_2'
        filename = 'test_itrim_returns_on_negative_status__ticket_3531'
        logical_path = os.path.join(self.admin.session_collection, filename)

        try:
            lib.create_ufs_resource(self.admin, resc1, test.settings.HOSTNAME_2)
            lib.create_ufs_resource(self.admin, resc2, test.settings.HOSTNAME_3)

            # Create a data object
            self.admin.assert_icommand(['itouch', '-R', resc1, logical_path])
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, resc1))

            # Replicate to another resource so that we have something to trim
            self.admin.assert_icommand(['irepl', '-R', resc2, logical_path])
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, resc2))

            # Use incompatible parameters -n and -S
            rc,_,_ = self.admin.assert_icommand(['itrim', '-n0', '-S', resc2, logical_path],
                                                'STDERR', 'USER_INCOMPATIBLE_PARAMS')
            self.assertNotEqual(rc, 0, 'itrim should have non-zero error code on trim failure')

            # Ensure that both replicas still exist and that neither was trimmed
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, resc1))
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, resc2))

        finally:
            self.admin.run_icommand(['irm', '-f', logical_path])
            lib.remove_resource(self.admin, resc1)
            lib.remove_resource(self.admin, resc2)


    def test_itrim_displays_incorrect_count__issue_3531(self):
        resc1 = 'issue_3531_1'
        resc2 = 'issue_3531_2'

        filename = 'test_itrim_displays_incorrect_count__issue_3531'
        filepath = os.path.join(self.admin.local_session_dir, filename)
        filesize = int(pow(2, 20) + pow(10,5))
        filesizeMB = round(float(filesize)/1048576, 3)
        lib.make_file(filepath, filesize)

        logical_path = os.path.join(self.admin.session_collection, filename)

        try:
            lib.create_ufs_resource(self.admin, resc1, test.settings.HOSTNAME_2)
            lib.create_ufs_resource(self.admin, resc2, test.settings.HOSTNAME_3)

            # Create the data object.
            self.admin.assert_icommand(['iput', '-R', resc1, filepath, logical_path])
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, resc1))

            # Replicate to another resource so that we have something to trim.
            self.admin.assert_icommand(['irepl', '-R', resc2, logical_path])
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, resc2))

            # Trim down to 1 replica, targeting the replica on resc1 and ensure the correct
            # size is displayed in the resulting output.
            self.admin.assert_icommand(['itrim', '-N1', '-S', resc1, logical_path], 'STDOUT',
                                       'Total size trimmed = {} MB. Number of files trimmed = 1.'.format(str(filesizeMB)))

            # Ensure that the replica on resc1 was trimmed and the replica on resc2 remains.
            self.assertFalse(lib.replica_exists_on_resource(self.admin, logical_path, resc1))
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, resc2))

        finally:
            self.admin.run_icommand(['irm', '-f', logical_path])
            lib.remove_resource(self.admin, resc1)
            lib.remove_resource(self.admin, resc2)
