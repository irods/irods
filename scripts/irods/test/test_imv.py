import os
import sys
import unittest

from . import session
from .. import lib
from .. import paths
from .. import test
from ..configuration import IrodsConfig
from ..controller import IrodsController

class Test_Imv(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):

    def setUp(self):
        super(Test_Imv, self).setUp()
        self.admin = self.admin_sessions[0]
        self.user = self.user_sessions[0]

    def tearDown(self):
        super(Test_Imv, self).tearDown()

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_imv_boost_any_cast_exception_on_rename__issue_4301(self):
        config = IrodsConfig()

        with lib.file_backed_up(config.server_config_path):
            core_re_path = os.path.join(config.core_re_directory, 'core.re')

            try:
                with lib.file_backed_up(core_re_path):
                    with open(core_re_path, 'a') as core_re:
                        core_re.write('pep_api_data_obj_rename_post(*a, *b, *c) {}\n')
                    IrodsController(config).reload_configuration()

                    src = 'test_file_issue_4301_a.txt'
                    lib.make_file(src, 1024, 'arbitrary')
                    self.admin.assert_icommand(['iput', src])

                    dst = 'test_file_issue_4301_b.txt'
                    self.admin.assert_icommand(['imv', src, dst])
                    self.admin.assert_icommand('ils', 'STDOUT', dst)

            finally:
                IrodsController(config).reload_configuration()

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_imv_prints_error_on_non_existent_collection__issue_4414(self):
        # Put a file into iRODS.
        filename = os.path.join(self.admin.local_session_dir, 'foo')
        lib.make_file(filename, 1024, 'arbitrary')
        self.admin.assert_icommand(['iput', filename])

        # Try to move the data object under a non-existent collection.
        data_object = os.path.basename(filename)
        new_path = os.path.join(self.admin.session_collection, 'non_existent_collection.d', data_object)
        _, stderr, ec = self.admin.run_icommand(['imv', data_object, new_path])
        self.assertTrue('-814000 CAT_UNKNOWN_COLLECTION' in stderr)
        self.assertTrue(ec != 0)

    def test_imv_with_apostrophe_logical_path__issue_5759(self):
        """Test imv with apostrophes in the logical path.

        For each imv, the logical path of the source and destination data objects, will contain
        an apostrophe in either the collection name, data object name, both, or neither.
        """

        local_file = os.path.join(self.user.local_session_dir, 'test_imv_with_apostrophe_logical_path__issue_5759')
        lib.make_file(local_file, 1024, 'arbitrary')

        collection_names = ["collection", "collect'ion"]

        data_names = ["data_object", "data'_object"]

        for source_coll in collection_names:
            source_collection_path = os.path.join(self.user.session_collection, 'source', source_coll)
            self.user.assert_icommand(['imkdir', '-p', source_collection_path])

            for destination_coll in collection_names:
                destination_collection_path = os.path.join(self.user.session_collection, 'destination', destination_coll)
                self.user.assert_icommand(['imkdir', '-p', destination_collection_path])

                for source_name in data_names:
                    source_logical_path = os.path.join(source_collection_path, source_name)
                    self.user.assert_icommand(['iput', local_file, source_logical_path])
                    self.user.assert_icommand(['ils', '-l', source_logical_path], 'STDOUT', source_name)

                    for destination_name in data_names:
                        destination_logical_path = os.path.join(destination_collection_path, destination_name)
                        print('source=[{0}], destination=[{1}]'.format(source_logical_path, destination_logical_path))
                        self.user.assert_icommand(['imv', source_logical_path, destination_logical_path])
                        self.user.assert_icommand(['ils', '-l', destination_logical_path], 'STDOUT', destination_name)
                        self.user.assert_icommand_fail(['ils', '-l', source_logical_path], 'STDOUT', source_name)

                        self.user.assert_icommand(['imv', destination_logical_path, source_logical_path])
                        self.user.assert_icommand(['ils', '-l', source_logical_path], 'STDOUT', source_name)
                        self.user.assert_icommand_fail(['ils', '-l', destination_logical_path], 'STDOUT', source_name)

                    self.user.assert_icommand(['irm', '-rf', destination_collection_path])
                    self.user.assert_icommand(['imkdir', '-p', destination_collection_path])

                    self.user.assert_icommand(['irm', '-rf', source_collection_path])
                    self.user.assert_icommand(['imkdir', '-p', source_collection_path])


