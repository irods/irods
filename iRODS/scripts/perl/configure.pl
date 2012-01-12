#
# Perl

#
# Configure the iRODS system before compiling or installing.
#
# Usage is:
# 	perl configure.pl [options]
#
# Options:
#	--help		List all options
#	
#	Options vary depending upon the source distribution.
#	Please use --help to get a list of current options.
#
# Script options select the database to use and its location,
# the iRODS server components, and optional iRODS modules to
# build into the system.
#
# The script analyzes your OS to determine configuration parameters
# that are OS and CPU specific.
#
# Configuration results are written to:
#
# 	config/config.mk
# 		A Makefile include file used during compilation
# 		of iRODS.
#
# 	config/irods.config
#		A parameter file used by iRODS scripts to start
#		and stop iRODS, etc.
#
# 	irodsctl
#		A shell script for running the iRODS control
#		Perl script to start/stop servers.
#

use File::Spec;
use File::Copy;
use File::Basename;
use Cwd;
use Cwd 'abs_path';
use Config;

$version{"configure.pl"} = "September 2011";

my $output;
my $status;






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
#
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
# print information to various configuration files.
my $thisOS     = getCurrentOS( );
my $thisProcessor = getCurrentProcessor( );
my $thisUser   = getCurrentUser( );
my $thisUserID = $<;
my $thisHost   = getCurrentHostName( );
my %thisHostAddresses = getCurrentHostAddresses( );

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

if ( $thisUserID == 0 )
{
	printError( "Usage error:\n" );
	printError( "    This script should *not* be run as root.\n" );
	exit( 1 );
}





########################################################################
#
# Collect available modules.
#	Build a list of available modules.  Each module has
#	enable/disable options.  The default setting is determined
#	by the module's "info.txt" file.
#
$startDir = cwd( );
%modules = ( );
if ( -d $modulesDir )
{
	# Make a list of all modules in the directory
	chdir( $modulesDir );
	while ( defined( $module = <*> ) )
	{
		# A module must have an 'info.txt' file describing the
		# module in order for it to be configurable by this script
		my $infoPath = File::Spec->catfile(
			$modulesDir, $module, "info.txt" );
		if ( -e $infoPath )
		{
			my $value = getPropertyValue( $infoPath, "enabled", 0 );
			if ( $value =~ /ye?s?/i )
			{
				$modules{ $module } = "yes";
			}
			else
			{
				$modules{ $module } = "no";
			}
		}
	}
    	chdir( $startDir );
}





########################################################################
#
# Print a help message then exit before going further.
#	We have to print this after collecting the module list
#	above so that we can list enable/disable options for
#	those modules.
#
foreach $arg ( @ARGV )
{
	if ( !( $arg =~ /-?-?h(elp)/) )
	{
		next;
	}

	printTitle( "Configure iRODS\n" );
	printTitle( "------------------------------------------------------------------------\n" );
	printNotice( "This script configures iRODS for the current host based upon\n" );
	printNotice( "CPU and OS attributes and command-line arguments.\n" );
	printNotice( "\n" );
	printNotice( "Usage is:\n" );
	printNotice( "    configure [options]\n" );
	printNotice( "\n" );
	printNotice( "Help options:\n" );
	printNotice( "    --help                      Show this help information\n" );
	printNotice( "\n" );
	printNotice( "Verbosity options:\n" );
	printNotice( "    --quiet                     Suppress all messages\n" );
	printNotice( "    --verbose                   Output all messages (default)\n" );
	printNotice( "\n" );
	printNotice( "iCAT options:\n" );
	printNotice( "    --enable-icat               Enable iRODS metadata catalog files\n" );
	printNotice( "    --disable-icat              Disable iRODS metadata catalog files\n" );
	printNotice( "    --icat-host=<HOST>          Use iRODS+iCAT server on host\n" );
	printNotice( "    --enable-psgcat             Enable Postgres database catalog\n" );
	printNotice( "    --disable-psgcat            Disable Postgres database catalog\n" );
	printNotice( "    --enable-oracat             Enable Oracle database catalog\n" );
	printNotice( "    --disable-oracat            Disable Oracle database catalog\n" );
	printNotice( "    --enable-mycat              Enable MySQL database catalog\n" );
	printNotice( "    --disable-mycat             Disable MySQL database catalog\n" );
	printNotice( "\n" );
	printNotice( "    --enable-psghome=<DIR>      Set the Postgres directory\n" );
	printNotice( "    --enable-newodbc            Use the new ODBC interface\n" );
	printNotice( "    --enable-oldodbc            Use the old ODBC interface\n" );
	printNotice( "\n" );
	printNotice( "iRODS options:\n" );
	printNotice( "    --enable-parallel           Enable parallel computation\n" );
	printNotice( "    --disable-parallel          Disable parallel computation\n" );
	printNotice( "    --enable-file64bit          Enable large files\n" );
	printNotice( "    --disable-file64bit         Disable large files\n" );
	printNotice( "\n" );
	printNotice( "Authentication options:\n" );
	printNotice( "    --enable-gsi                Enable GSI authentication\n" );
	printNotice( "    --globus-location=<DIR>     Where Globus is installed\n" );
	printNotice( "    --gsi-install-type=<TYPE>   Globus/GSI installation type\n" );
	printNotice( "                                See \$GLOBUS_LOCATION/include\n" );
	printNotice( "    --disable-gsi               Disable GSI authentication\n" );
	if ( scalar keys %modules > 0 )
	{
		printNotice( "\n" );
		printNotice( "Module options:\n" );
		foreach $module (keys %modules)
		{
			my $infoPath = File::Spec->catfile( $modulesDir, $module, "info.txt" );
			my $brief = getPropertyValue( $infoPath, "brief" );
			printNotice( "    --enable-$module        $brief\n" );
			printNotice( "    --disable-$module       $brief\n" );
		}
	}
	exit( 0 );
}





########################################################################
#
# Set a default configuration.
#
# Some or all of these may be overridden by the iRODS configuration file.
# After that, they form the starting point of further configuration and
# are written back to the iRODS configuration files at the end of this
# script.
#
%irodsConfigVariables = ( );		# irods.config settings
%configMkVariables = ( );		# config.mk settings
%platformMkVariables = ( );		# platform.mk settings

$configMkVariables{ "RODS_CAT" } = "";	# Enable iCAT
$configMkVariables{ "PSQICAT" }  = "";	# Enable Postgres iCAT
$configMkVariables{ "ORAICAT" }  = "";	# Disable Oracle iCAT
$configMkVariables{ "MYICAT" }  = "";	# Disable MySQL iCAT
$configMkVariables{ "NEW_ODBC" } = "1";	# New ODBC drivers
$configMkVariables{ "PARA_OPR" } = "1";	# Parallel

