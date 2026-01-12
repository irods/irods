import json
import os
import sys
import unittest

from . import session
from .. import lib
from .. import paths
from .. import test
from ..configuration import IrodsConfig
from ..controller import IrodsController

admins = [('otherrods', 'rods')]
users  = [('alice', 'rods')]


class Test_Rule_Engine_Plugin_Metadata_Guard(session.make_sessions_mixin(admins, users), unittest.TestCase):

    def setUp(self):
        super(Test_Rule_Engine_Plugin_Metadata_Guard, self).setUp()
        self.rods = session.make_session_for_existing_admin()
        self.admin = self.admin_sessions[0]
        self.user = self.user_sessions[0]

    def tearDown(self):
        super(Test_Rule_Engine_Plugin_Metadata_Guard, self).tearDown()

    def test_incorrect_configuration_does_not_block_usage(self):
        config = IrodsConfig()

        # Set invalid JSON configuration for the root collection.
        root_coll = os.path.join('/', self.admin.zone_name)
        json_config = '{bad JSON config}'
        self.rods.assert_icommand(['imeta', 'set', '-C', root_coll, self.metadata_guard_attribute_name(), json_config])

        # The number of times the expected error message is seen in the log file.
        count = 0

        with lib.file_backed_up(config.server_config_path):
            self.enable_rule_engine_plugin(config)

            # This should cause an exception to be thrown and logged, but users
            # should still be able to proceed. Verify that the log contains a valid
            # error message.
            log_offset = lib.get_file_size_by_path(paths.server_log_path())
            self.rods.assert_icommand(['imeta', 'set', '-C', root_coll, self.metadata_guard_attribute_name(), json_config])
            count = lib.count_occurrences_of_string_in_log(paths.server_log_path(), 'Cannot parse Rule Engine Plugin configuration', log_offset)

        # Clean up.
        self.rods.assert_icommand(['imeta', 'rm', '-M', '-C', root_coll, self.metadata_guard_attribute_name(), json_config])

        self.assertTrue(count > 0)

    def test_authorized_users_via_editors_list_can_manipulate_metadata_in_guarded_namespace(self):
        config = IrodsConfig()

        # Set JSON configuration for the root collection.
        root_coll = os.path.join('/', self.admin.zone_name)
        json_config = json.dumps({
            'prefixes': ['irods::'],
            'admin_only': False,
            'editors': [
                {'type': 'group', 'name': 'rodsadmin'},
                {'type': 'user',  'name': self.admin.username},
                {'type': 'user',  'name': self.rods.username},
                {'type': 'user',  'name': self.user.username}
            ]
        })
        self.rods.assert_icommand(['imeta', 'set', '-C', root_coll, self.metadata_guard_attribute_name(), json_config])

        with lib.file_backed_up(config.server_config_path):
            self.enable_rule_engine_plugin(config)

            coll = self.admin.session_collection
            attribute_name = 'irods::guarded_attribute'
            self.admin.assert_icommand(['imeta', 'set', '-C', coll, attribute_name, 'abc'])
            self.admin.assert_icommand(['ichmod', 'write', self.user.username, coll])

            new_attr_value = 'DEF'
            self.user.assert_icommand(['imeta', 'set', '-C', coll, attribute_name, new_attr_value])
            self.admin.assert_icommand(['imeta', 'rm', '-C', coll, attribute_name, new_attr_value])

        # Clean up.
        self.rods.assert_icommand(['imeta', 'rm', '-M', '-C', root_coll, self.metadata_guard_attribute_name(), json_config])

    def test_unauthorized_users_cannot_manipulate_metadata_in_guarded_namespace(self):
        config = IrodsConfig()

        root_coll = os.path.join('/', self.admin.zone_name)
        json_configs = [
            json.dumps({
                'prefixes': ['irods::'],
                'admin_only': True
            }),
            json.dumps({
                'prefixes': ['irods::'],
                'editors': [
                    {'type': 'user', 'name': self.rods.username},
                    {'type': 'user', 'name': self.admin.username}
                ]
            }),
            json.dumps({
                'prefixes': ['irods::'],
                'editors': [
                    {'type': 'group', 'name': 'rodsadmin'}
                ]
            })
        ]

        self.rods.assert_icommand(['iadmin', 'mkgroup', 'rodsadmin'])
        self.rods.assert_icommand(['iadmin', 'atg', 'rodsadmin', self.admin.username])
        self.rods.assert_icommand(['iadmin', 'atg', 'rodsadmin', self.rods.username])

        for json_config in json_configs:
            # Set JSON configuration for the root collection.
            self.rods.assert_icommand(['imeta', 'set', '-C', root_coll, self.metadata_guard_attribute_name(), json_config])

            with lib.file_backed_up(config.server_config_path):
                self.enable_rule_engine_plugin(config)

                coll = self.admin.session_collection
                attribute_name = 'irods::guarded_attribute'
                self.admin.assert_icommand(['imeta', 'set', '-C', coll, attribute_name, 'abc'])
                self.admin.assert_icommand(['imeta', 'add', '-C', coll, attribute_name, 'def'])

                def check_metadata():
                    out, err, ec = self.admin.run_icommand(['imeta', 'ls', '-C', coll])
                    self.assertEqual(ec, 0)
                    self.assertEqual(len(err), 0)
                    self.assertTrue('attribute: {0}\nvalue: {1}'.format(attribute_name, 'abc') in out)
                    self.assertTrue('attribute: {0}\nvalue: {1}'.format(attribute_name, 'def') in out)
                    self.assertTrue('attribute: {0}\nvalue: {1}'.format(attribute_name, 'DEF') not in out)
                    self.assertTrue('attribute: {0}\nvalue: {1}'.format(attribute_name, 'GHI') not in out)

                # Verify that the metadata is what we expect.
                check_metadata()

                # Give an unauthorized user write access to the collection with protected metadata.
                self.admin.assert_icommand(['ichmod', 'write', self.user.username, coll])

                self.user.assert_icommand(['imeta', 'set', '-C', coll, attribute_name, 'DEF'], 'STDERR', ['CAT_INSUFFICIENT_PRIVILEGE_LEVEL'])
                self.user.assert_icommand(['imeta', 'add', '-C', coll, attribute_name, 'GHI'], 'STDERR', ['CAT_INSUFFICIENT_PRIVILEGE_LEVEL'])

                # Show that the plugin protected the metadata.
                check_metadata()

            # Clean up.
            self.rods.assert_icommand(['imeta', 'rm', '-M', '-C', root_coll, self.metadata_guard_attribute_name(), json_config])

        self.rods.assert_icommand(['iadmin', 'rfg', 'rodsadmin', self.admin.username])
        self.rods.assert_icommand(['iadmin', 'rmgroup', 'rodsadmin'])

    def test_plugin_does_not_throw_exception_when_json_config_has_not_been_set_as_metadata(self):
        config = IrodsConfig()

        with lib.file_backed_up(config.server_config_path):
            self.enable_rule_engine_plugin(config)

            log_offset = lib.get_file_size_by_path(paths.server_log_path())
            self.admin.assert_icommand(['imeta', 'set', '-C', self.admin.session_collection, 'a', 'v'])
            self.admin.assert_icommand(['imeta', 'ls', '-C', self.admin.session_collection], 'STDOUT', ['attribute: a', 'value: v'])

            self.assertEqual(lib.count_occurrences_of_string_in_log(paths.server_log_path(), 'error', log_offset), 0)
            self.assertEqual(lib.count_occurrences_of_string_in_log(paths.server_log_path(), 'exception', log_offset), 0)
            self.assertEqual(lib.count_occurrences_of_string_in_log(paths.server_log_path(), 'SYS_CONFIG_FILE_ERR', log_offset), 0)

    def test_plugin_honors_admin_only_config_option_when_rodsadmins_manipulate_metadata__issue_25(self):
        config = IrodsConfig()

        # Set JSON configuration for the root collection.
        root_coll = os.path.join('/', self.admin.zone_name)
        json_config = json.dumps({
            'prefixes': ['irods::'],
            'admin_only': True
        })
        self.rods.assert_icommand(['imeta', 'set', '-C', root_coll, self.metadata_guard_attribute_name(), json_config])

        with lib.file_backed_up(config.server_config_path):
            self.enable_rule_engine_plugin(config)

            # Show that metadata can be manipulated by admins.
            for attribute_name in ['issue_25', 'irods::issue_25']:
                self.admin.assert_icommand(['imeta', 'set', '-C', self.admin.session_collection, attribute_name, 'v0'])
                self.admin.assert_icommand(['imeta', 'add', '-C', self.admin.session_collection, attribute_name, 'v1'])
                self.admin.assert_icommand(['imeta', 'ls', '-C', self.admin.session_collection], 'STDOUT', ['attribute: ' + attribute_name, 'value: v0', 'value: v1'])
                self.admin.assert_icommand(['imeta', 'rm', '-C', self.admin.session_collection, attribute_name, 'v0'])
                self.admin.assert_icommand(['imeta', 'rm', '-C', self.admin.session_collection, attribute_name, 'v1'])

        # Clean up.
        self.rods.assert_icommand(['imeta', 'rm', '-M', '-C', root_coll, self.metadata_guard_attribute_name(), json_config])

    def test_plugin_honors_admin_only_config_option_when_rodsusers_manipulate_metadata__issue_25(self):
        config = IrodsConfig()

        # Set JSON configuration for the root collection.
        root_coll = os.path.join('/', self.admin.zone_name)
        json_config = json.dumps({
            'prefixes': ['irods::'],
            'admin_only': True
        })
        self.rods.assert_icommand(['imeta', 'set', '-C', root_coll, self.metadata_guard_attribute_name(), json_config])

        with lib.file_backed_up(config.server_config_path):
            self.enable_rule_engine_plugin(config)

            # Show that unguarded metadata can be manipulated by non-admins.
            attribute_name = 'issue_25'
            self.user.assert_icommand(['imeta', 'set', '-C', self.user.session_collection, attribute_name, 'v0'])
            self.user.assert_icommand(['imeta', 'add', '-C', self.user.session_collection, attribute_name, 'v1'])
            self.user.assert_icommand(['imeta', 'ls', '-C', self.user.session_collection], 'STDOUT', ['attribute: ' + attribute_name, 'value: v0', 'value: v1'])
            self.user.assert_icommand(['imeta', 'rm', '-C', self.user.session_collection, attribute_name, 'v0'])
            self.user.assert_icommand(['imeta', 'rm', '-C', self.user.session_collection, attribute_name, 'v1'])

            # Show that the plugin does not allow non-admins to manipulate guarded metadata.
            attribute_name = 'irods::issue_25'
            self.user.assert_icommand(['imeta', 'set', '-C', self.user.session_collection, attribute_name, 'v'], 'STDERR', ['CAT_INSUFFICIENT_PRIVILEGE_LEVEL'])
            self.user.assert_icommand(['imeta', 'add', '-C', self.user.session_collection, attribute_name, 'v'], 'STDERR', ['CAT_INSUFFICIENT_PRIVILEGE_LEVEL'])
            self.user.assert_icommand(['imeta', 'rm', '-C', self.user.session_collection, attribute_name, 'v'], 'STDERR', ['CAT_INSUFFICIENT_PRIVILEGE_LEVEL'])
            self.user.assert_icommand(['imeta', 'ls', '-C', self.user.session_collection], 'STDOUT', ['None'])

        # Clean up.
        self.rods.assert_icommand(['imeta', 'rm', '-M', '-C', root_coll, self.metadata_guard_attribute_name(), json_config])

    def test_plugin_cannot_be_bypassed_via_imeta_mod__issue_58(self):
        config = IrodsConfig()

        # Set JSON configuration for the root collection.
        root_coll = os.path.join('/', self.admin.zone_name)
        json_config = json.dumps({
            'prefixes': ['irods::'],
            'admin_only': True
        })
        self.rods.assert_icommand(['imeta', 'set', '-C', root_coll, self.metadata_guard_attribute_name(), json_config])

        try:
            with lib.file_backed_up(config.server_config_path):
                self.enable_rule_engine_plugin(config)

                self.user.assert_icommand(['imeta', 'add', '-C', self.user.session_collection, 'unprotected::issue58', 'v'])
                self.user.assert_icommand(['imeta', 'mod', '-C', self.user.session_collection, 'unprotected::issue58', 'v', 'n:irods::issue58'], 'STDERR', ['CAT_INSUFFICIENT_PRIVILEGE_LEVEL'])

                # Same as above, but with units
                self.user.assert_icommand(['imeta', 'add', '-C', self.user.session_collection, 'unprotected::withunits', 'v', 'u'])
                self.user.assert_icommand(['imeta', 'mod', '-C', self.user.session_collection, 'unprotected::withunits', 'v', 'u', 'n:irods::withunits'], 'STDERR', ['CAT_INSUFFICIENT_PRIVILEGE_LEVEL'])
                self.user.assert_icommand(['imeta', 'mod', '-C', self.user.session_collection, 'unprotected::withunits', 'v', 'n:irods::withunits', 'u'], 'STDERR', ['CAT_INSUFFICIENT_PRIVILEGE_LEVEL'])
        finally:
            # Clean up.
            self.rods.assert_icommand(['imeta', 'rm', '-M', '-C', root_coll, self.metadata_guard_attribute_name(), json_config])

    #
    # Utility Functions
    #

    def enable_rule_engine_plugin(self, config):
        config.server_config['log_level']['rule_engine'] = 'trace'
        config.server_config['log_level']['legacy'] = 'trace'
        config.server_config['plugin_configuration']['rule_engines'].insert(0, {
            'instance_name': 'irods_rule_engine_plugin-metadata_guard-instance',
            'plugin_name': 'irods_rule_engine_plugin-metadata_guard',
            'plugin_specific_configuration': {}
        })
        lib.update_json_file_from_dict(config.server_config_path, config.server_config)
        IrodsController().reload_configuration()

    def metadata_guard_attribute_name(self):
        return 'irods::metadata_guard'
