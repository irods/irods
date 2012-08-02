import pydevtest_sessions as s
from nose.tools import with_setup
from nose.plugins.skip import SkipTest
from pydevtest_common import assertiCmd, assertiCmdFail
import commands

@with_setup(s.admin_session_up,s.admin_session_down)
def test_empty_icd():
  # assertions
  assertiCmd(s.adminsession,"icd "+s.testdir) # get into subdir
  assertiCmd(s.adminsession,"icd") # just go home
  assertiCmd(s.adminsession,"ils","LIST","/tempZone/home/rods:") # listing

@with_setup(s.admin_session_up,s.admin_session_down)
def test_empty_icd_verbose():
  # assertions
  assertiCmd(s.adminsession,"icd "+s.testdir) # get into subdir
  assertiCmd(s.adminsession,"icd -v","LIST","Deleting (if it exists) session envFile:") # home, verbose
  assertiCmd(s.adminsession,"ils","LIST","/tempZone/home/rods:") # listing

@with_setup(s.admin_session_up,s.admin_session_down)
def test_icd_to_subdir():
  # assertions
  assertiCmd(s.adminsession,"icd "+s.testdir) # get into subdir
  assertiCmd(s.adminsession,"ils","LIST","/tempZone/home/rods/"+s.sessionid+"/"+s.testdir+":") # listing

@with_setup(s.admin_session_up,s.admin_session_down)
def test_icd_to_parentdir():
  # assertions
  assertiCmd(s.adminsession,"icd ..") # go to parent
  assertiCmd(s.adminsession,"ils","LIST","/tempZone/home/rods:") # listing

@with_setup(s.admin_session_up,s.admin_session_down)
def test_icd_to_root():
  # assertions
  assertiCmd(s.adminsession,"icd /") # go to root
  assertiCmd(s.adminsession,"ils","LIST","/:") # listing

@with_setup(s.admin_session_up,s.admin_session_down)
def test_icd_to_root_with_badpath():
  # assertions
  assertiCmd(s.adminsession,"icd /doesnotexist","LIST","No such directory (collection):") # go to root with bad path
