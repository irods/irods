import sys
if (sys.version_info >= (2,7)):
    import unittest
else:
    import unittest2 as unittest
from pydevtest_common import assertiCmd, assertiCmdFail, getiCmdOutput, create_local_testfile, get_hostname
import pydevtest_sessions as s
from resource_suite import ResourceSuite, ShortAndSuite
from test_chunkydevtest import ChunkyDevTest
import socket
import os
import commands
import shutil
import subprocess
import re

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

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass

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

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass

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

class Test_Compound_with_MockArchive_Resource(unittest.TestCase, ResourceSuite, ChunkyDevTest):

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

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass

    def test_local_iput_with_force_and_destination_resource__ticket_1706(self):
        # local setup
        filename = "iputwithforceanddestination.txt"
        filepath = create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist")                           # should not be listed
        assertiCmd(s.adminsession,"iput "+filename)                                                      # put file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)                                # replicate to test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)                                    # 
        assertiCmd(s.adminsession,"iput -f -R %s %s %s" % (self.testresc, doublefile, filename) )        # overwrite test repl with different data
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," "+filename])                        # default resource cache should have dirty copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," "+filename])                        # default resource archive should have dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," "+doublesize+" "," "+filename]) # default resource cache should not have doublesize file
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," "+doublesize+" "," "+filename]) # default resource archive should not have doublesize file
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," "+doublesize+" ","& "+filename])    # targeted resource should have new double clean copy
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    ###################
    # irepl
    ###################

    def test_irepl_update_replicas(self):
        # local setup
        filename = "updatereplicasfile.txt"
        filepath = create_local_testfile(filename)
        hostname = get_hostname()
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)

        # assertions
        assertiCmd(s.adminsession,"iadmin mkresc thirdresc unixfilesystem %s:/tmp/thirdrescVault" % hostname)   # create third resource
        assertiCmd(s.adminsession,"iadmin mkresc fourthresc unixfilesystem %s:/tmp/fourthrescVault" % hostname) # create fourth resource
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist")              # should not be listed
        assertiCmd(s.adminsession,"iput "+filename)                                         # put file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)                   # replicate to test resource
        assertiCmd(s.adminsession,"irepl -R thirdresc "+filename)                           # replicate to third resource
        assertiCmd(s.adminsession,"irepl -R fourthresc "+filename)                          # replicate to fourth resource
        assertiCmd(s.adminsession,"iput -f -R "+self.testresc+" "+doublefile+" "+filename)  # repave overtop test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)                       # for debugging

        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," & "+filename]) # should have a dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," & "+filename]) # should have a dirty copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," & "+filename])     # should have a clean copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," & "+filename]) # should have a dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 4 "," & "+filename]) # should have a dirty copy

        assertiCmd(s.adminsession,"irepl -U "+filename)                                 # update last replica

        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," & "+filename]) # should have a dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," & "+filename]) # should have a dirty copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," & "+filename])     # should have a clean copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," & "+filename]) # should have a dirty copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 4 "," & "+filename])     # should have a clean copy

        assertiCmd(s.adminsession,"irepl -aU "+filename)                                # update all replicas

        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," & "+filename])     # should have a clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," & "+filename])     # should have a clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," & "+filename])     # should have a clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," & "+filename])     # should have a clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 4 "," & "+filename])     # should have a clean copy

        assertiCmd(s.adminsession,"irm -f "+filename)                                   # cleanup file
        assertiCmd(s.adminsession,"iadmin rmresc thirdresc")                            # remove third resource
        assertiCmd(s.adminsession,"iadmin rmresc fourthresc")                           # remove third resource

        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)
    def test_irepl_over_existing_second_replica__ticket_1705(self):
        # local setup
        filename = "secondreplicatest.txt"
        filepath = create_local_testfile(filename)
        # assertions
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist")          # should not be listed
        assertiCmd(s.adminsession,"iput -R "+self.testresc+" "+filename)                # put file
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)                   # for debugging
        assertiCmd(s.adminsession,"irepl "+filename)                                    # replicate to default resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)                   # for debugging
        assertiCmd(s.adminsession,"irepl "+filename)                                    # replicate overtop default resource
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," & "+filename]) # should not have a replica 3
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)               # replicate overtop test resource
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," & "+filename]) # should not have a replica 3
        # local cleanup
        os.remove(filepath)

    def test_irepl_over_existing_third_replica__ticket_1705(self):
        # local setup
        filename = "thirdreplicatest.txt"
        filepath = create_local_testfile(filename)
        hostname = get_hostname()
        # assertions
        assertiCmd(s.adminsession,"iadmin mkresc thirdresc unixfilesystem %s:/tmp/thirdrescVault" % hostname) # create third resource
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist") # should not be listed
        assertiCmd(s.adminsession,"iput "+filename)                            # put file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)      # replicate to test resource
        assertiCmd(s.adminsession,"irepl -R thirdresc "+filename)              # replicate to third resource
        assertiCmd(s.adminsession,"irepl "+filename)                           # replicate overtop default resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)      # replicate overtop test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmd(s.adminsession,"irepl -R thirdresc "+filename)              # replicate overtop third resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 4 "," & "+filename]) # should not have a replica 4
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 5 "," & "+filename]) # should not have a replica 5
        assertiCmd(s.adminsession,"irm -f "+filename)                          # cleanup file
        assertiCmd(s.adminsession,"iadmin rmresc thirdresc")                   # remove third resource
        # local cleanup
        os.remove(filepath)

    def test_irepl_over_existing_bad_replica__ticket_1705(self):
        # local setup
        filename = "reploverwritebad.txt"
        filepath = create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist") # should not be listed
        assertiCmd(s.adminsession,"iput "+filename)                            # put file
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)      # replicate to test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmd(s.adminsession,"iput -f %s %s" % (doublefile, filename) )   # overwrite default repl with different data
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," & "+filename]) # default resource cache should have clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," "+doublesize+" "," & "+filename]) # default resource cache should have new double clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," & "+filename]) # default resource archive should have clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," "+doublesize+" "," & "+filename]) # default resource archive should have new double clean copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 2 "+self.testresc," "+doublesize+" ","  "+filename]) # test resource should not have doublesize file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)      # replicate back onto test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 "+self.testresc," "+doublesize+" "," & "+filename]) # test resource should have new clean doublesize file
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," & "+filename]) # should not have a replica 3
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    def test_iput_with_purgec(self):
        # local setup
        filename = "purgecfile.txt"
        filepath = os.path.abspath(filename)
        f = open(filepath,'wb')
        f.write("TESTFILE -- ["+filepath+"]")
        f.close()

        # assertions
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",filename) # should not be listed
        assertiCmd(s.adminsession,"iput --purgec "+filename) # put file
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 ",filename]) # should not be listed (trimmed)
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 ",filename]) # should be listed once - replica 1
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 2 ",filename]) # should be listed only once

        # local cleanup
        output = commands.getstatusoutput( 'rm '+filepath )

    def test_iget_with_purgec(self):
        # local setup
        filename = "purgecgetfile.txt"
        filepath = os.path.abspath(filename)
        f = open(filepath,'wb')
        f.write("TESTFILE -- ["+filepath+"]")
        f.close()

        # assertions
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",filename) # should not be listed
        assertiCmd(s.adminsession,"iput "+filename) # put file
        assertiCmd(s.adminsession,"iget -f --purgec "+filename) # get file and purge 'cached' replica
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 ",filename]) # should not be listed (trimmed)
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 ",filename]) # should be listed once
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 2 ",filename]) # should not be listed

        # local cleanup
        output = commands.getstatusoutput( 'rm '+filepath )

    def test_irepl_with_purgec(self):
        # local setup
        filename = "purgecreplfile.txt"
        filepath = os.path.abspath(filename)
        f = open(filepath,'wb')
        f.write("TESTFILE -- ["+filepath+"]")
        f.close()

        # assertions
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",filename) # should not be listed
        assertiCmd(s.adminsession,"iput "+filename) # put file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" --purgec "+filename) # replicate to test resource
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 ",filename]) # should not be listed (trimmed)
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 ",filename]) # should be listed twice - 2 of 3
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 ",filename]) # should be listed twice - 1 of 3

        # local cleanup
        output = commands.getstatusoutput( 'rm '+filepath )

