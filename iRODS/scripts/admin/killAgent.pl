#!/usr/bin/perl

## Copyright (c) 2007 Data Intensive Cyberinfrastructure Foundation. All rights reserved.
## For full copyright notice please refer to files in the COPYRIGHT directory
## Written by Jean-Yves Nief of CCIN2P3 and copyright assigned to Data Intensive Cyberinfrastructure Foundation

##
## This script kills irodsAgent process older than a certain wall clock time (default: 7 days).
## This is useful to clean up irodsAgent which are idle since a long time (eg: dead or interrupted client...).
## Only the irodsAgent spawned by a set of clients (icommands, fuse clients...) will be killed.
## Therefore, all the irodsAgent which are older than a certain time will be killed by default.
## If one wishes to have some exceptions for irodsAgent spawned by certain icommands etc..., the 
## list of clients has to be specified in the config file scripts/admin/killAgent.config. The 
## corresponding irodsAgent won't be killed even if they are older than the time specified in
## the config file mentionned above.
## Usage: ./killAgent.pl
## Option: --dry-run : does not do anything, just print what should be deleted based on the criteria in the
##                    config file.
##         --help : this help.

use File::Basename;
use Getopt::Long;

&GetOptions ("--help" => \$optHelp,"--dry-run" => \$optDryRun);
if( $optHelp ) {
	print_help(); # display help and close if -help option is mentionned
	exit;
}

$maxHours = 7 * 24; # default: will kill process older than 7 days
@listException = ""; # default: no exception.
$ldir = dirname($0);

$cfgfile = "$ldir/killAgent.config";
open CFGFILE, $cfgfile or die "impossible to open $cfgfile \n";
@lines= <CFGFILE>;
foreach $line (@lines) {
		if ( substr($line, 0, 1) ne "#" ) {
			chomp $line;
			($confVar, $value) = split /=/, $line;
			if ( $confVar eq "maxHours" ) {
				$maxHours = $value;
			}
			if ( $confVar eq "listException" ) {
				@listException = split /,/, $value;
			}
		}
}
close CFGFILE;

$host = `hostname`;
$irodsClientDir = "$ldir/../../clients/icommands/bin";
$irodsEnvFile = "$ENV{'HOME'}/.irods/.irodsEnv"; # default location for the connection params
# check if an other .irodsEnv is used for this particular installation
$irodsctlFile = "$ldir/../perl/irodsctl.pl";
open IRODSCTLFILE, $irodsctlFile or die "impossible to open $irodsctlFile \n";
@lines= <IRODSCTLFILE>;
foreach $line (@lines) {
	if ( substr($line, 0, 1) ne "#" && $line =~ "irodsEnvFile" && $line !~ "if ") {
		chomp $line;
		@splitStr = split(/[=;"]/, $line);
		foreach $str(@splitStr) {
			if ( $str ne " " && $str ne "irodsEnvFile" ) {
				$irodsEnvFile = $str;
			}
		}
	}
}
#
@psOut = `irodsEnvFile=$irodsEnvFile; export irodsEnvFile; $irodsClientDir/ips -H $host`;
$countKilled = 0;
$potenKilled = 0;
foreach $line(@psOut) {
	if ( $line !~ /Server:/ ) {
		$exception = 0;
		foreach $cmd(@listException) {
			if ( $line =~ $cmd ) {
				$exception = 1;
			}
		}
		if ( $exception == 0 ) {
			@spl = split /\s+/, $line;
			$pid = @spl[1];
			($hourElapsed, $garb1) = split /:/, @spl[3];
			if ( $hourElapsed >= $maxHours ) {
				if ( $optDryRun ) {
					$potenKilled += 1;
					print "Dry run: irodsAgent with pid $pid could be killed.\n";
				}
				else {
					$countKilled += 1;
					$killcom = "kill -9 $pid";
					$rc = system($killcom);
					if ( $rc == 0 ) {
						print "irodsAgent with pid $pid has been killed.\n";
					}
				}
			}
		}
	}
}
print "Summary:\n";
print "   $countKilled irodsAgent has(have) been killed.\n";
print "   $potenKilled irodsAgent could have been killed.\n";

###############
# Help option #
###############
sub print_help {
	print "\n";
	print "This script kills irodsAgent older than a certain wall clock time.\n\n";
	print "Options:\n";
	print "1) --dry-run: does not do anything, just print what should be deleted based on the criteria\n";
	print "              in the config file.\n";
	print "2) --help: to obtain this message\n";
	print "\n";
	return (0);
}

