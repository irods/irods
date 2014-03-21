import pydevtest_sessions as s
from nose.tools import with_setup
from nose.plugins.skip import SkipTest
from pydevtest_common import assertiCmd, assertiCmdFail, interruptiCmd
import commands
import os
import datetime
import time
import shutil
import random

def test_bigfiles():
    raise SkipTest
    for filename in ["lfile1","lfile2"]:
        f = file(filename,'wb')
        targetfilesize = 50000    # size in bytes
        f.write('aaa')
        f.seek(targetfilesize-1)  # seek to one character before targetsize (leaving a sparse file)
        f.write('bbb')
        f.write('\x00')           # write single null character
        f.close()
        
@with_setup(s.twousers_up,s.twousers_down)
def test_original_devtest():

    raise SkipTest # we now have test_chunkydevtest.py which duplicates this test

    # build expected variables with similar devtest names
    progname = "README"
    myssize = str(os.stat(progname).st_size)
    username = s.adminsession.getUserName()
    irodszone = s.adminsession.getZoneName()
    testuser1 = s.sessions[1].getUserName()
    irodshome = "/"+irodszone+"/home/rods/"+s.adminsession.sessionId
    irodsdefresource = s.adminsession.getDefResource()
    dir_w = "."
    sfile2 = dir_w+"/sfile2"
    commands.getstatusoutput( "cat "+progname+" "+progname+" > "+sfile2 )
    mysdir = "/tmp/irodssdir"
    myldir = dir_w+"/ldir"
    if os.path.exists( myldir ):
        shutil.rmtree( myldir )

    # begin original devtest
    assertiCmd(s.adminsession,"ilsresc", "LIST", s.testresc)
    assertiCmd(s.adminsession,"ilsresc -l", "LIST", s.testresc)
    assertiCmd(s.adminsession,"imiscsvrinfo", "LIST", ["relVersion"] )
    assertiCmd(s.adminsession,"iuserinfo", "LIST", "name: "+username )
    assertiCmd(s.adminsession,"ienv", "LIST", "irodsZone" )
