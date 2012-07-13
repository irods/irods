import sessions as s
from nose import with_setup
from zonetests_common import assertiCmd, assertiCmdFail
import commands

@with_setup(s.admin_session_up,s.admin_session_down)
def test_local_imkdir():
  # local setup
  testdir = "testdir"
  # assertions
  assertiCmdFail(s.adminsession,"ils -L "+testdir,"LIST",testdir) # should not be listed
  assertiCmd(s.adminsession,"imkdir "+testdir) # imkdir
  assertiCmd(s.adminsession,"ils -L "+testdir,"LIST",testdir) # should be listed

@with_setup(s.admin_session_up,s.admin_session_down)
def test_local_imkdir_already_exists():
  # local setup
  testdir = "testdir"
  # assertions
  assertiCmd(s.adminsession,"imkdir "+testdir) # imkdir
  assertiCmdFail(s.adminsession,"imkdir "+testdir) # should fail, already exists

@with_setup(s.admin_session_up,s.admin_session_down)
def test_local_imkdir_already_exists():
  # local setup
  testdir = "testdir"
  # assertions
  assertiCmd(s.adminsession,"imkdir "+testdir) # imkdir
  assertiCmdFail(s.adminsession,"imkdir "+testdir) # should fail, already exists

@with_setup(s.admin_session_up,s.admin_session_down)
def test_local_imkdir_with_parent():
  # local setup
  testdir = "parent/testdir"
  # assertions
  assertiCmdFail(s.adminsession,"ils -L "+testdir,"LIST",testdir) # should not be listed
  assertiCmd(s.adminsession,"imkdir -p "+testdir) # imkdir with parent
  assertiCmd(s.adminsession,"ils -L "+testdir,"LIST",testdir) # should be listed
