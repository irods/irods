import os
import sys
from enum import Enum

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from .. import lib
from .. import test

class Test_ichmod(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):
    def setUp(self):
        super(Test_ichmod, self).setUp()

        self.admin = self.admin_sessions[0]
        self.user = self.user_sessions[0]

    def tearDown(self):
        super(Test_ichmod, self).tearDown()

    def test_ichmod_r_with_garbage_path(self):
        self.admin.assert_icommand('imkdir -p sub_dir1\\\\%/subdir2/')
        self.admin.assert_icommand('ichmod read ' + self.user.username + ' -r sub_dir1\\\\%/')
        self.admin.assert_icommand('ichmod inherit -r sub_dir1\\\\%/')
        filepath = os.path.join(self.admin.local_session_dir, 'file')
        lib.make_file(filepath, 1)
        self.admin.assert_icommand('iput ' + filepath + ' sub_dir1\\\\%/subdir2/')
        self.user.assert_icommand('iget ' + self.admin.session_collection + '/sub_dir1\\\\%/subdir2/file ' + os.path.join(self.user.local_session_dir, ''))

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'some error messages differ depending on where one is connected, so skip in this case for now')
    def test_ichmod_access_levels(self):
        filename1 = 'test_ichmod_access_levels'
        filename2 = 'test_access_levels_put_file'
        local_file_path = os.path.join(self.admin.local_session_dir, filename1)
        public_collection = os.path.join('/' + self.admin.zone_name, 'home', 'public')

        test_object = os.path.join(public_collection, filename1)
        test_collection_for_put = os.path.join(public_collection, 'test_ichmod_collection_put')
        test_collection_for_access = os.path.join(public_collection, 'test_ichmod_collection_access')
        test_collection_for_create = os.path.join(test_collection_for_put, 'new_collection')

        user_file_path = os.path.join(self.user.local_session_dir, filename2)
        user_iget_file = os.path.join(self.user.local_session_dir, filename1)

        coll_access_path = os.path.join(test_collection_for_put, filename1)

        class Access(Enum):
            NULL            = 1
            READ_METADATA   = 2
            READ_OBJECT     = 3
            CREATE_METADATA = 4
            MODIFY_METADATA = 5
            DELETE_METADATA = 6
            CREATE_OBJECT   = 7
            MODIFY_OBJECT   = 8
            DELETE_OBJECT   = 9
            OWN             = 10

        # The access_specifier can be a string, or a list of strings
        # if aliases exist. All aliases will be tested
        access_specifiers = {
            Access.NULL            : "null",
            Access.READ_METADATA   : "read_metadata",
            Access.READ_OBJECT     : ["read_object", "read"],
            Access.CREATE_METADATA : "create_metadata",
            Access.MODIFY_METADATA : "modify_metadata",
            Access.DELETE_METADATA : "delete_metadata",
            Access.CREATE_OBJECT   : "create_object",
            Access.MODIFY_OBJECT   : ["modify_object", "write"],
            Access.DELETE_OBJECT   : "delete_object",
            Access.OWN             : "own"
        }

        # Here the test commands we test with every access level are defined:
        # 'cmd' is the command to run with assert_icommand
        # the 'checks' list contains an access level range and the output of the command to match
        # in the format (<min_level>, <max_level>, <arguments>)
        # *arguments will be passed to the assert_icommands function
        #
        #
        test_commands = {
            # Test that iget needs READ OBJECT
            'iget': {
                'cmd': ['iget' , '-f', test_object, user_iget_file],
                'checks': [
                    (Access.NULL, Access.READ_METADATA,  ['STDERR', 'USER_INPUT_PATH_ERR']),
                    (Access.READ_OBJECT, Access.OWN,   [])
                ]
            },
            # Test that iput needs MODIFY_OBJECT on the collection
            # NB why is CREATE OBJECT not enough?
            'iput': {
                'cmd': ['iput' , user_file_path, os.path.join(test_collection_for_put, filename2)],
                'checks': [
                    (Access.NULL, Access.CREATE_OBJECT, ['STDERR', 'CAT_NO_ACCESS_PERMISSION']),
                    (Access.MODIFY_OBJECT, Access.OWN,  [])
                ]
            },
            # Test that READ OBJECT is needed to read dataobject metadata
            'imeta_dataobject_read': {
                'cmd': ['imeta', 'ls', '-d', test_object],
                'checks': [
                    (Access.NULL, Access.READ_METADATA, ['STDERR', 'does not exist']),
                    (Access.READ_OBJECT, Access.OWN, ['STDOUT', 'attribute: attr1'])
                ]
            },
            # To modify metadata on an object, CREATE METADATA is required
            'imeta_dataobject_modify': {
                'cmd': ['imeta', 'set', '-d', test_object, 'attr1', 'new_value', 'and_new_unit'],
                'checks': [
                    (Access.NULL, Access.READ_OBJECT,  ['STDERR', 'CAT_NO_ACCESS_PERMISSION']),
                    (Access.CREATE_METADATA, Access.OWN, [])
                ]
            },
            # Test that DELETE METADATA is required to delete metadata from a dataobject
            'imeta_dataobject_delete': {
                'cmd': ['imeta', 'rm', '-d', test_object, 'remove', 'this', 'avu'],
                'checks': [
                    (Access.NULL, Access.MODIFY_METADATA, ['STDERR', 'CAT_NO_ACCESS_PERMISSION']),
                    (Access.DELETE_METADATA, Access.OWN, [])
                ]
            },
            # Test that READ OBJECT is needed to read collection metadata
            'imeta_collection_read': {
                'cmd': ['imeta', 'ls', '-C', test_collection_for_access],
                'checks': [
                    (Access.NULL, Access.READ_METADATA, ['STDERR', 'does not exist']),
                    (Access.READ_OBJECT , Access.OWN, ['STDOUT', 'attribute: attr2'])
                ]
            },
            # Test that CREATE METADATA is needed to modify dataobject metadata
            'imeta_collection_modify': {
                'cmd': ['imeta', 'set', '-C', test_collection_for_access, 'coll_attr', 'coll_value', 'new_coll_unit'],
                'checks': [
                    (Access.NULL, Access.READ_OBJECT , ['STDERR', 'CAT_NO_ACCESS_PERMISSION']),
                    (Access.CREATE_METADATA, Access.OWN, [])
                ]
            },
            # Test that DELETE METADATA is required to delete metadata from a collection
            'imeta_collection_delete': {
                'cmd': ['imeta', 'rm', '-C', test_collection_for_access, 'remove', 'this', 'coll_avu'],
                'checks': [
                    (Access.NULL, Access.MODIFY_METADATA, ['STDERR', 'CAT_NO_ACCESS_PERMISSION']),
                    (Access.DELETE_METADATA, Access.OWN, [])
                ]
            },
            # Test that OWN is required to delete a dataobject
            'irm': {
                'cmd': ['irm', test_object],
                'checks': [
                    (Access.NULL, Access.READ_METADATA, ['STDERR', 'does not exist']),
                    (Access.READ_OBJECT , Access.DELETE_OBJECT, ['STDERR', 'CAT_NO_ACCESS_PERMISSION']),
                    (Access.OWN, Access.OWN, [])
                ]
            },
            # Test that you need READ OBJECT on a collection to delete a dataobject in that collection
            'irm_coll_perms': {
                'cmd': ['irm', coll_access_path],
                'checks': [
                    (Access.NULL, Access.READ_METADATA, ['STDERR', 'does not exist']),
                    (Access.READ_OBJECT, Access.OWN, [])
                ]
            },
            # Test that MODIFY_OBJECT is needed to create a sub collection
            'imkdir': {
                'cmd': ['imkdir', test_collection_for_create],
                'checks': [
                    (Access.NULL, Access.CREATE_OBJECT, ['STDERR', 'CAT_NO_ACCESS_PERMISSION']),
                    (Access.MODIFY_OBJECT, Access.OWN, [])
                ]
            },
            # Test for irmdir required permissions (DELETE_OBJECT)
            'irmdir': {
                'cmd': ['irmdir', test_collection_for_access],
                'checks': [
                    (Access.NULL, Access.READ_METADATA, ['STDOUT', 'does not exist']),
                    (Access.READ_OBJECT, Access.MODIFY_OBJECT, ['STDOUT', 'rcRmColl failed with error']),
                    (Access.DELETE_OBJECT, Access.OWN, [])
                ]
            }
        }

        lib.make_file(local_file_path, 1)
        lib.make_file(user_file_path, 1)

        for access_level in Access:
            for access_level_alias in lib.iterfy(access_specifiers[access_level]):
            # Set up some collections and dataobjects
                self.admin.run_icommand(['ichmod', '-r', '-M', 'own', self.admin.username, public_collection])
                self.admin.run_icommand(['irm','-rf', test_object, test_collection_for_put, test_collection_for_access])
                self.admin.run_icommand(['iput', '-f', local_file_path, test_object])
                self.admin.run_icommand(['imkdir', test_collection_for_put, test_collection_for_access])
                self.admin.run_icommand(['ichmod', access_level_alias, self.user.username, test_object])
                self.admin.run_icommand(['ichmod', access_level_alias, self.user.username, test_collection_for_put])
                self.admin.run_icommand(['ichmod', access_level_alias, self.user.username, test_collection_for_access])
                self.admin.run_icommand(['iput', '-f', local_file_path, coll_access_path])
                self.admin.run_icommand(['ichmod', 'own', self.user.username, coll_access_path])
                self.admin.run_icommand(['imeta', 'set', '-d', test_object, 'attr1', 'val1', 'unit1'])
                self.admin.run_icommand(['imeta', 'set', '-d', test_object, 'remove', 'this', 'avu'])
                self.admin.run_icommand(['imeta', 'set', '-C', test_collection_for_access, 'attr2', 'val2', 'unit2'])
                self.admin.run_icommand(['imeta', 'set', '-C', test_collection_for_access, 'remove', 'this', 'coll_avu'])
                for test_command, params in test_commands.items():
                    for check in params.get('checks'):
                        if access_level.value in range(check[0].value, check[1].value + 1):
                            self.user.assert_icommand(params.get('cmd'), *check[2])

    def test_ichmod_does_not_bypass_permission_model_for_data_object__issue_6579(self):
        data_object = 'testFile'
        user_permissions = ['null',
                            'read_metadata',
                            'read',
                            'read_object',
                            'create_metadata',
                            'modify_metadata',
                            'delete_metadata',
                            'create_object',
                            'write',
                            'modify_object',
                            'delete_object',
                            'own']

        self.user.assert_icommand(['itouch', data_object])

        for permission in user_permissions:
            with self.subTest(permission):
                self.user.assert_icommand(['ichmod', permission, self.user.username, data_object])
                out, err, ec = self.user.run_icommand(['ichmod', 'own', self.user.username, data_object])

                if permission == 'own':
                    self.assertEqual(ec, 0)
                    self.assertEqual(len(err), 0)
                    self.assertEqual(len(out), 0)
                else:
                    self.assertEqual(ec, 8)
                    self.assertIn('CAT_NO_ACCESS_PERMISSION', err)
                    self.assertEqual(len(out), 0)

                    # Restore data object permissions or error will be thrown on cleanup
                    self.admin.assert_icommand(['ichmod', '-M', 'own', self.user.username, os.path.join(self.user.session_collection, data_object)])

    def test_ichmod_does_not_bypass_permission_model_for_collection__issue_6579(self):
        collection = 'testCol'
        user_permissions = ['null',
                            'read_metadata',
                            'read',
                            'read_object',
                            'create_metadata',
                            'modify_metadata',
                            'delete_metadata',
                            'create_object',
                            'write',
                            'modify_object',
                            'delete_object',
                            'own']

        self.user.assert_icommand(['imkdir', collection])

        for permission in user_permissions:
            with self.subTest(permission):
                self.user.assert_icommand(['ichmod', permission, self.user.username, collection])
                out, err, ec = self.user.run_icommand(['ichmod', 'own', self.user.username, collection])

                if permission == 'own':
                    self.assertEqual(ec, 0)
                    self.assertEqual(len(err), 0)
                    self.assertEqual(len(out), 0)
                else:
                    self.assertEqual(ec, 8)
                    self.assertIn('CAT_NO_ACCESS_PERMISSION', err)
                    self.assertEqual(len(out), 0)

                    # Restore collection permissions or error will be thrown on cleanup
                    self.admin.assert_icommand(['ichmod', '-M', 'own', self.user.username, os.path.join(self.user.session_collection, collection)])