#    assertiCmd(s.adminsession,"icd $irodshome" )
    assertiCmd(s.adminsession,"ipwd", "LIST", "home" )
    assertiCmd(s.adminsession,"ihelp ils", "LIST", "ils" )
    assertiCmd(s.adminsession,"ierror -14000", "LIST", "SYS_API_INPUT_ERR" )
    assertiCmd(s.adminsession,"iexecmd hello", "LIST", "Hello world" )
    assertiCmd(s.adminsession,"ips -v", "LIST", "ips" )
    assertiCmd(s.adminsession,"iqstat", "LIST", "No delayed rules pending for user rods" )
    assertiCmd(s.adminsession,"imkdir icmdtest")

    # make a directory of large files
    assertiCmd(s.adminsession,"ils -AL","LIST","home") # debug
    assertiCmd(s.adminsession,"iput -K --wlock "+progname+" "+irodshome+"/icmdtest/foo1" )
    assertiCmd(s.adminsession,"ichksum -f "+irodshome+"/icmdtest/foo1", "LIST", "performed = 1" )
    assertiCmd(s.adminsession,"iput -kf "+progname+" "+irodshome+"/icmdtest/foo1" )
    assertiCmd(s.adminsession,"ils "+irodshome+"/icmdtest/foo1" , "LIST", "foo1" )
    assertiCmd(s.adminsession,"ils -l "+irodshome+"/icmdtest/foo1", "LIST", ["foo1",myssize] )
    assertiCmd(s.adminsession,"iadmin ls "+irodshome+"/icmdtest", "LIST", "foo1" )
    assertiCmd(s.adminsession,"ils -A "+irodshome+"/icmdtest/foo1", "LIST", username+"#"+irodszone+":own" )
    assertiCmd(s.adminsession,"ichmod read "+testuser1+" "+irodshome+"/icmdtest/foo1" )
    assertiCmd(s.adminsession,"ils -A "+irodshome+"/icmdtest/foo1", "LIST", testuser1+"#"+irodszone+":read" )
    assertiCmd(s.adminsession,"irepl -B -R "+s.testresc+" --rlock "+irodshome+"/icmdtest/foo1" )
    assertiCmd(s.adminsession,"ils -l "+irodshome+"/icmdtest/foo1", "LIST", "1 "+s.testresc )

    # overwrite a copy
    assertiCmd(s.adminsession,"itrim -S "+irodsdefresource+" -N1 "+irodshome+"/icmdtest/foo1" )
    assertiCmdFail(s.adminsession,"ils -L "+irodshome+"/icmdtest/foo1", "LIST", irodsdefresource )
    assertiCmd(s.adminsession,"iphymv -R "+irodsdefresource+" "+irodshome+"/icmdtest/foo1" )
    assertiCmd(s.adminsession,"ils -l "+irodshome+"/icmdtest/foo1", "LIST", irodsdefresource[0:19] )
    assertiCmd(s.adminsession,"imeta add -d "+irodshome+"/icmdtest/foo1 testmeta1 180 cm" )
    assertiCmd(s.adminsession,"imeta ls -d "+irodshome+"/icmdtest/foo1", "LIST", ["testmeta1"] )
    assertiCmd(s.adminsession,"imeta ls -d "+irodshome+"/icmdtest/foo1", "LIST", ["180"] )
    assertiCmd(s.adminsession,"imeta ls -d "+irodshome+"/icmdtest/foo1", "LIST", ["cm"] )
    assertiCmd(s.adminsession,"icp -K -R "+s.testresc+" "+irodshome+"/icmdtest/foo1 "+irodshome+"/icmdtest/foo2" )
    assertiCmd(s.adminsession,"ils "+irodshome+"/icmdtest/foo2", "LIST", "foo2" )
    assertiCmd(s.adminsession,"imv "+irodshome+"/icmdtest/foo2 "+irodshome+"/icmdtest/foo4" )
    assertiCmd(s.adminsession,"ils -l "+irodshome+"/icmdtest/foo4", "LIST", "foo4" )
    assertiCmd(s.adminsession,"imv "+irodshome+"/icmdtest/foo4 "+irodshome+"/icmdtest/foo2" )
    assertiCmd(s.adminsession,"ils -l "+irodshome+"/icmdtest/foo2", "LIST", "foo2" )
    assertiCmd(s.adminsession,"ichksum "+irodshome+"/icmdtest/foo2", "LIST", "foo2" )
    assertiCmd(s.adminsession,"imeta add -d "+irodshome+"/icmdtest/foo2 testmeta1 180 cm" )
    assertiCmd(s.adminsession,"imeta add -d "+irodshome+"/icmdtest/foo1 testmeta2 hello" )
    assertiCmd(s.adminsession,"imeta ls -d "+irodshome+"/icmdtest/foo1", "LIST", ["testmeta1"] )
    assertiCmd(s.adminsession,"imeta ls -d "+irodshome+"/icmdtest/foo1", "LIST", ["hello"] )
    assertiCmd(s.adminsession,"imeta qu -d testmeta1 = 180", "LIST", "foo1" )
    assertiCmd(s.adminsession,"imeta qu -d testmeta2 = hello", "LIST", "dataObj: foo1" )
    assertiCmd(s.adminsession,"iget -f -K --rlock "+irodshome+"/icmdtest/foo2 "+dir_w )
    assert myssize == str(os.stat(dir_w+"/foo2").st_size)
    os.unlink( dir_w+"/foo2" )

    # we have foo1 in $irodsdefresource and foo2 in testresource
    # make a directory containing 20 small files
    if not os.path.isdir(mysdir):
        os.mkdir(mysdir)
    for i in range(20):
        mysfile = mysdir+"/sfile"+str(i)
	shutil.copyfile( progname, mysfile )

    assertiCmd(s.adminsession,"irepl -B -R "+s.testresc+" "+irodshome+"/icmdtest/foo1" )
    phypath = dir_w+"/"+"foo1."+str(random.randrange(10000000))
    assertiCmd(s.adminsession,"iput -kfR "+irodsdefresource+" -p "+phypath+" "+sfile2+" "+irodshome+"/icmdtest/foo1" )
    # show have 2 different copies
    assertiCmd(s.adminsession,"ils -l "+irodshome+"/icmdtest/foo1", "LIST", ["foo1",myssize] )
    assertiCmd(s.adminsession,"ils -l "+irodshome+"/icmdtest/foo1", "LIST", ["foo1",str(os.stat(sfile2).st_size)] )
    # update all old copies
    assertiCmd(s.adminsession,"irepl -U "+irodshome+"/icmdtest/foo1" )
    # make sure the old size is not there
    assertiCmdFail(s.adminsession,"ils -l "+irodshome+"/icmdtest/foo1", "LIST", myssize )
    assertiCmd(s.adminsession,"itrim -S "+irodsdefresource+" "+irodshome+"/icmdtest/foo1" )
    # bulk test
    assertiCmd(s.adminsession,"iput -bIvPKr "+mysdir+" "+irodshome+"/icmdtest", "LIST", "Bulk upload" )
    # iput with a lot of options
    rsfile = dir_w+"/rsfile"
    if os.path.isfile( rsfile ):
        os.unlink( rsfile )
    assertiCmd(s.adminsession,"iput -PkITr -X "+rsfile+" --retries 10 "+mysdir+" "+irodshome+"/icmdtestw", "LIST", "Processing" )
    assertiCmd(s.adminsession,"imv "+irodshome+"/icmdtestw "+irodshome+"/icmdtestw1" )
    assertiCmd(s.adminsession,"ils -lr "+irodshome+"/icmdtestw1", "LIST", "sfile10" )
    assertiCmd(s.adminsession,"ils -Ar "+irodshome+"/icmdtestw1", "LIST", "sfile10" )
    assertiCmd(s.adminsession,"irm -rvf "+irodshome+"/icmdtestw1", "LIST", "num files done" )
    if os.path.isfile( rsfile ):
        os.unlink( rsfile )
    assertiCmd(s.adminsession,"iget -vIKPfr -X rsfile --retries 10 "+irodshome+"/icmdtest "+dir_w+"/testx", "LIST", "opened" )
    if os.path.isfile( rsfile ):
        os.unlink( rsfile )
    commands.getstatusoutput( "tar -chf "+dir_w+"/testx.tar -C "+dir_w+"/testx ." )
    assertiCmd(s.adminsession,"iput "+dir_w+"/testx.tar "+irodshome+"/icmdtestx.tar" )
    assertiCmd(s.adminsession,"ibun -x "+irodshome+"/icmdtestx.tar "+irodshome+"/icmdtestx" )
    assertiCmd(s.adminsession,"ils -lr "+irodshome+"/icmdtestx", "LIST", ["foo2"] )
    assertiCmd(s.adminsession,"ils -lr "+irodshome+"/icmdtestx", "LIST", ["sfile10"] )
    assertiCmd(s.adminsession,"ibun -cDtar "+irodshome+"/icmdtestx1.tar "+irodshome+"/icmdtestx" )
    assertiCmd(s.adminsession,"ils -l "+irodshome+"/icmdtestx1.tar", "LIST", "testx1.tar" )
    if os.path.exists(dir_w+"/testx1"):
        shutil.rmtree(dir_w+"/testx1")
    os.mkdir( dir_w+"/testx1" )
    if os.path.isfile( dir_w+"/testx1.tar" ):
        os.unlink( dir_w+"/testx1.tar" )
    assertiCmd(s.adminsession,"iget "+irodshome+"/icmdtestx1.tar "+dir_w+"/testx1.tar" )
    commands.getstatusoutput( "tar -xvf "+dir_w+"/testx1.tar -C "+dir_w+"/testx1" )
    output = commands.getstatusoutput( "diff -r "+dir_w+"/testx "+dir_w+"/testx1/icmdtestx" )
    print "output is ["+str(output)+"]"
    assert output[0] == 0
    assert output[1] == "", "diff output was not empty..."
    
    
    # test ibun with gzip
    assertiCmd(s.adminsession,"ibun -cDgzip "+irodshome+"/icmdtestx1.tar.gz "+irodshome+"/icmdtestx" )
    assertiCmd(s.adminsession,"ibun -x "+irodshome+"/icmdtestx1.tar.gz "+irodshome+"/icmdtestgz")
    if os.path.isfile( "icmdtestgz" ):
        os.unlink( "icmdtestgz" )
    assertiCmd(s.adminsession,"iget -vr "+irodshome+"/icmdtestgz "+dir_w+"", "LIST", "icmdtestgz")
    output = commands.getstatusoutput( "diff -r "+dir_w+"/testx "+dir_w+"/icmdtestgz/icmdtestx" )
    print "output is ["+str(output)+"]"
    assert output[0] == 0
    assert output[1] == "", "diff output was not empty..."
    shutil.rmtree( dir_w+"/icmdtestgz")
    assertiCmd(s.adminsession,"ibun --add "+irodshome+"/icmdtestx1.tar.gz "+irodshome+"/icmdtestgz")
    assertiCmd(s.adminsession,"irm -rf "+irodshome+"/icmdtestx1.tar.gz "+irodshome+"/icmdtestgz")
    # test ibun with bzip2
    assertiCmd(s.adminsession,"ibun -cDbzip2 "+irodshome+"/icmdtestx1.tar.bz2 "+irodshome+"/icmdtestx")
    assertiCmd(s.adminsession,"ibun -xb "+irodshome+"/icmdtestx1.tar.bz2 "+irodshome+"/icmdtestbz2")
    if os.path.isfile( "icmdtestbz2" ):
        os.unlink( "icmdtestbz2" )
    assertiCmd(s.adminsession,"iget -vr "+irodshome+"/icmdtestbz2 "+dir_w+"", "LIST", "icmdtestbz2")
    output = commands.getstatusoutput( "diff -r "+dir_w+"/testx "+dir_w+"/icmdtestbz2/icmdtestx" )
    print "output is ["+str(output)+"]"
    assert output[0] == 0
    assert output[1] == "", "diff output was not empty..."
    shutil.rmtree( dir_w+"/icmdtestbz2" )
    assertiCmd(s.adminsession,"irm -rf "+irodshome+"/icmdtestx1.tar.bz2")
