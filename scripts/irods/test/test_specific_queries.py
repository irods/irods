import os
import unittest

from . import session

class Test_Specific_Queries(session.make_sessions_mixin([], [('alice', 'apass')]), unittest.TestCase):

    def setUp(self):
        super(Test_Specific_Queries, self).setUp()
        self.user = self.user_sessions[0]

    def tearDown(self):
        super(Test_Specific_Queries, self).tearDown()

    def test_DataObjInCollReCur__issue_6900(self):
        collection = os.path.join(self.user.session_collection, 'test.d')
        self.user.assert_icommand(['imkdir', 'test.d'])

        data_object_name = 'issue_6900'
        self.user.assert_icommand(['itouch', os.path.join(collection, data_object_name)])

        expected_output = ['EMPTY_RESC_NAME\n', collection + '\n', data_object_name + '\n']
        self.user.assert_icommand(['iquest', '--sql', 'DataObjInCollReCur', collection, collection], 'STDOUT', expected_output)
