import sys
if (sys.version_info >= (2,7)):
    import unittest
else:
    import unittest2 as unittest
from pydevtest_common import assertiCmd, assertiCmdFail, interruptiCmd
import pydevtest_sessions as s
import commands
import os
import shlex
import datetime
import time
import psutil

class ResourceBase(object):

    def __init__(self):
        print "in ResourceBase.__init__"
        self.testfile = "pydevtest_testfile.txt"
        self.testdir = "pydevtest_testdir"
        self.testresc = "pydevtest_TestResc"
        self.anotherresc = "pydevtest_AnotherResc"

    def run_resource_setup(self):
        print "run_resource_setup - BEGIN"
        # set up resource itself
        for i in self.my_test_resource["setup"]:
            parameters = shlex.split(i) # preserves quoted substrings
            if parameters[0] == "iadmin":
                print s.adminsession.runAdminCmd(parameters[0],parameters[1:])
            else:
                output = commands.getstatusoutput(" ".join(parameters))
                print output
        # set up test resource
        print "run_resource_setup - creating test resources"
        output = commands.getstatusoutput("hostname")
        hostname = output[1]
        s.adminsession.runAdminCmd('iadmin',["mkresc",self.testresc,"unix file system",hostname+":/tmp/pydevtest_"+self.testresc])
        s.adminsession.runAdminCmd('iadmin',["mkresc",self.anotherresc,"unix file system",hostname+":/tmp/pydevtest_"+self.anotherresc])
        # set up test files
        print "run_resource_setup - generating local testfile"
        f = open(self.testfile,'wb')
        f.write("I AM A TESTFILE -- ["+self.testfile+"]")
        f.close()
        print "run_resource_setup - adding testfile to grid"
        s.adminsession.runCmd('imkdir',[self.testdir])
        s.adminsession.runCmd('iput',[self.testfile])
        print "run_resource_setup - adding testfile to grid public directory"
        s.adminsession.runCmd('icp',[self.testfile,"../../public/"]) # copy of testfile into public
        print "run_resource_setup - setting permissions"
        # permissions
        s.adminsession.runCmd('ichmod',["read",s.users[1]['name'],"../../public/"+self.testfile]) # read for user1
        s.adminsession.runCmd('ichmod',["write",s.users[2]['name'],"../../public/"+self.testfile]) # write for user2
        # set test group
        self.testgroup = s.testgroup
        print "run_resource_setup - END"

    def run_resource_teardown(self):
        print "run_resource_teardown - BEGIN"
        # local file cleanup
        print "run_resource_teardown - removing local testfile"
        os.unlink(self.testfile)
        # remove grid test files
        print "run_resource_teardown  - removing testfile from grid public directory"
        s.adminsession.runCmd('icd')
        s.adminsession.runCmd('irm',[self.testfile,"../public/"+self.testfile])
        # remove any bundle files
        print "run_resource_teardown  - removing any bundle files"
        s.adminsession.runCmd('irm -rf ../../bundle')
        # tear down admin session files
        print "run_resource_teardown  - admin session removing session files"
        s.adminsession.runCmd('irm',['-r',s.adminsession.sessionId])
        # clean trash
        print "run_resource_teardown  - clean trash"
        s.adminsession.runCmd('irmtrash',['-M'])
        # remove resc
        print "run_resource_teardown  - removing test resources"
        s.adminsession.runAdminCmd('iadmin',['rmresc',self.testresc])
        s.adminsession.runAdminCmd('iadmin',['rmresc',self.anotherresc])
        # tear down resource itself
        print "run_resource_teardown  - tearing down actual resource"
        for i in self.my_test_resource["teardown"]:
            parameters = shlex.split(i) # preserves quoted substrings
            if parameters[0] == "iadmin":
                print s.adminsession.runAdminCmd(parameters[0],parameters[1:])
            else:
                output = commands.getstatusoutput(" ".join(parameters))
                print output
        print "run_resource_teardown - END"

