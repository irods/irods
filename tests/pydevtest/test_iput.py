import sessions as s
from nose import with_setup
from zonetests_common import assertiCmd, assertiCmdFail
import commands

@with_setup(s.admin_session_up,s.admin_session_down)
def test_local_iput():
  # local setup
  datafilename = "textfile.txt"
  f = open(datafilename,'wb')
  f.write("TESTFILE -- ["+datafilename+"]")
  f.close()
  # assertions
  assertiCmdFail(s.adminsession,"ils -L "+datafilename,"LIST",datafilename) # should not be listed
  assertiCmd(s.adminsession,"iput "+datafilename) # iput
  assertiCmd(s.adminsession,"ils -L "+datafilename,"LIST",datafilename) # should be listed
  # local cleanup
  output = commands.getstatusoutput( 'rm '+datafilename )

@with_setup(s.admin_session_up,s.admin_session_down)
def test_local_iput_overwrite():
  # local setup
  datafilename = "textfile.txt"
  output = commands.getstatusoutput( 'echo "i am a testfile" > '+datafilename )
  # assertions
  assertiCmd(s.adminsession,"iput "+datafilename) # iput
  assertiCmdFail(s.adminsession,"iput "+datafilename) # fail, already exists
  assertiCmd(s.adminsession,"iput -f "+datafilename) # iput again, force
  # local cleanup
  output = commands.getstatusoutput( 'rm '+datafilename )

@with_setup(s.admin_session_up,s.admin_session_down)
def test_local_iput_checksum():
  # local setup
  datafilename = "textfile.txt"
  output = commands.getstatusoutput( 'echo "i am a testfile" > '+datafilename )
  # assertions
  assertiCmd(s.adminsession,"iput -K "+datafilename) # iput
  assertiCmd(s.adminsession,"ils -L","LIST","423ff2c4ecca1307ac45e8a59e09bd3c") # check proper checksum
  # local cleanup
  output = commands.getstatusoutput( 'rm '+datafilename )
