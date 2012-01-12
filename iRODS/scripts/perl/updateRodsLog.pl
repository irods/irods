#!/usr/bin/perl
#
# Copyright (c), The Regents of the University of California            ***
# For more information please refer to files in the COPYRIGHT directory ***
#
# This perl script replaces some tables in rodsLog.c after the
# rodsErrorTable.h file is updated.  It is run by gmake.
#

$Defs=`grep '#define' ../lib/core/include/rodsErrorTable.h | grep -v RODS_ERROR_TABLE_H | grep -v '* #' | awk '{ printf("%s ", \$2) }'`;

$_=$Defs;
@DO_LIST=split(" ",$_);

$InputFile="../lib/core/src/rodsLog.c";
$OutputFile =$InputFile . ".tmp.128923";
$InputFileOrig = $InputFile . ".orig";
open(FileIn, $InputFile) || die("Can't open input file " . "$InputFile");
open(FileOut, ">".$OutputFile) || die("Can't open output file "."$OutputFile");
$step=0; # 0 copying the first part of src, 1 inserting new code,
         # 2 skipping old generated code, 3 copying to the end
$testLine="BEGIN generated code";
while($line = <FileIn>) {
    $lineNum++;
    $where = index( $line, $testLine);
    if ($where >= 0) {
	$step++;
	if ($step eq "1") {
	    printf FileOut $line;
	    printf FileOut "   int irodsErrors[]={ \n";
	    $count=0;
	    foreach $DO_LIST (@DO_LIST) {
		printf FileOut "    " . $DO_LIST . ", \n" ;
		$count++;
	    }
	    printf FileOut "};\n   char *irodsErrorNames[]={ \n";
	    foreach $DO_LIST (@DO_LIST) {
		printf FileOut "    \"" . $DO_LIST . "\", \n" 
	    }
	    printf FileOut "};\n";
	    printf FileOut "int irodsErrorCount= " . $count . ";\n";
	    $step++;
	    $testLine="END generated code";
	}
    }
    if ($step != 2) { print FileOut $line; }
}
close(FileIn);
close(FileOut);

if ( -e $InputFileOrig ) {
    unlink($InputFile);
}
else {
    rename($InputFile, $InputFileOrig);
}
rename($OutputFile, $InputFile);