$irodsConfigVariables{ "IRODS_HOME" } = $IRODS_HOME;
$irodsConfigVariables{ "IRODS_PORT" } = "1247";
$irodsConfigVariables{ "IRODS_ADMIN_NAME" } = "rods";
$irodsConfigVariables{ "IRODS_ADMIN_PASSWORD" } = "rods";
$irodsConfigVariables{ "IRODS_ICAT_HOST" } = "";

$irodsConfigVariables{ "DATABASE_TYPE" } = "";			# No database
$irodsConfigVariables{ "DATABASE_ODBC_TYPE" } = "";		# No ODBC!?
$irodsConfigVariables{ "DATABASE_EXCLUSIVE_TO_IRODS" } ="0";	# Database just for iRODS
$irodsConfigVariables{ "DATABASE_HOME" } = "$IRODS_HOME/../iRodsPostgres";	# Database directory

$irodsConfigVariables{ "DATABASE_HOST" } = "";			# Database host
$irodsConfigVariables{ "DATABASE_PORT" } = "5432";		# Database port
$irodsConfigVariables{ "DATABASE_ADMIN_NAME" } = $thisUser;	# Database admin
$irodsConfigVariables{ "DATABASE_ADMIN_PASSWORD" } = "";	# Database admin password





########################################################################
#
# Load and validate irods.config.
#	This function sets a large number of important global variables
#	based upon values in the irods.config file.  Those include the
#	type of database in use (if any), the path to that database,
#	the host and port for the database, and the initial account
#	name and password for the database and iRODS.
#
#	This function also validates that the values look reasonable and
#	prints messages if they do not.
#
#	A complication is that the config file also sets $IRODS_HOME.
#	This is intended for use by other scripts run *after* this
#	configuration script.  So, at this point $IRODS_HOME in the
#	file is invalid.  So, we ignore it and restore the value we've
#	already determined.
#
my $savedIRODS_HOME = $IRODS_HOME;
copyTemplateIfNeeded( $irodsConfig );
if ( loadIrodsConfig( ) == 0 )
{
	# Configuration failed to load or validate.  An error message
	# has already been output.
	exit( 1 );
}
$IRODS_HOME = $savedIRODS_HOME;


# Set configuration variables based upon irods.config.
#	We intentionally *do not* transfer $IRODS_HOME from the
#	configuration file.  During an initial install, that value
#	is probably empty or wrong.  Instead, use the directory
#	relative to where we are executing.
#
#	All other values in irods.config provide defaults.  When
#	this configure script is done, these values, as modified
#	by command-line arguments, are written back to irods.config
#	to provide new defaults for the next time (if ever) that
#	this script is run.
$irodsConfigVariables{ "IRODS_PORT" }           = $IRODS_PORT;
$irodsConfigVariables{ "IRODS_ADMIN_NAME" }     = $IRODS_ADMIN_NAME;
$irodsConfigVariables{ "IRODS_ADMIN_PASSWORD" } = $IRODS_ADMIN_PASSWORD;
$irodsConfigVariables{ "IRODS_ICAT_HOST" } 	= $IRODS_ICAT_HOST;

$irodsConfigVariables{ "DATABASE_TYPE" }        = $DATABASE_TYPE;
$irodsConfigVariables{ "DATABASE_ODBC_TYPE" }   = $DATABASE_ODBC_TYPE;
$irodsConfigVariables{ "DATABASE_EXCLUSIVE_TO_IRODS" } = $DATABASE_EXCLUSIVE_TO_IRODS;
$irodsConfigVariables{ "DATABASE_HOME" }        = $DATABASE_HOME;

$irodsConfigVariables{ "DATABASE_HOST" }        = $DATABASE_HOST;
$irodsConfigVariables{ "DATABASE_PORT" }        = $DATABASE_PORT;
$irodsConfigVariables{ "DATABASE_ADMIN_NAME" }  = $DATABASE_ADMIN_NAME;
$irodsConfigVariables{ "DATABASE_ADMIN_PASSWORD" } = $DATABASE_ADMIN_PASSWORD;

if ($GSI_AUTH == 1) {
	$configMkVariables{ "GSI_AUTH" } = $GSI_AUTH;
	$configMkVariables{ "GLOBUS_LOCATION" } = $GLOBUS_LOCATION;
	$configMkVariables{ "GSI_INSTALL_TYPE" } = $GSI_INSTALL_TYPE;
        # Use system ssl if GSI's ssl lib doesn't exist
	my $testLib = $GLOBUS_LOCATION . "/lib/libssl_" . $GSI_INSTALL_TYPE . ".a";
	if ( ! -e $testLib ) {
	    $configMkVariables{ "GSI_SSL" } = ssl;
	}
        # Use system crypto if GSI's crypto lib doesn't exist
	my $testLib = $GLOBUS_LOCATION . "/lib/libcrypto_" . $GSI_INSTALL_TYPE . ".a";
	if ( ! -e $testLib ) {
	    $configMkVariables{ "GSI_CRYPTO" } = crypto;
	}
}

if ( $DATABASE_ODBC_TYPE =~ /unix/i )
{
	$configMkVariables{ "NEW_ODBC" } = "1";	# New ODBC drivers
}
else
{
	$configMkVariables{ "NEW_ODBC" } = "";	# Old ODBC drivers
}

if ( $DATABASE_TYPE =~ /postgres/i )
{
	# Postgres.
	$configMkVariables{ "RODS_CAT" } = "1";	# Enable iCAT
	$configMkVariables{ "PSQICAT" }  = "1";	# Enable Postgres iCAT
	$configMkVariables{ "ORAICAT" }  = "";	# Disable Oracle iCAT
	$configMkVariables{ "MYICAT" }  = ""; # Disable MySQL iCAT
}
elsif ( $DATABASE_TYPE =~ /oracle/i )
{
	# Oracle.
	$configMkVariables{ "RODS_CAT" } = "1";	# Enable iCAT
	$configMkVariables{ "PSQICAT" }  = "";	# Disable Postgres iCAT
	$configMkVariables{ "ORAICAT" }  = "1";	# Enable Oracle iCAT
	$configMkVariables{ "MYICAT" }  = "";   # Disable MySQL iCAT
}
elsif ( $DATABASE_TYPE =~ /mysql/i )
{
	# MySQL.
	$configMkVariables{ "RODS_CAT" } = "1";	# Enable iCAT
	$configMkVariables{ "PSQICAT" }  = "";	# Disable Postgres iCAT
	$configMkVariables{ "ORAICAT" }  = "";	# Disable Oracle iCAT
	$configMkVariables{ "MYICAT" }  = "1";	# Enable MySQL iCAT
}
else
{
	# Unknown or no database.  No iCAT.
	$configMkVariables{ "RODS_CAT" } = "";	# Disable iCAT
	$configMkVariables{ "PSQICAT" }  = "";	# Disable Postgres iCAT
	$configMkVariables{ "ORAICAT" }  = "";	# Disable Oracle iCAT
	$configMkVariables{ "MYICAT" }  = ""; # Disable MySQL iCAT
}





