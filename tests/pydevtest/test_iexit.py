from nose.plugins.skip import SkipTest
from pydevtest_common import assertiCmd, assertiCmdFail, interruptiCmd
import pydevtest_sessions as s
import commands

class Test_iexit(object):

    def setUp(self):
        s.twousers_up()
    def tearDown(self):
        s.twousers_down()

    ###################
    # iexit
    ###################

    def test_iexit(self):
        # assertions
        assertiCmd(s.adminsession,"iexit") # just go home

    def test_iexit_verbose(self):
        # assertions
        assertiCmd(s.adminsession,"iexit -v","LIST","Deleting (if it exists) session envFile:") # home, verbose

    def test_iexit_with_bad_option(self):
        # assertions
        assertiCmdFail(s.adminsession,"iexit -z") # run iexit with bad option

    def test_iexit_with_bad_parameter(self):
        # assertions
        assertiCmdFail(s.adminsession,"iexit badparameter") # run iexit with bad parameter

