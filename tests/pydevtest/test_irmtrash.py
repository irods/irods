import pydevtest_sessions as s
from nose.tools import with_setup
from nose.plugins.skip import SkipTest
from pydevtest_common import assertiCmd, assertiCmdFail
import commands

@with_setup(s.admin_session_up,s.admin_session_down)
def test_irmtrash_admin():
  # assertions
  assertiCmd(s.adminsession,"irm "+s.testfile) # remove from grid
  assertiCmd(s.adminsession,"ils -rL /"+s.myzone+"/trash/home/"+s.myuser+"/","LIST",s.testfile) # should be listed
  assertiCmd(s.adminsession,"irmtrash") # should be listed
  assertiCmdFail(s.adminsession,"ils -rL /"+s.myzone+"/trash/home/"+s.myuser+"/","LIST",s.testfile) # should be deleted