class Test_Compound_with_UniversalMSS_Resource(unittest.TestCase, ResourceSuite, ChunkyDevTest):

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

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass

    def test_local_iput_with_force_and_destination_resource__ticket_1706(self):
        # local setup
        filename = "iputwithforceanddestination.txt"
        filepath = create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist")                           # should not be listed
        assertiCmd(s.adminsession,"iput "+filename)                                                      # put file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)                                # replicate to test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)                                    # 
        assertiCmd(s.adminsession,"iput -f -R %s %s %s" % (self.testresc, doublefile, filename) )        # overwrite test repl with different data
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," "+filename])                        # default resource cache should have dirty copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," "+filename])                        # default resource archive should have dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," "+doublesize+" "," "+filename]) # default resource cache should not have doublesize file
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," "+doublesize+" "," "+filename]) # default resource archive should not have doublesize file
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," "+doublesize+" ","& "+filename])    # targeted resource should have new double clean copy
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    ###################
    # irepl
    ###################

    def test_irepl_update_replicas(self):
        # local setup
        filename = "updatereplicasfile.txt"
        filepath = create_local_testfile(filename)
        hostname = get_hostname()
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)

        # assertions
        assertiCmd(s.adminsession,"iadmin mkresc thirdresc unixfilesystem %s:/tmp/thirdrescVault" % hostname)   # create third resource
        assertiCmd(s.adminsession,"iadmin mkresc fourthresc unixfilesystem %s:/tmp/fourthrescVault" % hostname) # create fourth resource
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist")              # should not be listed
        assertiCmd(s.adminsession,"iput "+filename)                                         # put file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)                   # replicate to test resource
        assertiCmd(s.adminsession,"irepl -R thirdresc "+filename)                           # replicate to third resource
        assertiCmd(s.adminsession,"irepl -R fourthresc "+filename)                          # replicate to fourth resource
        assertiCmd(s.adminsession,"iput -f -R "+self.testresc+" "+doublefile+" "+filename)  # repave overtop test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)                       # for debugging

        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," & "+filename]) # should have a dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," & "+filename]) # should have a dirty copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," & "+filename])     # should have a clean copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," & "+filename]) # should have a dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 4 "," & "+filename]) # should have a dirty copy

        assertiCmd(s.adminsession,"irepl -U "+filename)                                 # update last replica

        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," & "+filename]) # should have a dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," & "+filename]) # should have a dirty copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," & "+filename])     # should have a clean copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," & "+filename]) # should have a dirty copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 4 "," & "+filename])     # should have a clean copy

        assertiCmd(s.adminsession,"irepl -aU "+filename)                                # update all replicas

        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," & "+filename])     # should have a clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," & "+filename])     # should have a clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," & "+filename])     # should have a clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," & "+filename])     # should have a clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 4 "," & "+filename])     # should have a clean copy

        assertiCmd(s.adminsession,"irm -f "+filename)                                   # cleanup file
        assertiCmd(s.adminsession,"iadmin rmresc thirdresc")                            # remove third resource
        assertiCmd(s.adminsession,"iadmin rmresc fourthresc")                           # remove third resource

        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)
    def test_irepl_over_existing_second_replica__ticket_1705(self):
        # local setup
        filename = "secondreplicatest.txt"
        filepath = create_local_testfile(filename)
        # assertions
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist")          # should not be listed
        assertiCmd(s.adminsession,"iput -R "+self.testresc+" "+filename)                # put file
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)                   # for debugging
        assertiCmd(s.adminsession,"irepl "+filename)                                    # replicate to default resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)                   # for debugging
        assertiCmd(s.adminsession,"irepl "+filename)                                    # replicate overtop default resource
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," & "+filename]) # should not have a replica 3
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)               # replicate overtop test resource
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," & "+filename]) # should not have a replica 3
        # local cleanup
        os.remove(filepath)

    def test_irepl_over_existing_third_replica__ticket_1705(self):
        # local setup
        filename = "thirdreplicatest.txt"
        filepath = create_local_testfile(filename)
        hostname = get_hostname()
        # assertions
        assertiCmd(s.adminsession,"iadmin mkresc thirdresc unixfilesystem %s:/tmp/thirdrescVault" % hostname) # create third resource
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist") # should not be listed
        assertiCmd(s.adminsession,"iput "+filename)                            # put file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)      # replicate to test resource
        assertiCmd(s.adminsession,"irepl -R thirdresc "+filename)              # replicate to third resource
        assertiCmd(s.adminsession,"irepl "+filename)                           # replicate overtop default resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)      # replicate overtop test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmd(s.adminsession,"irepl -R thirdresc "+filename)              # replicate overtop third resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 4 "," & "+filename]) # should not have a replica 4
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 5 "," & "+filename]) # should not have a replica 5
        assertiCmd(s.adminsession,"irm -f "+filename)                          # cleanup file
        assertiCmd(s.adminsession,"iadmin rmresc thirdresc")                   # remove third resource
        # local cleanup
        os.remove(filepath)

    def test_irepl_over_existing_bad_replica__ticket_1705(self):
        # local setup
        filename = "reploverwritebad.txt"
        filepath = create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist") # should not be listed
        assertiCmd(s.adminsession,"iput "+filename)                            # put file
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)      # replicate to test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmd(s.adminsession,"iput -f %s %s" % (doublefile, filename) )   # overwrite default repl with different data
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," & "+filename]) # default resource cache should have clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," "+doublesize+" "," & "+filename]) # default resource cache should have new double clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," & "+filename]) # default resource archive should have clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," "+doublesize+" "," & "+filename]) # default resource archive should have new double clean copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 2 "+self.testresc," "+doublesize+" ","  "+filename]) # test resource should not have doublesize file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)      # replicate back onto test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 "+self.testresc," "+doublesize+" "," & "+filename]) # test resource should have new clean doublesize file
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," & "+filename]) # should not have a replica 3
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    def test_iput_with_purgec(self):
        # local setup
        filename = "purgecfile.txt"
        filepath = os.path.abspath(filename)
        f = open(filepath,'wb')
        f.write("TESTFILE -- ["+filepath+"]")
        f.close()

        # assertions
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",filename) # should not be listed
        assertiCmd(s.adminsession,"iput --purgec "+filename) # put file
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 ",filename]) # should not be listed (trimmed)
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 ",filename]) # should be listed once - replica 1
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 2 ",filename]) # should be listed only once

        # local cleanup
        output = commands.getstatusoutput( 'rm '+filepath )

    def test_iget_with_purgec(self):
        # local setup
        filename = "purgecgetfile.txt"
        filepath = os.path.abspath(filename)
        f = open(filepath,'wb')
        f.write("TESTFILE -- ["+filepath+"]")
        f.close()

        # assertions
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",filename) # should not be listed
        assertiCmd(s.adminsession,"iput "+filename) # put file
        assertiCmd(s.adminsession,"iget -f --purgec "+filename) # get file and purge 'cached' replica
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 ",filename]) # should not be listed (trimmed)
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 ",filename]) # should be listed once
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 2 ",filename]) # should not be listed

        # local cleanup
        output = commands.getstatusoutput( 'rm '+filepath )

    def test_irepl_with_purgec(self):
        # local setup
        filename = "purgecreplfile.txt"
        filepath = os.path.abspath(filename)
        f = open(filepath,'wb')
        f.write("TESTFILE -- ["+filepath+"]")
        f.close()

        # assertions
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",filename) # should not be listed
        assertiCmd(s.adminsession,"iput "+filename) # put file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" --purgec "+filename) # replicate to test resource
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 ",filename]) # should not be listed (trimmed)
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 ",filename]) # should be listed twice - 2 of 3
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 ",filename]) # should be listed twice - 1 of 3

        # local cleanup
        output = commands.getstatusoutput( 'rm '+filepath )

