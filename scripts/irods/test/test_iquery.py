import json
import unittest

from . import session

rodsadmins = []
rodsusers  = [('alice', 'apass')]

class Test_IQuery(session.make_sessions_mixin(rodsadmins, rodsusers), unittest.TestCase):

    def setUp(self):
        super(Test_IQuery, self).setUp()
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
