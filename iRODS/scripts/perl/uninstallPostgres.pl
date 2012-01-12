#
# Perl

#
# Uninstall Postgres installed for iRODS.
#
# Usage:
#	perl uninstallPostgres.pl [options]
#
# Options:
# 	--help    List all of the options
#
# This Perl script removes a previous installation of Postgres
# created by the installPostgres.pl script.
#

use File::Spec;
use File::Copy;
use File::Basename;
use Cwd;
use Cwd "abs_path";
use Config;

$version{"uninstallPostgres.pl"} = "March 2010";





########################################################################
#
# Confirm execution from the top-level iRODS directory.
#
$IRODS_HOME = cwd( );	# Might not be actual iRODS home.  Fixed below.

# Where is the configuration directory for iRODS?  This is where
# support scripts are kept.
$configDir = File::Spec->catdir( $IRODS_HOME, "config" );
if ( ! -e $configDir )
{
	# Configuration directory does not exist.  Perhaps this
	# script was run from the scripts or scripts/perl subdirectories.
	# Look up one directory.
	$IRODS_HOME = File::Spec->updir( );
	$configDir  = File::Spec->catdir( $IRODS_HOME, "config" );
	if ( ! -e $configDir )
	{
		$IRODS_HOME = File::Spec->updir( );
		$configDir  = File::Spec->catdir( $IRODS_HOME, "config" );
		if ( ! -e $configDir )
		{
			# Nope.  Complain.
			print( "Usage error:\n" );
			print( "    Please run this script from the top-level directory\n" );
			print( "    of the iRODS distribution.\n" );
			exit( 1 );
		}
	}
}

# Make the $IRODS_HOME path absolute.
$IRODS_HOME = abs_path( $IRODS_HOME );
$configDir  = abs_path( $configDir );





########################################################################
# Initialize.
#

# Get the script name.  We'll use it for some print messages.
my $scriptName = $0;

# Load support scripts.
my $perlScriptsDir = File::Spec->catdir( $IRODS_HOME, "scripts", "perl" );
require File::Spec->catfile( $perlScriptsDir, "utils_paths.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_print.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_file.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_platform.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_config.pl" );

# Get the path to Perl.  We'll use it for running other Perl scripts.
my $perl = $Config{"perlpath"};
if ( !defined( $perl ) || $perl eq "" )
{
	# Not defined.  Find it.
	$perl = findCommand( "perl" );
}

# Determine the execution environment.  These values are used
# later to select different options for different OSes, or to
# print information to the log file or various configuration files.
my $thisOS     = getCurrentOS( );
my $thisUser   = getCurrentUser( );
my $thisUserID = $<;
my $thisHost   = getCurrentHostName( );
%thisHostAddresses = getCurrentHostAddresses( );

# Name a log file.
$logFile  = File::Spec->catfile( $logDir, "uninstallPostgres.log" );

# Postgres installation configuration.
$installPostgresConfig = File::Spec->catfile( $configDir, "installPostgres.config" );





########################################################################
#
# Check script usage.
#
setPrintVerbose( 1 );
$noAsk = 0;

# Don't allow root to run this.
if ( $thisUserID == 0 )
{
	printError( "Usage error:\n" );
	printError( "    This script should *not* be run as root.\n" );
	exit( 1 );
}

foreach $arg ( @ARGV )
{
	# Print help information and quit
	if ( $arg =~ /-?-?h(elp)/ )
	{
		printTitle( "Uninstall Postgres for iRODS\n" );
		printTitle( "------------------------------------------------------------------------\n" );
		printNotice( "This script uninstalls an iRODS Postgres installation.\n" );
		printNotice( "\n" );
		printNotice( "Usage:\n" );
		printNotice( "    uninstallPostgres [options]\n" );
		printNotice( "\n" );
		printNotice( "Help options:\n" );
		printNotice( "    --help      Show this help information\n" );
		printNotice( "\n" );
		printNotice( "Verbosity options:\n" );
		printNotice( "    --quiet     Suppress all messages\n" );
		printNotice( "    --verbose   Output all messages (default)\n" );
		printNotice( "    --noask     Don't ask questions, assume 'yes'\n" );
		exit( 0 );
	}

	# Make further actions quiet
	if ( $arg =~ /-?-?q(uiet)/ )
	{
		setPrintVerbose( 0 );
		next
	}

	# Make further actions verbose
	if ( $arg =~ /-?-?v(erbose)/ )
	{
		setPrintVerbose( 1 );
		next;
	}

	# Suppress questions
	if ( $arg =~ /-?-?noa(sk)/ )
	{
		$noAsk = 1;
		next;
	}

	printError( "Unknown option:  $arg\n" );
	printError( "Use --help for a list of options.\n" );
	exit( 1 );
}