class Test_Compound_Resource(unittest.TestCase, ResourceSuite, ChunkyDevTest):

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

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass

    @unittest.skip("TEMPORARY")
    def test_iget_prefer_from_archive__ticket_1660(self):
        # define core.re filepath
        corefile = "/var/lib/eirods/iRODS/server/config/reConfigs/core.re"
        backupcorefile = corefile+"--"+self._testMethodName

        # new file to put and get
        filename = "archivepolicyfile.txt"
        filepath = create_local_testfile(filename)

        # manipulate core.re (leave as 'when_necessary' - default)

        # put the file
        assertiCmd(s.adminsession,"iput "+filename)        # put file

        # manually update the replica in archive vault
        output = getiCmdOutput(s.adminsession,"ils -L "+filename)
        archivereplicaphypath = output[0].split()[-1]  # split into tokens, get the last one
        myfile = open(archivereplicaphypath, "w")
        myfile.write('MANUALLY UPDATED ON ARCHIVE\n')
        myfile.close()

        # get file
        retrievedfile = "retrieved.txt"
        os.system("rm -f %s" % retrievedfile)
        assertiCmd(s.adminsession,"iget -f %s %s" % (filename, retrievedfile)) # get file from cache

        # confirm retrieved file is same as original
        assert 0 == os.system("diff %s %s" % (filepath, retrievedfile))

        # manipulate the core.re to add the new policy
        shutil.copy(corefile,backupcorefile)
        myfile = open(corefile, "a")
        myfile.write('pep_resource_resolve_hierarchy_pre(*OUT){*OUT="compound_resource_cache_refresh_policy=always";}\n')
        myfile.close()

        # restart the server to reread the new core.re
        os.system("/var/lib/eirods/iRODS/irodsctl stop")
        os.system("sleep 2")
        os.system("/var/lib/eirods/iRODS/irodsctl start")

        # manually update the replica in archive vault
        output = getiCmdOutput(s.adminsession,"ils -L "+filename)
        archivereplicaphypath = output[0].split()[-1]  # split into tokens, get the last one
        myfile = open(archivereplicaphypath, "w")
        myfile.write('MANUALLY UPDATED ON ARCHIVE **AGAIN**\n')
        myfile.close()

        # get the file
        assertiCmd(s.adminsession,"iget -f %s %s" % (filename, retrievedfile)) # get file from archive

        # confirm this is the new archive file
        matchfound = False
        for line in open(retrievedfile):
            if "**AGAIN**" in line:
                matchfound = True
        assert matchfound

        # restore the original core.re
        shutil.copy(backupcorefile,corefile)
        os.remove(backupcorefile)

        # local cleanup
        os.remove(filepath)
        os.remove(retrievedfile)

    def test_local_iput_with_force_and_destination_resource__ticket_1706(self):
        # local setup
        filename = "iputwithforceanddestination.txt"
        filepath = create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist")                           # should not be listed
        assertiCmd(s.adminsession,"iput "+filename)                                                      # put file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)                                # replicate to test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)                                    # debugging
        assertiCmd(s.adminsession,"iput -f -R %s %s %s" % (self.testresc, doublefile, filename) )        # overwrite test repl with different data
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," "+filename])                        # default resource cache should have dirty copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," "+filename])                        # default resource archive should have dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," "+doublesize+" "," "+filename]) # default resource cache should not have doublesize file
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," "+doublesize+" "," "+filename]) # default resource archive should not have doublesize file
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," "+doublesize+" ","& "+filename])    # targeted resource should have new double clean copy
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    ###################
    # irepl
    ###################
    
    def test_irepl_update_replicas(self):
        # local setup
        filename = "updatereplicasfile.txt"
        filepath = create_local_testfile(filename)
        hostname = get_hostname()
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)

        # assertions
        assertiCmd(s.adminsession,"iadmin mkresc thirdresc unixfilesystem %s:/tmp/thirdrescVault" % hostname)   # create third resource
        assertiCmd(s.adminsession,"iadmin mkresc fourthresc unixfilesystem %s:/tmp/fourthrescVault" % hostname) # create fourth resource
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist")              # should not be listed
        assertiCmd(s.adminsession,"iput "+filename)                                         # put file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)                   # replicate to test resource
        assertiCmd(s.adminsession,"irepl -R thirdresc "+filename)                           # replicate to third resource
        assertiCmd(s.adminsession,"irepl -R fourthresc "+filename)                          # replicate to fourth resource
        assertiCmd(s.adminsession,"iput -f -R "+self.testresc+" "+doublefile+" "+filename)  # repave overtop test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)                       # for debugging

        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," & "+filename]) # should have a dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," & "+filename]) # should have a dirty copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," & "+filename])     # should have a clean copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," & "+filename]) # should have a dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 4 "," & "+filename]) # should have a dirty copy

        assertiCmd(s.adminsession,"irepl -U "+filename)                                 # update last replica

        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," & "+filename]) # should have a dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," & "+filename]) # should have a dirty copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," & "+filename])     # should have a clean copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," & "+filename]) # should have a dirty copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 4 "," & "+filename])     # should have a clean copy

        assertiCmd(s.adminsession,"irepl -aU "+filename)                                # update all replicas

        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," & "+filename])     # should have a clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," & "+filename])     # should have a clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," & "+filename])     # should have a clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," & "+filename])     # should have a clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 4 "," & "+filename])     # should have a clean copy

        assertiCmd(s.adminsession,"irm -f "+filename)                                   # cleanup file
        assertiCmd(s.adminsession,"iadmin rmresc thirdresc")                            # remove third resource
        assertiCmd(s.adminsession,"iadmin rmresc fourthresc")                           # remove third resource

        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)


    def test_irepl_over_existing_second_replica__ticket_1705(self):
        # local setup
        filename = "secondreplicatest.txt"
        filepath = create_local_testfile(filename)
        # assertions
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist")          # should not be listed
        assertiCmd(s.adminsession,"iput -R "+self.testresc+" "+filename)                # put file
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)                   # for debugging
        assertiCmd(s.adminsession,"irepl "+filename)                                    # replicate to default resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)                   # for debugging
        assertiCmd(s.adminsession,"irepl "+filename)                                    # replicate overtop default resource
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," & "+filename]) # should not have a replica 3
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)               # replicate overtop test resource
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," & "+filename]) # should not have a replica 3
        # local cleanup
        os.remove(filepath)

    def test_irepl_over_existing_third_replica__ticket_1705(self):
        # local setup
        filename = "thirdreplicatest.txt"
        filepath = create_local_testfile(filename)
        hostname = get_hostname()
        # assertions
        assertiCmd(s.adminsession,"iadmin mkresc thirdresc unixfilesystem %s:/tmp/thirdrescVault" % hostname) # create third resource
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist") # should not be listed
        assertiCmd(s.adminsession,"iput "+filename)                            # put file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)      # replicate to test resource
        assertiCmd(s.adminsession,"irepl -R thirdresc "+filename)              # replicate to third resource
        assertiCmd(s.adminsession,"irepl "+filename)                           # replicate overtop default resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)      # replicate overtop test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmd(s.adminsession,"irepl -R thirdresc "+filename)              # replicate overtop third resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 4 "," & "+filename]) # should not have a replica 4
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 5 "," & "+filename]) # should not have a replica 5
        assertiCmd(s.adminsession,"irm -f "+filename)                          # cleanup file
        assertiCmd(s.adminsession,"iadmin rmresc thirdresc")                   # remove third resource
        # local cleanup
        os.remove(filepath)

    def test_irepl_over_existing_bad_replica__ticket_1705(self):
        # local setup
        filename = "reploverwritebad.txt"
        filepath = create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist") # should not be listed
        assertiCmd(s.adminsession,"iput "+filename)                            # put file
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)      # replicate to test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmd(s.adminsession,"iput -f %s %s" % (doublefile, filename) )   # overwrite default repl with different data
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," & "+filename]) # default resource cache should have clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," "+doublesize+" "," & "+filename]) # default resource cache should have new double clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," & "+filename]) # default resource archive should have clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," "+doublesize+" "," & "+filename]) # default resource archive should have new double clean copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 2 "+self.testresc," "+doublesize+" ","  "+filename]) # test resource should not have doublesize file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)      # replicate back onto test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 "+self.testresc," "+doublesize+" "," & "+filename]) # test resource should have new clean doublesize file
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," & "+filename]) # should not have a replica 3
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    def test_iput_with_purgec(self):
        # local setup
        filename = "purgecfile.txt"
        filepath = os.path.abspath(filename)
        f = open(filepath,'wb')
        f.write("TESTFILE -- ["+filepath+"]")
        f.close()

        # assertions
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",filename) # should not be listed
        assertiCmd(s.adminsession,"iput --purgec "+filename) # put file
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 ",filename]) # should not be listed (trimmed)
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 ",filename]) # should be listed once - replica 1
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 2 ",filename]) # should be listed only once

        # local cleanup
        output = commands.getstatusoutput( 'rm '+filepath )

    def test_iget_with_purgec(self):
        # local setup
        filename = "purgecgetfile.txt"
        filepath = os.path.abspath(filename)
        f = open(filepath,'wb')
        f.write("TESTFILE -- ["+filepath+"]")
        f.close()

        # assertions
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",filename) # should not be listed
        assertiCmd(s.adminsession,"iput "+filename) # put file
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename) # should be listed
        assertiCmd(s.adminsession,"iget -f --purgec "+filename) # get file and purge 'cached' replica
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 ",filename]) # should not be listed (trimmed)
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 ",filename]) # should be listed once
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 2 ",filename]) # should not be listed

        # local cleanup
        output = commands.getstatusoutput( 'rm '+filepath )

    def test_irepl_with_purgec(self):
        # local setup
        filename = "purgecreplfile.txt"
        filepath = os.path.abspath(filename)
        f = open(filepath,'wb')
        f.write("TESTFILE -- ["+filepath+"]")
        f.close()

        # assertions
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",filename) # should not be listed
        assertiCmd(s.adminsession,"iput "+filename) # put file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" --purgec "+filename) # replicate to test resource
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 ",filename]) # should not be listed (trimmed)
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 ",filename]) # should be listed twice - 2 of 3
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 ",filename]) # should be listed twice - 1 of 3

        # local cleanup
        output = commands.getstatusoutput( 'rm '+filepath )

