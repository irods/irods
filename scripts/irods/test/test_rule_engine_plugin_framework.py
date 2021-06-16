from __future__ import print_function
import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest
import copy
import inspect
import json
import os
import socket
import tempfile
import time  # remove once file hash fix is committed #2279
import subprocess
import mmap
import shutil
import re

from .. import lib
from .. import paths
from .. import test
from . import settings
from .resource_suite import ResourceBase
from ..configuration import IrodsConfig
from ..controller import IrodsController
from ..core_file import temporary_core_file
from .rule_texts_for_tests import rule_texts
from ..test.command import assert_command
from . import session

class Test_Rule_Engine_Plugin_Framework(session.make_sessions_mixin([('otherrods', 'rods')], []), unittest.TestCase):

    plugin_name = IrodsConfig().default_rule_engine_plugin

    def setUp(self):
        super(Test_Rule_Engine_Plugin_Framework, self).setUp()
        self.admin = self.admin_sessions[0]

    def tearDown(self):
        super(Test_Rule_Engine_Plugin_Framework, self).tearDown()

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python' or test.settings.RUN_IN_TOPOLOGY, "Skip for Python REP and Topology Testing")
    def test_bulk_put_operation__5164(self):
        depth = 2
        files_per_level = 1
        file_size = 100

        # make a local test dir under /tmp/
        local_dir = 'test_bulk_put_operation__5164'
        if(os.path.exists(local_dir)):
            shutil.rmtree(local_dir)

        try:
            lib.make_deep_local_tmp_dir(local_dir, depth, files_per_level, file_size)

            logical_path = os.path.join(self.admin.session_collection, local_dir)

            config = IrodsConfig()
            core_re_path = os.path.join(config.core_re_directory, 'core.re')

            with lib.file_backed_up(core_re_path):
                # Add PEPs to the core.re file.
                with open(core_re_path, 'a') as core_re:
                    core_re.write("""

    pep_api_bulk_data_obj_put_post(*INST, *COMM, *INP, *BUFF) {{
        *lp = "{}"
        *kvp.'attribute' = '*INP'
        *err = errormsg(msiSetKeyValuePairsToObj(*kvp, *lp, "-C"), *msg)
        if(*err < 0) {{
            writeLine("serverLog", "XXXX - *err [*msg]")
        }}
    }}

                    """.format(logical_path))

                self.admin.assert_icommand(['iput', '-b', '-r', local_dir], 'STDOUT_SINGLELINE', 'Running')
                self.admin.assert_icommand(['ils', '-l'], 'STDOUT_SINGLELINE', 'rods')
                self.admin.assert_icommand(['imeta', 'ls', '-C', logical_path], 'STDOUT_SINGLELINE', 'logical_path')
        finally:
            self.admin.assert_icommand(['irm', '-r', '-f', logical_path])
            shutil.rmtree(local_dir)

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python' or test.settings.RUN_IN_TOPOLOGY, "Skip for Python REP and Topology Testing")
    def test_continuation_does_not_cause_NO_MICROSERVICE_FOUND_ERR__issue_4383(self):
        config = IrodsConfig()

        with lib.file_backed_up(config.server_config_path):
            # PEPs.
            pep_api_data_obj_put_post = 'pep_api_data_obj_put_post'

            # Rule engine error codes to return.
            RULE_ENGINE_CONTINUE = 5000000

            # Enable the Passthrough REP (make it the first REP in the list).
            # Configure the Passthrough REP to return 'RULE_ENGINE_CONTINUE' to the REPF.
            # Set the log level to 'trace' for the rule engine and legacy logger categories.
            config.server_config['log_level']['rule_engine'] = 'trace'
            config.server_config['log_level']['legacy'] = 'trace'
            config.server_config['plugin_configuration']['rule_engines'].insert(0, {
                'instance_name': 'irods_rule_engine_plugin-passthrough-instance',
                'plugin_name': 'irods_rule_engine_plugin-passthrough',
                'plugin_specific_configuration': {
                    'return_codes_for_peps': [
                        {
                            'regex': '^' + pep_api_data_obj_put_post + '$',
                            'code': RULE_ENGINE_CONTINUE
                        }
                    ]
                }
            })
            lib.update_json_file_from_dict(config.server_config_path, config.server_config)

            # Capture the current size of the log file. This will be used as the starting
            # point for searching the log file for a particular string.
            log_offset = lib.get_file_size_by_path(paths.server_log_path())

            # Put a file (before fix, would produce "NO_MICROSERVICE_FOUND_ERR").
            filename = 'test_file_issue_4383.txt'
            lib.make_file(filename, 1024, 'arbitrary')
            self.admin.assert_icommand(['iput', filename])
            os.unlink(filename)

            # Look through the log file and try to find "PLUGIN_ERROR_MISSING_SHARED_OBJECT".
            with open(paths.server_log_path(), 'r') as log_file:
                mm = mmap.mmap(log_file.fileno(), 0, access=mmap.ACCESS_READ)
                index = mm.find("Returned '{0}' to REPF.".format(str(RULE_ENGINE_CONTINUE)), log_offset)
                self.assertTrue(index != -1)
                self.assertTrue(mm.find("PLUGIN_ERROR_MISSING_SHARED_OBJECT", index) == -1)
                mm.close()

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python' or test.settings.RUN_IN_TOPOLOGY, "Skip for Python REP and Topology Testing")
    def test_no_continuation_on_error(self):
        config = IrodsConfig()

        with lib.file_backed_up(config.server_config_path):
            # PEPs.
            pep_api_obj_stat_pre = 'pep_api_obj_stat_pre'
            pep_api_obj_stat_post = 'pep_api_obj_stat_post'

            # Rule engine error codes to return.
            RULE_ENGINE_CONTINUE = 5000000
            CAT_PASSWORD_EXPIRED = -840000

            # Enable the Passthrough REP (make it the first REP in the list).
            # Configure the Passthrough REP to return 'CAT_PASSWORD_EXPIRED' and 'RULE_ENGINE_CONTINUE'
            # to the REPF. Set the log level to 'trace' for the rule engine and legacy logger categories.
            config.server_config['log_level']['rule_engine'] = 'trace'
            config.server_config['log_level']['legacy'] = 'trace'
            config.server_config['plugin_configuration']['rule_engines'].insert(0, {
                'instance_name': 'irods_rule_engine_plugin-passthrough-instance',
                'plugin_name': 'irods_rule_engine_plugin-passthrough',
                'plugin_specific_configuration': {
                    'return_codes_for_peps': [
                        {
                            'regex': '^' + pep_api_obj_stat_pre + '$',
                            'code': CAT_PASSWORD_EXPIRED
                        },
                        {
                            'regex': '^' + pep_api_obj_stat_post + '$',
                            'code': RULE_ENGINE_CONTINUE
                        }
                    ]
                }
            })
            lib.update_json_file_from_dict(config.server_config_path, config.server_config)

            # Capture the current size of the log file. This will be used as the starting
            # point for searching the log file for a particular string.
            log_offset = lib.get_file_size_by_path(paths.server_log_path())

            # Trigger PEPs. Should return an error immediately.
            self.admin.assert_icommand('ils', 'STDERR_MULTILINE', [str(CAT_PASSWORD_EXPIRED)])

            # Search the log file for instances of CAT_PASSWORD_EXPIRED and RULE_ENGINE_CONTINUE.
            # Should only find CAT_PASSWORD_EXPIRED.
            msg_1 = "Returned '{0}' to REPF.".format(str(CAT_PASSWORD_EXPIRED))
            lib.delayAssert(
                lambda: lib.log_message_occurrences_equals_count(
                    msg=msg_1,
                    start_index=log_offset))
            msg_2 = "Returned '{0}' to REPF.".format(str(RULE_ENGINE_CONTINUE))
            lib.delayAssert(
                lambda: lib.log_message_occurrences_equals_count(
                    msg=msg_2,
                    count=0,
                    start_index=log_offset))


    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python' or test.settings.RUN_IN_TOPOLOGY, "Skip for Python REP and Topology Testing")
    def test_continuation_followed_by_an_error(self):
        config = IrodsConfig()

        with lib.file_backed_up(config.server_config_path):
            # PEPs.
            pep_database_open_pre = 'pep_database_open_pre'
            pep_api_gen_query_pre = 'pep_api_gen_query_pre'

            # Rule engine error codes to return.
            RULE_ENGINE_CONTINUE = 5000000
            CAT_PASSWORD_EXPIRED = -840000

            # Enable the Passthrough REP (make it the first REP in the list).
            # Configure the Passthrough REP to return 'CAT_PASSWORD_EXPIRED' and 'RULE_ENGINE_CONTINUE'
            # to the REPF. Set the log level to 'trace' for the rule engine and legacy logger categories.
            config.server_config['log_level']['rule_engine'] = 'trace'
            config.server_config['log_level']['legacy'] = 'trace'
            config.server_config['plugin_configuration']['rule_engines'].insert(0, {
                'instance_name': 'irods_rule_engine_plugin-passthrough-instance-1',
                'plugin_name': 'irods_rule_engine_plugin-passthrough',
                'plugin_specific_configuration': {
                    'return_codes_for_peps': [
                        {
                            'regex': '^' + pep_database_open_pre + '$',
                            'code': RULE_ENGINE_CONTINUE,
                        },
                        {
                            'regex': '^' + pep_api_gen_query_pre + '$',
                            'code': RULE_ENGINE_CONTINUE,
                        }
                    ]
                }
            })
            config.server_config['plugin_configuration']['rule_engines'].insert(1, {
                'instance_name': 'irods_rule_engine_plugin-passthrough-instance-2',
                'plugin_name': 'irods_rule_engine_plugin-passthrough',
                'plugin_specific_configuration': {
                    'return_codes_for_peps': [
                        {
                            'regex': '^' + pep_api_gen_query_pre + '$',
                            'code': CAT_PASSWORD_EXPIRED,
                        }
                    ]
                }
            })
            lib.update_json_file_from_dict(config.server_config_path, config.server_config)

            # Capture the current size of the log file. This will be used as the starting
            # point for searching the log file for a particular string.
            log_offset = lib.get_file_size_by_path(paths.server_log_path())

            # Trigger PEPs. Should return an error immediately.
            self.admin.assert_icommand('ils', 'STDERR_MULTILINE', [str(CAT_PASSWORD_EXPIRED)])

            # Search the log file for instances of CAT_PASSWORD_EXPIRED and RULE_ENGINE_CONTINUE.
            with open(paths.server_log_path(), 'r') as log_file:
                msg_1 = "Returned '{0}' to REPF.".format(str(RULE_ENGINE_CONTINUE))
                msg_2 = "Returned '{0}' to REPF.".format(str(CAT_PASSWORD_EXPIRED))
                mm = mmap.mmap(log_file.fileno(), 0, access=mmap.ACCESS_READ)
                index = mm.find(msg_1, log_offset)
                self.assertTrue(index != -1)
                self.assertTrue(mm.find(msg_2, index) != -1)
                mm.close()

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python' or test.settings.RUN_IN_TOPOLOGY, "Skip for Python REP and Topology Testing")
    def test_exec_rule_text_and_expression_supports_continuation__issue_4299(self):
        config = IrodsConfig()

        with lib.file_backed_up(config.server_config_path):
            # PEPs.
            pep_api_exec_my_rule_pre = 'pep_api_exec_my_rule_pre'
            pep_api_exec_rule_expression_pre = 'pep_api_exec_rule_expression_pre'

            # Rule engine error codes to return.
            RULE_ENGINE_CONTINUE = 5000000

            # Lower the delay server's sleep time so that rules are executed quicker.
            config.server_config['advanced_settings']['rule_engine_server_sleep_time_in_seconds'] = 1

            # Enable the Passthrough REP (make it the first REP in the list).
            # Configure the Passthrough REP to return 'RULE_ENGINE_CONTINUE' to the REPF.
            # Set the log level to 'trace' for the rule engine and legacy logger categories.
            config.server_config['log_level']['rule_engine'] = 'trace'
            config.server_config['log_level']['legacy'] = 'trace'
            config.server_config['plugin_configuration']['rule_engines'].insert(0, {
                'instance_name': 'irods_rule_engine_plugin-passthrough-instance',
                'plugin_name': 'irods_rule_engine_plugin-passthrough',
                'plugin_specific_configuration': {
                    'return_codes_for_peps': [
                        {
                            'regex': '^' + pep_api_exec_my_rule_pre + '$',
                            'code': RULE_ENGINE_CONTINUE
                        },
                        {
                            'regex': '^' + pep_api_exec_rule_expression_pre + '$',
                            'code': RULE_ENGINE_CONTINUE
                        }
                    ]
                }
            })
            lib.update_json_file_from_dict(config.server_config_path, config.server_config)

            IrodsController().restart(test_mode=True)

            # Test 1 - exec_rule_text
            # ~~~~~~~~~~~~~~~~~~~~~~~
            msg = 'exec_rule_text: Using -r to target a REP is not required anymore!'
            self.admin.assert_icommand(['irule', 'writeLine("stdout", "{0}")'.format(msg), 'null', 'ruleExecOut'], 'STDOUT', msg)

            # Test 2 - exec_rule_expression
            # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            # Capture the current size of the log file. This will be used as the starting
            # point for searching the log file for a particular string.
            log_offset = lib.get_file_size_by_path(paths.server_log_path())

            msg = 'exec_rule_expression: Using -r to target a REP is not required anymore!'
            self.admin.assert_icommand(['irule', 'delay("0.1s") {{ writeLine("serverLog", "{0}"); }}'.format(msg), 'null', 'null'])

            lib.delayAssert(lambda: lib.log_message_occurrences_greater_than_count(msg=msg, count=0, start_index=log_offset))

        IrodsController().restart(test_mode=True)

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python' or test.settings.RUN_IN_TOPOLOGY, "Skip for Python REP and Topology Testing")
    def test_repf_supports_skipping_operations_and_nrep_returns_positive_error_codes_to_repf__issues_4752_4753(self):
        config = IrodsConfig()
        core_re_path = os.path.join(config.core_re_directory, 'core.re')

        with lib.file_backed_up(core_re_path):
            # Disable put operation by returning RULE_ENGINE_SKIP_OPERATION (a.k.a. 5001000) to the REPF.
            with open(core_re_path, 'a') as core_re:
                core_re.write('pep_api_data_obj_put_pre(*INSTANCE_NAME, *COMM, *DATAOBJINP, *BUFFER, *PORTAL_OPR_OUT) { 5001000 }')
                core_re.write('pep_api_data_obj_put_post(*INSTANCE_NAME, *COMM, *DATAOBJINP, *BUFFER, *PORTAL_OPR_OUT) { -165000 }')

            filename = os.path.join(self.admin.local_session_dir, 'skip_operation.txt')
            lib.make_file(filename, 1, 'arbitrary')

            # This proves that the Post-PEP fired.
            self.admin.assert_icommand(['iput', filename], 'STDERR', ['SYS_UNKNOWN_ERROR'])

            # This proves that the operation was skipped.
            self.admin.assert_icommand(['ils', os.path.basename(filename)], 'STDERR_SINGLELINE', '{} does not exist'.format(os.path.basename(filename)))

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python' or test.settings.RUN_IN_TOPOLOGY, "Skip for Python REP and Topology Testing")
    def test_repf_logs_warning_if_skip_operation_code_is_returned_from_non_pre_peps__issue_4800(self):
        config = IrodsConfig()

        with lib.file_backed_up(config.server_config_path):
            config.server_config['log_level']['rule_engine'] = 'trace'
            lib.update_json_file_from_dict(config.server_config_path, config.server_config)

            core_re_path = os.path.join(config.core_re_directory, 'core.re')

            # Test Post-PEPs and Finally-PEPs.
            for pep_suffix in ['post', 'finally']:
                with lib.file_backed_up(core_re_path):
                    # Return RULE_ENGINE_SKIP_OPERATION (a.k.a. 5001000) to the REPF.
                    with open(core_re_path, 'a') as core_re:
                        core_re.write('pep_api_data_obj_put_{0}(*INSTANCE_NAME, *COMM, *DATAOBJINP, *BUFFER, *PORTAL_OPR_OUT) {{ 5001000 }}'.format(pep_suffix))

                    filename = os.path.join(self.admin.local_session_dir, 'foo')
                    lib.make_file(filename, 1, 'arbitrary')

                    # Capture the current size of the log file. This will be used as the starting
                    # point for searching the log file for a particular string.
                    log_offset = lib.get_file_size_by_path(paths.server_log_path())

                    self.admin.assert_icommand(['iput', filename])

                    msg = 'RULE_ENGINE_SKIP_OPERATION (5001000) incorrectly returned from PEP [pep_api_data_obj_put_{0}]'.format(pep_suffix)
                    lib.delayAssert(lambda: lib.log_message_occurrences_greater_than_count(msg=msg, count=0, start_index=log_offset))

                    self.admin.assert_icommand(['irm', os.path.basename(filename)])

            # Test Except-PEPs.
            with lib.file_backed_up(core_re_path):
                # Return RULE_ENGINE_SKIP_OPERATION (a.k.a. 5001000) to the REPF.
                with open(core_re_path, 'a') as core_re:
                    core_re.write('pep_api_data_obj_put_pre(*INSTANCE_NAME, *COMM, *DATAOBJINP, *BUFFER, *PORTAL_OPR_OUT) { -1 }\n')
                    core_re.write('pep_api_data_obj_put_except(*INSTANCE_NAME, *COMM, *DATAOBJINP, *BUFFER, *PORTAL_OPR_OUT) { 5001000 }\n')

                filename = os.path.join(self.admin.local_session_dir, 'foo')
                lib.make_file(filename, 1, 'arbitrary')

                # Capture the current size of the log file. This will be used as the starting
                # point for searching the log file for a particular string.
                log_offset = lib.get_file_size_by_path(paths.server_log_path())

                self.admin.assert_icommand_fail(['iput', filename])

                msg = 'RULE_ENGINE_SKIP_OPERATION (5001000) incorrectly returned from PEP [pep_api_data_obj_put_except]'
                lib.delayAssert(lambda: lib.log_message_occurrences_greater_than_count(msg=msg, count=0, start_index=log_offset))

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-irods_rule_language' or test.settings.RUN_IN_TOPOLOGY, "Skip for Native REP and Topology Testing")
    def test_python_rule_engine_plugin_supports_repf_continuation(self):
        config = IrodsConfig()

        with lib.file_backed_up(config.server_config_path):
            pep_name = 'pep_database_open_pre'
            RULE_ENGINE_CONTINUE = 5000000

            # Enable the Passthrough REP (make it the second REP in the list).
            # Configure the Passthrough REP to return 'RULE_ENGINE_CONTINUE' to the REPF.
            # Set the log level to 'trace' for the rule engine and legacy logger categories.
            config.server_config['log_level']['rule_engine'] = 'trace'
            config.server_config['log_level']['legacy'] = 'trace'
            config.server_config['plugin_configuration']['rule_engines'].insert(1, {
                'instance_name': 'irods_rule_engine_plugin-passthrough-instance',
                'plugin_name': 'irods_rule_engine_plugin-passthrough',
                'plugin_specific_configuration': {
                    'return_codes_for_peps': [
                        {
                            'regex': '^' + pep_name + '$',
                            'code': RULE_ENGINE_CONTINUE
                        }
                    ]
                }
            })
            lib.update_json_file_from_dict(config.server_config_path, config.server_config)

            core_re_path = os.path.join(config.core_re_directory, 'core.py')

            with lib.file_backed_up(core_re_path):
                first_msg = 'This should appear first!'

                # Add a new PEP to core.py that will write to the log file.
                # This will help to verify that the PREP is not blocking anything.
                with open(core_re_path, 'a') as core_re:
                    core_re.write(("def {0}(rule_args, callback, rei):\n"
                                   "    callback.writeLine('serverLog', '{1}')\n"
                                   "    return {2}\n").format(pep_name, first_msg, str(RULE_ENGINE_CONTINUE)))

                # Trigger the PREP and Passthrough REP.
                self.admin.run_icommand(['ils'])

                found_msg_1 = False
                found_msg_2 = False

                # Verify that the PREP message appears in the log file before the message
                # produced by the Passthrough REP.
                with open(paths.server_log_path(), 'r') as log_file:
                    mm = mmap.mmap(log_file.fileno(), 0, access=mmap.ACCESS_READ)
                    index = mm.find(first_msg)
                    if index != -1:
                        found_msg_1 = True
                        found_msg_2 = (mm.find("Returned '{0}' to REPF.".format(str(RULE_ENGINE_CONTINUE)), index) != -1)
                    mm.close()

                self.assertTrue(found_msg_1)
                self.assertTrue(found_msg_2)

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-irods_rule_language' or test.settings.RUN_IN_TOPOLOGY, "Skip for Native REP and Topology Testing")
    def test_python_rule_engine_plugin_supports_repf_skip_operation(self):
        config = IrodsConfig()
        core_re_path = os.path.join(config.core_re_directory, 'core.py')

        with lib.file_backed_up(core_re_path):
            # Disable puts by returning RULE_ENGINE_SKIP_OPERATION (a.k.a. 5001000) to the REPF.
            with open(core_re_path, 'a') as core_re:
                core_re.write('def pep_api_data_obj_put_pre(rule_args, callback, rei):\n\treturn 5001000\n')
                core_re.write('def pep_api_data_obj_put_post(rule_args, callback, rei):\n\treturn -165000\n')

            filename = os.path.join(self.admin.local_session_dir, 'skip_operation.txt')
            lib.make_file(filename, 1, 'arbitrary')

            # This proves that the Post-PEP fired.
            self.admin.assert_icommand(['iput', filename], 'STDERR', ['SYS_UNKNOWN_ERROR'])

            # This proves that the operation was skipped.
            self.admin.assert_icommand(['ils', os.path.basename(filename)], 'STDERR_SINGLELINE', '{} does not exist'.format(os.path.basename(filename)))

