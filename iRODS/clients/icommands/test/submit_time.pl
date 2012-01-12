#!/usr/bin/perl -w
#
# script to get time results from load test of the sql server.
#
# Copyright (c), CCIN2P3
# For more information please refer to files in the COPYRIGHT directory.

use strict;
use Cwd;
use Sys::Hostname;

#-- Declare variables

my $average;
my $cnt;
my $diftime;
my $dir_w;
my $errorfile;
my $errorname;
my $file;
my $host;
my $line;
my @list;
my $maxi;
my $min;
my $mini;
my $os;
my $rc;
my @real;
my $sec;
my $test_number;
my $time;
my @tmpfile;
my $variance;
my @words;

#-- Check OS type

$os           = `uname -s`;
chomp( $os );

#-- Define some

$host         = hostname();
if ( $host =~ '.' ) {
	@words = split( /\./, $host );
	$host  = $words[0];
}

#-- Find current working directory and make consistency path

$dir_w        = cwd();
$errorfile    = $dir_w . "/testError_load_" . $host . "_";

#-- Text on screen and ask number of tests that should run on //

print( "\nPlease, enter the number of tests you have done on parallel: " );
chomp( $test_number = <STDIN> );
if ( ! $test_number or $test_number !~ /[0-9]/ ) {
	print( "\nYou should give number. Please retry.\n\n");
	exit;
} else {
	print( "\n" );
}

undef( @list );

#-- Loop on the number of tests

for ( $cnt=1; $cnt<=$test_number; $cnt++ ) {
	$errorname    = $errorfile    . $cnt . ".error";	
	push( @list, $errorname );
}

#-- Open error file to get time

$cnt = 1;
foreach $file ( @list ) {
	$rc = open( LOG, "< $file" );
	if ( ! $rc ) {
		print( "Unable to open the file $file in read mode.\n" );
		exit;
	}
	@tmpfile = dumpFileContent( $file );

#---- Loop on file content

	foreach $line ( @tmpfile ) {
		chomp( $line );
		if ( $line =~ "ERROR" ) { next; }
		if ( $line =~ "real" ) {
			@words = split( "real", $line );
			$line  = $words[1];
			$line  =~ s/^\s+//;		# remove blanks
			$line  =~ s/\s+$//;		# remove blanks

#------ For SunOS

			if ( $os eq "SunOS" ) {
				if ( $line =~ ":") {
					@words = split( ":", $line );
					$min   = $words[0];
					$sec   = $words[1];
				} else {
					$min   = 0;
					$sec   = $line;
				}
			}

#------ For Linux

			if ( $os eq "Linux" ) {
				@words = split( "s", $line );
				$line  = $words[0];
				if ( $line =~ "m") {
					@words = split( "m", $line );
					$min   = $words[0];
					$sec   = $words[1];
				} else {
					$min   = 0;
					$sec   = $line;
				}
			}

#------ For Aix

			if ( $os eq "AIX" ) {
				@words = split( "s", $line );
				$line  = $words[0];
				if ( $line =~ "m") {
					@words = split( "m", $line );
					$min   = $words[0];
					$sec   = $words[1];
				} else {
					$min   = 0;
					$sec   = $line;
				}
			}

#------ For MacOS X

			if ( $os eq "Darwin" ) {
				@words = split( "s", $line );
				$line  = $words[0];
				if ( $line =~ "m") {
					@words = split( "m", $line );
					$min   = $words[0];
					$sec   = $words[1];
				} else {
					$min   = 0;
					$sec   = $line;
				}
			}

#------ Make time
			
			$time  = $min * 60 + $sec;
			push( @real, $time );
			printf( "Test number %4d: time = %9.3f seconds.\n", $cnt, $time );
			last;
		}
	}
	close( LOG );
	$cnt++;
}

#-- Compute average time

$average =  0;
$maxi    = -1;
$mini    =  999999;
foreach $time ( @real ) {
	$average += $time;
	if ( $time > $maxi ) { $maxi = $time; }
	if ( $time < $mini ) { $mini = $time; }
}
$average = $average / $test_number;
$diftime = $maxi - $mini;

#-- Compute variance

$variance = 0;
if ( $test_number > 1 ) {
	foreach $time ( @real ) { $variance += ( $average - $time)**2; }
	$variance = $variance / ( $test_number - 1 );
	$variance = sqrt( $variance );
}

#-- Print results

printf( "\nAverage: %9.3f +/- %7.3f seconds for a sample of %4d scripts.\n", $average, $variance, $test_number );
printf( "Minimum: %9.3f seconds, maximum: %9.3f seconds, difference: %9.3f seconds.\n", $mini, $maxi, $diftime );
print( "The error is the standard deviation computed as: sqrt( 1/(N-1) * sum( (time(i) - average)**2 ) ), i from 1 to N.\n\n" );

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
