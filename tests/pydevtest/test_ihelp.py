import pydevtest_sessions as s
from nose import with_setup
from pydevtest_common import assertiCmd, assertiCmdFail
import commands

@with_setup(s.admin_session_up,s.admin_session_down)
def test_local_ihelp_all():
  # local setup
  # assertions
  assertiCmd(s.adminsession,"ihelp -a","LIST","Usage") # run ihelp on all icommands
  # local cleanup

