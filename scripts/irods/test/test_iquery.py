import json
import unittest

from . import session

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
