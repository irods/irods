#!/usr/bin/perl -w
#
# script to testing multi-host data movement of icommands.
# icommands location has to be put in the PATH env variable or their PATH will be asked
# at the beginning of the execution of this script.
#
# Usage: ./mhostsTestiCommands.pl [help] [debug] [noprompt] [addr1 addr2 addr3]
#    help - Print usage messages.
#    debug - print debug messages.
#    noprompt - Assumes iinit was done before running this script 
#        and will not ask for path nor password input.
#    addrN - The 3 iRODS server addresses for this test. If they are not
#        defined, the values defined by $localHostAddr, $host2Addr and $host3Addr
#        in this script will be used.
# 
#

use strict;
use Cwd;
use Sys::Hostname;
use File::stat;
use File::Copy;
use JSON;

#-- Initialization
# This test has 3 servers and 3 resources. localHostAddr is the address of
# the server where this test script is run. These 3 addresses can be input 
# on the mhostsTestiCommands.pl command line.If they are not given, 
# the values defined below will be used.
# my $localHostAddr="srbbrick15.ucsd.edu";
my $localHostAddr="one.ucsd.edu";
my $host2Addr="srbbrick15.ucsd.edu";
my $host3Addr="srbbrick14.ucsd.edu";
my $resc1="myresc1";
my $resc2="myresc2";
my $resc3="myresc3";
# $doRbudpTest - whether to do Rbudp Test. "yes" or "no". Default is yes
my $doRbudpTest = "yes";
my @hostList;
my $hostAddr;

my $debug;
my $entry;
my @failureList;
my $i;
my $input;
my $irodsdefresource;
my $irodshost;
my $irodshome;
my $irodszone;
my $line;
my @list;
my $misc;
my $nfailure;
my $nsuccess;
my $rc;
my @returnref;
my @successlist;
my @summarylist;
my @tmp_tab;
my $username;
my @words;

# If noprompt_flag is set to 1, it assume iinit was done before running this
# script and will not ask for path and password input. A "noprompt" input 
# will set it.
my $noprompt_flag;
my $arg;
my $inHostCnt;

$debug = 0;
$noprompt_flag = 0;
$inHostCnt = 0;
foreach $arg (@ARGV)
{
    if ( $arg =~ "debug" ) {
	if ($inHostCnt > 0) {
            &printUsage ();
            exit ( 1 );
	} else {
            $debug = 1;
        }
    } elsif ( $arg =~ "noprompt" ) {
        if ($inHostCnt > 0) {
            &printUsage ();
            exit ( 1 );
        } else {
	    $noprompt_flag = 1;
        }
    } elsif ( $arg =~ "help" ) {
        &printUsage ();
	exit( 0 );
    }  elsif ($inHostCnt == 0) {
	$localHostAddr=$arg;
	$inHostCnt++;
    }  elsif ($inHostCnt == 1) {
        $host2Addr=$arg;
        $inHostCnt++;
    }  elsif ($inHostCnt == 2) {
        $host3Addr=$arg;
        $inHostCnt++;
    } else {
	print ("Too many address input - must be 0 or 3.\n\n");
        &printUsage ();
	exit ( 1 );
    }
}

if ($inHostCnt > 0 && $inHostCnt < 3 ) {
    print ("Not enough address input - must be 0 or 3.\n\n");
    &printUsage ();
    exit ( 1 );
}

print ("Host addresses used are:  $localHostAddr  $host2Addr  $host3Addr\n");
push ( @hostList, $localHostAddr );
push ( @hostList, $host2Addr );
push ( @hostList, $host3Addr );

my $dir_w        = cwd();
my $testsrcdir = $dir_w . '/testsrc';
my $myldir = $testsrcdir . '/ldir';
my $mylsize;
my $mysdir = $testsrcdir . '/sdir';
my $myssize;
my $host         = hostname();
if ( $host =~ '.' ) {
	@words = split( /\./, $host );
	$host  = $words[0];
}
my $irodsfile;
my $irodsEnvFile = $ENV{'IRODS_ENVIRONMENT_FILE'};
if ($irodsEnvFile) {
    $irodsfile = $irodsEnvFile;
} else {
    $irodsfile    = "$ENV{HOME}/.irods/irods_environment.json";
}
my $ntests       = 0;
my $progname     = $0;