#    assertiCmd(s.adminsession,"iphybun -R "+s.resgroup+" -Dbzip2 "+irodshome+"/icmdtestbz2" ) # no resgroup anymore
    assertiCmd(s.adminsession,"iphybun -R "+s.anotherresc+" -Dbzip2 "+irodshome+"/icmdtestbz2" )
    assertiCmd(s.adminsession,"itrim -N1 -S "+s.testresc+" -r "+irodshome+"/icmdtestbz2", "LIST", "Total size trimmed" )
    assertiCmd(s.adminsession,"itrim -N1 -S "+irodsdefresource+" -r "+irodshome+"/icmdtestbz2", "LIST", "Total size trimmed" )
    # get the name of bundle file
    output = commands.getstatusoutput( "ils -L "+irodshome+"/icmdtestbz2/icmdtestx/foo1 | tail -n1 | awk '{ print $NF }'")
    print output[1]
    bunfile = output[1]
    assertiCmd(s.adminsession,"ils --bundle "+bunfile, "LIST", "Subfiles" )
    assertiCmd(s.adminsession,"irm -rf "+irodshome+"/icmdtestbz2")
    assertiCmd(s.adminsession,"irm -f --empty "+bunfile )


    commands.getstatusoutput( "mv "+sfile2+" /tmp/sfile2" )
    commands.getstatusoutput( "cp /tmp/sfile2 /tmp/sfile2r" )
    assertiCmd(s.adminsession,"ireg -KR "+s.testresc+" /tmp/sfile2 "+irodshome+"/foo5" )
    commands.getstatusoutput( "cp /tmp/sfile2 /tmp/sfile2r" )
    assertiCmd(s.adminsession,"ireg -KR "+s.anotherresc+" --repl /tmp/sfile2r  "+irodshome+"/foo5" )
    assertiCmd(s.adminsession,"iget -fK "+irodshome+"/foo5 "+dir_w+"/foo5" )
    output = commands.getstatusoutput("diff /tmp/sfile2  "+dir_w+"/foo5")
    print "output is ["+str(output)+"]"
    assert output[0] == 0
    assert output[1] == "", "diff output was not empty..."
    assertiCmd(s.adminsession,"ireg -KCR "+s.testresc+" "+mysdir+" "+irodshome+"/icmdtesta" )
    if os.path.exists(dir_w+"/testa"):
        shutil.rmtree( dir_w+"/testa" )
    assertiCmd(s.adminsession,"iget -fvrK "+irodshome+"/icmdtesta "+dir_w+"/testa", "LIST", "testa" )
    output = commands.getstatusoutput("diff -r "+mysdir+" "+dir_w+"/testa" )
    print "output is ["+str(output)+"]"
    assert output[0] == 0
    assert output[1] == "", "diff output was not empty..."
    shutil.rmtree( dir_w+"/testa" )
    # test ireg with normal user
    testuser2home = "/"+irodszone+"/home/"+s.sessions[2].getUserName()
    commands.getstatusoutput( "cp /tmp/sfile2 /tmp/sfile2c" )
    # this should fail
    assertiCmd(s.sessions[2],"ireg -KR "+s.testresc+" /tmp/sfile2c "+testuser2home+"/foo5", "ERROR", "SYS_NO_PATH_PERMISSION" )
    assertiCmd(s.sessions[2],"iput -R "+s.testresc+" /tmp/sfile2c "+testuser2home+"/foo5" )
    # this should fail
