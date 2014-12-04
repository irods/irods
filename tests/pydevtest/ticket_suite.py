import sys
if (sys.version_info >= (2, 7)):
    import unittest
else:
    import unittest2 as unittest
from resource_suite import ResourceBase
from pydevtest_common import assertiCmd, assertiCmdFail, interruptiCmd
import pydevtest_sessions as s
import commands


class Test_TicketSuite(unittest.TestCase, ResourceBase):

    my_test_resource = {"setup": [], "teardown": []}

    def setUp(self):
        ResourceBase.__init__(self)
        s.twousers_up()
        self.run_resource_setup()

    def tearDown(self):
        self.run_resource_teardown()
        s.twousers_down()

    def test_list_no_tickets(self):
        assertiCmd(s.adminsession, "iticket ls")

    def test_iticket_bad_subcommand(self):
        assertiCmd(s.adminsession, "iticket badsubcommand", "LIST", "unrecognized command")
