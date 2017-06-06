from __future__ import print_function
import sys
if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import inspect
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

    def test_network_pep(self):
        self.helper_test_pep(rule_texts[self.plugin_name][self.class_name][inspect.currentframe().f_code.co_name], "iput -f --metadata ATTR;VALUE;UNIT "+self.testfile)

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
