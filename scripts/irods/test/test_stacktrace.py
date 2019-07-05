import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from .. import test
from .. import paths
from .. import lib
from ..configuration import IrodsConfig

class Test_Stacktrace(session.make_sessions_mixin([('otherrods', 'rods')], []), unittest.TestCase):

    plugin_name = IrodsConfig().default_rule_engine_plugin

    def setUp(self):
        super(Test_Stacktrace, self).setUp()
        self.admin = self.admin_sessions[0]

    def tearDown(self):
        super(Test_Stacktrace, self).tearDown()

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python' or test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_stacktraces_appear_in_log__issue_4382(self):
        self.admin.assert_icommand_fail("irule 'msiSegFault()' null ruleExecOut")
        self.assertTrue(lib.count_occurrences_of_string_in_log(paths.server_log_path(), 'Dumping stacktrace and exiting') > 0)
        self.assertTrue(lib.count_occurrences_of_string_in_log(paths.server_log_path(), '"stacktrace":') > 0)
        self.assertTrue(lib.count_occurrences_of_string_in_log(paths.server_log_path(), '<0>\\tOffset:') > 0)

