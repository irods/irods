#!/usr/bin/perl
#
# Copyright (c), The Regents of the University of California            ***
# For more information please refer to files in the COPYRIGHT directory ***
#
# This is a test script to put a bit of a load onto the system and
# time how long operations take.  Currently, we're looking at how long
# it takes to vacuum the postgres DB after storing and retrieving many
# files.  On Zuri (a linux desktop), I've gotten these results:
# 40,000 files:
# vacuum 21 seconds (due to other activity before) (with 10 sec sleep)
# iput   10 minutes 9 secs
# ils     7 seconds
# irm    27 minutes 7 seconds
# vacuum 40 seconds (with 10 second sleep)
#
# A better test on how long vacuumdb takes would be to vary the load
# on the database more, and to time it when lots of db entries still
# exist.

require "ctime.pl";

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

sub printDeltaTime() {
    $now=time();
    $Date= &ctime($now);
    $Date=substr($Date,11,8);
    $secs = $now-$start; 
    if ($secs < 61) {
	printf("%s Done. Time for this step = %d seconds.\n", $Date, $secs); 
    }
    else {
	$minutes = int($secs/60);
	$minSecs = $minutes * 60;
	$rsecs = $secs - $minSecs;
	printf(
	"%s Done. Time for this step = %d minutes %d seconds (%d seconds).\n", 
	       $Date, $minutes, $rsecs, $secs); 

    }
    $start=time();
}


# make many (1000*j's limit) very small files
`mkdir test`;
chdir "test";
$j=0;
while ($j < 40) {
    printf "making 1000 files in d$j\n";
    mkfiles(1000, "d$j");
    $j=$j + 1;
}
chdir "..";

$start=time();

# irm
runCmd(1, "irm -r test");
print $cmdStdout;
printDeltaTime();

sleep(1);
$start=time();

# vacuum
chdir "../..";  # to the top dir; needed for install.pl
runCmd(0, "install.pl vacuum");
print $cmdStdout;
print "\n";
printDeltaTime();
chdir "server/test";  # back to the server/test directory

# iput
runCmd(0, "iput -r test");
print $cmdStdout;
printDeltaTime();

# ils
runCmd(1, "ils -r test | wc");
print $cmdStdout;
printDeltaTime();

# irm
runCmd(0, "irm -r test");
print $cmdStdout;
printDeltaTime();

# vacuum
chdir "../..";  # to the top dir; needed for install.pl
runCmd(0, "install.pl vacuum");
printDeltaTime();
