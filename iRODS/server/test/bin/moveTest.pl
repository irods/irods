#!/usr/bin/perl
#
# Copyright (c), The Regents of the University of California            ***
# For more information please refer to files in the COPYRIGHT directory ***
#
# This is a test script to check the icat chlRenameObject and chlMoveObject
# functions.
#
# It needs to run from this directory so that test_chl will work and the
# icommands need to be in the path and iinit run.  It assumes the
# user is 'rods' and that certain file and collection names do not exist
# and can be removed.


# run a command
# if option is 0 (normal), check the exit code 
sub runCmd {
    my($option, $cmd) = @_;
    print "running: $cmd \n";
    $cmdStdout=`$cmd`;
    $cmdStat=$?;
    if ($option == 0) {
	if ($cmdStat!=0) {
	    print "The following command failed:";
	    print "$cmd \n";
	    print "Exit code= $cmdStat \n";
	    die("command failed");
	}
    }
}

# get our zone name
runCmd(0, "ienv | grep irodsZone | tail -1");
chomp($cmdStdout);
$ix = index($cmdStdout,"=");
$myZone=substr($cmdStdout, $ix+1);

runCmd(1, "ls > foo1");
runCmd(1, "ls > foo1234");

runCmd(1, "irm -fr d1");
runCmd(0, "imkdir d1");

runCmd(0, "imkdir d1/d234");
runCmd(0, "imkdir d1/d234/testdir3");

runCmd(0, "iput foo1234 d1/d234/testdir3");

runCmd(1, "irm -fr t2");
runCmd(0, "imkdir t2");

runCmd(1, "irm -f foo1234");
runCmd(1, "irm -f foo_abcdefg");

# Set the environment variable for the config dir since
# this is now one more level down.
$ENV{'irodsConfigDir'}="../../config";

$testCmd="test_chl sql \"select coll_id from R_COLL_MAIN where coll_name = " . "?" . "\" /$myZone/home/rods/d1/d234 1 | grep -v NOTICE | grep -v Completed";
runCmd(0, $testCmd);
print "res:" . $cmdStdout;
$d234Obj=$cmdStdout;
chomp($d234Obj);

runCmd(0, "test_chl rename $d234Obj d123567");
runCmd(0, "test_chl rename $d234Obj abc");
runCmd(0, "test_chl rename $d234Obj abc_asf");
runCmd(0, "test_chl rename $d234Obj 123");
runCmd(0, "test_chl rename $d234Obj d234"); # back to what it was
runCmd(0, "ils d1");
print $cmdStdout;


$testCmd="test_chl sql \"select coll_id from R_COLL_MAIN where coll_name = " . "?" . "\" /$myZone/home/rods/t2 1 | grep -v NOTICE | grep -v Completed";
runCmd(0, $testCmd);
print "res:" . $cmdStdout;
$t2Obj=$cmdStdout;
chomp($t2Obj);

runCmd(0, "ils -r");
print $cmdStdout;

runCmd(0, "test_chl move $d234Obj $t2Obj");

runCmd(0, "ils -r");
print $cmdStdout;

unlink (foo1234b);
runCmd(0, "iget t2/d234/testdir3/foo1234 foo1234b");
runCmd(0, "diff foo1234 foo1234b");
unlink (foo1234b);

$testCmd="test_chl sql \"select coll_id from R_COLL_MAIN where coll_name = " . "?" . "\" /$myZone/home/rods 1 | grep -v NOTICE | grep -v Completed";
runCmd(0, $testCmd);
print "res:" . $cmdStdout;
$homeObj=$cmdStdout;
chomp($homeObj);

runCmd(0, "iadmin ls /$myZone/home/rods/t2/d234/testdir3 | grep foo");
print "res:" . $cmdStdout;
$index=index($cmdStdout," ");
$t1=substr($cmdStdout, $index+1);
$index=index($t1," ");
$dataObj=substr($t1,0,$index);
printf("dataObj=%s\n",$dataObj);

runCmd(0, "test_chl rename $dataObj 123456");
runCmd(0, "test_chl rename $dataObj 12345");
runCmd(0, "test_chl rename $dataObj 12ab");
runCmd(0, "test_chl rename $dataObj 12abc");
runCmd(0, "test_chl rename $dataObj foo1234");
# need to rename back

runCmd(0, "test_chl move $dataObj $homeObj");
runCmd(1, "test_chl move $dataObj 2000001"); # test non-existing target


runCmd(0, "ils");
print "res:" . $cmdStdout;

unlink (foo1234b);
runCmd(0, "iget foo1234 foo1234b");
runCmd(0, "diff foo1234 foo1234b");
unlink (foo1234b);

runCmd(0, "test_chl rename $dataObj foo_abcdefg");
unlink (foo1234b);
runCmd(0, "iget foo_abcdefg foo1234b");
runCmd(0, "diff foo1234 foo1234b");
unlink (foo1234b);

runCmd(0, "irm -f foo_abcdefg");
runCmd(0, "irm -fr d1");
runCmd(0, "irm -fr t2");

runCmd(1, "test_chl rename $d234Obj d234"); # test non-existing object
runCmd(1, "test_chl move $d234Obj $homeObj"); # test non-existing object

runCmd(0, "imkdir xdira");
runCmd(0, "icd xdira");
runCmd(0, "imkdir dir1");
runCmd(0, "imkdir dir2");
runCmd(0, "imkdir dir1/dir1a");
runCmd(0, "imkdir dir2/dir2a");
runCmd(0, "icd ..");
#imv /$myZone/home/rods/xdira /$myZone/trash/home/rods/xdira
runCmd(0, "irm -r xdira");
runCmd(0, "irmtrash");

print "Success \n";
