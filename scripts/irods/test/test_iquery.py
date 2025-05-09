import json
import unittest

from . import session
from .. import lib
from .. import test
from ..configuration import IrodsConfig
from ..controller import IrodsController

rodsadmins = [('otherrods', 'rods')]
rodsusers  = [('alice', 'apass')]

class Test_IQuery(session.make_sessions_mixin(rodsadmins, rodsusers), unittest.TestCase):

    def setUp(self):
        super(Test_IQuery, self).setUp()
        self.admin = self.admin_sessions[0]
        self.user = self.user_sessions[0]

    def tearDown(self):
        super(Test_IQuery, self).tearDown()

    def test_iquery_can_run_a_query__issue_7570(self):
        json_string = json.dumps([[self.user.session_collection]])
        self.user.assert_icommand(
            ['iquery', f"select COLL_NAME where COLL_NAME = '{self.user.session_collection}'"], 'STDOUT', [json_string])

    def test_iquery_supports_reading_query_from_stdin__issue_7570(self):
        json_string = json.dumps([[self.user.session_collection]])
        self.user.assert_icommand(
            ['iquery'], 'STDOUT', [json_string], input=f"select COLL_NAME where COLL_NAME = '{self.user.session_collection}'")

    def test_iquery_returns_error_on_invalid_zone__issue_7570(self):
        ec, out, err = self.user.assert_icommand_fail(['iquery', '-z', 'invalid_zone', 'select COLL_NAME'], 'STDOUT')
        self.assertEqual(ec, 1)
        self.assertEqual(len(out), 0)
        self.assertEqual(err, 'error: -26000\n') # SYS_INVALID_ZONE_NAME

        ec, out, err = self.user.assert_icommand_fail(['iquery', '-z', '', 'select COLL_NAME'], 'STDOUT')
        self.assertEqual(ec, 1)
        self.assertEqual(len(out), 0)
        self.assertEqual(err, 'error: -26000\n') # SYS_INVALID_ZONE_NAME

        ec, out, err = self.user.assert_icommand_fail(['iquery', '-z', ' ', 'select COLL_NAME'], 'STDOUT')
        self.assertEqual(ec, 1)
        self.assertEqual(len(out), 0)
        self.assertEqual(err, 'error: -26000\n') # SYS_INVALID_ZONE_NAME

    def test_iquery_returns_error_on_invalid_query_tokens__issue_7570(self):
        ec, out, err = self.user.assert_icommand_fail(['iquery', ' '], 'STDOUT')
        self.assertEqual(ec, 1)
        self.assertEqual(len(out), 0)
        self.assertEqual(err, 'error: -167000\n') # SYS_LIBRARY_ERROR

        ec, out, err = self.user.assert_icommand_fail(['iquery', 'select'], 'STDOUT')
        self.assertEqual(ec, 1)
        self.assertEqual(len(out), 0)
        self.assertEqual(err, 'error: -167000\n') # SYS_LIBRARY_ERROR

        ec, out, err = self.user.assert_icommand_fail(['iquery', 'select INVALID_COLUMN'], 'STDOUT')
        self.assertEqual(ec, 1)
        self.assertEqual(len(out), 0)
        self.assertEqual(err, 'error: -130000\n') # SYS_INVALID_INPUT_PARAM

    def test_iquery_returns_error_when_closing_single_quote_is_missing__issue_6393(self):
        ec, out, err = self.user.assert_icommand_fail(
            ['iquery', f"select COLL_NAME where COLL_NAME = '{self.user.session_collection}"], 'STDOUT')
        self.assertEqual(ec, 1)
        self.assertEqual(len(out), 0)
        self.assertEqual(err, 'error: -167000\n') # SYS_LIBRARY_ERROR

    def test_iquery_distinguishes_embedded_IN_substring_from_IN_operator__issue_3064(self):
        attr_name = 'originalVersionId'
        attr_value = 'ignored'

        try:
            self.user.assert_icommand(['imeta', 'add', '-C', self.user.session_collection, attr_name, attr_value])

            json_string = json.dumps([[attr_name, attr_value]], separators=(',', ':'))
            for op in ['IN', 'in']:
                with self.subTest(f'op={op}'):
                    query_string = f"select META_COLL_ATTR_NAME, META_COLL_ATTR_VALUE where META_COLL_ATTR_NAME {op} ('{attr_name}')"
                    self.user.assert_icommand(['iquery', query_string], 'STDOUT', [json_string])

        finally:
            # Remove the metadata.
            self.user.assert_icommand(['imeta', 'rm', '-C', self.user.session_collection, attr_name, attr_value])

    def test_iquery_supports_OR_operator__issue_4069(self):
        query_string = f"select COLL_NAME where COLL_NAME = '{self.user.home_collection}' or COLL_NAME like '{self.admin.home_collection}' order by COLL_NAME"
        json_string = json.dumps([[self.user.home_collection], [self.admin.home_collection]], separators=(',', ':'))
        self.admin.assert_icommand(['iquery', query_string], 'STDOUT', json_string)

    def test_iquery_supports_embedded_single_quotes__issue_5727(self):
        # Create a collection containing an embedded single quote in the name.
        collection = "issue'5727.c"
        self.user.assert_icommand(['imkdir', collection])

        # Create a data object containing an embedded single quote in the name.
        data_object = "test_iquery_supports_embedded_single_quotes'__issue_5727"
        self.user.assert_icommand(['istream', 'write', data_object], input='data')

        # Show the GenQuery2 parser provides ways of dealing with embedded single quotes.
        escaped_strings = [
            data_object.replace("'", '\\x27'),  # Uses hex value to escape single quote.
            data_object.replace("'", "''")      # Uses two single quotes to escape single quote like in SQL.
        ]
        json_string = json.dumps([[data_object, self.user.session_collection]], separators=(',', ':'))
        for escaped in escaped_strings:
            query_string = f"select DATA_NAME, COLL_NAME where COLL_NAME = '{self.user.session_collection}' or DATA_NAME = '{escaped}'"
            with self.subTest(f'escaped_string={escaped}'):
                self.user.assert_icommand(['iquery', query_string], 'STDOUT', [json_string])

    def test_iquery_returns_error_on_invalid_query_string__issue_5734(self):
        # The following query is missing a closing single quote on the first condition
        # in the WHERE clause.
        query_string = "select DATA_ID, DATA_REPL_NUM, COLL_NAME, DATA_NAME, DATA_RESC_HIER, DATA_PATH, META_DATA_ATTR_VALUE " + \
            "where META_DATA_ATTR_NAME = 'id_foo and META_DATA_ATTR_VALUE in ('260,'3261852','3261856','3261901','3080907','3083125'," + \
            "'3083853','3084203','3085046','3091021','3092210','3092313','3093766','3094073','3094078','3095017','3095445','3095522','3097128'," + \
            "'3097225','3097311','3097480','3097702','3097750')"
        ec, out, err = self.user.assert_icommand_fail(['iquery', query_string], 'STDOUT')
        self.assertEqual(ec, 1)
        self.assertEqual(len(out), 0)
        self.assertEqual(err, 'error: -167000\n') # SYS_LIBRARY_ERROR

    def test_genquery2_maps_genquery_user_zone_columns_to_correct_database_columns__issue_8134_8135(self):
        # Show that the column mapping listing contains the correct mappings.
        _, out, _ = self.user.assert_icommand(['iquery', '-c'], 'STDOUT')
        collection_mapping_is_correct = False
        data_object_mapping_is_correct = False
        for line in out.splitlines():
            if '(R_USER_MAIN.zone_name)' in line:
                if 'COLL_ACCESS_USER_ZONE' in line: collection_mapping_is_correct = True
                elif 'DATA_ACCESS_USER_ZONE' in line: data_object_mapping_is_correct = True
        self.assertTrue(collection_mapping_is_correct)
        self.assertTrue(data_object_mapping_is_correct)

        # Show that usage of the GenQuery2 COLL permissions columns results in non-empty resultsets.
        query_string = f"select COLL_ACCESS_USER_NAME, COLL_ACCESS_USER_ZONE, COLL_ACCESS_PERM_NAME where COLL_NAME = '{self.user.home_collection}'"
        json_string = json.dumps([[self.user.username, self.user.zone_name, 'own']], separators=(',', ':'))
        self.user.assert_icommand(['iquery', query_string], 'STDOUT', [json_string])

        # Show that usage of the GenQuery2 DATA permissions columns results in non-empty resultsets.
        data_name = 'test_genquery2_maps_genquery_user_zone_columns_to_correct_database_columns__issue_8134_8135'
        self.user.assert_icommand(['itouch', data_name])
        query_string = f"select DATA_ACCESS_USER_NAME, DATA_ACCESS_USER_ZONE, DATA_ACCESS_PERM_NAME where DATA_NAME = '{data_name}'"
        json_string = json.dumps([[self.user.username, self.user.zone_name, 'own']], separators=(',', ':'))
        self.user.assert_icommand(['iquery', query_string], 'STDOUT', [json_string])

        # Show that use of the GenQuery2 COLL permissions columns in a GROUP BY clause works as well.
        query_string = f"select COLL_ACCESS_USER_NAME, COLL_ACCESS_USER_ZONE, count(COLL_ID) where COLL_NAME = '{self.user.home_collection}' group by COLL_ACCESS_USER_NAME, COLL_ACCESS_USER_ZONE"
        json_string = json.dumps([[self.user.username, self.user.zone_name, '1']], separators=(',', ':'))
        self.user.assert_icommand(['iquery', query_string], 'STDOUT', [json_string])

        # Show that use of the GenQuery2 DATA permissions columns in a GROUP BY clause works as well.
        query_string = f"select DATA_ACCESS_USER_NAME, DATA_ACCESS_USER_ZONE, count(DATA_ID) where DATA_NAME = '{data_name}' group by DATA_ACCESS_USER_NAME, DATA_ACCESS_USER_ZONE"
        json_string = json.dumps([[self.user.username, self.user.zone_name, '1']], separators=(',', ':'))
        self.user.assert_icommand(['iquery', query_string], 'STDOUT', [json_string])

    def test_iquery_correctly_parses_queries_with_tabs_in_the_where_clause__issue_7795(self):
        # Run a basic query to get the expected output.
        query_string = f"select USER_ID where USER_NAME = '{self.user.username}'"
        expected_output = self.user.assert_icommand(["iquery", query_string], "STDOUT")[1].strip()
        expected_user_id = json.loads(expected_output)[0][0]

        # Now construct a query that should return the same results, but with a tab in the where clause.
        query_string_with_tab_in_where_clause = query_string + "\tand USER_TYPE = 'rodsuser'"

        # iquest will succeed because it uses a real parser now.
        user_id = self.user.assert_icommand(["iquest", "%s", query_string_with_tab_in_where_clause], "STDOUT")[1].strip()

        # ...should match.
        self.assertEqual(user_id, expected_user_id)

        # iquery will succeed because it uses a real parser.
        output = self.user.assert_icommand(["iquery", query_string_with_tab_in_where_clause], "STDOUT")[1].strip()
        user_id = json.loads(output)[0][0]

        # ...should match.
        self.assertEqual(user_id, expected_user_id)

    def test_genquery2_honors_group_permissions__issue_8259(self):
        # As the rodsuser, create a new data object.
        data_object = 'issue_8259'
        self.user.assert_icommand(['itouch', data_object])

        for user_type in ['rodsuser', 'groupadmin']:
            with self.subTest(f'user type is [{user_type}]'):
                try:
                    self.admin.assert_icommand(['iadmin', 'moduser', self.user.username, 'type', user_type])
                    self.admin.assert_icommand(['iuserinfo', self.user.username], 'STDOUT', ['type: ' + user_type])

                    # As the rodsadmin, create a new group and add the rodsuser to it.
                    group = 'issue_8259_group'
                    self.admin.assert_icommand(['iadmin', 'mkgroup', group])
                    self.admin.assert_icommand(['iadmin', 'atg', group, self.user.username])

                    # Remove the rodsuser's permissions from the session collection and data object.
                    self.user.assert_icommand(['ichmod', '-r', 'null', self.user.username, self.user.session_collection])

                    # As the rodsuser, show that GenQuery2 cannot locate the collection or data object.
                    self.user.assert_icommand(['iquery', f"select COLL_NAME where COLL_NAME = '{self.user.session_collection}'"], 'STDOUT', ['[]'])
                    self.user.assert_icommand(['iquery', f"select DATA_NAME where DATA_NAME = '{data_object}'"], 'STDOUT', ['[]'])

                    # Grant the group permission to read the rodsuser's session collection and show that
                    # the rodsuser can now locate the collection because GenQuery2 honors group permissions.
                    self.admin.assert_icommand(['ichmod', '-M', 'read', group, self.user.session_collection])
                    json_string = json.dumps([[self.user.session_collection]])
                    self.user.assert_icommand(['iquery', f"select COLL_NAME where COLL_NAME = '{self.user.session_collection}'"], 'STDOUT', [json_string])

                    # Show the rodsuser still cannot see the data object even though they were granted
                    # read permission on the session collection.
                    self.user.assert_icommand(['iquery', f"select DATA_NAME where DATA_NAME = '{data_object}'"], 'STDOUT', ['[]'])

                    # Grant the group permission to read the rodsuser's data object and show that the
                    # rodsuser can now locate the data object because GenQuery2 honors group permissions.
                    self.admin.assert_icommand(['ichmod', '-M', 'read', group, f'{self.user.session_collection}/{data_object}'])
                    json_string = json.dumps([[data_object]])
                    self.user.assert_icommand(['iquery', f"select DATA_NAME where DATA_NAME = '{data_object}'"], 'STDOUT', [json_string])

                    # Show the rodsuser can locate the session collection and data object because of
                    # the group permissions.
                    query_string = f"select COLL_NAME, DATA_NAME where COLL_NAME = '{self.user.session_collection}' and DATA_NAME = '{data_object}'"
                    json_string = json.dumps([[self.user.session_collection, data_object]], separators=(',', ':'))
                    self.user.assert_icommand(['iquery', query_string], 'STDOUT', [json_string])

                finally:
                    self.admin.run_icommand(['ichmod', '-M', '-r', 'own', self.user.username, self.user.session_collection])
                    self.admin.run_icommand(['iadmin', 'rmgroup', group])
                    self.admin.run_icommand(['iadmin', 'moduser', self.user.username, 'type', 'rodsuser'])

    def test_genquery2_sql_generation_for_sql_distinct_keyword__issue_8261(self):
        with self.subTest('GenQuery2 does not automatically insert DISTINCT keyword into SQL'):
            _, out, _ = self.user.assert_icommand(['iquery', '--sql-only', 'select DATA_NAME'], 'STDOUT')
            sql = out.strip()
            self.assertNotIn('distinct', sql)
            self.assertNotIn('select distinct', sql)

        with self.subTest('GenQuery2 supports SELECT DISTINCT syntax'):
            _, out, _ = self.user.assert_icommand(['iquery', '--sql-only', 'select distinct DATA_NAME'], 'STDOUT')
            self.assertTrue(out.strip().startswith('select distinct '))

        with self.subTest('GenQuery2 supports DISTINCT keyword in aggregate functions'):
            _, out, _ = self.user.assert_icommand(['iquery', '--sql-only', 'select count(DATA_NAME)'], 'STDOUT')
            self.assertTrue(out.strip().startswith('select count(t0.data_name)'))

            _, out, _ = self.user.assert_icommand(['iquery', '--sql-only', 'select count(distinct DATA_NAME)'], 'STDOUT')
            self.assertTrue(out.strip().startswith('select count(distinct t0.data_name)'))

            self.user.assert_icommand(['iquery', '--sql-only', 'select count(DATA_NAME) order by count(DATA_NAME)'],
                'STDOUT', [' order by count(t0.data_name) '])

            self.user.assert_icommand(['iquery', '--sql-only', 'select count(distinct DATA_NAME) order by count(distinct DATA_NAME)'],
                'STDOUT', [' order by count(distinct t0.data_name) '])

            # Show that the handling of the DISTINCT keyword is independent of the SQL function.
            # GenQuery2 only validates structure, not correctness, of input arguments.
            _, out, _ = self.user.assert_icommand(['iquery', '--sql-only', 'select issue_8261_fn(DATA_NAME)'], 'STDOUT')
            self.assertTrue(out.strip().startswith('select issue_8261_fn(t0.data_name)'))

            _, out, _ = self.user.assert_icommand(['iquery', '--sql-only', 'select issue_8261_fn(distinct DATA_NAME)'], 'STDOUT')
            self.assertTrue(out.strip().startswith('select issue_8261_fn(distinct t0.data_name)'))

            self.user.assert_icommand(['iquery', '--sql-only', 'select issue_8261_fn(distinct DATA_NAME) order by issue_8261_fn(distinct DATA_NAME)'],
                'STDOUT', [' order by issue_8261_fn(distinct t0.data_name) '])

    def test_genquery2_handling_of_distinct_keyword__issue_8261(self):
        resc0 = 'issue_8261_genquery2_ufs0'
        resc1 = 'issue_8261_genquery2_ufs1'
        data_object = 'issue_8261_data_object.txt'

        try:
            # Create a couple resources to help prove the GenQuery2 parser is working as intended.
            lib.create_ufs_resource(self.admin, resc0)
            lib.create_ufs_resource(self.admin, resc1)

            # Create three replicas. These will be used to show that the GenQuery2 API no longer
            # operates on the logical layer by default.
            self.user.assert_icommand(['itouch', data_object])
            self.user.assert_icommand(['irepl', '-R', resc0, data_object])
            self.user.assert_icommand(['irepl', '-R', resc1, data_object])

            with self.subTest('Counting number of replicas'):
                query_string = f"select count(DATA_NAME) where COLL_NAME = '{self.user.session_collection}'"
                json_string = json.dumps([['3']], separators=(',', ':'))
                self.user.assert_icommand(['iquery', query_string], 'STDOUT', [json_string])

            with self.subTest('Counting number of data objects'):
                query_string = f"select count(distinct DATA_NAME) where COLL_NAME = '{self.user.session_collection}'"
                json_string = json.dumps([['1']], separators=(',', ':'))
                self.user.assert_icommand(['iquery', query_string], 'STDOUT', [json_string])

            with self.subTest('List logical name of replicas'):
                query_string = f"select DATA_NAME where COLL_NAME = '{self.user.session_collection}'"
                json_string = json.dumps([[data_object], [data_object], [data_object]], separators=(',', ':'))
                self.user.assert_icommand(['iquery', query_string], 'STDOUT', [json_string])

            with self.subTest('List logical name of data objects'):
                query_string = f"select distinct DATA_NAME where COLL_NAME = '{self.user.session_collection}'"
                json_string = json.dumps([[data_object]], separators=(',', ':'))
                self.user.assert_icommand(['iquery', query_string], 'STDOUT', [json_string])

        finally:
            self.user.run_icommand(['irm', '-f', data_object])
            self.admin.run_icommand(['iadmin', 'rmresc', resc0])
            self.admin.run_icommand(['iadmin', 'rmresc', resc1])

    @unittest.skipUnless(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Test must be run from a Catalog Service Consumer')
    def test_genquery2_does_not_fail_when_log_level_is_set_to_trace_and_request_is_redirected_to_the_provider__issue_8439(self):
        config = IrodsConfig()
        controller = IrodsController()

        try:
            with lib.file_backed_up(config.server_config_path):
                # While the bug only requires setting the "api" log category to "trace", we go beyond
                # that to verify there are no issues with other log categories.
                for log_category in config.server_config['log_level']:
                    config.server_config['log_level'][log_category] = 'trace'
                lib.update_json_file_from_dict(config.server_config_path, config.server_config)
                controller.reload_configuration()
                self.user.assert_icommand(['iquery', 'select COLL_NAME'], 'STDOUT', [json.dumps([self.user.session_collection])])

        finally:
            controller.reload_configuration()
