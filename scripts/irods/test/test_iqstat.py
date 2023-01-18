from __future__ import print_function
import datetime
import os
import re
import sys
import json

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from ..configuration import IrodsConfig
from ..test.command import assert_command

class Test_Iqstat(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):

    def setUp(self):
        super(Test_Iqstat, self).setUp()
        self.rule_ids = ''
        self.admin = self.admin_sessions[0]
        self.user = self.user_sessions[0]

    def tearDown(self):
        for rule_id in self.rule_ids.split():
            self.admin.run_icommand('iqdel {}'.format(rule_id))
        super(Test_Iqstat, self).tearDown()

    def spawn_delay_rule(self,sess):
        dtnow = datetime.datetime.now()
        epoch_with_micros = '({0:%s})({0:%f})'.format(dtnow)
        sess.assert_icommand('''irule -r irods_rule_engine_plugin-irods_rule_language-instance '''
                             '''"delay('<EF>1m</EF><PLUSET>1s</PLUSET>'){{'{}'}}" null null'''
                             ''.format(epoch_with_micros), 'EMPTY', desired_rc = 0)
        rc, rule_id, err = sess.assert_icommand('''iquest "%s" "select RULE_EXEC_ID where RULE_EXEC_NAME like '%{}%'"'''
                                                    ''.format(epoch_with_micros), 'STDOUT', desired_rc = 0)
        self.rule_ids += rule_id
        return rule_id.strip()

    def test_iqstat_with_all_option_and_id__issue_6740(self):

        rule_id = self.spawn_delay_rule(self.user)

        # Asserting long format.  (iqstat traditionally prints in long format (-l) if invoked with an ID for match.)
        self.admin.assert_icommand('iqstat -a {}'.format(rule_id), 'STDOUT_SINGLELINE', r'id:\s\s*{}'.format(rule_id), use_regex = True)

    def test_iqstat_with_all_option_and_no_id__issue_6740(self):

        rule_ids = [ self.spawn_delay_rule(self.user),
                     self.spawn_delay_rule(self.admin) ]

        # Assert brief format
        _, out, err = self.admin.assert_icommand('iqstat -a', 'STDOUT_SINGLELINE', r'id\s\s*name', use_regex = True)
        for rule_id in rule_ids:
            self.assertRegexpMatches(out, re.compile('^' + rule_id + r'\s\s*', re.MULTILINE))

        # Assert long format
        _, out, err = self.admin.assert_icommand('iqstat -al', 'STDOUT_SINGLELINE', '^\s*---*\s*$', use_regex = True)
        self.admin.assert_icommand('iqstat -al', 'STDOUT_SINGLELINE', '^\s*--*\s*$', use_regex = True)
        output_sections = [x.strip() for x in re.compile('^\s*--*\s*$', re.MULTILINE).split(out)]
        # In the long-format sections, search for /id: {id}/m as a match within each; exactly two such sections should match the existing rule id's
        self.assertEqual(2, len([y for y in output_sections for id in rule_ids
                                 if re.compile(r'^id:\s*{}\s*$'.format(id), re.MULTILINE).search(y)]))

