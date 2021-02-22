from __future__ import print_function
import os
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from .. import test
from .. import lib
from .. import paths
from ..configuration import IrodsConfig
from textwrap import dedent

class Test_Dynamic_PEPs(session.make_sessions_mixin([('otherrods', 'rods')], []), unittest.TestCase):

    plugin_name = IrodsConfig().default_rule_engine_plugin

    def setUp(self):
        super(Test_Dynamic_PEPs, self).setUp()
        self.admin = self.admin_sessions[0]

    def tearDown(self):
        super(Test_Dynamic_PEPs, self).tearDown()

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python' or test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_if_collInp_t_is_exposed__issue_4370(self):
        config = IrodsConfig()

        with lib.file_backed_up(config.server_config_path):
            core_re_path = os.path.join(config.core_re_directory, 'core.re')

            with lib.file_backed_up(core_re_path):
                prefix = "i4370_PATH => "

                with open(core_re_path, 'a') as core_re:
                    core_re.write('pep_api_coll_create_pre(*a, *b, *coll_input) { writeLine("serverLog", "' + prefix + '" ++ *coll_input.coll_name); }\n')

                coll_path = os.path.join(self.admin.session_collection, "i4370_test_collection")
                self.admin.assert_icommand(['imkdir', coll_path])
                lib.delayAssert(lambda: lib.log_message_occurrences_equals_count(msg=prefix + coll_path))

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python' or test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_finally_peps_are_supported__issue_4773(self):
        config = IrodsConfig()
        core_re_path = os.path.join(config.core_re_directory, 'core.re')

        with lib.file_backed_up(core_re_path):
            msg = 'FINALLY PEPS SUPPORTED!!!'

            with open(core_re_path, 'a') as core_re:
                core_re.write('''
                    pep_api_data_obj_put_finally(*INSTANCE_NAME, *COMM, *DATAOBJINP, *BUFFER, *PORTAL_OPR_OUT) {{
                        writeLine("serverLog", "{0}");
                    }}
                '''.format(msg))

            filename = os.path.join(self.admin.local_session_dir, 'finally_peps.txt')
            lib.make_file(filename, 1, 'arbitrary')

            # Check log for message written by the finally PEP.
            log_offset = lib.get_file_size_by_path(paths.server_log_path())
            self.admin.assert_icommand(['iput', filename])
            lib.delayAssert(lambda: lib.log_message_occurrences_greater_than_count(msg=msg, count=0, start_index=log_offset))

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python' or test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_openedDataObjInp_t_serializer__issue_5408(self):
        config = IrodsConfig()

        with lib.file_backed_up(config.server_config_path):
            core_re_path = os.path.join(config.core_re_directory, 'core.re')

            with lib.file_backed_up(core_re_path):
                with open(core_re_path, 'a') as core_re:
                    attribute = ':'.join(['test_openedDataObjInp_t_serializer__issue_5408', 'pep_api_data_obj_write_post'])
                    filename = 'test_openedDataObjInp_t_serializer__issue_5408.txt'
                    logical_path = os.path.join(self.admin.session_collection, filename)
                    core_re.write(dedent('''
                        pep_api_data_obj_write_post(*INSTANCE_NAME, *COMM, *DATAOBJWRITEINP, *BUF)
                        {{
                            *my_l1descInx = *DATAOBJWRITEINP.l1descInx;

                            msiAddKeyVal(*key_val_pair,"{attribute}","the_l1descInx=[*my_l1descInx]");
                            msiAssociateKeyValuePairsToObj(*key_val_pair,"{logical_path}","-d");
                        }}'''.format(**locals())))

                try:
                    self.admin.assert_icommand(['istream', 'write', logical_path], input=filename)

                    expected_value = 'the_l1descInx=[{}]'.format(3)
                    lib.delayAssert(
                        lambda: lib.metadata_attr_with_value_exists(self.admin, attribute, expected_value),
                        maxrep=10
                    )

                finally:
                    self.admin.assert_icommand(['irm', '-f', logical_path])
                    self.admin.assert_icommand(['iadmin', 'rum'])

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python' or test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_bytesBuf_t_serializer__issue_5408(self):
        config = IrodsConfig()

        with lib.file_backed_up(config.server_config_path):
            core_re_path = os.path.join(config.core_re_directory, 'core.re')

            with lib.file_backed_up(core_re_path):
                with open(core_re_path, 'a') as core_re:
                    attribute = ':'.join(['test_bytesBuf_t_serializer__issue_5408', 'pep_api_replica_close_post'])
                    filename = 'test_bytesBuf_t_serializer__issue_5408.txt'
                    logical_path = os.path.join(self.admin.session_collection, filename)
                    core_re.write(dedent('''
                        pep_api_replica_close_post(*INSTANCE_NAME, *COMM, *BUF)
                        {{
                            *my_len = *BUF.len
                            *my_buf = *BUF.buf

                            msiAddKeyVal(*key_val_pair,"{attribute}","the_len=[*my_len],the_buf=[*my_buf]");
                            msiAssociateKeyValuePairsToObj(*key_val_pair,"{logical_path}","-d");
                        }}'''.format(**locals())))

                try:
                    self.admin.assert_icommand(['itouch', logical_path])

                    expected_buf = r'{"fd":3}'
                    expected_len = len(expected_buf)
                    expected_value = 'the_len=[{0}],the_buf=[{1}]'.format(expected_len, expected_buf)
                    lib.delayAssert(
                        lambda: lib.metadata_attr_with_value_exists(self.admin, attribute, expected_value),
                        maxrep=10
                    )

                finally:
                    self.admin.assert_icommand(['irm', '-f', logical_path])
                    self.admin.assert_icommand(['iadmin', 'rum'])


