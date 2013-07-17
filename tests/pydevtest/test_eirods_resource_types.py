import unittest
from pydevtest_common import assertiCmd, assertiCmdFail
import pydevtest_sessions as s
from resource_suite import ResourceSuite, ShortAndSuite
from test_chunkydevtest import ChunkyDevTest
import socket

class Test_UnixFileSystem_Resource(unittest.TestCase, ResourceSuite, ChunkyDevTest):

    my_test_resource = {
        "setup"    : [  
        ],
        "teardown" : [
        ],
    }

    def setUp(self):
        ResourceSuite.__init__(self)
        s.twousers_up()
        self.run_resource_setup()

    def tearDown(self):
        self.run_resource_teardown()
        s.twousers_down()

class Test_Passthru_Resource(unittest.TestCase, ResourceSuite, ChunkyDevTest):

    hostname = socket.gethostname()
    my_test_resource = {
        "setup"    : [  
            "iadmin modresc demoResc name origResc",
            "iadmin mkresc demoResc passthru",
            "iadmin mkresc unix1Resc 'unix file system' "+hostname+":/var/lib/eirods/unix1RescVault",
            "iadmin addchildtoresc demoResc unix1Resc",
        ],
        "teardown" : [ 
            "iadmin rmchildfromresc demoResc unix1Resc",
            "iadmin rmresc unix1Resc",
            "iadmin rmresc demoResc",
            "iadmin modresc origResc name demoResc",
            "rm -rf /var/lib/eirods/unix1RescVault",
        ],
    }

    def setUp(self):
        ResourceSuite.__init__(self)
        s.twousers_up()
        self.run_resource_setup()

    def tearDown(self):
        self.run_resource_teardown()
        s.twousers_down()

class Test_Random_Resource(unittest.TestCase, ResourceSuite, ChunkyDevTest):

    hostname = socket.gethostname()
    my_test_resource = {
        "setup"    : [
            "iadmin modresc demoResc name origResc",
            "iadmin mkresc demoResc random",
            "iadmin mkresc unix1Resc 'unix file system' "+hostname+":/var/lib/eirods/unix1RescVault",
            "iadmin mkresc unix2Resc 'unix file system' "+hostname+":/var/lib/eirods/unix2RescVault",
            "iadmin mkresc unix3Resc 'unix file system' "+hostname+":/var/lib/eirods/unix3RescVault",
            "iadmin addchildtoresc demoResc unix1Resc",
            "iadmin addchildtoresc demoResc unix2Resc",
            "iadmin addchildtoresc demoResc unix3Resc",
        ],
        "teardown" : [
            "iadmin rmchildfromresc demoResc unix3Resc",
            "iadmin rmchildfromresc demoResc unix2Resc",
            "iadmin rmchildfromresc demoResc unix1Resc",
            "iadmin rmresc unix3Resc",
            "iadmin rmresc unix2Resc",
            "iadmin rmresc unix1Resc",
            "iadmin rmresc demoResc",
            "iadmin modresc origResc name demoResc",
            "rm -rf /var/lib/eirods/unix1RescVault",
            "rm -rf /var/lib/eirods/unix2RescVault",
            "rm -rf /var/lib/eirods/unix3RescVault",
        ],
    }

    def setUp(self):
        ResourceSuite.__init__(self)
        s.twousers_up()
        self.run_resource_setup()

    def tearDown(self):
        self.run_resource_teardown()
        s.twousers_down()

class Test_NonBlocking_Resource(unittest.TestCase, ResourceSuite, ChunkyDevTest):

    hostname = socket.gethostname()
    my_test_resource = {
        "setup"    : [
            "iadmin modresc demoResc name origResc",
            "iadmin mkresc demoResc nonblocking "+hostname+":/var/lib/eirods/nbVault",
        ],
        "teardown" : [
            "iadmin rmresc demoResc",
            "iadmin modresc origResc name demoResc",
        ],
    }

    def setUp(self):
        ResourceSuite.__init__(self)
        s.twousers_up()
        self.run_resource_setup()

    def tearDown(self):
        self.run_resource_teardown()
        s.twousers_down()

