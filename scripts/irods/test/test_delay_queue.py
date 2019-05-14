from __future__ import print_function
import sys
if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import json
import os
import time

from . import resource_suite
from .. import test
from .. import paths
from .. import lib
from ..configuration import IrodsConfig
from ..controller import IrodsController
from .rule_texts_for_tests import rule_texts

@unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: reads server log')
class Test_Delay_Queue(resource_suite.ResourceBase, unittest.TestCase):
    plugin_name = IrodsConfig().default_rule_engine_plugin
    class_name = 'Test_Delay_Queue'

    def setUp(self):
        super(Test_Delay_Queue, self).setUp()

    def tearDown(self):
        super(Test_Delay_Queue, self).tearDown()

    def count_strings_in_queue(self, string):
        _,out,_ = self.admin.assert_icommand(['iqstat'], 'STDOUT', 'id     name')
        count = 0
        for line in out.splitlines():
            if string in line:
                count += 1
        return count

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Delete this line on resolution of #4094')
    def test_failed_delay_job(self):
        irodsctl = IrodsController()
        server_config_filename = paths.server_config_path()

        # Simple delay rule with a writeLine
        rule_text_key = 'test_failed_delay_job'
        rule_text = rule_texts[self.plugin_name][self.class_name][rule_text_key]
        rule_file = rule_text_key + '.r'
        with open(rule_file, 'w') as f:
            f.write(rule_text)

        try:
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
                irodsctl.restart(test_mode=True)

                # Fire off rule and wait for message to get written out to serverLog
                initial_size_of_server_log = lib.get_file_size_by_path(paths.server_log_path())
                self.admin.assert_icommand(['irule', '-F', rule_file], 'STDOUT_SINGLELINE', "rule queued")

                # Expected count is 3 because...
                # 1. writeLine writes the string
                # 2. rsExecRuleExpression writes the rule text on failure
                # 3. delayed execution server writes the failed rule text under delay_server category
                expected_count = 3
                lib.delayAssert(
                    lambda: lib.log_message_occurrences_equals_count(
                        msg='We are about to fail...',
                        count=expected_count,
                        start_index=initial_size_of_server_log))

        finally:
            os.remove(rule_file)
            irodsctl.restart(test_mode=True)

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Delete this line on resolution of #4094')
    def test_delay_queue_with_long_job(self):
        irodsctl = IrodsController()
        server_config_filename = paths.server_config_path()

        delay_job_batch_size = 5
        re_server_sleep_time = 3
        sooner_delay = 1
        later_delay = re_server_sleep_time * 2
        long_job_run_time = re_server_sleep_time * 3

        parameters = {}
        parameters['delay_job_batch_size'] = delay_job_batch_size
        parameters['sooner_delay'] = sooner_delay
        parameters['later_delay'] = later_delay
        parameters['long_job_run_time'] = long_job_run_time
        rule_text_key = 'test_delay_queue_with_long_job'
        rule_text = rule_texts[self.plugin_name][self.class_name][rule_text_key].format(**parameters)
        rule_file = rule_text_key + '.r'
        with open(rule_file, 'w') as f:
            f.write(rule_text)

        try:
            # load server_config.json to inject new settings
            with open(server_config_filename) as f:
                svr_cfg = json.load(f)
            svr_cfg['advanced_settings']['maximum_number_of_concurrent_rule_engine_server_processes'] = 2
            svr_cfg['advanced_settings']['rule_engine_server_sleep_time_in_seconds'] = re_server_sleep_time

            # dump to a string to repave the existing server_config.json
            new_server_config = json.dumps(svr_cfg, sort_keys=True, indent=4, separators=(',', ': '))
            with lib.file_backed_up(server_config_filename):
                # repave the existing server_config.json
                with open(server_config_filename, 'w') as f:
                    f.write(new_server_config)

                # Bounce server to apply setting
                irodsctl.restart(test_mode=True)

                # Fire off rule and ensure the delay queue is correctly populated
                initial_size_of_server_log = lib.get_file_size_by_path(paths.server_log_path())
                self.admin.assert_icommand(['irule', '-F', rule_file])
                actual_count = self.count_strings_in_queue('writeLine')
                expected_count = delay_job_batch_size * 2 + 2

                # "Sooner" rules should execute
                expected_count = delay_job_batch_size
                lib.delayAssert(
                    lambda: lib.log_message_occurrences_equals_count(
                        msg='sooner:',
                        count=expected_count,
                        start_index=initial_size_of_server_log))
                self.assertTrue(0 == self.count_strings_in_queue('sooner:'))
                lib.delayAssert(
                    lambda: lib.log_message_occurrences_equals_count(
                        msg='later:',
                        count=0,
                        start_index=initial_size_of_server_log))
                self.assertTrue(delay_job_batch_size == self.count_strings_in_queue('later:'))
                self.assertTrue(1 == self.count_strings_in_queue('Sleeping...'))

                # "Later" rules should execute... but long-running job should still be in queue
                lib.delayAssert(
                    lambda: lib.log_message_occurrences_equals_count(
                        msg='later:',
                        count=expected_count,
                        start_index=initial_size_of_server_log))
                self.assertTrue(0 == self.count_strings_in_queue('later:'))
                self.assertTrue(1 == self.count_strings_in_queue('Sleeping...'))

                # Wait for long-running job to finish and make sure it has
                lib.delayAssert(
                    lambda: lib.log_message_occurrences_equals_count(
                        msg='Waking!',
                        start_index=initial_size_of_server_log))
                self.admin.assert_icommand(['iqstat'], 'STDOUT', 'No delayed rules pending for user')

        finally:
            os.remove(rule_file)
            irodsctl.restart(test_mode=True)

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Delete this line on resolution of #4094')
    def test_batch_delay_processing__3941(self):
        irodsctl = IrodsController()
        server_config_filename = paths.server_config_path()

        expected_count = 10
        parameters = {}
        parameters['expected_count'] = expected_count
        rule_text_key = 'test_batch_delay_processing__3941'
        rule_text = rule_texts[self.plugin_name][self.class_name][rule_text_key].format(**parameters)
        rule_file = rule_text_key + '.r'
        with open(rule_file, 'w') as f:
            f.write(rule_text)

        # load server_config.json to inject new settings
        with open(server_config_filename) as f:
            svr_cfg = json.load(f)
        re_server_sleep_time = 5
        svr_cfg['advanced_settings']['maximum_number_of_concurrent_rule_engine_server_processes'] = 2
        svr_cfg['advanced_settings']['rule_engine_server_sleep_time_in_seconds'] = re_server_sleep_time

        # dump to a string to repave the existing server_config.json
        new_server_config = json.dumps(svr_cfg, sort_keys=True, indent=4, separators=(',', ': '))
        with lib.file_backed_up(server_config_filename):
            # repave the existing server_config.json
            with open(server_config_filename, 'w') as f:
                f.write(new_server_config)

            # Bounce server to apply setting
            irodsctl.restart(test_mode=True)

            # Fire off rule and ensure the delay queue is correctly populated
            initial_size_of_server_log = lib.get_file_size_by_path(paths.server_log_path())
            self.admin.assert_icommand(['irule', '-F', rule_file])
            actual_count = self.count_strings_in_queue('writeLine')

            # Wait for messages to get written out and ensure that all the messages were written to serverLog
            lib.delayAssert(
                lambda: lib.log_message_occurrences_equals_count(
                    msg='writeLine: inString = delay ',
                    count=expected_count,
                    start_index=initial_size_of_server_log))
            self.admin.assert_icommand(['iqstat'], 'STDOUT', 'No delayed rules pending for user')

        os.remove(rule_file)

        # Bounce server to get back original settings
        irodsctl.restart(test_mode=True)

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Delete this line on resolution of #4094')
    def test_delay_block_with_output_param__3906(self):
        irodsctl = IrodsController()
        server_config_filename = paths.server_config_path()

        rule_text_key = 'test_delay_block_with_output_param__3906'
        rule_text = rule_texts[self.plugin_name][self.class_name][rule_text_key]
        rule_file = rule_text_key + '.r'
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
            irodsctl.restart(test_mode=True)

            # Fire off rule and wait for message to get written out to serverLog
            initial_size_of_server_log = lib.get_file_size_by_path(paths.server_log_path())
            self.admin.assert_icommand(['irule', '-F', rule_file], 'STDOUT_SINGLELINE', "rule queued")
            lib.delayAssert(
                lambda: lib.log_message_occurrences_equals_count(
                    msg='writeLine: inString = delayed rule executed',
                    start_index=initial_size_of_server_log))

        os.remove(rule_file)

        # Bounce server to get back original settings
        irodsctl.restart(test_mode=True)

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Delete this line on resolution of #4094')
    def test_delay_queue_connection_refresh(self):
        irodsctl = IrodsController()
        config = IrodsConfig()

        rule_sleep_time = 4
        parameters = {}
        parameters['sleep_time'] = rule_sleep_time
        rule_text_key = 'test_delay_queue_connection_refresh'
        rule_text = rule_texts[self.plugin_name][self.class_name][rule_text_key].format(**parameters)
        rule_file = rule_text_key + '.r'
        with open(rule_file, 'w') as f:
            f.write(rule_text)

        try:
            # load server_config.json to inject new settings
            server_config_filename = paths.server_config_path()
            with open(server_config_filename) as f:
                svr_cfg = json.load(f)
            svr_cfg['advanced_settings']['maximum_number_of_concurrent_rule_engine_server_processes'] = 1
            svr_cfg['advanced_settings']['rule_engine_server_sleep_time_in_seconds'] = 1

            # dump to a string to repave the existing server_config.json
            new_server_config = json.dumps(svr_cfg, sort_keys=True, indent=4, separators=(',', ': '))
            with lib.file_backed_up(server_config_filename):
                with lib.file_backed_up(config.client_environment_path):
                    # repave the existing server_config.json
                    with open(server_config_filename, 'w') as f:
                        f.write(new_server_config)

                    lib.update_json_file_from_dict(
                        config.client_environment_path,
                        {'irods_connection_pool_refresh_time_in_seconds' : 2})

                    # Bounce server to apply setting
                    irodsctl.restart(test_mode=True)

                    # Fire off rule and ensure the delay queue is correctly populated
                    initial_size_of_server_log = lib.get_file_size_by_path(paths.server_log_path())
                    self.admin.assert_icommand(['irule', '-F', rule_file])
                    time.sleep(rule_sleep_time * 2 + 2)

                    sleeping_pids = []
                    waking_pids = []
                    with open(paths.server_log_path(), 'r') as f:
                        f.seek(initial_size_of_server_log)
                        for line in f:
                            if 'sleep' in line:
                                log_line_map = json.loads(line)
                                sleeping_pids.append(log_line_map['server_pid'])
                            if 'wakeup' in line:
                                log_line_map = json.loads(line)
                                waking_pids.append(log_line_map['server_pid'])

                    expected_count = 2
                    self.assertTrue(len(sleeping_pids) == expected_count)
                    self.assertTrue(len(waking_pids) == expected_count)
                    for i in range(expected_count):
                        self.assertTrue(sleeping_pids[i] == waking_pids[i],
                            msg='sleeping_pid:{0} != waking_pid:{1}'.format(sleeping_pids[i], waking_pids[i]))
                        if i < expected_count - 1:
                            self.assertTrue(sleeping_pids[i] != sleeping_pids[i + 1],
                                msg='pids are equal:[{}]'.format(sleeping_pids[i]))

        finally:
            os.remove(rule_file)
            irodsctl.restart(test_mode=True)

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Delete this line on resolution of #4094')
    def test_sigpipe_in_delay_server(self):
        irodsctl = IrodsController()

        re_server_sleep_time = 2
        longer_delay_time = re_server_sleep_time * 2
        parameters = {}
        parameters['longer_delay_time'] = str(longer_delay_time)
        rule_text_key = 'test_sigpipe_in_delay_server'
        rule_text = rule_texts[self.plugin_name][self.class_name][rule_text_key].format(**parameters)
        rule_file = rule_text_key + '.r'
        with open(rule_file, 'w') as f:
            f.write(rule_text)

        try:
            # load server_config.json to inject new settings
            server_config_filename = paths.server_config_path()
            with open(server_config_filename) as f:
                svr_cfg = json.load(f)
            svr_cfg['advanced_settings']['rule_engine_server_sleep_time_in_seconds'] = re_server_sleep_time

            # dump to a string to repave the existing server_config.json
            new_server_config = json.dumps(svr_cfg, sort_keys=True, indent=4, separators=(',', ': '))
            with lib.file_backed_up(server_config_filename):
                # repave the existing server_config.json
                with open(server_config_filename, 'w') as f:
                    f.write(new_server_config)

                # Bounce server to apply setting
                irodsctl.restart(test_mode=True)

                # Fire off rule and wait for message to get written out to serverLog
                initial_size_of_server_log = lib.get_file_size_by_path(paths.server_log_path())
                self.admin.assert_icommand(['irule', '-F', rule_file], 'STDOUT_SINGLELINE', "rule queued")

                # Delayed rule writes to log and delay server writes rule that failed (agent should have died, so no rsExecRuleExpression)
                expected_count = 2
                lib.delayAssert(
                    lambda: lib.log_message_occurrences_equals_count(
                        msg='We are about to segfault...',
                        count=expected_count,
                        start_index=initial_size_of_server_log))

                # See if the later rule is executed (i.e. delay server is still alive)
                lib.delayAssert(
                    lambda: lib.log_message_occurrences_equals_count(
                        msg='Follow-up rule executed later!',
                        start_index=initial_size_of_server_log))


        finally:
            os.remove(rule_file)
            self.admin.assert_icommand(['iqdel', '-a'])
            irodsctl.restart(test_mode=True)

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Delete this line on resolution of #4094')
    def test_exception_in_delay_server(self):
        config = IrodsConfig()
        irodsctl = IrodsController()
        server_config_filename = paths.server_config_path()

        delay_job_sleep_time = 5
        parameters = {}
        parameters['sleep_time'] = str(delay_job_sleep_time)
        parameters['longer_delay_time'] = str(delay_job_sleep_time)
        rule_text_key = 'test_exception_in_delay_server'
        rule_text = rule_texts[self.plugin_name][self.class_name][rule_text_key].format(**parameters)
        rule_file = rule_text_key + '.r'
        with open(rule_file, 'w') as f:
            f.write(rule_text)

        odbc_ini_file = os.path.join(test.settings.FEDERATION.IRODS_DIR, '.odbc.ini')
        try:
            # load server_config.json to inject new settings
            server_config_filename = paths.server_config_path()
            with open(server_config_filename) as f:
                svr_cfg = json.load(f)
            svr_cfg['advanced_settings']['rule_engine_server_sleep_time_in_seconds'] = 1

            # dump to a string to repave the existing server_config.json
            new_server_config = json.dumps(svr_cfg, sort_keys=True, indent=4, separators=(',', ': '))
            with lib.file_backed_up(server_config_filename):
                with lib.file_backed_up(config.client_environment_path):
                    # repave the existing server_config.json
                    with open(server_config_filename, 'w') as f:
                        f.write(new_server_config)

                    lib.update_json_file_from_dict(
                        config.client_environment_path,
                        {'irods_connection_pool_refresh_time_in_seconds' : 2})

                    # Bounce server to apply setting
                    irodsctl.restart(test_mode=True)
                    initial_size_of_server_log = lib.get_file_size_by_path(paths.server_log_path())
                    with lib.file_backed_up(odbc_ini_file):
                        self.admin.assert_icommand(['irule', '-F', rule_file])
                        time.sleep(2)
                        os.unlink(odbc_ini_file)
                        time.sleep(delay_job_sleep_time * 2)

                    # Restore .odbc.ini and wait for delay server to execute rule properly
                    # The delay server should not have zombified, so the later rule should have executed
                    lib.delayAssert(
                        lambda: lib.log_message_occurrences_equals_count(
                            msg='Follow-up rule executed later!',
                            start_index=initial_size_of_server_log))

                    # The delay server should have caught the exception from the connection error on trying to delete the rule the first time
                    unexpected_string = 'terminating with uncaught exception of type std::runtime_error: connect error'
                    lib.delayAssert(
                        lambda: lib.log_message_occurrences_equals_count(
                            msg=unexpected_string,
                            count=0,
                            start_index=initial_size_of_server_log))

        finally:
            os.remove(rule_file)
            irodsctl.restart(test_mode=True)


@unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: reads server log')
class Test_Execution_Frequency(resource_suite.ResourceBase, unittest.TestCase):
    plugin_name = IrodsConfig().default_rule_engine_plugin
    class_name = 'Test_Execution_Frequency'

    def setUp(self):
        super(Test_Execution_Frequency, self).setUp()

        self.server_config_filename = paths.server_config_path()

        # load server_config.json to inject new settings
        with open(self.server_config_filename) as f:
            svr_cfg = json.load(f)
        self.re_server_sleep_time = 2
        svr_cfg['advanced_settings']['rule_engine_server_sleep_time_in_seconds'] = self.re_server_sleep_time

        self.new_server_config = json.dumps(svr_cfg, sort_keys=True, indent=4, separators=(',', ': '))

    def tearDown(self):
        super(Test_Execution_Frequency, self).tearDown()

    def assert_repeat_in_log(self, string, start_index, expected_count):
        lib.delayAssert(
            lambda: lib.log_message_occurrences_equals_count(
                msg=string,
                count=expected_count,
                start_index=start_index))

    #def test_repeat_for_ever(self):
    #def test_repeat_until_success(self):
    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Delete this line on resolution of #4094')
    def test_repeat_n_times(self):
        irodsctl = IrodsController()

        repeat_delay = self.re_server_sleep_time * 3
        repeat_n = 3
        repeat_string = 'repeating {repeat_n} times every {repeat_delay} seconds'.format(**locals())
        parameters = {}
        parameters['repeat_n'] = repeat_n
        parameters['repeat_delay'] = repeat_delay
        parameters['repeat_string'] = repeat_string
        rule_text_key = 'test_repeat_n_times'
        rule_text = rule_texts[self.plugin_name][self.class_name][rule_text_key].format(**parameters)
        rule_file = rule_text_key + '.r'
        with open(rule_file, 'w') as f:
            f.write(rule_text)

        try:
            with lib.file_backed_up(self.server_config_filename):
                # repave the existing server_config.json
                with open(self.server_config_filename, 'w') as f:
                    f.write(self.new_server_config)

                # Bounce server to apply setting
                irodsctl.restart(test_mode=True)

                string = 'writeLine: inString = {}'.format(repeat_string)
                initial_size_of_server_log = lib.get_file_size_by_path(paths.server_log_path())
                self.admin.assert_icommand(['irule', '-F', rule_file])

                for i in range(repeat_n):
                    self.assert_repeat_in_log(string, initial_size_of_server_log, i + 1)

        finally:
            os.remove(rule_file)
            irodsctl.restart(test_mode=True)

    #def test_repeat_until_time(self):
    #def test_repeat_until_success_or_until_time(self):
    #def test_repeat_until_success_or_n_times(self):
    #def test_double_for_ever(self):
    #def test_double_until_success(self):
    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Delete this line on resolution of #4094')
    def test_double_n_times(self):
        irodsctl = IrodsController()

        repeat_delay = self.re_server_sleep_time * 3
        repeat_n = 3
        repeat_string = 'doubling {repeat_n} times every {repeat_delay} seconds'.format(**locals())
        parameters = {}
        parameters['repeat_n'] = repeat_n
        parameters['repeat_delay'] = repeat_delay
        parameters['repeat_string'] = repeat_string
        rule_text_key = 'test_double_n_times'
        rule_text = rule_texts[self.plugin_name][self.class_name][rule_text_key].format(**parameters)
        rule_file = rule_text_key + '.r'
        with open(rule_file, 'w') as f:
            f.write(rule_text)

        try:
            with lib.file_backed_up(self.server_config_filename):
                # repave the existing server_config.json
                with open(self.server_config_filename, 'w') as f:
                    f.write(self.new_server_config)

                # Bounce server to apply setting
                irodsctl.restart(test_mode=True)

                string = 'writeLine: inString = {}'.format(repeat_string)
                initial_size_of_server_log = lib.get_file_size_by_path(paths.server_log_path())
                self.admin.assert_icommand(['irule', '-F', rule_file])
                for i in range(repeat_n):
                    self.assert_repeat_in_log(string, initial_size_of_server_log, i + 1)

        finally:
            os.remove(rule_file)
            irodsctl.restart(test_mode=True)

    #def test_double_until_time(self):
    #def test_double_until_success_or_until_time(self):
    #def test_double_until_success_or_n_times(self):
    #def test_double_until_success_upto_time(self):

