#!/usr/bin/perl -w
#
# script to submit test the load of the sql server.
#
# Copyright (c), CCIN2P3
# For more information please refer to files in the COPYRIGHT directory.

use strict;
use Cwd;
use Sys::Hostname;
use File::Copy;

#-- Declare variables

my $cmd;
my $cnt;
my @commandlist;
my $comment_line;
my $counter;
my $dir_w;
my $errorfile;
my $errorname;
my $host;
my $i;
my $input;
my $irodsdefresource;
my $irodsfile;
my $irodsfile_h;
my $irodshome;
my $irodshost;
my $irodszone;
my $line;
my @list;
my $misc;
my $outputfile;
my $outputname;
my $progname;
my $rc;
my $resgroup;
my $ruletestfile;
my $ruletestname;
my $scriptfile1;
my $scriptfile2;
my $scriptname1;
my $scriptname2;
my $targetfile;
my $targetname;
my $test_number;
my $testgroup;
my $testmeta1;
my $testmeta2;
my $testresource;
my $testuser1;
my $testuser2;
my $tmpdir;
my $tmpdirload;
my $tmpfile1;
my $tmpfile2;
my $tmpfile3;
my $tmpfile4;
my $username;
my @words;

#-- Define some

$comment_line = "\#\#\#\#\#\# Command";
$irodsfile    = "$ENV{HOME}/.irods/.irodsEnv";

#-- Get hostname

$host         = hostname();
if ( $host =~ '.' ) {
	@words = split( /\./, $host );
	$host  = $words[0];
}

#-- Find current working directory and make consistency path

$dir_w        = cwd();
$outputfile   = $dir_w . "/testSurvey_load_" . $host . "_";
$errorfile    = $dir_w . "/testError_load_"  . $host . "_";
$ruletestfile = $dir_w . "/testRule_load_"   . $host . "_";
$scriptfile1  = $dir_w . "/testScript_load_" . $host . "_";
$scriptfile2  = $dir_w . "/testScript_main_" . $host . "_";
$targetfile   = $dir_w . "/testTarget_load_" . $host . "_";

