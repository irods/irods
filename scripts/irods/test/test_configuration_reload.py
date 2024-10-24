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
