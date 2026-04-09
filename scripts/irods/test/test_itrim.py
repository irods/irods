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
from .. import paths
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

            self.admin.assert_icommand('itrim -S invalid_resc {0}'.format(filename), 'STDERR', 'status = -78000 SYS_RESC_DOES_NOT_EXIST')
            self.admin.assert_icommand('itrim -n999 {0}'.format(filename), 'STDERR', 'status = -164000 SYS_REPLICA_DOES_NOT_EXIST')
            self.admin.assert_icommand('itrim -n-1 {0}'.format(filename), 'STDERR', 'status = -164000 SYS_REPLICA_DOES_NOT_EXIST')
            self.admin.assert_icommand('itrim -nX {0}'.format(filename), 'STDERR', 'status = -403000 USER_INVALID_REPLICA_INPUT')

            # No error cases.
            # In this case, no replicas are trimmed because the specified minimum number of replicas to keep is higher
            # than the number of replicas that exist.
            self.admin.assert_icommand('itrim -N9 -n0 {0}'.format(filename), 'STDOUT', 'data objects trimmed = 0')
            # In this case, replica 0 is trimmed because the replica on resc_5 is still good so at least one good
            # replica would remain after trimming, and the minimum number of replicas to keep is lower than the number
            # of replicas that exist.
            self.admin.assert_icommand('itrim -N2 -n0 {0}'.format(filename), 'STDOUT', 'data objects trimmed = 1')
            self.admin.assert_icommand('itrim -N2 -S resc_1 {0}'.format(filename), 'STDOUT', 'data objects trimmed = 1')
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT', filename)
            self.admin.assert_icommand('itrim -S resc_2 {0}'.format(filename), 'STDOUT', 'data objects trimmed = 1')
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT', filename)

            self.admin.assert_icommand('itrim -N2 {0}'.format(filename), 'STDOUT', 'data objects trimmed = 1')
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT', filename)
            self.admin.assert_icommand('itrim -N2 {0}'.format(filename), 'STDOUT', 'data objects trimmed = 0')
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT', filename)
            self.admin.assert_icommand('itrim -N1 {0}'.format(filename), 'STDOUT', 'data objects trimmed = 1')
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT', filename)

            self.admin.assert_icommand('itrim {0}'.format(filename), 'STDOUT', 'data objects trimmed = 0')
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT', filename)

        finally:
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT', filename) # debugging
            # Clean up.
            self.admin.assert_icommand('irm -f {0}'.format(filename))
            for i in range(replicas):
                self.admin.assert_icommand('iadmin rmresc resc_{0}'.format(i))

    def test_itrim_option_N_is_deprecated__issue_4860(self):
        original_resource = 'issue_4860_0'
        resc1 = 'issue_4860_1'
        resc2 = 'issue_4860_2'
        filename = 'test_itrim_option_N_is_deprecated__issue_4860'
        logical_path = os.path.join(self.admin.session_collection, filename)

        try:
            lib.create_ufs_resource(self.admin, original_resource, test.settings.HOSTNAME_1)
            lib.create_ufs_resource(self.admin, resc1, test.settings.HOSTNAME_2)
            lib.create_ufs_resource(self.admin, resc2, test.settings.HOSTNAME_3)

            # Create 3 replicas so that a trim will actually occur.
            self.admin.assert_icommand(['itouch', '-R', original_resource, logical_path])
            self.admin.assert_icommand(['irepl', '-R', resc1, logical_path])
            self.admin.assert_icommand(['irepl', '-R', resc2, logical_path])
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, original_resource))
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, resc1))
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, resc2))

            # Ensure the deprecation message appears when a replica is not trimmed.
            self.admin.assert_icommand(['itrim', '-N4', logical_path], 'STDOUT', 'data objects trimmed = 0')

            # Ensure the deprecation message appears when a replica is trimmed.
            self.admin.assert_icommand(['itrim', '-N2', logical_path], 'STDOUT', 'data objects trimmed = 1')

        finally:
            self.admin.assert_icommand(['ils', '-l', logical_path], 'STDOUT', filename) # debugging
            self.admin.run_icommand(['irm', '-f', logical_path])
            lib.remove_resource(self.admin, original_resource)
            lib.remove_resource(self.admin, resc1)
            lib.remove_resource(self.admin, resc2)

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
            self.admin.assert_icommand(['itrim', '-N', '1', '-n', '0', data_object], 'STDOUT', ['data objects trimmed = 1'])

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
                                       'Total size trimmed = {} MB. Number of data objects trimmed = 1.'.format(str(filesizeMB)))

            # Ensure that the replica on resc1 was trimmed and the replica on resc2 remains.
            self.assertFalse(lib.replica_exists_on_resource(self.admin, logical_path, resc1))
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, resc2))

        finally:
            self.admin.run_icommand(['irm', '-f', logical_path])
            lib.remove_resource(self.admin, resc1)
            lib.remove_resource(self.admin, resc2)

    def test_itrim_does_not_remove_the_last_replica__issue_7468(self):
        resc1 = 'issue_7468_1'
        resc2 = 'issue_7468_2'

        filename = 'test_itrim_does_not_remove_the_last_replica__issue_7468'
        logical_path = os.path.join(self.admin.session_collection, filename)

        try:
            lib.create_ufs_resource(self.admin, resc1, test.settings.HOSTNAME_2)
            lib.create_ufs_resource(self.admin, resc2, test.settings.HOSTNAME_3)

            # Create the data object.
            self.admin.assert_icommand(['itouch', '-R', resc1, logical_path])
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, resc1))

            # Replicate to another resource so that we have something to trim.
            self.admin.assert_icommand(['irepl', '-R', resc2, logical_path])
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, resc2))

            # Now make sure that one of the replicas is stale...
            lib.set_replica_status(self.admin, logical_path, 0, 0)
            self.admin.assert_icommand(['ils', '-l', logical_path], 'STDOUT', filename) # debugging

            # Try trimming replica 1, which is marked "good". Ensure that it fails because it is the last good replica.
            self.admin.assert_icommand(['itrim', '-N1', '-n1', logical_path], 'STDERR', 'USER_INCOMPATIBLE_PARAMS')
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, resc1))
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, resc2))

            # Now mark the other replica as stale.
            lib.set_replica_status(self.admin, logical_path, 1, 0)
            self.admin.assert_icommand(['ils', '-l', logical_path], 'STDOUT', filename) # debugging

            # Try to trim replica 0 with no minimum number of replicas to keep specified. This does nothing because the
            # default minimum number of replicas to keep is 2 and there are only 2 replicas. An error is not expected
            # here because the client does not direct the API to do anything that it cannot carry out. The minimum
            # number of replicas required (2) is still being met.
            self.admin.assert_icommand(['itrim', '-n0', logical_path], 'STDOUT', 'Number of data objects trimmed = 0.')
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, resc1))
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, resc2))

            # Trim replica 0 and specify a minimum number of replicas to keep of 1. This should succeed because there is
            # still another replica and the replica being trimmed is stale.
            self.admin.assert_icommand(['itrim', '-N1', '-n0', logical_path], 'STDOUT', 'Number of data objects trimmed = 1.')
            self.assertFalse(lib.replica_exists_on_resource(self.admin, logical_path, resc1))
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, resc2))

            # Try trimming replica 0, which is now "stale". Ensure that it fails because it is the last replica.
            self.admin.assert_icommand(['itrim', '-N1', '-n1', logical_path], 'STDOUT', 'Number of data objects trimmed = 0.')
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, resc2))

        finally:
            self.admin.assert_icommand(['ils', '-l', logical_path], 'STDOUT', filename) # debugging
            self.admin.run_icommand(['irm', '-f', logical_path])
            lib.remove_resource(self.admin, resc1)
            lib.remove_resource(self.admin, resc2)

    def test_itrim_minimum_replicas_to_keep_invalid_values__issue_7502(self):
        original_resource = 'test_itrim_minimum_replicas_to_keep_invalid_values_resc_0'
        resc1 = 'test_itrim_minimum_replicas_to_keep_invalid_values_resc_1'
        resc2 = 'test_itrim_minimum_replicas_to_keep_invalid_values_resc_2'

        filename = 'test_itrim_minimum_replicas_to_keep_invalid_values'
        logical_path = os.path.join(self.admin.session_collection, filename)

        try:
            lib.create_ufs_resource(self.admin, original_resource, test.settings.HOSTNAME_1)
            lib.create_ufs_resource(self.admin, resc1, test.settings.HOSTNAME_2)
            lib.create_ufs_resource(self.admin, resc2, test.settings.HOSTNAME_3)

            # Create the data object.
            self.admin.assert_icommand(['itouch', '-R', original_resource, logical_path])

            # Replicate to two other resources so that we have something to trim, even with the default minimum.
            self.admin.assert_icommand(['irepl', '-R', resc1, logical_path])
            self.admin.assert_icommand(['irepl', '-R', resc2, logical_path])
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, original_resource))
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, resc1))
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, resc2))

            # -N cannot be 0.
            self.admin.assert_icommand(['itrim', '-N0', logical_path], 'STDERR', 'SYS_INVALID_INPUT_PARAM')
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, original_resource))
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, resc1))
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, resc2))

            # -N cannot be negative.
            self.admin.assert_icommand(['itrim', '-N"-1"', logical_path], 'STDERR', 'SYS_INVALID_INPUT_PARAM')
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, original_resource))
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, resc1))
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, resc2))

            # -N cannot overflow. Maximum value is INT32_MAX (2^31 - 1 = 2147483647). Should be enough for anyone.
            self.admin.assert_icommand(
                ['itrim', '-N2147483648', logical_path], 'STDERR', 'SYS_INVALID_INPUT_PARAM')
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, original_resource))
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, resc1))
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, resc2))

            # -N must be a number.
            self.admin.assert_icommand(['itrim', '-Nnope', logical_path], 'STDERR', 'SYS_INVALID_INPUT_PARAM')
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, original_resource))
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, resc1))
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, resc2))

            # -N must be an Integer.
            self.admin.assert_icommand(['itrim', '-N"3.14"', logical_path], 'STDERR', 'SYS_INVALID_INPUT_PARAM')
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, original_resource))
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, resc1))
            self.assertTrue(lib.replica_exists_on_resource(self.admin, logical_path, resc2))

        finally:
            self.admin.assert_icommand(['ils', '-l', logical_path], 'STDOUT', filename) # debugging
            self.admin.run_icommand(['irm', '-f', logical_path])
            lib.remove_resource(self.admin, original_resource)
            lib.remove_resource(self.admin, resc1)
            lib.remove_resource(self.admin, resc2)

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Checks local resource vault.")
    def test_itrim_all_permission_combinations__issue_8450(self):
        trim_resource = "trim_resc"
        keep_resource = "keep_resc"
        trim_vault_path = os.path.join(paths.home_directory(), f"{trim_resource}_vault")
        keep_vault_path = os.path.join(paths.home_directory(), f"{keep_resource}_vault")

        user = session.mkuser_and_return_session('rodsuser', "alice", "apass", lib.get_hostname())
        user_collection = "/".join([user.session_collection, self.id() + "-coll"])
        admin_collection = "/".join([self.admin.session_collection, self.id() + "-coll"])
        user_data_object = "/".join([user_collection, self.id() + ".data"])
        admin_data_object = "/".join([admin_collection, self.id() + ".data"])
        user_trim_physical_path = os.path.join(
            trim_vault_path, os.path.sep.join(user_data_object.split("/")[2:]))
        admin_trim_physical_path = os.path.join(
            trim_vault_path, os.path.sep.join(admin_data_object.split("/")[2:]))

        try:
            self.admin.assert_icommand(
                ["iadmin", "mkresc", trim_resource, "unixfilesystem",
                 ":".join([test.settings.HOSTNAME_1, trim_vault_path])], "STDOUT")
            self.admin.assert_icommand(
                ["iadmin", "mkresc", keep_resource, "unixfilesystem",
                 ":".join([test.settings.HOSTNAME_1, keep_vault_path])], "STDOUT")

            user.assert_icommand(["imkdir", user_collection])
            user.assert_icommand(["itouch", "-R", trim_resource, user_data_object])
            self.assertTrue(os.path.exists(user_trim_physical_path),
                            msg=f"Data not created at [{user_trim_physical_path}] as expected.")
            self.admin.assert_icommand(["imkdir", admin_collection])
            self.admin.assert_icommand(["itouch", "-R", trim_resource, admin_data_object])
            self.assertTrue(os.path.exists(admin_trim_physical_path),
                            msg=f"Data not created at [{admin_trim_physical_path}] as expected.")
            user.assert_icommand(["irepl", "-R", keep_resource, user_data_object])
            self.admin.assert_icommand(["irepl", "-R", keep_resource, admin_data_object])

            all_permissions = lib.irods_permissions

            import enum
            class Outcome(enum.Enum):
                DEL = 'DEL'  # replica physically removed
                SIL = 'SIL'  # replica not removed silently (rc=0, no error)
                ERR = 'ERR'  # replica not removed, CAT_NO_ACCESS_PERMISSION error
                DNE = 'DNE'  # replica not removed and the reason given is that the object does not exist

            DEL = Outcome.DEL
            SIL = Outcome.SIL
            ERR = Outcome.ERR
            DNE = Outcome.DNE

            # Expected outcomes for a regular user (alice) trying to itrim -N1 -S an admin-owned object.
            # Rows = collection permission, Columns = object permission (in all_permissions order).
            # fmt: off
            expected_as_rodsuser = {
                #                  null  read_metadata  read_object  create_metadata  modify_metadata  delete_metadata  create_object  modify_object  delete_object  own
                'null':           [DNE,  DNE,           DNE,         DNE,             DNE,             DNE,             DNE,           DNE,           DNE,           DNE],
                'read_metadata':  [DNE,  DNE,           DNE,         DNE,             DNE,             DNE,             DNE,           DNE,           DNE,           DNE],
                'read_object':    [DNE,  DNE,           ERR,         ERR,             ERR,             ERR,             ERR,           ERR,           DEL,           DEL],
                'create_metadata':[DNE,  DNE,           ERR,         ERR,             ERR,             ERR,             ERR,           ERR,           DEL,           DEL],
                'modify_metadata':[DNE,  DNE,           ERR,         ERR,             ERR,             ERR,             ERR,           ERR,           DEL,           DEL],
                'delete_metadata':[DNE,  DNE,           ERR,         ERR,             ERR,             ERR,             ERR,           ERR,           DEL,           DEL],
                'create_object':  [DNE,  DNE,           ERR,         ERR,             ERR,             ERR,             ERR,           ERR,           DEL,           DEL],
                'modify_object':  [DNE,  DNE,           ERR,         ERR,             ERR,             ERR,             ERR,           ERR,           DEL,           DEL],
                'delete_object':  [DNE,  DNE,           ERR,         ERR,             ERR,             ERR,             ERR,           ERR,           DEL,           DEL],
                'own':            [DNE,  DNE,           ERR,         ERR,             ERR,             ERR,             ERR,           ERR,           DEL,           DEL],
            }

            # Expected outcomes for an admin (otherrods) trying to itrim -N1 -S a user-owned object.
            expected_as_rodsadmin = {
                #                  null  read_metadata  read_object  create_metadata  modify_metadata  delete_metadata  create_object  modify_object  delete_object  own
                'null':           [ERR,  ERR,           ERR,         ERR,             ERR,             ERR,             ERR,           ERR,           DEL,           DEL],
                'read_metadata':  [ERR,  ERR,           ERR,         ERR,             ERR,             ERR,             ERR,           ERR,           DEL,           DEL],
                'read_object':    [ERR,  ERR,           ERR,         ERR,             ERR,             ERR,             ERR,           ERR,           DEL,           DEL],
                'create_metadata':[ERR,  ERR,           ERR,         ERR,             ERR,             ERR,             ERR,           ERR,           DEL,           DEL],
                'modify_metadata':[ERR,  ERR,           ERR,         ERR,             ERR,             ERR,             ERR,           ERR,           DEL,           DEL],
                'delete_metadata':[ERR,  ERR,           ERR,         ERR,             ERR,             ERR,             ERR,           ERR,           DEL,           DEL],
                'create_object':  [ERR,  ERR,           ERR,         ERR,             ERR,             ERR,             ERR,           ERR,           DEL,           DEL],
                'modify_object':  [ERR,  ERR,           ERR,         ERR,             ERR,             ERR,             ERR,           ERR,           DEL,           DEL],
                'delete_object':  [ERR,  ERR,           ERR,         ERR,             ERR,             ERR,             ERR,           ERR,           DEL,           DEL],
                'own':            [ERR,  ERR,           ERR,         ERR,             ERR,             ERR,             ERR,           ERR,           DEL,           DEL],
            }
            # fmt: on

            # Each entry is (executor, owner, object_path, trim_physical_path, expected_matrix).
            test_cases = [
                (user,        self.admin, admin_data_object, admin_trim_physical_path, expected_as_rodsuser),
                (self.admin,  user,       user_data_object,  user_trim_physical_path,  expected_as_rodsadmin),
            ]

            for executor, owner, object_path, physical_path, expected in test_cases:
                collection_path = os.path.dirname(object_path)

                for coll_perm in all_permissions:
                    for i, obj_perm in enumerate(all_permissions):
                        with self.subTest(executor=executor.username, coll_perm=coll_perm, obj_perm=obj_perm):
                            owner.assert_icommand(["ichmod", coll_perm, executor.username, collection_path])
                            owner.assert_icommand(["ichmod", obj_perm,  executor.username, object_path])

                            _, err, rc = executor.run_icommand(["itrim", "-N1", "-S", trim_resource, object_path])

                            replica_gone = not os.path.exists(physical_path)

                            if replica_gone:
                                outcome = DEL
                                # Recreate replica on trim_resource for subsequent iterations.
                                owner.assert_icommand(["irepl", "-R", trim_resource, object_path])
                            elif rc != 0 and "does not exist" in err:
                                outcome = DNE
                            elif rc != 0:
                                outcome = ERR
                            else:
                                outcome = SIL

                            self.assertEqual(expected[coll_perm][i], outcome,
                                msg=f"executor={executor.username} coll_perm={coll_perm} obj_perm={obj_perm}: "
                                    f"expected {expected[coll_perm][i]}, got {outcome}")
                            if outcome == ERR:
                                self.assertIn("CAT_NO_ACCESS_PERMISSION", err,
                                    msg=f"executor={executor.username} coll_perm={coll_perm} obj_perm={obj_perm}: "
                                        f"expected CAT_NO_ACCESS_PERMISSION in stderr: [{err}]")

                            # Restore the owner's own permission so subsequent iterations work correctly.
                            owner.run_icommand(["ichmod", "-M", "own", owner.username, object_path])

        finally:
            user.run_icommand(["irm", "-rf", user_collection])
            self.admin.run_icommand(["irm", "-rf", admin_collection])
            user.__exit__()
            with session.make_session_for_existing_admin() as admin_session:
                admin_session.run_icommand(['iadmin', 'rmuser', user.username])
                admin_session.run_icommand(["iadmin", "rmresc", trim_resource])
                admin_session.run_icommand(["iadmin", "rmresc", keep_resource])


