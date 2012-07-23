import pydevtest_sessions as s
from nose import with_setup
from pydevtest_common import assertiCmd, assertiCmdFail
import commands

# LISTS

@with_setup(s.admin_session_up,s.admin_session_down)
def test_list_zones():
  assertiCmd(s.adminsession,"iadmin lz","LIST",s.myzone)
  assertiCmdFail(s.adminsession,"iadmin lz","LIST","notazone")

@with_setup(s.admin_session_up,s.admin_session_down)
def test_list_resources():
  assertiCmd(s.adminsession,"iadmin lr","LIST",s.myresc)
  assertiCmdFail(s.adminsession,"iadmin lr","LIST","notaresource")

@with_setup(s.admin_session_up,s.admin_session_down)
def test_list_users():
  assertiCmd(s.adminsession,"iadmin lu","LIST","rods#"+s.myzone)
  assertiCmdFail(s.adminsession,"iadmin lu","LIST","notauser")

# USERS

@with_setup(s.admin_session_up,s.admin_session_down)
def test_create_and_remove_new_user():
  testuser1 = "testuser1"
  assertiCmdFail(s.adminsession,"iadmin lu","LIST",testuser1+"#"+s.myzone) # should not be listed
  assertiCmd(s.adminsession,"iadmin mkuser "+testuser1+" rodsuser") # add rodsuser
  assertiCmd(s.adminsession,"iadmin lu","LIST",testuser1+"#"+s.myzone) # should be listed
  assertiCmdFail(s.adminsession,"iadmin rmuser notauser") # bad remove
  assertiCmd(s.adminsession,"iadmin rmuser "+testuser1) # good remove
  assertiCmdFail(s.adminsession,"iadmin lu","LIST",testuser1+"#"+s.myzone) # should be gone

