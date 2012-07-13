import sessions as s
from nose import with_setup
from zonetests_common import assertiCmd, assertiCmdFail
import commands

@with_setup(s.admin_session_up,s.admin_session_down)
def test_irm():
  # local setup
  datafilename = "textfile.txt"
  output = commands.getstatusoutput( 'echo "i am a testfile" > '+datafilename )
  # assertions
  assertiCmd(s.adminsession,"iput "+datafilename) # iput
  assertiCmd(s.adminsession,"irm "+datafilename) # remove from grid
  assertiCmdFail(s.adminsession,"ils -L "+datafilename,"LIST",datafilename) # should be deleted
  # local cleanup
  output = commands.getstatusoutput( 'rm '+datafilename )
