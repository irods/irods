import unittest

import json
import os
import socket

from . import session
from ..configuration import IrodsConfig
from ..controller import IrodsController
from .. import lib
from .. import paths

SessionsMixin = session.make_sessions_mixin([('otherrods', 'rods')], [])

plugin_name = IrodsConfig().default_rule_engine_plugin

@unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language',
                     'Only implemented for NREP. See #4094.')
class TestConfigurationReload(SessionsMixin, unittest.TestCase):

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

    # Test to see that it recognizes the  configuration's change
    def test_server_correctly_detects_change_in_server_config(self):
        irodsctl = IrodsController()
        server_config_filename = paths.server_config_path()
        PROP_NAME = "is_issue_6456_done"
        with lib.file_backed_up(server_config_filename):
            with open(server_config_filename) as f:
                svr_cfg = json.load(f)
            svr_cfg[PROP_NAME] = "No?"
            with open(server_config_filename, 'w') as f:
                f.write(json.dumps(svr_cfg, sort_keys=True,
                        indent=4, separators=(',', ': ')))

            irodsctl.reload_configuration()
            rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
            self.admin.assert_icommand(['irule', '-r', rep_name, TestConfigurationReload.prepare_rule(
                f"/{PROP_NAME}"), 'null', 'ruleExecOut'], "STDOUT", ["?"])
            svr_cfg[PROP_NAME] = "#6456 is done"
            with open(server_config_filename, 'w') as f:
                f.write(json.dumps(svr_cfg, sort_keys=True,
                        indent=4, separators=(',', ': ')))

            irodsctl.reload_configuration()
            self.admin.assert_icommand(['irule', '-r', rep_name, TestConfigurationReload.prepare_rule(
                f"/{PROP_NAME}"), 'null', 'ruleExecOut'], "STDOUT", ["#6456 is done"])

    def test_reload_command__issue_6529(self):
        server_config_filename = paths.server_config_path()
        PROP_NAME = "is_issue_6529_done"
        with lib.file_backed_up(server_config_filename):
            # Make the configuration changes
            with open(server_config_filename) as f:
                svr_cfg = json.load(f)
            svr_cfg[PROP_NAME] = "Yes?"
            with open(server_config_filename, 'w') as f:
                f.write(json.dumps(svr_cfg, sort_keys=True,
                        indent=4, separators=(',', ': ')))

            # Invoke irods-grid reload --all
            with session.make_session_for_existing_admin() as admin_session:
                admin_session.environment_file_contents = IrodsConfig().client_environment
                _, out, _ = admin_session.assert_icommand(
                    "irods-grid reload --all", 'STDOUT_MULTILINE')

                # Load the output of the command
                obj = json.loads(out)

                # Find the configuration changes for the host running the tests (local host). All hosts will exhibit at
                # least one configuration change when the reload command is issued with --all because the reCacheSalt
                # is captured each time the configuration is loaded. This being the case, we need to make sure we are
                # only looking at configuration changes for the local host.
                changes = None
                hostname_running_this_test = socket.gethostname()
                for host in obj['hosts']:
                    if hostname_running_this_test == host['hostname']:
                        changes = host['configuration_changes_made']
                        break

                # We expect the changes to be a list with two items. One is the configuration change made above. The
                # other is the reCacheSalt, which is always captured when the configuration is loaded.
                self.assertIsNotNone(changes)
                self.assertEqual(len(changes), 2)

                # Find the object holding the test property.
                change = {}
                for entry in changes:
                    if entry["path"] == f"/{PROP_NAME}":
                        change = entry
                        break

                # Verify that the properties are what we expect.
                self.assertEqual(change["path"], f"/{PROP_NAME}")
                self.assertEqual(change["op"], "add")
                self.assertEqual(change["value"], "Yes?")

            # Also make sure that the actual value is correct, as far as the
            # rule engine is concerned.
            rule = self.prepare_rule(PROP_NAME)
            rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
            self.admin.assert_icommand(
                ["irule", '-r', rep_name, rule, 'null', 'ruleExecOut'], 'STDOUT', ["Yes?"])

        # Restart the server so that the runtime properties introduced by this test
        # do not leak into other tests.
        IrodsController().restart(test_mode=True)

