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
from . import session
from .. import test
from .. import paths
from .. import lib
from ..configuration import IrodsConfig
from ..controller import IrodsController
from .rule_texts_for_tests import rule_texts

class Test_Delay_Queue(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):
    plugin_name = IrodsConfig().default_rule_engine_plugin
    class_name = 'Test_Delay_Queue'

    def setUp(self):
        super(Test_Delay_Queue, self).setUp()

        self.admin = self.admin_sessions[0]

        self.filename = 'test_delay_queue'
        self.local_path = os.path.join(self.admin.local_session_dir, self.filename)
        self.logical_path = os.path.join(self.admin.session_collection, self.filename)
        if not os.path.exists(self.local_path):
            lib.make_file(self.local_path, 1024)
        self.admin.assert_icommand(['iput', self.local_path, self.logical_path])
        out, err, rc = self.admin.run_icommand(['iquest', '%s', '"select DATA_ID where DATA_NAME = \'{}\'"'.format(self.filename)])
        self.assertEqual(rc, 0, msg='error occurred getting data_id:[{}],stderr:[{}]'.format(rc, err))
        self.data_id = out.strip()

    def tearDown(self):
        self.admin.assert_icommand(['irm', '-f', self.logical_path])
        self.admin.assert_icommand(['iadmin', 'rum'])

        super(Test_Delay_Queue, self).tearDown()

    def no_delayed_rules(self):
        expected_str = 'No delayed rules pending for user'
        out,_,_ = self.admin.run_icommand(['iqstat'])
        return expected_str in out

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
        parameters = {}
        parameters['logical_path'] = self.logical_path
        parameters['metadata_attr'] = rule_text_key
        rule_text = rule_texts[self.plugin_name][self.class_name][rule_text_key].format(**parameters)
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
                irodsctl.restart()

                self.admin.assert_icommand(['irule', '-F', rule_file], 'STDOUT_SINGLELINE', "rule queued")

                lib.delayAssert(
                    lambda: lib.metadata_attr_with_value_exists(
                        self.admin,
                        parameters['metadata_attr'],
                        'We are about to fail...')
                )
                self.assertFalse(lib.metadata_attr_with_value_exists(
                    self.admin,
                    parameters['metadata_attr'],
                    'You should never see this line.'))

        finally:
            os.remove(rule_file)
            irodsctl.restart()

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Delete this line on resolution of #4094')
    def test_delay_queue_with_long_job(self):
        irodsctl = IrodsController()
        server_config_filename = paths.server_config_path()

        delay_job_batch_size = 5
        re_server_sleep_time = 3
        sooner_delay = 0.1
        later_delay = re_server_sleep_time * 5
        long_job_run_time = re_server_sleep_time * 10

        rule_text_key = 'test_delay_queue_with_long_job'
        parameters = {}
        parameters['delay_job_batch_size'] = delay_job_batch_size
        parameters['sooner_delay'] = sooner_delay
        parameters['later_delay'] = later_delay
        parameters['long_job_run_time'] = long_job_run_time
        parameters['logical_path'] = self.logical_path
        parameters['metadata_attr'] = rule_text_key
        parameters['metadata_value'] = rule_text_key
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
                irodsctl.restart()

                # Fire off rule and ensure the delay queue is correctly populated
                self.admin.assert_icommand(['irule', '-F', rule_file])
                self.assertEqual(
                    delay_job_batch_size * 2 + 2,
                    self.count_strings_in_queue('msiAssociateKeyValuePairsToObj'))

                # "Sooner" rules should execute
                expected_count = delay_job_batch_size
                for i in range(parameters['delay_job_batch_size']):
                    lib.delayAssert(
                        lambda: lib.metadata_attr_with_value_exists(
                            self.admin,
                            '_'.join([parameters['metadata_attr'], 'sooner']),
                            'sooner: ' + str(i))
                    )
                self.assertEqual(0, self.count_strings_in_queue('sooner:'))
                self.assertEqual(delay_job_batch_size, self.count_strings_in_queue('later:'))
                self.assertEqual(1, self.count_strings_in_queue('Sleeping...'))

                for i in range(parameters['delay_job_batch_size']):
                    self.assertFalse(lib.metadata_attr_with_value_exists(
                        self.admin,
                        '_'.join([parameters['metadata_attr'], 'later']),
                        'later: ' + str(i))
                    )

                # "Later" rules should execute... but long-running job should still be in queue
                for i in range(parameters['delay_job_batch_size']):
                    lib.delayAssert(
                        lambda: lib.metadata_attr_with_value_exists(
                            self.admin,
                            '_'.join([parameters['metadata_attr'], 'later']),
                            'later: ' + str(i))
                    )
                self.assertEqual(0, self.count_strings_in_queue('later:'))
                self.assertEqual(1, self.count_strings_in_queue('Sleeping...'))

                # Wait for long-running job to finish and make sure it has
                lib.delayAssert(
                    lambda: lib.metadata_attr_with_value_exists(
                        self.admin,
                        parameters['metadata_attr'], 'Waking!')
                )
                lib.delayAssert(self.no_delayed_rules)

        finally:
            os.remove(rule_file)
            irodsctl.restart()

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Delete this line on resolution of #4094')
    def test_batch_delay_processing__3941(self):
        irodsctl = IrodsController()
        server_config_filename = paths.server_config_path()

        expected_count = 10
        rule_text_key = 'test_batch_delay_processing__3941'
        parameters = {}
        parameters['expected_count'] = expected_count
        parameters['logical_path'] = self.logical_path
        parameters['metadata_attr'] = rule_text_key
        parameters['metadata_value'] = rule_text_key
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
            irodsctl.restart()

            # Fire off rule and ensure the delay queue is correctly populated
            self.admin.assert_icommand(['irule', '-F', rule_file])

            # Wait for messages to get written out and ensure that all the messages were written to serverLog
            for i in range(parameters['expected_count']):
                lib.delayAssert(
                    lambda: lib.metadata_attr_with_value_exists(
                        self.admin,
                        parameters['metadata_attr'],
                        '_'.join([parameters['metadata_value'], str(i)]))
                )
            lib.delayAssert(self.no_delayed_rules)

        os.remove(rule_file)

        # Bounce server to get back original settings
        irodsctl.restart()

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Delete this line on resolution of #4094')
    def test_delay_block_with_output_param__3906(self):
        irodsctl = IrodsController()
        server_config_filename = paths.server_config_path()

        rule_text_key = 'test_delay_block_with_output_param__3906'
        parameters = {}
        parameters['metadata_attr'] = rule_text_key
        parameters['metadata_value'] = 'wedidit'
        parameters['logical_path'] = self.logical_path
        rule_text = rule_texts[self.plugin_name][self.class_name][rule_text_key].format(**parameters)
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
            irodsctl.restart()

            # Fire off rule and wait for message to get written out to serverLog
            self.admin.assert_icommand(['irule', '-F', rule_file], 'STDOUT_SINGLELINE', "rule queued")
            lib.delayAssert(
                lambda: lib.metadata_attr_with_value_exists(
                    self.admin,
                    parameters['metadata_attr'], parameters['metadata_value'])
            )

        os.remove(rule_file)

        # Bounce server to get back original settings
        irodsctl.restart()

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Delete this line on resolution of #4094')
    def test_delay_queue_connection_refresh(self):
        irodsctl = IrodsController()
        config = IrodsConfig()

        rule_sleep_time = 4
        rule_text_key = 'test_delay_queue_connection_refresh'
        parameters = {}
        parameters['sleep_time'] = rule_sleep_time
        parameters['metadata_attr'] = rule_text_key
        parameters['logical_path'] = self.logical_path
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
                    irodsctl.restart()

                    # Fire off rule and ensure the delay queue is correctly populated
                    self.admin.assert_icommand(['irule', '-F', rule_file])

                    iquest = '"select META_DATA_ATTR_VALUE where META_DATA_ATTR_NAME = \'{}\'"'

                    job_names = ['job_1', 'job_2']
                    pid_map = {}
                    for job in job_names:
                        attr = '::'.join([parameters['metadata_attr'], job])
                        lib.delayAssert(lambda:
                            2 == len(self.admin.run_icommand(['iquest', '%s', iquest.format(attr)])[0].splitlines()))
                        pid_map[job] = {}
                        out,_,rc = self.admin.run_icommand(['iquest', '%s', iquest.format(attr)])
                        self.assertEqual(0, rc)
                        for value in out.splitlines():
                            s = value.split('::')
                            job_id = str(s[0])
                            pid = str(s[1])
                            pid_map[job][job_id] = pid

                    self.assertEqual(pid_map['job_1']['before'], pid_map['job_1']['after'])
                    self.assertEqual(pid_map['job_2']['before'], pid_map['job_2']['after'])
                    self.assertNotEqual(pid_map['job_1']['before'], pid_map['job_2']['before'])
                    self.assertNotEqual(pid_map['job_1']['after'], pid_map['job_2']['after'])

        finally:
            os.remove(rule_file)
            irodsctl.restart()

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Delete this line on resolution of #4094')
    def test_sigpipe_in_delay_server(self):
        irodsctl = IrodsController()

        re_server_sleep_time = 2
        longer_delay_time = re_server_sleep_time * 2
        rule_text_key = 'test_sigpipe_in_delay_server'
        parameters = {}
        parameters['longer_delay_time'] = str(longer_delay_time)
        parameters['logical_path'] = self.logical_path
        parameters['metadata_attr'] = rule_text_key
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
                irodsctl.restart()

                # Fire off rule and wait for message to get written out to serverLog
                self.admin.assert_icommand(['irule', '-F', rule_file], 'STDOUT_SINGLELINE', "rule queued")

                lib.delayAssert(
                    lambda: lib.metadata_attr_with_value_exists(
                        self.admin,
                        parameters['metadata_attr'],
                        'We are about to segfault...')
                )
                lib.delayAssert(
                    lambda: lib.metadata_attr_with_value_exists(
                        self.admin,
                        parameters['metadata_attr'],
                        'Follow-up rule executed later!')
                )
                self.assertFalse(lib.metadata_attr_with_value_exists(
                    self.admin,
                    parameters['metadata_attr'],
                    'You should never see this line.'))

        finally:
            os.remove(rule_file)
            self.admin.assert_icommand(['iqdel', '-a'])
            irodsctl.restart()

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Delete this line on resolution of #4094')
    def test_exception_in_delay_server(self):
        config = IrodsConfig()
        irodsctl = IrodsController()
        server_config_filename = paths.server_config_path()

        delay_job_sleep_time = 5
        rule_text_key = 'test_exception_in_delay_server'
        parameters = {}
        parameters['sleep_time'] = str(delay_job_sleep_time)
        parameters['longer_delay_time'] = str(delay_job_sleep_time)
        parameters['logical_path'] = self.logical_path
        parameters['metadata_attr'] = rule_text_key
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
                    irodsctl.restart()
                    with lib.file_backed_up(odbc_ini_file):
                        self.admin.assert_icommand(['irule', '-F', rule_file])
                        time.sleep(2)
                        # remove connection to database while rule is executing (should be sleeping)
                        os.unlink(odbc_ini_file)
                        time.sleep(delay_job_sleep_time * 2)

                    # Restore .odbc.ini and wait for delay server to execute rule properly
                    # The delay server should not have zombified, so the later rule should have executed
                    lib.delayAssert(
                        lambda: lib.metadata_attr_with_value_exists(
                            self.admin,
                            parameters['metadata_attr'],
                            'Follow-up rule executed later!')
                    )

        finally:
            os.remove(rule_file)
            irodsctl.restart()

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Skip for PREP and Topology Testing')
    def test_session_variables_are_not_allowed_in_delay_rules(self):
        rep_instance = 'irods_rule_engine_plugin-irods_rule_language-instance'
        rule_text = 'delay("{0}") {{ writeLine("serverLog", "$userNameClient"); }}'.format(rep_instance)
        self.admin.assert_icommand(['irule', '-r', rep_instance, rule_text, 'null', 'ruleExecOut'], 'STDERR', ['-1209000 RE_UNSUPPORTED_SESSION_VAR'])

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Skip for PREP and Topology Testing')
    def test_static_peps_can_schedule_delay_rules_without_session_variables(self):
        config = IrodsConfig()

        with lib.file_backed_up(config.server_config_path):
            # Lower the delay server's sleep time so that rules are executed quicker.
            config.server_config['advanced_settings']['rule_engine_server_sleep_time_in_seconds'] = 1
            lib.update_json_file_from_dict(config.server_config_path, config.server_config)
            IrodsController().restart()

            core_re_path = os.path.join(config.core_re_directory, 'core.re')

            with lib.file_backed_up(core_re_path):
                # Add static PEP with delay rule to the core.re file.
                with open(core_re_path, 'a') as core_re:
                    core_re.write('''
                        acPostProcForPut
                        {
                            *object_path = $objPath;

                            delay('<INST_NAME>irods_rule_engine_plugin-irods_rule_language-instance</INST_NAME>') {
                                *kvp.'delay_rule_fired' = 'YES';
                                msiSetKeyValuePairsToObj(*kvp, *object_path, "-d");
                            }
                        }
                    ''')

                filename = os.path.join(self.admin.local_session_dir, 'delay_indirectly.txt')
                lib.make_file(filename, 1, 'arbitrary')

                # Trigger the static PEP and verify that the metadata was added.
                self.admin.assert_icommand(['iput', filename])

                def assert_metadata_exists():
                    out, _, ec = self.admin.run_icommand(['imeta', 'ls', '-d', os.path.basename(filename)])
                    return ec == 0 and 'attribute: delay_rule_fired' in out and 'value: YES' in out

                lib.delayAssert(lambda: assert_metadata_exists)

        # Restore the server's configuration settings.
        IrodsController().restart()

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Skip for PREP and Topology Testing')
    def test_session_variables_can_be_used_in_delay_rules_indirectly(self):
        config = IrodsConfig()

        with lib.file_backed_up(config.server_config_path):
            # Lower the delay server's sleep time so that rules are executed quicker.
            config.server_config['advanced_settings']['rule_engine_server_sleep_time_in_seconds'] = 1
            lib.update_json_file_from_dict(config.server_config_path, config.server_config)
            IrodsController().restart()

            core_re_path = os.path.join(config.core_re_directory, 'core.re')

            with lib.file_backed_up(core_re_path):
                # Add static PEP and indirect delay rule to the core.re file.
                with open(core_re_path, 'a') as core_re:
                    core_re.write('''
                        schedule_delay_rule(*object_path)
                        {
                            delay('<INST_NAME>irods_rule_engine_plugin-irods_rule_language-instance</INST_NAME>') {
                                *kvp.'delay_rule_fired' = 'YES';
                                msiSetKeyValuePairsToObj(*kvp, *object_path, "-d");
                            }
                        }

                        acPostProcForPut
                        {
                            schedule_delay_rule($objPath);
                        }
                    ''')

                filename = os.path.join(self.admin.local_session_dir, 'delay_indirectly.txt')
                lib.make_file(filename, 1, 'arbitrary')

                # Trigger the static PEP and verify that the metadata was added.
                self.admin.assert_icommand(['iput', filename])

                def assert_metadata_exists():
                    out, _, ec = self.admin.run_icommand(['imeta', 'ls', '-d', os.path.basename(filename)])
                    return ec == 0 and 'attribute: delay_rule_fired' in out and 'value: YES' in out

                lib.delayAssert(lambda: assert_metadata_exists)

        # Restore the server's configuration settings.
        IrodsController().restart()


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
                irodsctl.restart()

                string = 'writeLine: inString = {}'.format(repeat_string)
                initial_size_of_server_log = lib.get_file_size_by_path(paths.server_log_path())
                self.admin.assert_icommand(['irule', '-F', rule_file])

                for i in range(repeat_n):
                    self.assert_repeat_in_log(string, initial_size_of_server_log, i + 1)

        finally:
            os.remove(rule_file)
            irodsctl.restart()

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
                irodsctl.restart()

                string = 'writeLine: inString = {}'.format(repeat_string)
                initial_size_of_server_log = lib.get_file_size_by_path(paths.server_log_path())
                self.admin.assert_icommand(['irule', '-F', rule_file])
                for i in range(repeat_n):
                    self.assert_repeat_in_log(string, initial_size_of_server_log, i + 1)

        finally:
            os.remove(rule_file)
            irodsctl.restart()

    #def test_double_until_time(self):
    #def test_double_until_success_or_until_time(self):
    #def test_double_until_success_or_n_times(self):
    #def test_double_until_success_upto_time(self):

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Skip for PREP and Topology Testing')
    def test_delay_rule_created_by_rodsuser_does_not_cause_error_message_to_appear_in_log__issue_5257(self):
        config = IrodsConfig()
        log_offset = 0
        delay_msg = 'Printed by test for issue 5257.'

        with lib.file_backed_up(config.server_config_path):
            # Lower the delay server's sleep time so that rules are executed quicker.
            config.server_config['advanced_settings']['rule_engine_server_sleep_time_in_seconds'] = 1
            lib.update_json_file_from_dict(config.server_config_path, config.server_config)
            IrodsController().restart()

            rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
            rule_text = '''
                delay("<EF>1s</EF><INST_NAME>{0}</INST_NAME>") {{
                    writeLine("serverLog", "{1}");
                }}
            '''.format(rep_name, delay_msg)

            # Capture the current log file size. This will serve as an offset for when we inspect
            # the log file for messages.
            log_offset = lib.get_file_size_by_path(paths.server_log_path())

            # Schedule the delay rule.
            self.user0.assert_icommand(['irule', '-r', rep_name, rule_text, 'null', 'ruleExecOut'])

        IrodsController().restart()

        # Verify that the log file contains the correct results.
        api_perm_msg = 'rsApiHandler: User has no permission for apiNumber 708'
        lib.delayAssert(lambda: lib.log_message_occurrences_equals_count(msg=api_perm_msg, count=0, start_index=log_offset))
        lib.delayAssert(lambda: lib.log_message_occurrences_greater_than_count(msg=delay_msg, count=0, start_index=log_offset))

