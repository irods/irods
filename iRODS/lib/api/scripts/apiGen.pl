#!/usr/bin/perl
#
# Copyright (c), The Regents of the University of California            ***
# For more information please refer to files in the COPYRIGHT directory ***
#
# This script generates some of the .c and .h files 
# that implement the iRODS API client/server calls.
#
# See docs/DesignSpecs/apiFramework.
#
# For example, a new 'fileFoo' API.
# First set up a lib/api/include/fileFoo.h
# (copy fileCreate.h to fileFoo.h and change all
# the create strings to foo, preserving the case usage).
# Then, run apiGen.pl fileFoo
#
# It will:
#   update /lib/api/include/apiNumber.h
#   update /lib/api/include/apiTable.h
#   update /lib/api/include/apiPackTable.h
#   update /lib/api/include/apiHeaderAll.h
#   update /config/api.mk
#   generate ../server/src/api/rsFileFoo.c (empty tho)  and
#   generate ../lib/src/api/rcFileFoo.c 
#
# The generated rsFileFoo.c will have to be edited to add specific
# calls for the function, but the other files should be fine as
# updated or generated.
#
# The cvs add and commit commands that need to be run are displayed.
#
# You can safely rerun this script with the same input multiple times;
# it will avoid repeating the same updates in each file.  But it will
# show you the filenames of the updated files (cvs commands).
#
# Previous versions of this script attempted to generate rsFileFoo.c,
# but with the various types it is now too complicated to do that and
# they would still need to be hand edited anyway.  So we're better of
# just doing it by hand: checking and copying one of the existing
# ones, and tayloring it for the new call.
#
# This will have to be updated as other types of calls are developed.


#$debugFlag="1";

# Some relative paths:
$SSA="../../../server/api/src";
$CSA="../src";
$CLIA="../include";

$CONFIG_DIR="../../../config";



$prevUNAME="";

($InputArgument)=@ARGV;
if (!$InputArgument) {
    printf( "Missing API call name argument\n" );
    printf( "Usage is:  perl apiGen.pl name\n" );
    printf( "   'name' is the name of a new API call to be added into the\n" );
    printf( "   source and include files.\n" );
    die("Usage");
}
$ch=substr($InputArgument,$i,1);
$uc1=uc($ch) . substr($InputArgument,1); # the input str with 1st character uppercase


$incFile="$CLIA/$InputArgument" . ".h";
if (!-e $incFile) {
    printf("$incFile does not exist\n");
    die("Usage");
}
$cvsAdd = $incFile;
$cvsCommit=$incFile;

#
# Make the Upper case name string, for example FILE_CREATE
#
$j=length($InputArgument);
$UNAME="";
for ($i=0;$i<$j;$i++) {
    $ch=substr($InputArgument,$i,1);
    if ($ch ge "A" && $ch le "Z") {
	$UNAME=$UNAME . "_";
	$UNAME=$UNAME . $ch;
    }
    else {
	$UNAME=$UNAME . uc($ch);
    }
}

# The string for the #define of the number for the API call
$nStr=$UNAME . "_AN";


#
# Update apiNumber.h with this new one, inserting at the end of the section, incrementing
# the value by one from the previous last one in the section.
# The section is determined by checking the main include file for a description, so it
# needs to have a comment on the type of API call this will be.
#
$apiNumberH="$CLIA/apiNumber.h";
$apiN=`grep $nStr $apiNumberH`;
if ($apiN) {
    printf("WARNING, did not update $apiNumberH, $nStr already defined\n");

# prevUNAME is normally set in the else clause below, so in this case
# determine it differently.
    $S1=`grep -B4 $nStr $apiNumberH`;
    chomp($S1);
    $i1=rindex($S1,"#define");
    $i2=rindex($S1,"#define",$i1-1);
    $i3=index($S1, "_AN",$i2);
    $prevUNAME = substr($S1,$i2+8,$i3-$i2-5);
    dbPrintf("prevUNAME=%s\n",$prevUNAME);
}
else {
    $didOne=0;
    $typeApi=`grep -i 'Internal I/O' $incFile`;
    if ($typeApi) {
	insertNewDefInFile("$apiNumberH", "Internal I/O", $nStr);
	$isLowLevel="1";
	$didOne=1;
    }
    $typeApi=`grep -i 'Object File' $incFile`;
    if ($typeApi) {
	insertNewDefInFile("$apiNumberH", "Object File", $nStr);
	$didOne=1;
    }
    $typeApi=`grep -i 'Metadata' $incFile`;
    if ($typeApi) {
	insertNewDefInFile("$apiNumberH", "Metadata", $nStr);
	$didOne=1;
    }
    if ($didOne == 0) {
	printf("Cannot determine the type of API call; needs to be defined in $incFile\n");
	printf("It should be Internal I/O, Object File I/O, or Metadata\n");
	die("No type defined");
    }
    printf("Updated $apiNumberH\n");
}
$cvsCommit=$cvsCommit . " " . $apiNumberH;