########################################################################
#
# Check command line argument(s).   Use them to set the configuration.
#
$noHeader = 0;
foreach $arg ( @ARGV )
{
	# Postgres iCAT
	if ( $arg =~ /--disable-psgi?cat/ )
	{
		$configMkVariables{ "PSQICAT" } = "";
		$configMkVariables{ "RODS_CAT" } = "";
		next;
	}
	if ( $arg =~ /--enable-psgi?cat/ )
	{
		$configMkVariables{ "PSQICAT" } = "1";
		$configMkVariables{ "RODS_CAT" } = "1";
		$configMkVariables{ "ORAICAT" } = "";
		$configMkVariables{ "MYICAT" } = "";
		next;
	}

	# Oracle iCAT
	if ( $arg =~ /--disable-orai?cat/ )
	{
		$configMkVariables{ "ORAICAT" } = "";
		$configMkVariables{ "RODS_CAT" } = "";
		next;
	}
	if ( $arg =~ /--enable-orai?cat/ )
	{
		$configMkVariables{ "PSQICAT" } = "";
		$configMkVariables{ "RODS_CAT" } = "1";
		$configMkVariables{ "ORAICAT" } = "1";
		$configMkVariables{ "MYICAT" } = "";
		next;
	}

	# MySQL iCAT
	if ( $arg =~ /--disable-myi?cat/ )
	{
		$configMkVariables{ "MYICAT" } = "";
		$configMkVariables{ "RODS_CAT" } = "";
		next;
	}
	if ( $arg =~ /--enable-myi?cat/ )
	{
		$configMkVariables{ "PSQICAT" } = "";
		$configMkVariables{ "RODS_CAT" } = "1";
		$configMkVariables{ "ORAICAT" } = "";
		$configMkVariables{ "MYICAT" } = "1";
		next;
	}

	# iCAT
	if ( $arg =~ /--disable-icat/ )
	{
		$configMkVariables{ "PSQICAT" } = "";
		$configMkVariables{ "RODS_CAT" } = "";
		$configMkVariables{ "ORAICAT" } = "";
		$configMkVariables{ "MYICAT" } = "";
		next;
	}
	if ( $arg =~ /--enable-icat/ )
	{
		$configMkVariables{ "PSQICAT" } = "1";	# Default to Postgres
		$configMkVariables{ "RODS_CAT" } = "1";
		$configMkVariables{ "ORAICAT" } = "";
		$configMkVariables{ "MYICAT" } = "";
		next;
	}
	if ( $arg =~ /--icat-host=(.*)/ )
	{
		$irodsConfigVariables{ "IRODS_ICAT_HOST" } = $1;
		next;
	}


	# Postgres install directory
	if ( $arg =~ /--enable-psghome=(.*)/ )
	{
		my $psgdir = $1;
		$configMkVariables{ "PSQICAT" } = "1";	# Default to Postgres
		$configMkVariables{ "RODS_CAT" } = "1";
		$configMkVariables{ "ORAICAT" } = "";
		$configMkVariables{ "MYICAT" } = "";
		my $default = $irodsConfigVariables{ "DATABASE_HOME" };
		my $psgdir_abs  = abs_path( $psgdir );
		my $default_abs = abs_path( $default );
		if ( !( $psgdir_abs =~ $default_abs ) )
		{
			# Different directory.  Assume not exclusive use.
			$irodsConfigVariables{ "DATABASE_HOME" } = $psgdir;
			$irodsConfigVariables{ "DATABASE_EXCLUSIVE_TO_IRODS" } ="0";
		}
		next;
	}

	# Parallel execution
	if ( $arg =~ /--disable-parallel/ )
	{
		$configMkVariables{ "PARA_OPR" } = "";
		next;
	}
	if ( $arg =~ /--enable-parallel/ )
	{
		$configMkVariables{ "PARA_OPR" } = "1";
		next;
	}

	# 64-bit file accesses
	if ( $arg =~ /--disable-file64bit/ )
	{
		$configMkVariables{ "FILE_64BITS" } = "";
		next;
	}
	if ( $arg =~ /--enable-file64bit/ )
	{
		$configMkVariables{ "FILE_64BITS" } = "1";
		next;
	}

	# 64-bit addressing
	if ( $arg =~ /--disable-addr64bit/ )
	{
		$configMkVariables{ "ADDR_64BITS" } = "";
		next;
	}
	if ( $arg =~ /--enable-addr64bit/ )
	{
		$configMkVariables{ "ADDR_64BITS" } = "1";
		next;
	}

	# New or old ODBC code
	if ( $arg =~ /--enable-newodbc/ )
	{
		$configMkVariables{ "NEW_ODBC" } = "1";
		next;
	}
	if ( $arg =~ /--enable-oldodbc/ )
	{
		$configMkVariables{ "NEW_ODBC" } = "";
		next;
	}

	# iRODS server port
	if ( $arg =~ /--enable-i?rodsport=(.*)/ )
	{
		$irodsConfigVariables{ "IRODS_PORT" } = $1;
		next;
	}

	# GSI
	if ( $arg =~ /--enable-gsi/ )
	{
		$configMkVariables{ "GSI_AUTH" } = "1";
		next;
	}
	if ( $arg =~ /--disable-gsi/ )
	{
		$configMkVariables{ "GSI_AUTH" } = "";
		next;
	}
	if ( $arg =~ /--globus-location=(.*)/ )
	{
		$configMkVariables{ "GSI_AUTH" } = "1";
		$configMkVariables{ "GLOBUS_LOCATION" } = $1;
		next;
	}
	if ( $arg =~ /--gsi-install-type=(.*)/ )
	{
		$configMkVariables{ "GSI_AUTH" } = "1";
		$configMkVariables{ "GSI_INSTALL_TYPE" } = $1;
		next;
	}

	# Modules
	my $modargfound = 0;
	foreach $module ( keys %modules )
	{
		if ( $arg =~ /--disable-$module/ )
		{
			$modules{ $module } = "no";
			$modargfound = 1;
			last;
		}
		elsif ( $arg =~ /--enable-$module/ )
		{
			$modules{ $module } = "yes";
			$modargfound = 1;
			last;
		}
	}
	if ( $modargfound )
	{
		next;
	}

	if ( $arg =~ /-?-?q(uiet)/ )
	{
		setPrintVerbose( 0 );
		next;
	}

	if ( $arg =~ /-?-?v(erbose)/ )
	{
		setPrintVerbose( 1 );
		next;
	}
	if ( $arg =~ /^-?-?indent$/i )		# Indent everything
	{
		setMasterIndent( "        " );
		next;
	}

	if ( $arg =~ /^-?-?noheader$/i )	# Suppress header message
	{
		$noHeader = 1;
		next;
	}

	# Unknown argument
	printError( "Unknown option:  $arg\n" );
	printError( "Use --help for a list of options.\n" );
	exit( 1 );
}


