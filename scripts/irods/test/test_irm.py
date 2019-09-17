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

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_irm_option_n_is_deprecated__issue_3451(self):
        filename = 'test_file_issue_3451.txt'
        lib.make_file(filename, 1024)
        self.admin.assert_icommand('iput {0}'.format(filename))
        self.admin.assert_icommand('irm -n0 {0}'.format(filename), 'STDOUT', '-n is deprecated.')

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_disallow_simultaneous_usage_of_options_r_and_n__issue_3661(self):
        filename = 'test_file_issue_3661.txt'
        filename_path = os.path.join(self.admin.local_session_dir, filename)
        lib.make_file(filename_path, 1024)

        self.admin.assert_icommand('iput {0}'.format(filename_path))
        self.admin.assert_icommand('irm -rn0 {0}'.format(filename), 'STDERR', 'USER_INCOMPATIBLE_PARAMS')

    def test_irm_delete_collection_with_ampersand_in_name_causes_error__issue_3398(self):
        collection = 'testDeleteACollectionWithAmpInTheNameBug170 && hail hail rock & roll  &'
        self.admin.assert_icommand("imkdir '{0}'".format(collection))
        self.admin.assert_icommand("ils -l '{0}'".format(self.admin.session_collection), 'STDOUT', collection)
        self.admin.assert_icommand("irm -r '{0}'".format(collection))
        self.admin.assert_icommand("ils -l '{0}'".format(self.admin.session_collection_trash), 'STDOUT', collection)

    def test_irm_unregister_as_rodsuser(self):
        filename = 'test_irm_unregister_as_rodsuser'
        local_data_path = str(os.path.join(self.admin.local_session_dir, filename))
        lib.make_file(local_data_path, 1024)

        try:
            # Put a file in rodsuser's home collection
            # The user should not be able to unregister from the vault
            self.user.assert_icommand(['iput', local_data_path])
            data_in_vault_path = str(os.path.join(self.user.get_vault_session_path(), filename))
            logical_path_to_obj = str(os.path.join(self.user.session_collection, filename))
            self.user.assert_icommand(['irm', '-U', logical_path_to_obj], 'STDERR', 'CANT_UNREG_IN_VAULT_FILE')
            self.user.assert_icommand(['ils', '-L', logical_path_to_obj], 'STDOUT', data_in_vault_path)
            # Unregister from the vault as admin
            self.admin.assert_icommand(['ichmod', '-M', 'own', self.admin.username, logical_path_to_obj])
            self.admin.assert_icommand(['irm', '-U', logical_path_to_obj])
            self.user.assert_icommand(['ils', '-L', logical_path_to_obj], 'STDERR', 'does not exist')
            self.assertTrue(os.path.exists(data_in_vault_path), msg='Data missing after unregister in [{}]'.format(data_in_vault_path))

            # Register a file in rodsuser's home collection and give only read/write access
            logical_path_to_registered_file = str(os.path.join(self.user.session_collection, filename))
            self.admin.assert_icommand(['ichmod', '-M', '-r', 'own', self.admin.username, self.user.session_collection])
            self.admin.assert_icommand(['ireg', local_data_path, logical_path_to_registered_file])
            self.admin.assert_icommand(['ichmod', 'write', self.user.username, logical_path_to_registered_file])
            self.user.assert_icommand(['irm', '-U', logical_path_to_registered_file], 'STDERR', 'CAT_NO_ACCESS_PERMISSION')
            self.admin.assert_icommand(['ils', '-L', logical_path_to_registered_file], 'STDOUT', local_data_path)
            # Now grant ownership - the user should be able to unregister
            self.admin.assert_icommand(['ichmod', 'own', self.user.username, logical_path_to_registered_file])
            self.user.assert_icommand(['irm', '-U', logical_path_to_registered_file])
            self.admin.assert_icommand(['ils', '-L', logical_path_to_registered_file], 'STDERR', 'does not exist')
            self.assertTrue(os.path.exists(local_data_path), msg='Data missing after unregister in [{}]'.format(local_data_path))
        finally:
            if os.path.exists(local_data_path):
                os.unlink(local_data_path)
