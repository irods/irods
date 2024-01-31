from __future__ import print_function
import os
import shutil
import socket
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from .. import test
from .. import lib

class Test_Iunreg(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):
    def setUp(self):
        super(Test_Iunreg, self).setUp()
        self.admin = self.admin_sessions[0]
        self.user = self.user_sessions[0]

        self.vault_path_root = os.path.join(os.getcwd(), 'regvault')
        os.mkdir(self.vault_path_root)
        self.resc1 = 'regHere'
        self.admin.assert_icommand([
            'iadmin', 'mkresc', self.resc1, 'unixfilesystem',
            ':'.join([socket.gethostname(), self.vault_path_root])],
            'STDOUT', 'Creating resource')

        self.registered_filename = 'test_iunreg_file'
        self.local_data_path = str(os.path.join(self.admin.local_session_dir, self.registered_filename))
        lib.make_file(self.local_data_path, 1024)

        self.logical_path_to_obj = str(os.path.join(self.user.session_collection, self.registered_filename))
        self.data_in_default_vault_path = str(os.path.join(self.user.get_vault_session_path(), self.registered_filename))
        self.data_in_repl_vault_path = str(os.path.join(self.vault_path_root))

    def tearDown(self):
        self.admin.assert_icommand(['iadmin', 'rmresc', self.resc1])
        shutil.rmtree(self.vault_path_root, ignore_errors=True)
        if os.path.exists(self.local_data_path):
            os.unlink(self.local_data_path)
        super(Test_Iunreg, self).tearDown()

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, "Skip for topology testing from resource server")
    def test_iunreg_from_vault(self):
        # Put a file in rodsuser's home collection
        # The user should not be able to unregister from the vault
        self.user.assert_icommand(['iput', self.local_data_path])
        self.user.assert_icommand(['irepl', '-R', self.resc1, self.logical_path_to_obj])
        self.user.assert_icommand(['iunreg', self.logical_path_to_obj], 'STDERR', 'CANT_UNREG_IN_VAULT_FILE')
        self.user.assert_icommand(['ils', '-L', self.logical_path_to_obj], 'STDOUT', self.data_in_default_vault_path)
        # Unregister from the vault as admin
        self.admin.assert_icommand(['ichmod', '-M', 'own', self.admin.username, self.logical_path_to_obj])
        self.admin.assert_icommand(['iunreg', self.logical_path_to_obj])
        self.user.assert_icommand(['ils', '-L', self.logical_path_to_obj], 'STDERR', 'does not exist')
        self.assertTrue(
            os.path.exists(self.data_in_default_vault_path),
            msg='Data missing from vault after unregister:[{}]'.format(self.data_in_default_vault_path))
        self.assertTrue(
            os.path.exists(self.data_in_repl_vault_path),
            msg='Data missing from vault after unregister:[{}]'.format(self.data_in_repl_vault_path))

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, "Skip for topology testing from resource server")
    def test_iunreg_non_vault(self):
        # Register a file in rodsuser's home collection and give only read/write access
        self.admin.assert_icommand(['ichmod', '-M', '-r', 'own', self.admin.username, self.user.session_collection])
        self.admin.assert_icommand(['ireg', self.local_data_path, self.logical_path_to_obj])
        self.admin.assert_icommand(['ichmod', 'write', self.user.username, self.logical_path_to_obj])
        self.user.assert_icommand(['iunreg', self.logical_path_to_obj], 'STDERR', 'CAT_NO_ACCESS_PERMISSION')
        self.admin.assert_icommand(['ils', '-L', self.logical_path_to_obj], 'STDOUT', self.local_data_path)
        # Now grant ownership - the user should be able to unregister
        self.admin.assert_icommand(['ichmod', 'own', self.user.username, self.logical_path_to_obj])
        self.user.assert_icommand(['iunreg', self.logical_path_to_obj])
        self.user.assert_icommand(['ils', '-L', self.logical_path_to_obj], 'STDERR', 'does not exist')
        self.assertTrue(
            os.path.exists(self.local_data_path),
            msg='Data missing from local file after unregister:[{}]'.format(self.local_data_path))

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, "Skip for topology testing from resource server")
    def test_iunreg_vault_and_non_vault(self):
        self.admin.assert_icommand(['ichmod', '-M', '-r', 'own', self.admin.username, self.user.session_collection])
        try:
            # Register a file in rodsuser's home collection and replicate to a vault
            self.admin.assert_icommand(['ireg', self.local_data_path, self.logical_path_to_obj])
            self.admin.assert_icommand(['ichmod', 'own', self.user.username, self.logical_path_to_obj])
            self.user.assert_icommand(['irepl', '-R', self.resc1, self.logical_path_to_obj])
            # iunreg by rodsuser should fail to unregister the item in the vault, but not the local file
            self.user.assert_icommand(['iunreg', self.logical_path_to_obj], 'STDERR', 'CANT_UNREG_IN_VAULT_FILE')
            out,_,_ = self.user.run_icommand(['ils', '-L', self.logical_path_to_obj])
            self.assertTrue(
                self.local_data_path not in out,
                msg='Failed to unregister replica in [{}]'.format(self.local_data_path))
            self.assertTrue(
                os.path.exists(self.local_data_path),
                msg='Data missing from local file after unregister:[{}]'.format(self.local_data_path))
            self.data_in_repl_vault_path = str(os.path.join(self.vault_path_root))
            self.assertTrue(
                self.data_in_repl_vault_path in out,
                msg='Replica unregistered from vault by rodsuser:[{}]'.format(self.local_data_path))
            self.assertTrue(
                os.path.exists(self.data_in_repl_vault_path),
                msg='Data missing from vault after unregister:[{}]'.format(self.data_in_repl_vault_path))
        finally:
            self.admin.assert_icommand(['iunreg', '-M', self.logical_path_to_obj])

    def test_iunreg_object_admin_flag(self):
        # Put a file in rodsuser's home collection
        self.user.assert_icommand(['iput', '-R', self.resc1, self.local_data_path])
        # Remove permissions from rodsadmin
        self.admin.assert_icommand(['ichmod', '-M', 'own', self.admin.username, self.logical_path_to_obj])
        self.admin.assert_icommand(['ichmod', '-M', 'null', self.admin.username, self.logical_path_to_obj])
        # Unregister from the vault as admin (and fail)
        self.admin.assert_icommand(['iunreg', self.logical_path_to_obj], 'STDERR', 'CAT_NO_ACCESS_PERMISSION')
        self.user.assert_icommand(['ils', '-L', self.logical_path_to_obj], 'STDOUT', self.data_in_repl_vault_path)
        # Again, with feeling (admin flag)
        self.admin.assert_icommand(['iunreg', '-M', self.logical_path_to_obj])
        self.user.assert_icommand(['ils', '-L', self.logical_path_to_obj], 'STDERR', 'does not exist')
        self.assertTrue(
            os.path.exists(self.data_in_repl_vault_path),
            msg='Data missing from vault after unregister:[{}]'.format(self.data_in_repl_vault_path))

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, "Skip for topology testing from resource server")
    def test_iunreg_replica_number(self):
        logical_path_to_obj = str(os.path.join(self.admin.session_collection, self.registered_filename))
        # Register file and replicate to another resource
        self.admin.assert_icommand(['ireg', self.local_data_path, logical_path_to_obj])
        self.admin.assert_icommand(['irepl', '-R', self.resc1, logical_path_to_obj])
        # Unregister 1 of the replicas by number
        # The implied minimum number of replicas to keep is 2, so no unreg occurs here as it is using the trim API.
        self.admin.assert_icommand(['iunreg', '-n0', logical_path_to_obj], 'STDOUT', 'Number of files trimmed = 0.')
        self.admin.assert_icommand(['iunreg', '-n1', '-N1', logical_path_to_obj], 'STDOUT', 'Number of files trimmed = 1.')
        out,_,_ = self.admin.run_icommand(['ils', '-L', logical_path_to_obj])
        self.assertTrue(
            self.local_data_path in out,
            msg='Erroneously unregistered replica:[{}]'.format(self.local_data_path))
        self.assertTrue(
            os.path.exists(self.local_data_path),
            msg='Data missing from local file after unregister:[{}]'.format(self.local_data_path))
        self.assertTrue(
            self.data_in_repl_vault_path not in out,
            msg='Failed to unregister replica:[{}]'.format(self.data_in_repl_vault_path))
        self.assertTrue(
            os.path.exists(self.data_in_repl_vault_path),
            msg='Data missing from vault after unregister:[{}]'.format(self.data_in_repl_vault_path))

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, "Skip for topology testing from resource server")
    def test_iunreg_target_resource(self):
        logical_path_to_obj = str(os.path.join(self.admin.session_collection, self.registered_filename))
        self.admin.assert_icommand(['ireg', self.local_data_path, logical_path_to_obj])
        self.admin.assert_icommand(['irepl', '-R', self.resc1, logical_path_to_obj])
        self.admin.assert_icommand(['iunreg', '-S', self.resc1, '-N1', logical_path_to_obj], 'STDOUT', 'Number of files trimmed = 1.')
        out,_,_ = self.admin.run_icommand(['ils', '-L', logical_path_to_obj])
        self.assertTrue(
            self.local_data_path in out,
            msg='Erroneously unregistered replica:[{}]'.format(self.local_data_path))
        self.assertTrue(
            os.path.exists(self.local_data_path),
            msg='Data missing from local file after unregister:[{}]'.format(self.local_data_path))
        self.assertTrue(
            self.data_in_repl_vault_path not in out,
            msg='Failed to unregister replica:[{}]'.format(self.data_in_repl_vault_path))
        self.assertTrue(
            os.path.exists(self.data_in_repl_vault_path),
            msg='Data missing from vault after unregister:[{}]'.format(self.data_in_repl_vault_path))

    def test_iunreg_recursive_admin_flag(self):
        # Put a file in rodsuser's home collection
        new_coll_name = 'local_collection'
        coll_path = os.path.join(
            os.path.dirname(self.logical_path_to_obj),
            new_coll_name)
        self.user.assert_icommand(['imkdir', coll_path])
        self.user.assert_icommand(['iput', '-R', self.resc1, self.local_data_path, coll_path])
        # Remove permissions from rodsadmin
        self.admin.assert_icommand(['ichmod', '-M', '-r', 'own', self.admin.username, coll_path])
        self.admin.assert_icommand(['ichmod', '-M', '-r', 'null', self.admin.username, coll_path])
        # Unregister from the vault as admin (and fail)
        self.admin.assert_icommand(['iunreg', '-r', coll_path], 'STDERR', 'CAT_NO_ACCESS_PERMISSION')
        self.user.assert_icommand(['ils', '-L', '-r', coll_path], 'STDOUT', self.data_in_repl_vault_path)
        # Again, with feeling (admin flag)
        self.admin.assert_icommand(['iunreg', '-M', '-r', coll_path])
        self.user.assert_icommand(['ils', '-L', '-r', coll_path], 'STDERR', 'does not exist')
        self.assertTrue(
            os.path.exists(self.data_in_repl_vault_path),
            msg='Data missing from vault after unregister:[{}]'.format(self.data_in_repl_vault_path))