$progname     = $0;
if ( $progname !~ '/' ) {
	$progname = $dir_w . '/' . $progname;
} else {
	if ( substr( $progname, 0, 2 ) eq './' ) {
		@words    = split( /\//, $progname );
		$i        = $#words;
		$progname = $dir_w . '/' . $words[$i];
	}
	if ( substr( $progname, 0, 1 ) ne '/' ) {
		$progname = $dir_w . '/' . $progname;
	}
}

#-- Dump content of $irodsfile to @list

@list = dumpFileContent( $irodsfile );

#-- Loop on content of @list

foreach $line ( @list ) {
 	chomp( $line );
	if ( ! $line ) { next; }
 	if ( $line =~ /irodsUserName/ ) {
		( $misc, $username ) = split( / /, $line );
		$username = substr( $username, 1, -1 );
		next;
	}
	if ( $line =~ /irodsHome/ ) {
		( $misc, $irodshome ) = split( /=/, $line );
		next;
	}
	if ( $line =~ /irodsZone/ ) {
		( $misc, $irodszone ) = split( / /, $line );
		$irodszone = substr( $irodszone, 1, -1 );
		next;
	}
	if ( $line =~ /irodsHost/ ) {
		( $misc, $irodshost ) = split( / /, $line );
		$irodshost = substr( $irodshost, 1, -1 );
		next;
	}
	if ( $line =~ /irodsDefResource/ ) {
		( $misc, $irodsdefresource ) = split( /=/, $line );
	}
}

#-- Remove previous file if any

if ( -e "nohup.out" ) { unlink( "nohup.out" ); }

#-- Text on screen

print( "\nBe sure that the path of the irods command is on your shell environment.\n" );

#-- Ask irods pwd

print( "\nPlease, enter the password of $username iRODS user used for the test: " );
chomp( $input = <STDIN> );
if ( ! $input ) {
	print( "\nYou should give a pwd.\n\n");
	exit;
}

#-- Ask number of tests that should run on //

print( "Please, enter the number of tests you want to do on parallel: " );
chomp( $test_number = <STDIN> );
if ( ! $test_number or $test_number !~ /[0-9]/ ) {
	print( "\nYou should give number. Please retry.\n\n");
	exit;
} else {
	print( "\n" );
}

#-- Loop on the number of tests, make scripts ans run them

undef( @list );
for ( $cnt=1; $cnt<=$test_number; $cnt++ ) {

#---- Define some variables

	$scriptname1  = $scriptfile1  . $cnt . ".sh";
	$scriptname2  = $scriptfile2  . $cnt . ".sh";
	$ruletestname = $ruletestfile . $cnt . ".irb";
	$targetname   = $targetfile   . $cnt . ".txt";
	$outputname   = $outputfile   . $cnt . ".log";
	$errorname    = $errorfile    . $cnt . ".error";
	$irodsfile_h  = "/tmp/.irodsEnv_" . $host . "_" . $cnt;
	
	push( @list, $errorname );
	
	$testuser1    = "testuser1_" . $host . "_" . $cnt;
	$testuser2    = "testuser2_" . $host . "_" . $cnt;
	$testgroup    = "testgroup_" . $host . "_" . $cnt;
	$testresource = "testresource_" . $host . "_" . $cnt;
	$tmpdir       = "/tmp/foo_" . $host . "_" . $cnt;
	$resgroup     = "resgroup_" . $host . "_" . $cnt;
	$tmpdirload   = "$irodshome/test_load_" . $host . "_" . $cnt;
	$testmeta1    = "testmeta1_" . $host . "_" . $cnt;
	$testmeta2    = "testmeta2_" . $host . "_" . $cnt;
	$tmpfile1     = "$tmpdirload/foo1";
	$tmpfile2     = "$tmpdirload/foo2";
	$tmpfile3     = "$tmpdirload/foo3";
	$tmpfile4     = "$tmpdirload/foo4";

#---- Remove previous files if any

	if ( -e $scriptname1  ) { unlink( $scriptname1 ); }
	if ( -e $scriptname2  ) { unlink( $scriptname2 ); }
	if ( -e $ruletestname ) { unlink( $ruletestname ); }
	if ( -e $outputname   ) { unlink( $outputname ); }
	if ( -e $errorname    ) { unlink( $errorname ); }
	if ( -e $irodsfile_h  ) { unlink( $irodsfile_h ); }

#---- Copy irodsEnv to /tmp

	$rc = copy( $irodsfile, $irodsfile_h );
	if ( ! $rc ) {
		print( FILE "Unable to copy $irodsfile_h.\n" );
		exit;	
	}

#---- Undef @commandlist and make list of commands

	undef( @commandlist );
	
	push( @commandlist, "iinit $input" );
	push( @commandlist, "iadmin lt" );
	push( @commandlist, "iadmin lz" );
	push( @commandlist, "iadmin mkuser $testuser1 domainadmin" );
	push( @commandlist, "iadmin lu $testuser1" );
	push( @commandlist, "iadmin moduser $testuser1 type rodsuser" );
	push( @commandlist, "iadmin lu $testuser1" );
	push( @commandlist, "iadmin mkuser $testuser2 rodsuser" );
	push( @commandlist, "iadmin lu" );
	push( @commandlist, "iadmin mkgroup $testgroup" );
	push( @commandlist, "iadmin atg $testgroup $testuser1" );
	push( @commandlist, "iadmin atg $testgroup $testuser2" );
	push( @commandlist, "iadmin lg $testgroup" );
	push( @commandlist, "iadmin rfg $testgroup $testuser2" );
	push( @commandlist, "iadmin lg $testgroup" );
	push( @commandlist, "iadmin rfg $testgroup $testuser1" );
	push( @commandlist, "iadmin mkresc $testresource \"unix file system\" cache $irodshost \"$tmpdir\"" );
	push( @commandlist, "iadmin lr $testresource" );
	push( @commandlist, "iadmin lr $testresource" );
	push( @commandlist, "iadmin lr $testresource" );
	push( @commandlist, "iadmin modresc $testresource comment \"Modify by me $username\"" );
	push( @commandlist, "iadmin lr $testresource" );
	push( @commandlist, "iadmin mkgroup $resgroup" );
	push( @commandlist, "iadmin atg $resgroup $username" );
	push( @commandlist, "iadmin atrg $resgroup $testresource" );
	push( @commandlist, "iadmin atrg $resgroup $irodsdefresource" );
	push( @commandlist, "iadmin lrg $resgroup" );
	push( @commandlist, "ilsresc" );
	push( @commandlist, "imiscsvrinfo" );
	push( @commandlist, "iuserinfo" );
	push( @commandlist, "imkdir $tmpdirload" );
	push( @commandlist, "iput -K -N 2 $progname $tmpfile1" );
	push( @commandlist, "ils $tmpfile1" );
	push( @commandlist, "iadmin ls $tmpdirload" );
	push( @commandlist, "ils -A $tmpfile1" );
	push( @commandlist, "ichmod read $testuser1 $tmpfile1" );
	push( @commandlist, "ils -A $tmpfile1" );
	push( @commandlist, "irepl -B -R $testresource $tmpfile1" );
	push( @commandlist, "ils -l $tmpfile1" );
	push( @commandlist, "imeta add -d $tmpfile1 $testmeta1 180 cm" );
	push( @commandlist, "imeta ls -d $tmpfile1" );
	push( @commandlist, "icp -K $tmpfile1 $tmpfile2" );
	push( @commandlist, "ils $tmpfile2" );
	push( @commandlist, "imeta add -d $tmpfile2 $testmeta1 180 cm" );
	push( @commandlist, "imeta add -d $tmpfile1 $testmeta2 hello" );
	push( @commandlist, "imeta ls -d $tmpfile1" );
	push( @commandlist, "imeta qu -d $testmeta1 = 180" );
	push( @commandlist, "imeta qu -d $testmeta2 = hello" );
	push( @commandlist, "iget -f -K -N 2 $tmpfile1 $targetname" );
	push( @commandlist, "irule -F $ruletestname" );
	push( @commandlist, "irm -f $tmpfile3" );
	push( @commandlist, "imeta rm -d $tmpfile1 $testmeta2 hello" );
	push( @commandlist, "imeta rm -d $tmpfile2 $testmeta1 180 cm" );
	push( @commandlist, "irm -f $tmpfile2" );
	push( @commandlist, "imeta rm -d $tmpfile1 $testmeta1 180 cm" );
	push( @commandlist, "irm -f $tmpfile1" );
	push( @commandlist, "irm -r $tmpdirload" );
	push( @commandlist, "iadmin rfrg $resgroup $irodsdefresource" );
	push( @commandlist, "iadmin rfrg $resgroup $testresource" );
	push( @commandlist, "iadmin rfg $resgroup $username" );
	push( @commandlist, "iadmin rmgroup $resgroup" );
	push( @commandlist, "iadmin rmresc $testresource" );
	push( @commandlist, "iadmin rmgroup $testgroup" );
	push( @commandlist, "iadmin rmuser $testuser2" );
	push( @commandlist, "iadmin rmuser $testuser1" );
	push( @commandlist, "iadmin lg" );
	push( @commandlist, "iadmin lg" );
	push( @commandlist, "iadmin lr" );
	push( @commandlist, "iexit" );

#---- Make main scriptfile

	$rc = open( FILE, ">$scriptname2" );
	if ( ! $rc ) {
		print( "Unable to open the file $scriptname2 in write mode.\n" );
		exit;
	}
	
	print( FILE "#!/bin/sh\n" );
	print( FILE "\n" );
	print( FILE "chmod 600 $outputname\n" );
	print( FILE "chmod 600 $errorname\n" );
	print( FILE "\n" );
	print( FILE "date_beg=\`date\`\n" );
	print( FILE "\n" );
	print( FILE "if [ -f $targetname ]; then\n" );
	print( FILE "	echo \"\\n\\nRemove $targetname\"\n" );
	print( FILE "	rm $targetname\n" );
	print( FILE "fi\n" );
	print( FILE "\n" );
	print( FILE "time $scriptname1\n" );
	print( FILE "\n" );
	print( FILE "date_end=\`date\`\n" );
	print( FILE "\n" );
	print( FILE "echo \"End of $scriptname2\"\n" );
	print( FILE "echo \"date begin: \${date_beg}\"\n" );
	print( FILE "echo \"date ended: \${date_end}\"\n" );
	print( FILE "echo \"Upper results by script number $cnt\" 1>&2\n" );
	print( FILE "exit\n" );
	close( FILE );

	chmod( 0700, $scriptname2 );

#---- Make load scriptfile

	$rc = open( FILE, ">$scriptname1" );
	if ( ! $rc ) {
		print( "Unable to open the file $scriptname1 in write mode.\n" );
		exit;
	}

	print( FILE "#!/bin/sh\n" );
	print( FILE "\n" );
	print( FILE "irodsEnvFile=${irodsfile_h}\n" );
	print( FILE "export irodsEnvFile\n" );
	print( FILE "\n" );
	print( FILE "echo \n" );
	
	$counter = 0;
	foreach $cmd ( @commandlist ) {
		$counter++;
		print( FILE "echo \"$comment_line $counter - $cmd\"\n" );
		print( FILE "$cmd\n" );		
	}

	print( FILE "\n" );
	print( FILE "echo \"End of $scriptname1\"\n" );
	print( FILE "exit\n" );
	close( FILE );
	
	chmod( 0700, $scriptname1 );

#---- Make testrule file

	$rc = open( FILE, ">$ruletestname" );
	if ( ! $rc ) {
		print( "Unable to open the file $ruletestname in write mode.\n" );
		exit;
	}
	
	print( FILE "# This is an example of an input for the irule command.\n" );
	print( FILE "# This first input line is the rule body.\n" );
	print( FILE "# The second input line is the input parameter in the format of:\n" );
	print( FILE "#   label=value. e.g., *A=$tmpfile1\n" );
	print( FILE "# Multiple inputs can be specified using the \'\%\' character as the separator.\n" );
	print( FILE "# The third input line is the output description. Multiple outputs can be specified\n" );
	print( FILE "# using the \'\%\' character as the separator.\n" );
	print( FILE "#\n" );
	print( FILE "myTestRule||msiDataObjOpen(*A,*S_FD)" );
	print( FILE "##msiDataObjCreate(*B,$irodsdefresource,*D_FD)" );
	print( FILE "##msiDataObjLseek(*S_FD,10,SEEK_SET,*junk1)" );
	print( FILE "##msiDataObjRead(*S_FD,10000,*R_BUF)" );
	print( FILE "##msiDataObjWrite(*D_FD,*R_BUF,*W_LEN)" );
	print( FILE "##msiDataObjClose(*S_FD,*junk2)" );
	print( FILE "##msiDataObjClose(*D_FD,*junk3)" );
	print( FILE "##msiDataObjCopy(*B,*C,$irodsdefresource,*junk4)" );
	print( FILE "##delayExec(msiDataObjRepl(*C,$irodsdefresource,*junk5),<A></A>)" );
	print( FILE "##msiDataObjUnlink(*B,*junk6)" );
	print( FILE "\n" );
	print( FILE "*A=$tmpfile1\%*B=$tmpfile4\%*C=$tmpfile3" );
	print( FILE "\n" );
	print( FILE "*R_BUF\%*W_LEN" );
	print( FILE "\n" );
	close( FILE );
	
#---- Submit the scripts

	print( "Fork process $cnt.\n" );
	$rc = system( "nohup $scriptname2 1>>$outputname 2>>$errorname &" );
	if ( $rc != 0 ) { print( "Child error, rc = $rc.\n" ); }
}

#-- End

exit;
##########################################################################################################################
# dumpFileContent: open a file in order to dump its content to an array

sub dumpFileContent {
	my $file = shift;
	my @filecontent;
	my $line;

	open( DUMP, $file ) or die "Unable to open the file $file in read mode.\n";
	foreach $line ( <DUMP> ) {
		$line =~ s/^\s+//;		# remove blanks
		$line =~ s/\s+$//;		# remove blanks
		push( @filecontent, $line );
	}
	close( DUMP );
	return( @filecontent );
}