class test_itrim_target_replica_selection_decision_making__issue_7515(unittest.TestCase):

	hostnames = [test.settings.ICAT_HOSTNAME, test.settings.HOSTNAME_1, test.settings.HOSTNAME_2, test.settings.HOSTNAME_3]
	resource_names = [f'ufs{i}' for i in range(len(hostnames))]

	@classmethod
	def setUpClass(cls):
		# Create a test admin user and a test regular user.
		cls.admin = session.mkuser_and_return_session('rodsadmin', 'otherrods', 'rods', lib.get_hostname())
		cls.user = session.mkuser_and_return_session('rodsuser', 'smeagol', 'spass', lib.get_hostname())

		# Create some test resources.
		for i, resource_name in enumerate(cls.resource_names):
			lib.create_ufs_resource(cls.admin, resource_name, hostname=cls.hostnames[i])

	@classmethod
	def tearDownClass(cls):
		with session.make_session_for_existing_admin() as admin_session:
			# Remove all the test resources.
			for resource_name in cls.resource_names:
				lib.remove_resource(admin_session, resource_name)

			# Remove the regular test user.
			cls.user.__exit__()
			admin_session.assert_icommand(['iadmin', 'rmuser', cls.user.username])

			# Remove the test admin.
			cls.admin.__exit__()
			admin_session.assert_icommand(['iadmin', 'rmuser', cls.admin.username])

	def setUp(cls):
		# Create a data object and replicate to each resource.
		contents = 'trim-ly mcskimbly'
		cls.data_name = 'test_itrim_target_replica_selection_decision_making.txt'
		cls.logical_path = '/'.join([cls.user.session_collection, cls.data_name])
		for i, resource_name in enumerate(cls.resource_names):
			if i == 0:
				cls.user.assert_icommand(['istream', 'write', '-R', resource_name, cls.logical_path], input=contents)
				continue

			cls.user.assert_icommand(['irepl', '-R', resource_name, cls.logical_path])
			cls.assertTrue(lib.replica_exists_on_resource(cls.user, cls.logical_path, resource_name))

	def tearDown(cls):
		# Remove the data object.
		cls.user.assert_icommand(['irm', '-f', cls.logical_path])

	def set_replica_statuses(self, replica_status_list):
		# Ensure that the number of replicas will line up with the number of replica statuses.
		self.assertEqual(len(self.resource_names), len(replica_status_list))
		for replica_number, replica_status in enumerate(replica_status_list):
			# The replica statuses should only be numbers (either literally or as a string).
			self.assertTrue(isinstance(replica_status, int) or isinstance(replica_status, str))
			lib.set_replica_status(self.admin, self.logical_path, replica_number, replica_status)

	def check_replica_statuses(self, replica_status_list):
		# Ensure that the number of replicas will line up with the number of replica statuses.
		self.assertEqual(len(self.resource_names), len(replica_status_list))
		for replica_number, replica_status in enumerate(replica_status_list):
			if replica_status is None:
				# We use None to represent a replica which does not exist.
				self.assertFalse(lib.replica_exists(self.user, self.logical_path, replica_number))
			else:
				self.assertEqual(
					str(replica_status), lib.get_replica_status(self.user, self.data_name, replica_number))

	def do_test(self, replica_status_list_at_start, replica_status_list_at_end):
		try:
			self.set_replica_statuses(replica_status_list_at_start)

			self.user.assert_icommand(['ils', '-l', self.logical_path], 'STDOUT', self.data_name) # debugging

			self.user.assert_icommand(['itrim', self.logical_path], 'STDOUT', 'Number of data objects trimmed = 1.')

			self.check_replica_statuses(replica_status_list_at_end)

		finally:
			# For debugging...
			self.user.assert_icommand(['ils', '-l', self.logical_path], 'STDOUT', self.data_name)

	def test_case00_XXXX(self):
		"""
		case	0
		start	XXXX
		end 	--XX
		"""
		self.do_test([0, 0, 0, 0], [None, None, 0, 0])

	def test_case01_XXXG(self):
		"""
		case	1
		start	XXX&
		end		--X&
		"""
		self.do_test([0, 0, 0, 1], [None, None, 0, 1])

	def test_case02_XXGX(self):
		"""
		case	2
		start	XX&X
		end 	--&X
		"""
		self.do_test([0, 0, 1, 0], [None, None, 1, 0])

	def test_case03_XXGG(self):
		"""
		case	3
		start	XX&&
		end 	--&&
		"""
		self.do_test([0, 0, 1, 1], [None, None, 1, 1])

	def test_case04_XGXX(self):
		"""
		case	4
		start	X&XX
		end 	-&-X
		"""
		self.do_test([0, 1, 0, 0], [None, 1, None, 0])

	def test_case05_XGXG(self):
		"""
		case	5
		start	X&X&
		end 	-&-&
		"""
		self.do_test([0, 1, 0, 1], [None, 1, None, 1])

	def test_case06_XGGX(self):
		"""
		case	6
		start	X&&X
		end 	-&&-
		"""
		self.do_test([0, 1, 1, 0], [None, 1, 1, None])

	def test_case07_XGGG(self):
		"""
		case	7
		start	X&&&
		end 	--&&
		"""
		self.do_test([0, 1, 1, 1], [None, None, 1, 1])

	def test_case08_GXXX(self):
		"""
		case	8
		start	&XXX
		end 	&--X
		"""
		self.do_test([1, 0, 0, 0], [1, None, None, 0])

	def test_case09_GXXG(self):
		"""
		case	9
		start	&XX&
		end 	&--&
		"""
		self.do_test([1, 0, 0, 1], [1, None, None, 1])

	def test_case10_GXGX(self):
		"""
		case	10
		start	&X&X
		end 	&-&-
		"""
		self.do_test([1, 0, 1, 0], [1, None, 1, None])

	def test_case11_GXGG(self):
		"""
		case	11
		start	&X&&
		end 	--&&
		"""
		self.do_test([1, 0, 1, 1], [None, None, 1, 1])

	def test_case12_GGXX(self):
		"""
		case	12
		start	&&XX
		end		&&--
		"""
		self.do_test([1, 1, 0, 0], [1, 1, None, None])

	def test_case13_GGXG(self):
		"""
		case	13
		start	&&X&
		end		-&-&
		"""
		self.do_test([1, 1, 0, 1], [None, 1, None, 1])

	def test_case14_GGGX(self):
		"""
		case	14
		start	&&&X
		end		-&&-
		"""
		self.do_test([1, 1, 1, 0], [None, 1, 1, None])

	def test_case15_GGGG(self):
		"""
		case	15
		start	&&&&
		end		--&&
		"""
		self.do_test([1, 1, 1, 1], [None, None, 1, 1])

