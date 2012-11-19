import pydevtest_sessions as s
from nose.tools import with_setup
from nose.plugins.skip import SkipTest
from pydevtest_common import assertiCmd, assertiCmdFail
import commands

@with_setup(s.adminonly_up,s.adminonly_down)
def test_irm_doesnotexist():
    assertiCmdFail(s.adminsession,"irm doesnotexist") # does not exist
    
@with_setup(s.adminonly_up,s.adminonly_down)
def test_irm():
    assertiCmd(s.adminsession,"ils -L "+s.testfile,"LIST",s.testfile) # should be listed
    assertiCmd(s.adminsession,"irm "+s.testfile) # remove from grid
    assertiCmdFail(s.adminsession,"ils -L "+s.testfile,"LIST",s.testfile) # should be deleted
    trashpath = "/"+s.adminsession.getZoneName()+"/trash/home/"+s.adminsession.getUserName()+"/"+s.adminsession.sessionId
    assertiCmd(s.adminsession,"ils -L "+trashpath+"/"+s.testfile,"LIST",s.testfile) # should be in trash

@with_setup(s.adminonly_up,s.adminonly_down)
def test_irm_force():
    assertiCmd(s.adminsession,"ils -L "+s.testfile,"LIST",s.testfile) # should be listed
    assertiCmd(s.adminsession,"irm -f "+s.testfile) # remove from grid
    assertiCmdFail(s.adminsession,"ils -L "+s.testfile,"LIST",s.testfile) # should be deleted
    trashpath = "/"+s.adminsession.getZoneName()+"/trash/home/"+s.adminsession.getUserName()+"/"+s.adminsession.sessionId
    assertiCmdFail(s.adminsession,"ils -L "+trashpath+"/"+s.testfile,"LIST",s.testfile) # should not be in trash

@with_setup(s.adminonly_up,s.adminonly_down)
def test_irm_specific_replica():
    assertiCmd(s.adminsession,"ils -L "+s.testfile,"LIST",s.testfile) # should be listed
    assertiCmd(s.adminsession,"irepl -R "+s.testresc+" "+s.testfile) # creates replica
    assertiCmd(s.adminsession,"ils -L "+s.testfile,"LIST",s.testfile) # should be listed twice
    assertiCmd(s.adminsession,"irm -n 0 "+s.testfile) # remove original from grid
    assertiCmd(s.adminsession,"ils -L "+s.testfile,"LIST",["1 "+s.testresc,s.testfile]) # replica 1 should be there
    assertiCmdFail(s.adminsession,"ils -L "+s.testfile,"LIST",["0 "+s.adminsession.getDefResource(),s.testfile]) # replica 0 should be gone
    trashpath = "/"+s.adminsession.getZoneName()+"/trash/home/"+s.adminsession.getUserName()+"/"+s.adminsession.sessionId
    assertiCmdFail(s.adminsession,"ils -L "+trashpath+"/"+s.testfile,"LIST",["0 "+s.adminsession.getDefResource(),s.testfile]) # replica should not be in trash

@with_setup(s.adminonly_up,s.adminonly_down)
def test_irm_recursive_file():
    assertiCmd(s.adminsession,"ils -L "+s.testfile,"LIST",s.testfile) # should be listed
    assertiCmd(s.adminsession,"irm -r "+s.testfile) # should not fail, even though a collection

@with_setup(s.adminonly_up,s.adminonly_down)
def test_irm_recursive():
    assertiCmd(s.adminsession,"icp -r "+s.testdir+" copydir") # make a dir copy
    assertiCmd(s.adminsession,"ils -L ","LIST","copydir") # should be listed
    assertiCmd(s.adminsession,"irm -r copydir") # should remove
    assertiCmdFail(s.adminsession,"ils -L ","LIST","copydir") # should not be listed
    
@with_setup(s.oneuser_up,s.oneuser_down)
def test_irm_with_read_permission():
    assertiCmd(s.sessions[1],"icd ../../public") # switch to shared area
    assertiCmd(s.sessions[1],"ils -AL "+s.testfile,"LIST",s.testfile) # should be listed
    assertiCmdFail(s.sessions[1],"irm "+s.testfile) # read perm should not be allowed to remove
    assertiCmd(s.sessions[1],"ils -AL "+s.testfile,"LIST",s.testfile) # should still be listed

@with_setup(s.twousers_up,s.twousers_down)
def test_irm_with_write_permission():
    assertiCmd(s.sessions[2],"icd ../../public") # switch to shared area
    assertiCmd(s.sessions[2],"ils -AL "+s.testfile,"LIST",s.testfile) # should be listed
    assertiCmdFail(s.sessions[2],"irm "+s.testfile) # write perm should not be allowed to remove
    assertiCmd(s.sessions[2],"ils -AL "+s.testfile,"LIST",s.testfile) # should still be listed