class test_renaming_collections_with_special_characters__issue_6239(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.user = session.mkuser_and_return_session("rodsuser", "smeagol", "spass", lib.get_hostname())
        cls.admin = session.mkuser_and_return_session("rodsadmin", "otherrods", "rods", lib.get_hostname())

    @classmethod
    def tearDownClass(cls):
        with session.make_session_for_existing_admin() as admin_session:
            # Remove the regular test user.
            cls.user.__exit__()
            admin_session.assert_icommand(["iadmin", "rmuser", cls.user.username])

            # Remove the test admin.
            cls.admin.__exit__()
            admin_session.assert_icommand(["iadmin", "rmuser", cls.admin.username])

    def test_spaces_in_collection_renamed_by_admin_does_not_orphan_contents(self):
        local_resource = "issue_6239_resource"
        self.admin.assert_icommand(
            ["iadmin", "mkresc", local_resource, "unixfilesystem",
             lib.get_hostname() + ":" + paths.irods_directory() + f"/{local_resource}"], "STDOUT", local_resource)

        subcollection_basename = "XXXX_vX.X.X_analysis1-2021-10-27-12-19-57.5/index"
        source_subcollection_suffix = f" Ss  /{subcollection_basename}"
        data_object_name = "data_object.txt"
        contents = "test_spaces_in_parent_collection_does_not_break_renaming_collection_by_admin"

        # Collection names and expected renamed collection names...
        source_parent_collection = self.user.session_collection
        source_subcollection = f"{source_parent_collection}/{source_subcollection_suffix}"
        source_collection = os.path.dirname(source_subcollection)
        source_data_object = f"{source_subcollection}/{data_object_name}"
        destination_parent_collection = self.admin.session_collection
        # The collection to be renamed is a subcollection of the base subcollection, so the destination collection path
        # will omit the base subcollection. So, we will use the subcollection "basename".
        destination_collection = f"{destination_parent_collection}/{subcollection_basename}"
        destination_data_object = f"{destination_collection}/{data_object_name}"

        # Directories in vault...
        source_parent_directory = self.user.get_vault_session_path(local_resource)
        source_directory = f"{source_parent_directory}/{source_subcollection_suffix}"
        source_file = f"{source_directory}/{data_object_name}"
        destination_parent_directory = self.admin.get_vault_session_path(local_resource)
        destination_directory = f"{destination_parent_directory}/{subcollection_basename}"
        destination_file = f"{destination_directory}/{data_object_name}"

        try:
            # Ensure that the destination data object and its parent collection do not exist.
            self.assertFalse(lib.collection_exists(self.admin, destination_collection),
                             msg=f"Destination collection [{destination_collection}] exists.")
            self.assertFalse(lib.replica_exists(self.admin, destination_data_object, 0),
                             msg=f"Destination data object [{destination_data_object}] exists.")

            # Create collection structure, put some data in it, and give permissions to the admin user to rename it.
            self.user.assert_icommand(["imkdir", "-p", source_subcollection])
            self.assertTrue(lib.collection_exists(self.user, source_subcollection),
                            msg=f"Source collection [{source_subcollection}] does not exist.")
            self.user.assert_icommand(["istream", "write", "-R", local_resource, source_data_object], input=contents)
            self.user.assert_icommand(["ichmod", "-r", "own", self.admin.username, self.user.session_collection])

            self.user.assert_icommand(["ils", "-lr"], "STDOUT")
            self.admin.assert_icommand(["ils", "-lr"], "STDOUT")

            # Ensure that the source data object exists.
            self.assertTrue(lib.replica_exists(self.user, source_data_object, 0),
                            msg=f"Source data object [{source_data_object}] does not exist.")

            # Ensure that the data is written to the expected location in the vault.
            self.assertTrue(os.path.exists(source_file), msg=f"Source file [{source_file}] does not exist.")
            self.assertFalse(os.path.exists(destination_file), msg=f"Destination file [{destination_file}] exists.")

            # Move a subcollection to a completely different parent collection as the admin user. The source collection
            # targets the dirname because we want to target that particular subcollection, not the "leaf" subcollection.
            self.admin.assert_icommand(["imv", source_collection, destination_parent_collection])

            # The subcollection targeted for renaming should no longer exist, but its parent collection should exist
            # as well as the collection to which it was renamed.
            self.assertFalse(lib.collection_exists(self.user, source_subcollection),
                             msg=f"Source collection [{source_subcollection}] exists.")
            self.assertTrue(lib.collection_exists(self.user, os.path.dirname(source_collection)),
                            msg=f"Source collection [{source_collection}] does not exist.")
            self.assertTrue(lib.collection_exists(self.admin, destination_collection),
                            msg=f"Destination collection [{destination_collection}] does not exist.")

            # Ensure that the data object was renamed.
            self.assertTrue(lib.replica_exists(self.admin, destination_data_object, 0),
                            msg=f"Destination data object [{destination_data_object}] does not exist.")
            self.assertFalse(lib.replica_exists(self.user, source_data_object, 0),
                             msg=f"Source data object [{source_data_object}] exists.")

            # Ensure that the data is renamed to the expected location in the vault and the source file no longer exists.
            self.assertTrue(os.path.exists(destination_file), msg=f"Destination file [{destination_file}] does not exist.")
            self.assertFalse(os.path.exists(source_file), msg=f"Source file [{source_file}] exists.")

        finally:
            self.user.assert_icommand(["ils", "-lr"], "STDOUT")
            self.user.run_icommand(["irm", "-rf", source_parent_collection])
            self.admin.assert_icommand(["ils", "-lr"], "STDOUT")
            self.admin.run_icommand(["irm", "-rf", destination_parent_collection])
            self.admin.run_icommand(["iadmin", "rmresc", local_resource])

    def test_multibyte_characters_in_parent_collection_does_not_break_renaming_collection(self):
        subcollection_basename = "gamma"
        # Note the multibyte character: é
        source_relative_collection_path = "béta alpha"
        source_subcollection_suffix = f"{source_relative_collection_path}/{subcollection_basename}"
        destination_relative_collection_path = "béta zeta"
        destination_subcollection_suffix = f"{destination_relative_collection_path}/{subcollection_basename}"

        # Collection names and expected renamed collection names...
        source_parent_collection = self.user.session_collection
        source_subcollection = f"{source_parent_collection}/{source_subcollection_suffix}"
        # The collection to be renamed is a subcollection of the base subcollection, so the destination collection path
        # will omit the base subcollection. So, we will use the subcollection "basename".
        destination_parent_collection = source_parent_collection
        destination_collection = f"{destination_parent_collection}/{destination_subcollection_suffix}"

        try:
            # Ensure that the destination collection does not exist.
            self.assertFalse(lib.collection_exists(self.user, destination_collection),
                             msg=f"Destination collection [{destination_collection}] exists.")

            # Create collection structure, put some data in it, and give permissions to the admin user to rename it.
            self.user.assert_icommand(["icd", source_parent_collection])
            self.user.assert_icommand(["imkdir", "-p", source_subcollection])
            self.assertTrue(lib.collection_exists(self.user, source_subcollection),
                            msg=f"Destination collection [{source_subcollection}] does not exist.")

            self.user.assert_icommand(["ipwd"], "STDOUT")
            self.user.assert_icommand(["ils", "-lr"], "STDOUT")

            # Rename the collection in place and ensure that the subcollections are created.
            self.user.assert_icommand(["imv", source_relative_collection_path, destination_relative_collection_path])

            self.assertFalse(lib.collection_exists(self.user, source_subcollection),
                             msg=f"Source collection [{source_subcollection}] exists.")
            self.assertTrue(lib.collection_exists(self.user, destination_collection),
                            msg=f"Destination collection [{destination_collection}] does not exist.")

        finally:
            self.user.assert_icommand(["ils", "-lr"], "STDOUT")
            self.user.assert_icommand(["irm", "-rf", source_parent_collection])
            self.user.assert_icommand(["irm", "-rf", destination_parent_collection])


class test_moving_and_renaming_collections_with_multibyte_characters__issue_6239(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.user = session.mkuser_and_return_session("rodsuser", "smeagol", "spass", lib.get_hostname())
        cls.admin = session.mkuser_and_return_session("rodsadmin", "otherrods", "rods", lib.get_hostname())

    @classmethod
    def tearDownClass(cls):
        with session.make_session_for_existing_admin() as admin_session:
            # Remove the regular test user.
            cls.user.__exit__()
            admin_session.assert_icommand(["iadmin", "rmuser", cls.user.username])

            # Remove the test admin.
            cls.admin.__exit__()
            admin_session.assert_icommand(["iadmin", "rmuser", cls.admin.username])

    def setUp(self):
        self.base_parent_collection = self.user.session_collection
        self.user.assert_icommand(["icd", self.base_parent_collection])

        self.base_subcollection = "above"
        self.base_multibyte_collection = "béta12"
        self.subcollection_suffix = "/".join(["mycol", "subcol"])
        self.relative_multibyte_collection = "/".join(
            [self.base_subcollection, self.base_multibyte_collection, self.subcollection_suffix])

        self.user.assert_icommand(["imkdir", "-p", self.relative_multibyte_collection])

        self.sibling_subcollection = "sibling"
        self.user.assert_icommand(["imkdir", "-p", "/".join([self.base_subcollection, self.sibling_subcollection])])

    def do_imv_test(self, imv_destination_input):
        # Full logical path to deepest subcollection under the imv source.
        source_coll = "/".join(
            [self.base_parent_collection, self.relative_multibyte_collection])
        # Full logical path to deepest subcollection under the expected imv destination.
        destination_coll = "/".join(
            [self.base_parent_collection, imv_destination_input, self.subcollection_suffix])
        # above/béta12
        imv_source_input = "/".join([self.base_subcollection, self.base_multibyte_collection])

        self.assertTrue(lib.collection_exists(self.user, source_coll))
        destination_coll = "/".join(
            [self.base_parent_collection, imv_destination_input, self.subcollection_suffix])

        self.user.assert_icommand(["imv", imv_source_input, imv_destination_input])
        self.assertFalse(lib.collection_exists(self.user, source_coll))
        self.assertTrue(lib.collection_exists(self.user, destination_coll))

    def test_move_only_up(self):
        # imv above/béta12 béta12
        self.do_imv_test("béta12")

    def test_move_only_down(self):
        # imv above/béta12 above/sibling/béta12
        self.do_imv_test("/".join([self.base_subcollection, self.sibling_subcollection, "béta12"]))

    def test_rename_only_same_length(self):
        # imv above/béta12 above/béta34
        self.do_imv_test("/".join([self.base_subcollection, "béta34"]))

    def test_rename_only_longer_length(self):
        # imv above/béta12 above/béta345
        self.do_imv_test("/".join([self.base_subcollection, "béta345"]))

    def test_rename_only_shorter_length(self):
        # imv above/béta12 above/béta3
        self.do_imv_test("/".join([self.base_subcollection, "béta3"]))

    def test_move_up_rename_same_length(self):
        # imv above/béta12 béta34
        self.do_imv_test("béta34")

    def test_move_up_rename_longer_length(self):
        # imv above/béta12 béta345
        self.do_imv_test("béta345")

    def test_move_up_rename_shorter_length(self):
        # imv above/béta12 béta3
        self.do_imv_test("béta3")

    def test_move_down_rename_same_length(self):
        # imv above/béta12 above/sibling/béta34
        self.do_imv_test("/".join([self.base_subcollection, self.sibling_subcollection, "béta34"]))

    def test_move_down_rename_longer_length(self):
        # imv above/béta12 above/sibling/béta345
        self.do_imv_test("/".join([self.base_subcollection, self.sibling_subcollection, "béta345"]))

    def test_move_down_rename_shorter_length(self):
        # imv above/béta12 above/sibling/béta3
        self.do_imv_test("/".join([self.base_subcollection, self.sibling_subcollection, "béta3"]))

    def test_move_down_rename_same_length_with_additional_multibyte_character(self):
        # imv above/béta12 above/sibling/bété34
        self.do_imv_test("/".join([self.base_subcollection, self.sibling_subcollection, "bété34"]))

    def test_move_down_rename_longer_length_with_additional_multibyte_character(self):
        # imv above/béta12 above/sibling/béété34
        self.do_imv_test("/".join([self.base_subcollection, self.sibling_subcollection, "béété34"]))

    def test_move_down_rename_shorter_length_with_additional_multibyte_character(self):
        # imv above/béta12 above/sibling/béé34
        self.do_imv_test("/".join([self.base_subcollection, self.sibling_subcollection, "béé34"]))
