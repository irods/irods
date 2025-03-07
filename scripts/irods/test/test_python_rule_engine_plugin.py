import os
import sys
import tempfile
import time
import unittest
from textwrap import dedent

from . import session
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


class Test_Python_Rule_Engine_Plugin(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):
    plugin_name = IrodsConfig().default_rule_engine_plugin
    class_name = 'Test_Python_Rule_Engine_Plugin'

    def setUp(self):
        super(Test_Python_Rule_Engine_Plugin, self).setUp()
        self.admin = self.admin_sessions[0]
        self.user = self.user_sessions[0]

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
                self.user.assert_icommand(
                    [ 'irule','-r', 'irods_rule_engine_plugin-irods_rule_language-instance',
                      '''writeLine("stdout", error("SYS_INVALID_FILE_PATH") == errorcode(myrule()))''',
                      'null', 'ruleExecOut' ],
                    'STDOUT_SINGLELINE',['true'] )

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-python', 'only applicable for python REP')
    def test_REPF_does_not_deallocate_KeyValPair__issue_8265(self):
        # Create a rule file which attempts to add an AVU to the session collection.
        avu_name = 'issue_8265_attr_name'
        avu_value = 'issue_8265_attr_value'
        rule_file = os.path.join(self.admin.local_session_dir, 'issue_8265.r')
        with open(rule_file, 'w') as f:
            f.write(dedent(
                f"""
                import irods_types
                def main(rule_args, callback, rei):
                    collection = '{self.admin.session_collection}'
                    avu = '{avu_name}={avu_value}'
                    res = callback.msiString2KeyValPair(avu, irods_types.BytesBuf())
                    callback.msiSetKeyValuePairsToObj(res['arguments'][1], collection, '-C')

                INPUT null
                OUTPUT ruleExecOut
                """))

        # Run the rule and show that it added the expected AVU.
        self.admin.assert_icommand(['irule', '-r', 'irods_rule_engine_plugin-python-instance', '-F', rule_file])
        self.admin.assert_icommand(['imeta', 'ls', '-C', self.admin.session_collection], 'STDOUT', [
            f'attribute: {avu_name}\n',
            f'value: {avu_value}\n'
        ])
