from nose.plugins.skip import SkipTest
from pydevtest_common import assertiCmd, assertiCmdFail, interruptiCmd
import pydevtest_sessions as s
import commands

class Test_ihelp(object):

    def setUp(self):
        s.twousers_up()
    def tearDown(self):
        s.twousers_down()

    ###################
    # ihelp
    ###################

    def test_local_ihelp(self):
        # assertions
        assertiCmd(s.adminsession,"ihelp","LIST","The following is a list of the icommands") # run ihelp

    def test_local_ihelp_with_help(self):
        # assertions
        assertiCmd(s.adminsession,"ihelp -h","LIST","Display i-commands synopsis") # run ihelp with help

    def test_local_ihelp_all(self):
        # assertions
        assertiCmd(s.adminsession,"ihelp -a","LIST","Usage") # run ihelp on all icommands

    def test_local_ihelp_with_good_icommand(self):
        # assertions
        assertiCmd(s.adminsession,"ihelp ils","LIST","Usage") # run ihelp with good icommand

    def test_local_ihelp_with_bad_icommand(self):
        # assertions
        assertiCmdFail(s.adminsession,"ihelp idoesnotexist") # run ihelp with bad icommand

    def test_local_ihelp_with_bad_option(self):
        # assertions
        assertiCmdFail(s.adminsession,"ihelp -z") # run ihelp with bad option