class Test_Compound_with_MockArchive_Resource(unittest.TestCase, ResourceSuite):

    hostname = socket.gethostname()
    my_test_resource = {
        "setup"    : [
            "iadmin modresc demoResc name origResc",
            "iadmin mkresc demoResc compound",
            "iadmin mkresc cacheResc 'unix file system' "+hostname+":/var/lib/eirods/cacheRescVault",
            "iadmin mkresc archiveResc mockarchive "+hostname+":/var/lib/eirods/archiveRescVault univMSSInterface.sh",
            "iadmin addchildtoresc demoResc cacheResc cache",
            "iadmin addchildtoresc demoResc archiveResc archive",
        ],
        "teardown" : [
            "iadmin rmchildfromresc demoResc archiveResc",
            "iadmin rmchildfromresc demoResc cacheResc",
            "iadmin rmresc archiveResc",
            "iadmin rmresc cacheResc",
            "iadmin rmresc demoResc",
            "iadmin modresc origResc name demoResc",
            "rm -rf /var/lib/eirods/archiveRescVault",
            "rm -rf /var/lib/eirods/cacheRescVault",
        ],
    }

    def setUp(self):
        ResourceSuite.__init__(self)
        s.twousers_up()
        self.run_resource_setup()

    def tearDown(self):
        self.run_resource_teardown()
        s.twousers_down()

    def test_irm_specific_replica(self):
        assertiCmd(s.adminsession,"ils -L "+self.testfile,"LIST",self.testfile) # should be listed
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+self.testfile) # creates replica
        assertiCmd(s.adminsession,"ils -L "+self.testfile,"LIST",self.testfile) # should be listed twice
        assertiCmd(s.adminsession,"irm -n 0 "+self.testfile) # remove original from cacheResc only
        assertiCmd(s.adminsession,"ils -L "+self.testfile,"LIST",["2 "+self.testresc,self.testfile]) # replica 2 should still be there
        assertiCmdFail(s.adminsession,"ils -L "+self.testfile,"LIST",["0 "+s.adminsession.getDefResource(),self.testfile]) # replica 0 should be gone
        trashpath = "/"+s.adminsession.getZoneName()+"/trash/home/"+s.adminsession.getUserName()+"/"+s.adminsession.sessionId
        assertiCmdFail(s.adminsession,"ils -L "+trashpath+"/"+self.testfile,"LIST",["0 "+s.adminsession.getDefResource(),self.testfile]) # replica should not be in trash

    @unittest.skip("NOTSURE / FIXME ... -K not supported, perhaps")
    def test_local_iput_checksum(self):
        pass

class Test_Compound_with_UniversalMSS_Resource(unittest.TestCase, ResourceSuite):

    hostname = socket.gethostname()
    my_test_resource = {
        "setup"    : [
            "iadmin modresc demoResc name origResc",
            "iadmin mkresc demoResc compound",
            "iadmin mkresc cacheResc 'unix file system' "+hostname+":/var/lib/eirods/cacheRescVault",
            "iadmin mkresc archiveResc univmss "+hostname+":/var/lib/eirods/archiveRescVault univMSSInterface.sh",
            "iadmin addchildtoresc demoResc cacheResc cache",
            "iadmin addchildtoresc demoResc archiveResc archive",
        ],
        "teardown" : [
            "iadmin rmchildfromresc demoResc archiveResc",
            "iadmin rmchildfromresc demoResc cacheResc",
            "iadmin rmresc archiveResc",
            "iadmin rmresc cacheResc",
            "iadmin rmresc demoResc",
            "iadmin modresc origResc name demoResc",
            "rm -rf /var/lib/eirods/archiveRescVault",
            "rm -rf /var/lib/eirods/cacheRescVault",
        ],
    }

    def setUp(self):
        ResourceSuite.__init__(self)
        s.twousers_up()
        self.run_resource_setup()

    def tearDown(self):
        self.run_resource_teardown()
        s.twousers_down()

    def test_irm_specific_replica(self):
        assertiCmd(s.adminsession,"ils -L "+self.testfile,"LIST",self.testfile) # should be listed
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+self.testfile) # creates replica
        assertiCmd(s.adminsession,"ils -L "+self.testfile,"LIST",self.testfile) # should be listed twice
        assertiCmd(s.adminsession,"irm -n 0 "+self.testfile) # remove original from cacheResc only
        assertiCmd(s.adminsession,"ils -L "+self.testfile,"LIST",["2 "+self.testresc,self.testfile]) # replica 2 should still be there
        assertiCmdFail(s.adminsession,"ils -L "+self.testfile,"LIST",["0 "+s.adminsession.getDefResource(),self.testfile]) # replica 0 should be gone
        trashpath = "/"+s.adminsession.getZoneName()+"/trash/home/"+s.adminsession.getUserName()+"/"+s.adminsession.sessionId
        assertiCmdFail(s.adminsession,"ils -L "+trashpath+"/"+self.testfile,"LIST",["0 "+s.adminsession.getDefResource(),self.testfile]) # replica should not be in trash