#
# Update apiTable.h, 
# first checking some settings the .h file (which are used later too)
#
$testStrIn=$InputArgument . "Inp_PI";
$inStruct =`grep $testStrIn $incFile`;  # check if there are input packing instructions
dbPrintf("incFile=:%s:\n",$incFile);
dbPrintf("testStrIn=:%s:\n",$testStrIn);
dbPrintf("inStruct=:%s:\n",$inStruct);
$testStrOut=$InputArgument . "Out_PI";
$outStruct=`grep $testStrOut $incFile`;  # check if there are output packing instructions
$inBS=`grep -i 'input byte stream' $incFile`;   # see if there is an input byte stream
$outBS=`grep -i 'output byte stream' $incFile`; # see if there is an output byte stream
$inpBBuf=`grep InpBBuf $incFile`; # see if there is an input byte buffer struct
$outBBuf=`grep OutBBuf $incFile`; # see if there is an output byte buffer struct
$theString="    {$nStr, RODS_API_VERSION, ";
if ($isLowLevel) {
# Typical settings for low level
    $userAuth="REMOTE_USER_AUTH";
    $proxyAuth="REMOTE_PRIV_USER_AUTH";
}
else {
# Typical settings for high level
    $userAuth="REMOTE_USER_AUTH";
    $proxyAuth="REMOTE_USER_AUTH";
}
$theString=$theString . $userAuth . ", ";
$theString=$theString . $proxyAuth . ", \n      ";

if ($inStruct) {
    $theString=$theString . " \"$testStrIn\", ";
}
else {
    $theString=$theString . "NULL, ";
}
if ($inpBBuf) {
    $theString=$theString . "1, ";
}
else {
    $theString=$theString . "0, ";
}
if ($outStruct) {
    $theString=$theString . " \"$testStrOut\", ";
}
else {
    $theString=$theString . "NULL, ";
}
if ($outBBuf) {
    $theString=$theString . "1, ";
}
else {
    $theString=$theString . "0, ";
}
$theString = $theString . "(funcPtr) RS_$UNAME},\n";
dbPrintf("theString = %s\n",$theString);

dbPrintf("prevUNAME=:%s:\n",$prevUNAME);
if (!$prevUNAME){ die("preUNAME not determined"); }
$len=length($prevUNAME);
$RS_Str = substr($prevUNAME,0, length($prevUNAME)-3);
$RS_Str = "RS_" . $RS_Str; # prepend "RS_"
$theFile="$CLIA/apiTable.h";
dbPrintf("RS_Str=%s\n",$RS_Str);
$grp="grep $nStr $theFile";
dbPrintf("grep=%s\n",$grp);
$apiTab=`grep $nStr $theFile`;
dbPrintf("apiTab=%s\n",$apiTab);
if ($apiTab) {
    printf("WARNING, did not update $theFile, $nStr already defined\n");
}
else {
    insertInFile("after", $theFile, $RS_Str, $theString);
    printf("Updated $theFile\n");
}
$cvsCommit=$cvsCommit . " " . $theFile;


