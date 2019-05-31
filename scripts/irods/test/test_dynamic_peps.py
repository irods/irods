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
from .. import paths
from ..configuration import IrodsConfig

class Test_Dynamic_PEPs(session.make_sessions_mixin([('otherrods', 'rods')], []), unittest.TestCase):

    plugin_name = IrodsConfig().default_rule_engine_plugin

    def setUp(self):
        super(Test_Dynamic_PEPs, self).setUp()
        self.admin = self.admin_sessions[0]

    def tearDown(self):
        super(Test_Dynamic_PEPs, self).tearDown()

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python' or test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_if_collInp_t_is_exposed__issue_4370(self):
        config = IrodsConfig()

        with lib.file_backed_up(config.server_config_path):
            core_re_path = os.path.join(config.core_re_directory, 'core.re')

            with lib.file_backed_up(core_re_path):
                prefix = "i4370_PATH => "

                with open(core_re_path, 'a') as core_re:
                    core_re.write('pep_api_coll_create_pre(*a, *b, *coll_input) { writeLine("serverLog", "' + prefix + '" ++ *coll_input.coll_name); }\n')

                coll_path = os.path.join(self.admin.session_collection, "i4370_test_collection")
                self.admin.assert_icommand(['imkdir', coll_path])
                self.assertTrue(lib.count_occurrences_of_string_in_log(paths.server_log_path(), prefix + coll_path) == 1)

