from nose.plugins.skip import SkipTest
from pydevtest_common import assertiCmd, assertiCmdFail, interruptiCmd
import pydevtest_sessions as s
import commands

class Test_icd(object):

    ###################
    # icd
    ###################

    def test_empty_icd(self):
        # assertions
        assertiCmd(s.adminsession,"icd "+s.testdir) # get into subdir
        assertiCmd(s.adminsession,"icd") # just go home
        assertiCmd(s.adminsession,"ils","LIST","/"+s.adminsession.getZoneName()+"/home/"+s.adminsession.getUserName()+":") # listing

    def test_empty_icd_verbose(self):
        # assertions
        assertiCmd(s.adminsession,"icd "+s.testdir) # get into subdir
        assertiCmd(s.adminsession,"icd -v","LIST","Deleting (if it exists) session envFile:") # home, verbose
        assertiCmd(s.adminsession,"ils","LIST","/"+s.adminsession.getZoneName()+"/home/"+s.adminsession.getUserName()+":") # listing

    def test_icd_to_subdir(self):
        # assertions
        assertiCmd(s.adminsession,"icd "+s.testdir) # get into subdir
        assertiCmd(s.adminsession,"ils","LIST","/"+s.adminsession.getZoneName()+"/home/"+s.adminsession.getUserName()+"/"+s.adminsession.sessionId+"/"+s.testdir+":") # listing

    def test_icd_to_parentdir(self):
        # assertions
        assertiCmd(s.adminsession,"icd ..") # go to parent
        assertiCmd(s.adminsession,"ils","LIST","/"+s.adminsession.getZoneName()+"/home/"+s.adminsession.getUserName()+":") # listing

    def test_icd_to_root(self):
        # assertions
        assertiCmd(s.adminsession,"icd /") # go to root
        assertiCmd(s.adminsession,"ils","LIST","/:") # listing

    def test_icd_to_root_with_badpath(self):
        # assertions
        assertiCmd(s.adminsession,"icd /doesnotexist","LIST","No such directory (collection):") # go to root with bad path


