from __future__ import print_function
import sys
if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import os
import time

from . import resource_suite
from .. import test
from . import settings
from .. import paths
from .. import lib

class Test_Native_Rule_Engine_Plugin(resource_suite.ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_Native_Rule_Engine_Plugin, self).setUp()

    def tearDown(self):
        super(Test_Native_Rule_Engine_Plugin, self).tearDown()

    def helper_test_pep(self, rules_to_prepend, icommand, strings_to_check_for=['THIS IS AN OUT VARIABLE'], number_of_strings_to_look_for=1):
        corefile = paths.core_re_directory() + "/core.re"

        with lib.file_backed_up(corefile):
            time.sleep(1)  # remove once file hash fix is commited #2279
            lib.prepend_string_to_file(rules_to_prepend, corefile)
            time.sleep(1)  # remove once file hash fix is commited #2279

            initial_size_of_server_log = lib.get_file_size_by_path(paths.server_log_path())

            self.admin.run_icommand(icommand)

        for s in strings_to_check_for:
            count = lib.count_occurrences_of_string_in_log(paths.server_log_path(), s, start_index=initial_size_of_server_log)
            assert number_of_strings_to_look_for == count, 'Found {0} instead of {1} occurrences of {2}'.format(count, number_of_strings_to_look_for, s)

    def test_network_pep(self):
        self.helper_test_pep("""
pep_network_agent_start_pre(*INST,*CTX,*OUT) {
*OUT = "THIS IS AN OUT VARIABLE"
}
pep_network_agent_start_post(*INST,*CTX,*OUT){
writeLine( 'serverLog', '*OUT')
}
""", "iput -f --metadata ATTR;VALUE;UNIT "+self.testfile)

    def test_auth_pep(self):
        self.helper_test_pep("""
pep_resource_resolve_hierarchy_pre(*INSTANCE,*CONTEXT,*OUT,*OPERATION,*HOST,*PARSER,*VOTE){
*OUT = "THIS IS AN OUT VARIABLE"
}
pep_resource_resolve_hierarchy_post(*INSTANCE,*CONTEXT,*OUT,*OPERATION,*HOST,*PARSER,*VOTE){
writeLine( 'serverLog', '*OUT')
}
""", "iput -f --metadata ATTR;VALUE;UNIT "+self.testfile, ['THIS IS AN OUT VARIABLE'], 2)

    def test_out_variable(self):
        self.helper_test_pep("""
pep_resource_resolve_hierarchy_pre(*INSTANCE,*CONTEXT,*OUT,*OPERATION,*HOST,*PARSER,*VOTE){
*OUT = "THIS IS AN OUT VARIABLE"
}
pep_resource_resolve_hierarchy_post(*INSTANCE,*CONTEXT,*OUT,*OPERATION,*HOST,*PARSER,*VOTE){
writeLine( 'serverLog', '*OUT')
}
""", "iput -f --metadata ATTR;VALUE;UNIT "+self.testfile, ['THIS IS AN OUT VARIABLE'], 2)

    def test_re_serialization(self):
        self.helper_test_pep("""
pep_resource_resolve_hierarchy_pre(*INSTANCE,*CONTEXT,*OUT,*OPERATION,*HOST,*PARSER,*VOTE){
writeLine("serverLog", "pep_resource_resolve_hierarchy_pre - [*INSTANCE] [*CONTEXT] [*OUT] [*OPERATION] [*HOST] [*PARSER] [*VOTE]");
}
""", "iput -f --metadata ATTR;VALUE;UNIT "+self.testfile,
            ['user_auth_info_auth_flag=5', 'user_rods_zone=tempZone', 'user_user_name=otherrods', 'metadataIncluded=ATTR;VALUE;UNIT'], 2)

    def test_api_plugin(self):
        self.helper_test_pep("""
pep_api_hello_world_pre(*INST, *OUT, *HELLO_IN, *HELLO_OUT) {
    writeLine("serverLog", "pep_api_hello_world_pre - *INST *OUT *HELLO_IN, *HELLO_OUT");
}
pep_api_hello_world_post(*INST, *OUT, *HELLO_IN, *HELLO_OUT) {
    writeLine("serverLog", "pep_api_hello_world_post - *INST *OUT *HELLO_IN, *HELLO_OUT");
}
""", "iapitest",
            ['pep_api_hello_world_pre - api_instance auth_scheme=native', 'that=hello, world.++++this=42, null_value', 'HELLO WORLD', 'pep_api_hello_world_post - api_instance auth_scheme=native', 'that=hello, world.++++this=42, that=hello, world.++++this=42++++value=128'])

    def test_rule_engine_2242(self):
        rule_file1 = "rule1_2242.r"
        rule_string1 = """
test {
    $userNameClient = \"foobar\";
}

INPUT *A=\"status\"
OUTPUT ruleExecOut
}
"""
        with open(rule_file1, 'wt') as f:
            print(rule_string1, file=f, end='')

        out, _, _ = self.admin.run_icommand("irule -F " + rule_file1)
        assert 'Update session variable $userNameClient not allowed' in out

        rule_file2 = "rule2_2242.r"
        rule_string2 = """
test {
    $status = \"1\";
}

acPreProcForWriteSessionVariable(*x) {
    writeLine(\"stdout\", \"bwahahaha\");
    succeed;
}

INPUT *A=\"status\"
OUTPUT ruleExecOut
"""
        with open(rule_file2, 'wt') as f:
            print(rule_string2, file=f, end='')

        self.admin.assert_icommand("irule -F " + rule_file2, "EMPTY")

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: reads re server log')
    def test_rule_engine_2309(self):
        corefile = paths.core_re_directory() + "/core.re"
        coredvm = paths.core_re_directory() + "/core.dvm"
        with lib.file_backed_up(coredvm):
            lib.prepend_string_to_file('oprType||rei->doinp->oprType\n', coredvm)
            with lib.file_backed_up(corefile):
                initial_size_of_server_log = lib.get_file_size_by_path(paths.server_log_path())
                rules_to_prepend = '''
 acSetNumThreads() {
     writeLine("serverLog","test_rule_engine_2309: put: acSetNumThreads oprType [$oprType]");
 }
 '''
                time.sleep(1)  # remove once file hash fix is commited #2279
                lib.prepend_string_to_file(rules_to_prepend, corefile)
                time.sleep(1)  # remove once file hash fix is commited #2279
                trigger_file = 'file_to_trigger_acSetNumThreads'
                lib.make_file(trigger_file, 4 * pow(10, 7))
                self.admin.assert_icommand('iput {0}'.format(trigger_file))
                assert 1 == lib.count_occurrences_of_string_in_log(
                    paths.server_log_path(), 'writeLine: inString = test_rule_engine_2309: put: acSetNumThreads oprType [1]', start_index=initial_size_of_server_log)
                assert 0 == lib.count_occurrences_of_string_in_log(paths.server_log_path(), 'RE_UNABLE_TO_READ_SESSION_VAR', start_index=initial_size_of_server_log)
                os.unlink(trigger_file)

            with lib.file_backed_up(corefile):
                initial_size_of_server_log = lib.get_file_size_by_path(paths.server_log_path())
                rules_to_prepend = '''
acSetNumThreads() {
    writeLine("serverLog","test_rule_engine_2309: get: acSetNumThreads oprType [$oprType]");
}
'''
                time.sleep(1)  # remove once file hash fix is commited #2279
                lib.prepend_string_to_file(rules_to_prepend, corefile)
                time.sleep(1)  # remove once file hash fix is commited #2279
                self.admin.assert_icommand('iget {0}'.format(trigger_file), use_unsafe_shell=True)
                assert 1 == lib.count_occurrences_of_string_in_log(
                    paths.server_log_path(), 'writeLine: inString = test_rule_engine_2309: get: acSetNumThreads oprType [2]', start_index=initial_size_of_server_log)
                assert 0 == lib.count_occurrences_of_string_in_log(paths.server_log_path(), 'RE_UNABLE_TO_READ_SESSION_VAR', start_index=initial_size_of_server_log)
                os.unlink(trigger_file)
