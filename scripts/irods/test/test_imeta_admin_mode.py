import os
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from .. import test

rodsusers  = [('alice', 'apass')]
rodsadmins = [('otherrods', 'rods')]

class Test_Imeta_Admin_Mode(session.make_sessions_mixin(rodsadmins, rodsusers), unittest.TestCase):

    # The tests in this file are focused on collections and data objects because they
    # are the only entities that permissions apply to. Admin mode ONLY applies to collections
    # and data objects.

    def setUp(self):
        super(Test_Imeta_Admin_Mode, self).setUp()
        self.admin = self.admin_sessions[0]
        self.user = self.user_sessions[0]

    def tearDown(self):
        super(Test_Imeta_Admin_Mode, self).tearDown()

    def test_subcommands_add_adda_set_mod_and_rm__issue_6124(self):
        attr_name  = 'issue_6124_attr'
        attr_value = 'issue_6124_value'

        test_cases = [
            {
                'name': os.path.join(self.user.session_collection, 'issue_6124_data_object'),
                'type': '-d'
            },
            {
                'name': os.path.join(self.user.session_collection, 'issue_6124_collection'),
                'type': '-C'
            }
        ]

        for tc in test_cases:
            object_name = tc['name']
            object_type_option = tc['type']

            for op in ['add', 'adda', 'set']:
                # rError messages do not return to the client after redirect to provider
                if test.settings.TOPOLOGY_FROM_RESOURCE_SERVER and op == 'adda':
                    continue

                try:
                    # Create a data object or collection based on the object type.
                    self.user.assert_icommand(['itouch' if object_type_option == '-d' else 'imkdir', object_name])

                    if op == 'adda':
                        # "adda" automatically enables admin mode, so the administrator is always allowed
                        # to add metadata.
                        self.admin.assert_icommand(['imeta', op, object_type_option, object_name, attr_name, attr_value],
                                                   'STDERR', ['"adda" is deprecated. Please use "add" with admin mode enabled instead.'])
                    else:
                        # Show that without the admin option, the administrator cannot add/set metadata on
                        # another user's collection or data object.
                        self.admin.assert_icommand(['imeta', op, object_type_option, object_name, attr_name, attr_value],
                                                   'STDERR', ['CAT_NO_ACCESS_PERMISSION'])

                        # Show that using the admin option allows the administrator to add metadata to
                        # collections and data objects they do not have permissions on.
                        self.admin.assert_icommand(['imeta', '-M', op, object_type_option, object_name, attr_name, attr_value])

                    # As the owner of the collection or data object, verify that the metadata is attached
                    # to the target object.
                    self.user.assert_icommand(['imeta', 'ls', object_type_option, object_name],
                                              'STDOUT', ['attribute: ' + attr_name, 'value: ' + attr_value])

                    # Show that without the admin option, the administrator cannot modify metadata on
                    # another user's collection or data object.
                    new_attr_value = attr_value + '_updated'
                    self.admin.assert_icommand(['imeta', 'mod', object_type_option, object_name, attr_name, attr_value, 'v:' + new_attr_value],
                                               'STDERR', ['CAT_NO_ACCESS_PERMISSION'])

                    # Show that using the admin option allows the administrator to modify metadata on
                    # collections and data objects they do not have permissions on.
                    self.admin.assert_icommand(['imeta', '-M', 'mod', object_type_option, object_name, attr_name, attr_value, 'v:' + new_attr_value])

                    # Show that the administrator can also remove metadata attached to other user's data.
                    self.admin.assert_icommand(['imeta', '-M', 'rm', object_type_option, object_name, attr_name, new_attr_value])

                    # As the owner of the collection or data object, verify that the metadata is no longer
                    # attached to the target object.
                    self.user.assert_icommand(['imeta', 'ls', object_type_option, object_name], 'STDOUT', ['None\n'])

                finally:
                    self.user.run_icommand(['irm', '-rf', object_name])

    def test_subcommands_addw_and_rmw__issue_6124(self):
        # Create data objects at the root of the current collection and
        # inside of the child collection.
        data_objects = [
            os.path.join(self.user.session_collection, 'foo1'),
            os.path.join(self.user.session_collection, 'foo2')
        ]
        for data_object in data_objects:
            self.user.assert_icommand(['itouch', data_object])

        attr_name  = 'issue_6124_attr'
        attr_value = 'issue_6124_value'

        # Show that without the admin option, the administrator cannot attach metadata to
        # another user's data objects they do not have permissions on.
        self.admin.assert_icommand(['imeta', 'addw', '-d', self.user.session_collection + '/%', attr_name, attr_value],
                                   'STDERR', ['CAT_NO_ACCESS_PERMISSION'])

        # Show that using the admin option allows the administrator to attach metadata to
        # data objects they do not have permissions on.
        self.admin.assert_icommand(['imeta', '-M', 'addw', '-d', self.user.session_collection + '/%', attr_name, attr_value],
                                   'STDOUT', ['AVU added to 2 data-objects'])

        for data_object in data_objects:
            # Show that without the admin option, the administrator cannot detach metadata from
            # another user's data objects they do not have permissions on.
            self.admin.assert_icommand(['imeta', 'rmw', '-d', data_object, '%', '%'], 'STDERR', ['CAT_NO_ACCESS_PERMISSION'])

            # Show that using the admin option allows the administrator to detach metadata from
            # another user's data objects they do not have permissions on.
            self.admin.assert_icommand(['imeta', '-M', 'rmw', '-d', data_object, '%', '%'])

            # As the owner of the data object, verify that the metadata is no longer attached to it.
            self.user.assert_icommand(['imeta', 'ls', '-d', data_object], 'STDOUT', ['None\n'])

    def test_subcommand_rmi__issue_6124(self):
        attr_name  = 'issue_6124_attr'
        attr_value = 'issue_6124_value'

        # True : indicates that the object is a data object.
        # False: indicates that the object is a collection.
        test_cases = [True, False]

        for is_data_object in test_cases:
            object_path = os.path.join(self.user.session_collection, 'foo')

            try:
                object_type_option = self.create_object(object_path, is_data_object)

                # Attach metadata to the newly created data object.
                self.user.assert_icommand(['imeta', 'add', object_type_option, object_path, attr_name, attr_value])

                # Verify that the metadata is attached to the data object.
                self.user.assert_icommand(['imeta', 'ls', object_type_option, object_path], 'STDOUT', ['attribute: ' + attr_name, 'value: ' + attr_value])

                # Get the ID of the metadata attribute name.
                # This will be used to detach the metadata from the data object.
                if is_data_object: gql = "select META_DATA_ATTR_ID where COLL_NAME = '{0}' and DATA_NAME = 'foo'".format(self.user.session_collection)
                else:              gql = "select META_COLL_ATTR_ID where COLL_NAME = '{0}/foo'".format(self.user.session_collection)
                _, out, _ = self.user.assert_icommand(['iquest', '%s', gql], 'STDOUT', ['\n'])
                attribute_id = out.strip()

                # Show that without the admin option, the administrator is not allowed to detach metadata
                # from an object they don't have permissions on.
                self.admin.assert_icommand(['imeta', 'rmi', object_type_option, object_path, attribute_id], 'STDERR', ['CAT_NO_ACCESS_PERMISSION'])

                # Show that with the admin option, the administrator is allowed to detach the metadata from
                # the data object.
                self.admin.assert_icommand(['imeta', '-M', 'rmi', object_type_option, object_path, attribute_id])

            finally:
                self.user.run_icommand(['irm', '-rf', object_path])


    def test_subcommand_cp__issue_6124(self):
        src_object = os.path.join(self.user.session_collection, 'src_object')
        dst_object = os.path.join(self.user.session_collection, 'dst_object')

        attr_name  = 'issue_6124_attr'
        attr_value = 'issue_6124_value'

        # True : indicates that the object is a data object.
        # False: indicates that the object is a collection.
        #
        # The following table produces the following tests:
        # - data object to data object
        # - data object to collection
        # - collection to collection
        # - collection to data object
        test_cases = [
            {'src': True,  'dst': True},
            {'src': True,  'dst': False},
            {'src': False, 'dst': False},
            {'src': False, 'dst': True}
        ]

        for tc in test_cases:
            try:
                # Create the source and destination objects and capture the object type.
                # The newly created objects are owned by "self.user".
                src_object_type_flag = self.create_object(src_object, tc['src'])
                dst_object_type_flag = self.create_object(dst_object, tc['dst'])

                # Attach metadata to the source object.
                self.user.assert_icommand(['imeta', 'set', src_object_type_flag, src_object, attr_name, attr_value])
                self.user.assert_icommand(['imeta', 'ls', src_object_type_flag, src_object], 'STDOUT', ['attribute: ' + attr_name, 'value: ' + attr_value])

                # Copy from source object's metadata to the destination object.
                #
                # First, show that an administrator is not allowed to copy metadata if they do not have
                # the appropriate permissions.
                #
                # Second, show that by using the admin option, an administrator is allowed to copy metadata
                # from one object to another, even if they don't have permissions set on the objects.
                self.admin.assert_icommand(['imeta', 'cp', src_object_type_flag, dst_object_type_flag, src_object, dst_object], 'STDERR', ['CAT_NO_ACCESS_PERMISSION'])
                self.admin.assert_icommand(['imeta', '-M', 'cp', src_object_type_flag, dst_object_type_flag, src_object, dst_object])

                # Verify that the metadata was copied from the source object to the destination object.
                self.user.assert_icommand(['imeta', 'ls', dst_object_type_flag, dst_object], 'STDOUT', ['attribute: ' + attr_name, 'value: ' + attr_value])

            finally:
                self.user.run_icommand(['irm', '-rf', src_object, dst_object])

    def create_object(self, _object_name, _is_data_object):
        if _is_data_object:
            self.user.assert_icommand(['itouch', _object_name])
            return '-d'

        self.user.assert_icommand(['imkdir', _object_name])
        return '-C'

