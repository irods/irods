from __future__ import print_function
import os
import sys
import mmap

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from .. import test
from .. import lib
from .. import paths
from ..configuration import IrodsConfig

class Test_Rule_Engine_Plugin_Passthrough(session.make_sessions_mixin([('otherrods', 'rods')], []), unittest.TestCase):

    plugin_name = IrodsConfig().default_rule_engine_plugin

    def setUp(self):
        super(Test_Rule_Engine_Plugin_Passthrough, self).setUp()
        self.admin = self.admin_sessions[0]

    def tearDown(self):
        super(Test_Rule_Engine_Plugin_Passthrough, self).tearDown()

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python' or test.settings.RUN_IN_TOPOLOGY, "Skip for Python REP and Topology Testing")
    def test_repf_continuation_using_passthrough_rep__issues_4147_4148_4179(self):
        config = IrodsConfig()

        with lib.file_backed_up(config.server_config_path):
            pep_name = 'pep_database_open_pre'
            RULE_ENGINE_CONTINUE = 5000000

            # Enable the Passthrough REP (make it the first REP in the list).
            # Configure the Passthrough REP to return 'RULE_ENGINE_CONTINUE' to the REPF.
            # Set the log level to 'trace' for the rule engine and legacy logger categories.
            config.server_config['log_level']['rule_engine'] = 'trace'
            config.server_config['log_level']['legacy'] = 'trace'
            config.server_config['plugin_configuration']['rule_engines'].insert(0, {
                'instance_name': 'irods_rule_engine_plugin-passthrough-instance',
                'plugin_name': 'irods_rule_engine_plugin-passthrough',
                'plugin_specific_configuration': {
                    'return_codes_for_peps': [
                        {
                            'regex': '^' + pep_name + '$',
                            'code': RULE_ENGINE_CONTINUE
                        }
                    ]
                }
            })
            lib.update_json_file_from_dict(config.server_config_path, config.server_config)

            core_re_path = os.path.join(config.core_re_directory, 'core.re')

            with lib.file_backed_up(core_re_path):
                second_msg = 'This should appear after the first message!'

                # Add a new PEP to core.re that will write to the log file.
                # This will help to verify that the Passthrough REP is not blocking anything.
                with open(core_re_path, 'a') as core_re:
                    core_re.write(pep_name + "(*a, *b, *c) { writeLine('serverLog', '" + second_msg + "'); }\n")

                # Trigger the Passthrough REP and the PEP that was just added to core.re.
                self.admin.run_icommand('ils')

                # Verify that the Passthrough REP message appears in the log file before
                # the message produced by the PEP in core.re.
                with open(paths.server_log_path(), 'r') as log_file:
                    mm = mmap.mmap(log_file.fileno(), 0, access=mmap.ACCESS_READ)
                    index = mm.find("Returned '{0}' to REPF.".format(str(RULE_ENGINE_CONTINUE)).encode())
                    self.assertTrue(index != -1)
                    self.assertTrue(mm.find(second_msg.encode(), index) != -1)
                    mm.close()