my $outputfile   = "multiHostTest" . $host . ".log";
my $sfile2 = $dir_w . '/sfile2';
my $sfile2size;
system ( "cat $progname $progname > $sfile2" );
$sfile2size =  stat ($sfile2)->size;


#-- Find current working directory and make consistency path

$outputfile   = $dir_w . '/' . $outputfile;

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

#-- Take debug level

# $debug = shift;
# if ( ! $debug ) {
# 	$debug = 0;
# } else {
# 	$debug = 1;
# }

#-- Print debug

if ( $debug ) {
	print( "\n" );
	print( "MAIN: irodsfile        = $irodsfile\n" );
	print( "MAIN: cwd              = $dir_w\n" );
	print( "MAIN: outputfile       = $outputfile\n" );
	print( "MAIN: progname         = $progname\n" );
	print( "\n" );
}

#-- Dump content of $irodsfile to @list
# read and parse the irods json environment file
# and extract useful values
my $json_text = do {
   open(my $json_fh, "<:encoding(UTF-8)", $irodsfile)
	 or die("Can't open \$filename\": $!\n");
	    local $/;
	       <$json_fh>
};

my $json = JSON->new;
my $data = $json->decode($json_text);

$username   = $data->{ "irods_user_name" };
$irodshome = $data->{ "irods_home" };
$irodszone = $data->{ "irods_zone_name" };
$irodshost = $data->{ "irods_host" };
$irodsdefresource = $data->{ "irods_default_resource" };


my $tempFile   = "/tmp/iCommand.log";
#@list = dumpFileContent( $irodsfile );

#-- Loop on content of @list
# The below parsing works in the current environment 
# but there are two shortcomings:
#   1) single quotes are removed, but if there were to be embedded ones,
#      they would be removed too.
#   2) if the name and value are separated by =, the line will not split right.
#foreach $line ( @list ) {
# 	chomp( $line );
#	if ( ! $line ) { next; }
# 	if ( $line =~ /irodsUserName/ ) {
#		( $misc, $username ) = split( / /, $line );
#		$username =~ s/\'//g; #remove all ' chars, if any
#		next;
#	}
#	if ( $line =~ /irodsHome/ ) {
#		( $misc, $irodshome ) = split( / /, $line );
#		$irodshome =~ s/\'//g; #remove all ' chars, if any
#		next;
##	}
#	if ( $line =~ /irodsZone/ ) {
#		( $misc, $irodszone ) = split( / /, $line );
#		$irodszone =~ s/\'//g; #remove all ' chars, if any
#		next;
#	}
#	if ( $line =~ /irodsHost/ ) {
#		( $misc, $irodshost ) = split( / /, $line );
#		$irodshost =~ s/\'//g; #remove all ' chars, if any
#		next;
#	}
#	if ( $line =~ /irodsDefResource/ ) {
#		( $misc, $irodsdefresource ) = split( / /, $line );
#		$irodsdefresource =~ s/\'//g; #remove all ' chars, if any
#	}
#}

#-- Print debug

if ( $debug ) {
	print( "MAIN: username         = $username\n" );
	print( "MAIN: irodshome        = $irodshome\n" );
	print( "MAIN: irodszone        = $irodszone\n" );
	print( "MAIN: irodshost        = $irodshost\n" );
	print( "MAIN: irodsdefresource = $irodsdefresource\n" );
}

#-- Environment setup and print to stdout

print( "\nThe results of the test will be written in the file: $outputfile\n\n" );
print( "Warning: you need to be a rodsadmin in order to pass successfully all the tests," );
print( " as some admin commands are being tested." );
if ( ! $noprompt_flag ) {
    print( " If icommands location has not been set into the PATH env variable," );
    print( " please give it now, else press return to proceed.\n" );
    print( "icommands location path: " );
    chomp( $input = <STDIN> );
    if ( $input ) { $ENV{'PATH'} .= ":$input"; }
    print "Please, enter the password of the iRODS user used for the test: ";
    chomp( $input = <STDIN> );
    if ( ! $input ) {
	print( "\nYou should give valid pwd.\n\n");
	exit;
    } else {
	print( "\n" );
    }


    runCmd( "iinit $input" );
}