#
# Add lines to apiPackTable.h
#
$theFile="$CLIA/apiPackTable.h";
$didPackTab="0";
if ($inStruct) {
    $apiPackTab=`grep $testStrIn $theFile`;
    if ($apiPackTab) {
	printf("WARNING, did not update $theFile, $testStrIn already defined\n");
    } 
    else {
	$theString="        {\"$testStrIn\", $testStrIn},\n";
	insertInFile("before", $theFile, "PACK_TABLE_END_PI", $theString);
	$didPackTab="1";
    }
}
if ($outStruct) {
    $apiPackTab=`grep $testStrOut $theFile`;
    if ($apiPackTab) {
	printf("WARNING, did not update $theFile, $testStrOut already defined\n");
    }
    else {
	$theString="        {\"$testStrOut\", $testStrOut},\n";
	insertInFile("before", $theFile, "PACK_TABLE_END_PI", $theString);
	$didPackTab="1";
    }
}
if ($didPackTab eq "1") {
    printf("Updated $theFile\n");
}
if ($inStruct or $outStruct) {
    $cvsCommit=$cvsCommit . " " . $theFile;
}


#
# Add the include file to apiHeaderAll.h
#
$incFileShort="$InputArgument" . ".h";
$theFile="$CLIA/apiHeaderAll.h";
$incLine="#include \"$incFileShort\"\n";
$incTest=`grep $incFileShort $theFile`;
if ($incTest) {
    printf("WARNING, did not update $theFile, $incFileShort already included\n");
}
else {
    insertInFile("before", $theFile, "#endif", $incLine);
    printf("Updated $theFile\n");
}
$cvsCommit=$cvsCommit . " " . $theFile;


#
# Add the .o files to config/api.mk file
#
$theFile="$CONFIG_DIR/api.mk";
$theLine="SVR_API_OBJS += " . "\$" . "(svrApiObjDir)/";
$ch=substr($InputArgument,0,1);
$NewName="rs" . uc($ch) . substr($InputArgument,1) . ".o";
dbPrintf("NewName = $NewName\n");
$theLine = $theLine . $NewName;
dbPrintf("theLine = $theLine\n");
$didUpdate="";
$isThere=`grep $NewName $theFile`;
if (!$isThere) {
    `echo '\n$theLine' >> $theFile`;
    $didUpdate="1";
}
$theLine="LIB_API_OBJS += " . "\$" . "(libApiObjDir)/";
$ch=substr($InputArgument,0,1);
$NewName="rc" . uc($ch) . substr($InputArgument,1) . ".o";
dbPrintf("NewName = $NewName\n");
$theLine = $theLine . $NewName;
dbPrintf("theLine = $theLine\n");
$isThere=`grep $NewName $theFile`;
if (!$isThere) {
    `echo '$theLine' >> $theFile`;
    $didUpdate="1";
}
if ($didUpdate) {
    printf("Updated $theFile\n");
}
else {
    printf("WARNING, did not update $theFile, $NewName already there\n");
}
$cvsCommit=$cvsCommit . " " . $theFile;

#
# Create empty server .c file
#
$ch=substr($InputArgument,0,1);
$NewName="rs" . uc($ch) . substr($InputArgument,1) . ".c";
$theFile="$SSA/" . $NewName;
dbPrintf("theFile = $theFile\n");
$InputFile="$InputArgument" . "Inp";

if (-e $theFile) {
    printf("WARNING, did not create empty $theFile as it already exists\n");
}
else {
    `touch $theFile`;
    printf("Created empty $theFile\n");
}
$cvsAdd = $cvsAdd . " " . $theFile;
$cvsCommit=$cvsCommit . " " . $theFile;