########################################################################
#
# Make sure the user wants to run this script.
#
if ( isPrintVerbose( ) )
{
	printTitle( "Uninstall Postgres for iRODS\n" );
	printTitle( "------------------------------------------------------------------------\n" );

	if ( ! $noAsk )
	{
		printNotice( "This script uninstalls an iRODS Postgres installation previously\n" );
		printNotice( "installed using the 'installPostgres' or 'setup' scripts.\n" );
		printNotice( "\n" );
		printNotice( "    * Do not use this if Postgres is used by other applications. *\n" );
		printNotice( "\n" );
		printNotice( "Uninstalling Postgres stops the servers, removes the iRODS\n" );
		printNotice( "database, and removes the Postgres applications and source.\n" );
		printNotice( "\n" );
		printNotice( "This will destroy the iRODS iCAT database and end access to\n" );
		printNotice( "data stored in iRODS.  This cannot be undone.\n" );
		printNotice( "\n" );
		printNotice( "    Continue yes/no?  " );
		if ( askYesNo( ) == 0 )
		{
			printNotice( "Canceled.\n" );
			exit( 1 );
		}
	}
}





########################################################################
#
# Open and initialize the log file.
#
if ( -e $logFile )
{
	unlink( $logFile );
}
openLog( $logFile );

printLog( "Uninstall Postgres for iRODS\n" );
printLog( "------------------------------------------------------------------------\n" );

# When was this script run?
printLog( getCurrentDateTime( ) . "\n" );

# What's the name of this script?
printLog( "\nScript:\n" );
printLog( "    Script:  $scriptName\n" );
printLog( "    CWD:  " . cwd( ) . "\n" );

# What version of perl?
printLog( "\nPerl:\n" );
printLog( "    Perl path:  $perl\n" );
printLog( "    Perl version:  $]\n" );

# What was the execution environment?
printLog( "\nSystem:\n" );
printLog( "    OS:  $thisOS\n" );
printLog( "    User:  $thisUser\n" );
printLog( "    User ID:  $thisUserID\n" );
printLog( "    Host name:  $thisHost\n" );
printLog( "    Host addresses:  " );
my $ip;
foreach $ip (keys %thisHostAddresses )
{
	printLog( "$ip " );
}
printLog( "\n" );

# What are the environment variables?
printLog( "\nEnvironment:  \n" );
my $env;
foreach $env (keys %ENV)
{
	printLog( "    " . $env . " = " . $ENV{$env} . "\n" );
}

# What versions of iRODS scripts are we using?
printLog( "\n" );
printLog( "Script versions:\n" );
foreach $ver (keys %version)
{
	printLog( "    $ver = " . $version{$ver} . "\n" );
}





########################################################################
#
# Load the Postgres configuration choices.
#
#	If there isn't a configuration file, then Postgres was not installed
#	by the iRODS installation script.  Abort.
#
if ( ! -e $installPostgresConfig )
{
	printError( "Uninstall problem:\n" );
	printError( "    The Postgres installation configuration file is missing.\n" );
	printError( "        File:  $installPostgresConfig\n" );
	printError( "    This probably means that Postgres was not installed previously\n" );
	printError( "    using the iRODS install scripts.  You'll need to uninstall\n" );
	printError( "    Postgres by-hand.\n" );
	printError( "\n" );
	printError( "Abort.\n" );
	printLog( "Abort:  Could not find config file:  $installPostgresConfig.\n" );
	closeLog( );
	exit( 1 );
}

# Set empty defaults.  If the config file doesn't override them,
# then these directories don't exist and there was no prior install.
$POSTGRES_SRC_DIR = "";
$POSTGRES_HOME    = "";

