from __future__ import print_function
import os
import sys
import textwrap

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from .. import lib
from .. import paths
from .. import test
from ..configuration import IrodsConfig
from ..core_file import temporary_core_file
from textwrap import dedent

class Test_Dynamic_PEPs(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):

    plugin_name = IrodsConfig().default_rule_engine_plugin

    def setUp(self):
        super(Test_Dynamic_PEPs, self).setUp()
        self.admin = self.admin_sessions[0]
        self.user = self.user_sessions[0]

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
                    self.admin.assert_icommand(['istream', 'write', logical_path], input='test data')

                    # We must hard-code the value of "expected_len" because null bytes are encoded as
                    # hexidecimal in the bytes buffer. If we tried to use the len() function to capture
                    # the length of "expected_buf", it would produce the wrong value. Because we are
                    # using rc_replica_close for this test, we know that the JSON passed to the API will
                    # have a length of 9 bytes. However, the metadata attribute value set on "logical_path"
                    # will have a length of 13 due to serialization.
                    expected_buf = r'{"fd":3}\x00'
                    expected_len = 9
                    expected_value = 'the_len=[{0}],the_buf=[{1}]'.format(expected_len, expected_buf)

                    lib.delayAssert(
                        lambda: lib.metadata_attr_with_value_exists(self.admin, attribute, expected_value),
                        maxrep=10
                    )

                finally:
                    self.admin.assert_icommand(['irm', '-f', logical_path])
                    self.admin.assert_icommand(['iadmin', 'rum'])

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python' or test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_data_obj_info_parameters_in_pep_database_reg_data_obj_post__issue_5554(self):
        pep_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                pep_database_reg_data_obj_post(*INSTANCE, *CONTEXT, *OUT, *DATA_OBJ_INFO) {{
                    *logical_path = *DATA_OBJ_INFO.logical_path;
                    *create_time = *DATA_OBJ_INFO.data_create;
                    *owner_name = *DATA_OBJ_INFO.data_owner_name;
                    *owner_zone = *DATA_OBJ_INFO.data_owner_zone;

                    msiAddKeyVal(*key_val_pair,'{attribute}::{path_attr}',         '*logical_path');
                    msiAddKeyVal(*key_val_pair,'{attribute}::{create_time_attr}',  '*create_time');
                    msiAddKeyVal(*key_val_pair,'{attribute}::{owner_name_attr}',   '*owner_name');
                    msiAddKeyVal(*key_val_pair,'{attribute}::{owner_zone_attr}',   '*owner_zone');

                    msiAssociateKeyValuePairsToObj(*key_val_pair,'{resource}','-R');
                }}
            ''')
        }

        name = 'test_data_obj_info_parameters_in_pep_database_reg_data_obj_post__issue_5554'
        resource = 'metadata_attr_resource'
        config = IrodsConfig()

        with lib.file_backed_up(config.server_config_path):
            core_re_path = os.path.join(config.core_re_directory, 'core.re')

            with lib.file_backed_up(core_re_path):
                parameters = {}
                parameters['attribute'] = name
                parameters['path_attr'] = 'path_attr'
                parameters['create_time_attr'] = 'create_time_attr'
                parameters['owner_name_attr'] = 'owner_name_attr'
                parameters['owner_zone_attr'] = 'owner_zone_attr'
                parameters['resource'] = resource

                with open(core_re_path, 'a') as core_re:
                    core_re.write(pep_map[self.plugin_name].format(**parameters))

                local_file = lib.create_local_testfile(os.path.join(self.admin.local_session_dir, name))
                logical_path = os.path.join(self.admin.session_collection, name)

                try:
                    self.admin.assert_icommand(['iadmin', 'mkresc', resource, 'passthru'], 'STDOUT', resource)

                    self.admin.assert_icommand(['ireg', local_file, logical_path])

                    create_time = self.admin.run_icommand(['iquest', '%s',
                        '''"select DATA_CREATE_TIME where COLL_NAME = '{0}' and DATA_NAME = '{1}'"'''.format(
                        os.path.dirname(logical_path), os.path.basename(logical_path))])[0]

                    lib.metadata_attr_with_value_exists(self.admin,
                                                        '::'.join([parameters['attribute'], parameters['path_attr']]),
                                                        logical_path),

                    lib.metadata_attr_with_value_exists(self.admin,
                                                        '::'.join([parameters['attribute'], parameters['create_time_attr']]),
                                                        create_time),

                    lib.metadata_attr_with_value_exists(self.admin,
                                                        '::'.join([parameters['attribute'], parameters['owner_name_attr']]),
                                                        self.admin.username),

                    lib.metadata_attr_with_value_exists(self.admin,
                                                        '::'.join([parameters['attribute'], parameters['owner_zone_attr']]),
                                                        self.admin.zone_name),

                finally:
                    self.admin.assert_icommand(['iadmin', 'rmresc', resource])
                    self.admin.assert_icommand(['irm', '-f', logical_path])
                    self.admin.assert_icommand(['iadmin', 'rum'])


    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'Only implemented for NREP.')
    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER , 'Database PEPs only fire on the catalog provider.')
    def test_pep_database_data_object_finalize__issue_6385(self):
        other_resource = 'test_pep_database_data_object_finalize_resc'
        filename = 'test_pep_database_data_object_finalize__issue_6385'
        contents = 'knock knock'
        logical_path = os.path.join(self.user.session_collection, filename)
        expected_attribute = 'latest_replica'

        pep_map = {
            'irods_rule_engine_plugin-irods_rule_language': dedent('''
                pep_database_data_object_finalize_post(*instance, *ctx, *out, *replicas) {{
                    # This PEP annotates an AVU to the target data object indicating the most recently created replica.

                    # Get a JSON handle so we can work with the replicas input.
                    msiStrlen(*replicas, *size);
                    msi_json_parse(*replicas, int(*size), *handle);

                    # Get the logical path based on the data ID. (Seems silly that we have to do this.)
                    msi_json_value(*handle, "/replicas/0/before/data_id", *data_id);
                    foreach (*result in select COLL_NAME, DATA_NAME where DATA_ID = '*data_id') {{
                        *logical_path = *result.COLL_NAME ++ '/' ++ *result.DATA_NAME;
                        break;
                    }}

                    # Loop over the replicas and find the one which was just created based on "before" replica status.
                    # We want to get the replica number so we can annotate it as metadata on the data object.
                    *replica_number = "null";

                    # *replicas is a pure array. Pass an empty string for the JSON pointer to get the size of the array.
                    msi_json_size(*handle, "/replicas", *array_len);
                    for (*i = 0; *i < *array_len; *i = *i + 1) {{
                        msi_json_value(*handle, "/replicas/*i/before/data_is_dirty", *status_before);
                        # '2' is the intermediate replica status.
                        if (*status_before == '2') {{
                            msi_json_value(*handle, "/replicas/*i/after/data_repl_num", *replica_number);
                            break;
                        }}
                    }}

                    # Free the JSON handle because we don't need it anymore.
                    msi_json_free(*handle);

                    # If replica_number is "null", then no replica was found with replica status of intermediate in its
                    # "before" object. This database operation is invoked both when locking the data object on open and
                    # when finalizing the data object on close. In that case, the new replica would not have been
                    # created yet, so it will not be found in the list of replicas above. We only want to annotate
                    # metadata when the object was newly created (i.e. its "before" replica status is intermediate) and
                    # is being closed. Objects opened for appending or overwriting will not have a "before" replica
                    # status of intermediate.
                    if (*replica_number != "null") {{
                        # Annotate the metadata indicating the replica number which was just updated.
                        msiAddKeyVal(*kvp, "{}", "*replica_number");
                        msiSetKeyValuePairsToObj(*kvp, "*logical_path", "-d");
                    }}

                }}''')
        }

        lib.create_ufs_resource(other_resource, self.admin, hostname=test.settings.HOSTNAME_2)

        try:
            putfile = os.path.join(self.user.local_session_dir, 'putme')
            lib.touch(putfile)

            with temporary_core_file() as core:
                core.add_rule(pep_map[self.plugin_name].format(expected_attribute))

                # Create the first replica with iput. There should be only one replica, it should be good, and it
                # should annotate metadata indicating that replica 0 as the latest replica.
                self.user.assert_icommand(['iput', putfile, logical_path])
                self.assertEqual(str(1), lib.get_replica_status(self.user, os.path.basename(logical_path), 0))
                self.assertTrue(lib.metadata_attr_with_value_exists(self.user, expected_attribute, str(0)))

                # Create a second replica with istream. There should now be two replicas, one should be good and one
                # should be stale, and the annotated metadata should indicate that replica 1 is the latest replica.
                self.user.assert_icommand(['istream', '-R', other_resource, 'write', logical_path], input=contents)
                self.assertEqual(str(0), lib.get_replica_status(self.user, os.path.basename(logical_path), 0))
                self.assertEqual(str(1), lib.get_replica_status(self.user, os.path.basename(logical_path), 1))
                self.assertTrue(lib.metadata_attr_with_value_exists(self.user, expected_attribute, str(1)))
                self.assertFalse(lib.metadata_attr_with_value_exists(self.user, expected_attribute, str(0)))

                # Overpave the stale replica with irepl. There should still be two replicas, both should be good, and
                # the annotated metadata should still indicate that replica 1 is the latest replica.
                self.user.assert_icommand(['irepl', logical_path])
                self.assertEqual(str(1), lib.get_replica_status(self.user, os.path.basename(logical_path), 0))
                self.assertEqual(str(1), lib.get_replica_status(self.user, os.path.basename(logical_path), 1))
                self.assertTrue(lib.metadata_attr_with_value_exists(self.user, expected_attribute, str(1)))
                self.assertFalse(lib.metadata_attr_with_value_exists(self.user, expected_attribute, str(0)))

        finally:
            print(self.user.run_icommand(['ils', '-AL', os.path.dirname(logical_path)])[0]) # Debugging
            print(self.user.run_icommand(['imeta', 'ls', '-d', logical_path])[0]) # Debugging

            for replica_number in [0, 1]:
                self.admin.run_icommand([
                    'iadmin', 'modrepl',
                    'logical_path', logical_path,
                    'replica_number', str(replica_number),
                    'DATA_REPL_STATUS', str(0)])

            self.user.run_icommand(['irm', '-f', logical_path])
            lib.remove_resource(other_resource, self.admin)