#-- Test the icommands and eventually compared the result to what is expected.

#-- Basic admin commands to make the needed resources.

runCmd( "iadmin mkresc $resc1 \"unix file system\" \"$localHostAddr:/tmp/myresc1\"", "", "", "", "iadmin rmresc $resc1" );
runCmd( "iadmin mkresc $resc2 \"unix file system\" \"$host2Addr:/tmp/myresc2\"", "", "", "", "iadmin rmresc $resc2" );
runCmd( "iadmin mkresc $resc3 \"unix file system\" \"$host3Addr:/tmp/myresc3\"", "", "", "", "iadmin rmresc $resc3" );
#runCmd( "iadmin mkresc compresource \"unix file system\" compound $host3Addr \"/tmp/compresc\"", "", "", "", "iadmin rmresc compresource" );
#runCmd( "iadmin atrg resgroup $resc3", "", "", "", "iadmin rfrg resgroup $resc3" );
#runCmd( "iadmin atrg resgroup compresource", "", "", "", "iadmin rfrg resgroup compresource" );

# transfer file test
# make directory containing 20 small files and a directory with 2 large files
system ( "mkdir $testsrcdir" );
mksdir ();
mkldir ();
my $testlfile = $myldir . '/lfile1';
runCmd( "icd $irodshome" );
runCmd( "imkdir $irodshome/icmdtest", "", "", "", "irm -r $irodshome/icmdtest" );
# loop through the all hosts 
foreach $hostAddr (@hostList) {
    print ("CONNECT to host: $hostAddr\n");
    # test small file put/get
    $ENV{'IRODS_HOST'}  = $hostAddr;
    runCmd( "iput -KrR $resc2 $progname $irodshome/icmdtest/foo1" );
    runCmd( "iget -f -K $irodshome/icmdtest/foo1 $dir_w" );
    runCmd( "diff  $dir_w/foo1 $progname", "", "NOANSWER" );
    runCmd( "iput -kf $progname $irodshome/icmdtest/foo1" );
    runCmd( "iget -f -K $irodshome/icmdtest/foo1 $dir_w" );
    runCmd( "diff  $dir_w/foo1 $progname", "", "NOANSWER" );
    system ( "irm -f $irodshome/icmdtest/foo1" );
    system ( "rm $dir_w/foo1" );
    # test large file put/get
    runCmd( "iput -KrR $resc2 $testlfile $irodshome/icmdtest/foo1" );
    runCmd( "iget -f -K $irodshome/icmdtest/foo1 $dir_w" );
    runCmd( "diff  $dir_w/foo1 $testlfile", "", "NOANSWER" );
    runCmd( "iput -kf -N0 $testlfile $irodshome/icmdtest/foo1" );
    runCmd( "iget -f -K -N0 $irodshome/icmdtest/foo1 $dir_w" );
    runCmd( "diff  $dir_w/foo1 $testlfile", "", "NOANSWER" );
    system ( "irm -f $irodshome/icmdtest/foo1" );
    system ( "rm $dir_w/foo1" );
    # test a directory of file between $resc2 and $resc3
    runCmd( "iput -KrR $resc2 --wlock $testsrcdir $irodshome/icmdtest/dir1" );

    runCmd( "ils -l $irodshome/icmdtest/dir1/sdir/sfile1", "", "LIST", "sfile1, $myssize" );
    runCmd( "irepl -Br -R $resc3 --rlock $irodshome/icmdtest/dir1" );
    runCmd( "ils -l $irodshome/icmdtest/dir1/sdir/sfile1", "", "LIST", "1 $resc3" );
    runCmd( "itrim -rS $resc2 -N1 $irodshome/icmdtest/dir1" );
    runCmd( "iphymv -rR $resc2 $irodshome/icmdtest/dir1" );
    runCmd( "ils -l $irodshome/icmdtest/dir1/sdir/sfile1", "", "LIST", "$resc2" );
    runCmd( "itrim -rS  $resc2 -N1 $irodshome/icmdtest/dir1" );
    runCmd( "icp -rK -R $resc3 $irodshome/icmdtest/dir1 $irodshome/icmdtest/dir2" );
    runCmd( "ils $irodshome/icmdtest/dir2/sdir/sfile1", "", "LIST", "sfile1" );
    runCmd( "imv $irodshome/icmdtest/dir2 $irodshome/icmdtest/dir3" );
    runCmd( "ils $irodshome/icmdtest/dir3/sdir/sfile1", "", "LIST", "sfile1" );
    runCmd( "ichksum -Kr $irodshome/icmdtest/dir3" );
    runCmd( "irm -vrf $irodshome/icmdtest/dir1" );
    # we have dir3 in $resc2. test between $resc2 and $resc1
    runCmd( "irepl -Br -R $resc1 $irodshome/icmdtest/dir3" );
    runCmd( "ils -l $irodshome/icmdtest/dir3/sdir/sfile1", "", "LIST", "$resc1" );
    runCmd( "itrim -rS $resc2 -N1 $irodshome/icmdtest/dir3" );
    runCmd( "iphymv -rR $resc2 $irodshome/icmdtest/dir3" );
    runCmd( "ils -l $irodshome/icmdtest/dir3/sdir/sfile1", "", "LIST", "$resc2" );
    runCmd( "itrim -rS  $resc2 -N1 $irodshome/icmdtest/dir3" );
    runCmd( "icp -rK -R $resc2 $irodshome/icmdtest/dir3 $irodshome/icmdtest/dir2" );
    runCmd( "ils $irodshome/icmdtest/dir2/sdir/sfile1", "", "LIST", "sfile1" );
    runCmd( "imv $irodshome/icmdtest/dir2 $irodshome/icmdtest/dir1" );
    runCmd( "ils $irodshome/icmdtest/dir1/sdir/sfile1", "", "LIST", "sfile1" );
    runCmd( "ichksum -Kr $irodshome/icmdtest/dir1" );
    runCmd( "irm -vrf $irodshome/icmdtest/dir3" );
    # we have dir1 in $resc2
    runCmd( "iget -f -rK --rlock $irodshome/icmdtest/dir1 $dir_w" );
    runCmd( "diff -r $dir_w/dir1 $testsrcdir", "", "NOANSWER" );
    system ( "rm -r $dir_w/dir1" );
    runCmd( "irm -vrf $irodshome/icmdtest/dir1" );

    # bulk test
    runCmd( "iput -bPKrR $resc2 $testsrcdir $irodshome/icmdtest/dir1" );
    runCmd( "iget -f -rK $irodshome/icmdtest/dir1 $dir_w" );
    runCmd( "diff -r $dir_w/dir1 $testsrcdir", "", "NOANSWER" );
    system ( "rm -r $dir_w/dir1" );
    # iphybun test. we have dir1 in $resc2`h
    runCmd( "iphybun -KRresgroup $irodshome/icmdtest/dir1/sdir" );
    # $resc3 is the cache resc of resgroup
    runCmd( "ibun -c -R $resc3 $irodshome/icmdtest/sdir.tar $irodshome/icmdtest/dir1/sdir" );
    runCmd( "ibun -x $irodshome/icmdtest/sdir.tar $irodshome/icmdtest/testbunx" );
    system ( "irm -fr $irodshome/icmdtest/sdir.tar $irodshome/icmdtest/testbunx" );
    runCmd( "itrim -rS $resc3 -N1 $irodshome/icmdtest/dir1/sdir" );
    runCmd( "itrim -rS $resc2 -N1 $irodshome/icmdtest/dir1/sdir" );
    runCmd( "iget -r $irodshome/icmdtest/dir1/sdir  $dir_w" );
    runCmd( "diff -r $dir_w/sdir $testsrcdir/sdir", "", "NOANSWER" );
    runCmd( "itrim -rS $resc3 -N1 $irodshome/icmdtest/dir1/sdir" );
    # get the name of bundle file
    my $bunfile = &getBunpathOfSubfile ( "$irodshome/icmdtest/dir1/sdir/sfile1" );
    runCmd( "irm -f --empty $bunfile" );
    # should not be able to remove it because it is not empty
    runCmd( "ils $bunfile",  "", "LIST", "$bunfile" );
    runCmd( "irm -rvf $irodshome/icmdtest/dir1" );
    runCmd( "irm -f --empty $bunfile" );
    system ( "rm -r $dir_w/sdir" );

    # resource group test
    runCmd( "iput -PKrR resgroup $testsrcdir/sdir $irodshome/icmdtest/sdir" );
    runCmd( "irepl -ar $irodshome/icmdtest/sdir" );
    runCmd( "itrim -rS $resc3 -N1 $irodshome/icmdtest/sdir" );
    runCmd( "iget -r $irodshome/icmdtest/sdir  $dir_w" );
    runCmd( "diff -r $dir_w/sdir $testsrcdir/sdir", "", "NOANSWER" );
    runCmd( "irm -rvf $irodshome/icmdtest/sdir" );
    runCmd( "imkdir $irodshome/icmdtestm" );
    runCmd( "imcoll -m filesystem -R $resc1 $dir_w/sdir $irodshome/icmdtestm" );
    runCmd( "ils -r $irodshome/icmdtestm", "", "LIST", "sfile1,sfile2" );
    runCmd( "imcoll -U $irodshome/icmdtestm" );
    system( "irm -rf $irodshome/icmdtestm" );
    system ( "rm -r $dir_w/sdir" );
    runCmd( "iexecmd -H $host2Addr hello", "", "LIST", "Hello world" );
    runCmd( "ips -H $host2Addr" );
    system ( "cp $progname /tmp/regphyfile" );
    runCmd( "ireg -KR $resc1 /tmp/regphyfile  $irodshome/icmdtest/foo5" );
    system ( "irm -f $irodshome/icmdtest/foo5" );

    # do the large files tests using RBUDP
    if ( $doRbudpTest =~ "yes" ) {
        runCmd( "iput -vQPKrR $resc2 --wlock $testsrcdir $irodshome/icmdtest/dir1" );
        runCmd( "irepl -BQvrPT -R $resc3 --rlock $irodshome/icmdtest/dir1" );
        runCmd( "itrim -vrS $resc2 -N1 $irodshome/icmdtest/dir1" );
        runCmd( "icp -vQKPTr -R $resc2 $irodshome/icmdtest/dir1 $irodshome/icmdtest/dir2" );
        system ( "irm -vrf $irodshome/icmdtest/dir1" );
        runCmd( "iget -vQPKr --rlock $irodshome/icmdtest/dir2 $dir_w/dir2" );
        runCmd( "diff -r $dir_w/dir2 $testsrcdir", "", "NOANSWER" );
        system ( "rm -r $dir_w/dir2" );
        system ( "irm -vrf $irodshome/icmdtest/dir2" );
    }
}
system ( "rm -r $testsrcdir" );
system ( "irmtrash" );

