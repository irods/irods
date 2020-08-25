from __future__ import print_function
import sys
if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import time
import tempfile
from textwrap import dedent

from . import resource_suite
from .. import paths
from .. import lib
from ..configuration import IrodsConfig
from ..core_file import ( temporary_core_file,
                          IRODS_RULE_LANGUAGE_RULE_ENGINE_PLUGIN_NAME,
                          PYTHON_RULE_ENGINE_PLUGIN_NAME )
from ..controller import IrodsController
import contextlib


@contextlib.contextmanager
def append_native_re_to_server_config (with_backup=False):
    irods_config = IrodsConfig()
    orig = irods_config.server_config['plugin_configuration']['rule_engines']

    try:
        IrodsController().stop()
        irods_config.server_config['plugin_configuration']['rule_engines'] = orig + [{
                "instance_name": "irods_rule_engine_plugin-irods_rule_language-instance",
                "plugin_name": "irods_rule_engine_plugin-irods_rule_language",
                "plugin_specific_configuration": {
                    "re_data_variable_mapping_set": [
                        "core"
                    ],
                    "re_function_name_mapping_set": [
                        "core"
                    ],
                    "re_rulebase_set": [
                        "core"
                    ],
                    "regexes_for_supported_peps": [
                        "ac[^ ]*",
                        "msi[^ ]*",
                        "[^ ]*pep_[^ ]*_(pre|post|except|finally)"
                    ]
                },
                "shared_memory_instance": "irods_rule_language_rule_engine"
            }]
        irods_config.commit(irods_config.server_config, irods_config.server_config_path, make_backup=with_backup)
        IrodsController().start()
        yield
    finally:
        IrodsController().stop()
        irods_config.server_config['plugin_configuration']['rule_engines'] = orig
        irods_config.commit(irods_config.server_config, irods_config.server_config_path, make_backup=with_backup)
        IrodsController().start()


class Test_Python_Rule_Engine_Plugin(resource_suite.ResourceBase, unittest.TestCase):
    plugin_name = IrodsConfig().default_rule_engine_plugin
    class_name = 'Test_Python_Rule_Engine_Plugin'

    def setUp(self):
        super(Test_Python_Rule_Engine_Plugin, self).setUp()

    def tearDown(self):
        super(Test_Python_Rule_Engine_Plugin, self).tearDown()

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-python', 'only applicable for python REP')
    def test_python_continuation_to_native_58 (self):
        with append_native_re_to_server_config():
            with temporary_core_file(plugin_name=IRODS_RULE_LANGUAGE_RULE_ENGINE_PLUGIN_NAME) as core_re,\
                 temporary_core_file(plugin_name=PYTHON_RULE_ENGINE_PLUGIN_NAME) as core_py:
                core_py.add_rule(dedent('''
                def pep_api_data_obj_put_pre(rule_args,callback,rei):
                    import irods_errors
                    callback.writeLine("serverLog", ":python PEP:")
                    return irods_errors.RULE_ENGINE_CONTINUE
                '''))
                core_re.add_rule(dedent('''
                pep_api_data_obj_put_pre(*A,*B,*C,*D,*E) {
                    writeLine("serverLog", ":native PEP:")
                }
                '''))
                time.sleep(5);
                initial_size_of_server_log = lib.get_file_size_by_path(paths.server_log_path())
                tmpf = tempfile.NamedTemporaryFile()
                self.admin.assert_icommand( ['iput',tmpf.name] )
                lib.delayAssert(
                    lambda: lib.log_message_occurrences_equals_count(
                        msg=':native PEP:',
                        count=1,
                        start_index=initial_size_of_server_log),maxrep=5)

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-python', 'only applicable for python REP')
    def test_python_error_to_native_58 (self):
        with append_native_re_to_server_config():
            with temporary_core_file(plugin_name=PYTHON_RULE_ENGINE_PLUGIN_NAME) as core_py:
                core_py.add_rule(dedent('''
                import irods_errors
                def myrule(*x):
                    return irods_errors.SYS_INVALID_FILE_PATH
                '''))
                self.user0.assert_icommand(
                    [ 'irule','-r', 'irods_rule_engine_plugin-irods_rule_language-instance',
                      '''writeLine("stdout", error("SYS_INVALID_FILE_PATH") == errorcode(myrule()))''',
                      'null', 'ruleExecOut' ],
                    'STDOUT_SINGLELINE',['true'] )
