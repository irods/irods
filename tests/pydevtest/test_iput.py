import pydevtest_sessions as s
from nose.tools import with_setup
from nose.plugins.skip import SkipTest
from pydevtest_common import assertiCmd, assertiCmdFail, interruptiCmd, suspendiCmd, resumeiCmd
import commands
import os
import datetime
import time

@with_setup(s.adminonly_up,s.adminonly_down)
def test_local_iput():
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

@with_setup(s.adminonly_up,s.adminonly_down)
def test_local_iput_overwrite():
    # local setup
    # assertions
    assertiCmdFail(s.adminsession,"iput "+s.testfile) # fail, already exists
    assertiCmd(s.adminsession,"iput -f "+s.testfile) # iput again, force
    # local cleanup

@with_setup(s.adminonly_up,s.adminonly_down)
def test_local_iput_checksum():
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

@with_setup(s.adminonly_up,s.adminonly_down)
def test_local_iput_onto_specific_resource():
    # local setup
    datafilename = "anotherfile.txt"
    f = open(datafilename,'wb')
    f.write("TESTFILE -- ["+datafilename+"]")
    f.close()
    # assertions
    assertiCmdFail(s.adminsession,"ils -L "+datafilename,"LIST",datafilename) # should not be listed
    assertiCmd(s.adminsession,"iput -R testResc "+datafilename) # iput
    assertiCmd(s.adminsession,"ils -L "+datafilename,"LIST",datafilename) # should be listed
    assertiCmd(s.adminsession,"ils -L "+datafilename,"LIST","testResc") # should be listed
    # local cleanup
    output = commands.getstatusoutput( 'rm '+datafilename )

@with_setup(s.adminonly_up,s.adminonly_down)
def test_local_iput_interrupt_directory():
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
    assert not os.path.exists(rf), rf+" should not yet exist, but did"
    interruptiCmd(s.adminsession,iputcmd,0.19)
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

@with_setup(s.adminonly_up,s.adminonly_down)
def test_local_iput_interrupt_largefile():
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
    assert not os.path.exists(rf), rf+" should not yet exist, but did"
    interruptiCmd(s.adminsession,iputcmd,0.18)
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

@with_setup(s.adminonly_up,s.adminonly_down)
def test_local_iput_physicalpath_no_permission():
    # local setup
    datafilename = "newfile.txt"
    f = open(datafilename,'wb')
    f.write("TESTFILE -- ["+datafilename+"]")
    f.close()
    # assertions
    assertiCmd(s.adminsession,"iput -p /newfileinroot.txt "+datafilename,"ERROR",["UNIX_FILE_CREATE_ERR","Permission denied"]) # should fail to write
    # local cleanup
    output = commands.getstatusoutput( 'rm '+datafilename )

@with_setup(s.adminonly_up,s.adminonly_down)
def test_local_iput_physicalpath():
    # local setup
    datafilename = "newfile.txt"
    f = open(datafilename,'wb')
    f.write("TESTFILE -- ["+datafilename+"]")
    f.close()
    # assertions
    fullpath = "/var/lib/e-irods/newphysicalpath.txt"
    assertiCmd(s.adminsession,"iput -p "+fullpath+" "+datafilename) # should complete
    assertiCmd(s.adminsession,"ils -L "+datafilename,"LIST",datafilename) # should be listed
    assertiCmd(s.adminsession,"ils -L "+datafilename,"LIST",fullpath) # should be listed
    # local cleanup
    output = commands.getstatusoutput( 'rm '+datafilename )

@with_setup(s.adminonly_up,s.adminonly_down)
def test_admin_local_iput_relative_physicalpath_into_server_bin():
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

@with_setup(s.oneuser_up,s.oneuser_down)
def test_local_iput_relative_physicalpath_into_server_bin():
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

@with_setup(s.adminonly_up,s.adminonly_down)
def test_local_iput_with_changed_target_filename():
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

@with_setup(s.twousers_up,s.twousers_down)
def test_local_iput_collision_with_wlock():
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

@with_setup(s.twousers_up,s.twousers_down)
def test_local_iput_collision_without_wlock():
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
    

