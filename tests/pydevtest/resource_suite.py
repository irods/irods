from nose.plugins.skip import SkipTest
from pydevtest_common import assertiCmd, assertiCmdFail
import pydevtest_sessions as s

class ResourceSuite(object):
    '''Define the tests to be run for a resource type.

    This class is designed to be used as a base class by developers
    when they write tests for their own resource plugins.
    '''

    def test_skip_me(self):
        raise SkipTest

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

    def test_empty_icd_user2(self):
        # assertions
        assertiCmd(s.sessions[2],"icd") # just go home
        assertiCmd(s.sessions[2],"ils","LIST","/"+s.sessions[2].getZoneName()+"/home/"+s.sessions[2].getUserName()+":") # listing

