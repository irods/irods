#!/usr/bin/perl
#
# Copyright (c), The Regents of the University of California            ***
# For more information please refer to files in the COPYRIGHT directory ***
#
# This test script that runs various tests on the ICAT quota
# functions. There are two modes that test different functionality
# (with some overlap): with and without the 'enabled' on the command
# line.  When running 'quotaTest.pl enabled', you must first update
# the acRescQuotaPolicy in core.re to enable quota enforcement.  This
# mode will exercise different SQL and confirm that, in this mode,
# storing files that would exceed that quota is prevented.
#
# Before running this, the icommands need to be in the path, iinit
# done for iadmin account, and the following environment variables
# set (RT is the iRODS top diretory, for example: /tbox/IRODS_BUILD/iRODS):
# export LD_LIBRARY_PATH=$RT/../pgsql/lib
# export irodsConfigDir=$RT/server/config
# PATH=$PATH:$RT/server/test/bin
#
# It will print "Success" at the end and have a 0 exit code, if
# everything works properly.
#
# Some of the icommands print error messages in various cases, but
# that's OK, sometimes they should or sometimes it doesn't matter.
# The important checks are being made.  If "Success" is printed at the
# end, then everything was OK.
#

($input1)=@ARGV;
if ($input1 eq "enabled") {
    $QuotaEnforcementEnabled=1;
    printf("Testing with Quota Enforcement enabled\n");
}

# =-=-=-=-=-=-=-
# ensure that the test bin dir is in the
# path env variable
use Cwd 'abs_path';
use File::Basename;

$script_path = dirname( abs_path( $0 ) );
#print "base path name [$script_path]\n";

$env_path = $ENV{PATH};
#print "path [$env_path]\n";

if( $env_path =~ /$script_path/ ) {
} else {
    $new_path = $script_path . ":" . $env_path;
    #print "new path [$new_path]\n";
    $ENV{PATH} = $new_path;
}

$F1="TestFile1";
$F2="TestFile2";
$F3="TestFile3";

# Quota Users:
$QU1="qu1";
$QU2="qu2";
$QU3="qu3";

$Resc="demoResc";
$Resc2="Resc2";

$QG1="quotaGroup1";