class Test_Compound_Resource(unittest.TestCase, ResourceSuite):

    hostname = socket.gethostname()
    my_test_resource = {
        "setup"    : [
            "iadmin modresc demoResc name origResc",
            "iadmin mkresc demoResc compound",
            "iadmin mkresc cacheResc 'unix file system' "+hostname+":/var/lib/eirods/cacheRescVault",
            "iadmin mkresc archiveResc 'unix file system' "+hostname+":/var/lib/eirods/archiveRescVault",
            "iadmin addchildtoresc demoResc cacheResc cache",
            "iadmin addchildtoresc demoResc archiveResc archive",
        ],
        "teardown" : [
            "iadmin rmchildfromresc demoResc archiveResc",
            "iadmin rmchildfromresc demoResc cacheResc",
            "iadmin rmresc archiveResc",
            "iadmin rmresc cacheResc",
            "iadmin rmresc demoResc",
            "iadmin modresc origResc name demoResc",
            "rm -rf /var/lib/eirods/archiveRescVault",
            "rm -rf /var/lib/eirods/cacheRescVault",
        ],
    }

    def setUp(self):
        ResourceSuite.__init__(self)
        s.twousers_up()
        self.run_resource_setup()

    def tearDown(self):
        self.run_resource_teardown()
        s.twousers_down()

    def test_irm_specific_replica(self):
        assertiCmd(s.adminsession,"ils -L "+self.testfile,"LIST",self.testfile) # should be listed
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+self.testfile) # creates replica
        assertiCmd(s.adminsession,"ils -L "+self.testfile,"LIST",self.testfile) # should be listed twice
        assertiCmd(s.adminsession,"irm -n 0 "+self.testfile) # remove original from cacheResc only
        assertiCmd(s.adminsession,"ils -L "+self.testfile,"LIST",["2 "+self.testresc,self.testfile]) # replica 2 should still be there
        assertiCmdFail(s.adminsession,"ils -L "+self.testfile,"LIST",["0 "+s.adminsession.getDefResource(),self.testfile]) # replica 0 should be gone
        trashpath = "/"+s.adminsession.getZoneName()+"/trash/home/"+s.adminsession.getUserName()+"/"+s.adminsession.sessionId
        assertiCmdFail(s.adminsession,"ils -L "+trashpath+"/"+self.testfile,"LIST",["0 "+s.adminsession.getDefResource(),self.testfile]) # replica should not be in trash

class Test_RoundRobin_Resource(unittest.TestCase, ResourceSuite, ChunkyDevTest):

    hostname = socket.gethostname()
    my_test_resource = {
        "setup"    : [
            "iadmin modresc demoResc name origResc",
            "iadmin mkresc demoResc roundrobin",
            "iadmin mkresc unix1Resc 'unix file system' "+hostname+":/var/lib/eirods/unix1RescVault",
            "iadmin mkresc unix2Resc 'unix file system' "+hostname+":/var/lib/eirods/unix2RescVault",
            "iadmin addchildtoresc demoResc unix1Resc",
            "iadmin addchildtoresc demoResc unix2Resc",
        ],
        "teardown" : [
            "iadmin rmchildfromresc demoResc unix2Resc",
            "iadmin rmchildfromresc demoResc unix1Resc",
            "iadmin rmresc unix2Resc",
            "iadmin rmresc unix1Resc",
            "iadmin rmresc demoResc",
            "iadmin modresc origResc name demoResc",
            "rm -rf /var/lib/eirods/unix1RescVault",
            "rm -rf /var/lib/eirods/unix2RescVault",
        ],
    }

    def setUp(self):
        ResourceSuite.__init__(self)
        s.twousers_up()
        self.run_resource_setup()

    def tearDown(self):
        self.run_resource_teardown()
        s.twousers_down()