#    assertiCmd(s.sessions[2],"irm -U "+testuser2home+"/foo5", "ERROR", "CANT_UNREG_IN_VAULT_FILE" )
    assertiCmd(s.sessions[2],"irm -f "+testuser2home+"/foo5" )
    os.unlink( "/tmp/sfile2c" )


    # mcoll test
    assertiCmd(s.adminsession,"imcoll -m link "+irodshome+"/icmdtesta "+irodshome+"/icmdtestb" )
    assertiCmd(s.adminsession,"ils -lr "+irodshome+"/icmdtestb", "LIST", "icmdtestb" )
    if os.path.exists(dir_w+"/testb"):
        shutil.rmtree( dir_w+"/testb" )
    assertiCmd(s.adminsession,"iget -fvrK "+irodshome+"/icmdtestb "+dir_w+"/testb", "LIST", "testb" )
    output = commands.getstatusoutput("diff -r "+mysdir+" "+dir_w+"/testb" )
    print "output is ["+str(output)+"]"
    assert output[0] == 0
    assert output[1] == "", "diff output was not empty..."
    assertiCmd(s.adminsession,"imcoll -U "+irodshome+"/icmdtestb" )
    assertiCmd(s.adminsession,"irm -rf "+irodshome+"/icmdtestb" )
    shutil.rmtree( dir_w+"/testb" )
    assertiCmd(s.adminsession,"imkdir "+irodshome+"/icmdtestm" )
    assertiCmd(s.adminsession,"imcoll -m filesystem -R "+s.testresc+" "+mysdir+" "+irodshome+"/icmdtestm" )
    assertiCmd(s.adminsession,"imkdir "+irodshome+"/icmdtestm/testmm" )
    assertiCmd(s.adminsession,"iput "+progname+" "+irodshome+"/icmdtestm/testmm/foo1" )
    assertiCmd(s.adminsession,"iput "+progname+" "+irodshome+"/icmdtestm/testmm/foo11" )
    assertiCmd(s.adminsession,"imv "+irodshome+"/icmdtestm/testmm/foo1 "+irodshome+"/icmdtestm/testmm/foo2" )
    assertiCmd(s.adminsession,"imv "+irodshome+"/icmdtestm/testmm "+irodshome+"/icmdtestm/testmm1" )

    # mv to normal collection
    assertiCmd(s.adminsession,"imv "+irodshome+"/icmdtestm/testmm1/foo2 "+irodshome+"/icmdtest/foo100" )
    assertiCmd(s.adminsession,"ils -l "+irodshome+"/icmdtest/foo100", "LIST", "foo100" )
    assertiCmd(s.adminsession,"imv "+irodshome+"/icmdtestm/testmm1 "+irodshome+"/icmdtest/testmm1" )
    assertiCmd(s.adminsession,"ils -lr "+irodshome+"/icmdtest/testmm1", "LIST", "foo11" )
    assertiCmd(s.adminsession,"irm -rf "+irodshome+"/icmdtest/testmm1 "+irodshome+"/icmdtest/foo100" )
    if os.path.exists(dir_w+"/testm"):
        shutil.rmtree( dir_w+"/testm" )
    assertiCmd(s.adminsession,"iget -fvrK "+irodshome+"/icmdtesta "+dir_w+"/testm", "LIST", "testm")
    output = commands.getstatusoutput("diff -r "+mysdir+" "+dir_w+"/testm" )
    print "output is ["+str(output)+"]"
    assert output[0] == 0
    assert output[1] == "", "diff output was not empty..."
    assertiCmd(s.adminsession,"imcoll -U "+irodshome+"/icmdtestm" )
    assertiCmd(s.adminsession,"irm -rf "+irodshome+"/icmdtestm" )
    shutil.rmtree( dir_w+"/testm" )
    assertiCmd(s.adminsession,"imkdir "+irodshome+"/icmdtestt" )
    assertiCmd(s.adminsession,"imcoll -m tar "+irodshome+"/icmdtestx.tar "+irodshome+"/icmdtestt" )
    assertiCmd(s.adminsession,"ils -lr "+irodshome+"/icmdtestt", "LIST", ["foo2"] )
    assertiCmd(s.adminsession,"ils -lr "+irodshome+"/icmdtestt", "LIST", ["foo1"] )
    if os.path.exists(dir_w+"/testt"):
        shutil.rmtree( dir_w+"/testt" )
    assertiCmd(s.adminsession,"iget -vr "+irodshome+"/icmdtestt  "+dir_w+"/testt", "LIST", "testt" )
    output = commands.getstatusoutput("diff -r  "+dir_w+"/testx "+dir_w+"/testt" )
    print "output is ["+str(output)+"]"
    assert output[0] == 0
    assert output[1] == "", "diff output was not empty..."
    assertiCmd(s.adminsession,"imkdir "+irodshome+"/icmdtestt/mydirtt" )
    assertiCmd(s.adminsession,"iput "+progname+" "+irodshome+"/icmdtestt/mydirtt/foo1mt" )
    assertiCmd(s.adminsession,"imv "+irodshome+"/icmdtestt/mydirtt/foo1mt "+irodshome+"/icmdtestt/mydirtt/foo1mtx" )

    # make a directory of 2 large files and 2 small files
    lfile = dir_w+"/lfile"
    lfile1 = dir_w+"/lfile1"
    commands.getstatusoutput( "echo 012345678901234567890123456789012345678901234567890123456789012 > "+lfile )
    for i in range(5):
        commands.getstatusoutput( "cat "+lfile+" "+lfile+" "+lfile+" "+lfile+" "+lfile+" "+lfile+" "+lfile+" "+lfile+" "+lfile+" > "+lfile1 )
        os.rename ( lfile1, lfile )
    os.mkdir( myldir )
    for i in range(1,3):
        mylfile = myldir+"/lfile"+str(i)
        mysfile = myldir+"/sfile"+str(i)
        if i != 2:
            shutil.copyfile( lfile, mylfile )
        else:
            os.rename( lfile, mylfile )
        shutil.copyfile( progname, mysfile )


