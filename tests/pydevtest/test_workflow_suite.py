import time
import sys
import shutil
import os
if (sys.version_info >= (2, 7)):
    import unittest
else:
    import unittest2 as unittest
from resource_suite import ResourceBase
from pydevtest_common import assertiCmd, assertiCmdFail, interruptiCmd, getiCmdOutput
import pydevtest_common
import pydevtest_sessions as s
import commands
import os
import shlex
import datetime
import time

ABSPATHTESTDIR = os.path.abspath(os.path.dirname(sys.argv[0]))
RODSHOME = ABSPATHTESTDIR + "/../../iRODS"
FILESDIR = ABSPATHTESTDIR + '/workflow_testfiles'


class Test_WorkflowSuite(unittest.TestCase, ResourceBase):

    my_test_resource = {"setup": [], "teardown": []}

    def setUp(self):
        ResourceBase.__init__(self)
        s.twousers_up()
        shutil.copy2('/bin/cp', RODSHOME + '/server/bin/cmd/mycp')
        shutil.copy2(FILESDIR + '/myWorkFlow', RODSHOME + '/server/bin/cmd/')
        shutil.copy2(FILESDIR + '/tt', RODSHOME + '/server/bin/cmd/')
        assertiCmd(s.adminsession, "iput -f " + FILESDIR + "/PHOTO.JPG /tempZone/home/rods")
        getiCmdOutput(s.adminsession, "irm -rf /tempZone/home/rods/workflow")
        assertiCmd(s.adminsession, "imkdir -p /tempZone/home/rods/workflow")

        self.run_resource_setup()

    def tearDown(self):
        assertiCmd(s.adminsession, "irm -f /tempZone/home/rods/PHOTO.JPG")
        assertiCmd(s.adminsession, "irm -rf /tempZone/home/rods/workflow/myWf")
        assertiCmd(s.adminsession, "irm -rf /tempZone/home/rods/workflow")
        self.run_resource_teardown()
        s.twousers_down()
        os.remove(RODSHOME + '/server/bin/cmd/mycp')
        os.remove(RODSHOME + '/server/bin/cmd/myWorkFlow')
        os.remove(RODSHOME + '/server/bin/cmd/tt')
        try:
            os.remove(RODSHOME + '/server/bin/cmd/PHOTO.JPG')
        except OSError:
            pass
        try:
            os.remove(RODSHOME + '/server/bin/cmd/myData')
        except OSError:
            pass

    @unittest.skipIf(pydevtest_common.irods_test_constants.RUN_AS_RESOURCE_SERVER, "Skip for topology testing from resource server")
    def test_workflow_set_one_up(self):

        # setup

        # ingest workflow file
        assertiCmd(s.adminsession, 'iput -D "msso file" ' + FILESDIR +
                   '/myWf.mss /tempZone/home/rods/workflow/myWf.mss')
        assertiCmd(s.adminsession, "ils -L /tempZone/home/rods/workflow", "LIST", "myWf.mss")

        # create WSO collection and associate with MSO
        assertiCmd(s.adminsession, "imkdir -p /tempZone/home/rods/workflow/myWf")
        assertiCmd(
            s.adminsession, "imcoll -m msso /tempZone/home/rods/workflow/myWf.mss /tempZone/home/rods/workflow/myWf")

        # ingest param file into WSO (there can be more than one)
        assertiCmd(s.adminsession, "iput " + FILESDIR + "/myWf.mpf /tempZone/home/rods/workflow/myWf")

        # put any data to be used by workflow into WSO collection
        assertiCmd(s.adminsession, "iput " + FILESDIR + "/myData /tempZone/home/rods/workflow/myWf/myData")

        # run the workflow
        assertiCmd(s.adminsession, "iget /tempZone/home/rods/workflow/myWf/myWf.run -",
                   "LIST", "Workflow Executed Successfully.")

        # teardown
        assertiCmd(s.adminsession, "imcoll -U /tempZone/home/rods/workflow/myWf")