class Test_Replication_Resource(unittest.TestCase, ResourceSuite):

    hostname = socket.gethostname()
    my_test_resource = {
        "setup"    : [
            "iadmin modresc demoResc name origResc",
            "iadmin mkresc demoResc replication",
            "iadmin mkresc unix1Resc 'unix file system' "+hostname+":/var/lib/eirods/unix1RescVault",
            "iadmin mkresc unix2Resc 'unix file system' "+hostname+":/var/lib/eirods/unix2RescVault",
            "iadmin mkresc unix3Resc 'unix file system' "+hostname+":/var/lib/eirods/unix3RescVault",
            "iadmin addchildtoresc demoResc unix1Resc",
            "iadmin addchildtoresc demoResc unix2Resc",
            "iadmin addchildtoresc demoResc unix3Resc",
        ],
        "teardown" : [
            "iadmin rmchildfromresc demoResc unix3Resc",
            "iadmin rmchildfromresc demoResc unix2Resc",
            "iadmin rmchildfromresc demoResc unix1Resc",
            "iadmin rmresc unix3Resc",
            "iadmin rmresc unix2Resc",
            "iadmin rmresc unix1Resc",
            "iadmin rmresc demoResc",
            "iadmin modresc origResc name demoResc",
            "rm -rf /var/lib/eirods/unix1RescVault",
            "rm -rf /var/lib/eirods/unix2RescVault",
            "rm -rf /var/lib/eirods/unix3RescVault",
        ],
    }

    def setUp(self):
        ResourceSuite.__init__(self)
        s.twousers_up()
        self.run_resource_setup()

    def tearDown(self):
        self.run_resource_teardown()
        s.twousers_down()

    def test_irm_specific_replica(self):
        # not allowed here - this is a managed replication resource
        assertiCmd(s.adminsession,"ils -L "+self.testfile,"LIST",self.testfile) # should be listed 3x
        assertiCmd(s.adminsession,"irm -n 0 "+self.testfile, "ERROR", "-1811000 EIRODS_INVALID_OPERATION") # try to remove one of the managed replicas

class Test_MultiLayered_Resource(unittest.TestCase, ResourceSuite, ChunkyDevTest):

    hostname = socket.gethostname()
    my_test_resource = {
        "setup"    : [
            "iadmin modresc demoResc name origResc",
            "iadmin mkresc demoResc passthru",
            "iadmin mkresc pass2Resc passthru",
            "iadmin mkresc rrResc roundrobin",
            "iadmin mkresc unix1Resc 'unix file system' "+hostname+":/var/lib/eirods/unix1RescVault",
            "iadmin mkresc unix2Resc 'unix file system' "+hostname+":/var/lib/eirods/unix2RescVault",
            "iadmin mkresc unix3Resc 'unix file system' "+hostname+":/var/lib/eirods/unix3RescVault",
            "iadmin addchildtoresc demoResc pass2Resc",
            "iadmin addchildtoresc pass2Resc rrResc",
            "iadmin addchildtoresc rrResc unix1Resc",
            "iadmin addchildtoresc rrResc unix2Resc",
            "iadmin addchildtoresc rrResc unix3Resc",
        ],
        "teardown" : [
            "iadmin rmchildfromresc rrResc unix3Resc",
            "iadmin rmchildfromresc rrResc unix2Resc",
            "iadmin rmchildfromresc rrResc unix1Resc",
            "iadmin rmchildfromresc pass2Resc rrResc",
            "iadmin rmchildfromresc demoResc pass2Resc",
            "iadmin rmresc unix3Resc",
            "iadmin rmresc unix2Resc",
            "iadmin rmresc unix1Resc",
            "iadmin rmresc rrResc",
            "iadmin rmresc pass2Resc",
            "iadmin rmresc demoResc",
            "iadmin modresc origResc name demoResc",
            "rm -rf /var/lib/eirods/unix1RescVault",
            "rm -rf /var/lib/eirods/unix2RescVault",
            "rm -rf /var/lib/eirods/unix3RescVault",
        ],
    }

    def setUp(self):
        ResourceSuite.__init__(self)
        s.twousers_up()
        self.run_resource_setup()

    def tearDown(self):
        self.run_resource_teardown()
        s.twousers_down()

