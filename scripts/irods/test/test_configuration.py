import copy
import json
import os
import socket
import subprocess
import time
import unittest

from . import session
from ..configuration import IrodsConfig
from ..controller import IrodsController
from .. import lib
from .. import paths
from .. import test

plugin_name = IrodsConfig().default_rule_engine_plugin

@unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language',
                     'Only implemented for NREP. See #4094.')
class TestConfigurationReload(session.make_sessions_mixin([('otherrods', 'rods')], []), unittest.TestCase):

    def setUp(self):
        super(TestConfigurationReload, self).setUp()
        self.admin = self.admin_sessions[0]

    @staticmethod
    def prepare_rule(property_name):
        """Prepare a query about the server settings."""
        return f"""
        msi_get_server_property("{property_name}",*thing);
        writeLine("stdout","*thing");
        """

    # Test to see that it recognizes the configuration's change
    def test_server_correctly_detects_change_in_server_config(self):
        irodsctl = IrodsController()
        server_config_filename = paths.server_config_path()
        prop_name = "is_issue_6456_done"

        try:
            with lib.file_backed_up(server_config_filename):
                # Load the server configuration file into memory.
                with open(server_config_filename) as f:
                    svr_cfg = json.load(f)

                for prop_value in ['No?', '#6456 is done']:
                    # Add a new configuration property and reload the configuration so that it
                    # is picked up by the server.
                    svr_cfg[prop_name] = prop_value
                    with open(server_config_filename, 'w') as f:
                        f.write(json.dumps(svr_cfg, sort_keys=True, indent=4, separators=(',', ': ')))
                    irodsctl.reload_configuration()

                    # Show the property's value was captured by the server.
                    rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
                    rule = TestConfigurationReload.prepare_rule(f"/{prop_name}")
                    self.admin.assert_icommand(['irule', '-r', rep_name, rule, 'null', 'ruleExecOut'], "STDOUT", [prop_value])

        finally:
            irodsctl.reload_configuration()


