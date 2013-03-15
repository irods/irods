import pydevtest_sessions as s
from nose.tools import with_setup
from nose.plugins.skip import SkipTest
from pydevtest_common import assertiCmd, assertiCmdFail
import commands

# LISTS

@with_setup(s.adminonly_up,s.adminonly_down)
def test_list_zones():
    assertiCmd(s.adminsession,"iadmin lz","LIST",s.adminsession.getZoneName())
    assertiCmdFail(s.adminsession,"iadmin lz","LIST","notazone")

@with_setup(s.adminonly_up,s.adminonly_down)
def test_list_resources():
    assertiCmd(s.adminsession,"iadmin lr","LIST",s.testresc)
    assertiCmdFail(s.adminsession,"iadmin lr","LIST","notaresource")

@with_setup(s.adminonly_up,s.adminonly_down)
def test_list_users():
    assertiCmd(s.adminsession,"iadmin lu","LIST","rods#"+s.adminsession.getZoneName())
    assertiCmdFail(s.adminsession,"iadmin lu","LIST","notauser")

@with_setup(s.twousers_up,s.twousers_down)
def test_list_groups():
    assertiCmd(s.adminsession,"iadmin lg","LIST",s.testgroup)
    assertiCmdFail(s.adminsession,"iadmin lg","LIST","notagroup")
    assertiCmd(s.adminsession,"iadmin lg "+s.testgroup,"LIST",[s.sessions[1].getUserName()])
    assertiCmd(s.adminsession,"iadmin lg "+s.testgroup,"LIST",[s.sessions[2].getUserName()])
    assertiCmdFail(s.adminsession,"iadmin lg "+s.testgroup,"LIST","notauser")

# RESOURCES

@with_setup(s.adminonly_up,s.adminonly_down)
def test_create_and_remove_new_resource():
    testresc1 = "testResc1"
    assertiCmdFail(s.adminsession,"iadmin lr","LIST",testresc1) # should not be listed
    output = commands.getstatusoutput("hostname")
    hostname = output[1]
    assertiCmd(s.adminsession,"iadmin mkresc "+testresc1+" \"unix file system\" "+hostname+":/tmp/pydevtest_"+testresc1) # unix
    assertiCmd(s.adminsession,"iadmin lr","LIST",testresc1) # should be listed
    assertiCmdFail(s.adminsession,"iadmin rmresc notaresource") # bad remove
    assertiCmd(s.adminsession,"iadmin rmresc "+testresc1) # good remove
    assertiCmdFail(s.adminsession,"iadmin lr","LIST",testresc1) # should be gone

@with_setup(s.adminonly_up,s.adminonly_down)
def test_modify_resource_comment():
    mycomment = "thisisacomment with some spaces"
    assertiCmdFail(s.adminsession,"iadmin lr "+s.testresc,"LIST",mycomment)
    assertiCmd(s.adminsession,"iadmin modresc "+s.testresc+" comment '"+mycomment+"'")
    assertiCmd(s.adminsession,"iadmin lr "+s.testresc,"LIST",mycomment)

# USERS

@with_setup(s.adminonly_up,s.adminonly_down)
def test_create_and_remove_new_user():
    testuser1 = "testaddandremoveuser"
    assertiCmdFail(s.adminsession,"iadmin lu","LIST",testuser1+"#"+s.adminsession.getZoneName()) # should not be listed
    assertiCmd(s.adminsession,"iadmin mkuser "+testuser1+" rodsuser") # add rodsuser
    assertiCmd(s.adminsession,"iadmin lu","LIST",testuser1+"#"+s.adminsession.getZoneName()) # should be listed
    assertiCmdFail(s.adminsession,"iadmin rmuser notauser") # bad remove
    assertiCmd(s.adminsession,"iadmin rmuser "+testuser1) # good remove
    assertiCmdFail(s.adminsession,"iadmin lu","LIST",testuser1+"#"+s.adminsession.getZoneName()) # should be gone