class Test_Replication_within_Replication_Resource(unittest.TestCase, ResourceSuite, ChunkyDevTest):

    hostname = socket.gethostname()
    my_test_resource = {
        "setup"    : [
            "iadmin modresc demoResc name origResc",
            "iadmin mkresc demoResc replication",
            "iadmin mkresc replResc replication",
            "iadmin mkresc unixA 'unix file system' "+hostname+":/var/lib/eirods/unixAVault",
            "iadmin mkresc unixB1 'unix file system' "+hostname+":/var/lib/eirods/unixB1Vault",
            "iadmin mkresc unixB2 'unix file system' "+hostname+":/var/lib/eirods/unixB2Vault",
            "iadmin addchildtoresc demoResc replResc",
            "iadmin addchildtoresc demoResc unixA",
            "iadmin addchildtoresc replResc unixB1",
            "iadmin addchildtoresc replResc unixB2",
        ],
        "teardown" : [
            "iadmin rmchildfromresc replResc unixB2",
            "iadmin rmchildfromresc replResc unixB1",
            "iadmin rmchildfromresc demoResc unixA",
            "iadmin rmchildfromresc demoResc replResc",
            "iadmin rmresc unixB1",
            "iadmin rmresc unixB2",
            "iadmin rmresc unixA",
            "iadmin rmresc replResc",
            "iadmin rmresc demoResc",
            "iadmin modresc origResc name demoResc",
            "rm -rf /var/lib/eirods/unixB2Vault",
            "rm -rf /var/lib/eirods/unixB1Vault",
            "rm -rf /var/lib/eirods/unixAVault",
        ],
    }

    def setUp(self):
        ResourceSuite.__init__(self)
        s.twousers_up()
        self.run_resource_setup()

    def tearDown(self):
        self.run_resource_teardown()
        s.twousers_down()

    def test_iget_with_purgec(self):
        # local setup
        filename = "purgecgetfile.txt"
        filepath = os.path.abspath(filename)
        f = open(filepath,'wb')
        f.write("TESTFILE -- ["+filepath+"]")
        f.close()

        # assertions
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist") # should not be listed
        assertiCmd(s.adminsession,"iput "+filename) # put file
        assertiCmd(s.adminsession,"iget -f --purgec "+filename) # get file
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 ",filename]) # replica 0 should be trimmed
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 ",filename]) # replica 1 should be listed
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 ",filename]) # replica 2 should be listed

        # local cleanup
        output = commands.getstatusoutput( 'rm '+filepath )

    def test_iput_with_purgec(self):
        # local setup
        filename = "purgecfile.txt"
        filepath = os.path.abspath(filename)
        f = open(filepath,'wb')
        f.write("TESTFILE -- ["+filepath+"]")
        f.close()

        # assertions
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist") # should not be listed
        assertiCmd(s.adminsession,"iput --purgec "+filename) # put file
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 ",filename]) # replica 0 should be trimmed
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 ",filename]) # replica 1 should be listed
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 ",filename]) # replica 2 should be listed

        # local cleanup
        output = commands.getstatusoutput( 'rm '+filepath )

    def test_irepl_with_purgec(self):
        # local setup
        filename = "purgecreplfile.txt"
        filepath = os.path.abspath(filename)
        f = open(filepath,'wb')
        f.write("TESTFILE -- ["+filepath+"]")
        f.close()

        # assertions
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist") # should not be listed
        assertiCmd(s.adminsession,"iput "+filename) # put file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" --purgec "+filename) # replicate to test resource
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 ",filename]) # replica 0 should be trimmed
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 ",filename]) # replica 1 should be listed
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 ",filename]) # replica 2 should be listed
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 3 ",filename]) # replica 2 should be listed

        # local cleanup
        output = commands.getstatusoutput( 'rm '+filepath )

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass

    def test_irepl_over_existing_bad_replica__ticket_1705(self):
        # local setup
        filename = "reploverwritebad.txt"
        filepath = create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist") # should not be listed
        assertiCmd(s.adminsession,"iput "+filename)                            # put file
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)      # replicate to test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmd(s.adminsession,"iput -f %s %s" % (doublefile, filename) )   # overwrite default repl with different data
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," & "+filename]) # default resource should have clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," "+doublesize+" "," & "+filename]) # default resource should have new double clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," & "+filename]) # default resource should have clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," "+doublesize+" "," & "+filename]) # default resource should have new double clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," & "+filename]) # default resource should have clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," "+doublesize+" "," & "+filename]) # default resource should have new double clean copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 3 "+self.testresc," "+doublesize+" ","  "+filename]) # test resource should not have doublesize file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)      # replicate back onto test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 3 "+self.testresc," "+doublesize+" "," & "+filename]) # test resource should have new clean doublesize file
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 4 ",filename]) # should not have a replica 4
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    def test_irepl_over_existing_second_replica__ticket_1705(self):
        # local setup
        filename = "secondreplicatest.txt"
        filepath = create_local_testfile(filename)
        # assertions
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist") # should not be listed
        assertiCmd(s.adminsession,"iput -R "+self.testresc+" "+filename)       # put file
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmd(s.adminsession,"irepl "+filename)               # replicate to default resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmd(s.adminsession,"irepl "+filename)               # replicate overtop default resource
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 4 "," & "+filename]) # should not have a replica 4
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)      # replicate overtop test resource
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 4 "," & "+filename]) # should not have a replica 4
        # local cleanup
        os.remove(filepath)

    def test_irepl_over_existing_third_replica__ticket_1705(self):
        # local setup
        filename = "thirdreplicatest.txt"
        filepath = create_local_testfile(filename)
        hostname = get_hostname()
        # assertions
        assertiCmd(s.adminsession,"iadmin mkresc thirdresc unixfilesystem %s:/tmp/thirdrescVault" % hostname) # create third resource
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist") # should not be listed
        assertiCmd(s.adminsession,"iput "+filename)                            # put file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)      # replicate to test resource
        assertiCmd(s.adminsession,"irepl -R thirdresc "+filename)              # replicate to third resource
        assertiCmd(s.adminsession,"irepl "+filename)                           # replicate overtop default resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)      # replicate overtop test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmd(s.adminsession,"irepl -R thirdresc "+filename)              # replicate overtop third resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 5 "," & "+filename]) # should not have a replica 5
        assertiCmd(s.adminsession,"irm -f "+filename)                          # cleanup file
        assertiCmd(s.adminsession,"iadmin rmresc thirdresc")                   # remove third resource
        # local cleanup
        os.remove(filepath)

    def test_irepl_update_replicas(self):
        # local setup
        filename = "updatereplicasfile.txt"
        filepath = create_local_testfile(filename)
        hostname = get_hostname()
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)

        # assertions
        assertiCmd(s.adminsession,"iadmin mkresc thirdresc unixfilesystem %s:/tmp/thirdrescVault" % hostname)   # create third resource
        assertiCmd(s.adminsession,"iadmin mkresc fourthresc unixfilesystem %s:/tmp/fourthrescVault" % hostname) # create fourth resource
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist")              # should not be listed
        assertiCmd(s.adminsession,"iput "+filename)                                         # put file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)                   # replicate to test resource
        assertiCmd(s.adminsession,"irepl -R thirdresc "+filename)                           # replicate to third resource
        assertiCmd(s.adminsession,"irepl -R fourthresc "+filename)                          # replicate to fourth resource
        assertiCmd(s.adminsession,"iput -f -R "+self.testresc+" "+doublefile+" "+filename)  # repave overtop test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)                       # for debugging

        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," & "+filename]) # should have a dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," & "+filename]) # should have a dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," & "+filename]) # should have a dirty copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," & "+filename])     # should have a clean copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 4 "," & "+filename]) # should have a dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 5 "," & "+filename]) # should have a dirty copy

        assertiCmd(s.adminsession,"irepl -U "+filename)                                 # update last replica

        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," & "+filename]) # should have a dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," & "+filename]) # should have a dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," & "+filename]) # should have a dirty copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," & "+filename])     # should have a clean copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 4 "," & "+filename]) # should have a dirty copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 5 "," & "+filename])     # should have a clean copy

        assertiCmd(s.adminsession,"irepl -aU "+filename)                                # update all replicas

        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," & "+filename])     # should have a clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," & "+filename])     # should have a clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," & "+filename])     # should have a clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," & "+filename])     # should have a clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 4 "," & "+filename])     # should have a clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 5 "," & "+filename])     # should have a clean copy

        assertiCmd(s.adminsession,"irm -f "+filename)                                   # cleanup file
        assertiCmd(s.adminsession,"iadmin rmresc thirdresc")                            # remove third resource
        assertiCmd(s.adminsession,"iadmin rmresc fourthresc")                           # remove third resource

        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    def test_irm_specific_replica(self):
        assertiCmd(s.adminsession,"ils -L "+self.testfile,"LIST",self.testfile) # should be listed
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+self.testfile) # creates replica
        assertiCmd(s.adminsession,"ils -L "+self.testfile,"LIST",self.testfile) # should be listed twice
        assertiCmd(s.adminsession,"irm -n 0 "+self.testfile) # remove original from grid
        assertiCmd(s.adminsession,"ils -L "+self.testfile,"LIST",["1 ",self.testfile]) # replica 1 should be there
        assertiCmd(s.adminsession,"ils -L "+self.testfile,"LIST",["2 ",self.testfile]) # replica 2 should be there
        assertiCmd(s.adminsession,"ils -L "+self.testfile,"LIST",["3 "+self.testresc,self.testfile]) # replica 3 should be there
        assertiCmdFail(s.adminsession,"ils -L "+self.testfile,"LIST",["0 "+s.adminsession.getDefResource(),self.testfile]) # replica 0 should be gone
        trashpath = "/"+s.adminsession.getZoneName()+"/trash/home/"+s.adminsession.getUserName()+"/"+s.adminsession.sessionId
        assertiCmdFail(s.adminsession,"ils -L "+trashpath+"/"+self.testfile,"LIST",["0 "+s.adminsession.getDefResource(),self.testfile]) # replica should not be in trash

    def test_local_iput_with_force_and_destination_resource__ticket_1706(self):
        # local setup
        filename = "iputwithforceanddestination.txt"
        filepath = create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist") # should not be listed
        assertiCmd(s.adminsession,"iput "+filename) # put file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename) # replicate to test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)
        assertiCmd(s.adminsession,"iput -f -R %s %s %s" % (self.testresc, doublefile, filename) ) # overwrite test repl with different data
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," "+filename]) # default resource should have dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," "+doublesize+" "," "+filename]) # default resource should not have doublesize file
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," "+filename]) # default resource should have dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," "+doublesize+" "," "+filename]) # default resource should not have doublesize file
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," "+filename]) # default resource should have dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," "+doublesize+" "," "+filename]) # default resource should not have doublesize file
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," "+doublesize+" ","& "+filename]) # targeted resource should have new double clean copy
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

