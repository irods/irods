import pydevtest_sessions as s
from nose.tools import with_setup
from nose.plugins.skip import SkipTest
from pydevtest_common import assertiCmd, assertiCmdFail
import commands

def test_basic_python_assertions():
    assert "a" in "abc"       # in
    assert "d" not in "abc"   # not in
    assert "abc" == "abc"     # equal
    assert "abc" != "abcd"    # not equal
    assert "234" > "123"      # string comparison
    assert 324324 > 8443      # int comparison
    assert 345 < 8423         # int comparison
    assert "546" > "30000"    # string comparison

def test_skip_this_test():
    raise SkipTest

@with_setup(s.adminonly_up,s.adminonly_down)
def test_attempt_bad_icommand():
    assertiCmdFail(s.adminsession,"idoesnotexist","LIST","nope")
