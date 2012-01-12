#!/usr/bin/perl
#
# Copyright (c), The Regents of the University of California            ***
# For more information please refer to files in the COPYRIGHT directory ***
#
# This is a test script that runs various tests on the ICAT quota
# functions.
#
# It will print "Success" at the end and have a 0 exit code, if all
# everything works properly.
#
# Some of the icommands print error messages in various cases, but
# that's OK, sometimes they should or sometimes it doesn't matter.
# The important checks are being made.  If "Success" is printed at the
# end, then everything was OK.
#

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
runCmd(0, "ienv | grep irodsZone | tail -1");
chomp($cmdStdout);
$ix = index($cmdStdout,"=");
$myZone=substr($cmdStdout, $ix+1);

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

    $ENV{'irodsAuthFileName'}=$tmpPwFile;
    $ENV{'irodsUserName'}=$QU1;
    runCmd(0, "echo 123 | iput $F1 $DIR");
    delete $ENV{'irodsUserName'};
    delete $ENV{'irodsAuthFileName'};

    runCmd(0, "test_chl checkquota $QU1 $Resc m100 $TType");
    calcUsage();
    runCmd(0, "test_chl checkquota $QU1 $Resc m50 $TType");

    runCmd(0, "iadmin suq $QU1 $TOpt 40");
    calcUsage();
    runCmd(0, "test_chl checkquota $QU1 $Resc 10 $TType");

    runCmd(0, "iadmin suq $QU1 $TOpt 40000000000000");
    calcUsage();
    runCmd(0, "test_chl checkquota $QU1 $Resc m39999999999950 $TType");

    $ENV{'irodsAuthFileName'}=$tmpPwFile;
    $ENV{'irodsUserName'}=$QU1;
    runCmd(0, "echo 123 | irm -f $DIR/$F1");
    runCmd(0, "echo 123 | irmtrash");
    delete $ENV{'irodsUserName'};
    delete $ENV{'irodsAuthFileName'};

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

    $ENV{'irodsAuthFileName'}=$tmpPwFile;
    $ENV{'irodsUserName'}=$StoreUser;
    $ST_DIR = "/$myZone/home/$StoreUser";
    runCmd(0, "echo 123 | iput $F1 $ST_DIR");
    delete $ENV{'irodsUserName'};
    delete $ENV{'irodsAuthFileName'};

    runCmd(0, "test_chl checkquota $TestUser $Resc m100 $TType");
    calcUsage();
    runCmd(0, "test_chl checkquota $TestUser $Resc m50 $TType");

    runCmd(0, "iadmin sgq $QG1 $TOpt 40");
    calcUsage();
    runCmd(0, "test_chl checkquota $TestUser $Resc 10 $TType");

    runCmd(0, "iadmin sgq $QG1 $TOpt 40000000000000");
    calcUsage();
    runCmd(0, "test_chl checkquota $TestUser $Resc m39999999999950 $TType");

    $ENV{'irodsAuthFileName'}=$tmpPwFile;
    $ENV{'irodsUserName'}=$StoreUser;
    $ST_DIR = "/$myZone/home/$StoreUser";
    runCmd(0, "echo 123 | irm -f $ST_DIR/$F1");
    runCmd(0, "echo 123 | irmtrash");
    delete $ENV{'irodsUserName'};
    delete $ENV{'irodsAuthFileName'};

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

    $ENV{'irodsAuthFileName'}=$tmpPwFile;
    $ENV{'irodsUserName'}=$StoreUser;
    $ST_DIR = "/$myZone/home/$StoreUser";
    runCmd(0, "echo 123 | iput $F1 $ST_DIR");
    delete $ENV{'irodsUserName'};
    delete $ENV{'irodsAuthFileName'};

    runCmd(0, "test_chl checkquota $TestUser $Resc 75 $TType");
    calcUsage();
    runCmd(0, "test_chl checkquota $TestUser $Resc 125 $TType");

    runCmd(0, "iadmin sgq $QG1 $TOpt 40");
    calcUsage();
    runCmd(0, "test_chl checkquota $TestUser $Resc 185 $TType");

    runCmd(0, "iadmin sgq $QG1 $TOpt 40000000000000");
    calcUsage();
    runCmd(0, "test_chl checkquota $TestUser $Resc m39999999999775 $TType");

    $ENV{'irodsAuthFileName'}=$tmpPwFile;
    $ENV{'irodsUserName'}=$StoreUser;
    $ST_DIR = "/$myZone/home/$StoreUser";
    runCmd(0, "echo 123 | irm -f $ST_DIR/$F1");
    runCmd(0, "echo 123 | irmtrash");
    delete $ENV{'irodsUserName'};
    delete $ENV{'irodsAuthFileName'};

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

$ENV{'irodsAuthFileName'}=$tmpPwFile;
$ENV{'irodsUserName'}=$QU1;
runCmd(1, "echo 123 | irm -f $DIR/$F1");
runCmd(1, "echo 123 | irmtrash");
delete $ENV{'irodsUserName'};
delete $ENV{'irodsAuthFileName'};

$ENV{'irodsAuthFileName'}=$tmpPwFile;
$ENV{'irodsUserName'}=$QU2;
$ST_DIR = "/$myZone/home/$QU2";
runCmd(1, "echo 123 | irm -f $ST_DIR/$F1");
runCmd(1, "echo 123 | irmtrash");
delete $ENV{'irodsUserName'};
delete $ENV{'irodsAuthFileName'};

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
$ENV{'irodsAuthFileName'}=$tmpPwFile;
$ENV{'irodsUserName'}=$QU3;
$ST_DIR = "/$myZone/home/$QU3";
runCmd(1, "echo 123 | irm -f $ST_DIR/$F2");
runCmd(1, "echo 123 | irmtrash");
delete $ENV{'irodsUserName'};
delete $ENV{'irodsAuthFileName'};

# Create user $QU3
runCmd(1, "iadmin rmuser $QU3");
runCmd(0, "iadmin mkuser $QU3 rodsuser");
runCmd(0, "iadmin moduser $QU3 password 123");

# Store a file as $QU3
$ENV{'irodsAuthFileName'}=$tmpPwFile;
$ENV{'irodsUserName'}=$QU3;
$ST_DIR = "/$myZone/home/$QU3";
runCmd(0, "echo 123 | iput $F2 $ST_DIR");
delete $ENV{'irodsUserName'};
delete $ENV{'irodsAuthFileName'};

# Add QU3 to the group
runCmd(0, "iadmin atg $QG1 $QU3");

# Verify that the quota usage now includes QU3's storage
runGroupTestsWithQU3("$Resc", "$Resc", "1", "$QU1", "$QU1");
runGroupTestsWithQU3("$Resc", "total", "2", "$QU1", "$QU1");
runGroupTestsWithQU3("$Resc", "$Resc", "1", "$QU2", "$QU1");
runGroupTestsWithQU3("$Resc", "total", "2", "$QU2", "$QU1");

# Remove $QU3's file
$ENV{'irodsAuthFileName'}=$tmpPwFile;
$ENV{'irodsUserName'}=$QU3;
$ST_DIR = "/$myZone/home/$QU3";
runCmd(1, "echo 123 | irm -f $ST_DIR/$F2");
runCmd(1, "echo 123 | irmtrash");
delete $ENV{'irodsUserName'};
delete $ENV{'irodsAuthFileName'};


# clean up
runCmd(0, "iadmin rmgroup $QG1");
runCmd(0, "iadmin rmuser $QU1");
runCmd(0, "iadmin rmuser $QU2");
runCmd(0, "iadmin rmuser $QU3");

printf("Success\n");
