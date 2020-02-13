import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from .. import test
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
        lib.delayAssert(lambda: message_in_log())
        for msg in ['Dumping stacktrace and exiting', '"stacktrace":', '<0>\\tOffset:']:
            lib.delayAssert(
                lambda: lib.log_message_occurrences_greater_than_count(
                    msg=msg,
                    count=0)
