#
# Perl

#
# Download, install, and configure  Postgres for iRODS.  This
# is the first step of a new iRODS installation.
#
# Usage:
#	perl installPostgres.pl [options]
#
# Options:
# 	--help      Show a list of options
#
# Verbosity options:
#       --quiet		Suppress all messages
#       --verbose	Output all messages (default)
#	--noask		Don't prompt
#	--noheader	Don't print the header message
#	--indent	Indent messages
#
# Other options:
#       --forcedownload	Force redownloading Postgres
#       --forceinit	Force initializing Postgres
#       --force		Same as --forcedownload --forceinit
#
#
# This Perl script automates the process of downloading a version
# of Postgres and ODBC source and compiling them for use by iRODS.
#
# You do not need this script if you are building Postgres yourself
# or using an existing Postgres installation.
#
#	** There are no user-editable parameters in this file. **
#

use File::Spec;
use File::Copy;
use File::Basename;
use Cwd;
use Cwd "abs_path";
use Config;

$version{"installPostgres.pl"} = "September 2011";






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
# Initialize
#

# Get the script name.  We'll use it for some print messages.
my $scriptName = $0;

# Load support scripts.
my $perlScriptsDir = File::Spec->catdir( $IRODS_HOME, "scripts", "perl" );
require File::Spec->catfile( $perlScriptsDir, "utils_paths.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_print.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_file.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_platform.pl" );

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
$thisOS   = getCurrentOS( );
$thisUser = getCurrentUser( );
$thisUserID = $<;
$thisHost = getCurrentHostName( );
%thisHostAddresses = getCurrentHostAddresses( );
$thisDate = getCurrentDateTime( );
$thisDate =~ s/ /_/g;

# Name a log file.
$logFile    = File::Spec->catfile( $logDir, "installPostgres.log" );

# Where are important configuration files?
$installPostgresConfig = File::Spec->catfile( $configDir, "installPostgres.config" );
$irodsConfig           = File::Spec->catfile( $configDir,  "irods.config" );

# Set the number of seconds to sleep after starting or stopping the
# database server.  This gives it time to start or stop before
# we do anything more.  The actual number here is a guess.  Different
# CPU speeds, host loads, or database versions may make this too little
# or too much.
$databaseStartStopDelay = 4;

# Set the number of installation steps and the starting step.
# The current step increases after each major part of the
# install.  These are used as a progress indicator for the
# user, but otherwise have no meaning.
$totalSteps  = 4;
$currentStep = 0;





########################################################################
#
# Check script usage.
#
setPrintVerbose( 1 );

# Don't allow root to run this.
if ( $thisUserID == 0 )
{
	printError( "Usage error:\n" );
	printError( "    This script should *not* be run as root.\n" );
	printError( "    Postgres will not install as root.\n" );
	exit( 1 );
}


# Initialize global flags.
$forcedownload = 0;
$forceinit = 0;
$noAsk = 0;
$noHeader = 0;
$commandLinePassword = "";


# Parse the command-line.
my $arg;
foreach $arg ( @ARGV )
{
	if ( $arg =~ /^-?-?h(elp)$/i )		# Help / usage
	{
		printUsage( );
		exit( 0 );
	}

	if ( $arg =~ /^-?-?q(uiet)$/i )		# Suppress output
	{
		setPrintVerbose( 0 );
		next
	}

	if ( $arg =~ /^-?-?v(erbose)$/i )	# Enable output
	{
		setPrintVerbose( 1 );
		next;
	}

	if ( $arg =~ /^-?-?noa(sk)$/i )		# Suppress questions
	{
		$noAsk = 1;
		next;
	}

	if ( $arg =~ /^-?-?forcedownload$/i )	# Force download
	{
		$forcedownload = 1;
		next;
	}

	if ( $arg =~ /^-?-?forceinit$/i )	# Force database init
	{
		$forceinit = 1;
		next;
	}

	if ( $arg =~ /^-?-?force$/i )		# Force all actions
	{
		$forcedownload = 1;
		$forceinit = 1;
		next;
	}

	if ( $arg =~ /^-?-?indent$/i )		# Force indent
	{
		setMasterIndent( "        " );
		next;
	}

	if ( $arg =~ /^-?-?noheader$/i )	# Suppress the header output
	{
		$noHeader = 1;
		next;
	}

	# This is an intentionally undocumented feature that may go away.
	# It is no longer required.
	if ( $arg =~ /^-?-?password$/i )	# Provide database password
	{
		$commandLinePassword = pop( @ARGV );
		next;
	}

	printError( "Unknown option:  $arg\n" );
	printUsage( );
	exit( 1 );
}





########################################################################
#
# Make sure the user wants to run this script.
#

if ( ! $noHeader )
{
	printTitle( "Install Postgres and ODBC for iRODS\n" );
	printTitle( "------------------------------------------------------------------------\n" );

	if ( isPrintVerbose( ) && ! $noAsk )
	{
		printNotice(
			"This script downloads, builds, and installs Postgres and ODBC drivers\n",
			"for iRODS.  It does not require administrator privileges.\n",
			"\n",
			"This could take up to ** 1/2 hour ** to run, depending upon your network\n",
			"connection and computer speed.\n",
			"\n" );
		if ( askYesNo( "    Continue (yes/no)?  " ) == 0 )
		{
			printError( "Canceled.\n" );
			exit( 1 );
		}
	}
}





########################################################################
#
# Open and initialize the log file.
#
openLog( $logFile );

# Because the log file contains account names and passwords, we change
# permissions on it so that only the current user can read it.
chmod( 0600, $logFile );

printLog( "Install Postgres and ODBC for iRODS\n" );
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
# Set defaults for configuration variables.  All can be overridden
# in the installPostgres.config file.
#
# Postgres FTP site access
$POSTGRES_FTP_ACCOUNT_NAME     = "anonymous";
$POSTGRES_FTP_ACCOUNT_PASSWORD = "anonymous@";
$POSTGRES_FTP_POSTGRES_DIR     = "pub/PostgreSQL/source";
$POSTGRES_FTP_ODBC_DIR         = "PostgreSQL/odbc/versions/src";
$POSTGRES_FTP_HOST             = "ftp5.us.postgresql.org";
#$POSTGRES_FTP_POSTGRES_DIR     = "pub/postgresql/source";
#$POSTGRES_FTP_ODBC_DIR         = "postgresql/odbc/versions/src";
#$POSTGRES_FTP_HOST             = "ftp10.us.postgresql.org";


# UNIX ODBC FTP site access
$UNIXODBC_FTP_ACCOUNT_NAME     = "anonymous";
$UNIXODBC_FTP_ACCOUNT_PASSWORD = "anonymous@";
$UNIXODBC_FTP_ODBC_DIR         = "pub/unixODBC";
$UNIXODBC_FTP_HOST             = "ftp.unixodbc.org";


# User-chosen directories
#	For legacy reasons, the default POSTGRES_SRC_DIR is ../iRodsPostgres
#	relative to the iRODS home directory.
$startDir = cwd( );
$DOWNLOAD_DIR     = File::Spec->tmpdir( );
$POSTGRES_SRC_DIR = File::Spec->catdir( $IRODS_HOME, "..", "iRodsPostgres" );
$POSTGRES_HOME    = File::Spec->catdir( $POSTGRES_SRC_DIR, "pgsql" );


# Postgres
$POSTGRES_PORT = "5432";
$POSTGRES_ADMIN_NAME = $thisUser;
$POSTGRES_ADMIN_PASSWORD = "";		# When empty, will prompt for it


# Other
$POSTGRES_FORCE_INIT = 0;





########################################################################
#
# Load the Postgres configuration choices.
#
# If there isn't a configuration file yet, create one.
$status = copyTemplateIfNeeded( $installPostgresConfig );
if ( $status == 0 )
{
	printError( "\nConfiguration problem:\n" );
	printError( "    Cannot copy the Postgres configuration template file:\n" );
	printError( "        File:  $installPostgresConfig.template\n" );
	printError( "    Permissions problem?  Missing file?\n" );
	printError( "\n" );
	printError( "Abort.  Please re-run this script when the problem is fixed.\n" );
	printLog( "Abort:  Could not copy $installPostgresConfig.template.\n" );
	closeLog( );
	exit( 1 );
}
if ( $status == 2 )
{
	printLog( "Default Postgres installation configuration file created:\n" );
	printLog( "    File:  $installPostgresConfig\n" );

	if ( isPrintVerbose( ) )
	{
		printNotice( "\nA default configuration file has been created for you:\n" );
		printNotice( "    File:  $installPostgresConfig\n" );
		printNotice( "\n" );
		if ( ! $noAsk )
		{
			printNotice( "The default configuration is what most people use, but you can stop now\n" );
			printNotice( "and edit the file if you like.  Re-run this script to continue from\n" );
			printNotice( "where you left off.\n" );
			printNotice( "\n" );
			if ( askYesNo( "    Continue without editing the file (yes/no)?  " ) == 0 )
			{
				printError( "\n" );
				printError( "Abort.  Please re-run this script after editing the configuration file.\n" );
				printLog( "Abort:  User wishes to edit default configuration file.\n" );
				closeLog( );
				exit( 1 );
			}
		}
	}
}

# Load the file, overriding prior default configurations.
require $installPostgresConfig;

if ( $POSTGRES_FORCE_INIT )
{
	$forceinit = 1;
}





########################################################################
#
# Check the user's configuration.
#
# While we have set defaults before importing the configuration file,
# that file could have set variables to bad values, or even undefined
# the variables.  So, we need to check everything before continuing.
#

# Postgres source to download
if ( ! defined( $POSTGRES_SOURCE ) || $POSTGRES_SOURCE eq "" )
{
	printError( "\nConfiguration error:\n" );
	printError( "    The installation configuration file does not set the name of the Postgres\n" );
	printError( "    source file to download and install.  Please edit the file and set\n" );
	printError( "    the \$POSTGRES_SOURCE value.\n" );
	printError( "        File:  $installPostgresConfig\n" );
	printError( "\n" );
	printError( "Abort.  Please re-run this script after updating the configuration file.\n" );
	printLog( "Abort.  Postgres configuration file did not set \$POSTGRES_SOURCE\n" );

	closeLog( );
	exit( 1 );
}

# ODBC source to download
if ( ! defined( $ODBC_SOURCE ) || $ODBC_SOURCE eq "" )
{
	printError( "\nConfiguration error:\n" );
	printError( "    The installation configuration file does not set the name of the Postgres\n" );
	printError( "    ODBC source file to download and install.  Please edit the file and set\n" );
	printError( "    the \$ODBC_SOURCE value.\n" );
	printError( "        File:  $installPostgresConfig\n" );
	printError( "\n" );
	printError( "Abort.  Please re-run this script after updating the configuration file.\n" );
	printLog( "Abort.  Postgres configuration file did not set \$ODBC_SOURCE\n" );

	closeLog( );
	exit( 1 );
}

