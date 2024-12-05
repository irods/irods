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
from ..controller import IrodsController
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

            try:
                with lib.file_backed_up(core_re_path):
                    prefix = "i4370_PATH => "

                    with open(core_re_path, 'a') as core_re:
                        core_re.write('pep_api_coll_create_pre(*a, *b, *coll_input) { writeLine("serverLog", "' + prefix + '" ++ *coll_input.coll_name); }\n')
                    IrodsController(config).reload_configuration()

                    coll_path = os.path.join(self.admin.session_collection, "i4370_test_collection")
                    self.admin.assert_icommand(['imkdir', coll_path])
                    lib.delayAssert(lambda: lib.log_message_occurrences_equals_count(msg=prefix + coll_path))

            finally:
                IrodsController(config).reload_configuration()

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python' or test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_finally_peps_are_supported__issue_4773(self):
        config = IrodsConfig()
        core_re_path = os.path.join(config.core_re_directory, 'core.re')

        with lib.file_backed_up(core_re_path):
            msg = 'FINALLY PEPS SUPPORTED!!!'

            try:
                with open(core_re_path, 'a') as core_re:
                    core_re.write('''
                        pep_api_data_obj_put_finally(*INSTANCE_NAME, *COMM, *DATAOBJINP, *BUFFER, *PORTAL_OPR_OUT) {{
                            writeLine("serverLog", "{0}");
                        }}
                    '''.format(msg))
                IrodsController(config).reload_configuration()

                filename = os.path.join(self.admin.local_session_dir, 'finally_peps.txt')
                lib.make_file(filename, 1, 'arbitrary')

                # Check log for message written by the finally PEP.
                log_offset = lib.get_file_size_by_path(paths.server_log_path())
                self.admin.assert_icommand(['iput', filename])
                lib.delayAssert(lambda: lib.log_message_occurrences_greater_than_count(msg=msg, count=0, start_index=log_offset))

            finally:
                IrodsController(config).reload_configuration()

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python' or test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_openedDataObjInp_t_serializer__issue_5408(self):
        config = IrodsConfig()

        with lib.file_backed_up(config.server_config_path):
            core_re_path = os.path.join(config.core_re_directory, 'core.re')

            try:
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
                    IrodsController(config).reload_configuration()

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

            finally:
                IrodsController(config).reload_configuration()

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python' or test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_bytesBuf_t_serializer__issue_5408(self):
        config = IrodsConfig()

        with lib.file_backed_up(config.server_config_path):
            core_re_path = os.path.join(config.core_re_directory, 'core.re')

            try:
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
                    IrodsController(config).reload_configuration()

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

            finally:
                IrodsController(config).reload_configuration()

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

            try:
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
                    IrodsController(config).reload_configuration()

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

            finally:
                IrodsController(config).reload_configuration()

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

        lib.create_ufs_resource(self.admin, other_resource, hostname=test.settings.HOSTNAME_2)

        try:
            putfile = os.path.join(self.user.local_session_dir, 'putme')
            lib.touch(putfile)

            with temporary_core_file() as core:
                core.add_rule(pep_map[self.plugin_name].format(expected_attribute))
                IrodsController().reload_configuration()

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
            lib.remove_resource(self.admin, other_resource)
            IrodsController().reload_configuration()

    @unittest.skipIf(plugin_name != 'irods_rule_engine_plugin-irods_rule_language' or test.settings.RUN_IN_TOPOLOGY, "Requires NREP and single node zone.")
    def test_if_const_vector_of_strings_is_exposed__issue_7570(self):
        with temporary_core_file() as core:
            prefix = 'test_if_const_vector_of_strings_is_exposed__issue_7570'
            attr_name_1 = f'{prefix}_iquery_input_value'
            attr_value_1 = f'{prefix}_vector_of_strings'
            attr_name_2 = f'{prefix}_vector_of_strings_size'

            # Add a rule to core.re which when triggered will add the vector size and elements
            # of "*values" to the session collection as AVUs.
            core.add_rule(dedent('''
            pep_database_execute_genquery2_sql_pre(*inst, *ctx, *out, *sql, *values, *output) {{
                msiModAVUMetadata('-C', '{self.user.session_collection}', 'add', '{attr_name_1}', *values."0", '');
                msiModAVUMetadata('-C', '{self.user.session_collection}', 'add', '{attr_name_2}', *values."size", '');
            }}
            '''.format(**locals())))
            IrodsController().reload_configuration()

            try:
                # Trigger PEP, ignore output.
                self.user.assert_icommand(['iquery', f"select COLL_NAME where COLL_NAME = '{attr_value_1}'"], 'STDOUT')

                # Show the session collection has an AVU attached to it.
                # This is only possible if vectors of strings are serializable.
                self.user.assert_icommand(['imeta', 'ls', '-C', self.user.session_collection], 'STDOUT', [
                    f'attribute: {attr_name_1}\nvalue: {attr_value_1}\n',
                    # The value is 2 due to the parser adding additional bind variables.
                    # For this specific case, a second bind variable for the username is included.
                    f'attribute: {attr_name_2}\nvalue: 2\n'
                ])

            finally:
                # This isn't required, but we do it here to keep the database metadata
                # tables small, for performance reasons.
                self.user.run_icommand(['imeta', 'rm', '-C', self.user.session_collection, attr_name_1, attr_value_1])
                self.user.run_icommand(['imeta', 'rm', '-C', self.user.session_collection, attr_name_2, '2'])
                IrodsController().reload_configuration()

    @unittest.skipIf(plugin_name != 'irods_rule_engine_plugin-irods_rule_language' or test.settings.RUN_IN_TOPOLOGY, 'Requires NREP and single node zone.')
    def test_if_vector_of_strings_is_exposed__issue_7570(self):
        config = IrodsConfig()

        with lib.file_backed_up(config.server_config_path):
            try:
                # Lower the delay server's sleep time so that rule are executed quicker.
                config.server_config['advanced_settings']['delay_server_sleep_time_in_seconds'] = 1
                lib.update_json_file_from_dict(config.server_config_path, config.server_config)

                with temporary_core_file() as core:
                    prefix = 'test_if_vector_of_strings_is_exposed__issue_7570'
                    attr_name_1 = f'{prefix}_delay_rule_info_element_value'
                    attr_name_2 = f'{prefix}_vector_of_strings_size'

                    # Add a rule to core.re which when triggered will add the vector size and elements
                    # of "*delay_rule_info" to "self.admin's" session collection as AVUs.
                    core.add_rule(dedent('''
                    pep_database_get_delay_rule_info_post(*inst, *ctx, *out, *rule_id, *delay_rule_info) {{
                        *trimmed_value = trimr(triml(*delay_rule_info."0", ' '), ' '); # Remove leading/trailing whitespace.
                        msiModAVUMetadata('-C', '{self.admin.session_collection}', 'add', '{attr_name_1}', *trimmed_value, '');
                        msiModAVUMetadata('-C', '{self.admin.session_collection}', 'add', '{attr_name_2}', *delay_rule_info."size", '');
                    }}
                    '''.format(**locals())))
                    IrodsController().reload_configuration()

                    # Give the service account user permission to add metadata to "self.admin's" session collection.
                    # Remember, "self.admin" represents a completely different rodsadmin AND the PEP that was added
                    # to core.re is only triggered by the service account user.
                    self.admin.assert_icommand(
                        ['ichmod', 'modify_object', config.client_environment['irods_user_name'], self.admin.session_collection])

                    # Placed here for clean-up purposes.
                    rep_name = self.plugin_name + '-instance'
                    delay_rule_body = 'writeLine("serverLog", "test_if_vector_of_strings_is_exposed__issue_7570");'
                    rule = f'delay("<INST_NAME>{rep_name}</INST_NAME>") {{ {delay_rule_body} }}'

                    try:
                        # Schedule a delay rule to trigger the database PEP.
                        self.admin.assert_icommand(['irule', '-r', rep_name, rule, 'null', 'null'])

                        # Show the session collection has an AVU attached to it.
                        # This is only possible if vectors of strings are serializable.

                        def has_avu():
                            out, _, _ = self.admin.run_icommand(['imeta', 'ls', '-C', self.admin.session_collection])
                            print(out) # For debugging purposes.
                            return all(avu in out for avu in [
                                f'attribute: {attr_name_1}\nvalue: {delay_rule_body}\n',
                                f'attribute: {attr_name_2}\nvalue: 12\n'
                            ])

                        lib.delayAssert(lambda: has_avu())

                    finally:
                        # This isn't required, but we do it here to keep the database metadata
                        # tables small, for performance reasons.
                        self.admin.run_icommand(['imeta', 'rm', '-C', self.admin.session_collection, attr_name_1, delay_rule_body])
                        self.admin.run_icommand(['imeta', 'rm', '-C', self.admin.session_collection, attr_name_2, '12'])

            finally:
                # Restart so the server's original configuration takes effect.
                IrodsController().reload_configuration()

    @unittest.skipIf(plugin_name != 'irods_rule_engine_plugin-irods_rule_language' or test.settings.RUN_IN_TOPOLOGY, "Requires NREP and single node zone.")
    def test_if_Genquery2Input_is_exposed__issue_7570(self):
        with temporary_core_file() as core:
            avu_prefix = 'test_if_Genquery2Input_is_exposed__issue_7570'

            # Add a rule to core.re which when triggered will add AVUs to the user's
            # session collection.
            core.add_rule(dedent('''
            pep_api_genquery2_pre(*inst, *comm, *input, *output) {{
                *v = *input.query_string;
                msiModAVUMetadata('-C', '{self.user.session_collection}', 'add', '{avu_prefix}_0', *v, '');

                *v = *input.zone;
                if (strlen(*v) > 0) {{
                    msiModAVUMetadata('-C', '{self.user.session_collection}', 'add', '{avu_prefix}_1', *v, '');
                }}
                else {{
                    msiModAVUMetadata('-C', '{self.user.session_collection}', 'add', '{avu_prefix}_1', 'unspecified', '');
                }}

                *v = *input.sql_only;
                msiModAVUMetadata('-C', '{self.user.session_collection}', 'add', '{avu_prefix}_2', *v, '');

                *v = *input.column_mappings;
                msiModAVUMetadata('-C', '{self.user.session_collection}', 'add', '{avu_prefix}_3', *v, '');
            }}
            '''.format(**locals())))
            IrodsController().reload_configuration()

            try:
                # Trigger PEP, ignore output.
                query_string = f"select COLL_NAME where COLL_NAME = '{self.user.session_collection}'"
                self.user.assert_icommand(['iquery', query_string], 'STDOUT')

                # Show the session collection has the expected AVUs attached to it.
                self.user.assert_icommand(['imeta', 'ls', '-C', self.user.session_collection], 'STDOUT', [
                    f'attribute: {avu_prefix}_0\nvalue: {query_string}\n',
                    f'attribute: {avu_prefix}_1\nvalue: unspecified\n',
                    f'attribute: {avu_prefix}_2\nvalue: 0\n',
                    f'attribute: {avu_prefix}_3\nvalue: 0\n',
                ])

                # Remove the AVUs to avoid duplicate AVU errors. These will cause the test to fail.
                self.user.assert_icommand(['imeta', 'rm', '-C', self.user.session_collection, f'{avu_prefix}_0', query_string])
                self.user.assert_icommand(['imeta', 'rm', '-C', self.user.session_collection, f'{avu_prefix}_1', 'unspecified'])
                self.user.assert_icommand(['imeta', 'rm', '-C', self.user.session_collection, f'{avu_prefix}_2', '0'])
                self.user.assert_icommand(['imeta', 'rm', '-C', self.user.session_collection, f'{avu_prefix}_3', '0'])

                # Now, with the zone explicitly passed.
                self.user.assert_icommand(['iquery', '-z', self.user.zone_name, query_string], 'STDOUT')

                # Show the session collection has the expected AVUs attached to it.
                self.user.assert_icommand(['imeta', 'ls', '-C', self.user.session_collection], 'STDOUT', [
                    f'attribute: {avu_prefix}_0\nvalue: {query_string}\n',
                    f'attribute: {avu_prefix}_1\nvalue: {self.user.zone_name}\n',
                    f'attribute: {avu_prefix}_2\nvalue: 0\n',
                    f'attribute: {avu_prefix}_3\nvalue: 0\n',
                ])

            finally:
                # This isn't required, but we do it here to keep the database metadata
                # tables small, for performance reasons.
                self.user.run_icommand(['imeta', 'rm', '-C', self.user.session_collection, f'{avu_prefix}_0', query_string])
                self.user.run_icommand(['imeta', 'rm', '-C', self.user.session_collection, f'{avu_prefix}_1', 'unspecified'])
                self.user.run_icommand(['imeta', 'rm', '-C', self.user.session_collection, f'{avu_prefix}_1', self.user.zone_name])
                self.user.run_icommand(['imeta', 'rm', '-C', self.user.session_collection, f'{avu_prefix}_2', '0'])
                self.user.run_icommand(['imeta', 'rm', '-C', self.user.session_collection, f'{avu_prefix}_3', '0'])

                IrodsController().reload_configuration()

    @unittest.skipIf(plugin_name != 'irods_rule_engine_plugin-irods_rule_language' or test.settings.RUN_IN_TOPOLOGY, "Requires NREP and single node zone.")
    def test_if_ExecMyRuleInp_is_exposed__issue_7552(self):
        with temporary_core_file() as core:
            avu_prefix = 'test_if_ExecMyRuleInp_is_exposed__issue_7552'
            # Add a rule to core.re which when triggered will add AVUs to the user's
            # session collection.
            core.add_rule(dedent('''
            pep_api_exec_my_rule_pre(*INSTANCE, *COMM, *STRUCTINP, *OUT) {{
                *v = *STRUCTINP.inpParamArray_0_inOutStruct;
                msiModAVUMetadata('-C', '{self.user.session_collection}', 'add', '{avu_prefix}_0', *v, '');

                *v = *STRUCTINP.inpParamArray_0_label;
                msiModAVUMetadata('-C', '{self.user.session_collection}', 'add', '{avu_prefix}_1', *v, '');

                *v = *STRUCTINP.inpParamArray_0_type;
                msiModAVUMetadata('-C', '{self.user.session_collection}', 'add', '{avu_prefix}_2', *v, '');

                *v = *STRUCTINP.inpParamArray_len;
                msiModAVUMetadata('-C', '{self.user.session_collection}', 'add', '{avu_prefix}_3', *v, '');

                *v = *STRUCTINP.inpParamArray_oprType;
                msiModAVUMetadata('-C', '{self.user.session_collection}', 'add', '{avu_prefix}_4', *v, '');

                *v = *STRUCTINP.myRule;
                msiModAVUMetadata('-C', '{self.user.session_collection}', 'add', '{avu_prefix}_5', *v, '');

                *v = *STRUCTINP.outParamDesc;
                msiModAVUMetadata('-C', '{self.user.session_collection}', 'add', '{avu_prefix}_6', *v, '');
            }}
            '''.format(**locals())))
            IrodsController().reload_configuration()

            try:
                # Should trigger PEP
                self.user.assert_icommand(['irule', "*C=*A++*B", "*A=potato%*B=tomato", "*C="])

                self.user.assert_icommand(['imeta', 'ls', '-C', self.user.session_collection], 'STDOUT', [
                    f'attribute: {avu_prefix}_0\nvalue: potato\n',
                    f'attribute: {avu_prefix}_1\nvalue: *A\n',
                    f'attribute: {avu_prefix}_2\nvalue: STR_PI\n',
                    f'attribute: {avu_prefix}_3\nvalue: 2\n',
                    f'attribute: {avu_prefix}_4\nvalue: 0\n',
                    f'attribute: {avu_prefix}_5\nvalue: @external rule {{ *C=*A++*B }}\n',
                    f'attribute: {avu_prefix}_6\nvalue: *C=\n',
                ])

            finally:
                # Remove the AVUs
                self.user.run_icommand(['imeta', 'rm', '-C', self.user.session_collection, f'{avu_prefix}_0', "potato"])
                self.user.run_icommand(['imeta', 'rm', '-C', self.user.session_collection, f'{avu_prefix}_1', "*A"])
                self.user.run_icommand(['imeta', 'rm', '-C', self.user.session_collection, f'{avu_prefix}_2', "STR_PI"])
                self.user.run_icommand(['imeta', 'rm', '-C', self.user.session_collection, f'{avu_prefix}_3', "2"])
                self.user.run_icommand(['imeta', 'rm', '-C', self.user.session_collection, f'{avu_prefix}_4', "0"])
                self.user.run_icommand(['imeta', 'rm', '-C', self.user.session_collection, f'{avu_prefix}_5', "@external rule {{ *C=*A++*B }}"])
                self.user.run_icommand(['imeta', 'rm', '-C', self.user.session_collection, f'{avu_prefix}_6', "*C="])

                IrodsController().reload_configuration()

    @unittest.skipIf(plugin_name != 'irods_rule_engine_plugin-irods_rule_language' or test.settings.RUN_IN_TOPOLOGY, "Requires NREP and single node zone.")
    def test_if_StructFileExtAndRegInp_is_exposed__issue_7413(self):
        with temporary_core_file() as core:
            avu_prefix = 'test_if_StructFileExtAndRegInp_is_exposed__issue_7413'

            # Add a rule to core.re which when triggered will add AVUs to the user's
            # session collection.
            core.add_rule(dedent('''
            pep_api_struct_file_ext_and_reg_post(*INSTANCE, *COMM, *STRUCTINP) {{
                *v = *STRUCTINP.obj_path;
                msiModAVUMetadata('-C', '{self.user.session_collection}', 'add', '{avu_prefix}_0', *v, '');

                *v = *STRUCTINP.collection_path;
                msiModAVUMetadata('-C', '{self.user.session_collection}', 'add', '{avu_prefix}_1', *v, '');

                *v = *STRUCTINP.opr_type;
                msiModAVUMetadata('-C', '{self.user.session_collection}', 'add', '{avu_prefix}_2', *v, '');

                *v = *STRUCTINP.flags;
                msiModAVUMetadata('-C', '{self.user.session_collection}', 'add', '{avu_prefix}_3', *v, '');

                *v = *STRUCTINP.defRescName;
                msiModAVUMetadata('-C', '{self.user.session_collection}', 'add', '{avu_prefix}_4', *v, '');

                if('' == *STRUCTINP.forceFlag) {{
                    msiModAVUMetadata('-C', '{self.user.session_collection}', 'add', '{avu_prefix}_5', 'forceFlagcheck', '');
                }}
            }}
            '''.format(**locals())))
            IrodsController().reload_configuration()

            TEST_COLLECTION_NAME = "%s/%s" % (self.user.session_collection, 'testcoll')
            TEST_TARFILE_NAME = "%s/%s" % (self.user.session_collection, 'test.tar')

            try:
                self.user.assert_icommand(['imkdir', TEST_COLLECTION_NAME], 'STDOUT')
                # Make some test files...
                for x in range(0, 5):
                    self.user.assert_icommand(['itouch', "%s/file%s" % (TEST_COLLECTION_NAME, x)])

                self.user.assert_icommand(['ibun', '-cDtar', "%s/%s" % (self.user.session_collection, 'test.tar'), TEST_COLLECTION_NAME])

                self.user.assert_icommand(['imkdir', TEST_COLLECTION_NAME + "_backup"], 'STDOUT')

                # Should trigger PEP
                self.user.assert_icommand(['ibun', '-x', '-f', TEST_TARFILE_NAME, TEST_COLLECTION_NAME + "_backup"])

                # Show the session collection has the expected AVUs attached to it.
                self.user.assert_icommand(['imeta', 'ls', '-C', self.user.session_collection], 'STDOUT', [
                    f'attribute: {avu_prefix}_0\nvalue: {TEST_TARFILE_NAME}\n',
                    f'attribute: {avu_prefix}_1\nvalue: {TEST_COLLECTION_NAME + "_backup"}\n',
                    f'attribute: {avu_prefix}_2\nvalue: 0\n',
                    f'attribute: {avu_prefix}_3\nvalue: 0\n',
                    f'attribute: {avu_prefix}_4\nvalue: {self.user.default_resource}\n',
                    f'attribute: {avu_prefix}_5\nvalue: forceFlagcheck\n',
                ])

            finally:
                # Remove the AVUs
                self.user.run_icommand(['imeta', 'rm', '-C', self.user.session_collection, f'{avu_prefix}_0', TEST_TARFILE_NAME])
                self.user.run_icommand(['imeta', 'rm', '-C', self.user.session_collection, f'{avu_prefix}_1', TEST_COLLECTION_NAME])
                self.user.run_icommand(['imeta', 'rm', '-C', self.user.session_collection, f'{avu_prefix}_2', '0'])
                self.user.run_icommand(['imeta', 'rm', '-C', self.user.session_collection, f'{avu_prefix}_3', '0'])
                self.user.run_icommand(['imeta', 'rm', '-C', self.user.session_collection, f'{avu_prefix}_4', self.user.default_resource])
                self.user.run_icommand(['imeta', 'rm', '-C', self.user.session_collection, f'{avu_prefix}_5', 'forceFlagcheck'])

                # Cleanup
                self.user.run_icommand(['irm', '-rf', TEST_COLLECTION_NAME])
                self.user.run_icommand(['irm', '-rf', TEST_COLLECTION_NAME + "_backup"])
                self.user.run_icommand(['irm', '-f', TEST_TARFILE_NAME])

                IrodsController().reload_configuration()

    @unittest.skipIf(plugin_name != 'irods_rule_engine_plugin-irods_rule_language' or test.settings.RUN_IN_TOPOLOGY, "Requires NREP and single node zone.")
    def test_if_MsParamArray_is_exposed__issue_7553(self):
        with temporary_core_file() as core:
            avu_prefix = 'test_if_MsParamArray_is_exposed__issue_7553'
            # Add a rule to core.re which when triggered will add AVUs to the user's
            # session collection.
            core.add_rule(dedent('''
            pep_api_exec_my_rule_post(*INSTANCE, *COMM, *STRUCTINP, *OUT) {{
                *v = *OUT."0_inOutStruct";
                msiModAVUMetadata('-C', '{self.user.session_collection}', 'add', '{avu_prefix}_0', *v, '');

                *v = *OUT."0_label";
                msiModAVUMetadata('-C', '{self.user.session_collection}', 'add', '{avu_prefix}_1', *v, '');

                *v = *OUT."0_type";
                msiModAVUMetadata('-C', '{self.user.session_collection}', 'add', '{avu_prefix}_2', *v, '');

                *v = *OUT.len;
                msiModAVUMetadata('-C', '{self.user.session_collection}', 'add', '{avu_prefix}_3', *v, '');

                *v = *OUT.oprType;
                msiModAVUMetadata('-C', '{self.user.session_collection}', 'add', '{avu_prefix}_4', *v, '');

                *v = *OUT."1_inOutStruct";
                msiModAVUMetadata('-C', '{self.user.session_collection}', 'add', '{avu_prefix}_5', *v, '');

                *v = *OUT."1_label";
                msiModAVUMetadata('-C', '{self.user.session_collection}', 'add', '{avu_prefix}_6', *v, '');

                *v = *OUT."1_type";
                msiModAVUMetadata('-C', '{self.user.session_collection}', 'add', '{avu_prefix}_7', *v, '');

                *v = *OUT."2_inOutStruct";
                msiModAVUMetadata('-C', '{self.user.session_collection}', 'add', '{avu_prefix}_8', *v, '');

                *v = *OUT."2_label";
                msiModAVUMetadata('-C', '{self.user.session_collection}', 'add', '{avu_prefix}_9', *v, '');

                *v = *OUT."2_type";
                msiModAVUMetadata('-C', '{self.user.session_collection}', 'add', '{avu_prefix}_10', *v, '');
            }}
            '''.format(**locals())))
            IrodsController().reload_configuration()

            try:
                # Should trigger PEP
                self.user.assert_icommand(['irule', '*A=4;*B=3.4;*C="potato"', 'null', '*A%*B%*C'], 'STDOUT')

                self.user.assert_icommand(['imeta', 'ls', '-C', self.user.session_collection], 'STDOUT', [
                    f'attribute: {avu_prefix}_0\nvalue: 4\n',
                    f'attribute: {avu_prefix}_1\nvalue: *A\n',
                    f'attribute: {avu_prefix}_2\nvalue: INT_PI\n',
                    f'attribute: {avu_prefix}_3\nvalue: 3\n',
                    f'attribute: {avu_prefix}_4\nvalue: 0\n',
                    f'attribute: {avu_prefix}_5\nvalue: 3.400000\n',
                    f'attribute: {avu_prefix}_6\nvalue: *B\n',
                    f'attribute: {avu_prefix}_7\nvalue: DOUBLE_PI\n',
                    f'attribute: {avu_prefix}_8\nvalue: potato\n',
                    f'attribute: {avu_prefix}_9\nvalue: *C\n',
                    f'attribute: {avu_prefix}_10\nvalue: STR_PI\n',
                ])

            finally:
                # Remove the AVUs
                self.user.run_icommand(['imeta', 'rm', '-C', self.user.session_collection, f'{avu_prefix}_0', "4"])
                self.user.run_icommand(['imeta', 'rm', '-C', self.user.session_collection, f'{avu_prefix}_1', "*A"])
                self.user.run_icommand(['imeta', 'rm', '-C', self.user.session_collection, f'{avu_prefix}_2', "INT_PI"])
                self.user.run_icommand(['imeta', 'rm', '-C', self.user.session_collection, f'{avu_prefix}_3', "3"])
                self.user.run_icommand(['imeta', 'rm', '-C', self.user.session_collection, f'{avu_prefix}_4', "0"])
                self.user.run_icommand(['imeta', 'rm', '-C', self.user.session_collection, f'{avu_prefix}_5', "3.400000"])
                self.user.run_icommand(['imeta', 'rm', '-C', self.user.session_collection, f'{avu_prefix}_6', "*B"])
                self.user.run_icommand(['imeta', 'rm', '-C', self.user.session_collection, f'{avu_prefix}_7', "DOUBLE_PI"])
                self.user.run_icommand(['imeta', 'rm', '-C', self.user.session_collection, f'{avu_prefix}_8', "potato"])
                self.user.run_icommand(['imeta', 'rm', '-C', self.user.session_collection, f'{avu_prefix}_9', "*C"])
                self.user.run_icommand(['imeta', 'rm', '-C', self.user.session_collection, f'{avu_prefix}_10', "STR_PI"])

                IrodsController().reload_configuration()
