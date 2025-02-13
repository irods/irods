from __future__ import print_function
import os
import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest

from .. import lib
from .. import paths
from .. import test
from .resource_suite import ResourceBase
from ..configuration import IrodsConfig


class Test_Irule(ResourceBase, unittest.TestCase):
    plugin_name = IrodsConfig().default_rule_engine_plugin
    class_name = 'Test_Irule'

    def setUp(self):
        super(Test_Irule, self).setUp()

    def tearDown(self):
        super(Test_Irule, self).tearDown()


    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER or plugin_name=='irods_rule_engine_plugin-python',
                     'Skip for topology testing from resource server: reads server log')
    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language',
                         'tests cache update - only applicable for irods_rule_language REP')
    def test_irule_printHello_in_serverLog_4189(self):
        svr_log_path = paths.server_log_path()
        initial_log_size = lib.get_file_size_by_path( svr_log_path )
        _, _ ,rc = self.admin.run_icommand(['irule', 'printHello', 'hi', 'null'])
        # With the above invalid parameter ("hi"),  irule should return an error code to OS...
        self.assertNotEqual (rc, 0)
        # and shouldn't run the requested operation:  i.e., writing to the log.
        lib.delayAssert(
            lambda: lib.log_message_occurrences_equals_count(
                msg='\nHello',
                count=0,
                start_index=initial_log_size))

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language',
                         'tests cache update - only applicable for irods_rule_language REP')
    def test_irule_printVariables_on_stdout_4189(self):

        stdout, stderr, rc =  self.admin.run_icommand(['irule', 'writeLine("stdout","[*a][*b]")', '*a=1%*b=2', 'ruleExecOut'])
        self.assertEqual (rc, 0)
        self.assertIn ("[1][2]", stdout)

        stdout, stderr, rc =  self.admin.run_icommand(['irule', 'writeLine("stdout","[*a]")', '*a=1%badInput', 'ruleExecOut'])

        self.assertNotIn( "[1]", stdout )
        self.assertIn( "badInput format error", stderr )
        self.assertNotEqual( rc, 0 )

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for irods_rule_language REP')
    def test_irule_does_not_crash_on_bad_rule_file__issue_7740(self):
        bad_rule = '''
        test_irule_does_not_crash_on_bad_rule_file__issue_7740 {
            writeLine("Did I do this right?");
        }
        OUTPUT
        '''
        path_to_file = os.path.join(self.admin.local_session_dir, 'issue_7740.r')

        with open(path_to_file, 'w') as f:
            f.write(bad_rule)

        self.admin.assert_icommand_fail(['irule', '-F', path_to_file, '-r', 'irods_rule_engine_plugin-irods_rule_language-instance'], desired_rc=2)

    # TODO (#4094): Write a similar test case for PREP
    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for irods_rule_language REP')
    def test_irule_exits_nonzero_on_write_writeline_failure__issue_8095(self):
        _, err, rc = self.admin.run_icommand(['irule', '-r', 'irods_rule_engine_plugin-irods_rule_language-instance', 'writeLine("/bad/path", "nopes")', 'null', 'null'])
        self.assertIn('-310000 USER_FILE_DOES_NOT_EXIST', err)
        self.assertNotEqual(rc, 0)

    # TODO (#4094): Write a similar test case for PREP
    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for irods_rule_language REP')
    def test_irule_exits_nonzero_on_write_writestring_failure__issue_8095(self):
        _, err, rc = self.admin.run_icommand(['irule', '-r', 'irods_rule_engine_plugin-irods_rule_language-instance', 'writeString("/bad/path", "nopes")', 'null', 'null'])
        self.assertIn('-310000 USER_FILE_DOES_NOT_EXIST', err)
        self.assertNotEqual(rc, 0)

    # TODO (#4094): Write a similar test case for PREP
    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for irods_rule_language REP')
    def test_irule_exits_nonzero_on_write_writekeyvalpairs_failure__issue_8095(self):
        _, err, rc = self.admin.run_icommand(['irule', '-r', 'irods_rule_engine_plugin-irods_rule_language-instance', 'msiString2KeyValPair("key=value",*Kvp); writeKeyValPairs("/bad/path", *Kvp, ":")', 'null', 'null'])
        self.assertIn('-310000 USER_FILE_DOES_NOT_EXIST', err)
        self.assertNotEqual(rc, 0)

    # TODO (#4094): Write a similar test case for PREP
    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for irods_rule_language REP')
    def test_irule_exits_nonzero_on_write_writebytesbuf_failure__issue_8095(self):
        _, err, rc = self.admin.run_icommand(['irule', '-r', 'irods_rule_engine_plugin-irods_rule_language-instance', 'msiStrToBytesBuf("potato",*Buf); writeBytesBuf("/bad/path", *Buf);', 'null', 'null'])
        self.assertIn('-310000 USER_FILE_DOES_NOT_EXIST', err)
        self.assertNotEqual(rc, 0)
