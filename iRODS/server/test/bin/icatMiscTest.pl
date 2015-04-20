#!/usr/bin/perl -w
#
# Copyright (c), The Regents of the University of California            ***
# For more information please refer to files in the COPYRIGHT directory ***
#
# This is a test script that runs various test_chl commands and various
# icommands to test iCat functions.  Most iCAT functions and
# sql-statements are exercised via icatTest.pl, moveTest.pl,
# clients/icommands/test/testiCommands.pl, and this script exercises a
# few more so that all are tested.  The ones done here are either
# difficult to set up for testing or are not currently used.

# This needs to run on the same host as the ICAT-enabled iRODS server.
# The test_chl and test_genq utilities link with the icat library to
# directly test some of the functions and cases that are difficult to
# exercise.

# To use with checkIcatLog.pl, redirect stdout to a file.  Since test_chl
# is using the ICAT code directly, the calls are not being logged.

# Corresponds to a .h value for regular user authentication level:
$LOCAL_USER_AUTH="3";



#
# Get bearings on installation details
#
use Cwd "abs_path";
use File::Basename;
use File::Spec;
$scriptfullpath = abs_path(__FILE__);
$scripttoplevel = dirname(dirname(dirname(dirname(dirname($scriptfullpath)))));
require "$scripttoplevel/iRODS/scripts/perl/utils_platform.pl";
require "$scripttoplevel/iRODS/scripts/perl/utils_config.pl";
if( -e "/etc/irods/server_config.json" )
{
        $configDir = "/etc/irods";
}
else
{
        $configDir = File::Spec->catdir( "$scripttoplevel", "iRODS", "config" );
}
$logDir = File::Spec->catdir( "$scripttoplevel", "iRODS", "server", "log" );



# get our zone name
runCmd(0, "ienv | grep irods_zone_name | tail -1");
chomp($cmdStdout);
$ix = index($cmdStdout,"-");
$myZone=substr($cmdStdout, $ix+1);
$myZone =~ s/^\s+//;

$F1="TestFile1";
$F2="TestFile2";
$Resc="demoResc";
$HOME="/$myZone/home/rods";
$Resc2="resc2";
$Resc2Path="/tmp/Vault";
$User2="user2";
$UserAdmin2="admin2";
$DIR1="$HOME/testDir1";
$DIR2="$HOME/testDir1DoesNotExist";
$tmpPwFile="/tmp/testPwFile.5678956";
$tmpAuthFile="/tmp/testAuthFile.12343598";
$USER="rods";
$ACCESS_GOOD="'read metadata'";
$ACCESS_BAD="'bad access'";


