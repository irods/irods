from __future__ import print_function
import os
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from .. import test
from .. import lib

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

            original_mtime = get_collection_mtime(self.admin, collection_path)

            self.assertTrue(lib.replica_exists(self.user, logical_path, 0))
            self.user.assert_icommand(['irm', logical_path])
            self.assertFalse(lib.replica_exists(self.user, logical_path, 0))

            new_mtime = get_collection_mtime(self.admin, collection_path)

            self.assertNotEqual(original_mtime, new_mtime, msg='collection mtime was not updated')

        finally:
            self.user.assert_icommand(['irmtrash'])
            self.admin.assert_icommand(['irm', '-r', '-f', collection_path])
            self.admin.assert_icommand(['irmtrash', '-M'])
