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
import textwrap

from . import resource_suite
from .. import test
from . import settings
from .. import paths
from .. import lib
from ..configuration import IrodsConfig
from ..core_file import temporary_core_file
from ..controller import IrodsController

def exec_icat_command(command):
    import paramiko

    hostname = test.settings.ICAT_HOSTNAME
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
        self.resc_name = 'Test_Native_Rule_Engine_Plugin_Resource'

    def tearDown(self):
        super(Test_Native_Rule_Engine_Plugin, self).tearDown()

    def reset_resource(self):
        self.admin.run_icommand(['iadmin', 'rmresc', self.resc_name])
        self.admin.assert_icommand(['iadmin', 'rum'])
        self.admin.assert_icommand(['iadmin', 'mkresc', self.resc_name, 'passthru'], 'STDOUT', self.resc_name)

    def helper_test_pep_with_metadata(self, rules_to_add, icommand, attr, values_to_check_for=None):
        if not isinstance(values_to_check_for, dict):
            raise TypeError('values_to_check_for must be a dict')

        with temporary_core_file() as core:
            core.add_rule(rules_to_add)
            self.admin.run_icommand(icommand)

        def assert_presence_of_attr_value_avu(attr, value, assert_true=True):
            try:
                lib.delayAssert(
                    lambda: lib.metadata_attr_with_value_exists(
                        self.admin, attr, value),
                    interval=1,
                    maxrep=10
                )
                if not assert_true:
                    raise AssertionError('attr {0} value {1} found when not expected'.format(attr, value))
            except AssertionError:
                print('failed to find attr {0} value {1}'.format(attr, value))
                if assert_true:
                    raise AssertionError('attr {0} value {1} not found when expected'.format(attr, value))

        for value, assert_true in values_to_check_for.items():
            assert_presence_of_attr_value_avu(attr, value, assert_true)

    def helper_test_pep(self, rules_to_add, icommand, strings_to_check_for=['THIS IS AN OUT VARIABLE'], number_of_strings_to_look_for=1):
        with temporary_core_file() as core:
            core.add_rule(rules_to_add)
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
    def test_failing_on_code_5043 (self):
        pep_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                fail_on_code(*name) {
                    fail( error(*name) )
                }
            ''')
        }

        rule_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                main {
                  *Name = "SYS_NOT_SUPPORTED"
                  writeLine("stdout", errorcode( fail_on_code(*Name) ) == error( *Name  ))
                }
                OUTPUT ruleExecOut
            ''')
        }

        with temporary_core_file() as core:
            core.add_rule(pep_map[self.plugin_name])
            with tempfile.NamedTemporaryFile(mode='w+t', suffix='.r') as rule_file:
                print(rule_map[self.plugin_name], file = rule_file)
                rule_file.flush()
                self.admin.assert_icommand(['irule','-r', self.plugin_name + '-instance','-F',rule_file.name],
                                           'STDOUT_SINGLELINE', ["true"])


    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python' or test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Native only test when not in a topology')
    def test_error_smsi_5043 (self):
        pep_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                validate_error_and_state_smsi(*name,*error_int) {
                    *msg = "[general_failure]"
                    *error_int = 0
                    *status = errorcode( *error_i = error(*name) )
                    if (*status != 0) {
                        *msg = "BAD error() call"
                    }
                    else {
                        *status = errorcode( *state_i = state(*name) )
                        if (*status != 0) {
                            *msg = "BAD state() call"
                        }
                        else {
                            if ((*error_i < 0 && *error_i == -*state_i) ||
                                (*error_i > 0 && *error_i ==  *state_i))
                            {
                              *msg = ""
                              *error_int = *error_i
                            }
                        }
                    }
                    if (*msg != "") { writeLine('stderr', *msg) }
                }
            ''')
        }

        rule_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                test() {
                    *symbols = list("SYS_INTERNAL_ERR",
                                    "RULE_ENGINE_CONTINUE")
                    foreach (*sym in *symbols) {
                      validate_error_and_state_smsi (*sym, *error_int)
                      writeLine("stdout", "*sym = *error_int")
                    }
                }
                OUTPUT ruleExecOut
            ''')
        }

        with temporary_core_file() as core:
            core.add_rule(pep_map[self.plugin_name])
            with tempfile.NamedTemporaryFile(mode='w+t', suffix='.r') as rule_file:
                print(rule_map[self.plugin_name], file = rule_file)
                rule_file.flush()
                self.admin.assert_icommand(['irule','-F',rule_file.name], 'STDOUT_MULTILINE', ['SYS_INTERNAL_ERR = -154000',
                                                                                               'RULE_ENGINE_CONTINUE = 5000000'])

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Native only test when not in a topology')
    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Do not run in topology on resource server')
    def test_peps_for_parallel_mode_transfers__4404(self):
        # Each PEP annotates a resource with a metadata AVU in a dynamic PEP.
        # Needs to be formatted such that argument 0 is 'get' or 'put' and argument 1 is the name of the resource to annotate.
        pep_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                pep_api_data_obj_{0}_pre (*INSTANCE_NAME, *COMM, *DATAOBJINP, *BUFFER, *PORTAL_OPR_OUT)
                {{
                    msiAddKeyVal(*key_val_pair,"test_peps_for_parallel_mode_transfers__4404_{0}","data-obj-{0}-pre");
                    msiAssociateKeyValuePairsToObj(*key_val_pair,"{1}","-R");
                }}
                pep_api_data_obj_{0}_post (*INSTANCE_NAME, *COMM, *DATAOBJINP, *BUFFER, *PORTAL_OPR_OUT)
                {{
                    msiAddKeyVal(*key_val_pair,"test_peps_for_parallel_mode_transfers__4404_{0}","data-obj-{0}-post");
                    msiAssociateKeyValuePairsToObj(*key_val_pair,"{1}","-R");
                }}
                pep_api_data_obj_{0}_except (*INSTANCE_NAME, *COMM, *DATAOBJINP, *BUFFER, *PORTAL_OPR_OUT)
                {{
                    msiAddKeyVal(*key_val_pair,"test_peps_for_parallel_mode_transfers__4404_{0}","data-obj-{0}-except");
                    msiAssociateKeyValuePairsToObj(*key_val_pair,"{1}","-R");
                }}
                pep_api_data_obj_{0}_finally (*INSTANCE_NAME, *COMM, *DATAOBJINP, *BUFFER, *PORTAL_OPR_OUT)
                {{
                    msiAddKeyVal(*key_val_pair,"test_peps_for_parallel_mode_transfers__4404_{0}","data-obj-{0}-finally");
                    msiAssociateKeyValuePairsToObj(*key_val_pair,"{1}","-R");
                }}
            ''')
        }

        (fd, largefile) = tempfile.mkstemp()
        os.write(fd, b"123456789abcdef\n"*(6*1024**2))
        os.close(fd)
        temp_base = os.path.basename(largefile)
        temp_dir  = os.path.dirname(largefile)
        extrafile = ""

        try:
            get_peps = pep_map[self.plugin_name].format('get', self.resc_name)
            put_peps = pep_map[self.plugin_name].format('put', self.resc_name)

            self.reset_resource()
            self.helper_test_pep_with_metadata( put_peps, "iput -f {}".format(largefile), 'test_peps_for_parallel_mode_transfers__4404_put',
                { "data-obj-put-pre": True, "data-obj-put-post": True, "data-obj-put-except": False, "data-obj-put-finally": True } )

            self.reset_resource()
            self.helper_test_pep_with_metadata( get_peps, "iget -f {} {}".format(temp_base,temp_dir), 'test_peps_for_parallel_mode_transfers__4404_get',
                { "data-obj-get-pre": True, "data-obj-get-post": True, "data-obj-get-except": False, "data-obj-get-finally": True } )

            self.reset_resource()
            self.helper_test_pep_with_metadata( put_peps, "iput {}".format(largefile), 'test_peps_for_parallel_mode_transfers__4404_put',
                { "data-obj-put-pre": True, "data-obj-put-post": False, "data-obj-put-except": True, "data-obj-put-finally": True } )

            self.admin.run_icommand('ichmod null {} {}'.format(self.admin.username,temp_base))

            extrafile = os.path.join(temp_dir, temp_base + ".x")

            self.reset_resource()
            self.helper_test_pep_with_metadata( get_peps, "iget {} {}".format(temp_base,extrafile), 'test_peps_for_parallel_mode_transfers__4404_get',
                { "data-obj-get-pre": True, "data-obj-get-post": False, "data-obj-get-except": True, "data-obj-get-finally": True } )

        finally:

            self.admin.run_icommand('ichmod -M own {} {}'.format(self.admin.username,temp_base))
            os.unlink(largefile)
            if extrafile and os.path.isfile(extrafile):
                os.unlink(extrafile)
            self.admin.assert_icommand(['iadmin', 'rmresc', self.resc_name])
            self.admin.assert_icommand(['iadmin', 'rum'])

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Native only test when not in a topology')
    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Do not run in topology on resource server')
    def test_dynamic_policy_enforcement_point_exception_for_plugins__4128(self):
        pre_pep_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent(f'''
                pep_resource_open_pre(*INST, *CTX, *OUT)
                {{
                    msiAddKeyVal(*key_val_pair,"test_dynamic_policy_enforcement_point_exception_for_plugins__4128_pre_pep_fail","PRE PEP FAIL");
                    msiAssociateKeyValuePairsToObj(*key_val_pair,"{self.resc_name}","-R");
                    failmsg(-1, "PRE PEP FAIL")
                }}
                pep_resource_open_except(*INST, *CTX, *OUT)
                {{
                    msiAddKeyVal(*key_val_pair,"test_dynamic_policy_enforcement_point_exception_for_plugins__4128_pre_pep_fail","EXCEPT FOR PRE PEP FAIL");
                    msiAssociateKeyValuePairsToObj(*key_val_pair,"{self.resc_name}","-R");
                }}
            ''')
        }

        op_pep_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent(f'''
                pep_resource_open_except(*INST, *CTX, *OUT)
                {{
                    msiAddKeyVal(*key_val_pair,"test_dynamic_policy_enforcement_point_exception_for_plugins__4128_op_fail","EXCEPT FOR OPERATION FAIL");
                    msiAssociateKeyValuePairsToObj(*key_val_pair,"{self.resc_name}","-R");
                }}
            ''')
        }

        post_pep_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent(f'''
                pep_resource_open_post(*INST, *CTX, *OUT)
                {{
                    msiAddKeyVal(*key_val_pair,"test_dynamic_policy_enforcement_point_exception_for_plugins__4128_post_pep_fail","POST PEP FAIL");
                    msiAssociateKeyValuePairsToObj(*key_val_pair,"{self.resc_name}","-R");
                    failmsg(-1, "POST PEP FAIL")
                }}
                pep_resource_open_except(*INST, *CTX, *OUT)
                {{
                    msiAddKeyVal(*key_val_pair,"test_dynamic_policy_enforcement_point_exception_for_plugins__4128_post_pep_fail","EXCEPT FOR POST PEP FAIL");
                    msiAssociateKeyValuePairsToObj(*key_val_pair,"{self.resc_name}","-R");
                }}
            ''')
        }

        try:
            path = self.admin.get_vault_session_path('demoResc')

            self.admin.run_icommand('iput -f '+self.testfile+' good_file')
            self.admin.run_icommand('iput -f '+self.testfile+' bad_file')
            os.unlink(os.path.join(path, 'bad_file'))

            pre_pep_fail = pre_pep_map[self.plugin_name]
            op_fail = op_pep_map[self.plugin_name]
            post_pep_fail = post_pep_map[self.plugin_name]

            self.reset_resource()
            self.helper_test_pep_with_metadata(pre_pep_fail, 'iget -f '+self.testfile,
                    inspect.currentframe().f_code.co_name + '_pre_pep_fail',
                    { 'EXCEPT FOR PRE PEP FAIL': True })

            self.reset_resource()
            self.helper_test_pep_with_metadata(op_fail, 'iget -f bad_file',
                    inspect.currentframe().f_code.co_name + '_op_fail',
                    { 'EXCEPT FOR OPERATION FAIL': True })

            self.reset_resource()
            self.helper_test_pep_with_metadata(post_pep_fail, 'iget -f '+self.testfile,
                    inspect.currentframe().f_code.co_name + '_post_pep_fail',
                    { 'EXCEPT FOR POST PEP FAIL': True })

        finally:
            self.admin.assert_icommand(['iadmin', 'rmresc', self.resc_name])
            self.admin.assert_icommand(['iadmin', 'rum'])

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Native only test when not in a topology')
    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Do not run in topology on resource server')
    def test_dynamic_policy_enforcement_point_exception_for_apis__4128(self):
        pre_pep_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent(f'''
                pep_api_data_obj_get_pre(*INST, *COMM, *INP, *PORT, *BUF)
                {{
                    msiAddKeyVal(*key_val_pair,"test_dynamic_policy_enforcement_point_exception_for_apis__4128_pre_pep_fail","PRE PEP FAIL");
                    msiAssociateKeyValuePairsToObj(*key_val_pair,"{self.resc_name}","-R");
                    failmsg(-1, "PRE PEP FAIL")
                }}
                pep_api_data_obj_get_except(*INST, *COMM, *INP, *PORT, *BUF)
                {{
                    msiAddKeyVal(*key_val_pair,"test_dynamic_policy_enforcement_point_exception_for_apis__4128_pre_pep_fail","EXCEPT FOR PRE PEP FAIL");
                    msiAssociateKeyValuePairsToObj(*key_val_pair,"{self.resc_name}","-R");
                }}
            ''')
        }

        op_pep_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent(f'''
                pep_api_data_obj_get_except(*INST, *COMM, *INP, *PORT, *BUF)
                {{
                    msiAddKeyVal(*key_val_pair,"test_dynamic_policy_enforcement_point_exception_for_apis__4128_op_fail","EXCEPT FOR OPERATION FAIL");
                    msiAssociateKeyValuePairsToObj(*key_val_pair,"{self.resc_name}","-R");
                }}
            ''')
        }

        post_pep_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent(f'''
                pep_api_data_obj_get_post(*INST, *COMM, *INP, *PORT, *BUF)
                {{
                    msiAddKeyVal(*key_val_pair,"test_dynamic_policy_enforcement_point_exception_for_apis__4128_post_pep_fail","POST PEP FAIL");
                    msiAssociateKeyValuePairsToObj(*key_val_pair,"{self.resc_name}","-R");
                    failmsg(-1, "POST PEP FAIL")
                }}
                pep_api_data_obj_get_except(*INST, *COMM, *INP, *PORT, *BUF)
                {{
                    msiAddKeyVal(*key_val_pair,"test_dynamic_policy_enforcement_point_exception_for_apis__4128_post_pep_fail","EXCEPT FOR POST PEP FAIL");
                    msiAssociateKeyValuePairsToObj(*key_val_pair,"{self.resc_name}","-R");
                }}
            ''')
        }
        try:
            path = self.admin.get_vault_session_path('demoResc')

            self.admin.run_icommand('iput -f '+self.testfile+' good_file')
            self.admin.run_icommand('iput -f '+self.testfile+' bad_file')
            os.unlink(os.path.join(path, 'bad_file'))

            pre_pep_fail = pre_pep_map[self.plugin_name]
            op_fail = op_pep_map[self.plugin_name]
            post_pep_fail = post_pep_map[self.plugin_name]

            self.reset_resource()
            self.helper_test_pep_with_metadata(pre_pep_fail, 'iget -f '+self.testfile,
                    inspect.currentframe().f_code.co_name + '_pre_pep_fail',
                    { 'EXCEPT FOR PRE PEP FAIL': True })

            self.reset_resource()
            self.helper_test_pep_with_metadata(op_fail, 'iget -f bad_file',
                    inspect.currentframe().f_code.co_name + '_op_fail',
                    { 'EXCEPT FOR OPERATION FAIL': True })

            self.reset_resource()
            self.helper_test_pep_with_metadata(post_pep_fail, 'iget -f '+self.testfile,
                    inspect.currentframe().f_code.co_name + '_post_pep_fail',
                    { 'EXCEPT FOR POST PEP FAIL': True })

        finally:
            self.admin.assert_icommand(['iadmin', 'rmresc', self.resc_name])
            self.admin.assert_icommand(['iadmin', 'rum'])


    @unittest.skipIf(plugin_name != 'irods_rule_engine_plugin-python' or not test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Python only test from resource server in a topology')
    def test_remote_rule_execution(self):
        rule_map = {
            'irods_rule_engine_plugin-python': textwrap.dedent('''
                def main(rule_args, callback, rei):
                    rule_code = "def main(rule_args, callback, rei):\\n    print('XXXX - PREP REMOTE EXEC TEST')"
                    callback.py_remote('{}', '', rule_code, '')
                INPUT null
                OUTPUT ruleExecOut
            ''')
        }

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
        rule_code = rule_map[self.plugin_name].format(test.settings.ICAT_HOSTNAME)
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
        pep_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                pep_network_agent_start_pre(*INST,*CTX,*OUT) {
                    *OUT = "THIS IS AN OUT VARIABLE"
                }
                pep_network_agent_start_post(*INST,*CTX,*OUT){
                    writeLine( 'serverLog', '*OUT')
                }
            '''),
            'irods_rule_engine_plugin-python': textwrap.dedent('''
                def pep_network_agent_start_pre(rule_args, callback, rei):
                    rule_args[2] = 'THIS IS AN OUT VARIABLE'

                def pep_network_agent_start_post(rule_args, callback, rei):
                    callback.writeLine('serverLog', rule_args[2])
            ''')
        }

        irodsctl = IrodsController()
        server_config_filename = paths.server_config_path()

        # load server_config.json to inject a new rule base
        with open(server_config_filename) as f:
            svr_cfg = json.load(f)

        # Occasionally, the expected message is printed twice due to the rule engine waking up, causing the test to fail
        # Make irodsDelayServer sleep for a long time so it won't wake up while the test is running
        svr_cfg['advanced_settings']['delay_server_sleep_time_in_seconds'] = 1000

        # dump to a string to repave the existing server_config.json
        new_server_config = json.dumps(svr_cfg, sort_keys=True, indent=4, separators=(',', ': '))

        with lib.file_backed_up(server_config_filename):
            # repave the existing server_config.json
            with open(server_config_filename, 'w') as f:
                f.write(new_server_config)

            # Bounce server to apply setting
            irodsctl.restart()

            # Actually run the test
            self.helper_test_pep(pep_map[self.plugin_name], "iput -f --metadata ATTR;VALUE;UNIT "+self.testfile)

        # Bounce server to get back original settings
        irodsctl.restart()

    def test_auth_pep(self):
        pep_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                pep_resource_resolve_hierarchy_pre(*INSTANCE,*CONTEXT,*OUT,*OPERATION,*HOST,*PARSER,*VOTE){
                    *OUT = "THIS IS AN OUT VARIABLE"
                }
                pep_resource_resolve_hierarchy_post(*INSTANCE,*CONTEXT,*OUT,*OPERATION,*HOST,*PARSER,*VOTE){
                    writeLine( 'serverLog', '*OUT')
                }
            '''),
            'irods_rule_engine_plugin-python': textwrap.dedent('''
                def pep_resource_resolve_hierarchy_pre(rule_args, callback, rei):
                    rule_args[2] = 'THIS IS AN OUT VARIABLE'

                def pep_resource_resolve_hierarchy_post(rule_args, callback, rei):
                    callback.writeLine('serverLog', rule_args[2])
            ''')
        }

        self.helper_test_pep(pep_map[self.plugin_name], "iput -f --metadata ATTR;VALUE;UNIT "+self.testfile)

    def test_out_variable(self):
        pep_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                pep_resource_resolve_hierarchy_pre(*INSTANCE,*CONTEXT,*OUT,*OPERATION,*HOST,*PARSER,*VOTE){
                    *OUT = "THIS IS AN OUT VARIABLE"
                }
                pep_resource_resolve_hierarchy_post(*INSTANCE,*CONTEXT,*OUT,*OPERATION,*HOST,*PARSER,*VOTE){
                    writeLine( 'serverLog', '*OUT')
                }
            '''),
            'irods_rule_engine_plugin-python': textwrap.dedent('''
                def pep_resource_resolve_hierarchy_pre(rule_args, callback, rei):
                    rule_args[2] = 'THIS IS AN OUT VARIABLE'

                def pep_resource_resolve_hierarchy_post(rule_args, callback, rei):
                    callback.writeLine('serverLog', rule_args[2])
            ''')
        }

        self.helper_test_pep(pep_map[self.plugin_name], "iput -f --metadata ATTR;VALUE;UNIT "+self.testfile)

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Python rule engine does not use re_serialization except for api plugins')
    def test_re_serialization(self):
        pep_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                pep_resource_resolve_hierarchy_pre(*INSTANCE,*CONTEXT,*OUT,*OPERATION,*HOST,*PARSER,*VOTE){
                    writeLine("serverLog", "pep_resource_resolve_hierarchy_pre - [*INSTANCE] [*CONTEXT] [*OUT] [*OPERATION] [*HOST] [*PARSER] [*VOTE]");
                }
            '''),
            'irods_rule_engine_plugin-python': textwrap.dedent('''
                def pep_resource_resolve_hierarchy_pre(rule_args, callback, rei):
                    callback.writeLine('serverLog', 'pep_resource_resolve_hierarchy_pre - [{0}] [{1}] [{2}] [{3}] [{4}] [{5}] [{6}]'.format(rule_args[0], rule_args[1], rule_args[2], rule_args[3], rule_args[4], rule_args[5], rule_args[6]))
            ''')
        }

        self.helper_test_pep(pep_map[self.plugin_name], "iput -f --metadata ATTR;VALUE;UNIT "+self.testfile,
            ['file_size=33', 'logical_path=/tempZone/home/otherrods', 'metadataIncluded'])

    def test_api_plugin(self):
        pep_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                pep_api_hello_world_pre(*INST, *OUT, *HELLO_IN, *HELLO_OUT) {
                    writeLine("serverLog", "pep_api_hello_world_pre - *INST *OUT *HELLO_IN, *HELLO_OUT");
                }
                pep_api_hello_world_post(*INST, *OUT, *HELLO_IN, *HELLO_OUT) {
                    writeLine("serverLog", "pep_api_hello_world_post - *INST *OUT *HELLO_IN, *HELLO_OUT");
                }
            '''),
            'irods_rule_engine_plugin-python': textwrap.dedent('''
                def pep_api_hello_world_pre(rule_args, callback, rei):
                    hello_inp = rule_args[2]
                    hello_inp_string = ', '.join([k + '=' + hello_inp[k] for k in hello_inp])
                    hello_out = rule_args[3]
                    hello_out_string = ', '.join([k + '=' + hello_out[k] for k in hello_out])
                    callback.writeLine('serverLog', 'pep_api_hello_world_pre - {0} {1} {2}, {3}'.format(rule_args[0], rule_args[1], hello_inp_string, hello_out_string))

                def pep_api_hello_world_post(rule_args, callback, rei):
                    hello_inp = rule_args[2]
                    hello_inp_string = ', '.join([k + '=' + hello_inp[k] for k in hello_inp])
                    hello_out = rule_args[3]
                    hello_out_string = ', '.join([k + '=' + hello_out[k] for k in hello_out])
                    callback.writeLine('serverLog', 'pep_api_hello_world_post - {0} {1} {2}, {3}'.format(rule_args[0], rule_args[1], hello_inp_string, hello_out_string))
            ''')
        }

        self.helper_test_pep(pep_map[self.plugin_name], "iapitest", ['pep_api_hello_world_pre -', ', null_value', 'HELLO WORLD', 'pep_api_hello_world_post -', 'value=128'])

    def test_out_string(self):
        pep_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                test(*OUT) { *OUT = "this_is_the_out_string"; }
                acPostProcForPut {
                    test(*out_string);
                    writeLine("serverLog", "out_string = *out_string");
                }
            '''),
            'irods_rule_engine_plugin-python': textwrap.dedent('''
                def test(rule_args, callback, rei):
                    rule_args[0] = 'this_is_the_out_string'
                def acPostProcForPut(rule_args, callback, rei):
                    callback.writeLine("serverLog", callback.test('')['arguments'][0]);
            ''')
        }

        self.helper_test_pep(pep_map[self.plugin_name], "iput -f "+self.testfile, ['this_is_the_out_string'])

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Python rule engine has no mechanism to update session vars')
    def test_rule_engine_2242(self):
        rule_map = {
            'irods_rule_engine_plugin-irods_rule_language': [
                textwrap.dedent('''
                    test {
                        $userNameClient = "foobar";
                    }
                    INPUT *A="status"
                    OUTPUT ruleExecOut
                '''),
                textwrap.dedent('''
                    test {
                        $status = \"1\";
                    }

                    acPreProcForWriteSessionVariable(*x) {
                        writeLine(\"stdout\", \"bwahahaha\");
                        succeed;
                    }

                    INPUT *A=\"status\"
                    OUTPUT ruleExecOut
                '''),
            ]
        }

        rules = rule_map[self.plugin_name]

        rule_file1 = "rule1_2242.r"
        with open(rule_file1, 'wt') as f:
            print(rules[0], file=f, end='')

        out, _, _ = self.admin.run_icommand("irule -r irods_rule_engine_plugin-irods_rule_language-instance -F " + rule_file1)
        assert 'Update session variable $userNameClient not allowed' in out

        rule_file2 = "rule2_2242.r"
        with open(rule_file2, 'wt') as f:
            print(rules[1], file=f, end='')

        self.admin.assert_icommand("irule -r irods_rule_engine_plugin-irods_rule_language-instance -F " + rule_file2, "EMPTY")

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: reads re server log')
    def test_rule_engine_2309(self):
        # Format this string with 'put' or 'get' to inject the desired string into the rule
        pep_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                acSetNumThreads() {{
                    writeLine("serverLog", "test_rule_engine_2309: {}: acSetNumThreads oprType [$oprType]");
                }}
            '''),
            'irods_rule_engine_plugin-python': textwrap.dedent('''
                def acSetNumThreads(rule_args, callback, rei):
                    opr_type = str(rei.doinp.oprType)
                    callback.writeLine('serverLog', 'test_rule_engine_2309: {}: acSetNumThreads oprType [' + str(opr_type) + ']')
            ''')
        }

        rule_text = pep_map[self.plugin_name]

        coredvm = paths.core_re_directory() + "/core.dvm"
        with lib.file_backed_up(coredvm):
            lib.prepend_string_to_file('oprType||rei->doinp->oprType\n', coredvm)
            with temporary_core_file() as core:
                core.add_rule(rule_text.format('put'))

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
                core.add_rule(rule_text.format('get'))

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
        rule_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                test_msiSegFault {{
                    writeLine("stdout", "We are about to segfault...");
                    msiSegFault();
                    writeLine("stdout", "You should never see this line.");
                }}
                OUTPUT ruleExecOut
                ''')
        }

        rule_file = 'test_msiSegFault.r'
        with open(rule_file, 'w') as f:
            f.write(rule_map[self.plugin_name])

        try:
            rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'

            # Should get SYS_INTERNAL_ERR because it's a segmentation fault
            self.admin.assert_icommand(['irule', '-r', rep_name, '-F', rule_file], 'STDERR', 'SYS_INTERNAL_ERR')

            # Should get CAT_INSUFFICIENT_PRIVILEGE_LEVEL in the log because this is for admin users only
            self.user0.assert_icommand(['irule', '-r', rep_name, '-F', rule_file], 'STDERR', 'CAT_INSUFFICIENT_PRIVILEGE_LEVEL')

        finally:
            os.unlink(rule_file)

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'rule language only')
    def test_msiSetResource__issue_7319(self):
        rule_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                test_msiSetResource {{
                    msiSetResource("testResc");
                }}
                OUTPUT ruleExecOut
                ''')
        }

        rule_file = 'test_msiSetResource.r'
        with open(rule_file, 'w') as f:
            f.write(rule_map[self.plugin_name])

        try:
            rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'

            self.admin.assert_icommand(['irule', '-r', rep_name, '-F', rule_file], 'STDERR', 'SYS_INTERNAL_NULL_INPUT_ERR', desired_rc=4)
            self.user0.assert_icommand(['irule', '-r', rep_name, '-F', rule_file], 'STDERR', 'SYS_INTERNAL_NULL_INPUT_ERR', desired_rc=4)

        finally:
            os.remove(rule_file)

    def test_re_serialization_for_RsComm_ptr__issue_5950(self):
        pep_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                pep_api_auth_request_pre(*instance, *comm, *request) {
                    # TODO: probably want to include more members here
                    *err = msiGetValByKey(*comm, "option", *option);
                    if (*err != 0) {
                        fail(*err);
                    }

                    if (*option != 'ils') {
                        fail(error(USER_INPUT_OPTION_ERR));
                    }
                }
            '''),
            'irods_rule_engine_plugin-python': textwrap.dedent('''
                def pep_api_auth_request_pre(rule_args, callback, rei):
                    option = rule_args[1].map()['option']
                    print(option)

                    return 0 if option == 'ils' else irods_errors.USER_INPUT_OPTION_ERR
            ''')
        }

        with temporary_core_file() as core:
            core.add_rule(pep_map[self.plugin_name])
            self.user0.assert_icommand(['ils'], 'STDOUT', self.user0.session_collection)

