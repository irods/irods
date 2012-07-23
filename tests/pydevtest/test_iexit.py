import pydevtest_sessions as s
from nose import with_setup
from pydevtest_common import assertiCmd, assertiCmdFail
import commands

@with_setup(s.admin_session_up,s.admin_session_down)
def test_iexit():
  # assertions
  assertiCmd(s.adminsession,"iexit") # just go home

@with_setup(s.admin_session_up,s.admin_session_down)
def test_iexit_verbose():
  # assertions
  assertiCmd(s.adminsession,"iexit -v","LIST","Deleting (if it exists) session envFile:") # home, verbose

@with_setup(s.admin_session_up,s.admin_session_down)
def test_iexit_with_bad_option():
  # assertions
  assertiCmdFail(s.adminsession,"iexit -z") # run iexit with bad option

