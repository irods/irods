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
