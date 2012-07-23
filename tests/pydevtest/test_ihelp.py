import pydevtest_sessions as s
from nose import with_setup
from pydevtest_common import assertiCmd, assertiCmdFail
import commands

@with_setup(s.admin_session_up,s.admin_session_down)
def test_local_ihelp():
  # assertions
  assertiCmd(s.adminsession,"ihelp","LIST","The following is a list of the icommands") # run ihelp

@with_setup(s.admin_session_up,s.admin_session_down)
def test_local_ihelp_with_help():
  # assertions
  assertiCmd(s.adminsession,"ihelp -h","LIST","Display i-commands synopsis") # run ihelp with help

@with_setup(s.admin_session_up,s.admin_session_down)
def test_local_ihelp_all():
  # assertions
  assertiCmd(s.adminsession,"ihelp -a","LIST","Usage") # run ihelp on all icommands

@with_setup(s.admin_session_up,s.admin_session_down)
def test_local_ihelp_with_good_icommand():
  # assertions
  assertiCmd(s.adminsession,"ihelp ils","LIST","Usage") # run ihelp with good icommand

@with_setup(s.admin_session_up,s.admin_session_down)
def test_local_ihelp_with_bad_icommand():
  # assertions
  assertiCmdFail(s.adminsession,"ihelp idoesnotexist") # run ihelp with bad icommand

@with_setup(s.admin_session_up,s.admin_session_down)
def test_local_ihelp_with_bad_option():
  # assertions
  assertiCmdFail(s.adminsession,"ihelp -z") # run ihelp with bad option