class Test_Replication_to_two_Compound_Resources(unittest.TestCase, ResourceSuite, ChunkyDevTest):

    hostname = socket.gethostname()
    my_test_resource = {
        "setup"    : [
            "iadmin modresc demoResc name origResc",
            "iadmin mkresc demoResc replication",
            "iadmin mkresc compResc1 compound",
            "iadmin mkresc compResc2 compound",
            "iadmin mkresc cacheResc1 'unix file system' "+hostname+":/var/lib/eirods/cacheResc1Vault",
            "iadmin mkresc archiveResc1 'unix file system' "+hostname+":/var/lib/eirods/archiveResc1Vault",
            "iadmin mkresc cacheResc2 'unix file system' "+hostname+":/var/lib/eirods/cacheResc2Vault",
            "iadmin mkresc archiveResc2 'unix file system' "+hostname+":/var/lib/eirods/archiveResc2Vault",
            "iadmin addchildtoresc demoResc compResc1",
            "iadmin addchildtoresc demoResc compResc2",
            "iadmin addchildtoresc compResc1 cacheResc1 cache",
            "iadmin addchildtoresc compResc1 archiveResc1 archive",
            "iadmin addchildtoresc compResc2 cacheResc2 cache",
            "iadmin addchildtoresc compResc2 archiveResc2 archive",
        ],
        "teardown" : [
            "iadmin rmchildfromresc compResc2 archiveResc2",
            "iadmin rmchildfromresc compResc2 cacheResc2",
            "iadmin rmchildfromresc compResc1 archiveResc1",
            "iadmin rmchildfromresc compResc1 cacheResc1",
            "iadmin rmchildfromresc demoResc compResc2",
            "iadmin rmchildfromresc demoResc compResc1",
            "iadmin rmresc archiveResc2",
            "iadmin rmresc cacheResc2",
            "iadmin rmresc archiveResc1",
            "iadmin rmresc cacheResc1",
            "iadmin rmresc compResc2",
            "iadmin rmresc compResc1",
            "iadmin rmresc demoResc",
            "iadmin modresc origResc name demoResc",
            "rm -rf /var/lib/eirods/archiveResc1Vault",
            "rm -rf /var/lib/eirods/cacheResc1Vault",
            "rm -rf /var/lib/eirods/archiveResc2Vault",
            "rm -rf /var/lib/eirods/cacheResc2Vault",
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
        assertiCmd(s.adminsession,"ils -L "+self.testfile,"LIST",["4 "+self.testresc,self.testfile]) # replica 2 should still be there
        assertiCmdFail(s.adminsession,"ils -L "+self.testfile,"LIST",["0 "+s.adminsession.getDefResource(),self.testfile]) # replica 0 should be gone
        trashpath = "/"+s.adminsession.getZoneName()+"/trash/home/"+s.adminsession.getUserName()+"/"+s.adminsession.sessionId
        assertiCmdFail(s.adminsession,"ils -L "+trashpath+"/"+self.testfile,"LIST",["0 "+s.adminsession.getDefResource(),self.testfile]) # replica should not be in trash

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass
    
    def test_iget_prefer_from_archive__ticket_1660(self):
        # define core.re filepath
        corefile = "/var/lib/eirods/iRODS/server/config/reConfigs/core.re"
        backupcorefile = corefile+"--"+self._testMethodName

        # new file to put and get
        filename = "archivepolicyfile.txt"
        filepath = create_local_testfile(filename)
        # manipulate core.re (leave as 'when_necessary' - default)

        # put the file
        assertiCmd(s.adminsession,"iput "+filename)     # put file

        # manually update the replicas in archive vaults
        output = getiCmdOutput(s.adminsession,"ils -L "+filename)
        print output[0]
        archive1replicaphypath = output[0].split()[-21] # split into tokens, get the 21st from the end
        archive2replicaphypath = output[0].split()[-1]  # split into tokens, get the last one
        print archive1replicaphypath
        print archive2replicaphypath
        myfile = open(archive1replicaphypath, "w")
        myfile.write('MANUALLY UPDATED ON ARCHIVE 1\n')
        myfile.close()
        myfile = open(archive2replicaphypath, "w")
        myfile.write('MANUALLY UPDATED ON ARCHIVE 2\n')
        myfile.close()

        # get file
        retrievedfile = "retrieved.txt"
        os.system("rm -f %s" % retrievedfile)
        assertiCmd(s.adminsession,"iget -f %s %s" % (filename, retrievedfile)) # get file from cache
        # confirm retrieved file is same as original
        assert 0 == os.system("diff %s %s" % (filepath, retrievedfile))
        print "original file diff confirmed"

        # manipulate the core.re to add the new policy
        shutil.copy(corefile,backupcorefile)
        myfile = open(corefile, "a")
        myfile.write('pep_resource_resolve_hierarchy_pre(*OUT){*OUT="compound_resource_cache_refresh_policy=always";}\n')
        myfile.close()

        # restart the server to reread the new core.re
        os.system("/var/lib/eirods/iRODS/irodsctl restart")

        # manually update the replicas in archive vaults
        output = getiCmdOutput(s.adminsession,"ils -L "+filename)
        archivereplica1phypath = output[0].split()[-21] # split into tokens, get the 21st from the end
        archivereplica2phypath = output[0].split()[-1]  # split into tokens, get the last one
        print archive1replicaphypath
        print archive2replicaphypath
        myfile = open(archivereplica1phypath, "w")
        myfile.write('MANUALLY UPDATED ON ARCHIVE 1 **AGAIN**\n')
        myfile.close()
        myfile = open(archivereplica2phypath, "w")
        myfile.write('MANUALLY UPDATED ON ARCHIVE 2 **AGAIN**\n')
        myfile.close()

        # confirm the new content is on disk
        for line in open(archivereplica1phypath):
            print line
        for line in open(archivereplica2phypath):
            print line

        # confirm the core file has new policy
        print "----- confirm core has new policy ----"
        for line in open(corefile):
            if "pep_" in line:
                print line
            else:
                print ".",
        print "----- confirmation done ----"

        # get the file
        assertiCmd(s.adminsession,"iget -V -f %s %s" % (filename, retrievedfile), "LIST", "NOTICE") # get file from archive

        # confirm this is the new archive file
        matchfound = False
        for line in open(retrievedfile):
            print line
            if "AGAIN" in line:
                matchfound = True
        assert matchfound

        # restore the original core.re
        shutil.copy(backupcorefile,corefile)
        os.remove(backupcorefile)

        # local cleanup
        os.remove(filepath)
#        os.remove(retrievedfile)

    def test_local_iput_with_force_and_destination_resource__ticket_1706(self):
        # local setup
        filename = "iputwithforceanddestination.txt"
        filepath = create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist")                           # should not be listed
        assertiCmd(s.adminsession,"iput "+filename)                                                      # put file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)                                # replicate to test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)                                    # debugging
        assertiCmd(s.adminsession,"iput -f -R %s %s %s" % (self.testresc, doublefile, filename) )        # overwrite test repl with different data
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," "+filename])                        # default resource cache   1 should have dirty copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," "+filename])                        # default resource archive 1 should have dirty copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," "+filename])                        # default resource cache   2 should have dirty copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," "+filename])                        # default resource archive 2 should have dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," "+doublesize+" "," "+filename]) # default resource cache   1 should not have doublesize file
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," "+doublesize+" "," "+filename]) # default resource archive 1 should not have doublesize file
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," "+doublesize+" "," "+filename]) # default resource cache   2 should not have doublesize file
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," "+doublesize+" "," "+filename]) # default resource archive 2 should not have doublesize file
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 4 "," "+doublesize+" ","& "+filename])    # targeted resource should have new double clean copy
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    ###################
    # irepl
    ###################

    def test_irepl_update_replicas(self):
        # local setup
        filename = "updatereplicasfile.txt"
        filepath = create_local_testfile(filename)
        hostname = get_hostname()
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)

        # assertions
        assertiCmd(s.adminsession,"iadmin mkresc thirdresc unixfilesystem %s:/tmp/thirdrescVault" % hostname)   # create third resource
        assertiCmd(s.adminsession,"iadmin mkresc fourthresc unixfilesystem %s:/tmp/fourthrescVault" % hostname) # create fourth resource
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist")              # should not be listed
        assertiCmd(s.adminsession,"iput "+filename)                                         # put file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)                   # replicate to test resource
        assertiCmd(s.adminsession,"irepl -R thirdresc "+filename)                           # replicate to third resource
        assertiCmd(s.adminsession,"irepl -R fourthresc "+filename)                          # replicate to fourth resource
        assertiCmd(s.adminsession,"iput -f -R "+self.testresc+" "+doublefile+" "+filename)  # repave overtop test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)                       # for debugging

        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," & "+filename]) # should have a dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," & "+filename]) # should have a dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," & "+filename]) # should have a dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," & "+filename]) # should have a dirty copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 4 "," & "+filename])     # should have a clean copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 5 "," & "+filename]) # should have a dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 6 "," & "+filename]) # should have a dirty copy

        assertiCmd(s.adminsession,"irepl -U "+filename)                                 # update last replica

        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," & "+filename]) # should have a dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," & "+filename]) # should have a dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," & "+filename]) # should have a dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," & "+filename]) # should have a dirty copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 4 "," & "+filename])     # should have a clean copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 5 "," & "+filename]) # should have a dirty copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 6 "," & "+filename])     # should have a clean copy

        assertiCmd(s.adminsession,"irepl -aU "+filename)                                # update all replicas

        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," & "+filename])     # should have a clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," & "+filename])     # should have a clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," & "+filename])     # should have a clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," & "+filename])     # should have a clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 4 "," & "+filename])     # should have a clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 5 "," & "+filename])     # should have a clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 6 "," & "+filename])     # should have a clean copy

        assertiCmd(s.adminsession,"irm -f "+filename)                                   # cleanup file
        assertiCmd(s.adminsession,"iadmin rmresc thirdresc")                            # remove third resource
        assertiCmd(s.adminsession,"iadmin rmresc fourthresc")                           # remove third resource

        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)


    def test_irepl_over_existing_second_replica__ticket_1705(self):
        # local setup
        filename = "secondreplicatest.txt"
        filepath = create_local_testfile(filename)
        # assertions
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist")          # should not be listed
        assertiCmd(s.adminsession,"iput -R "+self.testresc+" "+filename)                # put file
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)                   # for debugging
        assertiCmd(s.adminsession,"irepl "+filename)                                    # replicate to default resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)                   # for debugging
        assertiCmd(s.adminsession,"irepl "+filename)                                    # replicate overtop default resource
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 5 "," & "+filename]) # should not have a replica 5
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)               # replicate overtop test resource
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 5 "," & "+filename]) # should not have a replica 5
        # local cleanup
        os.remove(filepath)

    def test_irepl_over_existing_third_replica__ticket_1705(self):
        # local setup
        filename = "thirdreplicatest.txt"
        filepath = create_local_testfile(filename)
        hostname = get_hostname()
        # assertions
        assertiCmd(s.adminsession,"iadmin mkresc thirdresc unixfilesystem %s:/tmp/thirdrescVault" % hostname) # create third resource
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist") # should not be listed
        assertiCmd(s.adminsession,"iput "+filename)                            # put file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)      # replicate to test resource
        assertiCmd(s.adminsession,"irepl -R thirdresc "+filename)              # replicate to third resource
        assertiCmd(s.adminsession,"irepl "+filename)                           # replicate overtop default resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)      # replicate overtop test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmd(s.adminsession,"irepl -R thirdresc "+filename)              # replicate overtop third resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 6 "," & "+filename]) # should not have a replica 6
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 7 "," & "+filename]) # should not have a replica 7
        assertiCmd(s.adminsession,"irm -f "+filename)                          # cleanup file
        assertiCmd(s.adminsession,"iadmin rmresc thirdresc")                   # remove third resource
        # local cleanup
        os.remove(filepath)

    def test_irepl_over_existing_bad_replica__ticket_1705(self):
        # local setup
        filename = "reploverwritebad.txt"
        filepath = create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist") # should not be listed
        assertiCmd(s.adminsession,"iput "+filename)                            # put file
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)      # replicate to test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmd(s.adminsession,"iput -f %s %s" % (doublefile, filename) )   # overwrite default repl with different data
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," & "+filename]) # default resource cache 1 should have clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," "+doublesize+" "," & "+filename]) # default resource cache 1 should have new double clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," & "+filename]) # default resource archive 1 should have clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," "+doublesize+" "," & "+filename]) # default resource archive 1 should have new double clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," & "+filename]) # default resource cache 2 should have clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," "+doublesize+" "," & "+filename]) # default resource cache 2 should have new double clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," & "+filename]) # default resource archive 2 should have clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," "+doublesize+" "," & "+filename]) # default resource archive 2 should have new double clean copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 4 "+self.testresc," "+doublesize+" ","  "+filename]) # test resource should not have doublesize file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)      # replicate back onto test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 4 "+self.testresc," "+doublesize+" "," & "+filename]) # test resource should have new clean doublesize file
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 5 "," & "+filename]) # should not have a replica 5
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    def test_iput_with_purgec(self):
        # local setup
        filename = "purgecfile.txt"
        filepath = os.path.abspath(filename)
        f = open(filepath,'wb')
        f.write("TESTFILE -- ["+filepath+"]")
        f.close()

        # assertions
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist") # should not be listed
        assertiCmd(s.adminsession,"iput --purgec "+filename) # put file
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 ",filename]) # should not be listed (trimmed)
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 ",filename]) # should be listed 3x - replica 1
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 ",filename]) # should be listed 3x - replica 2
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 3 ",filename]) # should be listed 3x - replica 3
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 4 ",filename]) # should not have any extra replicas

        # local cleanup
        output = commands.getstatusoutput( 'rm '+filepath )

    def test_iget_with_purgec(self):
        # local setup
        filename = "purgecgetfile.txt"
        filepath = os.path.abspath(filename)
        f = open(filepath,'wb')
        f.write("TESTFILE -- ["+filepath+"]")
        f.close()

        # assertions
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist") # should not be listed
        assertiCmd(s.adminsession,"iput "+filename) # put file
        assertiCmd(s.adminsession,"iget -f --purgec "+filename) # get file and purge 'cached' replica
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 ",filename]) # should not be listed (trimmed)
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 ",filename]) # should be listed 3x - replica 1
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 ",filename]) # should be listed 3x - replica 2
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 3 ",filename]) # should be listed 3x - replica 3
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 4 ",filename]) # should not have any extra replicas

        # local cleanup
        output = commands.getstatusoutput( 'rm '+filepath )

    def test_irepl_with_purgec(self):
        # local setup
        filename = "purgecreplfile.txt"
        filepath = os.path.abspath(filename)
        f = open(filepath,'wb')
        f.write("TESTFILE -- ["+filepath+"]")
        f.close()

        # assertions
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",filename) # should not be listed
        assertiCmd(s.adminsession,"iput "+filename) # put file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" --purgec "+filename) # replicate to test resource
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 ",filename]) # should not be listed (trimmed)
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 ",filename]) # should be listed 4x - replica 1
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 ",filename]) # should be listed 4x - replica 2
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 3 ",filename]) # should be listed 4x - replica 3
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 4 ",filename]) # should be listed 4x - replica 4
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 5 ",filename]) # should not have any extra replicas

        # local cleanup
        output = commands.getstatusoutput( 'rm '+filepath )