#
# Generate the client .c file
#
$ch=substr($InputArgument,0,1);
$NewName="rc" . uc($ch) . substr($InputArgument,1) . ".c";
$theFile="$CSA/" . $NewName;
dbPrintf("theFile = $theFile\n");
$S1=`grep -A8 'prototype for the client' $incFile`;
dbPrintf ("S1=%s\n",$S1);
$i1=index($S1,"*/");
if ($i1 < 0) {die("No prototype string; check your input .h")};
$i2=index($S1,";");
if ($i2 < 0) {die("No prototype string; check your input .h")};
$prot1=substr($S1,$i1+2,$i2-$i1-2);
dbPrintf("prot1=%s\n",$prot1);
$OutputFile="$InputArgument" . "Out";
if (-e $theFile) {
    printf("WARNING, did not generate $theFile as it already exists\n");
}
else {
    $nStr=$UNAME . "_AN";
    $procArgs="conn, " . $nStr . ", ";
    if ($dataObj) {
	$procArgs=$procArgs . " dataObjInp, ";
    }
    else {
	if ($inStruct) {
	    $procArgs=$procArgs . " $InputFile, ";
	}
	else {
	    $procArgs=$procArgs . "NULL, ";
	}
    }

    if ($inBs) {
	$procArgs=$procArgs . "inBs, \n";
    }
    else {
	$procArgs=$procArgs . "NULL, \n";
    }
    if ($outStruct) {
	$procArgs=$procArgs . "        (void **) $OutputFile,";
    }
    else {
	$procArgs=$procArgs . "        (void **) NULL,";
    }
    if ($outBs) {
	$procArgs=$procArgs . " outBs";
    }
    else {
	$procArgs=$procArgs . " NULL";
    }
    writeFile($theFile, "/* This is script-generated code.  */ \n/* See $incFileShort for a description of this API call.*/\n\n#include \"$incFileShort\"\n$prot1\n{\n    int status;\n    status = procApiRequest ($procArgs);\n\n    return (status);\n}\n");
    printf("Generated $theFile\n");
}
$cvsAdd = $cvsAdd . " " .  $theFile;
$cvsCommit=$cvsCommit . " " . $theFile;
printf("When ready, you need to cvs add the 3 new files and commit all changes:\n");
printf("cvs add $cvsAdd\n");
printf("cvs commit $cvsCommit\n");
exit(0);

#
# Inserts into $InputFile, the line $newText, before or after the line that includes the string $testLine.
# $option specifies before or after.
sub insertInFile {
    my($option, $InputFile, $testLine, $newText) = @_;
    $InputFileOrig = $InputFile . ".orig";
    $OutputFile=$InputFile . ".tmp.4572938";
    open(FileIn, $InputFile) || die("Can't open input InputFile " . "$InputFile");
    open(FileOut, ">".$OutputFile) || die("Can't open output file "."$OutputFile");
    while($line = <FileIn>) {
	if ($option eq "after") {
	    printf FileOut $line;
	    $where = index( $line, $testLine);
	    if ($where >= 0) {
		printf FileOut $newText;
	    }
	}
	else {
	    $where = index( $line, $testLine);
	    if ($where >= 0) {
		printf FileOut $newText;
	    }
	    printf FileOut $line;
	}
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
}

#
# Insert a new #define line in the section denoted with the testLine, incrementing
# the value by one.
#
sub insertNewDefInFile {
    my($InputFile, $testLine, $newName) = @_;
    $InputFileOrig = $InputFile . ".orig";
    $OutputFile=$InputFile . ".tmp.4572938";
    $started=0;
    $ended=0;
    open(FileIn, $InputFile) || die("Can't open input InputFile " . "$InputFile");
    open(FileOut, ">".$OutputFile) || die("Can't open output file "."$OutputFile");
    $didInsert=0;
    while($line = <FileIn>) {
	$where = index( $line, $testLine);
	if ($where >= 0) {
	    $started=1;
	}
	else {
	    if ($started > 0) {
		$where = index( $line, "#define");
		if ($where < 0) {
		    $ended=1;
		}
		if ($ended == 1 && $didInsert==0 ) {
		    $lastVal=substr($prevLine,-5);
		    chomp($lastVal);
		    $nextVal=$lastVal+1;
		    printf FileOut "#define $newName \t\t\t$nextVal\n";
		    $ended = 2;

		    $prevUNAME=substr($prevLine,0,-5);   # save to Global
		    dbPrintf("prevUNAME=%s\n",$prevUNAME);
		    $prevUNAME=substr($prevUNAME, 8);
		    dbPrintf("prevUNAME=%s\n",$prevUNAME); # skip #define 
		    $blank=index($prevUNAME," ");
		    $tab=index($prevUNAME,"\t");
		    if ($tab < $blank) {
			$blank = $tab;
		    }
		    $prevUNAME=substr($prevUNAME,0,$blank-1); # and trim blanks and tabs
		    $didInsert=1;
		}
	    }
	}
	printf FileOut $line;
	$prevLine = $line;
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
}

sub writeFile {
    my($file, $text) = @_;
    open(F, ">$file");
    $_ = $text;
    print F;
    close F;
}

sub appendToFile{
    my($file, $text) = @_;
    open(F, ">>$file");
    $_ = $text;
    print F;
    close F;
}


sub dbPrintf{
    if ($debugFlag) {
	my($arg1, $arg2) = @_;
	printf($arg1, $arg2);
    }
}
