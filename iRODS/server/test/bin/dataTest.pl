#!/usr/bin/perl
#
# Copyright (c), The Regents of the University of California            ***
# For more information please refer to files in the COPYRIGHT directory ***
#
# This is a test script that runs various icommands to test data
# handling functions.  It can be run with the 3 resources all local or
# in an environment where the resources are on other hosts to test
# additional functionality.  Currently, this is very incomplete.
#
# It assumes iinit has been run, that the icommands are in the
# path, and that the three resources exist.
#
# It will print "Success" at the end and have a 0 exit code, if 
# everything works properly.
#
#

$B1="BigFile";
$B2="BigFile.2";

$D1="directory1";
$D2="DIR2";

$Resc1="demoResc";
$Resc2="demoResc2";
$Resc3="demoResc3";

# run a command
# if option is 0 (normal), check the exit code and fail if non-0
# if 1, don't care (the command may or may not succeed)
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


# Make a directory with input-arg empty files
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

# Makes a 200,000,000 byte file if it doesn't already exist.
sub mkbigfile {
    if (-e $B1) {
	printf("file $B1 exists, skipping creating it\n");
	return;
    }
    unlink($B1);
    unlink($B2);
    `echo "0123456789012345678901234567890123456789012345678" >> $B1`;
    for ($i=0;$i<3;$i++) {
	`cat $B1 $B1 $B1 $B1 $B1 $B1 $B1 $B1 $B1 $B1 >> $B2`;
	unlink($B1);
	`cat $B2 $B2 $B2 $B2 $B2 $B2 $B2 $B2 $B2 $B2 >> $B1`;
	unlink($B2);
    }
    `cat $B1 $B1 $B1 $B1 >> $B2`;
    unlink($B1);
    rename($B2, $B1);
}

# iput, irepl, and itrim using 2 resources, first one large file
# and then a set of small files.
sub dataTestTwoResc {
    my($myR1, $myR2) = @_;
    runCmd(1, "irm -rf $D1");
    runCmd(0, "imkdir $D1");
    runCmd(0, "icd $D1");
    runCmd(0, "iput -P -R $myR1 $B1");
    runCmd(0, "itrim -r -N 1 -n 0 $B1");
    runCmd(0, "irepl -r -R $myR2 $B1");
    runCmd(0, "itrim -r --dryrun -N 1 -n 0 $B1");
    runCmd(0, "itrim -r -N 1 -n 0 $B1");
    runCmd(0, "icd");
    runCmd(0, "ils -l $D1");
    printf("ils stdout:\n $cmdStdout");
    runCmd(0, "irm -rf $D1");

    runCmd(1, "irm -rf $D2");
    runCmd(0, "imkdir $D2");
    runCmd(0, "iput -R $myR1 -b -r $D2");
    runCmd(0, "irepl -r -R $myR2 $D2");
    runCmd(0, "itrim -r -N 1 -n 0 $D2");
    runCmd(0, "irm -rf $D2");
    runCmd(0, "iput -r $D2");
    runCmd(0, "irepl -r -R $myR2 $D2");
    runCmd(0, "itrim -r -N 1 -n 1 $D2");
    runCmd(0, "irm -rf $D2");
}

# iput, irepl, and itrim using 3 resources, first one large file and
# then a set of small files.  Stores in R1, then replicates to R2,
# then replicates from R2 to R3, then trims to R3, and replicates back
# to R1.
sub dataTestThreeResc {
    my($myR1, $myR2, $myR3) = @_;
    runCmd(1, "irm -rf $D1");
    runCmd(0, "imkdir $D1");
    runCmd(0, "icd $D1");
    runCmd(0, "iput -P -R $myR1 $B1");
    runCmd(0, "irepl -R $myR2 $B1");
    runCmd(0, "irepl -S $myR2 -R $myR3 $B1");
    runCmd(0, "irepl -S $myR2 -R $myR3 $B1");
    runCmd(0, "itrim -N 2 -n 0 $B1");
    runCmd(0, "itrim -N 1 -n 1 $B1");
    runCmd(0, "irepl -S $myR3 -R $myR1 $B1");
    runCmd(0, "icd");
    runCmd(0, "ils -l $D1");
    printf("ils stdout:\n $cmdStdout");
    runCmd(0, "irm -rf $D1");

    runCmd(1, "irm -rf $D2");
    runCmd(0, "iput -R $myR1 -b -r $D2");
    runCmd(0, "irepl -r -R $myR2 $D2");
    runCmd(0, "irepl -r -R $myR3 $D2");
    runCmd(0, "itrim -r -N 2 -n 0 $D2");
    runCmd(0, "itrim -r -N 1 -n 1 $D2");
    runCmd(0, "irepl -r -S $myR3 -R $myR1 $D2");
    runCmd(0, "irepl -r -R $myR2 $D2");
    runCmd(0, "itrim -r -N 1 -n 1 $D2");
    runCmd(0, "irm -rf $D2");
}


# get our zone name
runCmd(0, "ienv | grep irodsZone | tail -1");
chomp($cmdStdout);
$ix = index($cmdStdout,"=");
$myZone=substr($cmdStdout, $ix+1);


# Make sure we are in the irods home dir
runCmd(0, "icd");

# Clean up possible left over sub-process env (pwd) files, if any,
# to avoid collision if process number matches
runCmd(1, "rm -rf ~/.irods/.irodsEnv.*");

mkbigfile();
mkfiles(100, "$D2");
dataTestTwoResc ($Resc1, $Resc2);
dataTestTwoResc ($Resc2, $Resc1);
dataTestThreeResc ($Resc1, $Resc2, $Resc3);
printf("Success\n");