# run a command
# if option is 0 (normal), check the exit code and fail if non-0
# if 1, don't care
# if 2, should get a non-zero result, exit if not
sub runCmd {
    my($option, $cmd) = @_;
    use File::Basename;
    my $thescriptname = basename($0);
    chomp(my $therodslog = `ls -t $logDir/rodsLog* | head -n1`);
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

# Set the environment variable for the config dir since
# this is now one more level down.
$ENV{'irodsConfigDir'}=`perl \$`;

# GenQuery options
runCmd(0, "test_genq gen7 i");
runCmd(0, "test_genq gen7 i 1 10");
runCmd(2, "test_genq gen7 i 20 10");
runCmd(0, "test_genq gen8 / 0 10");
runCmd(2, "test_genq gen8 abc 0 10");
runCmd(0, "test_genq gen7 i 0 8 10"); # test totalRowCount
runCmd(0, "test_genq gen15 i 0 8"); # test AUTO_CLOSE

# GenQuery options to check access; exercise cmlCheckDirId
runCmd(0, "test_genq gen10 $USER $myZone $ACCESS_GOOD $HOME");
runCmd(0, "test_genq gen10 $USER $myZone $ACCESS_BAD $HOME");

# Scan the tables for possible cycles in the links between tables.
runCmd(0, "test_genq -1");

# Mod resource freespace
runCmd(0, "test_chl modrfs $Resc 123456789");
runCmd(0, "iadmin lr $Resc | grep -i free_space: | grep 123456789");

# Mod and rollback should not commit change
runCmd(0, "test_chl modrfs $Resc 987654321 rollback");
runCmd(0, "iadmin lr $Resc | grep -i free_space: | grep 123456789");

# Mod without commit should auto-commit on normal completion
# (since auditing is on, via debug ( irods_debug set to CATSQL )
runCmd(0, "test_chl modrfs $Resc 987654321 close");
runCmd(0, "iadmin lr $Resc | grep -i free_space: | grep 987654321");

# Mod without audit should not auto-commit
$ENV{'IRODS_DEBUG'}='noop'; # override value in irodsEnv file
runCmd(0, "test_chl modrfs $Resc 123456789 close");

load_server_config("$configDir/server_config.json");
# TGR - Jan 2015 - move to server_config.json
# hard-code default password - only run on new installations
$IRODS_ADMIN_PASSWORD = "rods";

if ($DATABASE_TYPE eq "oracle") {
#   oracle does autocommit so don't check the result
    runCmd(1, "iadmin lr $Resc | grep -i free_space: | grep 123456789");
}
else {
#   but postgres does, so check it
    runCmd(2, "iadmin lr $Resc | grep -i free_space: | grep 123456789");
}
delete $ENV{'IRODS_DEBUG'};

runCmd(0, "test_chl modrfs $Resc ''");

# Mod DataObj Meta without a dataID
unlink($F1);
runCmd(1, "irm -f $F1");
runCmd(0, "ls -l > $F1");
runCmd(0, "iput $F1");
runCmd(0, "test_chl mod /$myZone/home/rods/$F1 text $F1");
runCmd(0, "irm -f $F1");

# Admin form of UnregDataObj
runCmd(1, "iadmin rmresc $Resc2");
runCmd(0, "iadmin lr $Resc | grep resc_net:");
$ipIx=index($cmdStdout,"net:");
$hostName=substr($cmdStdout, $ipIx+4);
$hostName = `hostname`;
chomp($hostName);
printf("hostName:%s\n",$hostName);
runCmd(0,"iadmin mkresc $Resc2 'unix file system' $hostName:$Resc2Path");
runCmd(0, "iput $F1");
runCmd(0, "iadmin ls $HOME | grep $F1");
$index=index($cmdStdout," ");
$t1=substr($cmdStdout, $index+1);
$index=index($t1," ");
$dataID=substr($t1,0,$index);
printf("dataID=%s\n",$dataID);
runCmd(0, "irepl -R $Resc2 $F1");
runCmd(2, "test_chl rmpriv $HOME/$F1 $dataID 2"); # should fail, bad replica
runCmd(0, "test_chl rmpriv $HOME/$F1 $dataID 1");
runCmd(2, "test_chl rmpriv $HOME/$F1 $dataID 0"); # should fail, last replica
runCmd(0, "irm -f $F1");
runCmd(0, "iput $F1");
runCmd(0, "irepl -R $Resc2 $F1");
runCmd(0, "test_chl rm $HOME/$F1 0");
runCmd(0, "test_chl rm $HOME/$F1 1");
runCmd(2, "irm -f $F1");
runCmd(0, "iput $F1");
runCmd(0, "test_chl rm $HOME/$F1 999999"); # 999999 is taken as -1

# server style login/auth (altho actually redundant with other tests)
runCmd(1, "iadmin rmuser $User2");
runCmd(0, "iadmin mkuser $User2 rodsuser");
runCmd(0, "iadmin moduser $User2 password 123");
#$ENV{'IRODS_USER_NAME'}=$User2;
runCmd(0, "test_chl login $User2 123 $IRODS_ADMIN_PASSWORD");
#delete $ENV{'IRODS_USER_NAME'};

# Check non-admin access for deleting a rule
# TGR - removed vacuum, TODO: find a substitute for this
#runCmd(0, "iadmin pv 2030-12-31");
#runCmd(0, "iqstat | grep msiVacuum");
#$ix = index($cmdStdout, " ");
#$id = substr($cmdStdout, 0, $ix);
#chomp($id);
#$ENV{'IRODS_USER_NAME'}=$User2;
#runCmd(2, "echo 123 | iqdel $id");
#runCmd(2, "test_chl rmrule $id $User2");
#delete $ENV{'IRODS_USER_NAME'};
#runCmd(0, "iqdel $id");

# Temporary password
$ENV{'IRODS_USER_NAME'}=$User2;
$prevAuthFileName=$ENV{'IRODS_AUTHENTICATION_FILE'};  # old one, if any
$ENV{'IRODS_AUTHENTICATION_FILE'}=$tmpPwFile;
runCmd(0, "test_chl tpw 123 | grep  'temp pw'");
$temp1=$cmdStdout;
chomp($temp1);
$ixPw=index($temp1,"=");
$pw=substr($temp1, $ixPw+1);
unlink($tmpPwFile);
unlink($tmpAuthFile);
runCmd(0, "echo $pw > $tmpPwFile");
runCmd(0, "ils ../$User2 < $tmpPwFile");
delete $ENV{'IRODS_USER_NAME'};
if ($prevAuthFileName eq "") {
    delete $ENV{'IRODS_AUTHENTICATION_FILE'};
}
else {
    $ENV{'IRODS_AUTHENTICATION_FILE'}=$prevAuthFileName;
}
unlink($tmpPwFile);

# Temporary password for other
$prevAuthFileName=$ENV{'IRODS_AUTHENTICATION_FILE'};  # old one, if any
$ENV{'IRODS_AUTHENTICATION_FILE'}=$tmpPwFile;
runCmd(0, "test_chl tpwforother $IRODS_ADMIN_PASSWORD $User2 | grep  'temp pw'");
$temp1=$cmdStdout;
chomp($temp1);
$ixPw=index($temp1,"=");
$pw=substr($temp1, $ixPw+1);
unlink($tmpPwFile);
unlink($tmpAuthFile);
$ENV{'IRODS_USER_NAME'}=$User2;
runCmd(0, "echo badpw > $tmpPwFile");
runCmd(2, "ils ../$User2 < $tmpPwFile");  # should fail with a bad pw
unlink($tmpPwFile);
runCmd(0, "echo $pw > $tmpPwFile");
runCmd(0, "ils ../$User2 < $tmpPwFile");  # should work with the temp pw
printf($cmdStdout);
runCmd(2, "ils ../$User2 < $tmpPwFile");  # but only once
delete $ENV{'IRODS_USER_NAME'};
if ($prevAuthFileName eq "") {
    delete $ENV{'IRODS_AUTHENTICATION_FILE'};
}
else {
    $ENV{'IRODS_AUTHENTICATION_FILE'}=$prevAuthFileName;
}
unlink($tmpPwFile);

# auth test (special case)
runCmd(1, "iadmin rmuser $UserAdmin2");
runCmd(0, "iadmin mkuser $UserAdmin2 rodsadmin");
runCmd(0, "iadmin moduser $UserAdmin2 password abc");
runCmd(0, "test_chl checkauth $UserAdmin2 $User2 $myZone");
$temp2=$cmdStdout;
chomp($temp2);
$ix2=index($temp2,"clientPrivLevel=$LOCAL_USER_AUTH");
if ($ix2 < 0) {
    printf("stdout is:$temp2:");
    die("For checkauth on $user2 the clientPrivLevel is not $LOCAL_USER_AUTH");
}

# modColl test
runCmd(1, "imkdir $DIR1");
runCmd(0, "test_chl modc $DIR1 type2 '' ''");
runCmd(0, "test_chl modc $DIR1 '' info1 info2");
runCmd(2, "test_chl modc $DIR2 '' info1 info2");
runCmd(0, "test_chl modc $DIR1 NULL_SPECIAL_VALUE '' ''");
runCmd(0, "irm -rf $DIR1");

# generalUpdate tests
# (no longer used for non-Extension ICAT tables)
#runCmd(1, "test_genu 2 test 100078 name1");
#runCmd(0, "test_genu 1 test 100078 name1");
#runCmd(2, "test_genu 2 test 100099 name1");
#runCmd(0, "test_genu 2 test 100078 name1");
#
# Oct 2011, re-added test_genu tests to improve coverage a little.
# Without defining an extended ICAT (which would be difficult),
# coverage will be slight tho.
runCmd(2, "test_genu 1 test 100078 name1");
runCmd(2, "test_genu 2 test 100078 name1");

# multiple open/close test
runCmd(0, "test_chl open");

# new serverLoad functions
runCmd(0, "test_chl serverload 123");
runCmd(0, "test_chl purgeload -1");
runCmd(0, "test_chl serverdigest 456");
runCmd(0, "test_chl purgedigest -1");

# quota check
runCmd(0, "test_chl checkquota $User2 $Resc");

# check rule table updates
# (without a commit, does not get saved)
runCmd(0, "test_chl addrule a b c d e f g");
#
runCmd(0, "test_chl versionrulebase a");
runCmd(0, "test_chl versiondvmbase a");
runCmd(0, "test_chl versionfnmbase a");
runCmd(0, "test_chl insfnmtable a b c d");
runCmd(0, "test_chl insmsrvctable a b c d e f g h 1");
runCmd(0, "test_chl insdvmtable a b c d");

# check the iRODS-PAM password generation and expired deletion
runCmd(0, "test_chl getpampw $User2 01335433300"); #create expired irods-pam pw
$temp2=$cmdStdout;
chomp($temp2);
$ix2=index($temp2,"pw=");
$str1=substr($temp2, $ix2+3);
$ix3=index($str1," ");
$ipamPw=substr($str1,0,$ix3);
$ENV{'IRODS_USER_NAME'}=$User2;
$ENV{'IRODS_AUTHENTICATION_FILE'}="/tmp/xfile1";
runCmd(2, "echo $ipamPw | ils");  # should fail and should do SQL to rm pw
delete $ENV{'IRODS_USER_NAME'};
delete $ENV{'IRODS_AUTHENTICATION_FILE'};
runCmd(0, "test_chl getpampw $User2"); # create a normal one
runCmd(0, "test_chl getpampw $User2"); # do a second one to get an existing pw
runCmd(0, "iadmin rpp $User2");   # exercise the remove irods-PAM pw SQL

# clean up
runCmd(0, "iadmin rmuser $UserAdmin2");
runCmd(0, "iadmin rmuser $User2");
runCmd(0, "iadmin rmresc $Resc2");
runCmd(0, "rm -rf $Resc2Path");

# Check that an invalid simpleQuery does fail
$testCmd="test_chl sql \"select coll_id from R_COLL_MAIN where coll_name != " . "?" . "\" /$myZone/home/rods 1";
runCmd(2, $testCmd);

# Exercise a couple simple queries that are no longer used in iadmin:
$testCmd="test_chl sql \"select user_name from R_USER_MAIN where user_type_name='rodsgroup'\"";
runCmd(0, $testCmd);
$testCmd="test_chl sql \"select user_name||'#'||zone_name from R_USER_MAIN, R_USER_GROUP where R_USER_GROUP.user_id=R_USER_MAIN.user_id and R_USER_GROUP.group_user_id=(select user_id from R_USER_MAIN where user_name=" . "?)" . "\" g1 1";
runCmd(1, $testCmd);

# Exercise the chlGetLocalZone for coverage
runCmd(0, "test_chl getlocalzone $myZone");

printf("Success\n");
