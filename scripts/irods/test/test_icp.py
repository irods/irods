from __future__ import print_function
import inspect
import os
import shutil
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import ustrings
from .. import lib
from . import session
from .. import core_file
from .rule_texts_for_tests import rule_texts
from ..configuration import IrodsConfig

class Test_Icp(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):
    plugin_name = IrodsConfig().default_rule_engine_plugin
    class_name = 'Test_Icp'
    def setUp(self):
        super(Test_Icp, self).setUp()
        self.admin = self.admin_sessions[0]
        self.alice = self.user_sessions[0]

    def tearDown(self):
        super(Test_Icp, self).tearDown()

    def test_icp_closes_file_descriptors__4862(self):
        test_dir_path = 'test_icp_closes_file_descriptors__4862'
        logical_put_path = 'iput_large_dir'
        logical_cp_path = 'icp_large_dir'
        try:
            with lib.file_backed_up(self.alice._environment_file_path):
                del self.alice.environment_file_contents['irods_default_resource']
                filepath = core_file.CoreFile(self.plugin_name).filepath
                with lib.file_backed_up(filepath):
                    os.unlink(filepath)
                    with open(filepath, 'w') as f:
                        f.write(rule_texts[self.plugin_name][self.class_name][inspect.currentframe().f_code.co_name])

                    lib.make_large_local_tmp_dir(test_dir_path, 1024, 1024)

                    self.alice.assert_icommand(['iput', os.path.join(test_dir_path, 'junk0001')], 'STDERR', 'SYS_INVALID_RESC_INPUT')

                    self.alice.assert_icommand(['iput', '-R', 'demoResc', '-r', test_dir_path, logical_put_path], 'STDOUT', ustrings.recurse_ok_string())
                    _,_,err = self.alice.assert_icommand(['icp', '-r', logical_put_path, logical_cp_path], 'STDERR', 'SYS_INVALID_RESC_INPUT')
                    self.assertNotIn('SYS_OUT_OF_FILE_DESC', err, 'SYS_OUT_OF_FILE_DESC found in output.')
        finally:
            shutil.rmtree(test_dir_path, ignore_errors=True)

