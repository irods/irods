import unittest
import pydevtest_sessions as s
from pydevtest_common import assertiCmd, assertiCmdFail
import commands

class Test_Pydevtest_Examples(unittest.TestCase):

    def setUp(self):
        s.adminonly_up()
    def tearDown(self):
        s.adminonly_down()

    def test_basic_python_assertions(self):
        assert "a" in "abc"       # in
        assert "d" not in "abc"   # not in
        assert "abc" == "abc"     # equal
        assert "abc" != "abcd"    # not equal
        assert "234" > "123"      # string comparison
        assert 3444 == 3444       # equal
        assert 34 != 35           # not equal
        assert 324324 > 8443      # int comparison
        assert 345 < 8423         # int comparison
        assert "546" > "30000"    # string comparison

    @unittest.skip("default skip test")
    def test_skip_me(self):
        pass

    def test_attempt_bad_icommand(self):
        assertiCmdFail(s.adminsession,"idoesnotexist","LIST","nope")

    def test_catch_icommand_error(self):
        assertiCmd(s.adminsession,"ils filedoesnotexist","ERROR","does not exist or user lacks access permission")

    def test_show_variables(self):
        print help(s.adminsession)
        print s.adminsession.__dict__.keys()
        print s.sessions[0].__dict__.keys()
        print s.adminsession.sessionId
        print s.adminsession.sessionDir
#        assert False, "I am a placeholder"