##    for filename in ["lfile1","lfile2"]:
##        f = file(filename,'wb')
##        targetfilesize = 50000    # size in bytes
##        f.seek(targetfilesize-1)  # seek to one character before targetsize (leaving a sparse file)
##        f.write('\x00')           # write single null character
##        f.close()
##    for filename in ["sfile1","sfile2"]:
##        shutil.copyfile ( progname, filename )


    # test adding a large file to a mounted collection
    assertiCmd(s.adminsession,"iput "+myldir+"/lfile1 "+irodshome+"/icmdtestt/mydirtt" )
    assertiCmd(s.adminsession,"iget "+irodshome+"/icmdtestt/mydirtt/lfile1 "+dir_w+"/testt" )
    assertiCmd(s.adminsession,"irm -r "+irodshome+"/icmdtestt/mydirtt" )
    assertiCmd(s.adminsession,"imcoll -s "+irodshome+"/icmdtestt" )
    assertiCmd(s.adminsession,"imcoll -p "+irodshome+"/icmdtestt" )
    assertiCmd(s.adminsession,"imcoll -U "+irodshome+"/icmdtestt" )
    assertiCmd(s.adminsession,"irm -rf "+irodshome+"/icmdtestt" )
    shutil.rmtree(dir_w+"/testt")

    # iphybun test
    assertiCmd(s.adminsession,"iput -rR "+s.testresc+" "+mysdir+" "+irodshome+"/icmdtestp" )
#    assertiCmd(s.adminsession,"iphybun -KR "+s.resgroup+" "+irodshome+"/icmdtestp" ) # no resgroup anymore
    assertiCmd(s.adminsession,"iphybun -KR "+s.anotherresc+" "+irodshome+"/icmdtestp" )
    assertiCmd(s.adminsession,"itrim -rS "+s.testresc+" -N1 "+irodshome+"/icmdtestp", "LIST", "files trimmed" )
    output = commands.getstatusoutput( "ils -L "+irodshome+"/icmdtestp/sfile1 | tail -n1 | awk '{ print $NF }'")
    print output[1]
    bunfile = output[1]
    assertiCmd(s.adminsession,"irepl --purgec -R "+s.anotherresc+" "+bunfile )
    # jmc - resource groups are deprecated - assertiCmd(s.adminsession,"iget -r "+irodshome+"/icmdtestp  "+dir_w+"/testp" )
    # jmc - resource groups are deprecated - assertiCmd(s.adminsession,"diff -r $mysdir "+dir_w+"/testp", "", "NOANSWER" )
    assertiCmd(s.adminsession,"itrim -rS "+s.testresc+" -N1 "+irodshome+"/icmdtestp", "LIST", "files trimmed" )
    # get the name of bundle file
    assertiCmd(s.adminsession,"irm -f --empty "+bunfile )
    # should not be able to remove it because it is not empty
    assertiCmd(s.adminsession,"ils "+bunfile, "LIST", bunfile )
    assertiCmd(s.adminsession,"irm -rvf "+irodshome+"/icmdtestp", "LIST", "num files done" )
    assertiCmd(s.adminsession,"irm -f --empty "+bunfile )
    if os.path.exists(dir_w+"/testp"):
        shutil.rmtree( dir_w+"/testp" )
    shutil.rmtree( mysdir )

#    # resource group test
#    assertiCmd(s.adminsession,"iput -KR "+s.resgroup+"  "+progname+" "+irodshome+"/icmdtest/foo6" )
#    assertiCmd(s.adminsession,"ils -l "+irodshome+"/icmdtest/foo6", "LIST", ["foo6",s.anotherresc] )
#    assertiCmd(s.adminsession,"irepl -a "+irodshome+"/icmdtest/foo6" )
#    assertiCmd(s.adminsession,"ils -l "+irodshome+"/icmdtest/foo6", "LIST", [s.testresc] )
#    assertiCmd(s.adminsession,"ils -l "+irodshome+"/icmdtest/foo6", "LIST", [s.anotherresc] )
#    assertiCmd(s.adminsession,"itrim -S "+s.testresc+" -N1 "+irodshome+"/icmdtest/foo6" )
#    assertiCmdFail(s.adminsession,"ils -l "+irodshome+"/icmdtest/foo6", "LIST", [s.testresc] )
#    assertiCmd(s.adminsession,"iget -f "+irodshome+"/icmdtest/foo6 "+dir_w+"/foo6" )
#    assertiCmd(s.adminsession,"irepl -a "+irodshome+"/icmdtest/foo6" ) # TGR - adding because rereplication isn't happening automatically, since this is not a compound resource and group (would normally reget a copy into the cache from the archive
#    assertiCmd(s.adminsession,"ils -l "+irodshome+"/icmdtest/foo6", "LIST", [s.testresc] )
#    assertiCmd(s.adminsession,"ils -l "+irodshome+"/icmdtest/foo6", "LIST", [s.anotherresc] )
#    output = commands.getstatusoutput("diff -r "+progname+" "+dir_w+"/foo6" )
#    print "output is ["+str(output)+"]"
#    assert output[0] == 0
#    assert output[1] == "", "diff output was not empty..."
#    assertiCmd(s.adminsession,"itrim -S "+s.testresc+" -N1 "+irodshome+"/icmdtest/foo6" )
#    assertiCmd(s.adminsession,"iput -fR "+irodsdefresource+" "+progname+" "+irodshome+"/icmdtest/foo6" )
#    assertiCmd(s.adminsession,"ils -L "+irodshome+"/icmdtest/foo6", "LIST", s.anotherresc )
#    assertiCmd(s.adminsession,"irepl -UR "+s.anotherresc+" "+irodshome+"/icmdtest/foo6" )
#    assertiCmd(s.adminsession,"ils -l "+irodshome+"/icmdtest/foo6", "LIST", [irodsdefresource] )
#    assertiCmd(s.adminsession,"ils -l "+irodshome+"/icmdtest/foo6", "LIST", [s.anotherresc] )
#    assertiCmd(s.adminsession,"iget -f "+irodshome+"/icmdtest/foo6 "+dir_w+"/foo6" )
#    output = commands.getstatusoutput("diff -r "+progname+" "+dir_w+"/foo6" )
#    print "output is ["+str(output)+"]"
#    assert output[0] == 0
#    assert output[1] == "", "diff output was not empty..."
#    os.unlink( dir_w+"/foo6" )