class Test_Plugin_Instance_Delay(ResourceBase, unittest.TestCase):

    plugin_name = IrodsConfig().default_rule_engine_plugin

    def setUp(self):
        super(Test_Plugin_Instance_Delay, self).setUp()

    def tearDown(self):
        super(Test_Plugin_Instance_Delay, self).tearDown()

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python' or test.settings.RUN_IN_TOPOLOGY, "Skip for Python REP and Topology Testing")
    def test_delay_rule__4055(self):
        server_config_filename = paths.server_config_path()
        with open(server_config_filename) as f:
            svr_cfg = json.load(f)
        native_plugin_name = svr_cfg['plugin_configuration']['rule_engines'][0]['instance_name']
        delay_rule = """
do_it {
    delay("<PLUSET>%s</PLUSET>") {
        writeLine("serverLog", "4055_%s.")
    }
}
INPUT null
OUTPUT ruleExecOut
        """
        multiplier = {'':1, ' ':1, 's':1, 'm':60, 'h':3600, 'd':3600*24, 'y':3600*24*365}
        for numstring,units in [ ('9',''), (' 9',' '), ('20','s'), ('5','m '), ('30','d'), ('1','y') ]:
            with tempfile.NamedTemporaryFile(mode='wt', suffix='.r', dir='/tmp') as rule_file:
                delay = numstring + units
                rule_file.write(delay_rule%(delay,delay))  # delay used directly and as unique search string
                rule_file.flush()
                self.admin.assert_icommand(['irule', '-r', native_plugin_name, '-F', rule_file.name])
                tm = int(time.time())
                regex_time = '^\s*RULE_EXEC_TIME\s*=\s*([0-9]+)'
                regex_id = '^\s*RULE_EXEC_ID\s*=\s*([0-9]+)'
                dummy_rc, out, dummy_err = self.admin.assert_icommand(
                    ['iquest', """SELECT RULE_EXEC_ID, RULE_EXEC_NAME, RULE_EXEC_TIME where RULE_EXEC_NAME like '%%4055_%s.%%'""" % delay],
                    'STDOUT_MULTILINE', regex_time, use_regex=True)
                exec_tm =  int(re.search(regex_time,out,flags=re.MULTILINE).groups(1)[0]) # get rule execution time (epoch secs) and ID
                rule_id =  re.search(regex_id,out,flags=re.MULTILINE).groups(1)[0]
                expected_delay = int(numstring) * multiplier[units[:1]]
                self.assertTrue((tm + 0.6*expected_delay) < exec_tm < (tm + 1.4*expected_delay)) # assert rule execution time accurate
                self.admin.assert_icommand(['iqdel', rule_id])

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python' or test.settings.RUN_IN_TOPOLOGY, "Skip for Python REP and Topology Testing")
    def test_delay_rule_with_invalid_time_format__4055(self):
        server_config_filename = paths.server_config_path()
        with open(server_config_filename) as f:
            svr_cfg = json.load(f)
        delay_rule = """
do_it {
    delay("<PLUSET>%s</PLUSET>") {
        writeLine("serverLog", "4055_%s.")
    }
}
INPUT null
OUTPUT ruleExecOut
        """
        native_plugin_name = svr_cfg['plugin_configuration']['rule_engines'][0]['instance_name']
        # test wrong units and non-integer values
        for time_value in ('1x', '0.1d', '.9y', '', ' ', '-9s'):
            with tempfile.NamedTemporaryFile(mode='wt', suffix='.r', dir='/tmp') as rule_file:
                rule_file.write(delay_rule%(time_value,time_value))
                rule_file.flush()
                self.admin.assert_icommand(['irule', '-r', native_plugin_name, '-F', rule_file.name], 'STDERR', 'INPUT_ARG_NOT_WELL_FORMED_ERR')
                self.admin.assert_icommand(['iquest', """SELECT RULE_EXEC_ID, RULE_EXEC_NAME, RULE_EXEC_TIME where RULE_EXEC_NAME like '%%4055_%s.%%'"""%time_value],
                                           'STDOUT', 'CAT_NO_ROWS_FOUND')

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python' or test.settings.RUN_IN_TOPOLOGY, "Skip for Python REP and Topology Testing")
    def test_delay_rule__3849(self):
        server_config_filename = paths.server_config_path()

        # load server_config.json to inject a new rule base
        with open(server_config_filename) as f:
            svr_cfg = json.load(f)

        # swap the native and c++ rule engines
        native_plugin_cfg = svr_cfg['plugin_configuration']['rule_engines'][0]
        cpp_plugin_cfg = svr_cfg['plugin_configuration']['rule_engines'][1]

        svr_cfg['plugin_configuration']['rule_engines'][0] = cpp_plugin_cfg
        svr_cfg['plugin_configuration']['rule_engines'][1] = native_plugin_cfg

        native_plugin_name = native_plugin_cfg['instance_name']

        # dump to a string to repave the existing server_config.json
        new_server_config=json.dumps(svr_cfg, sort_keys=True,indent=4, separators=(',', ': '))

        delay_rule = """
do_it {
    delay("<INST_NAME>irods_rule_engine_plugin-irods_rule_language-instance</INST_NAME>") {
        writeLine("serverLog", "Test_Plugin_Instance_Delay")
    }
}
INPUT null
OUTPUT ruleExecOut
        """
        rule_file = tempfile.NamedTemporaryFile(mode='wt', dir='/tmp', delete=False).name + '.r'
        with open(rule_file, 'w') as f:
            f.write(delay_rule)

        with lib.file_backed_up(paths.server_config_path()):
            # repave the existing server_config.json
            with open(server_config_filename, 'w') as f:
                f.write(new_server_config)

            initial_log_size = lib.get_file_size_by_path(paths.server_log_path())

            self.admin.assert_icommand(['irule', '-r', native_plugin_name, '-F', rule_file])
            self.admin.assert_icommand('iqstat', 'STDOUT_SINGLELINE', 'writeLine')

            lib.delayAssert(lambda: lib.count_occurrences_of_string_in_log(paths.server_log_path(), 'Test_Plugin_Instance_Delay', start_index=initial_log_size))