if ( ! $noHeader )
{
	printTitle( "Configure iRODS\n" );
	printTitle( "------------------------------------------------------------------------\n" );
}





########################################################################
#
# Check module dependencies.
#
# For each enabled module, make sure all of the modules it
# depends upon are also enabled.
#
@modulesNeeded = ( );
foreach $module (keys %modules)
{
	if ( $modules{$module} eq "yes" )
	{
		my $infoPath = File::Spec->catfile( $modulesDir, $module, "info.txt" );
		my $deplist = getPropertyValue( $infoPath, "dependencies" );
		if ( defined( $deplist ) )
		{
			my @depends = split( " ", $deplist );
			foreach $depend (@depends)
			{
				if ( ! defined( $modules{$depend} ) ||
					$modules{$depend} eq "no" )
				{
					push( @modulesNeeded, $depend );
				}
			}
		}
	}
}

if ( scalar @modulesNeeded > 0 )
{
	printError( "Configuration error:\n" );
	printError( "    The following modules are depended upon by enabled modules\n" );
	printError( "    but were not enabled:\n" );
	foreach $module (@modulesNeeded)
	{
		printError( "        $module\n" );
	}
	printError( "\n" );
	printError( "    Please review your configuration and either enable these\n" );
	printError( "    modules, or disable the ones that require them.\n" );
	printError( "\n" );
	printError( "Abort.  Please re-run this script with updated options.\n" );
	exit( 1 );
}

$currentStep++;
printSubtitle( "\nStep $currentStep of $totalSteps:  Enabling modules...\n" );
if ( scalar keys %modules > 0 )
{
	my $tmp = "";
	foreach $module (keys %modules )
	{
		if ( $modules{$module} =~ "yes" )
		{
			$tmp .= " $module";
			printStatus( "$module\n" );
		}
	}
	$configMkVariables{ "MODULES"} = $tmp;
}
else
{
	printStatus( "    Skipped.  No modules enabled.\n" );
	$configMkVariables{ "MODULES"} = "";
}





########################################################################
#
# Verify database setup.
#
$currentStep++;
printSubtitle( "\nStep $currentStep of $totalSteps:  Verifying configuration...\n" );
if ( $configMkVariables{ "RODS_CAT" } ne "1" )
{
	# No iCAT.  No database.
	$irodsConfigVariables{ "DATABASE_TYPE" } = "";
	$irodsConfigVariables{ "DATABASE_HOME" } = "";
	$irodsConfigVariables{ "DATABASE_EXCLUSIVE_TO_IRODS" } ="0";
	$irodsConfigVariables{ "DATABASE_HOST" } = "";
	$irodsConfigVariables{ "DATABASE_PORT" } = "";
	$irodsConfigVariables{ "DATABASE_ADMIN_NAME" } = "";
	$irodsConfigVariables{ "DATABASE_ADMIN_PASSWORD" } = "";

	printStatus( "No database configured.\n" );
}
elsif ( $configMkVariables{ "PSQICAT" } eq "1" )
{
	# Configuration has enabled Postgres.  Make sure the
	# rest of the configuration matches.
	$irodsConfigVariables{ "DATABASE_TYPE" } = "postgres";
	$irodsConfigVariables{ "POSTGRES_HOME" } = $DATABASE_HOME;
	$configMkVariables{ "POSTGRES_HOME" } = $DATABASE_HOME;

	$databaseHome = $irodsConfigVariables{ "DATABASE_HOME" };
	if ( ! -e $databaseHome )
	{
		printError( "\n" );
		printError( "Configuration problem:\n" );
		printError( "    Cannot find the Postgres home directory.\n" );
		printError( "        Directory:  $databaseHome\n" );
		printError( "\n" );
		printError( "Abort.  Please re-run this script after fixing this problem.\n" );
		exit( 1 );
	}
	my $databaseBin = File::Spec->catdir( $databaseHome, "bin" );
	if ( ! -e $databaseBin )
	{
		# A common error/confusion is to give a database
		# home directory that is one up from the one to use.
		# For instance, if an iRODS install has put Postgres
		# in "here", then "here/pgsql/bin" is the bin directory,
		# not "here/bin".  Check for this and silently adjust.
		my $databaseBin2 = File::Spec->catdir( $databaseHome, "pgsql", "bin" );
		if ( ! -e $databaseBin2 )
		{
			# That didn't work either.  Complain, but
			# use the first tried bin directory in the
			# message.
			printError( "\n" );
			printError( "Configuration problem:\n" );
			printError( "    Cannot find the Postgres bin directory.\n" );
			printError( "        Directory:  $databaseBin\n" );
			printError( "\n" );
			printError( "Abort.  Please re-run this script after fixing this problem.\n" );
			exit( 1 );
		}
		# That worked.  Adjust.
		$DATABASE_HOME = File::Spec->catdir( $databaseHome, "pgsql" );
		$databaseHome  = $DATABASE_HOME;
		$irodsConfigVariables{ "POSTGRES_HOME" } = $DATABASE_HOME;
		$configMkVariables{ "POSTGRES_HOME" } = $DATABASE_HOME;
		$irodsConfigVariables{ "DATABASE_HOME" } = $DATABASE_HOME;
	}

	printStatus( "Postgres database found.\n" );
}
elsif ( $configMkVariables{ "ORAICAT" } eq "1" )
{
	# Configuration has enabled Oracle.  Make sure the
	# rest of the irodsConfigVariables matches.
	$irodsConfigVariables{ "DATABASE_TYPE" } = "oracle";
	$irodsConfigVariables{ "ORACLE_HOME" } = $DATABASE_HOME;
	$configMkVariables{ "ORACLE_HOME" } = $DATABASE_HOME;

	$databaseHome = $irodsConfigVariables{ "DATABASE_HOME" };
	if ( ! -e $databaseHome )
	{
		printError( "\n" );
		printError( "Configuration problem:\n" );
		printError( "    Cannot find the Oracle home directory.\n" );
		printError( "        Directory:  $databaseHome\n" );
		printError( "\n" );
		printError( "Abort.  Please re-run this script after fixing this problem.\n" );
		exit( 1 );
	}

	printStatus( "Oracle database found in $databaseHome\n" );
}
elsif ( $configMkVariables{ "MYICAT" } eq "1" )
{
	# Configuration has enabled MySQL.  Make sure the
	# rest of the configuration matches.
	$irodsConfigVariables{ "DATABASE_TYPE" } = "mysql";
	$irodsConfigVariables{ "UNIXODBC_HOME" } = $DATABASE_HOME;
	$configMkVariables{ "UNIXODBC_HOME" } = $DATABASE_HOME;
	$configMkVariables{ "UNIXODBC_DATASOURCE" } = $DB_NAME;

	$databaseHome = $irodsConfigVariables{ "DATABASE_HOME" };
	if ( ! -e $databaseHome )
	{
		printError( "\n" );
		printError( "Configuration problem:\n" );
		printError( "    Cannot find the unixODBC install directory.\n" );
		printError( "        Directory:  $databaseHome\n" );
		printError( "\n" );
		printError( "Abort.  Please re-run this script after fixing this problem.\n" );
		exit( 1 );
	}

	printStatus( "MySQL database found.\n" );
}
else
{
	# Configuration has no iCAT.
	$irodsConfigVariables{ "DATABASE_TYPE" } = "";
	$configMkVariables{ "RODS_CAT" } = "";

	printStatus( "No database configured.\n" );
}