#-- Execute rollback commands

if ( $debug ) { print( "\nMAIN ########### Roll back ################\n" ); }

for ( $i = $#returnref; $i >= 0; $i-- ) {
	undef( @tmp_tab );
	$line     = $returnref[$i];
	@tmp_tab = @{$line};	
	runCmd( $tmp_tab[0], $tmp_tab[1], $tmp_tab[2], $tmp_tab[3] );
}

#-- Execute last commands before leaving

if ( $debug ) { print( "\nMAIN ########### Last ################\n" ); }

runCmd( "iadmin lr", "negtest", "LIST", "testresource" );
runCmd( "irmtrash" );
if ( ! $noprompt_flag ) {
    runCmd( "iexit full" );
}
`/bin/rm -rf /tmp/foo`;# remove the vault for the testresource; needed in case
                       # another unix login runs this test on this host
`/bin/rm -rf /tmp/comp`;
#-- print the result of the test into testSurvey.log

$nsuccess = @successlist;
$nfailure = @failureList;

open( FILE, "> $outputfile" ) or die "unable to open $outputfile for writing\n";
print( FILE "============================================\n" );
print( FILE "number of successfully tested commands = $nsuccess\n" );
print( FILE "number of failed tested commands       = $nfailure\n" );
print( FILE "============================================\n\n" );
print( FILE "Summary of the consecutive commands which have been tested:\n\n" );

