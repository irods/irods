import unittest

import json
import os

from . import session
from ..controller import IrodsController
from .. import lib
from .. import paths

SessionsMixin = session.make_sessions_mixin([('otherrods', 'rods')], [])


class TestConfigurationReload(SessionsMixin, unittest.TestCase):
    def setUp(self):
        super(TestConfigurationReload, self).setUp()
        self.admin = self.admin_sessions[0]
        self.testdir = os.path.join(self.admin.session_collection, 'testdir')

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
            self.admin.assert_icommand(['irule', TestConfigurationReload.prepare_rule(
                f"/{PROP_NAME}"), 'null', 'ruleExecOut'], "STDOUT", ["?"])
            svr_cfg[PROP_NAME] = "#6456 is done"
            with open(server_config_filename, 'w') as f:
                f.write(json.dumps(svr_cfg, sort_keys=True,
                        indent=4, separators=(',', ': ')))

            irodsctl.reload_configuration()
            self.admin.assert_icommand(['irule', TestConfigurationReload.prepare_rule(
                f"/{PROP_NAME}"), 'null', 'ruleExecOut'], "STDOUT", ["#6456 is done"])

    def test_how_long_reloading_takes(self):
        test_count = 100
        server_config_filename = paths.server_config_path()
        with open(server_config_filename) as f:
            svr_config = json.load(f)
        property_name = "Hello"
        with lib.file_backed_up(server_config_filename):
            rule = TestConfigurationReload.prepare_rule(property_name)
            for i in range(test_count):
                svr_config[property_name] = i
                with open(server_config_filename, 'w') as f:
                    f.write(json.dumps(svr_config))

                IrodsController().reload_configuration()
                self.admin.assert_icommand(
                    ['irule', rule, 'null', 'ruleExecOut'], "STDOUT", [str(i)])

    def test_how_long_restarting_takes(self):
        test_count = 100
        server_config_filename = paths.server_config_path()
        with open(server_config_filename) as f:
            svr_config = json.load(f)
        property_name = "Hello"
        with lib.file_backed_up(server_config_filename):
            rule = TestConfigurationReload.prepare_rule(property_name)
            for i in range(test_count):
                svr_config[property_name] = i
                with open(server_config_filename, 'w') as f:
                    f.write(json.dumps(svr_config))

                IrodsController().restart(test_mode=True)
                self.admin.assert_icommand(
                    ['irule', rule, 'null', 'ruleExecOut'], "STDOUT", [str(i)])