########################################################################
#
# Check host characteristics.
#
$currentStep++;
printSubtitle( "\nStep $currentStep of $totalSteps:  Checking host system...\n" );


# What OS?
if ( $thisOS =~ /linux/i )
{
	$configMkVariables{ "OS_platform" } = "linux_platform";
	printStatus( "Host OS is Linux.\n" );
}
elsif ( $thisOS =~ /(sunos)|(solaris)/i )
{
	if ( $thisProcessor =~ /i.86/i )	# such as i386, i486, i586, i686
	{
		$configMkVariables{ "OS_platform" } = "solaris_pc_platform";
		printStatus( "Host OS is Solaris (PC).\n" );
	}
	else	# probably "sun4u" (sparc)
	{
		$configMkVariables{ "OS_platform" } = "solaris_platform";
		printStatus( "Host OS is Solaris (Sparc).\n" );
	}
}
elsif ( $thisOS =~ /aix/i )
{
	$configMkVariables{ "OS_platform" } = "aix_platform";
	printStatus( "Host OS is AIX.\n" );
}
elsif ( $thisOS =~ /irix/i )
{
	$configMkVariables{ "OS_platform" } = "sgi_platform";
	printStatus( "Host OS is SGI.\n" );
}
elsif ( $thisOS =~ /darwin/i )
{
	$configMkVariables{ "OS_platform" } = "osx_platform";
	printStatus( "Host OS is Mac OS X.\n" );
}
else
{
	printError( "\n" );
	printError( "Configuration problem:\n" );
	printError( "    Unrecognized OS type:  $thisOS\n" );
	printError( "\n" );
	printError( "Abort.  Please contact the iRODS developers for more information\n" );
	printError( "on support for this OS.\n" );
	exit( 1 );
}





########################################################################
#
# Find OS-specific tools
#	All of these may be overridden by setting variables in
#	irods.config.  The default leaves those variables empty,
#	in which case we search for the appropriate tools.
#

#
# Find perl
#
$platformMkVariables{ "PERL" } = choosePerl( );
printStatus( "Perl:        " . $platformMkVariables{ "PERL" } . "\n" );


#
# Find compiler and loader
#
($platformMkVariables{ "CC" },$platformMkVariables{ "CC_IS_GCC" },$platformMkVariables{ "LDR" }) =
	chooseCompiler( );
printStatus( "C compiler:  " . $platformMkVariables{ "CC" } . 
	($platformMkVariables{ "CC_IS_GCC" } ? " (gcc)" : "") . "\n" );

if ( defined( $CCFLAGS ) && $CCFLAGS ne "" )
{
	$platformMkVariables{ "CCFLAGS" } = $CCFLAGS;
	printStatus( "  Flags:     " . $CCFLAGS . "\n" );
}
else
{
	$platformMkVariables{ "CCFLAGS" } = '';
	printStatus( "  Flags:     none\n" );
}

printStatus( "Loader:      " . $platformMkVariables{ "LDR" } . "\n" );

if ( defined( $LDRFLAGS ) && $LDRFLAGS ne "" )
{
	$platformMkVariables{ "LDRFLAGS" } = $LDRFLAGS;
	printStatus( "  Flags:     " . $LDRFLAGS . "\n" );
}
else
{
	$platformMkVariables{ "LDRFLAGS" } = '';
	printStatus( "  Flags:     none\n" );
}


#
# Find ar
#
$platformMkVariables{ "AR" } = chooseArchiver( );
printStatus( "Archiver:    " . $platformMkVariables{ "AR" } . "\n" );


#
# Find ranlib
#
$platformMkVariables{ "RANLIB" } = chooseRanlib( );
if ( $platformMkVariables{ "RANLIB" } =~ /touch/i )
{
	printStatus( "Ranlib:      none needed\n" );
}
else
{
	printStatus( "Ranlib:      " . $platformMkVariables{ "RANLIB" } . "\n" );
}





#
# 64-bit addressing?
#
# Skip this check if a command-line option was given to enable
# 64-bit addressing.
#
if ( defined( $configMkVariables{ "ADDR_64BITS" } ) )
{
	printStatus( "64-bit addressing enabled.\n" );
}
else
{
	if ( is64bit( $platformMkVariables{ "CC" }, $platformMkVariables{ "CCFLAGS" },
	       $platformMkVariables{ "LDR" }, $platformMkVariables{ "LDRFLAGS" } ) )
	{
		printStatus( "64-bit addressing supported and automatically enabled.\n" );
		$configMkVariables{ "ADDR_64BITS" } = "1";
	}
	else
	{
		printStatus( "64-bit addressing not supported and automatically disabled.\n" );
		$configMkVariables{ "ADDR_64BITS" } = "";
	}
}