# Is the host 64-bit?  If so, is the UNIX ODBC download file chosen?
if ( $ODBC_SOURCE =~ /unixODBC/ )
{
	$unixODBC = 1;
}
else
{
	# Postgres ODBC
	$unixODBC = 0;
}
if ( is64bit( ) && !$unixODBC )
{
	printError( "\nConfiguration error:\n" );
	printError( "    This host uses 64-bit addressing.  You will need to use the UNIX ODBC\n" );
	printError( "    library instead of the Postgres ODBC library.  To make this change, edit\n" );
	printError( "    the installation configuration file and set \$ODBC_SOURCE to the name\n" );
	printError( "     of a UNIX ODBC distribution, such as 'unixODBC-2.2.12.tar.gz'.\n" );
	printError( "        File:  $installPostgresConfig\n" );
	printError( "\n" );
	printError( "Abort.  Please re-run this script after updating the configuration file.\n" );
	printLog( "Abort.  Host uses 64-bit addressing but \$ODBC_SOURCE isn't a UNIX ODBC.\n" );
	closeLog( );
	exit( 1 );
}

# Postgres host
if ( ! defined( $POSTGRES_HOST ) || $POSTGRES_HOST eq "" )
{
	$POSTGRES_HOST = $thisHost;
}
if ( defined( $LOCAL_HOST ) && $LOCAL_HOST ne "" )
{
	$POSTGRES_HOST = "localhost";
	$thisHost = "localhost";
}

# Postgres port
if ( ! defined( $POSTGRES_PORT ) || $POSTGRES_PORT eq "" )
{
	$POSTGRES_PORT = 5432;
}

# Postgres source directory
if ( !defined( $POSTGRES_SRC_DIR ) || $POSTGRES_SRC_DIR eq "" )
{
	printError( "\nConfiguration problem:\n" );
	printError( "    The installation configuration file does not set a directory into which\n" );
	printError( "    to place Postgres source.  Please edit the file and set \$POSTGRES_SRC_DIR.\n" );

	printError( "        File:  $installPostgresConfig\n" );
	printError( "\n" );
	printError( "Abort.  Please re-run this script after updating the configuration file.\n" );
	printLog( "Abort.  Postgres configuration file did not set \$POSTGRES_SRC_DIR\n" );
	closeLog( );
	exit( 1 );
}
if ( ! File::Spec->file_name_is_absolute( $POSTGRES_SRC_DIR ) )
{
	# Make it a full path
	$POSTGRES_SRC_DIR = File::Spec->catfile( $IRODS_HOME, $POSTGRES_SRC_DIR );
}

# Postgres install directory
if ( !defined( $POSTGRES_HOME ) || $POSTGRES_HOME eq "" )
{
	printError( "\nConfiguration problem:\n" );
	printError( "    The installation configuration file does not set a directory into which\n" );
	printError( "    to install Postgres.  Please edit the file and set \$POSTGRES_HOME.\n" );
	printError( "        File:  $installPostgresConfig\n" );
	printError( "\n" );
	printError( "Abort.  Please re-run this script after updating the configuration file.\n" );
	printLog( "Abort.  Postgres configuration file did not set \$POSTGRES_HOME\n" );
	closeLog( );
	exit( 1 );
}
if ( ! File::Spec->file_name_is_absolute( $POSTGRES_HOME ) )
{
	# Make it a full path
	$POSTGRES_HOME = File::Spec->catfile( $IRODS_HOME, $POSTGRES_HOME );
}

# Postgres administrator account name.
if ( !defined( $POSTGRES_ADMIN_NAME ) || $POSTGRES_ADMIN_NAME eq "" )
{
	# Default the database admin name to the user's login.
	$POSTGRES_ADMIN_NAME = $thisUser;
}

# Postgres administrator account password.
if ( $commandLinePassword ne "" )
{
	# A Password was given on the command-line.  It overrides
	# anything in the configuration file.
	$POSTGRES_ADMIN_PASSWORD = $commandLinePassword;
}
elsif ( !defined( $POSTGRES_ADMIN_PASSWORD ) || $POSTGRES_ADMIN_PASSWORD eq "" )
{
	# There was no password on the command-line or in
	# the configuration file.
	$POSTGRES_ADMIN_PASSWORD = "";

	# Old configurations used ADMIN_PW or DB_PASSWORD.
	# Are they available?
	if ( defined( $ADMIN_PW ) && $ADMIN_PW ne "" )
	{
		$POSTGRES_ADMIN_PASSWORD = $ADMIN_PW;
	}
	elsif ( defined( $DB_PASSWORD ) && $DB_PASSWORD ne "" )
	{
		$POSTGRES_ADMIN_PASSWORD = $DB_PASSWORD;
	}
	elsif ( isPrintVerbose( ) && ! $noAsk )
	{
		# If we can ask the user, get a password.
		printNotice( "\nFor security reasons, the Postgres account used by iRODS needs a password.\n" );
		printNotice( "\n" );

		# Prompt for a password and make sure it has no special characters
		while ( 1 )
		{
			$POSTGRES_ADMIN_PASSWORD = askString(
				"    Please enter a new password: ",
				"Sorry, but the account password cannot be left empty.\n" );
			if ( $POSTGRES_ADMIN_PASSWORD =~ /[^a-zA-Z0-9_\-\.]/ )
			{
				printError( "Sorry, but the account password can only include the characters\n",
					"a-Z A-Z 0-9, dash, period, and underscore.  Example:  rods.\n" );
				next;
			}

			my $confirm = askString(
				"    Please confirm the new password: ",
				"Sorry, but the account password cannot be left empty.\n" );
			last if ( $confirm eq $POSTGRES_ADMIN_PASSWORD );
			printError( "The passwords didn't match.  Please try again.\n" );
		}
	}

	# The password may still be empty if legacy variables
	# didn't have one or we couldn't ask the user for one.
	# Report the problem.
	if ( $POSTGRES_ADMIN_PASSWORD eq "" )
	{
		printError( "\nConfiguration problem:\n" );
		printError( "    For security reasons, the Postgres account used by iRODS needs\n" );
		printError( "    a password.  You can set this password in the installation\n" );
		printError( "    configuration file by setting \$POSTGRES_ADMIN_PASSWORD.\n" );
		printError( "        File:  $installPostgresConfig\n" );
		printError( "\n" );
		printError( "Abort.  Please re-run this script after updating the configuration file.\n" );
		printLog( "Abort.  Postgres configuration file did not set \$POSTGRES_ADMIN_PASSWORD\n" );
		closeLog( );
		exit( 1 );
	}
}





########################################################################
#
# Make sure this host has needed commands.
#
my $ccPath = getCurrentCC( );
if ( !defined( $ccPath ) )
{
	printError( "\nSystem problem:\n" );
	printError( "    Your host does not appear to have a C compiler\n" );
	printError( "    installed.  This is essential.\n" );
	if ( $thisOS =~ /Darwin/i )	# Mac OS X
	{
		printError( "\n",
			"    Please use your most recent Mac OS X installation DVD\n",
			"    and install the optional development tools.\n" );
	}
	elsif ( $thisOS =~ /Linux/i )
	{
		printError( "\n",
			"    Please use the Linux system management tools to download\n",
			"    and install the gcc, make, and related development tools.\n" );
	}
	printError( "\nAbort.  Please re-run this script after installing the development tools.\n" );
	printLog( "Abort.  Cannot find gcc or cc.\n" );
	closeLog( );
	exit( 1 );
}

$makePath = getCurrentMake( );
if ( !defined( $makePath ) )
{
	printError( "\nSystem problem:\n" );
	printError( "    Your host does not appear to have make or gmake\n" );
	printError( "    installed.  This is essential.\n" );
	if ( $thisOS =~ /Darwin/i )	# Mac OS X
	{
		printError( "\n",
			"    Please use your most recent Mac OS X installation DVD\n",
			"    and install the optional development tools.\n" );
	}
	elsif ( $thisOS =~ /Linux/i )
	{
		printError( "\n",
			"    Please use the Linux system management tools to download\n",
			"    and install the gcc, make, and related development tools.\n" );
	}
	printError( "\nAbort.  Please re-run this script after installing the development tools.\n" );
	printLog( "Abort.  Cannot find make or gmake.\n" );
	closeLog( );
	exit( 1 );
}





# Make the download directory if it doesn't already exist.
########################################################################
#
# Make the directories.
#
# Make the download directory if it doesn't already exist.
if ( ! -e $DOWNLOAD_DIR && mkdir( $DOWNLOAD_DIR, 0700 ) == 0 )
{
	printError( "\nInstall problem:\n" );
	printError( "    The download directory could not be created.  Permissions problem?\n" );
	printError( "        Directory:  $DOWNLOAD_DIR\n" );
	printError( "        Error:      $!\n" );
	printError( "\n" );
	printError( "Abort.  Please re-run this script when the problem is fixed.\n" );
	printLog( "Abort.  Cannot create the download directory.\n" );
	printLog( "    Directory:  $DOWNLOAD_DIR\n" );
	printLog( "    mkdir error:  $!\n" );
	closeLog( );
	exit( 1 );
}

# Make the Postgres working directory if it doesn't already exist.
if ( ! -e $POSTGRES_SRC_DIR && mkdir( $POSTGRES_SRC_DIR, 0700 ) == 0 )
{
	printError( "\nInstall problem:\n" );
	printError( "    The Postgres source directory could not be created.  Permissions problem?\n" );
	printError( "        Directory:  $POSTGRES_SRC_DIR\n" );
	printError( "        Error:      $!\n" );
	printError( "\n" );
	printError( "Abort.  Please re-run this script when the problem is fixed.\n" );
	printLog( "Abort.  Cannot create the Postgres source directory.\n" );
	printLog( "    Directory:  $POSTGRES_SRC_DIR\n" );
	printLog( "    mkdir error:  $!\n" );
	closeLog( );
	exit( 1 );
}