class test_collection_acl_inheritance(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):
    def setUp(self):
        super(test_collection_acl_inheritance, self).setUp()

        self.admin = self.admin_sessions[0]
        self.user = self.user_sessions[0]

        self.collection = os.path.join(self.admin.session_collection, 'sub_dir1', 'subdir2')
        self.admin.assert_icommand(['imkdir', '-p', self.collection])
        self.admin.assert_icommand(['ichmod', 'read', self.user.username, '-r', os.path.dirname(self.collection)])
        self.admin.assert_icommand(['ichmod', 'inherit', '-r', os.path.dirname(self.collection)])

    def tearDown(self):
        super(test_collection_acl_inheritance, self).tearDown()

    def test_collection_acl_inheritance_for_iput__issue_3032(self):
        filepath = os.path.join(self.admin.local_session_dir, 'file')
        lib.make_file(filepath, 1)

        logical_path = os.path.join(self.collection, 'file')

        # Put the data object into the inheritance structure and ensure that the user given
        # permission to read is able to get the data object.
        self.admin.assert_icommand(['iput', filepath, logical_path])
        self.admin.assert_icommand(['ils', '-LA', self.collection], 'STDOUT', 'file')
        self.user.assert_icommand(['iget', logical_path, os.path.join(self.user.local_session_dir, '')])

    def test_collection_acl_inheritance_for_icp__issue_3032(self):
        source_logical_path = os.path.join(self.admin.session_collection, 'file')
        destination_logical_path = os.path.join(self.collection, 'icp_file')
        filepath = os.path.join(self.admin.local_session_dir, 'file')
        lib.make_file(filepath, 1)

        # Copy from a data object outside of the inheritance structure to ensure that the ACL
        # inheritance is not applied to the original object.
        self.admin.assert_icommand(['iput', filepath, source_logical_path])
        self.admin.assert_icommand(['ils', '-LA', source_logical_path], 'STDOUT', 'file')

        # Copy the data object into the inheritance structure and ensure that the user given
        # permission to read is able to get the data object.
        self.admin.assert_icommand(['icp', source_logical_path, destination_logical_path])
        self.admin.assert_icommand(['ils', '-LA', self.collection], 'STDOUT', 'icp_file')
        self.user.assert_icommand(['iget', destination_logical_path, os.path.join(self.user.local_session_dir, '')])
