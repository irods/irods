import pydevtest_sessions as s
from nose.tools import with_setup
from nose.plugins.skip import SkipTest
from pydevtest_common import assertiCmd, assertiCmdFail
import commands

@with_setup(s.adminonly_up,s.adminonly_down)
def test_irm():
    # local setup
    # assertions
    assertiCmd(s.adminsession,"ils -L "+s.testfile,"LIST",s.testfile) # should be listed
    assertiCmd(s.adminsession,"irm "+s.testfile) # remove from grid
    assertiCmdFail(s.adminsession,"ils -L "+s.testfile,"LIST",s.testfile) # should be deleted
    # local cleanup

@with_setup(s.adminonly_up,s.adminonly_down)
def test_irm_doesnotexist():
    assertiCmdFail(s.adminsession,"irm doesnotexist") # does not exist