########################################################################
#
# Update files.
#
$currentStep++;
printSubtitle( "\nStep $currentStep of $totalSteps:  Updating configuration files...\n" );


# Update config.mk
#	The iRODS Makefiles check if config.mk has changed and trigger
#	a build-from-scratch if it has.  But the configuration we are
#	about to write to it might not be different.  If it isn't
#	different, we'd like to avoid triggering a rebuild.
#
#	So, copy the existing config.mk and update the copy.  Then
#	compare them.  If they are differ, copy the updated file
#	on top of the old file.  Otherwise, delete the copy and
#	leave the original as-is (since it is already uptodate).
printStatus( "Updating config.mk...\n" );
$status = copyTemplateIfNeeded( $configMk );
if ( $status == 0 )
{
	printError( "\nConfiguration problem:\n" );
	printError( "    Cannot find the configuration template:\n" );
	printError( "        File:  $configMk.in\n" );
	printError( "    Is the iRODS installation complete?\n" );
	printError( "\nAbort.  Please re-run this script when the problem is fixed.\n" );
	exit( 1 );
}
if ( $status == 2 )
{
	# New config.mk.  Update it in place.
	printStatus( "    Created $configMk\n" );

	($status, $output) = replaceVariablesInFile( $configMk, "make", 0, %configMkVariables );
	if ( $status == 0 )
	{
		printError( "\nConfiguration problem:\n" );
		printError( "    Could not update configuration file.\n" );
		printError( "        File:   $configMk\n" );
		printError( "        Error:  $output\n" );
		printError( "\nAbort.  Please re-run this script when the problem is fixed.\n" );
		exit( 1 );
	}
}
else
{
	# There is an existing config.mk.  Copy and update
	# the copy.
	$tmpConfigMk = File::Spec->catfile( File::Spec->tmpdir( ), "irods_configmk_$$.txt" );
	if ( copy( $configMk, $tmpConfigMk ) != 1 )
	{
		printError( "\nConfiguration problem:\n" );
		printError( "    Could not create a temporary file.\n" );
		printError( "        File:   $tmpConfigMk\n" );
		printError( "\nAbort.  Please re-run this script when the problem is fixed.\n" );
		exit( 1 );
	}

	($status, $output) = replaceVariablesInFile( $tmpConfigMk, "make", 0, %configMkVariables );
	if ( $status == 0 )
	{
		printError( "\nConfiguration problem:\n" );
		printError( "    Could not update configuration file.\n" );
		printError( "        File:   $tmpConfigMk\n" );
		printError( "        Error:  $output\n" );
		printError( "\nAbort.  Please re-run this script when the problem is fixed.\n" );
		exit( 1 );
	}
	# Are the files different?
	if ( diff( $tmpConfigMk, $configMk ) == 1 )
	{
		# Yup.  Replace the old file with the new one.
		move( $tmpConfigMk, $configMk );
	}
	else
	{
		# Nope.  They're the same.  Leave the original
		# untouched and delete the temp.
		printStatus( "    Skipped.  No change.\n" );
		unlink( $tmpConfigMk );
	}
}


# Update platform.mk
#	See the comments above for config.mk.  Again, with platform.mk
#	we don't want to write to the existing file if there are no
#	changes.  This avoids the change triggering a rebuild.
#       But if the template is newer (via a 'cvs update' for example),
#       we do need to copy it so that new settings will take effect.
printStatus( "Updating platform.mk...\n" );
$platformMk = File::Spec->catfile( $configDir, "platform.mk" );
$status = copyTemplateIfNeededOrNewer( $platformMk );
if ( $status == 0 )
{
	printError( "\nConfiguration problem:\n" );
	printError( "    Cannot find the configuration template:\n" );
	printError( "        File:  $platformMk.in\n" );
	printError( "    Is the iRODS installation complete?\n" );
	printError( "\nAbort.  Please re-run this script when the problem is fixed.\n" );
	exit( 1 );
}
if ( $status == 2 )
{
	# New platform.mk.  Update it in place.
	printStatus( "    Created $platformMk\n" );

	($status, $output) = replaceVariablesInFile( $platformMk, "make", 0, %platformMkVariables );
	if ( $status == 0 )
	{
		printError( "\nConfiguration problem:\n" );
		printError( "    Could not update configuration file.\n" );
		printError( "        File:   $platformMk\n" );
		printError( "        Error:  $output\n" );
		printError( "\nAbort.  Please re-run this script when the problem is fixed.\n" );
		exit( 1 );
	}
}
else
{
	# There is an existing platform.mk.  Copy and update
	# the copy.
	$tmpPlatformMk = File::Spec->catfile( File::Spec->tmpdir( ), "irods_platformmk_$$.txt" );
	if ( copy( $platformMk, $tmpPlatformMk ) != 1 )
	{
		printError( "\nConfiguration problem:\n" );
		printError( "    Could not create a temporary file.\n" );
		printError( "        File:   $tmpPlatformMk\n" );
		printError( "\nAbort.  Please re-run this script when the problem is fixed.\n" );
		exit( 1 );
	}

	($status, $output) = replaceVariablesInFile( $tmpPlatformMk, "make", 0, %platformMkVariables );
	if ( $status == 0 )
	{
		printError( "\nConfiguration problem:\n" );
		printError( "    Could not update configuration file.\n" );
		printError( "        File:   $tmpPlatformMk\n" );
		printError( "        Error:  $output\n" );
		printError( "\nAbort.  Please re-run this script when the problem is fixed.\n" );
		exit( 1 );
	}

	# Are the files different?
	if ( diff( $tmpPlatformMk, $platformMk ) == 1 )
	{
		# Yup.  Replace the old file with the new one.
		move( $tmpPlatformMk, $platformMk );
	}
	else
	{
		# Nope.  They're the same.  Leave the original
		# untouched and delete the temp.
		printStatus( "    Skipped.  No change.\n" );
		unlink( $tmpPlatformMk );
	}
}


# Update irods.config
printStatus( "Updating irods.config...\n" );
($status, $output) = replaceVariablesInFile( $irodsConfig, "perl", 0, %irodsConfigVariables );
if ( $status == 0 )
{
	printError( "\nConfiguration problem:\n" );
	printError( "    Could not update configuration file.\n" );
	printError( "        File:   $irodsConfig\n" );
	printError( "        Error:  $output\n" );
	printError( "\nAbort.  Please re-run this script when the problem is fixed.\n" );
	exit( 1 );
}
# Make sure irods.config is not world/other readable since it contains
# an administrator's password.
chmod( 0600, $irodsConfig );