# Make the Postgres install directory if it doesn't already exist.
if ( ! -e $POSTGRES_HOME && mkdir( $POSTGRES_HOME, 0700 ) == 0 )
{
	printError( "\nInstall problem:\n" );
	printError( "    The Postgres install directory could not be created.  Permissions problem?\n" );
	printError( "        Directory:  $POSTGRES_HOME\n" );
	printError( "        Error:      $!\n" );
	printError( "\n" );
	printError( "Abort.  Please re-run this script when the problem is fixed.\n" );
	printLog( "Abort.  Cannot create the Postgres install directory.\n" );
	printLog( "    Directory:  $POSTGRES_HOME\n" );
	printLog( "    mkdir error:  $!\n" );
	closeLog( );
	exit( 1 );
}

# Clean up paths.  Now that the directories exist, paths with ".." can be
# simplified.  (the abs_path() function actually walks the directory path
# to simplify it, so the directories must exist first)
$DOWNLOAD_DIR     = abs_path( $DOWNLOAD_DIR );
$POSTGRES_SRC_DIR = abs_path( $POSTGRES_SRC_DIR );
$POSTGRES_HOME    = abs_path( $POSTGRES_HOME );

# And set important paths.
$postgresBinDir  = File::Spec->catdir( $POSTGRES_HOME, "bin" );
$postgresLibDir  = File::Spec->catdir( $POSTGRES_HOME, "lib" );
$postgresDataDir = File::Spec->catdir( $POSTGRES_HOME, "data" );
$postgresEtcDir  = File::Spec->catdir( $POSTGRES_HOME, "etc" );





########################################################################
#
# Download, compile, install, and setup Postgres.
#

# 1.  Prepare by stopping servers, if any.
prepare( );

# 2.  Download and compile Postgres.
$source_dir = buildPostgres( );

# 3.  Download and compile ODBC.
if ( $unixODBC )
{
	buildUNIXODBC( $source_dir );
}
else
{
	buildPostgresODBC( $source_dir );
}

# 4.  Configure Postgres.
setupPostgresEnvironment( );
setupPostgres( );

# 5.  Save the configuration for iRODS.
saveIrodsConfiguration( );

# Done.
printLog( "\nDone.\n" );
closeLog( );
if ( ! $noHeader )
{
	printSubtitle( "\nDone.  Additional detailed information is in the log file:\n" );
	printSubtitle( "    $logFile\n" );
}
exit( 0 );











########################################################################
#
# Install steps
#
#	All of these functions execute individual steps in the
#	database installation.  They all output error messages themselves
#	and may exit the script directly.
#