def delay_assert(command, interval=1, maxrep=5):
    success = False
    for _ in range(maxrep):
        time.sleep(interval)  # wait for test to fire
        try:
            command()
            success = True
            break
        except AssertionError:
            continue
    if not success:
        raise AssertionError

class Test_Plugin_Instance_CppDefault(ResourceBase, unittest.TestCase):

    plugin_name = IrodsConfig().default_rule_engine_plugin

    def setUp(self):
        super(Test_Plugin_Instance_CppDefault, self).setUp()

    def tearDown(self):
        super(Test_Plugin_Instance_CppDefault, self).tearDown()

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python' or test.settings.RUN_IN_TOPOLOGY, "Skip for Python REP and Topology Testing")
    def test_delay_rule(self):
        delay_rule = """
{
    "policy_to_invoke" : "irods_policy_enqueue_rule",
    "delay_conditions" : "",
    "parameters" : {
        "policy_to_invoke" : "irods_policy_execute_rule",
        "parameters" : {
            "policy_to_invoke" : "create_flag_object",
            "parameters" : {
            },
            "configuration" : {
            }
        }
    }
}
INPUT null
OUTPUT ruleExecOut
        """
        rule_file = tempfile.NamedTemporaryFile(mode='wt', dir='/tmp', delete=False).name + '.r'
        with open(rule_file, 'w') as f:
            f.write(delay_rule)

        flag_file = self.admin.session_collection + '/flag_file'
        new_rule = """
create_flag_object(*p, *c, *o) {{
msiDataObjCreate("{0}", "forceFlag=", *out)
}}""".format(flag_file)

        with temporary_core_file() as core:
            core.add_rule(new_rule)

            config = IrodsConfig()
            with lib.file_backed_up(config.server_config_path):
                config.server_config['advanced_settings']['rule_engine_server_sleep_time_in_seconds'] = 1
                lib.update_json_file_from_dict(config.server_config_path, config.server_config)
                IrodsController().restart()

                self.admin.assert_icommand(['irule', '-F', rule_file])
                self.admin.assert_icommand('iqstat', 'STDOUT_SINGLELINE', 'irods_policy_execute_rule')
                delay_assert(lambda: self.admin.assert_icommand(['ils', '-l', flag_file],  'STDOUT_SINGLELINE', 'flag_file'))



    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python' or test.settings.RUN_IN_TOPOLOGY, "Skip for Python REP and Topology Testing")
    def test_delay_rule_already_scheduled(self):
        self.admin.assert_icommand(['iqdel', '-a'])
        delay_rule = """
{
    "policy_to_invoke" : "irods_policy_enqueue_rule",
    "delay_conditions" : "",
    "parameters" : {
        "policy_to_invoke" : "irods_policy_execute_rule",
        "parameters" : {
            "policy_to_invoke" : "sleep_policy",
            "parameters" : {
            },
            "configuration" : {
            }
        }
    }
}
INPUT null
OUTPUT ruleExecOut
        """
        rule_file = tempfile.NamedTemporaryFile(mode='wt', dir='/tmp', delete=False).name + '.r'
        with open(rule_file, 'w') as f:
            f.write(delay_rule)

        flag_file = self.admin.session_collection + '/flag_file'
        new_rule = """
sleep_policy(*p, *c, *o) {{
msiSleep("60", "0")
}}"""

        with temporary_core_file() as core:
            core.add_rule(new_rule)

            config = IrodsConfig()
            with lib.file_backed_up(config.server_config_path):
                config.server_config['advanced_settings']['rule_engine_server_sleep_time_in_seconds'] = 1
                lib.update_json_file_from_dict(config.server_config_path, config.server_config)
                IrodsController().restart()

                self.admin.assert_icommand(['irule', '-F', rule_file])
                self.admin.assert_icommand('iqstat', 'STDOUT_SINGLELINE', 'sleep_policy')

                # expect only one instance of the rule queued
                self.admin.assert_icommand(['irule', '-F', rule_file])
                out, _, _ = self.admin.run_icommand(['iqstat'])
                print(out)
                lines = out.splitlines()
                assert(len(lines) == 2)
