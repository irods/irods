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

        for s in strings_to_check_for:
            count = lib.count_occurrences_of_string_in_log(paths.server_log_path(), s, start_index=initial_size_of_server_log)
            assert number_of_strings_to_look_for == count, 'Found {0} instead of {1} occurrences of {2}'.format(count, number_of_strings_to_look_for, s)


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

        out, _, _ = self.admin.run_icommand("irule -F " + rule_file1)
        assert 'Update session variable $userNameClient not allowed' in out

        rule_file2 = "rule2_2242.r"
        rule_string2 = rule_texts[self.plugin_name][self.class_name]['test_rule_engine_2242_2']

        with open(rule_file2, 'wt') as f:
            print(rule_string2, file=f, end='')

        self.admin.assert_icommand("irule -F " + rule_file2, "EMPTY")

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
                assert 1 == lib.count_occurrences_of_string_in_log(
                    paths.server_log_path(), 'writeLine: inString = test_rule_engine_2309: put: acSetNumThreads oprType [1]', start_index=initial_size_of_server_log)
                assert 0 == lib.count_occurrences_of_string_in_log(paths.server_log_path(), 'RE_UNABLE_TO_READ_SESSION_VAR', start_index=initial_size_of_server_log)
                os.unlink(trigger_file)

            with temporary_core_file() as core:
                time.sleep(1)  # remove once file hash fix is committed #2279
                core.add_rule(rule_texts[self.plugin_name][self.class_name][inspect.currentframe().f_code.co_name + '_2'])
                time.sleep(1)  # remove once file hash fix is committed #2279

                initial_size_of_server_log = lib.get_file_size_by_path(paths.server_log_path())
                self.admin.assert_icommand('iget {0}'.format(trigger_file), use_unsafe_shell=True)
                assert 1 == lib.count_occurrences_of_string_in_log(
                    paths.server_log_path(), 'writeLine: inString = test_rule_engine_2309: get: acSetNumThreads oprType [2]', start_index=initial_size_of_server_log)
                assert 0 == lib.count_occurrences_of_string_in_log(paths.server_log_path(), 'RE_UNABLE_TO_READ_SESSION_VAR', start_index=initial_size_of_server_log)
                os.unlink(trigger_file)

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER or plugin_name == 'irods_rule_engine_plugin-python', 'Skip for topology testing from resource server: reads server log')
    def test_batch_delay_processing__3941(self):
        irodsctl = IrodsController()
        server_config_filename = paths.server_config_path()

        # Simple delay rule with a writeLine
        expected_count = 10
        rule_text = '''
test_batch_delay_processing {{
    for(*i = 0; *i < {expected_count}; *i = *i + 1) {{
        delay("<PLUSET>0.1s</PLUSET>") {{
            writeLine("serverLog", "delay *i");
        }}
    }}
}}
OUTPUT ruleExecOut
'''.format(**locals())
        rule_file = 'test_batch_delay_processing__3941.r'
        with open(rule_file, 'w') as f:
            f.write(rule_text)

        # load server_config.json to inject new settings
        with open(server_config_filename) as f:
            svr_cfg = json.load(f)
        re_server_sleep_time = 5
        svr_cfg['advanced_settings']['maximum_number_of_concurrent_rule_engine_server_processes'] = 2
        svr_cfg['advanced_settings']['rule_engine_server_sleep_time_in_seconds'] = re_server_sleep_time
        svr_cfg['advanced_settings']['rule_engine_server_execution_time_in_seconds'] = 120

        # dump to a string to repave the existing server_config.json
        new_server_config = json.dumps(svr_cfg, sort_keys=True, indent=4, separators=(',', ': '))
        with lib.file_backed_up(server_config_filename):
            # repave the existing server_config.json
            with open(server_config_filename, 'w') as f:
                f.write(new_server_config)

            # Bounce server to apply setting
            irodsctl.restart()

            # Fire off rule and ensure the delay queue is correctly populated
            initial_size_of_server_log = lib.get_file_size_by_path(paths.server_log_path())
            self.admin.assert_icommand(['irule', '-F', rule_file])
            _,out,_ = self.admin.assert_icommand(['iqstat'], 'STDOUT', 'id     name')
            actual_count = 0
            for line in out.splitlines():
                if 'writeLine' in line:
                    actual_count += 1
            self.assertTrue(expected_count == actual_count, msg='expected {expected_count} in delay queue, found {actual_count}'.format(**locals()))

            # Wait for messages to get written out and ensure that all the messages were written to serverLog
            time.sleep(re_server_sleep_time)
            self.admin.assert_icommand(['iqstat'], 'STDOUT', 'No delayed rules pending for user')
            actual_count = lib.count_occurrences_of_string_in_log(paths.server_log_path(), 'writeLine: inString = delay ', start_index=initial_size_of_server_log)
            self.assertTrue(expected_count == actual_count, msg='expected {expected_count} occurrences in serverLog, found {actual_count}'.format(**locals()))

        os.remove(rule_file)

        # Bounce server to get back original settings
        irodsctl.restart()

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: reads server log')
    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Delete this line on resolution of #4094')
    def test_delay_block_with_output_param__3906(self):
        irodsctl = IrodsController()
        server_config_filename = paths.server_config_path()

        # Simple delay rule with a writeLine
        rule_text = '''
test_delay_with_output_param {{
    delay("<PLUSET>0.1s</PLUSET>") {{
        writeLine("serverLog", "delayed rule executed");
    }}
    *status = "rule queued";
}}
INPUT null
OUTPUT *status
'''.format(**locals())
        rule_file = 'test_delay_with_output_param__3906.r'
        with open(rule_file, 'w') as f:
            f.write(rule_text)

        # load server_config.json to inject new settings
        with open(server_config_filename) as f:
            svr_cfg = json.load(f)
        re_server_sleep_time = 2
        svr_cfg['advanced_settings']['rule_engine_server_sleep_time_in_seconds'] = re_server_sleep_time

        # dump to a string to repave the existing server_config.json
        new_server_config = json.dumps(svr_cfg, sort_keys=True, indent=4, separators=(',', ': '))
        with lib.file_backed_up(server_config_filename):
            # repave the existing server_config.json
            with open(server_config_filename, 'w') as f:
                f.write(new_server_config)

            # Bounce server to apply setting
            irodsctl.restart()

            # Fire off rule and wait for message to get written out to serverLog
            initial_size_of_server_log = lib.get_file_size_by_path(paths.server_log_path())
            self.admin.assert_icommand(['irule', '-F', rule_file], 'STDOUT_SINGLELINE', "rule queued")
            time.sleep(re_server_sleep_time)
            actual_count = lib.count_occurrences_of_string_in_log(paths.server_log_path(), 'writeLine: inString = delayed rule executed', start_index=initial_size_of_server_log)
            self.assertTrue(1 == actual_count, msg='expected 1 occurrence in serverLog, found {actual_count}'.format(**locals()))

        os.remove(rule_file)

        # Bounce server to get back original settings
        irodsctl.restart()
