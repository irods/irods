from __future__ import print_function
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from .. import test
from ..configuration import IrodsConfig


class Test_Iqmod(session.make_sessions_mixin([('otherrods', 'rods')], []), unittest.TestCase):
    plugin_name = IrodsConfig().default_rule_engine_plugin

    def setUp(self):
        super(Test_Iqmod, self).setUp()
        self.admin = self.admin_sessions[0]

    def tearDown(self):
        super(Test_Iqmod, self).tearDown()

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Skip for PREP')
    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_iqmod_does_not_accept_invalid_priority_levels__issue_2759(self):
        # Schedule a delay rule.
        rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
        delay_rule = 'delay("<INST_NAME>{0}</INST_NAME><EF>1s</EF>") {{ 1 + 1; }}'.format(rep_name)
        self.admin.assert_icommand(['irule', '-r', rep_name, delay_rule, 'null', 'ruleExecOut'])

        try:
            # Get the delay rule's ID.
            delay_rule_id, err, ec = self.admin.run_icommand(['iquest', '%s', 'select RULE_EXEC_ID'])
            self.assertEqual(ec, 0)
            self.assertEqual(len(err), 0)
            self.assertGreater(len(delay_rule_id.strip()), 0)

            # Show that the priority cannot be set to a value outside of the accepted range [0-9].
            self.admin.assert_icommand(['iqmod', delay_rule_id.strip(), 'priority', '-1'], 'STDERR', ['-318000 USER_INPUT_OPTION_ERR'])
            self.admin.assert_icommand(['iqmod', delay_rule_id.strip(), 'priority',  '0'], 'STDERR', ['-130000 SYS_INVALID_INPUT_PARAM'])
            self.admin.assert_icommand(['iqmod', delay_rule_id.strip(), 'priority', '10'], 'STDERR', ['-130000 SYS_INVALID_INPUT_PARAM'])
        finally:
            self.admin.run_icommand(['iqdel', '-a'])

        self.admin.assert_icommand(['iquest', 'select RULE_EXEC_ID'], 'STDOUT', ['CAT_NO_ROWS_FOUND'])

