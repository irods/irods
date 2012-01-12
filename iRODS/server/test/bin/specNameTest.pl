#!/usr/bin/perl
#
# Copyright (c), The Regents of the University of California            ***
# For more information please refer to files in the COPYRIGHT directory ***
#
# This is a test script that runs creates and iput's files with
# special characters in the names, including & @ ' " - % and _
#
# It assumes iinit has been run, that the icommands are in the
# path.
#
# It will print "Success" at the end and have a 0 exit code, if
# everything works properly.
#

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

sub testOneFile {
    my($fileName, $option) = @_;
    unlink($fileName);
    `ls > "$fileName"`;
    if ($option == 0) {
	runCmd(0, "iput $fileName");
	runCmd(0, "ils -l $fileName");
	runCmd(0, "irm -f $fileName");
    }
    if ($option == 1) {
#       quote the name, i.e. put " " around the name
	$quotedFileName = "\"" . $fileName . "\"";
	runCmd(0, "iput $quotedFileName");
	runCmd(0, "ils -l $quotedFileName");
	runCmd(0, "irm -f $quotedFileName");
    }
    unlink($fileName);
    runCmd(1, "rm -f " . "\"" . $fileName . "\"");
}

sub testWithChar {
# option 2 is to quote the character with \
# option 3 is to use "./" when the char is at the beginning
    my($testChar, $option) = @_;
    if ($option == 2) {
	$testChar2 = "\\" . $testChar;
	$testChar = $testChar2;
	$option=0;
    }
    $prefix="";
    if ($option == 3) {
	$prefix="./";
	$option=0;
    }
    testOneFile($prefix . $testChar . "testFile", $option);
    testOneFile($prefix . $testChar . "Quoteinfilename.txt", $option);
    testOneFile("te" . $testChar . "stFile", $option);
    testOneFile("testFile" . $testChar, $option);
    testOneFile($prefix . $testChar . "test" . $testChar . "File", $option);
    testOneFile("test" . $testChar . "Fi" . $testChar . "le",
		$option);
    testOneFile($prefix . $testChar . "test" . $testChar . "Fi" . 
		$testChar . "le", $option);
    testOneFile($prefix . $testChar . "test" . $testChar . "Fi" . $testChar .
		"le" . $testChar, $option);
}


testWithChar("@", 0);
testWithChar("_", 0);
testWithChar("&", 1);
testWithChar("%", 0);
testWithChar("\"", 2);
testWithChar("'", 1);
testWithChar("-", 3);

printf("Success\n");
