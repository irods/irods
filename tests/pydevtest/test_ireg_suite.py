import time
import sys
import shutil
import os
if (sys.version_info >= (2, 7)):
    import unittest
else:
    import unittest2 as unittest
from resource_suite import ResourceBase
from pydevtest_common import assertiCmd, assertiCmdFail, interruptiCmd, getiCmdOutput, get_irods_top_level_dir
import pydevtest_sessions as s
import commands
import os
import shlex
import datetime
import time
import socket

RODSHOME = get_irods_top_level_dir() + "/iRODS/"
ABSPATHTESTDIR = os.path.abspath(os.path.dirname(sys.argv[0]))
RODSHOME = ABSPATHTESTDIR + "/../../iRODS"


class Test_ireg_Suite(unittest.TestCase, ResourceBase):

    my_test_resource = {"setup": [], "teardown": []}

    def setUp(self):
        ResourceBase.__init__(self)
        s.twousers_up()
        shutil.copy2(ABSPATHTESTDIR + '/test_ireg_suite.py', ABSPATHTESTDIR + '/file0')
        shutil.copy2(ABSPATHTESTDIR + '/test_ireg_suite.py', ABSPATHTESTDIR + '/file1')
        shutil.copy2(ABSPATHTESTDIR + '/test_ireg_suite.py', ABSPATHTESTDIR + '/file2')
        shutil.copy2(ABSPATHTESTDIR + '/test_ireg_suite.py', ABSPATHTESTDIR + '/file3')

        self.run_resource_setup()

        assertiCmd(s.adminsession, 'iadmin mkresc r_resc passthru', "LIST", "Creating")
        assertiCmd(s.adminsession, 'iadmin mkresc m_resc passthru', "LIST", "Creating")
        hostname = socket.gethostname()
        assertiCmd(s.adminsession, 'iadmin mkresc l_resc unixfilesystem ' +
                   hostname + ':/tmp/l_resc', "LIST", "Creating")

        assertiCmd(s.adminsession, "iadmin addchildtoresc r_resc m_resc")
        assertiCmd(s.adminsession, "iadmin addchildtoresc m_resc l_resc")

    def tearDown(self):
        assertiCmd(s.adminsession, "irm -f /tempZone/home/rods/file0")
        assertiCmd(s.adminsession, "irm -f /tempZone/home/rods/file1")
        assertiCmd(s.adminsession, "irm -f /tempZone/home/rods/file3")

        assertiCmd(s.adminsession, "irmtrash -M")
        assertiCmd(s.adminsession, "iadmin rmchildfromresc m_resc l_resc")
        assertiCmd(s.adminsession, "iadmin rmchildfromresc r_resc m_resc")

        assertiCmd(s.adminsession, "iadmin rmresc r_resc")
        assertiCmd(s.adminsession, "iadmin rmresc m_resc")
        assertiCmd(s.adminsession, "iadmin rmresc l_resc")

        os.remove(ABSPATHTESTDIR + '/file2')

        self.run_resource_teardown()
        s.twousers_down()

    def test_ireg_files(self):
        assertiCmd(s.adminsession, "ireg -R l_resc " + ABSPATHTESTDIR + "/file0 /tempZone/home/rods/file0", "EMPTY")
        assertiCmd(s.adminsession, "ils -l /tempZone/home/rods/file0", "LIST", "r_resc;m_resc;l_resc")

        assertiCmd(s.adminsession, "ireg -R r_resc " + ABSPATHTESTDIR + "/file1 /tempZone/home/rods/file1", "EMPTY")
        assertiCmd(s.adminsession, "ils -l /tempZone/home/rods/file1", "LIST", "r_resc;m_resc;l_resc")

        assertiCmd(s.adminsession, "ireg -R m_resc " + ABSPATHTESTDIR +
                   "/file2 /tempZone/home/rods/file2", "ERROR", "ERROR: regUtil: reg error for")

        assertiCmd(s.adminsession, "ireg -R demoResc " + ABSPATHTESTDIR + "/file3 /tempZone/home/rods/file3", "EMPTY")