# Load the file, overriding prior default configurations.
require $installPostgresConfig;

if ( !defined( $POSTGRES_HOME ) || ! -e $POSTGRES_HOME )
{
	printError( "Uninstall problem:\n" );
	printError( "    The Postgres database directory does not exist.  This can mean that\n" );
	printError( "    Postgres was not installed using this installation of iRODS, or that\n" );
	printError( "    it has already been uninstalled.\n" );
	printError( "        Config file:  $installPostgresConfig\n" );
	printError( "\n" );
	printError( "Abort.\n" );
	printLog( "Abort:  Could not find directory:  $POSTGRES_HOME.\n" );
	closeLog( );
	exit( 1 );
}






########################################################################
#
# Check the user's configuration.
#
if ( ! File::Spec->file_name_is_absolute( $POSTGRES_SRC_DIR ) )
{
	$POSTGRES_SRC_DIR = File::Spec->catfile( $IRODS_HOME, $POSTGRES_SRC_DIR );
}
if ( ! File::Spec->file_name_is_absolute( $POSTGRES_HOME ) )
{
	$POSTGRES_HOME = File::Spec->catfile( $IRODS_HOME, $POSTGRES_HOME );
}





########################################################################
#
# Uninstall.
#

# Stop iRODS and Postgres.  Ignore errors for the moment.
printSubtitle( "\nStopping servers...\n" );
printLog( "\nStopping servers...\n" );
my $output = `$irodsctl stop 2>&1`;
my $status = $?;
if ( $status != 0 )
{
	# Can't stop servers.  They may already be stopped.
	# No need to warn the user.
	printLog( "Unable to stop iRODS and Postgres servers:\n" );
	printLog( "    Exit code:  $status\n" );
	printLog( "    Output:     $output\n" );
	printLog( "Trying to continue anyway.\n" );
}

# Is Postgres running?  Shouldn't be.  If it is, something
# fatal went wrong above and we really can't continue.
my @pids = getProcessIds( "[ \/\\\\](postgres)" );
if ( $#pids >= 0 )
{
	printError( "Uninstall problem:\n" );
	printError( "    The Postgres server seems to be running still.\n" );
	printError( "    Uninstallation cannot continue until it is stopped.\n" );
	printError( "Abort.\n" );
	printLog( "Abort.  Postgres server still running.\n" );
	closeLog( );
	exit( 1 );
}


# Remove the installed Postgres.
printSubtitle( "Removing installed Postgres...\n" );
printLog( "Removing installed Postgres...\n" );
if ( -e $POSTGRES_HOME )
{
	$output = `rm -rf $POSTGRES_HOME 2>&1`;
	if ( $? != 0 )
	{
		printError( "Cannot fully delete Postgres installation directory:\n" );
		printError( "    $POSTGRES_HOME\n" );
		printError( "$output\n" );
		printError( "Trying to continue anyway.\n" );
		printLog( "Cannot fully delete Postgres installation directory:\n" );
		printLog( "    $POSTGRES_HOME\n" );
		printLog( "$output\n" );
		printLog( "Trying to continue anyway.\n" );
	}
}
else
{
	printStatus( "    Skipped.  Already removed.\n" );
	printLog( "Skipped.  Already removed.\n" );
}


# Remove the Postgres source.
printSubtitle( "Removing Postgres source...\n" );
printLog( "Removing Postgres source...\n" );
if ( -e $POSTGRES_SRC_DIR )
{
	$output = `rm -rf $POSTGRES_SRC_DIR 2>&1`;
	if ( $? != 0 )
	{
		printError( "Cannot fully delete Postgres source directory:\n" );
		printError( "    $POSTGRES_SRC_DIR\n" );
		printError( "$output\n" );
		printError( "Trying to continue anyway.\n" );
		printLog( "Cannot fully delete Postgres source directory:\n" );
		printLog( "    $POSTGRES_SRC_DIR\n" );
		printLog( "$output\n" );
		printLog( "Trying to continue anyway.\n" );
	}
}
else
{
	printStatus( "    Skipped.  Already removed.\n" );
	printLog( "Skipped.  Already removed.\n" );
}


printSubtitle( "Done.\n" );
printLog( "Done.\n" );
closeLog( );

exit( 0 );
