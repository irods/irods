from __future__ import print_function
import sys
if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import inspect
import json
import os
import time
import tempfile
from textwrap import dedent

from . import resource_suite
from .. import test
from . import settings
from .. import paths
from .. import lib
from ..configuration import IrodsConfig
from ..core_file import temporary_core_file
from .rule_texts_for_tests import rule_texts
from ..controller import IrodsController

def exec_icat_command(command):
    import paramiko

    hostname = 'icat.example.org'
    port = 22
    username = 'irods'
    password = ''

    print('Executing command on '+hostname+': ' + command)

    s = paramiko.SSHClient()
    s.load_system_host_keys()
    s.connect(hostname, port, username, password)
    (stdin, stdout, stderr) = s.exec_command(command)

    ol = []
    for l in stdout.readlines():
        ol.append(l)

    el = []
    for l in stderr.readlines():
        el.append(l)

    s.close()

    return ol, el


class Test_Native_Rule_Engine_Plugin(resource_suite.ResourceBase, unittest.TestCase):
    plugin_name = IrodsConfig().default_rule_engine_plugin
    class_name = 'Test_Native_Rule_Engine_Plugin'

    def setUp(self):
        super(Test_Native_Rule_Engine_Plugin, self).setUp()

    def tearDown(self):
        super(Test_Native_Rule_Engine_Plugin, self).tearDown()


    def helper_test_pep(self, rules_to_add, icommand, strings_to_check_for=['THIS IS AN OUT VARIABLE'], number_of_strings_to_look_for=1):
        with temporary_core_file() as core:
            time.sleep(1)  # remove once file hash fix is committed #2279
            core.add_rule(rules_to_add)
            time.sleep(1)  # remove once file hash fix is committed #2279
            initial_size_of_server_log = lib.get_file_size_by_path(paths.server_log_path())
            self.admin.run_icommand(icommand)

        def check_string_count_in_log_section(string, n_occurrences):
            lib.delayAssert(
                lambda: lib.log_message_occurrences_equals_count(
                    msg=string,
                    count=n_occurrences,
                    start_index=initial_size_of_server_log))

        if  isinstance(strings_to_check_for, dict):
            for s,n  in  strings_to_check_for.items():
                check_string_count_in_log_section (s,n)
        else:
            for s in strings_to_check_for:
                check_string_count_in_log_section (s,number_of_strings_to_look_for)


    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python' or test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Native only test when not in a topology')
    def test_peps_for_parallel_mode_transfers__4404(self):

        (fd, largefile) = tempfile.mkstemp()
        os.write(fd,"123456789abcdef\n"*(6*1024**2))
        os.close(fd)
        temp_base = os.path.basename(largefile)
        temp_dir  = os.path.dirname(largefile)
        extrafile = ""

        try:
            get_peps = dedent("""\
                pep_api_data_obj_get_pre (*INSTANCE_NAME, *COMM, *DATAOBJINP, *BUFFER, *PORTAL_OPR_OUT)
                {
                    writeLine("serverLog", "data-obj-get-pre")
                }
                pep_api_data_obj_get_post (*INSTANCE_NAME, *COMM, *DATAOBJINP, *BUFFER, *PORTAL_OPR_OUT)
                {
                    writeLine("serverLog", "data-obj-get-post")
                }
                pep_api_data_obj_get_except (*INSTANCE_NAME, *COMM, *DATAOBJINP, *BUFFER, *PORTAL_OPR_OUT)
                {
                    writeLine("serverLog", "data-obj-get-except")
                }
                """)

            put_peps = dedent("""\
                pep_api_data_obj_put_pre (*INSTANCE_NAME, *COMM, *DATAOBJINP, *BUFFER, *PORTAL_OPR_OUT)
                {
                    writeLine("serverLog", "data-obj-put-pre")
                }
                pep_api_data_obj_put_post (*INSTANCE_NAME, *COMM, *DATAOBJINP, *BUFFER, *PORTAL_OPR_OUT)
                {
                    writeLine("serverLog", "data-obj-put-post")
                }
                pep_api_data_obj_put_except (*INSTANCE_NAME, *COMM, *DATAOBJINP, *BUFFER, *PORTAL_OPR_OUT)
                {
                    writeLine("serverLog", "data-obj-put-except")
                }
                """)

            self.helper_test_pep( put_peps, "iput -f {}".format(largefile),
                             { "data-obj-put-pre": 1, "data-obj-put-post": 1, "data-obj-put-except": 0 } )

            self.helper_test_pep( get_peps, "iget -f {} {}".format(temp_base,temp_dir),
                             { "data-obj-get-pre": 1, "data-obj-get-post": 1, "data-obj-get-except": 0 } )

            self.helper_test_pep( put_peps, "iput {}".format(largefile),
                             { "data-obj-put-pre": 1, "data-obj-put-post": 0, "data-obj-put-except": 1 } )

            self.admin.run_icommand('ichmod null {} {}'.format(self.admin.username,temp_base))

            extrafile = os.path.join(temp_dir, temp_base + ".x")

            self.helper_test_pep( get_peps, "iget {} {}".format(temp_base,extrafile),
                             { "data-obj-get-pre": 1, "data-obj-get-post": 0, "data-obj-get-except": 1 } )

        finally:

            self.admin.run_icommand('ichmod own {} {}'.format(self.admin.username,temp_base))
            os.unlink(largefile)
            if extrafile and os.path.isfile(extrafile):
                os.unlink(extrafile)

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python' or test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Native only test when not in a topology')
    def test_dynamic_policy_enforcement_point_exception_for_plugins__4128(self):
       path = self.admin.get_vault_session_path('demoResc')

       self.admin.run_icommand('iput -f '+self.testfile+' good_file')
       self.admin.run_icommand('iput -f '+self.testfile+' bad_file')
       os.unlink(os.path.join(path, 'bad_file'))
       pre_pep_fail = """
           pep_resource_open_pre(*INST, *CTX, *OUT) {
               failmsg(-1, "PRE PEP FAIL")
           }
           pep_resource_open_except(*INST, *CTX, *OUT) {
               writeLine("serverLog", "EXCEPT FOR PRE PEP FAIL")
           }
       """
       self.helper_test_pep(pre_pep_fail, 'iget -f '+self.testfile, ['EXCEPT FOR PRE PEP FAIL'])

       op_fail = """
           pep_resource_open_except(*INST, *CTX, *OUT) {
               writeLine("serverLog", "EXCEPT FOR OPERATION FAIL")
           }
       """
       self.helper_test_pep(op_fail, 'iget -f bad_file', ['EXCEPT FOR OPERATION FAIL'])

       post_pep_fail = """
           pep_resource_open_post(*INST, *CTX, *OUT) {
               failmsg(-1, "POST PEP FAIL")
           }
           pep_resource_open_except(*INST, *CTX, *OUT) {
               writeLine("serverLog", "EXCEPT FOR POST PEP FAIL")
           }
       """
       self.helper_test_pep(post_pep_fail, 'iget -f '+self.testfile, ['EXCEPT FOR POST PEP FAIL'])

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python' or test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Native only test when not in a topology')
    def test_dynamic_policy_enforcement_point_exception_for_apis__4128(self):
       path = self.admin.get_vault_session_path('demoResc')

       self.admin.run_icommand('iput -f '+self.testfile+' good_file')
       self.admin.run_icommand('iput -f '+self.testfile+' bad_file')
       os.unlink(os.path.join(path, 'bad_file'))
       pre_pep_fail = """
           pep_api_data_obj_get_pre(*INST, *COMM, *INP, *PORT, *BUF) {
               failmsg(-1, "PRE PEP FAIL")
           }
           pep_api_data_obj_get_except(*INST, *COMM, *INP, *PORT, *BUF) {
               writeLine("serverLog", "EXCEPT FOR PRE PEP FAIL")
           }
       """
       self.helper_test_pep(pre_pep_fail, 'iget -f '+self.testfile, ['EXCEPT FOR PRE PEP FAIL'])

       op_fail = """
           pep_api_data_obj_get_except(*INST, *COMM, *INP, *PORT, *BUF) {
               writeLine("serverLog", "EXCEPT FOR OPERATION FAIL")
           }
       """
       self.helper_test_pep(op_fail, 'iget -f bad_file', ['EXCEPT FOR OPERATION FAIL'])

       post_pep_fail = """
           pep_api_data_obj_get_post(*INST, *COMM, *INP, *PORT, *BUF) {
               failmsg(-1, "POST PEP FAIL")
           }
           pep_api_data_obj_get_except(*INST, *COMM, *INP, *PORT, *BUF) {
               writeLine("serverLog", "EXCEPT FOR POST PEP FAIL")
           }
       """
       self.helper_test_pep(post_pep_fail, 'iget -f '+self.testfile, ['EXCEPT FOR POST PEP FAIL'])

    @unittest.skipIf(plugin_name != 'irods_rule_engine_plugin-python' or not test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Python only test from resource server in a topology')
    def test_remote_rule_execution(self):
        # =-=-=-=-=-=-=-=-
        # stat the icat log to get its current size
        log_stat_cmd = 'stat -c%s ' + paths.server_log_path()
        out, err = exec_icat_command(log_stat_cmd)

        if len(err) > 0:
            for l in err:
                print('stderr: ' + l)
            return

        if len(out) <= 0:
            print('ERROR: LOG SIZE EMPTY')
            assert(0)

        print('log size: ' + out[0])
        initial_log_size = int(out[0])

        # =-=-=-=-=-=-=-=-
        # run the remote rule to write to the log
        rule_code = rule_texts[self.plugin_name][self.class_name][inspect.currentframe().f_code.co_name]
        print('Executing code:\n'+rule_code)
        rule_file = 'test_remote_rule_execution.r'
        with open(rule_file, 'wt') as f:
            print(rule_code, file=f, end='')
        out, _, _ = self.admin.run_icommand("irule -F " + rule_file)

        # =-=-=-=-=-=-=-=-
        # search for the position of the token in the remote log
        token = 'XXXX - PREP REMOTE EXEC TEST'

        log_search_cmd = 'grep -ob "' + token + '" ' + paths.server_log_path()
        out, err = exec_icat_command(log_search_cmd)
        if len(err) > 0:
            for l in err:
                print('stderr: ' + l)
            assert(0)

        if len(out) <= 0:
            print('ERROR: TOKEN LOCATION EMPTY')
            assert(0)

        loc_str = out[-1].split(':')[0]
        print('token location: ' + loc_str)

        token_location = int(loc_str)

        # =-=-=-=-=-=-=-=-
        # compare initial log size to the location of the token in the file
        # the token should be located in the file some place after the initial
        # size of the file
        if initial_log_size > token_location:
            assert(0)
        else:
            assert(1)

    def test_network_pep(self):
        irodsctl = IrodsController()
        server_config_filename = paths.server_config_path()

        # load server_config.json to inject a new rule base
        with open(server_config_filename) as f:
            svr_cfg = json.load(f)

        # Occasionally, the expected message is printed twice due to the rule engine waking up, causing the test to fail
        # Make irodsReServer sleep for a long time so it won't wake up while the test is running
        svr_cfg['advanced_settings']['rule_engine_server_sleep_time_in_seconds'] = 1000

        # dump to a string to repave the existing server_config.json
        new_server_config = json.dumps(svr_cfg, sort_keys=True, indent=4, separators=(',', ': '))

        with lib.file_backed_up(server_config_filename):
            # repave the existing server_config.json
            with open(server_config_filename, 'w') as f:
                f.write(new_server_config)

            # Bounce server to apply setting
            irodsctl.restart()

            # Actually run the test
            self.helper_test_pep(rule_texts[self.plugin_name][self.class_name][inspect.currentframe().f_code.co_name], "iput -f --metadata ATTR;VALUE;UNIT "+self.testfile)

        # Bounce server to get back original settings
        irodsctl.restart()

    def test_auth_pep(self):
        self.helper_test_pep(rule_texts[self.plugin_name][self.class_name][inspect.currentframe().f_code.co_name], "iput -f --metadata ATTR;VALUE;UNIT "+self.testfile, ['THIS IS AN OUT VARIABLE'], 2)

    def test_out_variable(self):
        self.helper_test_pep(rule_texts[self.plugin_name][self.class_name][inspect.currentframe().f_code.co_name], "iput -f --metadata ATTR;VALUE;UNIT "+self.testfile, ['THIS IS AN OUT VARIABLE'], 2)

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Python rule engine does not use re_serialization except for api plugins')
    def test_re_serialization(self):
        self.helper_test_pep(rule_texts[self.plugin_name][self.class_name][inspect.currentframe().f_code.co_name], "iput -f --metadata ATTR;VALUE;UNIT "+self.testfile,
            ['file_size=33', 'logical_path=/tempZone/home/otherrods', 'metadataIncluded'], 2)

    def test_api_plugin(self):
        self.helper_test_pep(rule_texts[self.plugin_name][self.class_name][inspect.currentframe().f_code.co_name], "iapitest", ['pep_api_hello_world_pre -', ', null_value', 'HELLO WORLD', 'pep_api_hello_world_post -', 'value=128'])

    def test_out_string(self):
        self.helper_test_pep(rule_texts[self.plugin_name][self.class_name][inspect.currentframe().f_code.co_name], "iput -f "+self.testfile,
            ['this_is_the_out_string'])

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Python rule engine has no mechanism to update session vars')
    def test_rule_engine_2242(self):
        rule_file1 = "rule1_2242.r"
        rule_string1 = rule_texts[self.plugin_name][self.class_name]['test_rule_engine_2242_1']
        with open(rule_file1, 'wt') as f:
            print(rule_string1, file=f, end='')

        out, _, _ = self.admin.run_icommand("irule -r irods_rule_engine_plugin-irods_rule_language-instance -F " + rule_file1)
        assert 'Update session variable $userNameClient not allowed' in out

        rule_file2 = "rule2_2242.r"
        rule_string2 = rule_texts[self.plugin_name][self.class_name]['test_rule_engine_2242_2']

        with open(rule_file2, 'wt') as f:
            print(rule_string2, file=f, end='')

        self.admin.assert_icommand("irule -r irods_rule_engine_plugin-irods_rule_language-instance -F " + rule_file2, "EMPTY")

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: reads re server log')
    def test_rule_engine_2309(self):
        coredvm = paths.core_re_directory() + "/core.dvm"
        with lib.file_backed_up(coredvm):
            lib.prepend_string_to_file('oprType||rei->doinp->oprType\n', coredvm)
            with temporary_core_file() as core:
                time.sleep(1)  # remove once file hash fix is committed #2279
                core.add_rule(rule_texts[self.plugin_name][self.class_name][inspect.currentframe().f_code.co_name + '_1'])
                time.sleep(1)  # remove once file hash fix is committed #2279

                trigger_file = 'file_to_trigger_acSetNumThreads'
                lib.make_file(trigger_file, 4 * pow(10, 7))

                initial_size_of_server_log = lib.get_file_size_by_path(paths.server_log_path())
                self.admin.assert_icommand('iput {0}'.format(trigger_file))
                lib.delayAssert(
                    lambda: lib.log_message_occurrences_equals_count(
                        msg='writeLine: inString = test_rule_engine_2309: put: acSetNumThreads oprType [1]',
                        start_index=initial_size_of_server_log))
                lib.delayAssert(
                    lambda: lib.log_message_occurrences_equals_count(
                        msg='RE_UNABLE_TO_READ_SESSION_VAR',
                        count=0,
                        start_index=initial_size_of_server_log))
                os.unlink(trigger_file)

            with temporary_core_file() as core:
                time.sleep(1)  # remove once file hash fix is committed #2279
                core.add_rule(rule_texts[self.plugin_name][self.class_name][inspect.currentframe().f_code.co_name + '_2'])
                time.sleep(1)  # remove once file hash fix is committed #2279

                initial_size_of_server_log = lib.get_file_size_by_path(paths.server_log_path())
                self.admin.assert_icommand('iget {0}'.format(trigger_file), use_unsafe_shell=True)
                lib.delayAssert(
                    lambda: lib.log_message_occurrences_equals_count(
                        msg='writeLine: inString = test_rule_engine_2309: get: acSetNumThreads oprType [2]',
                        start_index=initial_size_of_server_log))
                lib.delayAssert(
                    lambda: lib.log_message_occurrences_equals_count(
                        msg='RE_UNABLE_TO_READ_SESSION_VAR',
                        count = 0,
                        start_index=initial_size_of_server_log))
                os.unlink(trigger_file)

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for irods_rule_language REP')
    def test_SYS_NOT_SUPPORTED__4174(self):
        rule_file = 'test_SYS_NOT_SUPPORTED__4174.r'
        rule_string = '''
test_SYS_NOT_SUPPORTED__4174_rule {
    fail(SYS_NOT_SUPPORTED);
}

INPUT null
OUTPUT ruleExecOut
'''
        with open(rule_file, 'w') as f:
            f.write(rule_string)

        self.admin.assert_icommand('irule -r irods_rule_engine_plugin-irods_rule_language-instance -F ' + rule_file, 'STDERR_SINGLELINE', 'SYS_NOT_SUPPORTED')
        os.unlink(rule_file)

    max_literal_strlen = 1021 # MAX_TOKEN_TEXT_LEN - 2

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'rule language only: irods#4311')
    def test_string_literal__4311(self):
        rule_text = '''
main {
   *b=".%s"
   msiStrlen(*b,*L)
   writeLine("stdout",*L)
}
INPUT null
OUTPUT ruleExecOut
''' % ('a'*self.max_literal_strlen,)

        rule_file = 'test_string_literal__4311.r'
        with open(rule_file, 'w') as f:
            f.write(rule_text)
        rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
        self.admin.assert_icommand(['irule', '-r', rep_name, '-F', rule_file], 'STDERR', ['ERROR'], desired_rc = 4)

        rule_file_2 = 'test_string_literal__4311_noerr.r'
        with open(rule_file_2, 'w') as f:
            f.write(rule_text.replace('".','"'))
        self.admin.assert_icommand(['irule', '-r', rep_name, '-F', rule_file_2], 'STDOUT_SINGLELINE', str(self.max_literal_strlen))

        os.remove(rule_file)
        os.remove(rule_file_2)

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'rule language only: irods#4311')
    def test_string_input__4311(self):
        rule_text = '''
main {
  msiStrlen(*a,*L)
  writeLine("stdout",*L)
}
INPUT *a=".%s"
OUTPUT ruleExecOut
''' % ('a'*self.max_literal_strlen,)

        rule_file = 'test_string_input__4311.r'
        with open(rule_file, 'w') as f:
            f.write(rule_text)
        rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
        self.admin.assert_icommand(['irule', '-r', rep_name, '-F', rule_file], 'STDERR', ["ERROR"], desired_rc = 4)

        rule_file_2 = 'test_string_input__4311_noerr.r'
        with open(rule_file_2, 'w') as f:
            f.write(rule_text.replace('".','"'))
        self.admin.assert_icommand(['irule', '-r', rep_name, '-F', rule_file_2], 'STDOUT_SINGLELINE', str(self.max_literal_strlen))

        os.remove(rule_file)
        os.remove(rule_file_2)

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'rule language only')
    def test_msiSegFault(self):
        rule_text = rule_texts[self.plugin_name][self.class_name]['test_msiSegFault']
        rule_file = 'test_msiSegFault.r'

        with open(rule_file, 'w') as f:
            f.write(rule_text)

        try:
            rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'

            # Should get SYS_INTERNAL_ERR because it's a segmentation fault
            self.admin.assert_icommand(['irule', '-r', rep_name, '-F', rule_file], 'STDERR', 'SYS_INTERNAL_ERR')

            # Should get CAT_INSUFFICIENT_PRIVILEGE_LEVEL in the log because this is for admin users only
            self.user0.assert_icommand(['irule', '-r', rep_name, '-F', rule_file], 'STDERR', 'CAT_INSUFFICIENT_PRIVILEGE_LEVEL')

        finally:
            os.unlink(rule_file)