class Test_Replication_to_two_Compound_Resources_with_Prefer_Archive(unittest.TestCase, ResourceSuite, ChunkyDevTest):

    hostname = socket.gethostname()
    my_test_resource = {
        "setup"    : [
            "iadmin modresc demoResc name origResc",
            "iadmin mkresc demoResc replication",
            "iadmin mkresc compResc1 compound",
            "iadmin mkresc compResc2 compound",
            "iadmin mkresc cacheResc1 'unix file system' "+hostname+":/var/lib/eirods/cacheResc1Vault",
            "iadmin mkresc archiveResc1 'unix file system' "+hostname+":/var/lib/eirods/archiveResc1Vault",
            "iadmin mkresc cacheResc2 'unix file system' "+hostname+":/var/lib/eirods/cacheResc2Vault",
            "iadmin mkresc archiveResc2 'unix file system' "+hostname+":/var/lib/eirods/archiveResc2Vault",
            "iadmin addchildtoresc demoResc compResc1",
            "iadmin addchildtoresc demoResc compResc2",
            "iadmin addchildtoresc compResc1 cacheResc1 cache",
            "iadmin addchildtoresc compResc1 archiveResc1 archive",
            "iadmin addchildtoresc compResc2 cacheResc2 cache",
            "iadmin addchildtoresc compResc2 archiveResc2 archive",
        ],
        "teardown" : [
            "iadmin rmchildfromresc compResc2 archiveResc2",
            "iadmin rmchildfromresc compResc2 cacheResc2",
            "iadmin rmchildfromresc compResc1 archiveResc1",
            "iadmin rmchildfromresc compResc1 cacheResc1",
            "iadmin rmchildfromresc demoResc compResc2",
            "iadmin rmchildfromresc demoResc compResc1",
            "iadmin rmresc archiveResc2",
            "iadmin rmresc cacheResc2",
            "iadmin rmresc archiveResc1",
            "iadmin rmresc cacheResc1",
            "iadmin rmresc compResc2",
            "iadmin rmresc compResc1",
            "iadmin rmresc demoResc",
            "iadmin modresc origResc name demoResc",
            "rm -rf /var/lib/eirods/archiveResc1Vault",
            "rm -rf /var/lib/eirods/cacheResc1Vault",
            "rm -rf /var/lib/eirods/archiveResc2Vault",
            "rm -rf /var/lib/eirods/cacheResc2Vault",
        ],
    }

    def setUp(self):
        ResourceSuite.__init__(self)
        # back up core file
        corefile = "/var/lib/eirods/iRODS/server/config/reConfigs/core.re"
        backupcorefile = corefile+"--"+self._testMethodName
        shutil.copy(corefile,backupcorefile)

        # manipulate the core.re to add the new policy
        myfile = open(corefile, "a")
        myfile.write('pep_resource_resolve_hierarchy_pre(*OUT){*OUT="compound_resource_cache_refresh_policy=always";}\n')
        myfile.close()
        
        s.twousers_up()
        self.run_resource_setup()

    def tearDown(self):
        self.run_resource_teardown()
        s.twousers_down()

        # restore the original core.re
        corefile = "/var/lib/eirods/iRODS/server/config/reConfigs/core.re"
        backupcorefile = corefile+"--"+self._testMethodName
        shutil.copy(backupcorefile,corefile)
        os.remove(backupcorefile)

    def test_irm_specific_replica(self):
        assertiCmd(s.adminsession,"ils -L "+self.testfile,"LIST",self.testfile) # should be listed
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+self.testfile) # creates replica
        assertiCmd(s.adminsession,"ils -L "+self.testfile,"LIST",self.testfile) # should be listed twice
        assertiCmd(s.adminsession,"irm -n 0 "+self.testfile) # remove original from cacheResc only
        assertiCmd(s.adminsession,"ils -L "+self.testfile,"LIST",["4 "+self.testresc,self.testfile]) # replica 2 should still be there
        assertiCmdFail(s.adminsession,"ils -L "+self.testfile,"LIST",["0 "+s.adminsession.getDefResource(),self.testfile]) # replica 0 should be gone
        trashpath = "/"+s.adminsession.getZoneName()+"/trash/home/"+s.adminsession.getUserName()+"/"+s.adminsession.sessionId
        assertiCmdFail(s.adminsession,"ils -L "+trashpath+"/"+self.testfile,"LIST",["0 "+s.adminsession.getDefResource(),self.testfile]) # replica should not be in trash

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass

    @unittest.skip("this is tested elsewhere")
    def test_iget_prefer_from_archive__ticket_1660(self):
        pass

    def test_local_iput_with_force_and_destination_resource__ticket_1706(self):
        # local setup
        filename = "iputwithforceanddestination.txt"
        filepath = create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist")                           # should not be listed
        assertiCmd(s.adminsession,"iput "+filename)                                                      # put file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)                                # replicate to test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)                                    # debugging
        assertiCmd(s.adminsession,"iput -f -R %s %s %s" % (self.testresc, doublefile, filename) )        # overwrite test repl with different data
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," "+filename])                        # default resource cache   1 should have dirty copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," "+filename])                        # default resource archive 1 should have dirty copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," "+filename])                        # default resource cache   2 should have dirty copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," "+filename])                        # default resource archive 2 should have dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," "+doublesize+" "," "+filename]) # default resource cache   1 should not have doublesize file
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," "+doublesize+" "," "+filename]) # default resource archive 1 should not have doublesize file
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," "+doublesize+" "," "+filename]) # default resource cache   2 should not have doublesize file
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," "+doublesize+" "," "+filename]) # default resource archive 2 should not have doublesize file
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 4 "," "+doublesize+" ","& "+filename])    # targeted resource should have new double clean copy
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    ###################
    # irepl
    ###################

    def test_irepl_update_replicas(self):
        # local setup
        filename = "updatereplicasfile.txt"
        filepath = create_local_testfile(filename)
        hostname = get_hostname()
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)

        # assertions
        assertiCmd(s.adminsession,"iadmin mkresc thirdresc unixfilesystem %s:/tmp/thirdrescVault" % hostname)   # create third resource
        assertiCmd(s.adminsession,"iadmin mkresc fourthresc unixfilesystem %s:/tmp/fourthrescVault" % hostname) # create fourth resource
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist")              # should not be listed
        assertiCmd(s.adminsession,"iput "+filename)                                         # put file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)                   # replicate to test resource
        assertiCmd(s.adminsession,"irepl -R thirdresc "+filename)                           # replicate to third resource
        assertiCmd(s.adminsession,"irepl -R fourthresc "+filename)                          # replicate to fourth resource
        assertiCmd(s.adminsession,"iput -f -R "+self.testresc+" "+doublefile+" "+filename)  # repave overtop test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)                       # for debugging

        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," & "+filename]) # should have a dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," & "+filename]) # should have a dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," & "+filename]) # should have a dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," & "+filename]) # should have a dirty copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 4 "," & "+filename])     # should have a clean copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 5 "," & "+filename]) # should have a dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 6 "," & "+filename]) # should have a dirty copy

        assertiCmd(s.adminsession,"irepl -U "+filename)                                 # update last replica

        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," & "+filename]) # should have a dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," & "+filename]) # should have a dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," & "+filename]) # should have a dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," & "+filename]) # should have a dirty copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 4 "," & "+filename])     # should have a clean copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 5 "," & "+filename]) # should have a dirty copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 6 "," & "+filename])     # should have a clean copy

        assertiCmd(s.adminsession,"irepl -aU "+filename)                                # update all replicas

        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," & "+filename])     # should have a clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," & "+filename])     # should have a clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," & "+filename])     # should have a clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," & "+filename])     # should have a clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 4 "," & "+filename])     # should have a clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 5 "," & "+filename])     # should have a clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 6 "," & "+filename])     # should have a clean copy

        assertiCmd(s.adminsession,"irm -f "+filename)                                   # cleanup file
        assertiCmd(s.adminsession,"iadmin rmresc thirdresc")                            # remove third resource
        assertiCmd(s.adminsession,"iadmin rmresc fourthresc")                           # remove third resource

        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)


    def test_irepl_over_existing_second_replica__ticket_1705(self):
        # local setup
        filename = "secondreplicatest.txt"
        filepath = create_local_testfile(filename)
        # assertions
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist")          # should not be listed
        assertiCmd(s.adminsession,"iput -R "+self.testresc+" "+filename)                # put file
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)                   # for debugging
        assertiCmd(s.adminsession,"irepl "+filename)                                    # replicate to default resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)                   # for debugging
        assertiCmd(s.adminsession,"irepl "+filename)                                    # replicate overtop default resource
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 5 "," & "+filename]) # should not have a replica 5
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)               # replicate overtop test resource
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 5 "," & "+filename]) # should not have a replica 5
        # local cleanup
        os.remove(filepath)

    def test_irepl_over_existing_third_replica__ticket_1705(self):
        # local setup
        filename = "thirdreplicatest.txt"
        filepath = create_local_testfile(filename)
        hostname = get_hostname()
        # assertions
        assertiCmd(s.adminsession,"iadmin mkresc thirdresc unixfilesystem %s:/tmp/thirdrescVault" % hostname) # create third resource
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist") # should not be listed
        assertiCmd(s.adminsession,"iput "+filename)                            # put file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)      # replicate to test resource
        assertiCmd(s.adminsession,"irepl -R thirdresc "+filename)              # replicate to third resource
        assertiCmd(s.adminsession,"irepl "+filename)                           # replicate overtop default resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)      # replicate overtop test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmd(s.adminsession,"irepl -R thirdresc "+filename)              # replicate overtop third resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 6 "," & "+filename]) # should not have a replica 6
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 7 "," & "+filename]) # should not have a replica 7
        assertiCmd(s.adminsession,"irm -f "+filename)                          # cleanup file
        assertiCmd(s.adminsession,"iadmin rmresc thirdresc")                   # remove third resource
        # local cleanup
        os.remove(filepath)

    def test_irepl_over_existing_bad_replica__ticket_1705(self):
        # local setup
        filename = "reploverwritebad.txt"
        filepath = create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist") # should not be listed
        assertiCmd(s.adminsession,"iput "+filename)                            # put file
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)      # replicate to test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmd(s.adminsession,"iput -f %s %s" % (doublefile, filename) )   # overwrite default repl with different data
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," & "+filename]) # default resource cache 1 should have clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," "+doublesize+" "," & "+filename]) # default resource cache 1 should have new double clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," & "+filename]) # default resource archive 1 should have clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," "+doublesize+" "," & "+filename]) # default resource archive 1 should have new double clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," & "+filename]) # default resource cache 2 should have clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," "+doublesize+" "," & "+filename]) # default resource cache 2 should have new double clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," & "+filename]) # default resource archive 2 should have clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," "+doublesize+" "," & "+filename]) # default resource archive 2 should have new double clean copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 4 "+self.testresc," "+doublesize+" ","  "+filename]) # test resource should not have doublesize file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)      # replicate back onto test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 4 "+self.testresc," "+doublesize+" "," & "+filename]) # test resource should have new clean doublesize file
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 5 "," & "+filename]) # should not have a replica 5
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    def test_iput_with_purgec(self):
        # local setup
        filename = "purgecfile.txt"
        filepath = os.path.abspath(filename)
        f = open(filepath,'wb')
        f.write("TESTFILE -- ["+filepath+"]")
        f.close()

        # assertions
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist") # should not be listed
        assertiCmd(s.adminsession,"iput --purgec "+filename) # put file
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 ",filename]) # should not be listed (trimmed)
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 ",filename]) # should be listed 3x - replica 1
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 ",filename]) # should be listed 3x - replica 2
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 3 ",filename]) # should be listed 3x - replica 3
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 4 ",filename]) # should not have any extra replicas

        # local cleanup
        output = commands.getstatusoutput( 'rm '+filepath )

    def test_iget_with_purgec(self):
        # local setup
        filename = "purgecgetfile.txt"
        filepath = os.path.abspath(filename)
        f = open(filepath,'wb')
        f.write("TESTFILE -- ["+filepath+"]")
        f.close()

        # assertions
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist") # should not be listed
        assertiCmd(s.adminsession,"iput "+filename) # put file
        assertiCmd(s.adminsession,"iget -f --purgec "+filename) # get file and purge 'cached' replica
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 ",filename]) # should not be listed (trimmed)
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 ",filename]) # should be listed 3x - replica 1
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 ",filename]) # should be listed 3x - replica 2
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 3 ",filename]) # should be listed 3x - replica 3
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 4 ",filename]) # should not have any extra replicas

        # local cleanup
        output = commands.getstatusoutput( 'rm '+filepath )

    def test_irepl_with_purgec(self):
        # local setup
        filename = "purgecreplfile.txt"
        filepath = os.path.abspath(filename)
        f = open(filepath,'wb')
        f.write("TESTFILE -- ["+filepath+"]")
        f.close()

        # assertions
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",filename) # should not be listed
        assertiCmd(s.adminsession,"iput "+filename) # put file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" --purgec "+filename) # replicate to test resource
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 ",filename]) # should not be listed (trimmed)
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 ",filename]) # should be listed 4x - replica 1
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 ",filename]) # should be listed 4x - replica 2
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 3 ",filename]) # should be listed 4x - replica 3
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 4 ",filename]) # should be listed 4x - replica 4
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 5 ",filename]) # should not have any extra replicas

        # local cleanup
        output = commands.getstatusoutput( 'rm '+filepath )

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

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass

class Test_Replication_Resource(unittest.TestCase, ResourceSuite, ChunkyDevTest):

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
        assertiCmd(s.adminsession,"ils -L "+self.testfile,"LIST",[" 0 "," & "+self.testfile]) # should be listed 3x
        assertiCmd(s.adminsession,"ils -L "+self.testfile,"LIST",[" 1 "," & "+self.testfile]) # should be listed 3x
        assertiCmd(s.adminsession,"ils -L "+self.testfile,"LIST",[" 2 "," & "+self.testfile]) # should be listed 3x
        assertiCmd(s.adminsession,"irm -n 1 "+self.testfile) # try to remove one of the managed replicas
        assertiCmd(s.adminsession,"ils -L "+self.testfile,"LIST",[" 0 "," & "+self.testfile]) # should be listed 2x
        assertiCmdFail(s.adminsession,"ils -L "+self.testfile,"LIST",[" 1 "," & "+self.testfile]) # should not be listed
        assertiCmd(s.adminsession,"ils -L "+self.testfile,"LIST",[" 2 "," & "+self.testfile]) # should be listed 2x

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass

    def test_local_iput_with_force_and_destination_resource__ticket_1706(self):
        # local setup
        filename = "iputwithforceanddestination.txt"
        filepath = create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist") # should not be listed
        assertiCmd(s.adminsession,"iput "+filename) # put file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename) # replicate to test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)
        assertiCmd(s.adminsession,"iput -f -R %s %s %s" % (self.testresc, doublefile, filename) ) # overwrite test repl with different data
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," "+filename]) # default resource should have dirty copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," "+filename]) # default resource should have dirty copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," "+filename]) # default resource should have dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," "+doublesize+" "," "+filename]) # default resource should not have doublesize file
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," "+doublesize+" "," "+filename]) # default resource should not have doublesize file
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," "+doublesize+" "," "+filename]) # default resource should not have doublesize file
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," "+doublesize+" ","& "+filename]) # targeted resource should have new double clean copy
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    def test_irepl_update_replicas(self):
        # local setup
        filename = "updatereplicasfile.txt"
        filepath = create_local_testfile(filename)
        hostname = get_hostname()
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)

        # assertions
        assertiCmd(s.adminsession,"iadmin mkresc thirdresc unixfilesystem %s:/tmp/thirdrescVault" % hostname)   # create third resource
        assertiCmd(s.adminsession,"iadmin mkresc fourthresc unixfilesystem %s:/tmp/fourthrescVault" % hostname) # create fourth resource
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist")              # should not be listed
        assertiCmd(s.adminsession,"iput "+filename)                                         # put file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)                   # replicate to test resource
        assertiCmd(s.adminsession,"irepl -R thirdresc "+filename)                           # replicate to third resource
        assertiCmd(s.adminsession,"irepl -R fourthresc "+filename)                          # replicate to fourth resource
        assertiCmd(s.adminsession,"iput -f -R "+self.testresc+" "+doublefile+" "+filename)  # repave overtop test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)                       # for debugging

        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," & "+filename]) # should have a dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," & "+filename]) # should have a dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," & "+filename]) # should have a dirty copy
        assertiCmd(s.adminsession,    "ils -L "+filename,"LIST",[" 3 "," & "+filename]) # should have a clean copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 4 "," & "+filename]) # should have a dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 5 "," & "+filename]) # should have a dirty copy

        assertiCmd(s.adminsession,"irepl -U "+filename)                                 # update last replica

        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," & "+filename]) # should have a dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," & "+filename]) # should have a dirty copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," & "+filename]) # should have a dirty copy
        assertiCmd(s.adminsession,    "ils -L "+filename,"LIST",[" 3 "," & "+filename]) # should have a clean copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 4 "," & "+filename]) # should have a dirty copy
        assertiCmd(s.adminsession,    "ils -L "+filename,"LIST",[" 5 "," & "+filename]) # should have a clean copy

        assertiCmd(s.adminsession,"irepl -aU "+filename)                                # update all replicas

        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," & "+filename])     # should have a clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," & "+filename])     # should have a clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," & "+filename])     # should have a clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 3 "," & "+filename])     # should have a clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 4 "," & "+filename])     # should have a clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 5 "," & "+filename])     # should have a clean copy

        assertiCmd(s.adminsession,"irm -f "+filename)                                   # cleanup file
        assertiCmd(s.adminsession,"iadmin rmresc thirdresc")                            # remove third resource
        assertiCmd(s.adminsession,"iadmin rmresc fourthresc")                           # remove third resource

        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    def test_irepl_over_existing_second_replica__ticket_1705(self):
        # local setup
        filename = "secondreplicatest.txt"
        filepath = create_local_testfile(filename)
        # assertions
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist")          # should not be listed
        assertiCmd(s.adminsession,"iput -R "+self.testresc+" "+filename)                # put file
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)                   # for debugging
        assertiCmd(s.adminsession,"irepl "+filename)                                    # replicate to default resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)                   # for debugging
        assertiCmd(s.adminsession,"irepl "+filename)                                    # replicate overtop default resource
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 4 "," & "+filename]) # should not have a replica 3
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)               # replicate overtop test resource
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 4 "," & "+filename]) # should not have a replica 3
        # local cleanup
        os.remove(filepath)

    def test_irepl_over_existing_third_replica__ticket_1705(self):
        # local setup
        filename = "thirdreplicatest.txt"
        filepath = create_local_testfile(filename)
        hostname = get_hostname()
        # assertions
        assertiCmd(s.adminsession,"iadmin mkresc thirdresc unixfilesystem %s:/tmp/thirdrescVault" % hostname) # create third resource
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist") # should not be listed
        assertiCmd(s.adminsession,"iput "+filename)                            # put file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)      # replicate to test resource
        assertiCmd(s.adminsession,"irepl -R thirdresc "+filename)              # replicate to third resource
        assertiCmd(s.adminsession,"irepl "+filename)                           # replicate overtop default resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)      # replicate overtop test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmd(s.adminsession,"irepl -R thirdresc "+filename)              # replicate overtop third resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)          # for debugging
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 5 "," & "+filename]) # should not have a replica 4
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 6 "," & "+filename]) # should not have a replica 5
        assertiCmd(s.adminsession,"irm -f "+filename)                          # cleanup file
        assertiCmd(s.adminsession,"iadmin rmresc thirdresc")                   # remove third resource
        # local cleanup
        os.remove(filepath)

    def test_irepl_over_existing_bad_replica__ticket_1705(self):
        # local setup
        filename = "reploverwritebad.txt"
        filepath = create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        assertiCmd(s.adminsession,"ils -L "+filename,"ERROR","does not exist")                                          # should not be listed
        assertiCmd(s.adminsession,"iput "+filename)                                                                     # put file
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)                                                   # for debugging
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)                                               # replicate to test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename)                                                   # for debugging
        assertiCmd(s.adminsession,"iput -f %s %s" % (doublefile, filename) )                                            # overwrite default repl with different data
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," & "+filename])                                     # default resource 1 should have clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 0 "," "+doublesize+" "," & "+filename])                  # default resource 1 should have new double clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," & "+filename])                                     # default resource 2 should have clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 "," "+doublesize+" "," & "+filename])                  # default resource 2 should have new double clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," & "+filename])                                     # default resource 3 should have clean copy
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 "," "+doublesize+" "," & "+filename])                  # default resource 3 should have new double clean copy
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 3 "+self.testresc," "+doublesize+" ","  "+filename]) # test resource should not have doublesize file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+filename)                                               # replicate back onto test resource
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 3 "+self.testresc," "+doublesize+" "," & "+filename])    # test resource should have new clean doublesize file
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 4 "," & "+filename])                                 # should not have a replica 3
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)


    def test_iput_with_purgec(self):
        # local setup
        filename = "purgecfile.txt"
        filepath = os.path.abspath(filename)
        f = open(filepath,'wb')
        f.write("TESTFILE -- ["+filepath+"]")
        f.close()

        # assertions
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",filename) # should not be listed
        assertiCmd(s.adminsession,"iput --purgec "+filename) # put file, but trim 'cache' copy (purgec) (backwards compatibility)
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 ",filename]) # should not be listed (trimmed first replica)
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 ",filename]) # should be listed twice - replica 2 of 3
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 ",filename]) # should be listed twice - replica 3 of 3

        # local cleanup
        output = commands.getstatusoutput( 'rm '+filepath )

    def test_iget_with_purgec(self):
        # local setup
        filename = "purgecgetfile.txt"
        filepath = os.path.abspath(filename)
        f = open(filepath,'wb')
        f.write("TESTFILE -- ["+filepath+"]")
        f.close()

        # assertions
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",filename) # should not be listed
        assertiCmd(s.adminsession,"iput "+filename) # put file
        assertiCmd(s.adminsession,"iget -f --purgec "+filename) # get file and purge 'cached' replica
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 ",filename]) # should not be listed (trimmed)
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 ",filename]) # should be listed twice - 2 of 3
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 ",filename]) # should be listed twice - 2 of 3

        # local cleanup
        output = commands.getstatusoutput( 'rm '+filepath )

    def test_irepl_with_purgec(self):
        # local setup
        filename = "purgecreplfile.txt"
        filepath = os.path.abspath(filename)
        f = open(filepath,'wb')
        f.write("TESTFILE -- ["+filepath+"]")
        f.close()

        # assertions
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",filename) # should not be listed
        assertiCmd(s.adminsession,"iput "+filename) # put file
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" --purgec "+filename) # replicate to test resource
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",[" 0 ",filename]) # should not be listed (trimmed)
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 1 ",filename]) # should be listed 3x - 1 of 3
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 2 ",filename]) # should be listed 3x - 3 of 3
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",[" 3 ",filename]) # should be listed 3x - 3 of 3

        # local cleanup
        output = commands.getstatusoutput( 'rm '+filepath )

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

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass

