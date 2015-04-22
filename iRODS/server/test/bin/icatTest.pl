#!/usr/bin/perl
#
# Copyright (c), The Regents of the University of California            ***
# For more information please refer to files in the COPYRIGHT directory ***
#
# This is a test script that runs various icommands to test iCat
# functions; part of a systematic test of all icat sql statements (in
# conjunction with moveTest.pl, checkIcatLog.pl, and
# clients/icommands/test/testiCommands.pl).
#
# It assumes iinit has been run, that the icommands are in the
# path and that the user is an admin.
#
# It will print "Success" at the end and have a 0 exit code, if all
# everything works properly.
#
# Some of the icommands print errors but that's by design; those
# commands should fail and this script will fail if they don't.
#

use Cwd 'abs_path';

$F1="TestFile1";
$F1LC="testfile1";
$F2="TestFile2";
$F3="TestFile3";
$F4="TestFile4";
$F4U="TESTFILE4";
$D1="directory1";
$D2="DIR2";
$D3="direct003";
$D3A="direct003/subA";
$D3B="direct003/suB";
$D3C="direct003/suA";
$D3D="direct003/subAD";
$D3E="direct003/subE";
$D4="directory04";
$D4A="directory04/sub1";
$TICKET1="ticket1";
$TICKET2="ticket2";
$TICKET3="ticket3";
$TICKET_HOST="pivo.ucsd.edu";

$U2="user2";
$U3="user3";
$U1="rods";
$Resc="demoResc";
$Resc2="Resc2";
$Zone2="ATestZoneName";
$Zone3="ADifTestZoneName";
$Resc2Path="/tmp/Vault";
$G1="group1";
#  $RG1="Resource_group";
$IN_FILE="icatTest.infile.24085";

# run a command
# if option is 0 (normal), check the exit code and fail if non-0
# if 1, don't care
# if 2, should get a non-zero result, exit if not
sub runCmd {
    my($option, $cmd, $stdoutVal) = @_;
    use File::Basename;
    my $thescriptname = basename($0);
    my $IRODS_HOME = dirname(dirname(dirname(dirname(abs_path(__FILE__)))));
    chomp(my $therodslog = `ls -t $IRODS_HOME/server/log/rodsLog* | head -n1`);
    open THERODSLOG, ">>$therodslog" or die "could not open [$therodslog]";
    print THERODSLOG " --- $thescriptname [$cmd] --- \n";
    close THERODSLOG;
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
    if ($option == 2) {
	if ($cmdStat==0) {
	    print "The following command should have failed:";
	    print "$cmd \n";
	    print "Exit code= $cmdStat \n";
	    die("command failed to fail");
	}
    }
    if ($stdoutVal != "") {
	if ($cmdStdout != $stdoutVal) {
	    print "stdout should have been: $stdoutVal\n";
	    print "but was: $cmdStdout";
	    die("incorrect stdout");
	}
    }
}


# Make a directory with input-arg empty files and test with that
sub mkfiles {
    my($numFiles, $dirName) = @_;
    if (-e $dirName) {
	printf("dir %s exists, skipping\n", $dirName);
    }
    else {
	`rm -rf $dirName`;
	`mkdir $dirName`;
	chdir "$dirName";
	$i=0;
	while ( $i < $numFiles ) {
	    `echo \"asdf\" > f$i`;
	    $i=$i+1;
	}
	chdir "..";
    }
}

# get our zone name
runCmd(0, "ienv | grep irods_zone_name | tail -1");
chomp($cmdStdout);
$ix = index($cmdStdout,"-");
$myZone=substr($cmdStdout, $ix+1);
$myZone =~ s/^\s+//;

# Move/rename tests
`ls -l > $F1`;
$F1a = "$F1" . "a";
$F1b = "$F1" . "b";

# Make sure we are in the irods home dir
runCmd(0, "icd");
# Clean up possible left over sub-process env (pwd) files, if any,
# to avoid collision if process number matches
runCmd(1, "rm -rf ~/.irods/irods_environment.json.*");

runCmd(1, "irm -f $F1");
runCmd(0, "iput $F1");
runCmd(0, "imv $F1 $F1a");
runCmd(0, "imv $F1a $F1b");
runCmd(0, "imv $F1b $F1");
runCmd(2, "imv $F1 $F1");
runCmd(0, "irm -f $F1");

# test that upper/lower case are different (can be problem with MySQL)
`ls -l > $F1LC`;
runCmd(2, "irm -f $F1LC");
runCmd(0, "iput $F1");
runCmd(0, "iput $F1LC");
runCmd(0, "irm -f $F1LC");
runCmd(0, "irm -f $F1");

$D1a = "$D1" . "a";
$D1b = "$D1" . "b";
runCmd(1, "irm -f -r $D1");
runCmd(0, "imkdir $D1");
runCmd(0, "iput $F1 $D1/$F1");
runCmd(0, "imv $D1 $D1a");
runCmd(0, "imv $D1a $D1b");
runCmd(2, "imv $D1b $D1b");
runCmd(0, "imv $D1b $D1");
runCmd(2, "imv $D1 $D1");

runCmd(0, "ils");
runCmd(2, "irm -f $D1");
runCmd(0, "irm -f -r $D1");
runCmd(0, "ils");

# metadata rm test
runCmd(0, "iput $F1");
runCmd(0, "imeta add -d $F1 a b");
runCmd(0, "irm -f $F1");

# basic exercise of without-distinct query
runCmd(0, "iquest no-distinct \"select RESC_ID\" ");

# basic exercise of upper case query
`ls -l > $F4`;
runCmd(1, "irm -f $F4");
runCmd(0, "iput $F4");
runCmd(0, "iquest uppercase \"select COLL_NAME, DATA_NAME WHERE DATA_NAME like '$F4U' \" ");
runCmd(0, "irm -f $F4");

# basic resource-down test
runCmd(0, "iadmin modresc $Resc status down");
runCmd(2, "iput -R $Resc $F1");
runCmd(0, "iadmin modresc $Resc status up");
runCmd(0, "iput -R $Resc $F1");
runCmd(0, "irm -f $F1");

#
# Multiple imkdir's should return an error now.
# ICAT should give CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME error
# which imkdir now displays (use to suppress).
runCmd(0, "imkdir $D1");
runCmd(2, "imkdir $D1");
runCmd(0, "irm -rf $D1");

# Repeated iput -rf test (was failing as ICAT code needed to
# rollback after an insert failure since it is now doing a 'begin').
mkfiles(2, "testIput_RF_repeat");
runCmd(0, "iput -rf  testIput_RF_repeat");
runCmd(0, "ils -l testIput_RF_repeat");
runCmd(0, "iput -rf  testIput_RF_repeat");
runCmd(0, "iput -rf  testIput_RF_repeat");
runCmd(0, "irm -rf testIput_RF_repeat");
runCmd(0, "rm -rf testIput_RF_repeat");

# Basic inheritance tests (for SQL checks)
runCmd(1, "irm -rf $D4");
runCmd(0, "imkdir $D4");
runCmd(0, "ichmod inherit $D4");
runCmd(0, "iput $F1 $D4/$F1");
runCmd(0, "imkdir $D4A");
runCmd(0, "ils -A $D4");
runCmd(0, "ichmod -r inherit $D4A");
runCmd(0, "irm -rf $D4");

# Token tests
runCmd(0, "iadmin at user_type rodstest test");
runCmd(0, "iadmin lt user_type");
runCmd(0, "iadmin lt user_type rodstest");
runCmd(0, "iadmin rt user_type rodstest");

# Zone tests
runCmd(0, "iadmin mkzone $Zone2 remote host:1234 comment");
runCmd(2, "iadmin mkzone $Zone2 remote host:1234 comment");
runCmd(0, "iadmin mkuser $U3#$Zone2 rodsuser");
runCmd(0, "iadmin lu $U3#$Zone2");
runCmd(0, "iadmin luz $Zone2");
runCmd(0, "iadmin rmuser $U3#$Zone2");
runCmd(0, "iadmin modzone $Zone2 conn host2.foo:435");
runCmd(0, "iadmin modzone $Zone2 comment 'another comment'");
runCmd(0, "iadmin modzone $Zone2 name $Zone3");
runCmd(2, "iadmin rmzone $Zone2");
runCmd(0, "iadmin rmzone $Zone3");
runCmd(2, "iadmin rmzone $myZone");
runCmd(0, "echo yes | iadmin modzone $myZone name $Zone3");
$ENV{'IRODS_ZONE_NAME'}=$Zone3;
runCmd(0, "echo yes | iadmin modzone $Zone3 name $myZone");
delete $ENV{'IRODS_ZONE_NAME'};

# Make another user to test making user and for other tests
runCmd(1, "iadmin rmuser $U2");
runCmd(2, "iadmin mkuser $U2 invalidUserType");
runCmd(0, "iadmin mkuser $U2 rodsuser");

# A particular error and sql check
runCmd(2, "iput $F1 ../$U2");

# chmod
runCmd(0, "iput $F1");
runCmd(0, "ichmod read $U2 $F1");
runCmd(0, "ichmod write $U2 $F1");
runCmd(0, "ichmod null $U2 $F1");
runCmd(0, "ichmod -M write $U2 $F1");
runCmd(0, "ichmod -M null $U2 $F1");
runCmd(0, "irm -f $F1");
runCmd(0, "imkdir $D1");
runCmd(0, "iput $F1 $D1/$F1");
runCmd(0, "ichmod -r read $U2 $D1");
runCmd(2, "ichmod -r read $U2 $D1a");
runCmd(0, "ichmod -r null $U2 $D1");
runCmd(0, "ichmod read $U2 $D1");
runCmd(0, "ichmod null $U2 $D1");
runCmd(0, "irm -f -r $D1");

# chmod recursive
runCmd(1, "irm -rf $D3");
runCmd(0, "imkdir $D3");
runCmd(0, "imkdir $D3A");
runCmd(0, "imkdir $D3B");
runCmd(0, "imkdir $D3C");
runCmd(0, "imkdir $D3D");
runCmd(0, "imkdir $D3E");
runCmd(0, "iput $F1 $D3/$F1");
runCmd(0, "iput $F1 $D3A/$F1");
runCmd(0, "iput $F1 $D3B/$F1");
runCmd(0, "iput $F1 $D3C/$F1");
runCmd(0, "iput $F1 $D3D/$F1");
runCmd(0, "iput $F1 $D3E/$F1");
runCmd(0, "iput $F1 $D3E/$F2");
runCmd(0, "ichmod -r read $U2 $D3");
runCmd(0,"sleep 1");
runCmd(0, "ils -Ar $D3 | grep $U2 | grep read | wc -l");
$u2Lines = $cmdStdout;
chomp($u2Lines);
if ($u2Lines != 13) {
    die("recursive ichmod failed");
}
runCmd(0, "irm -rf $D3");

# modDataObjMeta
runCmd(0, "iput -k $F1");
runCmd(0, "iput -f $F1");
runCmd(0, "iput -fK $F1");
runCmd(0, "irm -f $F1");

# isysmeta
runCmd(0, "iput $F1");
runCmd(0, "isysmeta mod $F1 +1h");
runCmd(0, "isysmeta mod $F1 2009-12-01");
runCmd(0, "isysmeta mod $F1 datatype 'tar file'");
runCmd(0, "isysmeta mod $F1 comment 0 'my comment'");
runCmd(0, "isysmeta mod $F1 comment 'my other comment'");
runCmd(0, "isysmeta ls $F1");
runCmd(0, "isysmeta ls -l $F1");
runCmd(0, "isysmeta ls -v $F1");
runCmd(0, "isysmeta ldt");
runCmd(0, "irm -f $F1");

# Metadata
unlink($F3);
`ls -l > $F3`;
runCmd(1, "irm -f $F3");
runCmd(0, "iput $F3");
runCmd(1, "irm -fr $D1");
runCmd(1, "irm -fr $D2");
runCmd(0, "imkdir $D1");
runCmd(0, "imkdir $D2");
runCmd(0, "iput $F1");
runCmd(0, "imeta add -d $F1 a b c");
runCmd(0, "imeta ls -d $F1");
runCmd(0, "imeta qu -d a = b");
runCmd(0, "imeta qu -d a = b | wc");
runCmd(0, "imeta qu -d a = c | grep 'No rows found'");
runCmd(0, "imeta cp -d $F1 -d $F3");
runCmd(0, "imeta rm -d $F3 a b c");
runCmd(0, "imeta rm -d $F1 a b c");
runCmd(0, "imeta adda -d $F1 x y z");
runCmd(0, "imeta rm -d $F1 x y z");
runCmd(0, "imeta add -d $F1 testAVUnumber 14");
runCmd(0, "imeta qu -d testAVUnumber '<' 6 | wc -l", "2");
runCmd(0, "imeta qu -d testAVUnumber 'n<' 6 | grep 'No rows found'");
runCmd(0, "imeta qu -d testAVUnumber 'n>' 6 | wc -l", "2");
runCmd(0, "imeta qu -d testAVUnumber 'n=' 14 | wc -l", "2");
runCmd(0, "imeta rm -d $F1 testAVUnumber 14");
runCmd(0, "irm -f $F3");

runCmd(0, "imeta add -C $D1 a b c");
runCmd(0, "imeta ls -C $D1");
runCmd(0, "imeta cp -C $D1 -C $D2");
runCmd(0, "imeta rm -C $D2 a b c");
runCmd(0, "imeta qu -C a = b");
runCmd(0, "imeta qu -C a = b | wc");
runCmd(0, "imeta qu -C a = c | grep 'No rows found'");
runCmd(0, "imeta rm -C $D1 a b c");
runCmd(0, "imeta adda -C $D1 x y z");
runCmd(0, "imeta rm -C $D1 x y z");
runCmd(0, "irm -rf $D2");

runCmd(0, "imeta add -u $U2 d e f");
runCmd(0, "imeta ls -u $U2");
runCmd(0, "imeta cp -u $U2 -u $U1");
runCmd(0, "imeta rm -u $U1 d e f");
runCmd(0, "imeta cp -u $U2 -d $F1");
runCmd(0, "imeta rm -d $F1 d e f");
runCmd(0, "imeta cp -u $U2 -r $Resc");
runCmd(0, "imeta rm -r $Resc d e f");
runCmd(0, "imeta qu -u d = e");
runCmd(0, "imeta qu -u d = e | wc");
runCmd(0, "imeta qu -d a = c | grep 'No rows found'");
runCmd(0, "imeta rm -u $U2 d e f");

runCmd(0, "imeta add -R $Resc a b c");
runCmd(0, "imeta ls -R $Resc");
runCmd(0, "imeta qu -R a = b");
runCmd(0, "imeta qu -R a = b | wc");
runCmd(0, "imeta qu -R a = c | grep 'No rows found'");
runCmd(0, "imeta rm -R $Resc a b c");

runCmd(0, "imeta add -R $Resc x y");  # no units
runCmd(0, "imeta rm -R $Resc x y");

$RandomValue1=rand(9999999);
$RandomValue2=rand(9999999);
$RandomValue3=rand(9999999);
runCmd(0, "imeta add -R $Resc $RandomValue1 $RandomValue2");
runCmd(0, "imeta rm -R $Resc $RandomValue1 $RandomValue2");
runCmd(0, "imeta add -R $Resc $RandomValue1 $RandomValue2 $RandomValue3");
runCmd(0, "imeta rm -R $Resc $RandomValue1 $RandomValue2 $RandomValue3");

# imeta wildcard testing
runCmd(1, "imeta rmw -r $Resc % % %");
runCmd(0, "imeta add -r $Resc attr aaabbb");
runCmd(0, "imeta add -r $Resc attr_a test");
runCmd(0, "imeta add -r $Resc attr_ test2");
runCmd(0, "imeta ls -r $Resc | grep attribute | wc -l", "3");
runCmd(0, "imeta ls -r $Resc attr | grep attribute | wc -l", "1");
runCmd(0, "imeta ls -r $Resc attr_ | grep attribute | wc -l", "1");
runCmd(0, "imeta lsw -r $Resc attr_ | grep attribute | wc -l", "1");
runCmd(0, "imeta lsw -r $Resc attr_% | grep attribute | wc -l", "2");
runCmd(0, "imeta add -r $Resc a b ccc");
runCmd(0, "imeta rmw -r $Resc a % cc%");
runCmd(0, "imeta rmw -r $Resc attr% % %");
runCmd(0, "imeta ls -r $Resc | grep attribute | wc -l", "0");

runCmd(1, "imkdir $D1");
runCmd(1, "imeta rmw -c $D1 % % %");
runCmd(0, "imeta add -c $D1 attr aaabbb");
runCmd(0, "imeta add -c $D1 attr_a test");
runCmd(0, "imeta add -c $D1 attr_ test2");
runCmd(0, "imeta ls -c $D1 | grep attribute | wc -l", "3");
runCmd(0, "imeta ls -c $D1 attr | grep attribute | wc -l", "1");
runCmd(0, "imeta ls -c $D1 attr_ | grep attribute | wc -l", "1");
runCmd(0, "imeta lsw -c $D1 attr_ | grep attribute | wc -l", "1");
runCmd(0, "imeta lsw -c $D1 attr_% | grep attribute | wc -l", "2");
runCmd(0, "imeta rm -c $D1 attr_ test2");
runCmd(0, "imeta ls -c $D1 | grep attribute | wc -l", "2");
runCmd(0, "imeta rmw -c $D1 attr% % %");
runCmd(0, "imeta ls -c $D1 | grep attribute | wc -l", "0");

runCmd(1, "imeta rmw -u $U1 % % %");
runCmd(0, "imeta add -u $U1 role archivist");
runCmd(0, "imeta add -u $U1 role_a test");
runCmd(0, "imeta add -u $U1 role_ test2");
runCmd(0, "imeta ls -u $U1 | grep attribute | wc -l", "3");
runCmd(0, "imeta ls -u $U1#$myZone | grep attribute | wc -l", "3");
runCmd(0, "imeta ls -u $U1 role | grep attribute | wc -l", "1");
runCmd(0, "imeta ls -u $U1 role_ | grep attribute | wc -l", "1");
runCmd(0, "imeta lsw -u $U1 role_ | grep attribute | wc -l", "1");
runCmd(0, "imeta lsw -u $U1 role_% | grep attribute | wc -l", "2");
runCmd(0, "imeta rmw -u $U1 role% % %");
runCmd(0, "imeta ls -u $U1 | grep attribute | wc -l", "0");

runCmd(1, "imeta rmw -d $F1 % % %");
runCmd(0, "imeta add -d $F1 miles 10");
runCmd(0, "imeta add -d $F1 miles% 11");
runCmd(0, "imeta add -d $F1 miles_a 12");
runCmd(0, "imeta ls -d $F1");
runCmd(0, "imeta ls -d $F1 badName");
runCmd(0, "imeta ls -u $U1 badName");
runCmd(2, "imeta ls -d badName");
runCmd(2, "imeta ls -C badName");
runCmd(2, "imeta ls -R badName");
runCmd(2, "imeta ls -u badName");
runCmd(2, "imeta ls -d badName badName2");
runCmd(2, "imeta ls -C badName badName2");
runCmd(2, "imeta ls -R badName badName2");
runCmd(2, "imeta ls -u badName badName2");
runCmd(0, "imeta ls -d $F1 | grep attribute | wc -l", "3");
runCmd(0, "imeta rm -d $F1 miles% 11");
runCmd(0, "imeta ls -d $F1 | grep attribute | wc -l", "2");
runCmd(0, "imeta add -d $F1 miles% 11");
runCmd(0, "imeta ls -d $F1 | grep attribute | wc -l", "3");
runCmd(0, "imeta rmw -d $F1 miles% 11");
runCmd(0, "imeta ls -d $F1 | grep attribute | wc -l", "2");
runCmd(0, "imeta add -d $F1 miles% 11");
runCmd(0, "imeta ls -d $F1 miles | grep attribute | wc -l", "1");
runCmd(0, "imeta ls -d $F1 miles% | grep attribute | wc -l", "1");
runCmd(0, "imeta lsw -d $F1 miles% | grep attribute | wc -l", "3");

runCmd(0, "imeta rmw -d $F1 miles% %");
runCmd(0, "imeta ls -d $F1 | grep attribute | wc -l", "0");

# imeta using ID to delete
runCmd(0, "imeta add -d $F1 a b c");
unlink($IN_FILE);
runCmd(0, "echo 'test\nls -d $F1\nquit' > $IN_FILE");
runCmd(1, "imeta < $IN_FILE");
$ix = index($cmdStdout,"id: ");
$metaId=substr($cmdStdout, $ix+4, 6);
chomp($metaId);
unlink($IN_FILE);
runCmd(0, "echo 'test\nrmi -d $F1 $metaId\nquit' > $IN_FILE");
runCmd(0, "imeta < $IN_FILE");
unlink($IN_FILE);

# imeta mod command
runCmd(0, "imeta add -d $F1 a b c");
runCmd(0, "imeta mod -d $F1 a b c v:b2");
runCmd(0, "imeta mod -d $F1 a b2 c n:a2");
runCmd(0, "imeta mod -d $F1 a2 b2 c u:c2");
runCmd(0, "imeta mod -d $F1 a2 b2 c2 n:a3 v:b3 u:c3");
runCmd(0, "imeta rm -d $F1 a3 b3 c3");
runCmd(0, "imeta add -u $U1 aaaaa bbbbb ccccc");
runCmd(0, "imeta mod -u $U1 aaaaa bbbbb ccccc v:b222");
runCmd(0, "imeta rm -u $U1 aaaaa b222 ccccc");
runCmd(0, "imeta add -c $D1 aaaaa bbbbb ccccc");
runCmd(0, "imeta mod -c $D1 aaaaa bbbbb ccccc v:b222");
runCmd(0, "imeta rm -c $D1 aaaaa b222 ccccc");
runCmd(0, "imeta add -r $Resc aaaaa bbbbb");
runCmd(0, "imeta mod -r $Resc aaaaa bbbbb n:a1");
runCmd(2, "imeta rm -r $Resc aaaaa bbbbb");
runCmd(0, "imeta rm -r $Resc a1 bbbbb");

# imeta set
runCmd(1, "imeta rm -d $F1 setAttr1 y z");
runCmd(0, "iadmin rum");
unlink($F2);
`ls -l > $F2`;
runCmd(0, "iput -f $F2");
runCmd(0, "imeta set -d $F1 setAttr1 y z");
runCmd(0, "imeta set -d $F1 setAttr1 y2 z");
runCmd(0, "imeta set -d $F1 setAttr1 y z");
runCmd(0, "imeta set -d $F2 setAttr1 y z");
runCmd(0, "imeta set -d $F1 setAttr1 y3 z");
runCmd(0, "imeta rm -d $F2 setAttr1 y z");
runCmd(0, "imeta rm -d $F1 setAttr1 y3 z");
runCmd(0, "iadmin rum");
runCmd(0, "imeta set -d $F1 setAttr2 1");
runCmd(0, "imeta set -d $F1 setAttr2 2");
runCmd(0, "imeta set -d $F2 setAttr3 3");
runCmd(0, "imeta set -d $F1 setAttr3 3");
runCmd(0, "imeta rm -d $F1 setAttr2 2");
runCmd(0, "irm -f $F2");

# basic test of imeta addw
runCmd(0, "imeta addw -d $F1 a4 b4 c4");
runCmd(0, "imeta rm -d $F1 a4 b4 c4");

runCmd(0, "irm -f $F1");
runCmd(0, "irm -f -r $D1");

# ModUser
$UA = $U2 . "a";
#runCmd(0, "iadmin moduser $U2 name $UA");  # moduser name no longer allowed
#runCmd(0, "iadmin moduser $UA name $U2");  # moduser name no longer allowed
runCmd(2, "iadmin moduser $U2 type badUserType");
runCmd(0, "iadmin moduser $U2 type rodsadmin");
runCmd(0, "iadmin moduser $U2 type rodsuser");
runCmd(0, "iadmin aua $U2 asdfsadfsadf/dfadsf/dadf/d/");
runCmd(0, "iadmin lua $U2");
runCmd(0, "iadmin lua $U2#$myZone");
runCmd(0, "iadmin lua");
runCmd(0, "iadmin luan asdfsadfsadf/dfadsf/dadf/d/");
runCmd(0, "iadmin rua $U2 asdfsadfsadf/dfadsf/dadf/d/");
#runCmd(0, "iadmin moduser $U2 zone badZone");

runCmd(0, "iadmin moduser $U2 zone $myZone");
runCmd(0, "iadmin moduser $U2 comment 'this is a comment'");
runCmd(0, "iadmin moduser $U2 comment ''");
runCmd(0, "iadmin moduser $U2 info 'this is Info field'");
runCmd(0, "iadmin moduser $U2 info ''");
runCmd(0, "iadmin moduser $U2 password 'abc'");
runCmd(0, "iadmin moduser $U2 password '1234'");
runCmd(2, "iadmin moduser $UA password '1234'");

# For groupadmin test below, make sure G! doesn't exist
runCmd(1, "iadmin rmgroup $G1");


# Auth as non-admin user and test groupadmin SQL
runCmd(0, "iadmin moduser $U2 type groupadmin");
unlink($F2);
$MYHOME=$ENV{'HOME'};
$authFile="$MYHOME/.irods/.irodsA";
runCmd(0, "ienv | grep irods_authentication_file | tail -1");
chomp($cmdStdout);
$ix = index($cmdStdout,"-");
$envAuth=substr($cmdStdout, $ix+1);
$envAuth =~ s/^\s+//;
#if ($envAuth ne "") {
if ($ix != -1) {
    $authFile=$envAuth;
}
runCmd(1, "mv $authFile $F2"); # save the auth file
runCmd(2, "iinit 1234");
$ENV{'IRODS_USER_NAME'}=$U2;
runCmd(0, "iinit 1234");
runCmd(2, "iadmin atg g1 user3"); # test SQL (just needs to be groupadmin to)
runCmd(0, "igroupadmin mkgroup $G1");
runCmd(0, "igroupadmin atg $G1 $U2"); # need to add self to group 1st
runCmd(0, "igroupadmin atg $G1 $U1");
runCmd(0, "igroupadmin rfg $G1 $U1");
runCmd(2, "ichmod -R write $U1 $Resc"); # test SQL (the non-admin)
runCmd(0, "echo '1234\nabcd\nabcd' | ipasswd"); # change the password
runCmd(0, "echo 'abcd\n1234\n1234' | ipasswd"); # change the password back
runCmd(0, "iinit --ttl 1 1234"); # also run new SQL for TTL passwords
runCmd(0, "iexit full");
runCmd(1, "mv $F2 $authFile"); # restore auth file
delete $ENV{'IRODS_USER_NAME'};
runCmd(0, "ils");

# Large numbers of temporary passwords.
# MAX_PASSWORDS in icatHighLevelRoutines.c is 40 so make more than
# that to exercise the SQL that handles that case.
$count=42;
for ($i=0;$i<$count;$i++) {
    runCmd(0,"test_chl tpw rods > /dev/null 2>&1");
}
# https://github.com/irods/irods/issues/1855
# break up the timestamps to avoid infinite loop
#for ($i=0;$i<$count/2;$i++) {
#    runCmd(0,"test_chl tpw rods > /dev/null 2>&1");
#}
#runCmd(0,"sleep 1");
#for ($i=0;$i<$count/2;$i++) {
#    runCmd(0,"test_chl tpw rods > /dev/null 2>&1");
#}

# Also make sure the new SQL works: that the password can be used.
# This uses 'iinit' with the temp password, which normally isn't done,
# but it can be used to test this way.
# Also note that if expired, these temp passwords should be removed
# when a valid temp password is used, but since they are not yet
# expired they won't be.  They should be cleaned up next time the test
# is run tho.
runCmd(0,"sleep 1");
runCmd(0,"test_chl tpw rods 2>&1 | grep derived");
$ix=index($cmdStdout,"=");
$pw=substr($cmdStdout,$ix+1);
runCmd(1, "mv $authFile $F2"); # save the auth file
runCmd(0, "iinit $pw");
runCmd(1, "mv $F2 $authFile"); # restore auth file
runCmd(0, "ils");

# group
$G1A = $G1 . "a";
runCmd(1, "iadmin rmgroup $G1");
runCmd(0, "iadmin mkgroup $G1");
runCmd(2, "iadmin atg $G1 $UA");
runCmd(2, "iadmin atg $G1A $UA");
runCmd(1, "iadmin rfg $G1 $U2");
runCmd(0, "iadmin atg $G1 $U2");
runCmd(2, "iadmin rfg $G1 $UA");
runCmd(0, "iadmin rfg $G1 $U2");
runCmd(2, "iadmin rfg $G1 $U2");
runCmd(0, "iadmin rmgroup $G1");

# basic quota tests
runCmd(1, "irm -f $F1");
runCmd(0, "iput $F1");
runCmd(0, "iadmin suq $U1 $Resc 10");
runCmd(1, "iadmin rmgroup $G1");
runCmd(0, "iadmin mkgroup $G1");
runCmd(0, "iadmin atg $G1 $U1");
runCmd(0, "iadmin sgq $G1 $Resc 10");
runCmd(0, "iadmin cu");
runCmd(0, "iadmin suq $U1 total 20");
runCmd(0, "iadmin sgq $G1 total 20");
runCmd(0, "iadmin cu");
runCmd(0, "iquota");
runCmd(0, "iquota -a");
runCmd(0, "iquota -u baduser");
runCmd(0, "iquota -u $U2");
runCmd(0, "iquota usage");
runCmd(0, "iquota -a usage");
runCmd(0, "iadmin suq $U1 total 0");
runCmd(0, "iadmin sgq $G1 total 0");
runCmd(0, "iadmin suq $U1 $Resc 0");
runCmd(0, "iadmin sgq $G1 $Resc 0");
runCmd(0, "iadmin lq");
runCmd(0, "iadmin lq $U1");
runCmd(0, "iadmin rmgroup $G1");
runCmd(0, "irm -f $F1");

# RegResource; use the same host as demoResc is defined on so
# this script can run from any client host
runCmd(0, "iadmin lr $Resc | grep resc_net:");
$ipIx=index($cmdStdout,"net:");
$hostName=substr($cmdStdout, $ipIx+4);
$hostName = `hostname`;
chomp($hostName);
printf("hostName:%s\n",$hostName);

runCmd(1, "iadmin rmresc $Resc2");
runCmd(0, "echo yes | iadmin modresc $Resc name $Resc2");
runCmd(0, "echo yes | iadmin modresc $Resc2 name $Resc");
runCmd(0, "iadmin mkresc  $Resc2 'unix file system' $hostName:$Resc2Path");
runCmd(0, "iadmin modresc $Resc2 comment 'this is a comment'");
runCmd(0, "iadmin modresc $Resc2 freespace 123456789");
runCmd(0, "iadmin modresc $Resc2 info 'this is info field'");
#runCmd(2, "iadmin modresc $Resc2 class 'badClass'");
#runCmd(0, "iadmin modresc $Resc2 class 'archive'");
#runCmd(0, "iadmin modresc $Resc2 class cache");
#runCmd(2, "iadmin modresc $Resc2 type 'badType'"); # irods no longer checks/cares
runCmd(0, "iadmin modresc $Resc2 type 'unix file system'");
runCmd(0, "iadmin modresc $Resc2 path /tmp/v1");
runCmd(0, "iadmin modresc $Resc2 path $Resc2Path");
runCmd(0, "iadmin modresc $Resc2 status up");

runCmd(0, "iadmin lr $Resc2");
runCmd(0, "iput -R $Resc2 $F1");


# After a file is put, so that at least one will be moved
runCmd(0, "echo yes | iadmin modrescdatapaths $Resc2 $Resc2Path/ /tmp/v1/");
runCmd(0, "echo yes | iadmin modrescdatapaths $Resc2 /tmp/v1/ $Resc2Path/ rods");


