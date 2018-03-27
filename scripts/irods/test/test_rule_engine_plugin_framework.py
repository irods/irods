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
import time  # remove once file hash fix is commited #2279
import subprocess

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

def delayAssert(a, interval=1, maxrep=100):
    for i in range(maxrep):
        time.sleep(interval)  # wait for test to fire
        if a():
            break
    assert a()



class Test_Plugin_Instance_Delay(ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_Plugin_Instance_Delay, self).setUp()

    def tearDown(self):
        super(Test_Plugin_Instance_Delay, self).tearDown()

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

            delayAssert(lambda: lib.count_occurrences_of_string_in_log(paths.server_log_path(), 'Test_Plugin_Instance_Delay', start_index=initial_log_size))