##    # test --purgec option # WILL NOT WORK WITHOUT COMPOUND RESOURCES.... COMMENTING OUT
##    assertiCmd(s.adminsession,"iput -R  "+s.resgroup+"  --purgec  "+progname+"  "+irodshome+"/icmdtest/foo7" )
##    assertiCmdFail(s.adminsession,"ils -l "+irodshome+"/icmdtest/foo7", "LIST", s.testresc )
##    assertiCmd(s.adminsession,"ils -l "+irodshome+"/icmdtest/foo7", "LIST", s.anotherresc )
##    assertiCmd(s.adminsession,"irepl -a "+irodshome+"/icmdtest/foo7" )
##    assertiCmd(s.adminsession,"ils -l "+irodshome+"/icmdtest/foo7", "LIST", s.anotherresc )
##    assertiCmd(s.adminsession,"ils -l "+irodshome+"/icmdtest/foo7", "LIST", s.testresc )
##    assertiCmd(s.adminsession,"iput -fR  "+s.resgroup+"  --purgec  "+progname+"  "+irodshome+"/icmdtest/foo7" )
##    assertiCmdFail(s.adminsession,"ils -l "+irodshome+"/icmdtest/foo7", "LIST", s.testresc )
##    assertiCmd(s.adminsession,"irepl -a "+irodshome+"/icmdtest/foo7" )
##    assertiCmd(s.adminsession,"ils -l "+irodshome+"/icmdtest/foo7", "LIST", s.anotherresc )
##    assertiCmd(s.adminsession,"ils -l "+irodshome+"/icmdtest/foo7", "LIST", s.testresc )
##    assertiCmd(s.adminsession,"irepl -R  "+s.anotherresc+"  --purgec "+irodshome+"/icmdtest/foo7" )
##    assertiCmdFail(s.adminsession,"ils -l "+irodshome+"/icmdtest/foo7", "LIST", s.testresc )
##    assertiCmd(s.adminsession,"irepl -a "+irodshome+"/icmdtest/foo7" )
##    assertiCmd(s.adminsession,"itrim -S  "+s.anotherresc+"  -N1 "+irodshome+"/icmdtest/foo7" )
##    assertiCmd(s.adminsession,"ils -l "+irodshome+"/icmdtest/foo7", "LIST", s.testresc )
##    assertiCmd(s.adminsession,"irepl -R  "+s.anotherresc+"  --purgec "+irodshome+"/icmdtest/foo7" )
##    assertiCmdFail(s.adminsession,"ils -l "+irodshome+"/icmdtest/foo7", "LIST", s.testresc )
##    assertiCmd(s.adminsession,"iget -f --purgec "+irodshome+"/icmdtest/foo7 "+dir_w+"/foo7" )
##    assertiCmdFail(s.adminsession,"ils -l "+irodshome+"/icmdtest/foo7", "LIST", s.testresc )
##    assertiCmd(s.adminsession,"irepl -a "+irodshome+"/icmdtest/foo7" )
##    assertiCmd(s.adminsession,"ils -l "+irodshome+"/icmdtest/foo7", "LIST", s.anotherresc )
##    assertiCmd(s.adminsession,"ils -l "+irodshome+"/icmdtest/foo7", "LIST", s.testresc )
##    assertiCmd(s.adminsession,"iget -f --purgec "+irodshome+"/icmdtest/foo7 "+dir_w+"/foo7" )
##    assertiCmdFail(s.adminsession,"ils -l "+irodshome+"/icmdtest/foo7", "LIST", s.testresc )
##    output = commands.getstatusoutput("diff -r "+progname+" "+dir_w+"/foo7" )
##    print "output is ["+str(output)+"]"
##    assert output[0] == 0
##    assert output[1] == "", "diff output was not empty..."
##    os.unlink( dir_w+"/foo7" )