#
# @brief	Prepare to install.
#
# If Postgres is already running, we cannot or should not install
# another Postgres.  If we find one running, ask the user what
# to do.  They may elect to try and stop Postgres.  If so, try to
# kill the processes.  If that doesn't work, stop.
#
# Messages are output on errors.
#
sub prepare()
{
	++$currentStep;
	printSubtitle( "\nStep $currentStep of $totalSteps:  Preparing to install...\n" );
	printLog( "\nPreparing to install...\n" );
	printLog( "------------------------------------------------------------------------\n" );
	printLog( getCurrentDateTime( ) . "\n\n" );


	my $output;

	# Look for a Postgres process.
	my @pids = getProcessIds( "[ \/\\\\](postgres)" );
	if ( $#pids < 0 )
	{
		# No prior Postgres.  Good.
		printStatus( "Stopping existing Postgres servers...\n" );
		printStatus( "    Skipped.  No servers running.\n" );
		printLog( "No Postgres servers already running.\n" );
		return;
	}

# Check if it's running on the port we want to use
	my $output = `ls  /tmp/.s.PGSQL.*.lock`;
	my $i = index($output, $POSTGRES_PORT);
	if ($i < 0) {
		printStatus( "    Skipped.  No servers running on our port.\n" );
		printLog( "No Postgres servers running on our port.\n" );
		return;
	}

	# Postgres is already running.  That's a problem.  What should we do?
	printError( "\nInstall problem:\n" );
	printError( "    Postgres is already installed and running on this system.\n" );

	if ( isPrintVerbose( ) )
	{
		# If we are being verbose, we can explain the problem.
		printError( "    There are several possible reasons and actions to take:\n" );
		printError( "\n" );
		printError( "    If you're installing iRODS for the first time:\n" );
		printError( "        Postgres is already installed on your system and probably in use\n" );
		printError( "        by something else.  You can share the existing Postgres or install\n" );
		printError( "        a second Postgres for iRODS only.  Please see the iRODS\n" );
		printError( "        documentation for how to set this up.\n" );
		printError( "\n");
		printError( "    If you're updating to a new iRODS:\n" );
		printError( "        You do not need to install Postgres again.  You can re-use the\n" );
		printError( "        the existing installation.  Please see the iRODS documentation\n" );
		printError( "        for how to set this up.\n" );
		printError( "\n");
		printError( "    If you want to run a separate independent Portgres:\n" );
		printError( "        You can install your own Postgres on a different port by\n" );
		printError( "        rerunning this script and specifying an alternative port for\n" );
		printError( "        your Postgres to use.\n");
		printError( "\n");
		printError( "    If a prior Postgres or iRODS installation failed:\n" );
		printError( "        Sorry about that.  A failed installation may have left Postgres\n" );
		printError( "        running.  You can stop Postgres below and continue with this\n" );
		printError( "        installation.\n" );
	}

	if ( ! $noAsk )
	{
		# If we haven't been told to ask no questions, then we
		# can ask the user what to do.
		printError( "\n" );
		printError( "If you're sure that the running Postgres is not in use, you can\n" );
		printError( "stop it now and continue this installation.\n" );

		printError( "\n" );
		if ( askYesNo( "    Stop Postgres (yes/no)?  " ) == 0 )
		{
			printError( "\nCanceled.  Existing Postgres server left running.\n" );
			printError( "\nAbort.  Please re-run this script after stopping Postgres.\n" );
			printLog( "Abort.  User did not want to stop a running Postgres.\n" );
			closeLog( );
			exit( 1 );
		}
		printStatus( "\n" );
	}
	else
	{
		printError( "\n" );
		printLog( "Abort.  Postgres is already running on this system.\n" );
		closeLog( );
		exit( 1 );
	}

	printStatus( "Stopping existing Postgres servers...\n" );
	printLog( "Postgres is running on this host.  User asked to stop it.\n" );
	my $postgresStopped = 0;


	# First try:  See if 'pg_ctl' is where we're going to install Postgres.
	# This can happen if we are re-installing Postgres atop a prior version.
	my $pgctl = File::Spec->catfile( $postgresBinDir, "pg_ctl" );
	printLog( "    Trying pg_ctl in $postgresBinDir...\n" );
	if ( -e $pgctl )
	{
		# Postgres is installed in the same directory we're
		# about to re-install into.
		$output = `$pgctl stop 2>&1`;
		if ( $? == 0 )
		{
			# Success.
			printLog( "        Command succeeded.\n" );

			# But be sure.
			sleep( $databaseStartStopDelay );
			@pids = getProcessIds( "[ \/\\\\](postgres)" );
			if ( $#pids < 0 )
			{
				# Yup.  No more processes.
				$postgresStopped = 1;
			}
			else
			{
				printLog( "        But failed to kill Postgres processes.\n" );
				# Didn't work.  Continue.
			}
		}
		else
		{
			printLog( "        Command failed.\n" );
			printLog( "            ", $output );
			# Didn't work.  Continue.
		}
	}
	else
	{
		printLog( "        Command not found.\n" );
		# Didn't work.  Continue.
	}


	# Second try:  See if 'pg_ctl' is on the user's path somewhere.
	if ( ! $postgresStopped )
	{
		printLog( "    Trying pg_ctl on user's path...\n" );
		$output = `pg_ctl stop 2>&1`;
		if ( $? == 0 )
		{
			# Success.
			printLog( "        Command succeeded.\n" );

			# But be sure.
			sleep( $databaseStartStopDelay );
			@pids = getProcessIds( "[ \/\\\\](postgres)" );
			if ( $#pids < 0 )
			{
				# Yup.  No more processes.
				$postgresStopped = 1;
			}
			else
			{
				printLog( "        But failed to kill Postgres processes.\n" );
				# Didn't work.  Continue.
			}
		}
		else
		{
			printLog( "        Command failed or not found.\n" );
			printLog( "            ", $output );
			# Didn't work.  Continue.
		}
	}


	# Third try:  Try a series of conventional directories and look
	# for 'pg_ctl' in any of them.
	if ( ! $postgresStopped )
	{
		my $pgctl = findCommand( "pg_ctl" );
		if ( defined( $pgctl ) )
		{
			$output = `$pgctl stop 2>&1`;
			if ( $? == 0 )
			{
				# Success.
				printLog( "        Command succeeded.\n" );

				# But be sure.
				sleep( $databaseStartStopDelay );
				@pids = getProcessIds( "[ \/\\\\](postgres)" );
				if ( $#pids < 0 )
				{
					# Yup.  No more processes.
					$postgresStopped = 1;
					last;
				}
				else
				{
					printLog( "        But failed to kill Postgres processes.\n" );
					# Didn't work.  Continue.
				}
			}
		}
	}


	# Fourth try: Since we can't find 'pg_ctl', all we can do is kill
	# the process.
	if ( ! $postgresStopped )
	{
		printLog( "    Trying to kill process...\n" );
		my $somethingKilled = 0;
		foreach $process ( @pids )
		{
			printLog( "        Killing process $process\n" );
			my $killStatus = kill( 'SIGINT', $process );
			if ( $killStatus == 0 )
			{
				$somethingKilled = 1;
			}
		}
		if ( $somethingKilled )
		{
			# But be sure.
			sleep( $databaseStartStopDelay );
			@pids = getProcessIds( "[ \/\\\\](postgres)" );
			if ( $#pids < 0 )
			{
				# Yup.  No more processes.
				$postgresStopped = 1;
			}
			else
			{
				printLog( "        But failed to kill Postgres processes.\n" );
				# Didn't work.  Continue.
			}
		}
	}


	if ( !$postgresStopped )
	{
		# Nothing worked!
		printError( "\nPostgres could not be stopped.  You may not have suffucient\n" );
		printError( "permissions.  Please check with your system administrator.\n" );
		printError( "\n" );
		printError( "Abort.  Please re-run this script after stopping Postgres.\n" );
		printLog( "Abort.  Could not stop a running Postgres.\n" );
		closeLog( );
		exit( 1 );
	}

	printStatus( "Postgres stopped.\n" );
	printLog( "    Postgres stopped.\n" );
}





#
# @brief	Download and build the Postgres source code.
#
# The Postgres source tar file is downloaded from the FTP server,
# placed in the user's download directory, uncompressed, and un-tared
# into the Postgres working directory.  There, it is configured,
# compiled, and installed.
#
# Messages are output on errors.
#
# @return
# 	the new directory containing Postgres source
#
sub buildPostgres
{
	my ($postgres_source_dir) = @_;

	my $status;
	my $output;

	++$currentStep;
	printSubtitle( "\nStep $currentStep of $totalSteps:  Installing Postgres...\n" );
	printLog( "\nInstalling Postgres\n" );
	printLog( "------------------------------------------------------------------------\n" );
	printLog( getCurrentDateTime( ) . "\n\n" );

	# Extract version number.
	my $vers;
	if ( $POSTGRES_SOURCE =~ /([0-9\.]*)\.[^0-9]/ )
	{
		$vers = $1;
	}
	else
	{
		printError( "Configuration problem:\n" );
		printError( "    The \$POSTGRES_SOURCE file name may be wrong.  It does not\n" );
		printError( "    contain a recognizable version number.  Please check the\n" );
		printError( "    $installPostgresConfig.\n" );
		printError( "\n" );
		printError( "Abort.  Please re-run this script when the problem is fixed.\n" );
		printLog( "Abort:  Postgres configure failed.\n" );
		closeLog( );
		exit( 1 );
	}

	printLog( "Downloading Postgres:  $POSTGRES_SOURCE\n" );


	# Download it, if needed.
	my $source_dir = download(
		$POSTGRES_FTP_HOST,
		$POSTGRES_FTP_ACCOUNT_NAME,
		$POSTGRES_FTP_ACCOUNT_PASSWORD,
		$POSTGRES_FTP_POSTGRES_DIR . "/v" . $vers,
		$POSTGRES_SOURCE,
		$POSTGRES_SRC_DIR );
	if ( !defined( $source_dir ) )
	{
		closeLog( );
		exit( 1 );
	}


	# Configure Postgres, if different from the last time, if any.
	#	--prefix
	#		Name the directory into which to install Postgres.
	#
	#	--enable-odbc
	#		Support ODBC access.
	#
	#	--without-readline
	#		Readline provides an editable command-line for
	#		Postgres.  However, it is not available by default
	#		on many OSes, and is optional.  So disable it.
	#
	#	--without-zlib
	#		libz (gzip compress) is nice to have, but optional.
	#		On most OSes, it is available in /usr/lib or
	#		/usr/local/lib.  But Postgres wants a static lib,
	#		and some OSes no longer include one, just dynamic.
	#		Solaris and Ubuntu 7.x, for instance, don't include
	#		static libz.  Ubuntu doesn't have a standard package
	#		to get it.  Since it is optional for Postgres anyway,
	#		disable it for all OSes so we don't have to worrry
	#		about it.
	#
	#	--with-pgport
	#		Set the port # for Postgres.
	#
	my $startingDir = cwd( );
	chdir( $source_dir );

	my $command = "./configure --prefix=$POSTGRES_HOME --enable-odbc --without-readline --without-zlib --with-pgport=$POSTGRES_PORT";

	printStatus( "Configuring...\n" );
	printLog( "\nConfiguring Postgres\n" );
	printLog( "------------------------------------------------------------------------\n" );
	printLog( getCurrentDateTime( ) . "\n\n" );

	my $doConfigure = 1;
	if ( -e "config.status" )
	{
		# The 'config.status' file is created at the end
		# of the configure process, so it only exists if
		# configure was run before and ran successfully.
		# We can skip running configure again if the
		# previous run has the same options as this one.
		if ( -e "config.log" )
		{
			# There is a log.  So, configure was run
			# before.  At the start of the log is
			# the configure line from last time.
			open( CONFIGLOG, "<config.log" );
			foreach $line ( <CONFIGLOG> )
			{
				if ( $line =~ /$command/ )
				{
					# Yes, the previous run
					# used the same command.
					# Skip configuring again.
					$doConfigure = 0;
					last;
				}
			}
			close( CONFIGLOG );
		}
	}

	if ( $doConfigure )
	{
		printLog( "  $command\n" );
		my $output = `$command 2>&1`;
		my $status = $?;
		printLog( $output );

		if ( $status != 0 )
		{
			printError( "Configure problem:\n" );
			printError( "    A problem occurred when configuring Postgres.  Please check the\n" );
			printError( "    install log in $logFile.\n" );
			printError( "\n" );
			printError( "Abort.  Please re-run this script when the problem is fixed.\n" );
			printLog( "Abort:  Postgres configure failed.\n" );
			closeLog( );
			exit( 1 );
		}

		# Clean after changing the configuration.
		printStatus( "Cleaning...\n" );
		printLog( "\nCleaning Postgres\n" );
		printLog( "------------------------------------------------------------------------\n" );
		printLog( getCurrentDateTime( ) . "\n\n" );
		my ($status,$output) = make( "clean" );
		printLog( $output );
		if ( $status != 0 )
		{
			printError( "Cannot clean previously compiled files from Postgres.\n" );
			printError( "Trying to continue anyway.\n" );
			printLog( "Cannot clean previously compiled files from Postgres.\n" );
			printLog( "Trying to continue anyway.\n" );
		}
	}
	else
	{
		printStatus( "    Skipped.  No change in configuration.\n" );
		printLog( "    Skipped.  No change in configuration.\n" );
	}


	# Compile and install.
	printStatus( "Compiling...\n" );
	printLog( "\nCompiling Postgres\n" );
	printLog( "------------------------------------------------------------------------\n" );
	printLog( getCurrentDateTime( ) . "\n\n" );
	($status,$output) = make( "" );
	printLog( $output );
	if ( $status != 0 )
	{
		printError( "\nBuild problem:\n" );
		printError( "    A problem occurred when compiling Postgres.  Please check the log:\n" );
		printError( "        Log:  $logFile\n" );
		printError( "\n" );
		printError( "Abort.  Please re-run this script when the problem is fixed.\n" );
		printLog( "Abort:  Postgres compile failed.\n" );
		closeLog( );
		exit( 1 );
	}

	printStatus( "Installing...\n" );
	printLog( "\nInstalling Postgres\n" );
	printLog( "------------------------------------------------------------------------\n" );
	printLog( getCurrentDateTime( ) . "\n\n" );
	($status,$output) = make( "install" );
	printLog( $output );
	if ( $status != 0 )
	{
		printError( "\nInstall problem:\n" );
		printError( "    A problem occurred when installing Postgres.  Please check the log:\n" );
		printError( "        Log:  $logFile\n" );
		printError( "\n" );
		printError( "Abort.  Please re-run this script when the problem is fixed.\n" );
		printLog( "Abort:  Postgres install failed.\n" );
		closeLog( );
		exit( 1 );
	}

	chdir( $startingDir );


	# Create an etc directory for configuration files
	if ( ! -e $postgresEtcDir )
	{
		mkdir( $postgresEtcDir, 0766 );
	}


	return $source_dir;
}





#
# @brief	Download and build the Postgres ODBC source code.
#
# The Postgres ODBC source tar file is downloaded from the FTP server,
# placed in the user's download directory, uncompressed, and un-tared
# into the Postgres working directory.  There, it is configured,
# compiled, and installed.
#
# Messages are output on errors.
#
# @param	$postgres_source_dir
# 	the Postgres source directory
# @return
# 	the new directory containing Postgres ODBC source
#
sub buildPostgresODBC
{
	my ($postgres_source_dir) = @_;

	my $status;
	my $output;

	++$currentStep;
	printSubtitle( "\nStep $currentStep of $totalSteps:  Installing Postgres ODBC...\n" );
	printLog( "\nInstalling Postgres ODBC\n" );
	printLog( "------------------------------------------------------------------------\n" );
	printLog( "    Downloading Postgres ODBC:  $ODBC_SOURCE\n" );
	printLog( getCurrentDateTime( ) . "\n\n" );


	# Create a directory for the source.
	my $destdir = File::Spec->catdir( $postgres_source_dir, "src", "interfaces", "odbc" );
	if ( ! -e $destdir && mkdir( $destdir, 0700 ) != 1 )
	{
		printError( "Install problem:\n" );
		printError( "    The Postgres ODBC directory $destdir\n" );
		printError( "    does not exist and cannot be created.  Permissions problem?\n" );
		printError( "    Please check the log in $logFile.\n" );
		printError( "\n" );
		printError( "Abort.  Please re-run this script when the problem is fixed.\n" );
		printLog( "Abort:  Cannot create Postgres ODBC directory:  $destdir\n" );
		printLog( "mkdir:  $!\n" );
		closeLog( );
		exit( 1 );
	}


	# Download.
	my $source_dir = download(
		$POSTGRES_FTP_HOST,
		$POSTGRES_FTP_ACCOUNT_NAME,
		$POSTGRES_FTP_ACCOUNT_PASSWORD,
		$POSTGRES_FTP_ODBC_DIR,
		$ODBC_SOURCE,
		$destdir );
	if ( !defined( $source_dir ) )
	{
		closeLog( );
		exit( 1 );
	}


	# Update the ODBC Makefiles
	setupODBCMakefiles( $postgres_source_dir, $source_dir );


	# Configure.
	my $startingDir = cwd( );
	chdir( $source_dir );

	my $command = "./configure --prefix=$POSTGRES_HOME --enable-static --without-unixodbc";
	if ( $thisOS =~ /SunOS/i )
	{
		$command .= " --disable-pthreads";
	}

	printStatus( "Configuring...\n" );
	printLog( "\nConfiguring Postgres ODBC\n" );
	printLog( "------------------------------------------------------------------------\n" );


	my $doConfigure = 1;
	if ( -e "config.status" )
	{
		# The 'config.status' file is created at the end
		# of the configure process, so it only exists if
		# configure was run before and ran successfully.
		# We can skip running configure again if the
		# previous run has the same options as this one.
		if ( -e "config.log" )
		{
			# There is a log.  So, configure was run
			# before.  At the start of the log is
			# the configure line from last time.
			open( CONFIGLOG, "<config.log" );
			foreach $line ( <CONFIGLOG> )
			{
				if ( $line =~ /$command/ )
				{
					# Yes, the previous run
					# used the same command.
					# Skip configuring again.
					$doConfigure = 0;
					last;
				}
			}
			close( CONFIGLOG );
		}
	}

	if ( $doConfigure )
	{
		printLog( "  $command\n" );
		$ENV{"PG_CONFIG"} = File::Spec->catfile( $postgresBinDir, "pg_config" );
		my $output = `$command 2>&1`;
		$ENV{"PG_CONFIG"} = undef;
		my $status = $?;
		printLog( $output );

		if ( $status != 0 )
		{
			printError( "Configure problem:\n" );
			printError( "    A problem occurred when configuring Postgres ODBC.  Please check the\n" );
			printError( "    log in $logFile.\n" );
			printError( "\n" );
			printError( "Abort.  Please re-run this script when the problem is fixed.\n" );
			printLog( "Abort:  Postgres ODBC configure failed.\n" );
			closeLog( );
			exit( 1 );
		}
	}

	# Patch ODBC's Makefile on SunOS to include the needed NSL and SOCKET libraries.
	if ( $thisOS =~ /SunOS/i )
	{
		if ( `grep nsl Makefile` eq "" )
		{
			`sed 's/LIBS =/LIBS =-lnsl -lsocket/g' < Makefile > Makefile2`;
			unlink( "Makefile" );
			move( "Makefile2",  "Makefile" );
		}
	} 


	# Clean after changing the configuration.
	if ( $doConfigure )
	{
		printStatus( "Cleaning...\n" );
		printLog( "\nCleaning Postgres ODBC\n" );
		printLog( "------------------------------------------------------------------------\n" );
		my ($status,$output) = make ( "clean" );
		printLog( $output );
		if ( $stats != 0 )
		{
			printError( "Cannot clean previously compiled files from Postgres ODBC.\n" );
			printError( "Trying to continue anyway.\n" );
			printLog( "Cannot clean previously compiled files from Postgres ODBC.\n" );
			printLog( "Trying to continue anyway.\n" );
		}
	}


	# Compile and install.
	printStatus( "Compiling...\n" );
	printLog( "\nCompiling Postgres ODBC\n" );
	printLog( "------------------------------------------------------------------------\n" );
	($status,$output) = make( "" );
	printLog( $output );
	if ( $status != 0 )
	{
		printError( "Build problem:\n" );
		printError( "    A problem occurred when compiling Postgres ODBC.  Please check the log:\n" );
		printError( "        Log:  $logFile\n" );
		printError( "\n" );
		printError( "Abort.  Please re-run this script when the problem is fixed.\n" );
		printLog( "Abort:  Postgres ODBC compile failed.\n" );
		closeLog( );
		exit( 1 );
	}

	printStatus( "Installing...\n" );
	printLog( "\nInstalling Postgres ODBC\n" );
	printLog( "------------------------------------------------------------------------\n" );
	($status,$output) = make( "install" );
	printLog( $output );
	if ( $status != 0 )
	{
		printError( "Install problem:\n" );
		printError( "    A problem occurred when installing Postgres ODBC.  Please check the log:\n" );
		printError( "        Log:  $logFile\n" );
		printError( "\n" );
		printError( "Abort.  Please re-run this script when the problem is fixed.\n" );
		printLog( "Abort:  Postgres ODBC install failed.\n" );
		closeLog( );
		exit( 1 );
	}


	# Copy library and include files into the Postgres installation.
	printStatus( "Copying files...\n" );
	printLog( "\nCopying files in Postgres ODBC\n" );
	printLog( "------------------------------------------------------------------------\n" );
	my $libdir = File::Spec->catdir( $POSTGRES_HOME, "lib" );
	my $incdir = File::Spec->catdir( $POSTGRES_HOME, "include" );
	my $path   = File::Spec->catfile( $libdir, "psqlodbc.a" );
	if ( -e $path )
	{
		# Rename the library file with 'lib' in front per UNIX
		# conventions, and needed on some platforms.
		my $libPath = File::Spec->catfile( $libdir, "libpsqlodbc.a" );
		if ( ! -e $libPath )
		{
			printStatus( "    Library...\n" );
			printLog( "    Library...\n" );
			copy( $path, $libPath );
		}
	}
	printStatus( "    Include files...\n" );
	printLog( "    Include files...\n" );
	my @incFiles = ( "iodbc.h", "isql.h", "isqlext.h" );
	foreach $inc ( @incFiles )
	{
		my $incPath = File::Spec->catfile( $incdir, $inc );
		if ( ! -e $incPath )
		{
			copy( $inc, $incdir );
		}
	}

	if ( $thisOS =~ /Darwin/i )	# Mac OS X
	{
# TODO:  Fix this
# Thru trial and error, Wayne found that these commands will create a link
# library with no missing externals.  This cp is a hack, but I'm not
# sure the right way to create what we want (it avoids a missing
# external of _globals).  Also, this whole thing of creating a .a file
# from all the .o's via libtool is odd, but the odbc configure/make
# doesn't seem to handle it right.
		printStatus( "    Mac library files...\n" );
		printLog( "    Mac library files...\n" );
		copy( "psqlodbc.lo", "psqlodbc.o" );
		my $output = `libtool -o libpsqlodbc.a *.o 2>&1`;
		if ( $? != 0 )
		{
			printLog( "Libtool errors:  $output\n" );
			printLog( "Trying to continue anyway.\n" );
		}
		copy( "libpsqlodbc.a", $libdir );
		chdir( $libdir );
		$output = `ranlib libpsqlodbc.a 2>&1`;
		if ( $? != 0 )
		{
			printLog( "Ranlib errors:  $output\n" );
			printLog( "Trying to continue anyway.\n" );
		}
	}

	chdir( $startingDir );


	# Set up Postgres
	#	This odbcinst.ini file is slightly incomplete.  It does not include
	#	the 'Database' value because we don't know the name of the
	#	database at this point.  That's done during the final stages
	#	of iRODS installation and is set by a different configuration file.
	#	So, for now, leave that value out.
	printStatus( "Updating ODBC configuration...\n" );
	printLog( "\nUpdating ODBC configuration\n" );
	printLog( "------------------------------------------------------------------------\n" );
	my $ini = File::Spec->catfile( $postgresEtcDir, "odbcinst.ini" );
	if ( -e $ini )
	{
		# Delete the old file.
		unlink( $ini );
	}
	printToFile( $ini,
		"[PostgreSQL]\n" .
		"Debug=0\n" .
		"CommLog=0\n" .
		"Servername=$thisHost\n" .
#		"Database=$DB_NAME\n" .		# To be added during iRODS setup
		"ReadOnly=no\n" .
                "Ksqo=0\n" .
		"Port=$POSTGRES_PORT\n" );

	return $source_dir;
}





#
# @brief	Download and build the UNIX ODBC source code.
#
# The UNIX ODBC source tar file is downloaded from the FTP server,
# placed in the user's download directory, uncompressed, and un-tared
# into the Postgres working directory.  There, it is configured,
# compiled, and installed.
#
# Messages are output on errors.
#
# @param	$postgres_source_dir
# 	the Postgres source directory
# @return
# 	the new directory containing UNIX ODBC source
#
sub buildUNIXODBC
{
	my ($postgres_source_dir) = @_;

	my $status;
	my $output;

	++$currentStep;
	printSubtitle( "\nStep $currentStep of $totalSteps:  Installing UNIX ODBC...\n" );
	printLog( "\nInstalling UNIX ODBC\n" );
	printLog( "------------------------------------------------------------------------\n" );
	printLog( getCurrentDateTime( ) . "\n\n" );
	printLog( "  Downloading UNIX ODBC:  $ODBC_SOURCE\n" );


	# Create a directory for the source.
	my $interfacesdir = File::Spec->catdir( $postgres_source_dir, "src", "interfaces" );
	my $destdir = File::Spec->catdir( $interfacesdir, "odbc" );
	if ( ! -e $destdir && mkdir( $destdir, 0700 ) != 1 )
	{
		printError( "Install problem:\n" );
		printError( "    The Postgres ODBC directory $destdir\n" );
		printError( "    does not exist and cannot be created.  Permissions problem?\n" );
		printError( "    Please check the log in $logFile.\n" );
		printError( "\n" );
		printError( "Abort.  Please re-run this script when the problem is fixed.\n" );
		printLog( "Abort:  Cannot create Postgres ODBC directory:  $destdir\n" );
		printLog( "mkdir:  $!\n" );
		closeLog( );
		exit( 1 );
	}


	# Download.
	my $source_dir = download(
		$UNIXODBC_FTP_HOST,
		$UNIXODBC_FTP_ACCOUNT_NAME,
		$UNIXODBC_FTP_ACCOUNT_PASSWORD,
		$UNIXODBC_FTP_ODBC_DIR,
		$ODBC_SOURCE,
		$destdir );
	if ( !defined( $source_dir ) )
	{
		closeLog( );
		exit( 1 );
	}


	# Update the ODBC Makefiles
	setupODBCMakefiles( $postgres_source_dir, $source_dir );


	# Configure, if needed.
	my $startingDir = cwd( );
	chdir( $source_dir );

	my $command = "./configure --prefix=$POSTGRES_HOME --enable-gui=no --enable-static";
	if ( $thisOS =~ /SunOS/i )
	{
		$command .= " --disable-pthreads";
	}

	printStatus( "Configuring...\n" );
	printLog( "\nConfiguring UNIX ODBC\n" );
	printLog( "------------------------------------------------------------------------\n" );
	printLog( getCurrentDateTime( ) . "\n\n" );


	my $doConfigure = 1;
	if ( -e "config.status" )
	{
		# The 'config.status' file is created at the end
		# of the configure process, so it only exists if
		# configure was run before and ran successfully.
		# We can skip running configure again if the
		# previous run has the same options as this one.
		if ( -e "config.log" )
		{
			# There is a log.  So, configure was run
			# before.  At the start of the log is
			# the configure line from last time.
			open( CONFIGLOG, "<config.log" );
			foreach $line ( <CONFIGLOG> )
			{
				if ( $line =~ /$command/ )
				{
					# Yes, the previous run
					# used the same command.
					# Skip configuring again.
					$doConfigure = 0;
					last;
				}
			}
			close( CONFIGLOG );
		}
	}

	if ( $doConfigure )
	{
		printLog( "  $command\n" );
		my $output = `$command 2>&1`;
		my $status = $?;
		printLog( $output );

		if ( $status != 0 )
		{
			printError( "Configure problem:\n" );
			printError( "    A problem occurred when configuring UNIX ODBC.  Please check the\n" );
			printError( "    log in $logFile.\n" );
			printError( "\n" );
			printError( "Abort.  Please re-run this script when the problem is fixed.\n" );
			printLog( "Abort:  UNIX ODBC configure failed.\n" );
			closeLog( );
			exit( 1 );
		}

		# Clean after the configuration changed
		printStatus( "Cleaning...\n" );
		printLog( "\nCleaning UNIX ODBC\n" );
		printLog( "------------------------------------------------------------------------\n" );
		printLog( getCurrentDateTime( ) . "\n\n" );
		my ($status,$output) = make( "clean" );
		printLog( $output );
		if ( $status != 0 )
		{
			printError( "Cannot clean previously compiled files from UNIX.\n" );
			printError( "Trying to continue anyway.\n" );
			printLog( "Cannot clean previously compiled files from UNIX.\n" );
			printLog( "Trying to continue anyway.\n" );
		}
	}


	# Compile and install.
	printStatus( "Compiling...\n" );
	printLog( "\nCompiling UNIX ODBC\n" );
	printLog( "------------------------------------------------------------------------\n" );
	printLog( getCurrentDateTime( ) . "\n\n" );
	($status,$output) = make( "" );
	printLog( $output );
	if ( $status != 0 )
	{
		printError( "\nBuild problem:\n" );
		printError( "    A problem occurred when compiling UNIX ODBC.  Please check the log:\n" );
		printError( "        Log:  $logFile\n" );
		printError( "\n" );
		printError( "Abort.  Please re-run this script when the problem is fixed.\n" );
		printLog( "Abort:  UNIX ODBC compile failed.\n" );
		closeLog( );
		exit( 1 );
	}

	printStatus( "Installing...\n" );
	printLog( "\nInstalling UNIX ODBC\n" );
	printLog( "------------------------------------------------------------------------\n" );
	printLog( getCurrentDateTime( ) . "\n\n" );
	($status,$output) = make( "install" );
	printLog( $output );
	if ( $status != 0 )
	{
		printError( "\nInstall problem:\n" );
		printError( "    A problem occurred when installing UNIX ODBC.  Please check the log:\n" );
		printError( "        Log:  $logFile\n" );
		printError( "\n" );
		printError( "Abort.  Please re-run this script when the problem is fixed.\n" );
		printLog( "Abort:  UNIX ODBC install failed.\n" );
		closeLog( );
		exit( 1 );
	}

	chdir( $startingDir );


	# Set up Postgres
	#	This odbc.ini file is slightly incomplete.  It does not include
	#	the 'Database' value because we don't know the name of the
	#	database at this point.  That's done during the final stages
	#	of iRODS installation and is set by a different configuration file.
	#	So, for now, leave that value out.
	printStatus( "Updating ODBC configuration...\n" );
	printLog( "\nUpdating ODBC configuration\n" );
	printLog( "------------------------------------------------------------------------\n" );
	printLog( getCurrentDateTime( ) . "\n\n" );
	my $ini = File::Spec->catfile( $postgresEtcDir, "odbc.ini" );
	if ( -e $ini )
	{
		# Delete the old file.
		unlink( $ini );
	}
	my $libPath = File::Spec->catfile( $postgresLibDir, "libodbcpsql.so" );
	printToFile( $ini,
		"[PostgreSQL]\n" .
		"Driver=$libPath\n" .
		"Debug=0\n" .
		"CommLog=0\n" .
		"Servername=$thisHost\n" .
#		"Database=$DB_NAME\n" .		# To be added during iRODS setup
		"ReadOnly=no\n" .
                "Ksqo=0\n" .
		"Port=$POSTGRES_PORT\n" );

	return $source_dir;
}





#
# @brief	Modify and create makefiles to build the ODBC drivers
# 		along with Postgres.
#
# Other functions above download and add the ODBC drivers into the Postgres
# src/interfaces/odbc directory.  This function adds a Makefile to that
# directory to invoke the newly installed driver's Makefile, and modifies
# the parent directory's Makefile to invoke the ODBC one.  This just
# connects the Makefiles into the set that comes with Postgres.
#
# Technically, this is unnecessary since we never invoke the top-level
# Makefile and expect it to trickle down into the ODBC drivers directory.
# But it seems like good form.
#
# Messages are output on errors.
#
# @param	$postgres_source_dir
# 	the Postgres source directory
# @param	$newOdbcDir
# 	the ODBC drivers directory
#
sub setupODBCMakefiles
{
	my ($postgres_source_dir, $newOdbcDir) = @_;


	# Where are the directories?
	my $interfacesDir = File::Spec->catdir( $postgres_source_dir, "src", "interfaces" );
	my $odbcDir       = File::Spec->catdir( $interfacesDir, "odbc" );


	# Postgres comes with a Makefile in the src/interfaces directory
	# that invokes Makefiles in the subdirectories.  Since we added
	# the 'odbc' directory, it doesn't know to invoke anything there.
	# So, modify the MAkefile (if needed) to go there too.
	my $odbcMakefile = File::Spec->catfile( $interfacesDir, "Makefile" );
	if ( -e $odbcMakefile )
	{
		printStatus( "Updating Postgres interfaces Makefile...\n" );
		printLog( "Updating Postgres interfaces Makefile...\n" );

		# Remove the old temp Makefile, if any.
		my $newMakefile = File::Spec->catfile( $interfacesDir, "Makefile2" );
		if ( -e $newMakefile )
		{
			unlink( $newMakefile );
		}

		# Create a new Makefile, copying the original but adding 'odbc'
		# to the directory list.
		my $alreadyDone = 0;
		open( NEWMAKEFILE, ">$newMakefile" );
		open( MAKEFILE, "<$odbcMakefile" );
		foreach $line ( <MAKEFILE> )
		{
			if ( $line =~ /Updated by iRODS/ )
			{
				$alreadyDone = 1;
				last;
			}
			if ( $line =~ /^DIRS\s*:-/ )
			{
				$line =~ s/$/ odbc/;
				print( NEWMAKEFILE "# Updated by iRODS installation script to add ODBC directory\n" );
			}
			print( NEWMAKEFILE $line );
		}
		close( MAKEFILE );
		close( NEWMAKEFILE );

		if ( $alreadyDone )
		{
			# Nevermind.  The Makefile is fine as-is.
			unlink( $newMakefile );
			printStatus( "    Skipped.  Makefile already updated.\n" );
			printLog( "    Skipped.  Makefile already updated.\n" );
		}
		# Move the new file to the old file
		elsif ( move( $newMakefile, $odbcMakefile ) == 0 )
		{
			printError( "Could not update Postgres interfaces Makefile\n" );
			printError( "Trying to continue anyway.\n" );
			printLog( "Could not update Postgres interfaces Makefile\n" );
			printLog( "Trying to continue anyway.\n" );
			unlink( $newMakefile );
		}
	}


	# The ODBC installation creates 'src/interfaces/odbc' and
	# then adds a downloaded ODBC driver in a subdirectory.
	# This leaves a gap in Makefiles.  Fill the gap by adding
	# a pass-through Makefile to the ODBC directory that
	# invokes the driver's Makefile.
	printStatus( "Creating ODBC Makefile...\n" );
	printLog( "Creating ODBC Makefile...\n" );
	my $driverMakefile = File::Spec->catfile( $odbcDir, "Makefile" );
	if ( -e $driverMakefile )
	{
		# If there is a Makefile here, it's probably because a previous
		# run of this script put it there.  That previous run may or
		# may not have been for this ODBC library or version.  To be
		# safe, delete the old Makefile and create a new one.
		unlink( $driverMakefile );
	}

	printToFile( $driverMakefile,
		"# Makefile for ODBC added by iRODS installation scripts\n" .
		"all install installdirs uninstall dep depend distprep clean distclean maintainer-clean:\n" .
		"\t@\$(MAKE) -C $newOdbcDir\n" );
}





#
# @brief	Set Postgres environment
#
# Set the standard Postgres environment variables so that
# we can run the Postgres commands during setup.
#
sub setupPostgresEnvironment
{
	# Location of data directory
	$ENV{'PGDATA'} = $postgresDataDir;

	# Postgres server port
	if ( defined( $POSTGRES_PORT ) && $POSTGRES_PORT ne "" )
	{
		$ENV{"PGPORT"} = $POSTGRES_PORT;
	}

	# Postgres host
	if ( defined( $POSTGRES_HOST ) && $POSTGRES_HOST ne "" &&
		$POSTGRES_HOST ne "localhost" && $POSTGRES_HOST ne $thisHost )
	{
		$ENV{"PGHOST"} = $POSTGRES_HOST;
	}

	# Postgres user
	$ENV{"PGUSER"} = "$POSTGRES_ADMIN_NAME";

	# Execution path
	my $oldPath = $ENV{'PATH'};
	$ENV{'PATH'} = "$postgresBinDir:$oldPath";

	# Library path
	my $libPath = $ENV{'LD_LIBRARY_PATH'};  
	if ( $libPath eq "" )
	{
		$libPath = "$postgresLibDir";
	}
	else
	{
		$libPath = "$postgresLibDir:$libPath";
		
	}
	$libPath = "$libPath:" . File::Spec->catdir( File::Spec->rootdir( ), "usr", "local", "lib" );
	$ENV{'LD_LIBRARY_PATH'} = $libPath;
}





#
# @brief	Set up Postgres, initializing the database etc.
#
# The freshly installed Postgres is set up by initializing its
# database and adjusting its configuration files.
#
# Messages are output on errors.
#
sub setupPostgres
{
	++$currentStep;
	printSubtitle( "\nStep $currentStep of $totalSteps:  Setting up Postgres...\n" );
	printLog( "\nSetting up Postgres\n" );
	printLog( "------------------------------------------------------------------------\n" );
	printLog( getCurrentDateTime( ) . "\n\n" );


	my $psqlconf  = File::Spec->catfile( $postgresDataDir, "postgresql.conf" );
	my $pghbaconf = File::Spec->catfile( $postgresDataDir, "pg_hba.conf" );

	# Initialize the database, if needed.
	my $doInit = 1;
	if ( -e $postgresDataDir )
	{
		# Postgres data directory exists.  But it might be incomplete
		# if a prior install failed.

		# Spot check to see that the expected files and directories
		# are there too.
		my $baseDir = File::Spec->catdir( $postgresDataDir, "base" );
		if ( -e $baseDir && -e $psqlconf && -e $pghbaconf )
		{
			# Yup.  Files and directories are there.
			# Assume we don't need to initdb.
			$doInit = 0;
		}

		if ( $forceinit || $doInit )
		{
			# Either we've been asked to force the install,
			# or above we found that the directory didn't
			# have everything it should have.  In either
			# case, now delete the data directory so that
			# we can start clean.
			if ( ! $noAsk )
			{
				printError( "\n" );
				printError( "Install problem:\n" );
				printError( "    The Postgres installation location appears to be in use by a\n" );
				printError( "    prior database.  This may be left over from a prior failed\n");
				printError( "    installation.  If you're sure, then we can remove the\n" );
				printError( "    prior database now and continue with the installation.\n" );
				printError( "        Directory:  $postgresDataDir\n" );
				printError( "\n" );
				my $answer = askYesNo( "    Remove (yes/no)?  " );
				if ( $answer == 0 )
				{
					printError( "\n" );
					printError( "Abort.  Please re-run this script after removing the directory.\n" );
					printLog( "Abort.  A prior Postgres database directory exists but the\n" );
					printLog( "user has elected to not remove it automatically.\n" );
					printLog( "    Directory:  $postgresDataDir\n" );
					closeLog( );
					exit( 1 );
				}
			}

			# Because the directory probably has files in it,
			# we have to use an 'rm -rf'.
			my $output = `rm -rf $postgresDataDir 2>&1`;
			if ( $? != 0 )
			{
				printError( "\n" );
				printError( "Install problem:\n" );
				printError( "    Cannot remove old Postgres data directory.\n" );
				printError( "        Directory:  $postgresDataDir\n" );
				printError( "        Error:      $output\n" );
				printError( "\n" );
				printError( "Abort.  Please re-run this script after removing the directory.\n" );
				printLog( "Abort.  A prior Postgres database directory exists\n" );
				printLog( "but could not be removed.\n" );
				printLog( "    Directory:  $postgresDataDir\n" );
				printLog( "    rm error:   $output\n" );
				closeLog( );
				exit( 1 );
			}

			printStatus( "Prior Postgres data directory removed.\n" );

			# Now we do need to initialize the database
			$doInit = 1;
		}
	}

	printStatus( "Initializing database...\n" );
	printLog( "Initializing database...\n" );
	if ( $doInit )
	{

		my $output = "";
		my $status = 0;
		my $initdb = File::Spec->catfile( $postgresBinDir, "initdb" );
		if ( $thisOS =~ /Darwin/i )	# Mac OS X
		{
			# The initdb option "lc-collate=C" used for other
			# OSes is not needed on a Mac.  It also can cause
			# problems with not enough shared memory.
			$output = `$initdb -D $postgresDataDir 2>&1`;
			$status = $?;
		}
		else
		{
			# Add the "lc-collate=C" option to insure that
			# Postgres returns sorted items in the order
			# needed.
			$output = `$initdb --lc-collate=C -D $postgresDataDir 2>&1`;
			$status = $?;
		}
		if ( $status != 0 )
		{
			printError( "\n" );
			printError( "Install problem:\n" );
			printError( "    Cannot initialize the Postgres database.\n" );
			printError( "        Error:  $output\n" );
			printError( "\n" );
			printError( "Abort.  Please re-run this script when the problem is fixed.\n" );
			printLog( "Abort.  Postgres initdb failed.\n" );
			printLog( "    Error:  $output\n" );
			closeLog( );
			exit( 1 );
		}
		printLog( $output );
	}
	else
	{
		printStatus( "    Skipped.  Database directories already present.\n" );
		printLog( "    Skipped.  Database directories already present.\n" );
	}



	# Adjust the Postgres configuration file.
	if ( $thisOS =~ /Darwin/i )	# Mac OS X
	{
		printStatus( "Adjusting postgresql.conf for Mac OS X...\n" );
		printLog( "Adjusting postgresql.conf for Mac OS X...\n" );

		# Edit the file in-place so that it retains its
		# existing ownership, permissions, hard links, and
		# other attributes.

		my $copyPsqlconf = createTempFilePath( "psqlconf" );
		if ( copy( $psqlconf, $copyPsqlconf ) != 1 )
		{
			printError( "Could not update postgresql.conf.\n" );
			printError( "Trying to continue anyway.\n" );
			printLog( "Could not update postgresql.conf.\n" );
			printLog( "Trying to continue anyway.\n" );
		}
		else
		{
			# Read the copy and write to the original.
			# Opening the original for writing truncates
			# it, and then we add lines back in.
			open( COPY, "<$copyPsqlconf" );
			open( ORIG, ">$psqlconf" );	# Truncates

			# Copy lines back in, watching for the
			# one to change.
			my $lineFound = 0;
			foreach $line ( <COPY> )
			{
				# Comment out 'lc_time =', which
				# causes problems on a Mac.
				# If this file has already been
				# adjusted, then this
				# will do nothing.
				if ( $line =~ /^lc_time/ )
				{
					# Comment it out.
					$line =~ s/^lc_time/#lc_time/i;
					$lineFound = 1;
				}
				print( ORIG $line );
			}
			close( COPY );
			close( ORIG );

			# The copy isn't needed any more.
			unlink( $copyPsqlconf );
		
			if ( ! $lineFound )
			{
				# The line wasn't found or changed.
				printStatus( "    Skipped.  File already adjusted.\n" );
				printLog( "    Skipped.  File already adjusted.\n" );
			}
		}
	} 
}





#
# @brief	Save the Postgres configuration for iRODS
#
# Further iRODS scripts use a database-agnostic configuration
# file 'irods.config'.  Here we save Postgres information into
# that file so that those scripts know what to do.
#
# Messages are output on errors.
#
sub saveIrodsConfiguration
{
	++$currentStep;
	printSubtitle( "\nStep $currentStep of $totalSteps:  Setting up iRODS...\n" );
	printLog( "\nSetting up iRODS\n" );
	printLog( "------------------------------------------------------------------------\n" );
	printLog( getCurrentDateTime( ) . "\n\n" );


	printStatus( "Updating iRODS configuration...\n" );
	my %configuration = (
		# iRODS should use Postgres
		"DATABASE_TYPE",   "postgres",

		# iRODS scripts can assume the Postgres install
		# is dedicated to iRODS.
		"DATABASE_EXCLUSIVE_TO_IRODS",   "1",

		# Postgres is installed here
		"DATABASE_HOME",    $POSTGRES_HOME,

		# Postgres uses this host
		"DATABASE_HOST",    $POSTGRES_HOST,

		# Postgres uses this port
		"DATABASE_PORT",    $POSTGRES_PORT,

		# Postgres administrator name
		"DATABASE_ADMIN_NAME",    $POSTGRES_ADMIN_NAME,

		# Postgres administrator password
		"DATABASE_ADMIN_PASSWORD",    $POSTGRES_ADMIN_PASSWORD,
	);
	if ( $unixODBC )
	{
		$configuration{"DATABASE_ODBC_TYPE"} = "unix";
	}
	else
	{
		$configuration{"DATABASE_ODBC_TYPE"} = "postgres";
	}

	printLog( "    Setting variables in irods.config....\n" );
	my $key;
	foreach $key (keys %configuration)
	{
		my $value = $configuration{$key};
		printLog( "      $key = $value\n" );
	}

	copyTemplateIfNeeded( $irodsConfig );
	my ($status, $output) = replaceVariablesInFile( $irodsConfig, "perl", 0, %configuration );
	if ( $status == 0 )
	{
		printError( "\n" );
		printError( "Install problem:\n" );
		printError( "    The iRODS configuration file could not be updated.\n" );
		printError( "        File:   $irodsConfig\n" );
		printError( "        Error:  $output\n" );
		printError( "\n" );
		printError( "Abort.  Please re-run this script when the problem is fixed.\n" );
		printLog( "Abort.  Cannot update iRODS configuration file.\n" );
		printLog( "    Error:  $output\n" );
		closeLog( );
		exit( 1 );
	}
}










########################################################################
#
# Utility Functions
#

#
# @brief	Download, uncompress, and un-tar a source archive.
#
# A source archive is FTP-ed and uncompressed into the download
# directory, then untared into the given destination directory.
#
# @param	$ftphost
# 	the FTP host name
# @param	$ftpaccount
# 	the FTP account name ("anonymous")
# @param	$ftppassword
# 	the FTP password ("-anonymous@")
# @param	$ftpdir	
# 	the FTP directory
# @param	$ftpfile
# 	the FTP file to download
# @param	$localdir
# 	the local directory to untar into
# @return
# 	the new untared directory, undef on error
#
sub download
{
	my ($ftphost,$ftpaccount,$ftppassword,$ftpdir,$ftpfile,$localdir) = @_;

	# Parse the file name to get:
	# 	filename	name without leading directories
	# 	filename_nogz	name without .gz extension
	# 	filename_notar	name without .tar extension
	my ($filename,$filename_nogz,$filename_notar) =
		parseDownloadFilename( $ftpfile );

	my $localfile = File::Spec->catfile( $localdir, $filename_notar );


	my $startingDir = cwd( );


	# Determine if we can skip any of the steps.
	my $skipFtp = 0;
	my $skipUncompress = 0;
	my $skipUntar = 0;
	my $skipReason = "";
	if ( -d $localfile )
	{
		# The untared source directory is already present.
		if ( $forcedownload )
		{
			# We've been asked to force a reinstall.
			# Ask the user if it is OK to remove the
			# old source directory.
			my $removeIt = 1;
			if ( ! $noAsk )
			{
				printNotice( "A directory containing a previous copy of the\n" );
				printNotice( "source already exists.  Remove the directory\n" );
				printNotice( "and all of its contents?\n" );
				$removeIt = askYesNo( "    Remove $localfile (yes/no)?  " );
			}
			if ( $removeIt )
			{
				# Because the directory
				# probably has files in it,
				# we have to use an 'rm -rf'.
				`rm -rf $localfile`;
				if ( $? != 0 )
				{
					printError( "Cannot remove old source directory:  $localfile\n" );
					printError( "Trying to continue anyway.\n" );
					printLog( "Cannot remove old source directory:  $localfile\n" );
					printLog( "Trying to continue anyway.\n" );
				}
			}
		}
		else
		{
			# There is no need to re-FTP, uncompress, and untar.
			$skipFtp = 1;
			$skipUncompress = 1;
			$skipUntar = 1;
			$skipReason = "Skipped.  Source directory already present";
		}
	}
	elsif ( $forcedownload )
	{
		# Force a re-download.  If there's previous
		# download files, remove them.
		my $path_nogz = File::Spec->catfile( $DOWNLOAD_DIR, $filename_nogz );
		my $path = File::Spec->catfile( $DOWNLOAD_DIR, $filename );
		if ( -e $path_nogz )
		{
			unlink( $path_nogz );
		}
		if ( -e $path )
		{
			unlink( $path );
		}
	}
	else
	{
		# The untared source directory isn't present.
		my $path_nogz = File::Spec->catfile( $DOWNLOAD_DIR, $filename_nogz );
		my $path = File::Spec->catfile( $DOWNLOAD_DIR, $filename );
		if ( -e $path_nogz )
		{
			# The uncompressed tar file is already present.
			# There is no need to re-FTP and uncompress.
			$skipFtp = 1;
			$skipUncompress = 1;
			$skipReason = "Skipped.  Uncompressed tar file already present";
		}
		elsif ( -e $path )
		{
			# The compressed tar file is already present.
			# There is no need to re-FTP.
			$skipFtp = 1;
			$skipReason = "Skipped.  Compressed tar file already present";
		}
	}


	# Download the file, if needed.
	chdir( $DOWNLOAD_DIR );
	my $ftpDestination = File::Spec->catfile( $DOWNLOAD_DIR, $filename );

	my $skipReasonPrinted = 0;
	if ( $skipFtp )
	{
		printStatus( "Downloading $filename...\n" );
		printStatus( "    $skipReason\n" );
		printLog( "  Downloading to $ftpDestination...\n" );
		printLog( "    $skipReason\n" );
		$skipReasonPrinted = 1;
	}
	else
	{
		printStatus( "Downloading $filename...\n" );
		printLog( "  Downloading to $ftpDestination...\n" );

		my ($status,$result, $output) = ftp( $ftphost, $ftpaccount, $ftppassword,
			$ftpdir . "/" . $filename );
		if ( $status != 1 )
		{
			chdir( $startingDir );
			printError( "\nDownload problem:\n" );
			if ( $status == 0 )
			{
				printError( "    The FTP server '$ftphost' did not respond when trying to download\n" );
				printError( "    the file '$filename' needed for this installation.  The server\n" );
				printError( "    may be overloaded or down for maintenance.  Please try again later.\n" );
				printError( "\n    See the log for further details:\n" );
				printError( "        Log:  $logFile\n" );
			}
			if ( $status == -1 )
			{
				printError( "    The FTP server '$ftphost' is rejecting logins for downloading\n" );
				printError( "    the file '$filename' needed for this installation.  The server\n" );
				printError( "    may be overloaded or down for maintenance.  Please try again later.\n" );
				printError( "\n    See the log for further details:\n" );
				printError( "        Log:  $logFile\n" );
			}
			if ( $status == -2 )
			{
				printError( "    The file '$filename' needed for this installation could not be\n" );
				printError( "    found at the FTP server '$ftphost'.  Please check with the iRODS\n" );
				printError( "    developers for information on alternate releases to use.\n" );
				printError( "\n    See the log for further details:\n" );
				printError( "        Log:  $logFile\n" );
			}

			printLog( "\nAbort:  FTP of $filename failed.\n" );
			printLog( "    FTP server:     $ftphost\n" );
			printLog( "    FTP account:    $ftpaccount\n" );
			printLog( "    FTP password:   $ftppassword\n" );
			printLog( "    FTP directory:  $ftpdir\n" );
			printLog( "    FTP file:       $filename\n" );
			printLog( "    Error:          $output\n" );
			return undef;
		}
	}


	# Uncompress the file, if needed.
	if ( $skipUncompress )
	{
		printLog( "  Uncompressing $ftpDestination...\n" );
		printLog( "    $skipReason\n" );
		if ( ! $skipReasonPrinted )
		{
			printStatus( "Uncompressing $filename...\n" );
			printStatus( "    $skipReason\n" );
			$skipReasonPrinted = 1;
		}
	}
	elsif ( $filename ne $filename_nogz )
	{
		printStatus( "Uncompressing $filename...\n" );
		printLog( "  Uncompressing $ftpDestination...\n" );

		my ($status,$output) = uncompress( $filename );
		if ( $status != 0 )
		{
			# Uncompress failed.
			if ( $skipFtp )
			{
				# We skipped FTPing the file earlier and
				# re-used a prior file.  Perhaps that file
				# was corrupted, maybe because a prior
				# download was interrupted.  Try to get
				# it again by recursing.
				unlink( $filename );
				chdir( $startingDir );
				printError( "            Failed.  File corrupted?\n" );
				printError( "            Re-downloading...\n" );
				printLog( "  Uncompress of $filename... failed.\n" );
				printLog( "    Exit code:  $status\n" );
				printLog( "    Error:      $output\n" );
				printLog( "  Re-downloading.\n" );
				return download( $ftphost,$ftpaccount,$ftppassword,$ftpdir,$ftpfile,$localdir );
			}

			chdir( $startingDir );
			printError( "\nDownload problem:\n" );
			printError( "    A problem occurred when uncompressing '$filename'.  Please check the\n" );
			printError( "    log for further details:\n" );
			printError( "        Log:  $logFile\n" );

			printLog( "\nAbort:  Uncompress of $filename failed.\n" );
			printLog( "    Exit code:  $status\n" );
			printLog( "    Error:      $output\n" );
			return undef;
		}
	}


	# Untar the archive into the destination directory, if needed.
	chdir( $localdir );
	my $tarSource = File::Spec->catfile( $DOWNLOAD_DIR, $filename_nogz );

	if ( $skipUntar )
	{
		printLog( "  Unarchiving $tarSource...\n" );
		printLog( "    $skipReason\n" );
		if ( ! $skipReasonPrinted )
		{
			printStatus( "Unarchiving $filename_nogz...\n" );
			printStatus( "    $skipReason\n" );
			$skipReasonPrinted = 1;
		}
	}
	else
	{
		printStatus( "Unarchiving $filename_nogz...\n" );
		printLog( "  Unarchiving $tarSource...\n" );

		my ($status,$output) = untar( $tarSource );
		if ( $status != 0 )
		{
			# Untar failed.
			if ( $skipFtp )
			{
				# We skipped FTPing the file earlier and
				# re-used a prior file.  Perhaps that file
				# was corrupted, maybe because a prior
				# download was interrupted.  Try to get
				# it again by recursing.
				unlink( $filename );
				unlink( $filename_nogz );
				chdir( $startingDir );
				printError( "            Failed.  File corrupted?\n" );
				printError( "            Re-downloading...\n" );
				printLog( "  Unarchiving of $filename... failed.\n" );
				printLog( "    Exit code:  $status\n" );
				printLog( "    Error:      $output\n" );
				printLog( "  Re-downloading.\n" );
				return download( $ftphost,$ftpaccount,$ftppassword,$ftpdir,$ftpfile,$localdir );
			}

			chdir( $startingDir );
			printError( "\nDownload problem:\n" );
			printError( "    A problem occurred when unarchiving '$filename_nogz'.\n" );
			printError( "    The tar command failed with $!.  Please check the log\n" );
			printError( "    for further details:\n" );
			printError( "        Log:  $logFile\n" );

			printLog( "\nAbort:  Tar extract of $filename_nogz failed.\n" );
			printLog( "    Exit code:  $status\n" );
			printLog( "    Error:      $output\n" );
			return undef;
		}
		if ( ! -e $filename_notar )
		{
			chdir( $startingDir );
			printError( "Download problem:\n" );
			printError( "    A problem occurred when unarchiving $filename_nogz.\n" );
			printError( "    The tar command did not leave a directory named\n" );
			printError( "    $filename_notar in $localdir.\n" );
			printError( "    Please check the log in $logFile.\n" );
			printLog( "\nAbort:  Tar extract of $filename_nogz failed.\n" );
			printLog( "   The untared directory that should be there, is not.\n" );
			return undef;
		}
	}

	chdir( $startingDir );


	# Return the name of the un-tared directory.
	return $localfile;
}





#
# @brief	Parse a download file path and return the file name,
# 		the uncompressed file name, and the version number.
#
# This function handles parsing a file path with multiple extensions,
# such as "filename.tar.gz".  It returns variants of the file path
# with those extensions stripped off.
#
# @param	$path
# 	the file path to parse
# @return
# 	a 3-tuple containing the download filename, the filename
# 	without the .gz extension, and the filename without the
# 	.tar and .gz extensions.
#
sub parseDownloadFilename($)
{
	my ($path)  = @_;

	# Split the path into a base file name, directories, and
	# a trailing suffix.
	my ($filename, $dirs1, $suffix1) = fileparse( $path, ".[^.]*" );

	# File names are presumed to have the form:
	#	file.tar.gz  <-- most likely
	#	file.tar.zip
	#	file.tar.z
	#	file.tar.Z
	#	file.tgz
	#	file.taz
	#	file.tar
	if ( $suffix1 =~ /^\.((gz)|(zip)|(z)|(Z))$/ )
	{
		# Compressed.  File had a .gz (or equiv) extension.
		# Strip off a .tar and return all three name variants.
		my ($filename_notar, $dirs2, $suffix2) =
			fileparse( $filename, ".[^.]*" );

		return ($filename . $suffix1,	# file.tar.gz
			$filename,		# file.tar
			$filename_notar);	# file
	}
	elsif ( $suffix1 =~ /^.((tgz)|(taz))$/ )
	{
		# Compressed.  File had .tgz shorthand for .tar.gz.
		return ($filename . $suffix1,	# file.tgz
			$filename . ".tar",	# file.tar
			$filename);		# file
	}

	# Not compressed.  File had a .tar extension (presumably).
	return ($filename . $suffix1,	# file.tar
		$filename . $suffix1,	# file.tar
		$filename);		# file
}





#
# @brief	Print a usage message.
#
# Print how to use this script.
#
sub printUsage
{
	# Force verbose output.
	setPrintVerbose( 1 );

	printTitle( "Install Postgres and ODBC for iRODS\n" );
	printTitle( "------------------------------------------------------------------------\n" );
	printNotice( "This application downloads, builds, and installs Postgres and ODBC\n" );
	printNotice( "drivers for iRODS.  It does not require administrator privileges.\n" );
	printNotice( "\n" );
	printNotice( "This could take up to ** 1/2 hour ** to run, depending upon your\n" );
	printNotice( "network connection and computer speed.\n" );
	printNotice( "\n" );
	printNotice( "Usage:\n" );
	printNotice( "    installPostgres [options]\n" );
	printNotice( "\n" );
	printNotice( "Help options:\n" );
	printNotice( "    --help           Show this help information\n" );
	printNotice( "\n" );
	printNotice( "Verbosity options:\n" );
	printNotice( "    --quiet          Suppress all messages\n" );
	printNotice( "    --verbose        Output all messages (default)\n" );
	printNotice( "    --noask          Don't ask questions, assume 'yes'\n" );
	printNotice( "    --noheader       Suppress printing extra header messages\n" );
	printNotice( "    --indent         Cause all output to be indented 8 characters\n" );
	printNotice( "\n" );
	printNotice( "Other options:\n" );
	printNotice( "    --forcedownload  Force re-download\n" );
	printNotice( "    --forceinit      Force initialization of old db, if present\n" );
	printNotice( "    --force          Same as --forcedownload --forceinit\n" );
	printNotice( "\n" );
	printNotice( "If a problem occurs and this application stops, please fix the problem\n" );
	printNotice( "and re-run this application.  It will only re-do the steps it needs to.\n" );
	printNotice( "You can force this application to re-do everything by using the --force option.\n" );
}