class test_server_configuration__issue_8012(session.make_sessions_mixin([('otherrods', 'rods')], []), unittest.TestCase):

    def setUp(self):
        super(test_server_configuration__issue_8012, self).setUp()
        self.otherrods = self.admin_sessions[0]

    def tearDown(self):
        super(test_server_configuration__issue_8012, self).tearDown()

    def test_server_is_functional_without_service_environment_json_file(self):
        with session.make_session_for_existing_admin() as rods:
            # Let's confirm the service account user's irods_environment.json file is configured
            # correctly. That is, make sure we can interact with the server.
            rods.assert_icommand(['ils'], 'STDOUT', [rods.home_collection])

            resc_name = 'issue_8012_ufs_resc'
            data_object = f'{self.otherrods.session_collection}/issue_8012_data_object.txt'

            try:
                # Break the service account user's irods_environment.json file so the icommands
                # no longer work. This helps prove the server no longer cares about it.
                old_rods_environment = copy.deepcopy(rods.environment_file_contents)
                rods.environment_file_contents.update({'irods_port': 1257})
                rods.assert_icommand(['ils'], 'STDERR', ['-305111 USER_SOCK_CONNECT_ERR'])

                # With the service account user's environment now broken, we start the true tests.

                # Create a new unixfilesystem resource. This resource will be used to confirm
                # redirection works in a topology environment. Creating the resource on
                # HOSTNAME_2 means a server redirect will be required to reach the target
                # resource (when creating a replica).
                lib.create_ufs_resource(self.otherrods, resc_name, test.settings.HOSTNAME_2)

                controller = IrodsController()

                with self.subTest('Server is functional after reloading the configuration'):
                    controller.reload_configuration()
                    self.otherrods.assert_icommand(['itouch', '-R', resc_name, data_object])
                    self.otherrods.assert_icommand(['ils', '-l', data_object], 'STDOUT', [os.path.basename(data_object), resc_name])
                    # Show that the service account user's environment is still in a bad state.
                    rods.assert_icommand(['ils'], 'STDERR', ['-305111 USER_SOCK_CONNECT_ERR'])

                with self.subTest('Server is functional after restart'):
                    controller.restart(test_mode=True)
                    old_mtime = lib.get_replica_mtime(self.otherrods, data_object, 0)
                    time.sleep(2) # Make sure the mtime changes.
                    self.otherrods.assert_icommand(['itouch', '-R', resc_name, data_object])
                    self.assertGreater(lib.get_replica_mtime(self.otherrods, data_object, 0), old_mtime)
                    # Show that the service account user's environment is still in a bad state.
                    rods.assert_icommand(['ils'], 'STDERR', ['-305111 USER_SOCK_CONNECT_ERR'])

            finally:
                self.otherrods.run_icommand(['irm', '-f', data_object])
                self.otherrods.run_icommand(['iadmin', 'rmresc', resc_name])
                rods.environment_file_contents = old_rods_environment 

            # Now that the service account user's environment has been restored, show they can
            # execute icommands again.
            rods.assert_icommand(['ils'], 'STDOUT', [rods.home_collection])

    @unittest.skipUnless(test.settings.USE_SSL, 'Requires TLS-enabled enivronment')
    @unittest.skipUnless(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Must be run from a Catalog Service Consumer')
    def test_server_honors_tls_options_for_server_to_server_connections(self):
        controller = IrodsController()

        try:
            config = IrodsConfig()
            with lib.file_backed_up(config.server_config_path):
                # The servers are properly configured at the start of this test, therefore we can
                # confirm correctness by misconfiguring the local server and watching it report TLS
                # errors.
                lib.update_json_file_from_dict(config.server_config_path, {
                    'tls_client': {
                        'ca_certificate_file': 'invalid',
                        'verify_server': 'cert'
                    }
                })
                controller.reload_configuration()

                # "ils" will redirect from the consumer to the provider because it needs to query the
                # catalog. This will result in the consumer server loading the TLS options to establish
                # a server-to-server connection.
                self.otherrods.assert_icommand(['ils'], 'STDERR', ['-2103000 SSL_HANDSHAKE_ERROR'])

        finally:
            controller.reload_configuration()


@unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, 'Only requires a single server')
class test_maximum_size_for_single_buffer_in_megabytes_constraints__issue_8373(unittest.TestCase):
    def test_configuration_validation_reports_values_under_minimum(self):
        config = IrodsConfig()
        with lib.file_backed_up(config.server_config_path):
            config.server_config["advanced_settings"]["maximum_size_for_single_buffer_in_megabytes"] = 0
            lib.update_json_file_from_dict(config.server_config_path, config.server_config)
            res = subprocess.run(['irodsServer', '-c'], capture_output=True, text=True)
            self.assertNotEqual(res.returncode, 0)
            self.assertIn('"valid": false', res.stderr)
            self.assertIn('"instanceLocation": "/advanced_settings/maximum_size_for_single_buffer_in_megabytes"', res.stderr)
            self.assertIn('"error": "Minimum value is 1 but found', res.stderr)

    def test_configuration_validation_reports_values_over_maximum(self):
        config = IrodsConfig()
        with lib.file_backed_up(config.server_config_path):
            config.server_config["advanced_settings"]["maximum_size_for_single_buffer_in_megabytes"] = 2048
            lib.update_json_file_from_dict(config.server_config_path, config.server_config)
            res = subprocess.run(['irodsServer', '-c'], capture_output=True, text=True)
            self.assertNotEqual(res.returncode, 0)
            self.assertIn('"valid": false', res.stderr)
            self.assertIn('"instanceLocation": "/advanced_settings/maximum_size_for_single_buffer_in_megabytes"', res.stderr)
            self.assertIn('"error": "Maximum value is 2047 but found', res.stderr)
