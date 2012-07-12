import sessions as s
from nose import with_setup
from zonetests_common import assertiCmd, assertiCmdFail
import commands

@with_setup(s.admin_session_up,s.admin_session_down)
def test_local_iput():
  # local setup
  datafilename = s.myhost+"_localfile.txt"
  f = open(datafilename,'wb')
  f.write("TESTFILE -- ["+datafilename+"]")
  f.close()
  # assertions
  assertiCmdFail(s.adminsession,"ils -L "+datafilename,"LIST",datafilename) # should not be listed
  assertiCmd(s.adminsession,"iput -fK "+datafilename) # iput
  assertiCmd(s.adminsession,"ils -L "+datafilename,"LIST",datafilename) # should be listed
  assertiCmd(s.adminsession,"irm "+datafilename) # remove from grid
  assertiCmdFail(s.adminsession,"ils -L "+datafilename,"LIST",datafilename) # should be deleted
  # local cleanup
  output = commands.getstatusoutput( 'rm '+datafilename )

@with_setup(s.admin_session_up,s.admin_session_down)
def test_local_iput_and_iget():
  # local setup
  datafilename = s.myhost+"_localfile.txt"
  f = open(datafilename,'wb')
  f.write("TESTFILE -- ["+datafilename+"]")
  f.close()
  # assertions
  assertiCmdFail(s.adminsession,"ils -L "+datafilename,"LIST",datafilename) # should not be listed
  assertiCmd(s.adminsession,"iput -fK "+datafilename) # iput
  assertiCmd(s.adminsession,"ils -L "+datafilename,"LIST",datafilename) # should be listed
  assertiCmd(s.adminsession,"iget -fK "+datafilename) # iget
  assertiCmd(s.adminsession,"irm "+datafilename) # remove from grid
  assertiCmdFail(s.adminsession,"ils -L "+datafilename,"LIST",datafilename) # should be deleted
  # local cleanup
  output = commands.getstatusoutput( 'rm '+datafilename )

@with_setup(s.admin_session_up,s.admin_session_down)
def test_local_iput_and_imv():
  # local setup
  datafilename = s.myhost+"_localfile.txt"
  datafilename2 = "moved_file.txt"
  f = open(datafilename,'wb')
  f.write("TESTFILE -- ["+datafilename+"]")
  f.close()
  # assertions
  assertiCmd(s.adminsession,"iput -fK "+datafilename) # iput
  assertiCmdFail(s.adminsession,"ils -L "+datafilename2,"LIST",datafilename2) # should not be listed
  assertiCmd(s.adminsession,"imv "+datafilename+" "+datafilename2) # move
  assertiCmd(s.adminsession,"ils -L "+datafilename2,"LIST",datafilename2) # should be listed
  assertiCmd(s.adminsession,"irm "+datafilename2) # remove from grid
  assertiCmdFail(s.adminsession,"ils -L "+datafilename2,"LIST",datafilename2) # should be deleted
  # local cleanup
  output = commands.getstatusoutput( 'rm '+datafilename )

@with_setup(s.admin_session_up,s.admin_session_down)
def test_local_iput_checksum():
  # local setup
  datafilename = "textfile.txt"
  output = commands.getstatusoutput( 'echo "i am a testfile" > '+datafilename )
  # assertions
  assertiCmdFail(s.adminsession,"ils -L","LIST",datafilename) # should not be listed yet
  assertiCmd(s.adminsession,"iput -fK "+datafilename) # iput
  assertiCmd(s.adminsession,"ils -L","LIST","423ff2c4ecca1307ac45e8a59e09bd3c") # check proper checksum
  # local cleanup
  output = commands.getstatusoutput( 'rm '+datafilename )