##    # test rule file # TGR SKIPPING HERE SINCE WE TEST ALL THE ACTUAL EXAMPLE CODE
##	assertiCmd(s.adminsession,"irule -F $ruletestfile", "", "", "", "irm "+irodshome+"/icmdtest/foo3" )


    # testing irsync
    assertiCmd(s.adminsession,"irsync "+progname+" i:"+irodshome+"/icmdtest/foo100" )
    assertiCmd(s.adminsession,"irsync i:"+irodshome+"/icmdtest/foo100 "+dir_w+"/foo100" )
    assertiCmd(s.adminsession,"irsync i:"+irodshome+"/icmdtest/foo100 i:"+irodshome+"/icmdtest/foo200" )
    assertiCmd(s.adminsession,"irm -f "+irodshome+"/icmdtest/foo100 "+irodshome+"/icmdtest/foo200")
    assertiCmd(s.adminsession,"iput -R "+s.testresc+" "+progname+" "+irodshome+"/icmdtest/foo100")
    assertiCmd(s.adminsession,"irsync "+progname+" i:"+irodshome+"/icmdtest/foo100" )
    assertiCmd(s.adminsession,"iput -R "+s.testresc+" "+progname+" "+irodshome+"/icmdtest/foo200")
    assertiCmd(s.adminsession,"irsync i:"+irodshome+"/icmdtest/foo100 i:"+irodshome+"/icmdtest/foo200" )
    os.unlink( dir_w+"/foo100" )


    # do test using xml protocol
    os.environ['irodsProt'] = "1"
    assertiCmd(s.adminsession,"ilsresc", "LIST", s.testresc )
    assertiCmd(s.adminsession,"imiscsvrinfo", "LIST", "relVersion" )
    assertiCmd(s.adminsession,"iuserinfo", "LIST", "name: "+username )
    assertiCmd(s.adminsession,"ienv", "LIST", "Release Version" )
    assertiCmd(s.adminsession,"icd "+irodshome )
    assertiCmd(s.adminsession,"ipwd", "LIST", "home" )
    assertiCmd(s.adminsession,"ihelp ils", "LIST", "ils" )
    assertiCmd(s.adminsession,"ierror -14000", "LIST", "SYS_API_INPUT_ERR" )
    assertiCmd(s.adminsession,"iexecmd hello", "LIST", "Hello world" )
    assertiCmd(s.adminsession,"ips -v", "LIST", "ips" )
    assertiCmd(s.adminsession,"iqstat", "LIST", "No delayed rules" )
    assertiCmd(s.adminsession,"imkdir "+irodshome+"/icmdtest1" )
    # make a directory of large files
    assertiCmd(s.adminsession,"iput -kf  "+progname+"  "+irodshome+"/icmdtest1/foo1" )
    assertiCmd(s.adminsession,"ils -l "+irodshome+"/icmdtest1/foo1", "LIST", ["foo1", myssize] )
    assertiCmd(s.adminsession,"iadmin ls "+irodshome+"/icmdtest1", "LIST", "foo1" )
    assertiCmd(s.adminsession,"ichmod read "+s.sessions[1].getUserName()+" "+irodshome+"/icmdtest1/foo1" )
    assertiCmd(s.adminsession,"ils -A "+irodshome+"/icmdtest1/foo1", "LIST", s.sessions[1].getUserName()+"#"+irodszone+":read" )
    assertiCmd(s.adminsession,"irepl -B -R "+s.testresc+" "+irodshome+"/icmdtest1/foo1" )
    # overwrite a copy
    assertiCmd(s.adminsession,"itrim -S  "+irodsdefresource+" -N1 "+irodshome+"/icmdtest1/foo1" )
    assertiCmd(s.adminsession,"iphymv -R  "+irodsdefresource+" "+irodshome+"/icmdtest1/foo1" )
    assertiCmd(s.adminsession,"imeta add -d "+irodshome+"/icmdtest1/foo1 testmeta1 180 cm" )
    assertiCmd(s.adminsession,"imeta ls -d "+irodshome+"/icmdtest1/foo1", "LIST", "testmeta1" )
    assertiCmd(s.adminsession,"imeta ls -d "+irodshome+"/icmdtest1/foo1", "LIST", "180" )
    assertiCmd(s.adminsession,"imeta ls -d "+irodshome+"/icmdtest1/foo1", "LIST", "cm" )
    assertiCmd(s.adminsession,"icp -K -R "+s.testresc+" "+irodshome+"/icmdtest1/foo1 "+irodshome+"/icmdtest1/foo2" )
    assertiCmd(s.adminsession,"imv "+irodshome+"/icmdtest1/foo2 "+irodshome+"/icmdtest1/foo4" )
    assertiCmd(s.adminsession,"imv "+irodshome+"/icmdtest1/foo4 "+irodshome+"/icmdtest1/foo2" )
    assertiCmd(s.adminsession,"ichksum -K "+irodshome+"/icmdtest1/foo2", "LIST", "foo2" )
    assertiCmd(s.adminsession,"iget -f -K "+irodshome+"/icmdtest1/foo2 "+dir_w )
    os.unlink ( dir_w+"/foo2" )
    #assertiCmd(s.adminsession,"irm "+irodshome+"/icmdtest/foo3" ) # DIDN'T EXIST ALREADY
    #assertiCmd(s.adminsession,"irule -F "+progname ) # USING IRULE IN SEPARATE TESTS
    assertiCmd(s.adminsession,"irsync "+progname+" i:"+irodshome+"/icmdtest1/foo1" )
    assertiCmd(s.adminsession,"irsync i:"+irodshome+"/icmdtest1/foo1 /tmp/foo1" )
    assertiCmd(s.adminsession,"irsync i:"+irodshome+"/icmdtest1/foo1 i:"+irodshome+"/icmdtest1/foo2" )
    os.unlink ( "/tmp/foo1" )
    os.environ['irodsProt'] = "0"

    # do the large files tests
    # mkldir ()
    lrsfile = dir_w+"/lrsfile"
    rsfile = dir_w+"/rsfile"
    if os.path.isfile( lrsfile ):
        os.unlink( lrsfile )
    if os.path.isfile( rsfile ):
        os.unlink( rsfile )
    assertiCmd(s.adminsession,"iput -vbPKr --retries 10 --wlock -X "+rsfile+" --lfrestart "+lrsfile+" -N 2 "+myldir+" "+irodshome+"/icmdtest/testy", "LIST", "New restartFile" )
    assertiCmd(s.adminsession,"ichksum -rK "+irodshome+"/icmdtest/testy", "LIST", "Total checksum performed" )
    if os.path.isfile( lrsfile ):
        os.unlink( lrsfile )
    if os.path.isfile( rsfile ):
        os.unlink( rsfile )
    assertiCmd(s.adminsession,"irepl -BvrPT -R "+s.testresc+" --rlock "+irodshome+"/icmdtest/testy", "LIST", "icmdtest/testy" )
    assertiCmd(s.adminsession,"itrim -vrS "+irodsdefresource+" --dryrun --age 1 -N 1 "+irodshome+"/icmdtest/testy", "LIST", "This is a DRYRUN" )
    assertiCmd(s.adminsession,"itrim -vrS "+irodsdefresource+" -N 1 "+irodshome+"/icmdtest/testy", "LIST", "a copy trimmed" )
    assertiCmd(s.adminsession,"icp -vKPTr -N 2 "+irodshome+"/icmdtest/testy "+irodshome+"/icmdtest/testz", "LIST", "Processing lfile1" )
    assertiCmd(s.adminsession,"irsync -r i:"+irodshome+"/icmdtest/testy i:"+irodshome+"/icmdtest/testz" )
    assertiCmd(s.adminsession,"irm -vrf "+irodshome+"/icmdtest/testy" )
    assertiCmd(s.adminsession,"iphymv -vrS "+irodsdefresource+" -R "+s.testresc+" "+irodshome+"/icmdtest/testz", "LIST", "icmdtest/testz" )

    if os.path.isfile( lrsfile ):
        os.unlink( lrsfile )
    if os.path.isfile( rsfile ):
        os.unlink( rsfile )
    if os.path.exists(dir_w+"/testz"):
        shutil.rmtree( dir_w+"/testz" )
    assertiCmd(s.adminsession,"iget -vPKr --retries 10 -X "+rsfile+" --lfrestart "+lrsfile+" --rlock -N 2 "+irodshome+"/icmdtest/testz "+dir_w+"/testz", "LIST", "testz" )
    assertiCmd(s.adminsession,"irsync -r "+dir_w+"/testz i:"+irodshome+"/icmdtest/testz" )
    assertiCmd(s.adminsession,"irsync -r i:"+irodshome+"/icmdtest/testz "+dir_w+"/testz" )
    if os.path.isfile( lrsfile ):
        os.unlink( lrsfile )
    if os.path.isfile( rsfile ):
        os.unlink( rsfile )
    output = commands.getstatusoutput( "diff -r "+dir_w+"/testz "+myldir )
    print "output is ["+str(output)+"]"
    assert output[0] == 0
    assert output[1] == "", "diff output was not empty..."
    # test -N0 transfer
    assertiCmd(s.adminsession,"iput -N0 -R "+s.testresc+" "+myldir+"/lfile1 "+irodshome+"/icmdtest/testz/lfoo100" )
    if os.path.isfile( dir_w+"/lfoo100" ):
        os.unlink( dir_w+"/lfoo100" )
    assertiCmd(s.adminsession,"iget -N0 "+irodshome+"/icmdtest/testz/lfoo100 "+dir_w+"/lfoo100" )
    output = commands.getstatusoutput( "diff "+myldir+"/lfile1 "+dir_w+"/lfoo100" )
    print "output is ["+str(output)+"]"
    assert output[0] == 0
    assert output[1] == "", "diff output was not empty..."
    shutil.rmtree( dir_w+"/testz" )
    os.unlink( dir_w+"/lfoo100" )
    assertiCmd(s.adminsession,"irm -vrf "+irodshome+"/icmdtest/testz" )


    # do the large files tests using RBUDP

    if os.path.isfile( lrsfile ):
        os.unlink( lrsfile )
    if os.path.isfile( rsfile ):
        os.unlink( rsfile )
    assertiCmd(s.adminsession,"iput -vQPKr --retries 10 -X "+rsfile+" --lfrestart "+lrsfile+" "+myldir+" "+irodshome+"/icmdtest/testy", "LIST", "icmdtest/testy" )
    assertiCmd(s.adminsession,"irepl -BQvrPT -R "+s.testresc+" "+irodshome+"/icmdtest/testy", "LIST", "icmdtest/testy" )
    assertiCmd(s.adminsession,"itrim -vrS "+irodsdefresource+" -N 1 "+irodshome+"/icmdtest/testy", "LIST", "a copy trimmed" )
    assertiCmd(s.adminsession,"icp -vQKPTr "+irodshome+"/icmdtest/testy "+irodshome+"/icmdtest/testz", "LIST", "Processing sfile1" )
    assertiCmd(s.adminsession,"irm -vrf "+irodshome+"/icmdtest/testy" )
    if os.path.isfile( lrsfile ):
        os.unlink( lrsfile )
    if os.path.isfile( rsfile ):
        os.unlink( rsfile )
    if os.path.exists(dir_w+"/testz"):
        shutil.rmtree( dir_w+"/testz" )
    assertiCmd(s.adminsession,"iget -vQPKr --retries 10 -X "+rsfile+" --lfrestart "+lrsfile+" "+irodshome+"/icmdtest/testz "+dir_w+"/testz", "LIST", "Processing sfile2" )
    if os.path.isfile( lrsfile ):
        os.unlink( lrsfile )
    if os.path.isfile( rsfile ):
        os.unlink( rsfile )
    output = commands.getstatusoutput( "diff -r "+dir_w+"/testz "+myldir )
    print "output is ["+str(output)+"]"
    assert output[0] == 0
    assert output[1] == "", "diff output was not empty..."
    shutil.rmtree( dir_w+"/testz" )
    assertiCmd(s.adminsession,"irm -vrf "+irodshome+"/icmdtest/testz" )
    shutil.rmtree( myldir )

