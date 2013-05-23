from nose.plugins.skip import SkipTest
from pydevtest_common import assertiCmd, assertiCmdFail, interruptiCmd
import pydevtest_sessions as s
import commands

class Test_imkdir(object):

    ###################
    # imkdir
    ###################

    def test_local_imkdir(self):
        # local setup
        mytestdir = "testingimkdir"
        # assertions
        assertiCmdFail(s.adminsession,"ils -L "+mytestdir,"LIST",mytestdir) # should not be listed
        assertiCmd(s.adminsession,"imkdir "+mytestdir) # imkdir
        assertiCmd(s.adminsession,"ils -L "+mytestdir,"LIST",mytestdir) # should be listed

    def test_local_imkdir_with_trailing_slash(self):
        # local setup
        mytestdir = "testingimkdirwithslash"
        # assertions
        assertiCmdFail(s.adminsession,"ils -L "+mytestdir+"/","LIST",mytestdir) # should not be listed
        assertiCmd(s.adminsession,"imkdir "+mytestdir+"/") # imkdir
        assertiCmd(s.adminsession,"ils -L "+mytestdir,"LIST",mytestdir) # should be listed

    def test_local_imkdir_with_trailing_slash_already_exists(self):
        # local setup
        mytestdir = "testingimkdirwithslash"
        # assertions
        assertiCmd(s.adminsession,"imkdir "+mytestdir+"/") # imkdir
        assertiCmdFail(s.adminsession,"imkdir "+mytestdir) # should fail, already exists
        assertiCmdFail(s.adminsession,"imkdir "+mytestdir+"/") # should fail, already exists

    def test_local_imkdir_when_dir_already_exists(self):
        # local setup
        mytestdir = "testingimkdiralreadyexists"
        # assertions
        assertiCmd(s.adminsession,"imkdir "+mytestdir) # imkdir
        assertiCmdFail(s.adminsession,"imkdir "+mytestdir) # should fail, already exists

    def test_local_imkdir_when_file_already_exists(self):
        # local setup
        # assertions
        assertiCmdFail(s.adminsession,"imkdir "+s.testfile) # should fail, filename already exists

    def test_local_imkdir_with_parent(self):
        # local setup
        mytestdir = "parent/testingimkdirwithparent"
        # assertions
        assertiCmdFail(s.adminsession,"ils -L "+mytestdir,"LIST",mytestdir) # should not be listed
        assertiCmd(s.adminsession,"imkdir -p "+mytestdir) # imkdir with parent
        assertiCmd(s.adminsession,"ils -L "+mytestdir,"LIST",mytestdir) # should be listed

    def test_local_imkdir_with_bad_option(self):
        # assertions
        assertiCmdFail(s.adminsession,"imkdir -z") # run imkdir with bad option


