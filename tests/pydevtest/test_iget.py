import sessions as s
from nose import with_setup
from zonetests_common import assertiCmd, assertiCmdFail
import commands

@with_setup(s.admin_session_up,s.admin_session_down)
def test_local_iget():
  # local setup
  datafilename = "textfile.txt"
  f = open(datafilename,'wb')
  f.write("TESTFILE -- ["+datafilename+"]")
  f.close()
  # assertions
  assertiCmd(s.adminsession,"iput "+datafilename) # iput
  output = commands.getstatusoutput( 'rm '+datafilename ) # rm local
  assertiCmd(s.adminsession,"iget "+datafilename) # iget
  # local cleanup
  output = commands.getstatusoutput( 'rm '+datafilename )