# Update irodsctl script
printStatus( "Updating irodsctl...\n" );
($status, $output) = replaceVariablesInFile( $irodsctl, "shell", 0, %irodsConfigVariables );
if ( $status == 0 )
{
	printError( "\nConfiguration problem:\n" );
	printError( "    Could not update script.\n" );
	printError( "        File:   $irodsctl\n" );
	printError( "        Error:  $output\n" );
	printError( "\nAbort.  Please re-run this script when the problem is fixed.\n" );
	exit( 1 );
}
# Make sure script is executable.  Don't give world/other permissions
# since it is meant for admin use only.
chmod( 0700, $irodsctl );





# Done!
if ( ! $noHeader )
{
	printNotice( "\nDone.\n" );
}


exit( 0 );







#
# @brief	Check if a command exists and is executable
#
# The given command is checked first to see if it is a full
# path to an executable.  If not, the user's path is scanned
# to find the command.
#
# @return	A numeric value:
# 			0 = command not found
# 			1 = command found
#
sub checkCommand($)
{
	my ($command) = @_;

	# If the command is a full path, does it exist as an executable?
	return 1 if ( -x $command );

	# Scan the path to find the command
	my @pathDirs = split( ':', $ENV{'PATH'} );
	foreach $pathDir (@pathDirs)
	{
		my $commandPath = File::Spec->catfile( $pathDir, $command );
		return 1 if ( -x $commandPath );
	}
	return 0;
}





#
# @brief	Choose a Perl for use by the Makefiles
#
# Return the name or path of the Perl command to be used by
# Makefiles.  If the irods.config file has set $PERL, and
# that points to an existing Perl command, use that.
# Otherwise use the same Perl this script was run with.
#
# Output an error message and exit if there is a problem.
#
# @return	the name or path of Perl
#
sub choosePerl()
{
	# Default to the Perl version running this script.  This is the
	# preferred way to go.
	if ( !defined( $PERL ) || $PERL eq "" )
	{
		$PERL = $perl;
		return $PERL;
	}

	# Use $PERL set in irods.config, if the command exists.
	return $PERL if ( checkCommand( $PERL ) );

	printError(
		"\n",
		"Configuration problem:\n",
		"    The Perl interpreter chosen in the iRODS configuration file does\n",
		"    not exist.  Please check your setting for the \$PERL variable\n",
		"    or leave it empty to use a default.\n",
		"        Perl:  $PERL\n",
		"        File:  $irodsConfig\n",
		"\n",
		"Abort.  Please re-run this script when the problem is fixed.\n" );
	exit( 1 );
}





#
# @brief	Choose a C compiler and loader for use by the Makefiles
#
# Return the name or path of the cc command and loader to be used by
# Makefiles.  If the irods.config file has set $CC, and that points
# to an existing and working C compiler, use that.  Otherwise look
# for the C compiler.  If $LDR points to an existing loader, use that.
# Otherwise use the C compiler.
#
# Output an error message and exit if there is a problem.
#
# @return	a 3-tuple including:
# 			the name or path of cc
# 			a 0 or 1 flag indicating if cc is gcc
#			the name or path of the loader
#
sub chooseCompiler()
{
	my $ccDiscovered = 1;
	my $ccIsGcc = 0;

	if ( !defined( $CC ) || $CC eq "" )
	{
		# Default to searching for the C compiler.
		#
		# On Solaris, look for the SunPro C compiler.
		if ( $thisOS =~ /(sunos)|(solaris)/i )
		{
			# Prefer the SunPro compiler, if available.
			#
			# Unfortunately, there is no standard place
			# for it to be installed except that the
			# installation's parent directory is usually
			# 'SUNWspro'.  So, let's look for that in
			# a few standard locations.
			my $root = File::Spec->rootdir( );
			my @dirs = (
				File::Spec->catdir( $root ),
				File::Spec->catdir( $root, "opt" ),
				File::Spec->catdir( $root, "usr" ),
				File::Spec->catdir( $root, "usr", "opt" ),
				File::Spec->catdir( $root, "usr", "local" ),
			);
			foreach $dir (@dirs)
			{
				my $d = File::Spec->catdir( $dir, "SUNWspro" );
				next if ( ! -d $d );

				# Directory exists!  Look for bin/cc
				my $try = File::Spec->catfile( $d, "bin", "cc" );
				if ( -x $try )
				{
					$CC = $try;
					last;
				}
				# Otherwise, false alarm.  Keep looking.
			}
			# If CC is not set by the loop above, fall through
			# and look for standard compilers.
		}

		if ( !defined( $CC ) || $CC eq "" )
		{
			# Look for gcc.
#			$CC = findCommand( "gcc" );
			$CC = findCommand( "g++" );
			if ( !defined( $CC ) || $CC eq "" )
			{
				# Look for cc.  Could fail.  Could find gcc
				# pretending to be cc (Mac OS X and Linux
				# do this).
				$CC = findCommand( "cc" );
                printError( "\n", "Failed to locate g++ compiler.  checking out." );
                exit( 1 );
			}
			else
			{
				$ccIsGcc = 1;
			}
		}

		# Complain if we didn't find anything.
		if ( !defined( $CC ) || $CC eq "" )
		{
			printError(
				"\n",
				"Configuration problem:\n",
				"    Cannot find a C compiler.  You can select a specific\n",
				"    C compiler by setting the \$CC variable in the iRODS\n",
				"    configuration file.\n",
				"        File:  $irodsConfig\n",
				"\n",
				"Abort.  Please re-run this script when the problem is fixed.\n" );
			exit( 1 );
		}
	}
	elsif ( ! checkCommand( $CC ) )
	{
		# irods.config sets $CC, but the file doesn't exist.
		printError(
			"\n",
			"Configuration problem:\n",
			"    The C compiler chosen in the iRODS configuration file does\n",
			"    not exist.  Please check your setting for the \$CC variable\n",
			"    or leave it empty to use a default.\n",
			"        CC:    $CC\n",
			"        File:  $irodsConfig\n",
			"\n",
			"Abort.  Please re-run this script when the problem is fixed.\n" );
		exit( 1 );
	}
	else
	{
		# Use the irods.config choice
		$ccDiscovered = 0;
	}


	# Check that CC really is a compiler by creating a
	# brief C program and trying to compile it.
	my $startingDir = cwd( );
	chdir( File::Spec->tmpdir( ) );
	my $cctemp = "irods_cc_$$.c";
	my $cctempObj = "irods_cc_$$.o";
	printToFile( $cctemp,
		"int main(int argc,char** argv) { int junk = argc; }\n" );
	$output = `$CC -c $cctemp 2>&1`;
	if ( $? != 0 )
	{
		# Compilation failed.  On Sun platforms, sometimes "cc" is
		# a script that simply prints that the user should go buy
		# a C compiler.  In this case, while "cc" will exist, it
		# won't compile and we'll get an error.
		unlink( $cctemp );
		unlink( $cctempObj);
		chdir( $startingDir );
		if ( $ccDiscovered == 1 )
		{
			printError(
				"\n",
				"Configuration problem:\n",
				"    Cannot find a working C compiler.  You can select a specific\n",
				"    C compiler by setting the \$CC variable in the iRODS\n",
				"    configuration file.\n",
				"        File:  $irodsConfig\n",
				"\n",
				"Abort.  Please re-run this script when the problem is fixed.\n" );
			exit( 1 );
		}
		printError(
			"\n",
			"Configuration problem:\n",
			"    The C compiler chosen in the iRODS configuration file did not\n",
			"    work.  Please check this setting and check that the compiler\n",
			"    works.  Some vendors install a 'cc' shell script that merely\n",
			"    prints a warning that a C compiler is not installed.\n",
			"        CC:    $CC\n",
			"        File:  $irodsConfig\n",
			"\n",
			"Abort.  Please re-run this script when the problem is fixed.\n" );
		exit( 1 );
	}
	unlink( $cctemp );
        unlink( $cctempObj);
	chdir( $startingDir );


	# Check if CC is some form of gcc.  The Makefiles
	# sometimes use 'if' checks to include special
	# compiler flags for gcc.
	if ( $ccIsGcc == 0 )
	{
		if ( $CC =~ /gcc/i )
		{
			# Name contains 'gcc'.
			$ccIsGcc = 1;
		}
		else
		{
			# Linux and Mac OS X often install 'gcc' as 'cc'
			$output = `$CC -v 2>&1`;
			if ( $output =~ /gcc version/i )
			{
				# Version information revealed it was gcc
				$ccIsGcc = 1;
			}
		}
	}


	# Now choose a loader.
	if ( !defined( $LDR ) || $LDR eq "" )
	{
		# Default to the C compiler.  This is the preferred choice.
		$LDR = $CC;
	}
	elsif ( ! checkCommand( $LDR ) )
	{
		# irods.config sets $LDR, but the file doesn't exist.
		printError(
			"\n",
			"Configuration problem:\n",
			"    The loader chosen in the iRODS configuration file does\n",
			"    not exist.  Please check your setting for the \$LDR variable\n",
			"    or leave it empty to use a default.\n",
			"        LDR:   $LDR\n",
			"        File:  $irodsConfig\n",
			"\n",
			"Abort.  Please re-run this script when the problem is fixed.\n" );
		exit( 1 );
	}

	return ($CC, $ccIsGcc, $LDR);
}





