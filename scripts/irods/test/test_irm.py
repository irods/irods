import os
import sys
import unittest

from . import session
from .. import lib
from .. import paths
from .. import test

class Test_Irm(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):

    def setUp(self):
        super(Test_Irm, self).setUp()
        self.admin = self.admin_sessions[0]
        self.user = self.user_sessions[0]

    def tearDown(self):
        super(Test_Irm, self).tearDown()

    def test_irm_option_U_is_invalid__issue_4681(self):
        filename = 'test_irm_option_U_is_deprecated.txt'
        logical_path = os.path.join(self.admin.session_collection, filename)
        contents = '4681'
        self.admin.assert_icommand(['istream', 'write', logical_path], input=contents)
        self.admin.assert_icommand(['irm', '-U', logical_path], 'STDERR', 'USER_INPUT_OPTION_ERR')
        # Make sure the data object was not removed
        lib.replica_exists(self.admin, logical_path, 0)
        self.admin.assert_icommand(['istream', 'read', logical_path], 'STDOUT', [contents])

    def test_irm_option_n_is_invalid__issue_3451_6340(self):
        filename = 'test_file_issue_3451.txt'
        logical_path = os.path.join(self.admin.session_collection, filename)
        self.admin.assert_icommand(['itouch', logical_path])
        self.admin.assert_icommand(['irm', '-n0', logical_path], 'STDERR', 'USER_INPUT_OPTION_ERR')
        # Make sure the data object was not removed
        lib.replica_exists(self.admin, logical_path, 0)

    def test_irm_delete_collection_with_ampersand_in_name_causes_error__issue_3398(self):
        collection = 'testDeleteACollectionWithAmpInTheNameBug170 && hail hail rock & roll  &'
        self.admin.assert_icommand("imkdir '{0}'".format(collection))
        self.admin.assert_icommand("ils -l '{0}'".format(self.admin.session_collection), 'STDOUT', collection)
        self.admin.assert_icommand("irm -r '{0}'".format(collection))
        self.admin.assert_icommand("ils -l '{0}'".format(self.admin.session_collection_trash), 'STDOUT', collection)

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_irm_with_force_does_not_delete_registered_data_objects__issue_4848(self):
        # Register a file into iRODS.
        filename = 'foo'
        filepath = os.path.join(self.admin.local_session_dir, filename)
        lib.make_file(filepath, 1024, 'arbitrary')
        data_object = os.path.join(self.admin.session_collection, filename)
        self.admin.assert_icommand(['ireg', filepath, data_object])

        # Remove the data object from iRODS and show that the data object
        # was unregistered instead of being deleted because it was not
        # in a vault.
        self.admin.assert_icommand(['irm', '-f', data_object])
        self.assertTrue(os.path.exists(filepath))

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_irm_with_force_silently_ignores_non_existent_data_objects__issue_5438(self):
        data_object = 'non_existent_data_object'

        # Show that without the force flag, removing a non-existent data object
        # results in an error.
        self.user.assert_icommand(['irm', data_object], 'STDERR', ['does not exist'])

        # Show that using the force flag does not result in an error when given a
        # non-existent data object.
        self.user.assert_icommand(['irm', '-f', data_object])


    def test_remove_data_object_in_collection_with_read_permissions__issue_6428(self):
        import time

        def get_collection_mtime(session, collection_path):
            return session.run_icommand(['iquest', '%s',
                f"select COLL_MODIFY_TIME where COLL_NAME = '{collection_path}'"])[0].strip()

        filename = 'issue_6428_object'
        collection_name = 'issue_6428_collection'
        collection_path = os.path.join('/' + self.admin.zone_name, 'home', 'public', collection_name)
        logical_path = os.path.join(collection_path, filename)

        try:
            self.admin.assert_icommand(['imkdir', collection_path])
            self.admin.assert_icommand(['itouch', logical_path])
            self.admin.assert_icommand(['ichmod', 'read', self.user.username, collection_path])
            self.admin.assert_icommand(['ichmod', 'own', self.user.username, logical_path])
            self.assertTrue(lib.replica_exists(self.user, logical_path, 0))

            original_mtime = get_collection_mtime(self.admin, collection_path)

            # Sleep here so that the collection mtime is guaranteed to be different if updated correctly.
            time.sleep(1)

            self.user.assert_icommand(['irm', logical_path])
            self.assertFalse(lib.replica_exists(self.user, logical_path, 0))

            new_mtime = get_collection_mtime(self.admin, collection_path)

            self.assertNotEqual(original_mtime, new_mtime, msg='collection mtime was not updated')

        finally:
            self.user.assert_icommand(['irmtrash'])
            self.admin.assert_icommand(['irm', '-r', '-f', collection_path])
            self.admin.assert_icommand(['irmtrash', '-M'])


@unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Checks local resource vault.")
class test_no_accidental_data_loss_on_unlink__issue_8441(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.admin = session.mkuser_and_return_session("rodsadmin", "otherrods", "rods", lib.get_hostname())
        self.user = session.mkuser_and_return_session('rodsuser', "alice", "apass", lib.get_hostname())

        self.resource = "leaky_resc"
        self.vault_path = os.path.join(paths.home_directory(), f"{self.resource}_vault")
        self.admin.assert_icommand(
            ["iadmin", "mkresc", self.resource, "unixfilesystem",
             ":".join([test.settings.HOSTNAME_1, self.vault_path])], "STDOUT")

    @classmethod
    def tearDownClass(self):
        self.user.__exit__()
        self.admin.__exit__()
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.run_icommand(['iadmin', 'rmuser', self.user.username])
            admin_session.run_icommand(['iadmin', 'rmuser', self.admin.username])
            admin_session.run_icommand(["iadmin", "rmresc", self.resource])

    def setUp(self):
        # Define all the paths that will be used by the tests.
        self.user_collection = "/".join([self.user.session_collection, self.id() + "-coll"])
        self.admin_collection = "/".join([self.admin.session_collection, self.id() + "-coll"])
        self.user_data_object = "/".join([self.user_collection, self.id() + ".data"])
        self.admin_data_object = "/".join([self.admin_collection, self.id() + ".data"])
        # The physical path is constructed by taking the logical path, removing the zone hint and leading slash, and
        # concatenating it with the resource vault path. So, we skip the first two elements in the split.
        self.user_physical_path = os.path.join(
            self.vault_path, os.path.sep.join(self.user_data_object.split("/")[2:]))
        self.admin_physical_path = os.path.join(
            self.vault_path, os.path.sep.join(self.admin_data_object.split("/")[2:]))
        # Create test collections and data objects owned by each user for use in the tests. Importantly, ensure that the
        # physical data is in the expected location.
        self.user.assert_icommand(["imkdir", self.user_collection])
        self.user.assert_icommand(["itouch", "-R", self.resource, self.user_data_object])
        self.assertTrue(os.path.exists(self.user_physical_path),
                        msg=f"Data not created at [{self.user_physical_path}] as expected.")
        self.admin.assert_icommand(["imkdir", self.admin_collection])
        self.admin.assert_icommand(["itouch", "-R", self.resource, self.admin_data_object])
        self.assertTrue(os.path.exists(self.admin_physical_path),
                        msg=f"Data not created at [{self.admin_physical_path}] as expected.")

    def tearDown(self):
        self.user.run_icommand(["irm", "-rf", self.user_collection])
        self.admin.run_icommand(["irm", "-rf", self.admin_collection])

    def assert_unlink_fails(self, owner, executor, collection_permission, object_permission, object_path):
        # Give specified permissions to the "executor" user by the "owner" user.
        collection_path = os.path.dirname(object_path)
        owner.assert_icommand(["ichmod", collection_permission, executor.username, collection_path])
        owner.assert_icommand(["ichmod", object_permission, executor.username, object_path])

        # Execute unlink on the data object as the "executor" user with irm -f.
        out, err, rc = executor.run_icommand(["irm", "-f", object_path])

        # Assert that the irm failed.
        self.assertIn("CAT_NO_ACCESS_PERMISSION", err,
                      msg=f"Expected CAT_NO_ACCESS_PERMISSION in error output. stderr: [{err}]")
        self.assertNotEqual(0, rc, msg="Unlink did not return an error code as expected.")
        self.assertEqual(0, len(out), msg=f"Unexpected output on stdout: [{out}]")
        # Assert that the unregister failed.
        self.assertTrue(lib.replica_exists_on_resource(owner, object_path, self.resource),
                        msg=f"Replica for [{object_path}] was unregistered unexpectedly.")

    def test_as_user_with_modify_on_collection_and_modify_on_object(self):
        owner = self.admin
        executor = self.user
        target_object = self.admin_data_object
        collection_permission = "modify_object"
        object_permission = "modify_object"
        self.assert_unlink_fails(owner, executor, collection_permission, object_permission, target_object)
        # Assert that the physical data was not unlinked.
        self.assertTrue(os.path.exists(self.admin_physical_path),
                        msg=f"Data at [{self.admin_physical_path}] unlinked unexpectedly.")

    def test_as_admin_with_modify_on_collection_and_modify_on_object(self):
        owner = self.user
        executor = self.admin
        target_object = self.user_data_object
        collection_permission = "modify_object"
        object_permission = "modify_object"
        self.assert_unlink_fails(owner, executor, collection_permission, object_permission, target_object)
        # Assert that the physical data was not unlinked.
        self.assertTrue(os.path.exists(self.user_physical_path),
                        msg=f"Data at [{self.user_physical_path}] unlinked unexpectedly.")