# run a command
# if option is 0 (normal), check the exit code and fail if non-0
# if 1, don't care
# if 2, should get a non-zero result, exit if not
sub runCmd {
    my($option, $cmd, $stdoutVal) = @_;
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


# get our zone name
runCmd(0, "ienv | grep irods_zone_name | tail -1");
chomp($cmdStdout);
$ix = index($cmdStdout,"-");
$myZone=substr($cmdStdout, $ix+1);
$myZone =~ s/^\s+//;

#`ls -l > $F1`;
#$F1a = "$F1" . "a";
#$F1b = "$F1" . "b";

$tmpPwFile="/tmp/testPwFile.5678956";

$DIR = "/$myZone/home/$QU1";

sub calcUsage {
    runCmd(0, "iadmin cu");
    printf("sleeping briefly\n");
    sleep(1);
}

sub runUserTests {
    my($TResc, $TOpt, $TType) = @_;
    printf("runUserTests with $TResc, $TOpt, $TType\n");
    runCmd(1, "iadmin suq $QU1 $TOpt 0"); # unset quota for this user, if any
    calcUsage();
    runCmd(0, "test_chl checkquota $QU1 $Resc 0 0");
    runCmd(0, "iadmin suq $QU1 $TOpt 100");
#   runCmd(0, "test_chl checkquota $QU1 $Resc 0 $TType"); # before cu
    calcUsage();
    runCmd(0, "test_chl checkquota $QU1 $Resc m100 $TType"); # m100 is -100

    $ENV{'IRODS_AUTHENTICATION_FILE'}=$tmpPwFile;
    $ENV{'IRODS_USER_NAME'}=$QU1;
    runCmd(0, "echo 123 | iput $F1 $DIR");
    delete $ENV{'IRODS_USER_NAME'};
    delete $ENV{'IRODS_AUTHENTICATION_FILE'};

    runCmd(0, "test_chl checkquota $QU1 $Resc m100 $TType");
    calcUsage();
    runCmd(0, "test_chl checkquota $QU1 $Resc m50 $TType");

    if ($QuotaEnforcementEnabled==1) {
	$ENV{'IRODS_AUTHENTICATION_FILE'}=$tmpPwFile;
	$ENV{'IRODS_USER_NAME'}=$QU1;
	runCmd(2, "echo 123 | iput $F2 $DIR"); # should fail, would go over quota
	delete $ENV{'IRODS_USER_NAME'};
	delete $ENV{'IRODS_AUTHENTICATION_FILE'};
    }

    runCmd(0, "iadmin suq $QU1 $TOpt 40");
    calcUsage();
    runCmd(0, "test_chl checkquota $QU1 $Resc 10 $TType");

    runCmd(0, "iadmin suq $QU1 $TOpt 40000000000000");
    calcUsage();
    runCmd(0, "test_chl checkquota $QU1 $Resc m39999999999950 $TType");

    $ENV{'IRODS_AUTHENTICATION_FILE'}=$tmpPwFile;
    $ENV{'IRODS_USER_NAME'}=$QU1;
    runCmd(0, "echo 123 | irm -f $DIR/$F1");
    runCmd(0, "echo 123 | irmtrash");
    delete $ENV{'IRODS_USER_NAME'};
    delete $ENV{'IRODS_AUTHENTICATION_FILE'};

    calcUsage();
    runCmd(0, "test_chl checkquota $QU1 $Resc m40000000000000 $TType");
    runCmd(0, "iadmin suq $QU1 $TOpt 40");
    calcUsage();
    runCmd(0, "test_chl checkquota $QU1 $Resc m40 $TType");

    runCmd(0, "iadmin suq $QU1 $TOpt 0"); # unset quota for this user
}

sub runGroupTests {
    my($TResc, $TOpt, $TType, $StoreUser, $TestUser) = @_;
    printf("runGroupTests with $TResc, $TOpt, $TType, $StoreUser, $TestUser\n");
    runCmd(1, "iadmin sgq $QG1 $TOpt 0"); # unset quota for this group, if any
    calcUsage();
    runCmd(0, "test_chl checkquota $QG1 $Resc 0 0");
    runCmd(0, "iadmin sgq $QG1 $TOpt 100");
#   runCmd(0, "test_chl checkquota $TestUser $Resc 0 $TType"); # before cu
    calcUsage();
    runCmd(0, "test_chl checkquota $TestUser $Resc m100 $TType"); #m100 is -100

    $ENV{'IRODS_AUTHENTICATION_FILE'}=$tmpPwFile;
    $ENV{'IRODS_USER_NAME'}=$StoreUser;
    $ST_DIR = "/$myZone/home/$StoreUser";
    runCmd(0, "echo 123 | iput $F1 $ST_DIR");
    delete $ENV{'IRODS_USER_NAME'};
    delete $ENV{'IRODS_AUTHENTICATION_FILE'};

    runCmd(0, "test_chl checkquota $TestUser $Resc m100 $TType");
    calcUsage();
    runCmd(0, "test_chl checkquota $TestUser $Resc m50 $TType");

    if ($QuotaEnforcementEnabled==1) {
	$ENV{'IRODS_AUTHENTICATION_FILE'}=$tmpPwFile;
	$ENV{'IRODS_USER_NAME'}=$StoreUser;
	$ST_DIR = "/$myZone/home/$StoreUser";
	runCmd(2, "echo 123 | iput $F2 $ST_DIR");  # should fail, over quota
	delete $ENV{'IRODS_USER_NAME'};
	delete $ENV{'IRODS_AUTHENTICATION_FILE'};
    }

    runCmd(0, "iadmin sgq $QG1 $TOpt 40");
    calcUsage();
    runCmd(0, "test_chl checkquota $TestUser $Resc 10 $TType");

    runCmd(0, "iadmin sgq $QG1 $TOpt 40000000000000");
    calcUsage();
    runCmd(0, "test_chl checkquota $TestUser $Resc m39999999999950 $TType");

    $ENV{'IRODS_AUTHENTICATION_FILE'}=$tmpPwFile;
    $ENV{'IRODS_USER_NAME'}=$StoreUser;
    $ST_DIR = "/$myZone/home/$StoreUser";
    runCmd(0, "echo 123 | irm -f $ST_DIR/$F1");
    runCmd(0, "echo 123 | irmtrash");
    delete $ENV{'IRODS_USER_NAME'};
    delete $ENV{'IRODS_AUTHENTICATION_FILE'};

    calcUsage();
    runCmd(0, "test_chl checkquota $TestUser $Resc m40000000000000 $TType");
    runCmd(0, "iadmin sgq $QG1 $TOpt 40");
    calcUsage();
    runCmd(0, "test_chl checkquota $TestUser $Resc m40 $TType");

    runCmd(0, "iadmin rfg $QG1 $TestUser");
    calcUsage();
    runCmd(0, "test_chl checkquota $TestUser $Resc 0 0");

    runCmd(0, "iadmin atg $QG1 $TestUser");

    runCmd(0, "iadmin rfg $QG1 $StoreUser");
    calcUsage();
    if ($StoreUser eq $TestUser) {
	runCmd(0, "test_chl checkquota $TestUser $Resc 0 0");
    }
    else {
        runCmd(0, "test_chl checkquota $TestUser $Resc m40 $TType");
    }
    runCmd(0, "iadmin atg $QG1 $StoreUser");


    runCmd(0, "iadmin sgq $QG1 $TOpt 0"); # unset quota for this user
}

sub  runGroupTestsWithQU3 {
    my($TResc, $TOpt, $TType, $StoreUser, $TestUser) = @_;
    printf("runGroupTestsWithQU3 with $TResc, $TOpt, $TType, $StoreUser, $TestUser\n");
    runCmd(1, "iadmin sgq $QG1 $TOpt 0"); # unset quota for this group, if any
    calcUsage();
    runCmd(0, "test_chl checkquota $QG1 $Resc 0 0");
    runCmd(0, "iadmin sgq $QG1 $TOpt 100");
#   runCmd(0, "test_chl checkquota $TestUser $Resc 0 $TType"); # before cu
    calcUsage();
    runCmd(0, "test_chl checkquota $TestUser $Resc 75 $TType");

    $ENV{'IRODS_AUTHENTICATION_FILE'}=$tmpPwFile;
    $ENV{'IRODS_USER_NAME'}=$StoreUser;
    $ST_DIR = "/$myZone/home/$StoreUser";
    runCmd(0, "echo 123 | iput $F1 $ST_DIR");
    delete $ENV{'IRODS_USER_NAME'};
    delete $ENV{'IRODS_AUTHENTICATION_FILE'};

    runCmd(0, "test_chl checkquota $TestUser $Resc 75 $TType");
    calcUsage();
    runCmd(0, "test_chl checkquota $TestUser $Resc 125 $TType");

    runCmd(0, "iadmin sgq $QG1 $TOpt 40");
    calcUsage();
    runCmd(0, "test_chl checkquota $TestUser $Resc 185 $TType");

    runCmd(0, "iadmin sgq $QG1 $TOpt 40000000000000");
    calcUsage();
    runCmd(0, "test_chl checkquota $TestUser $Resc m39999999999775 $TType");

    $ENV{'IRODS_AUTHENTICATION_FILE'}=$tmpPwFile;
    $ENV{'IRODS_USER_NAME'}=$StoreUser;
    $ST_DIR = "/$myZone/home/$StoreUser";
    runCmd(0, "echo 123 | irm -f $ST_DIR/$F1");
    runCmd(0, "echo 123 | irmtrash");
    delete $ENV{'IRODS_USER_NAME'};
    delete $ENV{'IRODS_AUTHENTICATION_FILE'};

    calcUsage();
    runCmd(0, "test_chl checkquota $TestUser $Resc m39999999999825 $TType");

    runCmd(0, "iadmin sgq $QG1 $TOpt 40");
    calcUsage();
    runCmd(0, "test_chl checkquota $TestUser $Resc 135 $TType");

    runCmd(0, "iadmin rfg $QG1 $TestUser");
    calcUsage();
    runCmd(0, "test_chl checkquota $TestUser $Resc 0 0");

    runCmd(0, "iadmin atg $QG1 $TestUser");

    runCmd(0, "iadmin rfg $QG1 $StoreUser");
    calcUsage();
    if ($StoreUser eq $TestUser) {
	runCmd(0, "test_chl checkquota $TestUser $Resc 0 0");
    }
    else {
        runCmd(0, "test_chl checkquota $TestUser $Resc 135 $TType");
    }
    runCmd(0, "iadmin atg $QG1 $StoreUser");


    runCmd(0, "iadmin sgq $QG1 $TOpt 0"); # unset quota for this user
}

$ENV{'IRODS_AUTHENTICATION_FILE'}=$tmpPwFile;
$ENV{'IRODS_USER_NAME'}=$QU1;
runCmd(1, "echo 123 | irm -f $DIR/$F1");
runCmd(1, "echo 123 | irmtrash");
delete $ENV{'IRODS_USER_NAME'};
delete $ENV{'IRODS_AUTHENTICATION_FILE'};

$ENV{'IRODS_AUTHENTICATION_FILE'}=$tmpPwFile;
$ENV{'IRODS_USER_NAME'}=$QU2;
$ST_DIR = "/$myZone/home/$QU2";
runCmd(1, "echo 123 | irm -f $ST_DIR/$F1");
runCmd(1, "echo 123 | irmtrash");
delete $ENV{'IRODS_USER_NAME'};
delete $ENV{'IRODS_AUTHENTICATION_FILE'};

runCmd(1, "iadmin rmuser $QU1");
runCmd(0, "iadmin mkuser $QU1 rodsuser");
runCmd(0, "iadmin moduser $QU1 password 123");

runCmd(1, "iadmin rmuser $QU2");
runCmd(0, "iadmin mkuser $QU2 rodsuser");
runCmd(0, "iadmin moduser $QU2 password 123");

runCmd(0, "iadmin cu");               # make sure it's initialized

unlink($F1);
runCmd(0, "head -c50 /etc/passwd > $F1");  # a 50 byte file
unlink($F2);
runCmd(0, "head -c175 /etc/passwd > $F2");  # a 175 byte file

runUserTests("$Resc", "$Resc", "1");
runUserTests("$Resc", "total", "2");

runCmd(1, "iadmin rmgroup $QG1");
runCmd(0, "iadmin mkgroup $QG1");
runCmd(0, "iadmin atg $QG1 $QU1");
runCmd(0, "iadmin atg $QG1 $QU2");

runGroupTests("$Resc", "$Resc", "1", "$QU1", "$QU1");
runGroupTests("$Resc", "total", "2", "$QU1", "$QU1");
runGroupTests("$Resc", "$Resc", "1", "$QU2", "$QU1");
runGroupTests("$Resc", "total", "2", "$QU2", "$QU1");

# Remove $QU3's file, if any
$ENV{'IRODS_AUTHENTICATION_FILE'}=$tmpPwFile;
$ENV{'IRODS_USER_NAME'}=$QU3;
$ST_DIR = "/$myZone/home/$QU3";
runCmd(1, "echo 123 | irm -f $ST_DIR/$F2");
runCmd(1, "echo 123 | irmtrash");
delete $ENV{'IRODS_USER_NAME'};
delete $ENV{'IRODS_AUTHENTICATION_FILE'};

# Create user $QU3
runCmd(1, "iadmin rmuser $QU3");
runCmd(0, "iadmin mkuser $QU3 rodsuser");
runCmd(0, "iadmin moduser $QU3 password 123");

if ($QuotaEnforcementEnabled!=1) {
# Store a file as $QU3
    $ENV{'IRODS_AUTHENTICATION_FILE'}=$tmpPwFile;
    $ENV{'IRODS_USER_NAME'}=$QU3;
    $ST_DIR = "/$myZone/home/$QU3";
    runCmd(0, "echo 123 | iput $F2 $ST_DIR");
    delete $ENV{'IRODS_USER_NAME'};
    delete $ENV{'IRODS_AUTHENTICATION_FILE'};

# Add QU3 to the group
    runCmd(0, "iadmin atg $QG1 $QU3");

# Verify that the quota usage now includes QU3's storage
    runGroupTestsWithQU3("$Resc", "$Resc", "1", "$QU1", "$QU1");
    runGroupTestsWithQU3("$Resc", "total", "2", "$QU1", "$QU1");
    runGroupTestsWithQU3("$Resc", "$Resc", "1", "$QU2", "$QU1");
    runGroupTestsWithQU3("$Resc", "total", "2", "$QU2", "$QU1");

# Remove $QU3's file
    $ENV{'IRODS_AUTHENTICATION_FILE'}=$tmpPwFile;
    $ENV{'IRODS_USER_NAME'}=$QU3;
    $ST_DIR = "/$myZone/home/$QU3";
    runCmd(1, "echo 123 | irm -f $ST_DIR/$F2");
    runCmd(1, "echo 123 | irmtrash");
    delete $ENV{'IRODS_USER_NAME'};
    delete $ENV{'IRODS_AUTHENTICATION_FILE'};
}


# clean up
runCmd(0, "iadmin rmgroup $QG1");
runCmd(0, "iadmin rmuser $QU1");
runCmd(0, "iadmin rmuser $QU2");
runCmd(0, "iadmin rmuser $QU3");

printf("Success\n");