$i = 1;
foreach $line ( @summarylist ) {
  print( FILE "$i - $line\n" );
  $i++;
}
print( FILE "\n\nList of successful tests:\n\n" );
foreach $line ( @successlist ) { print( FILE "$line" ); }
print( FILE "\n\nList of failed tests:\n\n" );
foreach $line ( @failureList ) { print( FILE "$line" ); }
print( FILE "\n" );
close( FILE );
exit;

##########################################################################################################################
# runCmd needs at least 8 arguments: 
#   1- command name + arguments
#   2- specify if it is a negative test by providing the "negtest" value, ie it is successfull if the test fails (optional).
#   3- output line of interest (optional), if equal to "LIST" then match test the entire list of answers provided in 4-. if equal to "NOANSWER" then expect no answer.
#   4- expected list of results separeted by ',' (optional: must be given if second argument provided else it will fail).
#	5- command name to go back to first situation
#	6- same as 2 but for 5
#	7- same as 3 but for 5
#	8- same as 4 but for 5

sub runCmd {
 	my ( $cmd, $testtype, $stringToCheck, $expResult, $r_cmd, $r_testtype, $r_stringToCheck, $r_expResult ) = @_;

	my $rc = 0;
	my @returnList;
	my @words;
	my $line;

 	my $answer     = "";
 	my @list       = "";
 	my $numinlist  = 0;
 	my $numsuccess = 0;
 	my $negtest    = 0;
 	my $result     = 1; 		# used only in the case where the answer of the command has to be compared to an expected answer.

#-- Check inputs

	if ( ! $cmd ) {
		print( "No command given to runCmd; Exit\n" );
		exit;
	}
	if ( ! $testtype ) {
		$testtype = 0;
		$negtest  = 0;
	} else {
		if ( $testtype eq "negtest" ) {
			$negtest = 1;
		} else {
			$negtest = 0;
		}	
	}
	if ( ! $stringToCheck   ) { $stringToCheck = ""; }
	if ( ! $expResult       ) { $expResult = ""; }
	if ( ! $r_cmd           ) { $r_cmd = ""; }
	if ( ! $r_testtype      ) { $r_testtype = ""; }
	if ( ! $r_stringToCheck ) { $r_stringToCheck = ""; }
	if ( ! $r_expResult     ) { $r_expResult = ""; }

#-- Update counter

	$ntests++;

#-- Print debug
	
	if ( $debug ) { print( "\n" ); }
	printf( "%3d - cmd executed: $cmd\n", $ntests );
	if ( $debug ) { print( "DEBUG: input to runCMd: $cmd, $testtype, $stringToCheck, $expResult.\n" ); }

#-- Push return command in list

	undef( @returnList );
	if ( $r_cmd ){
		$returnList[0] = $r_cmd;
		$returnList[1] = $r_testtype;
		$returnList[2] = $r_stringToCheck;
		$returnList[3] = $r_expResult;
		push( @returnref, \@returnList );
		
		if ( $debug ) { print( "DEBUG: roll back:       $returnList[0], $returnList[1], $returnList[2], $returnList[3].\n" ); }
	} else {
		if ( $debug ) { print( "DEBUG: roll back:       no.\n" ); }		
	}
	
#-- Push icommand in @summarylist

	push( @summarylist, "$cmd" );

#-- Execute icommand

	$rc = system( "$cmd > $tempFile" );

#-- check that the list of answers is part of the result of the command.

	if (( $rc == 0 ) and $stringToCheck ) {
		@words     = split( ",", $expResult );
		$numinlist = @words;
		@list      = dumpFileContent( $tempFile );
		
		if ( $debug ) {
			print( "DEBUG: numinlist = $numinlist\n" );
			print( "DEBUG: list =\n@list\n" );
		}
				
#---- If LIST is given as 3rd element: compare output of icommand to list in 4th argument
		
		if ( $stringToCheck eq "LIST" ) {
			foreach $line ( @list ) {
				chomp( $line );
				$line =~ s/^\s+//;
				$line =~ s/\s+$//;
				$answer .= "$line ";
			}
			chomp( $answer );
			$answer =~ s/^\s+//;
			$answer =~ s/\s+$//;

			if ( $debug ) { print( "DEBUG: answer    = $answer\n" ); }

			foreach $entry ( @words ) {
				if ( $answer =~ /$entry/ ) { $numsuccess++; }
			}
			
			if ( $numsuccess >= $numinlist ) {
				$result = 1;
			} else {
				$result = 0;
			}
		} elsif ( $stringToCheck eq "NOANSWER" ) {
			my $numanswer = @list;
			if ($numanswer == 0) {
				$result = 1;
                        } else {
                                $result = 0;
                        }
		} else {
			if ( $debug ) { print( "DEBUG: stringToCheck = $stringToCheck\n" ); }
			foreach $line ( @list ) {
				chomp( $line );
				if ( $debug ) { print( "DEBUG: line = $line\n" ); }
				if ( $line =~ /$stringToCheck/ ) {
					( $misc, $answer ) = split( /$stringToCheck/, $line );
					$answer =~ s/^\s+//;		# remove blanks
					$answer =~ s/\s+$//;		# remove blanks
					last;
				}
			}
			
			if ( $answer eq $words[0] ) {
				$result = 1;
			} else {
				$result = 0;
			}
		}
	}
	
	if ( $rc == 0 and ( $result ^ $negtest ) ) {
		push( @successlist, "$ntests - $cmd  ====> OK\n" );
		$result = 1;
	} else {
		push( @failureList, "$ntests - $cmd  ====> error code = $rc\n" );
		$result = 0;
	}

	if ( $debug ) { print( "DEBUG: result    = $result (1 is OK).\n" ); }			
	
	unlink( $tempFile );
	return();
}
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

