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

@with_setup(s.twousers_up,s.twousers_down)
def test_original_devtest():
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
    assertiCmd(s.adminsession,"ibun -cDtar "+irodshome+"/icmdtestx1.tar "+irodshome+"/icmdtestx", "LIST", "CREATE" )
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
    assertiCmd(s.adminsession,"ibun -cDgzip "+irodshome+"/icmdtestx1.tar.gz "+irodshome+"/icmdtestx", "LIST", "CREATE")
    assertiCmd(s.adminsession,"ibun -x "+irodshome+"/icmdtestx1.tar.gz "+irodshome+"/icmdtestgz")
    if os.path.isfile( "icmdtestgz" ):
        os.unlink( "icmdtestgz" )
    assertiCmd(s.adminsession,"iget -vr "+irodshome+"/icmdtestgz "+dir_w+"", "LIST", "icmdtestgz")
    output = commands.getstatusoutput( "diff -r "+dir_w+"/testx "+dir_w+"/icmdtestgz/icmdtestx" )
    print "output is ["+str(output)+"]"
    assert output[0] == 0
    assert output[1] == "", "diff output was not empty..."
    shutil.rmtree( dir_w+"/icmdtestgz")
    assertiCmd(s.adminsession,"ibun --add "+irodshome+"/icmdtestx1.tar.gz "+irodshome+"/icmdtestgz", "LIST", "ADD TO TAR")
    assertiCmd(s.adminsession,"irm -rf "+irodshome+"/icmdtestx1.tar.gz "+irodshome+"/icmdtestgz")
    # test ibun with bzip2
    assertiCmd(s.adminsession,"ibun -cDbzip2 "+irodshome+"/icmdtestx1.tar.bz2 "+irodshome+"/icmdtestx", "LIST", "CREATE")
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
    assertiCmd(s.adminsession,"iphybun -R "+s.resgroup+" -Dbzip2 "+irodshome+"/icmdtestbz2" )
    assertiCmd(s.adminsession,"itrim -N1 -S "+s.testresc+" -r "+irodshome+"/icmdtestbz2", "LIST", "Total size trimmed" )
    assertiCmd(s.adminsession,"itrim -N1 -S "+irodsdefresource+" -r "+irodshome+"/icmdtestbz2", "LIST", "Total size trimmed" )
    # get the name of bundle file
    output = commands.getstatusoutput( "ils -L "+irodshome+"/icmdtestbz2/icmdtestx/foo1 | tail -n1 | awk '{ print $NF }'")
    print output[1]
    bunfile = output[1]
#    bunfile = getBunpathOfSubfile ( ""+irodshome+"/icmdtestbz2/icmdtestx/foo1" )
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

