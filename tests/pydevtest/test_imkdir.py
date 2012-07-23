import pydevtest_sessions as s
from nose import with_setup
from pydevtest_common import assertiCmd, assertiCmdFail
import commands

@with_setup(s.admin_session_up,s.admin_session_down)
def test_local_imkdir():
  # local setup
  mytestdir = "testingimkdir"
  # assertions
  assertiCmdFail(s.adminsession,"ils -L "+mytestdir,"LIST",mytestdir) # should not be listed
  assertiCmd(s.adminsession,"imkdir "+mytestdir) # imkdir
  assertiCmd(s.adminsession,"ils -L "+mytestdir,"LIST",mytestdir) # should be listed

@with_setup(s.admin_session_up,s.admin_session_down)
def test_local_imkdir_with_trailing_slash():
  # local setup
  mytestdir = "testingimkdirwithslash"
  # assertions
  assertiCmdFail(s.adminsession,"ils -L "+mytestdir+"/","LIST",mytestdir) # should not be listed
  assertiCmd(s.adminsession,"imkdir "+mytestdir+"/") # imkdir
  assertiCmd(s.adminsession,"ils -L "+mytestdir,"LIST",mytestdir) # should be listed

@with_setup(s.admin_session_up,s.admin_session_down)
def test_local_imkdir_with_trailing_slash_already_exists():
  # local setup
  mytestdir = "testingimkdirwithslash"
  # assertions
  assertiCmd(s.adminsession,"imkdir "+mytestdir+"/") # imkdir
  assertiCmdFail(s.adminsession,"imkdir "+mytestdir) # should fail, already exists
  assertiCmdFail(s.adminsession,"imkdir "+mytestdir+"/") # should fail, already exists

@with_setup(s.admin_session_up,s.admin_session_down)
def test_local_imkdir_when_dir_already_exists():
  # local setup
  mytestdir = "testingimkdiralreadyexists"
  # assertions
  assertiCmd(s.adminsession,"imkdir "+mytestdir) # imkdir
  assertiCmdFail(s.adminsession,"imkdir "+mytestdir) # should fail, already exists

@with_setup(s.admin_session_up,s.admin_session_down)
def test_local_imkdir_when_file_already_exists():
  # local setup
  # assertions
  assertiCmdFail(s.adminsession,"imkdir "+s.testfile) # should fail, filename already exists

@with_setup(s.admin_session_up,s.admin_session_down)
def test_local_imkdir_with_parent():
  # local setup
  mytestdir = "parent/testingimkdirwithparent"
  # assertions
  assertiCmdFail(s.adminsession,"ils -L "+mytestdir,"LIST",mytestdir) # should not be listed
  assertiCmd(s.adminsession,"imkdir -p "+mytestdir) # imkdir with parent
  assertiCmd(s.adminsession,"ils -L "+mytestdir,"LIST",mytestdir) # should be listed

@with_setup(s.admin_session_up,s.admin_session_down)
def test_local_imkdir_with_bad_option():
  # assertions
  assertiCmdFail(s.adminsession,"imkdir -z") # run imkdir with bad option