# make a directory of 2 large files and 2 small fles
sub mkldir
{
    my $i;
    my $count = 5; 
    my $fcount = 2; 
    my $mylfile;
    my $mysfile;
    my $lfile = $dir_w . "/lfile";
    my $lfile1 = $dir_w . "/lfile1";
    system( "echo 012345678901234567890123456789012345678901234567890123456789012 > $lfile" );
    for ( $i = $count; $i >= 0; $i-- ) {
      system ( "cat $lfile $lfile $lfile $lfile $lfile $lfile $lfile $lfile $lfile > $lfile1" );
	rename ( $lfile1, $lfile );
    }
    $mylsize = stat ($lfile)->size;
    system ( "mkdir $myldir" );
    for ( $i = $fcount; $i > 0; $i-- ) {
        $mylfile = $myldir . '/' . 'lfile' . $i;
	if ($i != 1) {
	    copy ( $lfile, $mylfile );
	} else { 
	    rename ( $lfile, $mylfile );
	}
    }
}

# make a directory of small files and $sfile2
sub mksdir
{
    my $i;
    my $count = 20;
    my $mysfile;
    system ( "mkdir $mysdir" );
    $myssize = stat ($progname)->size;
    for ( $i = $count; $i > 0; $i-- ) {
        $mysfile = $mysdir . '/' . 'sfile' . $i;
	copy ( $progname, $mysfile );
    }
}

# given a sub file path, get the path of the bundle file
sub getBunpathOfSubfile ()
{
    my $subfilepath = shift;
    my $line;
    my @list;
    my @words;
    my $numwords;
    my $dumpFile="/tmp/myDumpFile";

    system  ("ils -L $subfilepath > $dumpFile" );
    @list      = dumpFileContent( $dumpFile );
    unlink( $dumpFile );
# bundle path is in 2nd line
    @words = split( / /, $list[1] );
    $numwords = @words;
# bundle path is in the last entry of the line
    return ( $words[$numwords - 1] );
}

sub printUsage ()
{
    print ("Usage: $0 [help] [debug] [noprompt] [addr1 addr2 addr3]\n");
    print ("  help - Print usage messages.\n");
    print ("  debug - Print debug messages.\n");
    print ("  noprompt -  Assumes iinit was done before running this script and\n");
    print ("    will not ask for password nor path input.\n");
    print ("  addrN - The 3 iRODS server addresses for this test. If they are not\n");
    print ("    defined, the values defined by localHostAddr, host2Addr and host3Addr\n");
    print ("    in this script will be used.\n");
}