# repl
runCmd(0, "irepl -R $Resc $F1");
runCmd(0, "irepl -R $Resc $F1"); # a second irepl to test a problem fixed Oct 2013
runCmd(0, "irm -f $F1");
runCmd(0, "imkdir $D1");
runCmd(0, "icd $D1");
unlink($F2);
`ls -l > $F2`;
runCmd(0, "iput $F1");
runCmd(0, "iput $F2");
runCmd(0, "ipwd");
runCmd(0, "icd");
runCmd(0, "irepl -r -R $Resc2 $D1");
runCmd(0, "irepl -r -R $Resc2 $D1");  # a 2nd irepl to test an Oct 2013 fix
runCmd(0, "irm -f -r $D1");

# Resource Groups
$Resc2A = $Resc2 . "a";
#runCmd(0, "iadmin atrg $RG1 $Resc");
#runCmd(0, "iadmin lrg $RG1");
#runCmd(0, "iadmin atrg $RG1 $Resc2");
#runCmd(2, "iadmin atrg $RG1 $Resc2A");
#runCmd(1, "iadmin rfrg $RG1 $Resc");
#runCmd(0, "imeta add -g $RG1 1111 2222");
#runCmd(0, "imeta cp -g $RG1 -r $Resc");
#runCmd(0, "imeta rm -g $RG1 1111 2222");
#runCmd(0, "imeta rm -r $Resc 1111 2222");
#runCmd(1, "iadmin rfrg $RG1 $Resc2");
#runCmd(0, "iadmin lrg $RG1");

# Rules
# TGR removed vacuum - TODO: find another rule to add
#runCmd(0, "iadmin pv 2030-12-31");
#runCmd(0, "iqstat | grep msiVacuum");
#$ix = index($cmdStdout, " ");
#$id = substr($cmdStdout, 0, $ix);
#chomp($id);
#$idBad = $id+232324;
#runCmd(2, "iqdel $idBad");
#runCmd(0, "iqmod $id exeAddress test");
#runCmd(0, "iqdel $id");

# Simple Query
runCmd(0, "iadmin lt");
runCmd(0, "iadmin lt zone_type");
runCmd(0, "iadmin lt zone_type local");
runCmd(0, "iadmin lr");
runCmd(0, "iadmin lr demoResc");
runCmd(0, "iadmin lz");
runCmd(0, "iadmin lz $myZone");
runCmd(0, "iadmin lg");
runCmd(0, "iadmin lg rodsadmin ");
runCmd(0, "iadmin lgd rodsadmin");
runCmd(1, "iadmin lf 10009");
runCmd(0, "iadmin ls");
runCmd(0, "iadmin lu");
runCmd(0, "iadmin lu rods");
runCmd(1, "iadmin lrg");
runCmd(1, "iadmin dspass 123");

# A special case for restoring owner access
unlink($F1);
`ls -l > $F1`;
runCmd(0, "iput $F1");
runCmd(0, "ichmod null $U1 $F1"); # remove our own access
unlink($F1);
runCmd(2, "iget $F1");
runCmd(0, "ichmod own $U1 $F1");  # should be able to restore access
runCmd(0, "iget $F1");
runCmd(0, "irm $F1");
runCmd(0, "irmtrash");
runCmd(0, "imkdir $D1");
runCmd(0, "iput $F1 $D1");
runCmd(0, "irm $D1/$F1");
runCmd(0, "ichmod read $U1 $D1");
runCmd(2, "iput $F1 $D1");
runCmd(0, "ichmod write $U1 $D1");
runCmd(0, "iput $F1 $D1");
runCmd(0, "irm $D1/$F1");
runCmd(0, "ichmod own $U1 $D1");
runCmd(0, "irm -rf $D1");
runCmd(0, "irmtrash");

#
# Admin mode of irmtrash
#
runCmd(0, "iput $F1");
runCmd(0, "irm $F1");
runCmd(0, "irmtrash -M");

#
# ichmod for DB Resource
#
runCmd(0, "ichmod -R write $U2 $Resc");
runCmd(0, "ichmod -R null $U2 $Resc");

#
# chmod, test a case that use to fail as the dir% directory
# would match the dir1 directory (icat routine was using 'like'),
$LD1="dir1";
$LD2="dir%";
$LD1S1="dir1/s1";
$LD1S2="dir1/s2";
runCmd(1, "irm -fr $LD1");
runCmd(1, "irm -rf $LD2");
runCmd(1, "imkdir $LD1");
runCmd(1, "imkdir $LD2");
runCmd(1, "imkdir $LD1S1");
runCmd(0, "iput $F1 $LD2");
runCmd(0, "iput $F1 $LD1S1");
runCmd(0, "iput $F1 $LD1S2");
# test that it sets all that it should
runCmd(0, "ichmod -r read $U2 $LD1");
runCmd(0,"sleep 1");
runCmd(0, "ils -rA $LD1 $LD2 | grep read | wc -l", "4");
runCmd(0, "ichmod -r null $U2 $LD1");
runCmd(0,"sleep 1");
runCmd(0, "ils -rA $LD1 $LD2 | grep read | wc -l", "0");
# test that is doesn't set too many
runCmd(0, "ichmod -r read $U2 $LD2");
runCmd(0,"sleep 1");
runCmd(0, "ils -rA $LD1 $LD2 | grep read | wc -l", "2");
# for check that delete works in sql
runCmd(0, "ichmod -r read $U2 $LD2");
runCmd(0, "ichmod -r read $U2 $LD2");
runCmd(0,"sleep 1");
runCmd(0, "ils -rA $LD1 $LD2 | grep read | wc -l", "2");
# check removing access
runCmd(0, "ichmod -r null $U2 $LD2");
runCmd(0,"sleep 1");
runCmd(0, "ils -rA $LD1 $LD2 | grep read | wc -l", "0");
# check removing the files
runCmd(0, "irm -fr $LD1");
runCmd(0, "irm -rf $LD2");

# specific query
runCmd(0, "iadmin asq 'select user_name from R_USER_MAIN'");
runCmd(0, "iquest --sql 'select user_name from R_USER_MAIN'");
runCmd(0, "iadmin rsq 'select user_name from R_USER_MAIN'");
runCmd(0, "iadmin asq 'select user_name from R_USER_MAIN' testAlias");
runCmd(0, "iquest --sql testAlias");
runCmd(0, "iadmin rsq testAlias");

# Queries with between and in, null and not null
runCmd(0, "iquest \"select RESC_NAME where RESC_CLASS_NAME IN ('bundle','archive')\"");
runCmd(0, "iquest \"select USER_NAME where USER_ID between '10000' '10110'\"");
runCmd(0, "iadmin moduser $U2 info 'this is Info field'"); # in Oracle 0-len is null
runCmd(0, "iquest \"select USER_NAME, USER_INFO where USER_INFO IS NOT NULL\"");
runCmd(0, "iquest \"select USER_NAME, USER_INFO where USER_INFO IS NULL\"");

# Queries with min, max, sum, avg, and count
runCmd(1, "irm -f $F1");
runCmd(0, "iput $F1");
runCmd(0, "iput $F2");
runCmd(0, "ipwd");
chomp($cmdStdout);
$iHome=$cmdStdout;
runCmd(0, "iquest \"select min(DATA_SIZE) where COLL_NAME = '$iHome'\"");
runCmd(0, "iquest \"select max(DATA_SIZE) where COLL_NAME = '$iHome'\"");
runCmd(0, "iquest \"select avg(DATA_SIZE) where COLL_NAME = '$iHome'\"");
runCmd(0, "iquest \"select sum(DATA_SIZE) where COLL_NAME = '$iHome'\"");
runCmd(0, "iquest \"select count(DATA_SIZE) where COLL_NAME = '$iHome'\"");
runCmd(0, "iquest \"select order_desc(DATA_SIZE) where COLL_NAME = '$iHome'\"");
runCmd(0, "irm -f $F2");
runCmd(0, "irm -f $F1");

# Queries with that find no rows should now (9/26/13) exit success (0)
runCmd(0, "iquest \"select USER_NAME, USER_ID where USER_ID = '8005'\"");
runCmd(0, "iquest --sql lsl badName");

#Tickets
runCmd(0, "iput $F1");
runCmd(1, "iticket delete $TICKET1");
runCmd(0, "iticket create read $F1 $TICKET1");
runCmd(0, "iticket mod $TICKET1 uses 10");
runCmd(1, "iticket mod badticketname uses 1");
runCmd(0, "iticket mod $TICKET1 add host $TICKET_HOST");
runCmd(0, "iticket mod $TICKET1 remove host $TICKET_HOST");
runCmd(0, "iticket mod $TICKET1 add user $U1");
runCmd(0, "iticket mod $TICKET1 remove user $U1");
runCmd(0, "iticket mod $TICKET1 expire 2012-02-02");
runCmd(0, "iticket mod $TICKET1 expire 0");
runCmd(0, "iticket ls $TICKET1");
runCmd(0, "iticket ls");
runCmd(0, "iget -f -t $TICKET1 $F1");
runCmd(0, "iticket mod $TICKET1 uses 0");
runCmd(0, "iadmin mkgroup $G1");
runCmd(0, "iticket mod $TICKET1 add group $G1");
runCmd(1, "iget -f -t $TICKET1 $F1");
runCmd(0, "iticket mod $TICKET1 remove group $G1");
runCmd(0, "iticket delete $TICKET1");
runCmd(1, "iticket delete $TICKET2");
runCmd(0, "iticket create write $F1 $TICKET2");
runCmd(0, "iticket mod $TICKET2 write-file 10");
runCmd(0, "iticket mod $TICKET2 write-byte 10000000");
runCmd(0, "iget -f -t $TICKET2 $F1");
runCmd(0, "ls -l >> $F1");
runCmd(0, "iput -f -t $TICKET2 $F1");
runCmd(0, "iticket delete $TICKET2");
runCmd(1, "iticket delete $TICKET3");
runCmd(1, "irm -fr $D2");
runCmd(0, "imkdir $D2");
runCmd(0, "iticket create write $D2 $TICKET3");
runCmd(0, "iput -t $TICKET3 $F1 $D2");
runCmd(0, "iticket delete $TICKET3");
runCmd(1, "irm -fr $D2");
runCmd(0, "iadmin rmgroup $G1");
runCmd(1, "iticket create read badfilename");
runCmd(0, "irm -f $F1");

# simple test to exercise the clean-up AVUs sql;
# will return CAT_SUCCESS_BUT_WITH_NO_INFO if there were none
runCmd(1, "iadmin rum");

# before removing, test changing the host
runCmd(0, "iadmin modresc $Resc2 host foo.sdsc.edu");
runCmd(0, "iadmin lr $Resc2");

# finished with temp resource
runCmd(0, "iadmin rmresc $Resc2");
runCmd(0, "rm -rf $Resc2Path");

# Test rmuser and clean up user added for test
runCmd(0, "iadmin rmuser $U2");

printf("Success\n");
