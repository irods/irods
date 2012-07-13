import sessions as s
from nose import with_setup
from zonetests_common import assertiCmd, assertiCmdFail
import commands

@with_setup(s.admin_session_up,s.admin_session_down)
def test_local_imv():
  # local setup
  datafilename = "textfile.txt"
  datafilename2 = "moved_file.txt"
  output = commands.getstatusoutput( 'echo "i am a testfile" > '+datafilename )
  # assertions
  assertiCmd(s.adminsession,"iput "+datafilename) # iput
  assertiCmd(s.adminsession,"imv "+datafilename+" "+datafilename2) # move
  assertiCmd(s.adminsession,"ils -L "+datafilename2,"LIST",datafilename2) # should be listed
  # local cleanup
  output = commands.getstatusoutput( 'rm '+datafilename )

@with_setup(s.admin_session_up,s.admin_session_down)
def test_local_imv_to_directory():
  # local setup
  datafilename = "textfile.txt"
  testdir = "testdir"
  output = commands.getstatusoutput( 'echo "i am a testfile" > '+datafilename )
  # assertions
  assertiCmd(s.adminsession,"imkdir "+testdir) # imkdir
  assertiCmd(s.adminsession,"iput "+datafilename) # iput
  assertiCmd(s.adminsession,"imv "+datafilename+" "+testdir) # move
  assertiCmd(s.adminsession,"ils -L "+testdir,"LIST",datafilename) # should be listed
  # local cleanup
  output = commands.getstatusoutput( 'rm '+datafilename )

@with_setup(s.admin_session_up,s.admin_session_down)
def test_local_imv_to_existing_filename():
  # local setup
  datafilename = "textfile.txt"
  datafilename2 = "textfile2.txt"
  output = commands.getstatusoutput( 'echo "i am a testfile" > '+datafilename )
  # assertions
  assertiCmd(s.adminsession,"iput "+datafilename) # iput
  assertiCmd(s.adminsession,"icp "+datafilename+" "+datafilename2) # icp
  assertiCmdFail(s.adminsession,"imv "+datafilename+" "+datafilename2) # cannot overwrite existing file
  # local cleanup
  output = commands.getstatusoutput( 'rm '+datafilename )