class ShortAndSuite(ResourceBase):

    def __init__(self):
        print "in ShortAndSuite.__init__"
        ResourceBase.__init__(self)

    def test_awesome(self):
        print "AWESOME!"

    def test_local_iget(self):
        print self.testfile

        # local setup
        localfile = "local.txt"
        # assertions
        assertiCmd(s.adminsession,"iget "+self.testfile+" "+localfile) # iget
        output = commands.getstatusoutput( 'ls '+localfile )
        print "  output: ["+output[1]+"]"
        assert output[1] == localfile
        # local cleanup
        output = commands.getstatusoutput( 'rm '+localfile )

class ResourceSuite(ResourceBase):
    '''Define the tests to be run for a resource type.

    This class is designed to be used as a base class by developers
    when they write tests for their own resource plugins.

    All these tests will be inherited and the developer can add
    any new tests for new functionality or replace any tests
    they need to modify.
    '''

    def __init__(self):
        print "in ResourceSuite.__init__"
        ResourceBase.__init__(self)

    ###################
    # iget
    ###################

    def test_local_iget(self):
        # local setup
        localfile = "local.txt"
        # assertions
        assertiCmd(s.adminsession,"iget "+self.testfile+" "+localfile) # iget
        output = commands.getstatusoutput( 'ls '+localfile )
        print "  output: ["+output[1]+"]"
        assert output[1] == localfile
        # local cleanup
        output = commands.getstatusoutput( 'rm '+localfile )

    def test_local_iget_with_overwrite(self):
        # local setup
        localfile = "local.txt"
        # assertions
        assertiCmd(s.adminsession,"iget "+self.testfile+" "+localfile) # iget
        assertiCmdFail(s.adminsession,"iget "+self.testfile+" "+localfile) # already exists
        assertiCmd(s.adminsession,"iget -f "+self.testfile+" "+localfile) # already exists, so force
        output = commands.getstatusoutput( 'ls '+localfile )
        print "  output: ["+output[1]+"]"
        assert output[1] == localfile
        # local cleanup
        output = commands.getstatusoutput( 'rm '+localfile )

    def test_local_iget_with_bad_option(self):
        # assertions
        assertiCmdFail(s.adminsession,"iget -z") # run iget with bad option

    ###################
    # imv
    ###################

    def test_local_imv(self):
        # local setup
        movedfile = "moved_file.txt"
        # assertions
        assertiCmd(s.adminsession,"imv "+self.testfile+" "+movedfile) # move
        assertiCmd(s.adminsession,"ils -L "+movedfile,"LIST",movedfile) # should be listed
        # local cleanup

    def test_local_imv_to_directory(self):
        # local setup
        # assertions
        assertiCmd(s.adminsession,"imv "+self.testfile+" "+self.testdir) # move
        assertiCmd(s.adminsession,"ils -L "+self.testdir,"LIST",self.testfile) # should be listed
        # local cleanup

    def test_local_imv_to_existing_filename(self):
        # local setup
        copyfile = "anotherfile.txt"
        # assertions
        assertiCmd(s.adminsession,"icp "+self.testfile+" "+copyfile) # icp
        assertiCmd(s.adminsession,"imv "+self.testfile+" "+copyfile, "ERROR", "CAT_NAME_EXISTS_AS_DATAOBJ") # cannot overwrite existing file
        # local cleanup

    ###################
    # iput
    ###################

    @unittest.skipIf( psutil.disk_usage('/').free < 6000000000 , "not enough free space for 2.3GB local file + iput" )
    def test_local_iput_with_really_big_file__ticket_1623(self):
        # regression test against ticket [#1623]
        # bigfilesize = [2287636992] is just under 'int' threshold
        # bigfilesize = [2297714688] is just big enough to trip 'int' size error buffers

        # local setup
        big = "reallybigfile.txt"
        bigger = "tmp.txt"
        f = open(big,'wb')
        f.write("skjfhrq274fkjhvifqp92348fuho3uigho4iulqf2h3foq3i47fuhqof9q834fyhoq3iufhq34f8923fhoq348fhurferfwheere")
        f.write("skjfhrq274fkjhvifqp92348fuho3uigho4iulqf2h3foq3i47fuhqof9q834fyhoq3iufhq34f8923fhoq348fhurferfwheere")
        f.write("skjfhrq274fkjhvg34eere----2")
        f.close()
        for i in range(9):
            commands.getstatusoutput( "cat "+big+" "+big+" "+big+" "+big+" "+big+" "+big+" > "+bigger )
            os.rename ( bigger, big )
        datafilename = big
        # assertions
        print "bigfilesize = ["+str(os.stat(datafilename).st_size)+"]"
        assertiCmd(s.adminsession,"ils -L "+datafilename,"ERROR",[datafilename,"does not exist"]) # should not be listed
        assertiCmd(s.adminsession,"iput "+datafilename) # iput
        assertiCmd(s.adminsession,"ils -L "+datafilename,"LIST",datafilename) # should be listed
        # local cleanup
        output = commands.getstatusoutput( 'rm '+datafilename )

    def test_local_iput(self):
        '''also needs to count and confirm number of replicas after the put'''
        # local setup
        datafilename = "newfile.txt"
        f = open(datafilename,'wb')
        f.write("TESTFILE -- ["+datafilename+"]")
        f.close()
        # assertions
        assertiCmdFail(s.adminsession,"ils -L "+datafilename,"LIST",datafilename) # should not be listed
        assertiCmd(s.adminsession,"iput "+datafilename) # iput
        assertiCmd(s.adminsession,"ils -L "+datafilename,"LIST",datafilename) # should be listed
        # local cleanup
        output = commands.getstatusoutput( 'rm '+datafilename )

    def test_local_iput_overwrite(self):
        assertiCmdFail(s.adminsession,"iput "+self.testfile) # fail, already exists
        assertiCmd(s.adminsession,"iput -f "+self.testfile) # iput again, force

    def test_local_iput_recursive(self):
        recursivedirname = "dir"

    def test_local_iput_checksum(self):
        # local setup
        datafilename = "newfile.txt"
        f = open(datafilename,'wb')
        f.write("TESTFILE -- ["+datafilename+"]")
        f.close()
        # assertions
        assertiCmd(s.adminsession,"iput -K "+datafilename) # iput
        assertiCmd(s.adminsession,"ils -L","LIST","d60af3eb3251240782712eab3d8ef3b1") # check proper checksum
        # local cleanup
        output = commands.getstatusoutput( 'rm '+datafilename )

    def test_local_iput_onto_specific_resource(self):
        # local setup
        datafilename = "anotherfile.txt"
        f = open(datafilename,'wb')
        f.write("TESTFILE -- ["+datafilename+"]")
        f.close()
        # assertions
        assertiCmdFail(s.adminsession,"ils -L "+datafilename,"LIST",datafilename) # should not be listed
        assertiCmd(s.adminsession,"iput -R "+self.testresc+" "+datafilename) # iput
        assertiCmd(s.adminsession,"ils -L "+datafilename,"LIST",datafilename) # should be listed
        assertiCmd(s.adminsession,"ils -L "+datafilename,"LIST",self.testresc) # should be listed
        # local cleanup
        output = commands.getstatusoutput( 'rm '+datafilename )

    def test_local_iput_interrupt_directory(self):
        # local setup
        datadir = "newdatadir"
        output = commands.getstatusoutput( 'mkdir '+datadir )
        datafiles = ["file1","file2","file3","file4","file5","file6","file7"]
        for datafilename in datafiles:
            print "-------------------"
            print "creating "+datafilename+"..."
            localpath = datadir+"/"+datafilename
            output = commands.getstatusoutput( 'dd if=/dev/zero of='+localpath+' bs=1M count=20' )
            print output[1]
            assert output[0] == 0, "dd did not successfully exit"
        rf="collectionrestartfile"
        # assertions
        iputcmd = "iput -X "+rf+" -r "+datadir
        if os.path.exists(rf): os.unlink(rf)
        interruptiCmd(s.adminsession,iputcmd,rf,10) # once restartfile reaches 10 bytes
        assert os.path.exists(rf), rf+" should now exist, but did not"
        output = commands.getstatusoutput( 'cat '+rf )
        print "  restartfile ["+rf+"] contents --> ["+output[1]+"]"
        assertiCmd(s.adminsession,"ils -L "+datadir,"LIST",datadir) # just to show contents
        assertiCmd(s.adminsession,iputcmd,"LIST","File last completed") # confirm the restart
        for datafilename in datafiles:
            assertiCmd(s.adminsession,"ils -L "+datadir,"LIST",datafilename) # should be listed
        # local cleanup
        output = commands.getstatusoutput( 'rm -rf '+datadir )
        output = commands.getstatusoutput( 'rm '+rf )

    def test_local_iput_interrupt_largefile(self):
        # local setup
        datafilename = "bigfile"
        print "-------------------"
        print "creating "+datafilename+"..."
        output = commands.getstatusoutput( 'dd if=/dev/zero of='+datafilename+' bs=1M count=150' )
        print output[1]
        assert output[0] == 0, "dd did not successfully exit"
        rf="bigrestartfile"
        # assertions
        iputcmd = "iput --lfrestart "+rf+" "+datafilename
        if os.path.exists(rf): os.unlink(rf)
        interruptiCmd(s.adminsession,iputcmd,rf,10) # once restartfile reaches 10 bytes
        assert os.path.exists(rf), rf+" should now exist, but did not"
        output = commands.getstatusoutput( 'cat '+rf )
        print "  restartfile ["+rf+"] contents --> ["+output[1]+"]"
        today = datetime.date.today()
        assertiCmd(s.adminsession,"ils -L "+datafilename,"LIST",[" 0 "+today.isoformat(),datafilename]) # will have zero length
        assertiCmd(s.adminsession,iputcmd,"LIST",datafilename+" was restarted successfully") # confirm the restart
        assertiCmdFail(s.adminsession,"ils -L "+datafilename,"LIST",[" 0 "+today.isoformat(),datafilename]) # length should not be zero
        # local cleanup
        output = commands.getstatusoutput( 'rm '+datafilename )
        output = commands.getstatusoutput( 'rm '+rf )

    def test_local_iput_physicalpath_no_permission(self):
        # local setup
        datafilename = "newfile.txt"
        f = open(datafilename,'wb')
        f.write("TESTFILE -- ["+datafilename+"]")
        f.close()
        # assertions
        assertiCmd(s.adminsession,"iput -p /newfileinroot.txt "+datafilename,"ERROR",["UNIX_FILE_CREATE_ERR","Permission denied"]) # should fail to write
        # local cleanup
        output = commands.getstatusoutput( 'rm '+datafilename )

    def test_local_iput_physicalpath(self):
        # local setup
        datafilename = "newfile.txt"
        f = open(datafilename,'wb')
        f.write("TESTFILE -- ["+datafilename+"]")
        f.close()
        # assertions
        fullpath = "/var/lib/eirods/newphysicalpath.txt"
        assertiCmd(s.adminsession,"iput -p "+fullpath+" "+datafilename) # should complete
        assertiCmd(s.adminsession,"ils -L "+datafilename,"LIST",datafilename) # should be listed
        assertiCmd(s.adminsession,"ils -L "+datafilename,"LIST",fullpath) # should be listed
        # local cleanup
        output = commands.getstatusoutput( 'rm '+datafilename )

    def test_admin_local_iput_relative_physicalpath_into_server_bin(self):
        # local setup
        datafilename = "newfile.txt"
        f = open(datafilename,'wb')
        f.write("TESTFILE -- ["+datafilename+"]")
        f.close()
        # assertions
        relpath = "relativephysicalpath.txt"
        assertiCmd(s.adminsession,"iput -p "+relpath+" "+datafilename) # should allow put into server/bin
        assertiCmd(s.adminsession,"ils -L "+datafilename,"LIST",datafilename)
        assertiCmd(s.adminsession,"ils -L "+datafilename,"LIST",relpath)
        # NOTE: cannot remove the file from virtual filesystem without force flag
        #       because iRODS cannot create similar path in trash
        # local cleanup
        output = commands.getstatusoutput( 'rm '+datafilename )

    def test_local_iput_relative_physicalpath_into_server_bin(self):
        # local setup
        datafilename = "newfile.txt"
        f = open(datafilename,'wb')
        f.write("TESTFILE -- ["+datafilename+"]")
        f.close()
        # assertions
        relpath = "relativephysicalpath.txt"
        assertiCmd(s.sessions[1],"iput -p "+relpath+" "+datafilename,"ERROR","PATH_REG_NOT_ALLOWED") # should error
        # local cleanup
        output = commands.getstatusoutput( 'rm '+datafilename )

    def test_local_iput_with_changed_target_filename(self):
        # local setup
        datafilename = "newfile.txt"
        f = open(datafilename,'wb')
        f.write("TESTFILE -- ["+datafilename+"]")
        f.close()
        # assertions
        changedfilename = "different.txt"
        assertiCmd(s.adminsession,"iput "+datafilename+" "+changedfilename) # should complete
        assertiCmd(s.adminsession,"ils -L "+changedfilename,"LIST",changedfilename) # should be listed
        # local cleanup
        output = commands.getstatusoutput( 'rm '+datafilename )

    def test_local_iput_collision_with_wlock(self):
        # local setup
        datafilename = "collisionfile1.txt"
        print "-------------------"
        print "creating "+datafilename+"..."
        output = commands.getstatusoutput( 'dd if=/dev/zero of='+datafilename+' bs=1M count=30' )
        print output[1]
        assert output[0] == 0, "dd did not successfully exit"
        # assertions

        begin = time.time()
        errorflag = False

        procs = set()
        pids = set()

        # start multiple icommands in parallel
        initialdelay = 3  # seconds
        for i in range(5):
            if i == 0:
                # add a three second delay before the first icommand
                p = s.adminsession.runCmd('iput',["-vf","--wlock",datafilename],waitforresult=False,delay=initialdelay)
            else:
                p = s.adminsession.runCmd('iput',["-vf","--wlock",datafilename],waitforresult=False,delay=0)
            procs.add(p)
            pids.add(p.pid)

        while pids:
            pid,retval=os.wait()
            for proc in procs:
                if proc.pid == pid:
                    print "pid "+str(pid)+":"
                    if retval != 0:
                        print "  * ERROR occurred *  <------------"
                        errorflag = True
                    print "  retval ["+str(retval)+"]"
                    print "  stdout ["+proc.stdout.read().strip()+"]"
                    print "  stderr ["+proc.stderr.read().strip()+"]"
                    pids.remove(pid)

        elapsed = time.time() - begin
        print "\ntotal time ["+str(elapsed)+"]"

        assert elapsed > initialdelay

        # local cleanup
        output = commands.getstatusoutput( 'rm '+datafilename )

        assert errorflag == False, "oops, had an error"

    def test_local_iput_collision_without_wlock(self):
        # local setup
        datafilename = "collisionfile2.txt"
        print "-------------------"
        print "creating "+datafilename+"..."
        output = commands.getstatusoutput( 'dd if=/dev/zero of='+datafilename+' bs=1M count=30' )
        print output[1]
        assert output[0] == 0, "dd did not successfully exit"
        # assertions

        begin = time.time()
        errorflag = False

        procs = set()
        pids = set()

        # start multiple icommands in parallel
        initialdelay = 3  # seconds
        for i in range(7):
            if i == 0:
                # add a three second delay before the first icommand
                p = s.adminsession.runCmd('iput',["-vf",datafilename],waitforresult=False,delay=initialdelay)
            else:
                p = s.adminsession.runCmd('iput',["-vf",datafilename],waitforresult=False,delay=0)
            procs.add(p)
            pids.add(p.pid)

        while pids:
            pid,retval=os.wait()
            for proc in procs:
                if proc.pid == pid:
                    print "pid "+str(pid)+":"
                    if retval != 0:
                        errorflag = True
                    else:
                        print "  * Unexpectedly, No ERROR occurred *  <------------"
                    print "  retval ["+str(retval)+"]"
                    print "  stdout ["+proc.stdout.read().strip()+"]"
                    print "  stderr ["+proc.stderr.read().strip()+"]"
                    pids.remove(pid)

        elapsed = time.time() - begin
        print "\ntotal time ["+str(elapsed)+"]"

        # local cleanup
        output = commands.getstatusoutput( 'rm '+datafilename )

        assert errorflag == True, "Expected ERRORs did not occur"

    ###################
    # ireg
    ###################

    def test_ireg_as_rodsadmin(self):
        # local setup
        filename = "newfile.txt"
        filepath = os.path.abspath(filename)
        f = open(filepath,'wb')
        f.write("TESTFILE -- ["+filepath+"]")
        f.close()

        # assertions
        assertiCmdFail(s.adminsession,"ils -L "+filename,"LIST",filename) # should not be listed
        assertiCmd(s.adminsession,"ireg "+filepath+" /"+s.adminsession.getZoneName()+"/home/"+s.adminsession.getUserName()+"/"+s.adminsession.sessionId+"/"+filename) # ireg
        assertiCmd(s.adminsession,"ils -L "+filename,"LIST",filename) # should be listed

        # local cleanup
        output = commands.getstatusoutput( 'rm '+filepath )

    def test_ireg_as_rodsuser(self):
        # local setup
        filename = "newfile.txt"
        filepath = os.path.abspath(filename)
        f = open(filepath,'wb')
        f.write("TESTFILE -- ["+filepath+"]")
        f.close()

        # assertions
        assertiCmdFail(s.sessions[1],"ils -L "+filename,"LIST",filename) # should not be listed
        assertiCmd(s.sessions[1],"ireg "+filepath+" /"+s.sessions[1].getZoneName()+"/home/"+s.sessions[1].getUserName()+"/"+s.sessions[1].sessionId+"/"+filename, "ERROR","SYS_NO_PATH_PERMISSION") # ireg
        assertiCmdFail(s.sessions[1],"ils -L "+filename,"LIST",filename) # should not be listed

        # local cleanup
        output = commands.getstatusoutput( 'rm '+filepath )

    def test_ireg_as_rodsuser_in_vault(self):
        # get vault base path
        cmdout = s.sessions[1].runCmd('iquest',["%s", "select RESC_VAULT_PATH where RESC_NAME = 'demoResc'"])
        vaultpath = cmdout[0].rstrip('\n')

        # make dir in vault if necessary
        dir = os.path.join(vaultpath, 'home', s.sessions[1].getUserName())
        if not os.path.exists(dir):
                os.makedirs(dir)

        # create file in vault
        filename = "newfile.txt"
        filepath = os.path.join(dir, filename)
        f = open(filepath,'wb')
        f.write("TESTFILE -- ["+filepath+"]")
        f.close()

        # assertions
        assertiCmdFail(s.sessions[1],"ils -L "+filename,"LIST",filename) # should not be listed
        assertiCmd(s.sessions[1],"ireg "+filepath+" /"+s.sessions[1].getZoneName()+"/home/"+s.sessions[1].getUserName()+"/"+s.sessions[1].sessionId+"/"+filename, "ERROR","SYS_NO_PATH_PERMISSION") # ireg
        assertiCmdFail(s.sessions[1],"ils -L "+filename,"LIST",filename) # should not be listed

        # local cleanup
        output = commands.getstatusoutput( 'rm '+filepath )

    ###################
    # irm
    ###################

    def test_irm_doesnotexist(self):
        assertiCmdFail(s.adminsession,"irm doesnotexist") # does not exist

    def test_irm(self):
        assertiCmd(s.adminsession,"ils -L "+self.testfile,"LIST",self.testfile) # should be listed
        assertiCmd(s.adminsession,"irm "+self.testfile) # remove from grid
        assertiCmdFail(s.adminsession,"ils -L "+self.testfile,"LIST",self.testfile) # should be deleted
        trashpath = "/"+s.adminsession.getZoneName()+"/trash/home/"+s.adminsession.getUserName()+"/"+s.adminsession.sessionId
        assertiCmd(s.adminsession,"ils -L "+trashpath+"/"+self.testfile,"LIST",self.testfile) # should be in trash

    def test_irm_force(self):
        assertiCmd(s.adminsession,"ils -L "+self.testfile,"LIST",self.testfile) # should be listed
        assertiCmd(s.adminsession,"irm -f "+self.testfile) # remove from grid
        assertiCmdFail(s.adminsession,"ils -L "+self.testfile,"LIST",self.testfile) # should be deleted
        trashpath = "/"+s.adminsession.getZoneName()+"/trash/home/"+s.adminsession.getUserName()+"/"+s.adminsession.sessionId
        assertiCmdFail(s.adminsession,"ils -L "+trashpath+"/"+self.testfile,"LIST",self.testfile) # should not be in trash

    def test_irm_specific_replica(self):
        assertiCmd(s.adminsession,"ils -L "+self.testfile,"LIST",self.testfile) # should be listed
        assertiCmd(s.adminsession,"irepl -R "+self.testresc+" "+self.testfile) # creates replica
        assertiCmd(s.adminsession,"ils -L "+self.testfile,"LIST",self.testfile) # should be listed twice
        assertiCmd(s.adminsession,"irm -n 0 "+self.testfile) # remove original from grid
        assertiCmd(s.adminsession,"ils -L "+self.testfile,"LIST",["1 "+self.testresc,self.testfile]) # replica 1 should be there
        assertiCmdFail(s.adminsession,"ils -L "+self.testfile,"LIST",["0 "+s.adminsession.getDefResource(),self.testfile]) # replica 0 should be gone
        trashpath = "/"+s.adminsession.getZoneName()+"/trash/home/"+s.adminsession.getUserName()+"/"+s.adminsession.sessionId
        assertiCmdFail(s.adminsession,"ils -L "+trashpath+"/"+self.testfile,"LIST",["0 "+s.adminsession.getDefResource(),self.testfile]) # replica should not be in trash

    def test_irm_recursive_file(self):
        assertiCmd(s.adminsession,"ils -L "+self.testfile,"LIST",self.testfile) # should be listed
        assertiCmd(s.adminsession,"irm -r "+self.testfile) # should not fail, even though a collection

    def test_irm_recursive(self):
        assertiCmd(s.adminsession,"icp -r "+self.testdir+" copydir") # make a dir copy
        assertiCmd(s.adminsession,"ils -L ","LIST","copydir") # should be listed
        assertiCmd(s.adminsession,"irm -r copydir") # should remove
        assertiCmdFail(s.adminsession,"ils -L ","LIST","copydir") # should not be listed

    def test_irm_with_read_permission(self):
        assertiCmd(s.sessions[1],"icd ../../public") # switch to shared area
        assertiCmd(s.sessions[1],"ils -AL "+self.testfile,"LIST",self.testfile) # should be listed
        assertiCmdFail(s.sessions[1],"irm "+self.testfile) # read perm should not be allowed to remove
        assertiCmd(s.sessions[1],"ils -AL "+self.testfile,"LIST",self.testfile) # should still be listed

    def test_irm_with_write_permission(self):
        assertiCmd(s.sessions[2],"icd ../../public") # switch to shared area
        assertiCmd(s.sessions[2],"ils -AL "+self.testfile,"LIST",self.testfile) # should be listed
        assertiCmdFail(s.sessions[2],"irm "+self.testfile) # write perm should not be allowed to remove
        assertiCmd(s.sessions[2],"ils -AL "+self.testfile,"LIST",self.testfile) # should still be listed

    ###################
    # irmtrash
    ###################

    def test_irmtrash_admin(self):
        # assertions
        assertiCmd(s.adminsession,"irm "+self.testfile) # remove from grid
        assertiCmd(s.adminsession,"ils -rL /"+s.adminsession.getZoneName()+"/trash/home/"+s.adminsession.getUserName()+"/","LIST",self.testfile) # should be listed
        assertiCmd(s.adminsession,"irmtrash") # should be listed
        assertiCmdFail(s.adminsession,"ils -rL /"+s.adminsession.getZoneName()+"/trash/home/"+s.adminsession.getUserName()+"/","LIST",self.testfile) # should be deleted

