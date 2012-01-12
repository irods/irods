#!/usr/bin/perl

# This script is part of a sequence of tests to verify ICAT
# functionality.  You need to first enable the sqlLOG logging in the
# source files and then run testiCommands.pl and (once written) other
# test scripts.

# What this script does is scan the source files and server log file
# for the sqlLOG items, reporting how many times each of the sqlLOG
# code markers have been executed (these make each of the unique SQL
# forms).  With the right tests, I'd like to see each of the SQL types
# run at least once to confirm that the SQL works.  This will be
# useful for the current bind-variable changes, when porting to a new
# DBMS, and anytime we make changes.

$onlyZero=1;
$srcFiles="../../icat/src/icatHighLevelRoutines.c ../../icat/src/icatGeneralQuery.c ../../icat/src/icatMidLevelRoutines.c ../../icat/src/icatGeneralUpdate.c";

if ($#ARGV < 0) {
# No filename, use default
    $fileName=`ls -t -C1 ../../log/rodsLog* | head -1`;
    chomp($fileName);
}
else {
    $fileName = $ARGV[0];
}

if (-e "moveTest.log") {
    $fileName = $fileName . " moveTest.log";
}

if (-e "icatMiscTest.log") {
    $fileName = $fileName . " icatMiscTest.log";
}

$theLines = `grep logSQL $srcFiles | grep " SQL "`;

$_=$theLines;
@lines=split("\n", $_);
$total=0;
$nonZero=0;
foreach $line (@lines) {
    chomp($line);
    $r = rindex($line, '"');
    $l = index($line, '"');
    $str=substr($line,$l+1,$r-$l-1);
    $count = `grep "$str" $fileName | wc`;
    if ($onlyZero) {
	if ($count==0) {printf("%d %s\n",$count, $str);}
    }
    else {
	printf("%d %s\n",$count, $str);
    }
    $total++;
    if ($count > 0) {
	$nonZero++;
    }
}
$zero = $total - $nonZero;
$percent = ($nonZero*100)/$total;
printf("Total = %d,  NonZero(tested)=%d,  Zero(not tested)=%d, %d% SQL forms tested\n", $total, $nonZero, $zero, 
       $percent);
if ($zero > 0) {
    die ("Some icat sql was not tested");
}
