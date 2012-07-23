import pydevtest_sessions as s
from nose import with_setup
from pydevtest_common import assertiCmd, assertiCmdFail
import commands

@with_setup(s.admin_session_up,s.admin_session_down)
def test_local_iput():
  # local setup
  datafilename = "newfile.txt"
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
  # assertions
  assertiCmdFail(s.adminsession,"iput "+s.testfile) # fail, already exists
  assertiCmd(s.adminsession,"iput -f "+s.testfile) # iput again, force
  # local cleanup

@with_setup(s.admin_session_up,s.admin_session_down)
def test_local_iput_checksum():
  # local setup
  datafilename = "newfile.txt"
  f = open(datafilename,'wb')
  f.write("TESTFILE -- ["+datafilename+"]")
  f.close()
  # assertions
  assertiCmd(s.adminsession,"iput -K "+datafilename) # iput
  assertiCmd(s.adminsession,"ils -L","LIST","d60af3eb3251240782712eab3d8ef3b1") # check proper checksum
  # local cleanup
  output = commands.getstatusoutput( 'rm '+datafilename )

@with_setup(s.admin_session_up,s.admin_session_down)
def test_local_iput_onto_specific_resource():
  # local setup
  datafilename = "anotherfile.txt"
  f = open(datafilename,'wb')
  f.write("TESTFILE -- ["+datafilename+"]")
  f.close()
  # assertions
  assertiCmdFail(s.adminsession,"ils -L "+datafilename,"LIST",datafilename) # should not be listed
  assertiCmd(s.adminsession,"iput -R testResc "+datafilename) # iput
  assertiCmd(s.adminsession,"ils -L "+datafilename,"LIST",datafilename) # should be listed
  assertiCmd(s.adminsession,"ils -L "+datafilename,"LIST","testResc") # should be listed
  # local cleanup
  output = commands.getstatusoutput( 'rm '+datafilename )