#
# @brief	Choose an archiver for use by the Makefiles
#
# Return the name or path of the ar command to be used by
# Makefiles.  If the irods.config file has set $AR, and
# that points to an existing archiver, use that.
# Otherwise look for it.
#
# Output an error message and exit if there is a problem.
#
# @return	the name or path of ar
#
sub chooseArchiver()
{
	if ( defined( $AR ) && $AR ne "" )
	{
		# irods.config sets $AR
		return $AR if ( checkCommand( $AR ) );

		printError(
			"\n",
			"Configuration problem:\n",
			"    The archiver chosen in the iRODS configuration file does\n",
			"    not exist.  Please check your setting for the \$AR variable\n",
			"    or leave it empty to use a default.\n",
			"        AR:    $AR\n",
			"        File:  $irodsConfig\n",
			"\n",
			"Abort.  Please re-run this script when the problem is fixed.\n" );
		exit( 1 );
	}

	# No command chosen.  Look for it.

	# On Solaris, look in /usr/xpg4 first if we are using
	# 64-bit addressing.
	if ( ( $thisOS =~ /(sunos)|(solaris)/i ) &&
		defined( $configMkVariables{ "ADDR_64BITS" } ) )
	{
		$AR = File::Spec->catfile( File::Spec->rootdir( ),
			"usr", "xpg4", "bin", "ar" );
		return $AR if ( -x $AR );
	}

	$AR = findCommand( "ar" );
	return $AR if ( defined( $AR ) );

	printError(
		"\n",
		"Configuration problem:\n",
		"    Cannot find the 'ar' command.  You can select a specific\n",
		"    archiver by setting the \$AR variable in the iRODS\n",
		"    configuration file.\n",
		"        File:  $irodsConfig\n",
		"\n",
		"Abort.  Please re-run this script when the problem is fixed.\n" );
	exit( 1 );
}




#
# @brief	Choose a ranlib for use by the Makefiles
#
# Return the name or path of the ranlib command to be used by
# Makefiles.  If the irods.config file has set $RANLIB, and
# that points to an existing ranlib, use that.
# Otherwise look for it.  If not found, use touch instead.
#
# Output an error message and exit if there is a problem.
#
# @return	the name or path of ar
#
sub chooseRanlib()
{
	if ( defined( $RANLIB ) && $RANLIB ne "" )
	{
		# irods.config sets $RANLIB
		return $RANLIB if ( checkCommand( $RANLIB ) );

		printError(
			"\n",
			"Configuration problem:\n",
			"    The 'ranlib' chosen in the iRODS configuration file does\n",
			"    not exist.  Please check your setting for the \$RANLIB variable\n",
			"    or leave it empty to use a default.\n",
			"        RANLIB: $RANLIB\n",
			"        File:   $irodsConfig\n",
			"\n",
			"Abort.  Please re-run this script when the problem is fixed.\n" );
		exit( 1 );
	}


	$RANLIB = findCommand( "ranlib" );
	return $RANLIB if ( defined( $RANLIB ) );

	# When 'ranlib' is not found, it usually means that the
	# OS automatically creates a random access library.  So,
	# just use 'touch' insteasd.
	$RANLIB = findCommand( "touch" );
	return $RANLIB if ( defined( $RANLIB ) );

	printError(
		"\n",
		"Configuration problem:\n",
		"    Cannot find the 'ranlib' command.  You can select a specific\n",
		"    ranlib by setting the \$RANLIB variable in the iRODS\n",
		"    configuration file.\n",
		"        File:  $irodsConfig\n",
		"\n",
		"Abort.  Please re-run this script when the problem is fixed.\n" );
	exit( 1 );
}
