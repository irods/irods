import pydevtest_sessions as s
from nose import with_setup
from pydevtest_common import assertiCmd, assertiCmdFail
import commands

@with_setup(s.admin_session_up,s.admin_session_down)
def test_local_iget():
  # local setup
  localfile = "local.txt"
  # assertions
  assertiCmd(s.adminsession,"iget "+s.testfile+" "+localfile) # iget
  output = commands.getstatusoutput( 'ls '+localfile )
  print "  output: ["+output[1]+"]"
  assert output[1] == localfile
  # local cleanup
  output = commands.getstatusoutput( 'rm '+localfile )

@with_setup(s.admin_session_up,s.admin_session_down)
def test_local_iget_with_overwrite():
  # local setup
  localfile = "local.txt"
  # assertions
  assertiCmd(s.adminsession,"iget "+s.testfile+" "+localfile) # iget
  assertiCmdFail(s.adminsession,"iget "+s.testfile+" "+localfile) # already exists
  assertiCmd(s.adminsession,"iget -f "+s.testfile+" "+localfile) # already exists, so force
  output = commands.getstatusoutput( 'ls '+localfile )
  print "  output: ["+output[1]+"]"
  assert output[1] == localfile
  # local cleanup
  output = commands.getstatusoutput( 'rm '+localfile )
