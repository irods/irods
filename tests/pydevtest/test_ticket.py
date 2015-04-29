import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest
from resource_suite import ResourceBase


class Test_Ticket(ResourceBase, unittest.TestCase):
    def setUp(self):
        super(Test_Ticket, self).setUp()

    def tearDown(self):
        super(Test_Ticket, self).tearDown()

    def test_list_no_tickets(self):
        self.admin.assert_icommand('iticket ls')

    def test_iticket_bad_subcommand(self):
        self.admin.assert_icommand('iticket badsubcommand', 'STDOUT_SINGLELINE', 'unrecognized command')
