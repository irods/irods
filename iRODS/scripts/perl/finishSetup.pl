#
# Perl

#
# Finish setting up iRODS and test it.  This is the last step
# in an iRODS installation.
#
# Usage is:
#	perl finishSetup.pl [options]
#
# Help options:
#	--help      Show a list of options
#
# Verbosity options:
# 	--quiet     Suppress all messages
# 	--verbose   Output all messages (default)
# 	--force     Force all actions
#
#
# This Perl script completes the installation of iRODS by creating
# the iRODS database and tables (if needed), initializing
# those tables, and setting up the user's initial iRODS account.
# iRODS tests are run to insure that everything is working.
#
#	** There are no user-editable parameters in this file. **
#

use File::Spec;
use File::Copy;
use Cwd;
use Cwd "abs_path";
use Config;

$version{"finishSetup.pl"} = "Oct 2010";





#
# Design Notes:  Perl and OS compatability
#	This script is written to work on Perl 5.0+ and on Linux,
#	Mac, and Solaris.  Perl features intentionally not used
#	because they are not available on Perl 5.0 include:
#
#		"\s"  in regular expressions is supposed to match
#		all white space characters.  It fails in older Perl.
#
#		"use Errno;" to get error messages fails in older Perl.
#
#		File handle variables.  They fail in older Perl.
#
#		"qr/.../" is supposed to define a compiled regular
#		expression that can be passed as an argument.  It
#		fails with a syntax error in older Perl.
#





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
	$IRODS_HOME = File::Spec->catdir( $IRODS_HOME, File::Spec->updir( ));
	$configDir  = File::Spec->catdir( $IRODS_HOME, "config" );
	if ( ! -e $configDir )
	{
		$IRODS_HOME = File::Spec->catdir( $IRODS_HOME, File::Spec->updir( ));
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
my $currentPort = 0;

# Load support scripts.
my $perlScriptsDir = File::Spec->catdir( $IRODS_HOME, "scripts", "perl" );
require File::Spec->catfile( $perlScriptsDir, "utils_paths.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_print.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_file.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_platform.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_config.pl" );
my $irodsctl = File::Spec->catfile( $perlScriptsDir, "irodsctl.pl" );

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
my %thisHostAddresses = getCurrentHostAddresses( );

# Name a log file.
my $logFile = File::Spec->catfile( $logDir, "finishSetup.log" );

# Set the number of seconds to sleep after starting or stopping the
# database server.  This gives it time to start or stop before
# we do anything more.  The actual number here is a guess.  Different
# CPU speeds, host loads, or database versions may make this too little
# or too much.
my $databaseStartStopDelay = 8;		# Seconds

# Set the default iRODS boot administrator name and password.
# These are the defaults when iRODS is first installed.  After
# the first user account has been created (done in this script),
# these aren't needed any more.  The boot account's password is
# later set to that first user account's password.
my $IRODS_ADMIN_BOOT_NAME     = "rodsBoot";
my $IRODS_ADMIN_BOOT_PASSWORD = "RODS";

# iRODS Server names (don't change these names without also changing
# code later in this file that depends upon them).
%servers = (
	# Nice name		process name
	"iRODS agents" =>	"(irodsAge)(nt)?",
	"iRODS rule servers" =>	"(irodsReS)(erver)?",
	"iRODS servers" =>	"(irodsSer)(ver)?"

	# Process names need to be able to match on just 8 characters,
	# due to limitations of SysV /bin/ps (such as Solaris) which
	# only shows the 1st 8 characters.
);





########################################################################
#
# Check script usage.
#

# Don't allow root to run this.
if ( $thisUserID == 0 )
{
	printError( "Usage error:\n" );
	printError( "    This script should *not* be run as root.\n" );
	exit( 1 );
}


# Initialize global flags.
my $force = 0;
my $noAsk = 0;
my $noHeader = 0;
my $isUpgrade = 0;
setPrintVerbose( 1 );


# Parse the command-line.
my $arg;
foreach $arg (@ARGV)
{
	if ( $arg =~ /^-?-?h(elp)$/ )		# Help / usage
	{
		printUsage( );
		exit( 0 );
	}
	if ( $arg =~ /^-?-?q(uiet)$/ )		# Suppress output
	{
		setPrintVerbose( 0 );
		next;
	}
	if ( $arg =~ /^-?-?v(erbose)$/ )	# Enable output
	{
		setPrintVerbose( 1 );
		next;
	}
	if ( $arg =~ /^-?-?noa(sk)$/i )		# Suppress questions
	{
		$noAsk = 1;
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

	if ( $arg =~ /^-?-?(force)$/ )		# Force all actions
	{
		$force = 1;
		next;
	}
	if ( $arg =~ /-?-?upgrade/ ) 	# Upgrade mode
	{
		$isUpgrade = 1;
		next;
	}

	printError( "Unknown option:  $arg\n" );
	printUsage( );
	exit( 1 );
}





########################################################################
#
# Confirm that prior setup stages have probably run.
#
# Make sure config/config.mk exists.  It is created by the 'configure'
# script, which must be run first.  If the file doesn't exist, it
# probably means scripts are being run out of order or that a prior
# stage failed.
if ( ! -e $configMk )
{
	printError( "Usage error:\n" );
	printError( "    The file 'config.mk' is missing.  This probably\n" );
	printError( "    means that prior setup stages have not been\n" );
	printError( "    completed yet.\n" );
	printError( "\n    Please run ./setup.\n" );
	exit( 1 );
}

# Make sure the i-command binaries exist.  They are built during 'make',
# which should already have occurred.
if ( ! -e $iadmin )
{
	printError( "Usage error:\n" );
	printError( "    The file 'iadmin' is missing.  This probably\n" );
	printError( "    means that prior setup stages have not been\n" );
	printError( "    completed yet.\n" );
	printError( "\n    Please run ./setup.\n" );
	exit( 1 );
}





########################################################################
#
# Make sure the user wants to run this script.
#

if ( ! $noHeader )
{
	printTitle( "Finish setting up iRODS\n" );
	printTitle( "------------------------------------------------------------------------\n" );

	if ( ! $noAsk )
	{
		printNotice(
			"This script finishes setting up iRODS.  Depending upon your chosen\n",
			"configuration, it sets up the database and finishes setting iRODS\n",
			"settings..  It does not require administrator privileges.\n",
			"\n" );
		if ($force) 
		{
		    printNotice(
			"Also note that since you are using the --force option this script\n",
			"will also drop the database and/or the database tables destroying all\n",
			"state information.  Only continue if you want your iRODS system to start\n",
			"over completely.\n",
			"\n" );
		}
		if ( askYesNo( "    Continue (yes/no)?  " ) == 0 )
		{
			printNotice( "Canceled.\n" );
			exit( 1 );
		}
	}
}





########################################################################
#
# Load and validate irods.config.
#
# This function sets a large number of important global variables based
# upon values in the irods.config file.  Those include the type of
# database in use (if any), the path to that database, the host and
# port for the database, and the initial account name and password
# for the database and iRODS.
#
# This function also validates that the values look reasonable and
# prints messages if they do not.
#
copyTemplateIfNeeded( $irodsConfig );
if ( loadIrodsConfig( ) == 0 )
{
	# Configuration failed to load or validate.  An error message
	# has already been output.
	exit( 1 );
}

# Make sure the home directory is set and valid.  If not, the installation
# is probably being run out of order or a prior stage failed.
if ( $IRODS_HOME eq "" || ! -e $IRODS_HOME )
{
	printError( "Usage error:\n" );
	printError( "    The IRODS_HOME setting in 'irods.config' is empty\n" );
	printError( "    or incorrect.  This probably means that prior\n" );
	printError( "    setup stages have not been completed yet.\n" );
	printError( "\n    Please run ./setup.\n" );
	exit( 1 );
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

# Because the log file contains echoed account names and passwords, we
# change permissions on it so that only the current user can read it.
chmod( 0600, $logFile );

printLog( "Finishing the iRODS setup\n" );
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
# Finish setting up iRODS.
#

my $totalSteps  = 0;
my $currentStep = 0;

# Initially, assume a database restart is not needed.  Any of the
# steps that might change database configuration files will change
# this flag to indicate that a restart is needed.  That restart
# happens just before configuring the iRODS server.
my $databaseRestartNeeded = 0;

if ( $IRODS_ADMIN_NAME eq "" )
{
	# There is no administrator account name.  Probably no
	# password either.  Without these, we cannot do any
	# server configuration.  This case occurs when the
	# build scripts are set to only build and install the
	# i-commands, so this is not an error.
	$totalSteps = 0;
	$currentStep = 0;

	# Nothing to do.  Fall through.
}
elsif ( $DATABASE_TYPE eq "" )
{
	# There is no database.  This means we are not installing
	# the iCAT on this host, so there is no need to create the
	# database schema and tables, or adjust the database's
	# configuration files.
	$totalSteps  = 3;
	$currentStep = 0;

	# 1.  Prepare by starting/stopping servers.
	prepare( );

	# 2.  Configure 
	configureIrodsServer( );

	# 3.  Configure the iRODS user account.
	configureIrodsUser( );
}
else
{
	# There is a database.  We need to configure it.
	$totalSteps  = 6;
	if ($isUpgrade) {$totalSteps=3;}
	$currentStep = 0;

	# Ignore the iCAT host name, if any, if we
	# are using a database.
	$IRODS_ICAT_HOST = "";

	# 1.  Prepare by starting/stopping servers.
	prepare( );

	# 2.  Set up the user account for the database.
	if (!$isUpgrade) {
	    configureDatabaseUser( );
	}

	# 3.  Create the database schema and tables.
	if (!$isUpgrade) {
	    createDatabaseAndTables( );
	}

	# 4.  Configure database security settings and restart.
	if (!$isUpgrade) {
	    configureDatabaseSecurity( );
	    restartDatabase( );
	    testDatabase( );
	}

	# 5.  Configure 
	configureIrodsServer( );

	# 6.  Configure the iRODS user account.
	configureIrodsUser( );
}

# Done.
printLog( "\nDone.\n" );
printLog( getCurrentDateTime( ) . "\n" );
closeLog( );
if ( ! $noHeader )
{
	printSubtitle( "\nDone.  Additional detailed information is in the log file:\n" );
	printSubtitle( "    $logFile\n" );
}
exit( 0 );










########################################################################
#
# Setup steps.
#
#	All of these functions execute individual steps in the
#	database setup.  They all output error messages themselves
#	and may exit the script directly.
#

#
# @brief	Prepare to initialize.
#
# Stop iRODS, if it is running, and start the database, if it is not.
# Test that we can communicate with the database.
#
# Output error messages and exit on problems.
#
sub prepare()
{
	++$currentStep;
	printSubtitle( "\nStep $currentStep of $totalSteps:  Preparing to initialize...\n" );
	printLog( "\nPreparing to initialize...\n" );
	printLog( "------------------------------------------------------------------------\n" );
	printLog( getCurrentDateTime( ) . "\n\n" );


	# Stop iRODS if it is running.
	printStatus( "Stopping iRODS server...\n" );
	printLog( "Stopping iRODS server...\n" );
	$currentPort = $IRODS_PORT;
	my $status = stopIrods( );
	if ( $status == 0 )
	{
		cleanAndExit( 1 );
	}
	if ( $status == 2 )
	{
		# Already started.
		printStatus( "    Skipped.  Server already stopped.\n" );
		printLog( "    Skipped.  Server already stopped.\n" );
	}



	# Start database if it isn't running.
	if ( $controlDatabase )
	{
		printStatus( "Starting database server...\n" );
		printLog( "\nStarting database server...\n" );
		$status = startDatabase( );
		if ( $status == 0 )
		{
			cleanAndExit( 1 );
		}
		if ( $status == 2 )
		{
			printStatus( "    Skipped.  Server already running.\n" );
			printLog( "    Skipped.  Server already running.\n" );
		}
	}
}






#
# @brief	Configure database user.
#
# Depending upon the database type, set up the user's account for the
# iRODS database.
#
# Output error messages and exit on problems.
#
sub configureDatabaseUser
{
	++$currentStep;
	printSubtitle( "\n" );
	printSubtitle( "Step $currentStep of $totalSteps:  Configuring database user...\n" );
	printLog( "\n" );
	printLog( "Configuring database user...\n" );
	printLog( "------------------------------------------------------------------------\n" );
	printLog( getCurrentDateTime( ) . "\n\n" );


	# Postgres
	if ( $DATABASE_TYPE eq "postgres" )
	{
		Postgres_ConfigureDatabaseUser( );
		Postgres_CreateAlternateDatabaseUser( );
		return;
	}

	# Oracle
	if ( $DATABASE_TYPE eq "oracle" )
	{
		printStatus( "Skipped.  For Oracle, database configured by DBA.\n" );
		printLog( "Skipped.  For Oracle, database configured by DBA.\n" );
		return;
	}

	# MySQL
	if ( $DATABASE_TYPE eq "mysql" )
	{
		printStatus( "Skipped.  For MySQL, database configured by DBA.\n" );
		printLog( "Skipped.  For MySQL, database configured by DBA.\n" );
		return;
	}

	# Empty
	if ( $DATABASE_TYPE eq "" )
	{
		printStatus( "    Skipped.  No database to configure.\n" );
		printLog( "    Skipped.  No database to configure.\n" );
		return;
	}


	# Otherwise skip it.
	printStatus( "    Skipped.  Unsupported database type:  $DATABASE_TYPE.\n" );
	printLog( "    Skipped.  Unsupported database type:  $DATABASE_TYPE.\n" );
}








#
# @brief	Create database schema and tables.
#
# Create the iCAT database schema, then initialize it with the
# iCAT tables.
#
# Output error messages and exit on problems.
#
sub createDatabaseAndTables
{
	++$currentStep;
	printSubtitle( "\n" );
	printSubtitle( "Step $currentStep of $totalSteps:  Creating database and tables...\n" );
	printLog( "\n" );
	printLog( "Creating database and tables...\n" );
	printLog( "------------------------------------------------------------------------\n" );
	printLog( getCurrentDateTime( ) . "\n\n" );

	if ( $DATABASE_TYPE eq "" )
	{
		# No database, therefore no iCAT to configure on this host.
		printStatus( "    Skipped.  No database to configure.\n" );
		printLog( "    Skipped.  No database to configure.\n" );
		return;
	}

	my $status;
	my $output;



	# Create the database.
	#	This step is skipped if the database already exists, and
	#	we're not in FORCE mode.
	#
	#	On a fresh database, the user's account doesn't have a
	#	password yet and database creation doesn't prompt for one.
	#	But if the database already exists, or if this script ran
	#	before and failed part way through, then the user's
	#	account may now have a password and they'll be prompted.
	$status = createDatabase( );
	if ( $status == 0 )
	{
		cleanAndExit( 1 );
	}
	if ( $status == 2 )
	{
		# The database is already there.  Chances are very good that
		# the tables and other SQL initializations have already
		# occurred too.  We cannot run them again without risking
		# corrupting the tables.  So, stop now.
		printStatus( "Creating database and tables...\n" );
		printStatus( "    Skipped.  Database and tables already created.\n" );
		printLog( "    Skipped.  Database and tables already created.\n" );
		return;
	}



	# Give the user access to the iCAT database.
	#	Set the user's password.
	#
	#	If this script has been run before, then the
	#	password may already be set.  But we can't tell.
	#	So just do it again.
	if ( $DATABASE_TYPE eq "postgres" )
	{
		printStatus( "Setting database user password...\n" );
		printLog( "Setting database user password...\n" );
		my $tmpSql = createTempFilePath( "sql" );

		my $sql = "alter user \"$DATABASE_ADMIN_NAME\" with password '$DATABASE_ADMIN_PASSWORD';";
		printLog( "    $sql\n" );

		# Create an empty file first and make it unreadable by others
		printToFile( $tmpSql, "\n" );
		chmod( 0600, $tmpSql );

		# Now add the password line(s)
		appendToFile( $tmpSql, "$sql\n" );
		# Set the encoding to be LATIN1.  This avoids a problem
		# in the current Postgres ODBC code that doesn't handle
		# non-latin encodings yet.
		$sql = "alter user $DATABASE_ADMIN_NAME set client_encoding to LATIN1;";
		printLog( "    $sql\n" );
		appendToFile( $tmpSql, "$sql\n" );

		($status,$output) = execute_sql( $DB_NAME, $tmpSql );
		unlink( $tmpSql );
		if ( $status != 0 )
		{
			 printError( "\nInstall problem:\n" );
			 printError( "    Could not set database password.\n" );
			 printError( "        ", $output );
			 printLog( "\nSQL failed:\n" );
			 printLog( "    ", $output );
			 cleanAndExit( 1 );
		}
	}


	# Create the tables.
	#	The iCAT SQL files issue a number of instructions to
	#	create tables and initialize state.  If this script
	#	has been run before, then these tables will already be
	#	there.  Running the SQL instructions a second time
	#	will corrupt the database.  So, we need to check first.
	printStatus( "Creating iCAT tables...\n" );
	printLog( "Creating iCAT tables...\n" );

	my $tables = listDatabaseTables( $DB_NAME );
	if ( ! defined( $tables ) )
	{
		printError( "\nInstall problem:\n" );
		printError( "    Could not list tables in the database:\n" );
		printError( "        ", $output );
		printLog( "\nSQL failed:\n" );
		printLog( "    ", $output );
		cleanAndExit( 1 );
	}

	printLog( "    List of existing tables (if any):\n" );
	my $tablesAlreadyCreated = 0;
	my @lines = split( "\n", $tables );
	my $line;
	foreach $line ( @lines )
	{
		# iRODS table names mostly start with RCORE_
		if ( $line =~ /rcore_/i )
		{
			$tablesAlreadyCreated = 1;
			printLog( "        $line (<-- for iRODS)\n" );
		}
		else
		{
			printLog( "        $line\n" );
		}
	}


	# Run the SQL if the tables aren't already there.
	if ( ! $tablesAlreadyCreated )
	{
		my @sqlfiles = (
			"icatCoreTables.sql",
			"icatSysTables.sql",
			"icatCoreInserts.sql",
			"icatSysInserts.sql" );

		if ( $ZONE_NAME ne $IRODS_DEFAULT) {
		    printStatus( "    Converting zone name in icatSysInserts.sql\n" );
		    printLog( "    Converting zone name in icatSysInserts.sql\n" );
		    my $sqlPath = File::Spec->catfile( $serverSqlDir, 
						       "icatSysInserts.sql");
		    my $sqlPathOrig = File::Spec->catfile( $serverSqlDir, 
						       "icatSysInserts.sql.orig");
		    if (!-e $sqlPathOrig) {
			rename($sqlPath, $sqlPathOrig);
		    }
		    unlink($sqlPath);
		    `cat $sqlPathOrig | sed s/$IRODS_DEFAULT_ZONE/$ZONE_NAME/g > $sqlPath`;
		}
		
		printStatus( "    Converting SQL to $DATABASE_TYPE form.\n" );
		printLog( "    Converting SQL to $DATABASE_TYPE form.\n" );
		my $convertSqlScript = File::Spec->catfile( $serverSqlDir,"convertSql.pl");
		($status,$output) = run( "$perl $convertSqlScript $DATABASE_TYPE $serverSqlDir" );
		if ( $status != 0 )
		{
		    printError( "\nInstall problem:\n" );
		    printError( "    Could not run $convertSqlScript\n" );
		    printError( "        ", $output );
		    printLog( "\nInstall problem:\n" );
		    printLog( "    Could not run $convertSqlScript\n" );
		    printLog( "    ", $output );
		    cleanAndExit( 1 );
		}
    
		my $alreadyCreated = 0;
		my $sqlfile;
		printStatus( "    Inserting ICAT tables...\n" );
		printLog( "    Inserting ICAT tables...\n" );
		foreach $sqlfile (@sqlfiles)
		{
			printLog( "    $sqlfile...\n" );
			my $sqlPath = File::Spec->catfile( $serverSqlDir, $sqlfile );
			($status,$output) = execute_sql( $DB_NAME, $sqlPath );
			if ( $status != 0 )
			{
				# Stop if any of the SQL scripts fails.
				printError( "\nInstall problem:\n" );
				printError( "    Could not create the iCAT tables.\n" );
				printError( "        ", $output );
				printLog( "\nSQL failed:\n" );
				printLog( "    ", $output );
				cleanAndExit( 1 );
			}
			printLog( "        ", $output );

			if ( $output =~ /error/i )
			{
				printError( "\nInstall problem:\n" );
				printError( "    Creation of the iCAT tables failed.\n" );
				printError( "        ", $output );
				printLog( "\nSQL error output:\n" );
				printLog( "    ", $output );
				cleanAndExit( 1 );
			}
		}

                # Now apply the site-defined ICAT tables, if any 
		my $sqlPath = File::Spec->catfile( $extendedIcatDir, "icatExtTables.sql" );
		if (-e $sqlPath) {
		    printStatus( "    Inserting ICAT Extension tables...\n" );
		    printLog( "    Inserting ICAT Extension tables...\n" );
		    printLog( "    $sqlPath...\n" );
		    ($status,$output) = execute_sql( $DB_NAME, $sqlPath );
		    if ( $status != 0 || $output =~ /error/i )
		    {
			printStatus( "\nInstall problem (but continuing):\n" );
			printStatus( "    Could not create the extended iCAT tables.\n" );
			printStatus( "        ", $output );
			printLog( "\nInstall problem:\n" );
			printLog( "    Could not create the extended iCAT tables.\n" );
		    }
		    printLog( "        ", $output );
		}
		my $sqlPath = File::Spec->catfile( $extendedIcatDir, "icatExtInserts.sql" );
		if (-e $sqlPath) {
		    printStatus( "    Inserting ICAT Extension table rows...\n" );
		    printLog( "    Inserting ICAT Extension table rows...\n" );
		    printLog( "    $sqlPath...\n" );
		    ($status,$output) = execute_sql( $DB_NAME, $sqlPath );
		    if ( $status != 0 || $output =~ /error/i )
		    {
			printStatus( "\nInstall problem (but continuing):\n" );
			printStatus( "    Could not insert the extended iCAT rows.\n" );
			printStatus( "        ", $output );
			printLog( "\nInstall problem:\n" );
			printLog( "    Could not insert the extended iCAT tables.\n" );
		    }
		    printLog( "        ", $output );
		}

#		

	}
	if ( $tablesAlreadyCreated )
	{
		printStatus( "    Skipped.  Tables already created.\n" );
		printLog( "    Skipped.  Tables already created.\n" );
	}

	# Create the Audit Extension SQL items if requested.
	if ($AUDIT_EXT == 1) {
		printStatus( "Inserting Audit Extensions...\n" );
		printLog( "Inserting Audit Extensions...\n" );
		my $auditExtSqlFile = File::Spec->catfile( $serverAuditExtSql,
				       "ext.sql" );
		copyTemplateIfNeeded( $auditExtSqlFile );
		my %variables = (
		    "rodsbuild", $DATABASE_ADMIN_NAME );

		($status,$output) = replaceVariablesInFile( $auditExtSqlFile,
				    "simple", 0, %variables );
		if ( $status == 0 )
		{
		    printError( "\nInstall problem:\n" );
		    printError( "    Cannot create ext.sql file.\n" );
		    printLog( "\nCannot create ext.sql file.\n" );
		    cleanAndExit( 1 );
		}

		($status,$output) = execute_sql( $DB_NAME, $auditExtSqlFile );
		if ( $status != 0 )
		{
		    # Stop if it failed.
		    printError( "\nInstall problem:\n" );
		    printError( "    Could not create the iCAT AuditExttables.\n" );
		    printError( "        ", $output );
		    printLog( "\nSQL failed:\n" );
		    printLog( "    ", $output );
		    cleanAndExit( 1 );
		}
		printLog( "        ", $output );
	}
}





#
# @brief	Configure database security settings.
#
# Different databases have different configuration files for controlling
# who can connect and how.  Here we adjust those based upon the selected
# database.
#
# Output error messages and exit on problems.
#
sub configureDatabaseSecurity
{
	++$currentStep;
	printSubtitle( "\n" );
	printSubtitle( "Step $currentStep of $totalSteps:  Configuring database security...\n" );
	printLog( "\n" );
	printLog( "Configuring database security...\n" );
	printLog( "------------------------------------------------------------------------\n" );
	printLog( getCurrentDateTime( ) . "\n\n" );


	# Postgres
	if ( $DATABASE_TYPE eq "postgres" )
	{
		if ( $controlDatabase )
		{
			Postgres_ConfigureDatabaseSecurity( );
		}
		else
		{
			printStatus( "Configuring security...\n" );
			printStatus( "    Skipped.  Existing Postgres configured by DBA.\n" );
			printLog( "    Skipped.  Existing Postgres configured by DBA.\n" );
		}
		return;
	}

	# Oracle
	if ( $DATABASE_TYPE eq "oracle" )
	{
		printStatus( "Configuring security...\n" );
		printStatus( "    Skipped.  Oracle database security configured by DBA.\n" );
		printLog( "    Skipped.  Oracle database security configured by DBA.\n" );
		return;
	}

	# MySQL
	if ( $DATABASE_TYPE eq "mysql" )
	{
		printStatus( "Configuring security...\n" );
		printStatus( "    Skipped.  MySQL database security configured by DBA.\n" );
		printLog( "    Skipped.  MySQL database security configured by DBA.\n" );
		return;
	}

	# Empty
	if ( $DATABASE_TYPE eq "" )
	{
		printStatus( "Configuring security...\n" );
		printStatus( "    Skipped.  No database to configure.\n" );
		printLog( "    Skipped.  No database to configure.\n" );
		return;
	}


	# Otherwise skip it.
	printStatus( "Configuring security...\n" );
	printStatus( "    Skipped.  Unsupported database type:  $DATABASE_TYPE.\n" );
	printLog( "    Skipped.  Unsupported database type:  $DATABASE_TYPE.\n" );
}





#
# @brief	Restart database.
#
# After changes to the database configuration, restart it.
#
# Output error messages and exit on problems.
#
sub restartDatabase()
{
	return if ( ! $controlDatabase );

	if ( ! $databaseRestartNeeded )
	{
		printStatus( "Restarting database server...\n" );
		printStatus( "    Skipped.  Not needed.\n" );
		return;
	}

	printStatus( "Stopping database server...\n" );
	my $status = stopDatabase( );
	if ( $status == 0 )
	{
		# Databse wouldn't stop.
		cleanAndExit( 1 );
	}
	if ( $status == 2 )
	{
		print( "    Skipped.  Database already stopped.\n" );
	}

	printStatus( "Starting database server...\n" );
	$status = startDatabase( );
	if ( $status == 0 )
	{
		# Databse wouldn't start.
		cleanAndExit( 1 );
	}
	if ( $status == 2 )
	{
		print( "    Skipped.  Database already started.\n" );
	}
}





#
# @brief	Test database communications.
#
# After setting up the database, make sure we can talk to it.
#
# Output error messages and exit on problems.
#
sub testDatabase()
{
	# Make sure communications are working.
	# 	This simple test issues a few SQL statements
	# 	to the database, testing that the connection
	# 	works.  iRODS is uninvolved at this point.
	printStatus( "Testing database communications...\n" );
	printLog( "\nTesting database communications with test_cll...\n" );

	my $test_cll = File::Spec->catfile( $serverTestBinDir, "test_cll" );
	my ($status,$output) = run( "$test_cll $DATABASE_ADMIN_NAME '$DATABASE_ADMIN_PASSWORD'" );
	printLog( "    ", $output );

	if ( $output !~ /The tests all completed normally/i )
	{
		printError( "\nConfiguration problem:\n" );
		printError( "    Communications with the database failed.\n" );
		printError( "        ", $output );
		printLog( "\nTests failed.\n" );
		cleanAndExit( 1 );
	}
}









#
# @brief	Configure iRODS.
#
# Update the iRODS server.config file and create the default
# directories.
#
# This is made more complicated by a lot of error checking to
# see if each of these tasks has already been done.  If a task
# has been done, we skip it and try the next one.  This insures
# that we'll pick up wherever we left off from a prior failure.
#
# The last step here changes the user's account *AND* the
# boot account to the user's chosen password.  If this script
# gets run a second time, we have to notice that the original
# boot password may longer work.  In that case, we try the
# user's password.  And if that fails, there's not much we can do.
#
# Output error messages and exit on problems.
#
sub configureIrodsServer
{
	++$currentStep;
	printSubtitle( "\n" );
	printSubtitle( "Step $currentStep of $totalSteps:  Configuring iRODS server...\n" );
	printLog( "\n" );
	printLog( "Configuring iRODS server...\n" );
	printLog( "------------------------------------------------------------------------\n" );
	printLog( getCurrentDateTime( ) . "\n\n" );



	printStatus( "Updating iRODS server.config...\n" );
	printLog( "Updating iRODS server.config...\n" );

	# Generate the MD5 scrambled form of the database password.
	#	We will be setting database connections to use MD5
	#	scrambling below.  From then on, we'll need the
	#	scrambling password.
	printLog( "    Getting scrambled admin password...\n" );
	my $scrambledPassword = scramblePassword( $DATABASE_ADMIN_PASSWORD );
	if ( !defined( $scrambledPassword ) )
	{
		printError( "\nInstall problem:\n" );
		printError( "    Cannot scramble administrator password.\n" );
		printLog( "\nCannot scramble adminstrator password.\n" );
		cleanAndExit( 1 );
	}


	# Update/set the iRODS server configuration.
	#	Tell iRODS the database host (often this host),
	#	the database administrator's name, and their MD5
	#	scrambled database password.
	#
	#	If this script is run more than once, then the file
	#	may already be updated.  It's so small there is
	#	little point in checking for this first.  Just
	#	overwrite it with the correct values.
	my $serverConfigFile = File::Spec->catfile( $serverConfigDir,
		"server.config" );
	copyTemplateIfNeeded( $serverConfigFile );

	my $host = ($IRODS_ICAT_HOST eq "") ? $DATABASE_HOST : $IRODS_ICAT_HOST;

	my %variables = (
		"icatHost",	$host,
		"DBKey",	$DB_KEY,
		"DBUsername",	$DATABASE_ADMIN_NAME,
		"DBPassword",	$scrambledPassword );
	printLog( "    Updating server.config....\n" );
	printLog( "        icatHost   = $host\n" );
	printLog( "        DBKey      = $DB_KEY\n" );
	printLog( "        DBUsername = $DATABASE_ADMIN_NAME\n" );
	printLog( "        DBPassword = $scrambledPassword\n" );

	($status,$output) = replaceVariablesInFile( $serverConfigFile,
		"config", 1, %variables );
	if ( $status == 0 )
	{
		printError( "\nInstall problem:\n" );
		printError( "    Updating of iRODS server.config failed.\n" );
		printError( "        ", $output );
		printLog( "\nCannot update variables in server.config.\n" );
		printLog( "    ", $output );
		cleanAndExit( 1 );
	}
	chmod( 0600, $serverConfigFile );


	if ( $IRODS_ICAT_HOST ne "" )
	{
		# We are using an existing iRODS server with an
		# iCAT enabled, so we don't need to install tables or
		# set up user accounts.
		if ( ! -e $userIrodsDir )
		{
		    # User's iRODS configuration directory doesn't exist yet.
		    # This is needed for iinit to be able to save the pw file.
		    printLog( "    mkdir $userIrodsDir\n" );
		    mkdir( $userIrodsDir, 0700 );
		}

		# Set the four key irods user env variables so iinit
		# will not attempt to create .irodsEnv (and prompt and
		# hang).  The .irodsEnv is created properly later.
		$ENV{"irodsHost"}=$thisHost;
		$ENV{"irodsPort"}=$IRODS_PORT;
		$ENV{"irodsUserName"}=$IRODS_ADMIN_NAME;
		$ENV{"irodsZone"}=$ZONE_NAME;

		printStatus( "Running 'iinit' to enable server to server connections...\n" );
		printLog( "Running 'iinit' to enable server to server connections...\n" );
		my ($status,$output) = run( "$iinit $IRODS_ADMIN_PASSWORD" );

		printStatus( "Using ICAT-enabled server on '$IRODS_ICAT_HOST'\n" );
		printLog( "Using ICAT-enabled server on '$IRODS_ICAT_HOST'\n" );

		# Unset environment variables temporarily set above.
		delete $ENV{"irodsHost"};
		delete $ENV{"irodsPort"};
		delete $ENV{"irodsUserName"};
		delete $ENV{"irodsZone"};
		return;
	}

	# Set the name of the authorization file created
	# by 'iinit' when using this boot environment.
	#	By default, this is 'install/auth.tmp',  We
	#	change it here to a full path so that it isn't
	#	dependent upon the directory from which this
	#	script is run.
	printLog( "\nUpdating iRODS irodsEnv.boot...\n" );
	my $bootEnv  = File::Spec->catfile( $configDir, "irodsEnv.boot" );
	my $authFile = File::Spec->catfile( $configDir, "auth.tmp" );
	my %envVariables = ( "irodsAuthFileName", $authFile, "irodsZone", $ZONE_NAME) ;
	printLog( "    irodsAuthFileName = $authFile\n" );

	($status,$output) = replaceVariablesInFile( $bootEnv, "config", 1, %envVariables );
	if ( $status == 0 )
	{
		printError( "\nInstall problem:\n" );
		printError( "    Updating of iRODS irodsEnv.boot failed.\n" );
		printError( "        ", $output );
		printLog( "\nCannot update variables in irodsEnv.boot.\n" );
		printLog( "    ", $output );
		cleanAndExit( 1 );
	}


	# Start iRODS.
	#	We need the server started so that we can
	#	issue commands to create an iRODS account and
	#	directories.
	printStatus( "Starting iRODS server with boot environment...\n" );
	printLog( "\nStarting iRODS server with boot environment...\n" );
	printLog( "    Setting irodsEnvFile to $bootEnv\n" );
	$ENV{"irodsEnvFile"} = $bootEnv;
	open( BOOTENV, "<$bootEnv" );
	my $line;
	foreach $line (<BOOTENV>)
	{
		printLog( "    ", $line );
		if ( $line =~ "irodsPort") {
			$currentPort = substr($line, 10);
			chomp($currentPort);
		}
	}
	close( BOOTENV );

	if ( startIrods( ) == 0 )
	{
		printError( "\nInstall problem:\n" );
		printError( "    Cannot start iRODS server.\n" );

		printError( "    \nIf your network environment is unusual, you may need to update the\n");
		printError( "    server/config/irodsHost file and rerun irodssetup.  See comments\n");
		printError( "    within irodsHost.\n");

		printLog( "\nCannot start iRODS server.\n" );

		printLog( "    \nIf your network environment is unusual, you may need to update the\n");
		printLog( "    server/config/irodsHost file and rerun irodssetup.  See comments\n");
		printLog( "    within irodsHost.\n");
		cleanAndExit( 1 );
	}


	# Connect to iRODS.
	#	Instead of the user's account (if they have one),
	#	use the default boot account.  This is configured
	#	in the boot environment script.
	#
	#	The boot account has a default password.  However,
	#	a last step of this function changes that boot
	#	account's password to the user's password.  If
	#	this script is run again, we have to try both
	#	passwords.
	printStatus( "Opening iRODS connection with boot password...\n" );
	printLog( "\nOpening iRODS connection using boot password...\n" );
	my $passwordsAlreadySet = 0;
	($status,$output) = run( "$iinit $IRODS_ADMIN_BOOT_PASSWORD" );
	if ( $status != 0 )
	{
		# Failed with the boot password.  Try the user's
		# password.
		printStatus( "Opening iRODS connection with user's password...\n" );
		printLog( "    Boot password failed.  Try user's password.\n" );
		printLog( "    ", $output );
		printLog( "\nOpening iRODS connection using user's password...\n" );
		($status,$output) = run( "$iinit $IRODS_ADMIN_PASSWORD" );
		if ( $status != 0 )
		{
			# Neither worked.
			if ( -e $authFile )
			{
				unlink( $authFile );
			}
			printError( "\nInstall problem:\n" );
			printError( "    Connection to the iRODS server failed.\n" );
			printError( "        ", $output );
			printLog( "\nCannot initialize connection to iRODS server using boot account.\n" );
			printLog( "    ", $output );
			cleanAndExit( 1 );
		}

		# Since we connected with the user's password
		# for the boot account, this script must have
		# been run once before and made it to the part
		# below that sets the boot acount to the user's
		# password.  Flag that we don't need to do
		# that again.
		$passwordsAlreadySet = 1;
	}

	my $somethingFailed = 0;


	# Create directories.
	#	If the directory already exists, continue without
	#	an error.  One of the other directories might not
	#	exist yet.
	#
	#	If the directory doesn't exist, create it.
	#
	#	Otherwise, something is wrong.  An error message is
	#	already output.  Flag the error so that we can
	#	clean up and quit later.
	printStatus( "Creating iRODS directories...\n" );
	printLog( "\nCreating iRODS directories...\n" );
	my $createdDirectory = 0;

	printLog( "    Create /$ZONE_NAME...\n" );
	$status = imkdir( "/$ZONE_NAME" );
	if ( $status == 0 ) { $somethingFailed = 1; }
	elsif ( $status == 1 ) { $createdDirectory = 1; }

	if ( !$somethingFailed )
	{
		printLog( "    Create /$ZONE_NAME/home...\n" );
		$status = imkdir( "/$ZONE_NAME/home" );
		if ( $status == 0 ) { $somethingFailed = 1; }
		elsif ( $status == 1 ) { $createdDirectory = 1; }
	}

	if ( !$somethingFailed )
	{
		printLog( "    Create /$ZONE_NAME/trash...\n" );
		$status = imkdir( "/$ZONE_NAME/trash" );
		if ( $status == 0 ) { $somethingFailed = 1; }
		elsif ( $status == 1 ) { $createdDirectory = 1; }
	}

	if ( !$somethingFailed )
	{
		printLog( "    Create /$ZONE_NAME/trash/home...\n" );
		$status = imkdir( "/$ZONE_NAME/trash/home" );
		if ( $status == 0 ) { $somethingFailed = 1; }
		elsif ( $status == 1 ) { $createdDirectory = 1; }
	}

	if ( !$somethingFailed && !$createdDirectory )
	{
		printStatus( "    Skipped.  Directories already created.\n" );
	}


	# At this point, if $somethingFailed == 1, then one or
	# more directories could not be created.  There's no
	# point in creating the user account since there is
	# no home directory for it.


	# Create the public group, if no earlier error occurred.
	#	If the group already exists, continue without
	#	an error.
	#
	#	If the group doesn't exist, create it.
	#
	#	Otherwise, something is wrong.  An error message is
	#	already output.  Flag the error so that we can
	#	clean up and quit later.
	if ( !$somethingFailed )
	{
		printStatus( "Creating iRODS group 'public'...\n" );
		printLog( "\nCreating iRODS group 'public'...\n" );

		my $createdGroup = 0;

		$status = imkgroup( "public" );
		if ( $status == 0 ) { $somethingFailed = 1; }
		elsif ( $status == 1 ) { $createdGroup = 1; }

		if ( !$somethingFailed && !$createdGroup )
		{
			printStatus( "    Skipped.  Group already created.\n" );
		}
	}

	# Create the admin account, if no earlier error occurred.
	#	If the account already exists, continue without
	#	an error.
	#
	#	If the account doesn't exist, create it.
	#
	#	Otherwise, something is wrong.  An error message is
	#	already output.  Flag the error so that we can
	#	clean up and quit later.
	if ( !$somethingFailed )
	{
		printStatus( "Creating iRODS user account...\n" );
		printLog( "\nCreating iRODS user account...\n" );

		my $createdUser = 0;

		$status = imkuser( $IRODS_ADMIN_NAME );
		if ( $status == 0 ) { $somethingFailed = 1; }
		elsif ( $status == 1 ) { $createdUser = 1; }

		if ( !$somethingFailed && !$createdUser )
		{
			printStatus( "    Skipped.  Account already created.\n" );
		}
	}

	# At this point, if $somethingFailed == 1, then one or
	# more directories could not be created, or the user
	# account could not be created.  There's no point in
	# changing directory ownership since there's no account
	# or there are no directories.


	# Give ownership permission to the admin for the main directories,
	# if no earlier error occurred.
	#	If the directory is already set properly, continue
	#	without an error
	#
	#	If not set right, set it.
	#
	#	Otherwise, something is wrong.  An error message is
	#	already output.  Flag the error so that we can
	#	clean up and quit later.
	if ( !$somethingFailed )
	{
		printStatus( "Setting iRODS directory ownership...\n" );
		printLog( "\nSetting iRODS directory ownership...\n" );
		my $chownedDirectory = 0;

		printLog( "    chown /\n" );
		$status = ichown( $IRODS_ADMIN_NAME, "/" );
		if ( $status == 0 ) { $somethingFailed = 1; }
		elsif ( $status == 1 ) { $chownedDirectory = 1; }

		printLog( "    chown /$ZONE_NAME...\n" );
		$status = ichown( $IRODS_ADMIN_NAME, "/$ZONE_NAME" );
		if ( $status == 0 ) { $somethingFailed = 1; }
		elsif ( $status == 1 ) { $chownedDirectory = 1; }

		if ( !$somethingFailed )
		{
			printLog( "    chown /$ZONE_NAME/home...\n" );
			$status = ichown( $IRODS_ADMIN_NAME, "/$ZONE_NAME/home" );
			if ( $status == 0 ) { $somethingFailed = 1; }
			elsif ( $status == 1 ) { $chownedDirectory = 1; }
		}

		if ( !$somethingFailed )
		{
			printLog( "    chown /$ZONE_NAME/trash...\n" );
			$status = ichown( $IRODS_ADMIN_NAME, "/$ZONE_NAME/trash" );
			if ( $status == 0 ) { $somethingFailed = 1; }
			elsif ( $status == 1 ) { $chownedDirectory = 1; }
		}

		if ( !$somethingFailed )
		{
			printLog( "    chown /$ZONE_NAME/trash/home...\n" );
			$status = ichown( $IRODS_ADMIN_NAME, "/$ZONE_NAME/trash/home" );
			if ( $status == 0 ) { $somethingFailed = 1; }
			elsif ( $status == 1 ) { $chownedDirectory = 1; }
		}

		if ( !$somethingFailed && !$chownedDirectory )
		{
			printStatus( "    Skipped.  Directories already uptodate.\n" );
		}
	}

	# At this point, if $somethingFailed == 1, then one or
	# more directories could not be created, the user account
	# could not be created, or directory ownership changes
	# failed.  Without an account, there's certainly no point
	# in setting a password next.


	# Set the password for the admin and boot accounts.
	#	Note that after we do this, if this script is run again
	#	it won't be able to use the default boot password to
	#	connect again.  It'll have to use the admin password.
	if ( !$somethingFailed && !$passwordsAlreadySet )
	{
		printStatus( "Setting iRODS user password...\n" );
		printLog( "\nSetting iRODS user password...\n" );
		my $command = "$iadmin moduser $IRODS_ADMIN_NAME password $IRODS_ADMIN_PASSWORD";
		if ( runIcommand( $command ) == 0 )
		{
			$somethingFailed = 1;
		}
		else
		{
			printStatus( "Setting iRODS boot password...\n" );
			printLog( "\nSetting iRODS boot password...\n" );
			$command = "$iadmin moduser $IRODS_ADMIN_BOOT_NAME password $IRODS_ADMIN_PASSWORD";
			if ( runIcommand( $command ) == 0 )
			{
				$somethingFailed = 1;
			}
		}
	}


	# Clean up.  Stop using the boot environment and delete
	# the authorization file.
	runIcommand( "$iexit full" );
	$ENV{"irodsEnvFile"} = undef;
	if ( -e $authFile )
	{
		unlink( $authFile );
	}


	# Stop iRODS.
	printStatus( "Stopping iRODS server...\n" );
	printLog( "\nStopping iRODS server...\n" );
	stopIrods( );

	# Switch to the new (non-boot) port (for checking processes)
	$currentPort = $IRODS_PORT;


	if ( $somethingFailed )
	{
		cleanAndExit( 1 );
	}
}





#
# @brief	Configure iRODS user.
#
# Update the user's iRODS environment file and set their default
# resource.  Test that it works.
#
# This is made more complicated by a lot of error checking to
# see if each of these tasks has already been done.  If a task
# has been done, we skip it and try the next one.  This insures
# that we'll pick up wherever we left off from a prior failure.
#
# Output error messages and exit on problems.
#
sub configureIrodsUser
{
	++$currentStep;
	printSubtitle( "\n" );
	printSubtitle( "Step $currentStep of $totalSteps:  Configuring iRODS user and starting server...\n" );
	printLog( "\n" );
	printLog( "Configuring iRODS user and starting server...\n" );
	printLog( "------------------------------------------------------------------------\n" );
	printLog( getCurrentDateTime( ) . "\n\n" );


	if ($isUpgrade)  {
	    # For an upgrade, just start the server and connect
	    # Start iRODS.
	    printStatus( "Starting iRODS server...\n" );
	    printLog( "\nStarting iRODS server...\n" );
	    if ( startIrods( ) == 0 )
	    {
		printError( "\nInstall problem:\n" );
		printError( "    Cannot start iRODS server.\n" );
		printLog( "\nCannot start iRODS server.\n" );
		cleanAndExit( 1 );
	    }

	    printStatus( "Opening iRODS connection...\n" );
	    printLog( "\nOpening iRODS connection...\n" );
	    my ($status,$output) = run( "$iinit $IRODS_ADMIN_PASSWORD" );
	    if ( $status != 0 )
	    {
		printError( "\nInstall problem:\n" );
		printError( "    Connection to iRODS server failed.\n" );
		printError( "        ", $output );
		printLog( "\nCannot open connection to iRODS server.\n" );
		printLog( "    ", $output );
		cleanAndExit( 1 );
	    }
	    return();
	}


	# Create a .rodsEnv file for the user, if needed.
	#	We could check to see if the existing file (if any)
	#	already matches the current configuration, but that's
	#	more hassle than it's worth.  Just set it.  If we
	#	set it to the same values, it doesn't hurt anything.
	printStatus( "Updating iRODS user's ~/.irods/.irodsEnv...\n" );
	printLog( "Updating iRODS user's ~/.irods/.irodsEnv...\n" );
	if ( ! -e $userIrodsDir )
	{
		# User's iRODS configuration directory doesn't exist yet.
		printLog( "    mkdir $userIrodsDir\n" );
		mkdir( $userIrodsDir, 0700 );
	}
	if ( -e $userIrodsFile )
	{
		# Move the user's existing configuration file.
		printLog( "    move $userIrodsFile $userIrodsFile.orig\n" );
		move( $userIrodsFile, $userIrodsFile . ".orig" );
		chmod( 0600, $userIrodsFile . ".orig" );
	}

	printToFile( $userIrodsFile,
		"# iRODS personal configuration file.\n" .
		"#\n" .
		"# This file was automatically created during iRODS installation.\n" .
		"#   Created " . getCurrentDateTime( ) . "\n" .
		"#\n" .
		"# iRODS server host name:\n" .
		"irodsHost '$thisHost'\n" .
		"# iRODS server port number:\n" .
		"irodsPort $IRODS_PORT\n" .
		"\n" .
		"# Default storage resource name:\n" .
		"irodsDefResource '$RESOURCE_NAME'\n" .
		"# Home directory in iRODS:\n" .
		"irodsHome '/$ZONE_NAME/home/$IRODS_ADMIN_NAME'\n" .
		"# Current directory in iRODS:\n" .
		"irodsCwd '/$ZONE_NAME/home/$IRODS_ADMIN_NAME'\n" .
		"# Account name:\n" .
		"irodsUserName '$IRODS_ADMIN_NAME'\n" .
		"# Zone:\n" .
		"irodsZone '$ZONE_NAME'\n" );
	chmod( 0600, $userIrodsFile );



	# Start iRODS.
	printStatus( "Starting iRODS server...\n" );
	printLog( "\nStarting iRODS server...\n" );
	if ( startIrods( ) == 0 )
	{
		printError( "\nInstall problem:\n" );
		printError( "    Cannot start iRODS server.\n" );
		printLog( "\nCannot start iRODS server.\n" );
		cleanAndExit( 1 );
	}



	# Connect.
	printStatus( "Opening iRODS connection...\n" );
	printLog( "\nOpening iRODS connection...\n" );
	my ($status,$output) = run( "$iinit $IRODS_ADMIN_PASSWORD" );
	if ( $status != 0 )
	{
		printError( "\nInstall problem:\n" );
		printError( "    Connection to iRODS server failed.\n" );
		printError( "        ", $output );
		printLog( "\nCannot open connection to iRODS server.\n" );
		printLog( "    ", $output );
		cleanAndExit( 1 );
	}


	# Create the default resource, if needed.
	printStatus( "Creating default resource...\n" );
	printLog( "\nCreating default resource...\n" );

	# List existing resources first to see if it already exists.
	($status,$output) = run( "$iadmin lr" );
	if ( $status == 0 && index($output,$RESOURCE_NAME) >= 0 )
	{
		printStatus( "    Skipped.  Resource already created.\n" );
		printLog( "    Skipped.  Resource already created.\n" );
	}
	else
	{
		# Resource doesn't appear to exist.  Create it.
		($status,$output) = run( "$iadmin mkresc $RESOURCE_NAME 'unix file system' archive $thisHost $RESOURCE_DIR $ZONE_NAME" );
		if ( $status != 0 )
		{
			printError( "\nInstall problem:\n" );
			printError( "    Cannot create default resource:\n" );
			printError( "        ", $output );
			printLog( "\nCannot create default resource:\n" );
			printLog( "    ", $output );
			cleanAndExit( 1 );
		}
	}


	# Test it to be sure it is working by putting a
	# dummy file and getting it.
	printStatus( "Testing resource...\n" );
	printLog( "\nTesting resource...\n" );
	my $tmpPutFile = "irods_put_$$.tmp";
	my $tmpGetFile = "irods_get.$$.tmp";
	printToFile( $tmpPutFile, "This is a test file." );

	($status,$output) = run( "$iput $tmpPutFile" );
	if ( $status != 0 )
	{
		printError( "\nInstall problem:\n" );
		printError( "    Cannot put test file into iRODS:\n" );
		printError( "        ", $output );
		printLog( "\nCannot put test file into iRODS:\n" );
		printLog( "    ", $output );
		cleanAndExit( 1 );
	}

	($status,$output) = run( "$iget $tmpPutFile $tmpGetFile" );
	if ( $status != 0 )
	{
		printError( "\nInstall problem:\n" );
		printError( "    Cannot get test file from iRODS:\n" );
		printError( "        ", $output );
		printLog( "\nCannot get test file from iRODS:\n" );
		printLog( "    ", $output );
		cleanAndExit( 1 );
	}

	($status,$output) = run( "diff $tmpPutFile $tmpGetFile" );
	if ( $status != 0 )
	{
		printError( "\nInstall problem:\n" );
		printError( "    Test file retrieved from iRODS doesn't match file written there:\n" );
		printError( "        ", $output );
		printLog( "\nGet file doesn't match put file:\n" );
		printLog( "    ", $output );
		cleanAndExit( 1 );
	}

	($status,$output) = run( "$irm -f $tmpPutFile" );
	if ( $status != 0 )
	{
		printError( "\nInstall problem:\n" );
		printError( "    Cannot remove test file from iRODS:\n" );
		printError( "        ", $output );
		printLog( "\nCannot remove test file from iRODS:\n" );
		printLog( "    ", $output );
		cleanAndExit( 1 );
	}

	unlink( $tmpPutFile );
	unlink( $tmpGetFile );

# No longer do the 'iexit full' ICAT-enable Servers either since the
# auth file is needed for server to server connections.
#	if ($DATABASE_TYPE ne "") {
#		($status,$output) = run( "$iexit full" );
#		if ( $status != 0 )
#		{
#			printError( "\nInstall problem:\n" );
#			printError( "    Cannot close iRODS connection:\n" );
#			printError( "        ", $output );
#			printLog( "\nCannot close iRODS connection:\n" );
#			printLog( "    ", $output );
#			cleanAndExit( 1 );
#		}
#	}


	# Stop iRODS.
# Leave the iRODS server running.
#	printStatus( "Stopping iRODS...\n" );
#	printLog( "Stopping iRODS...\n" );
#	if ( stopIrods( ) == 0 )
#	{
#		cleanAndExit( 1 );
#	}
}






########################################################################
#
# Support functions.
#
#	All of these functions support activities in the setup steps.
#	Some output error messages directly, but none exit the script
#	on their own.
#

#
# @brief	Create the database, if it doesn't exist already.
#
# This function creates the iRODS/iCAT database using the selected
# database type.  If the script is in FORCE mode, and there is a
# database already, that database is dropped first.
#
# Messages are output on errors.
#
# @return
# 	A numeric code indicating success or failure:
# 		0 = failure
# 		1 = created
# 		2 = already created
#
sub createDatabase()
{
	# Postgres
	if ( $DATABASE_TYPE eq "postgres" )
	{
		return Postgres_CreateDatabase( );
	}

	# Oracle
	if ( $DATABASE_TYPE eq "oracle" )
	{
		return Oracle_CreateDatabase( );
	}

	# MySQL
	if ( $DATABASE_TYPE eq "mysql" )
	{
		return MySQL_CreateDatabase( );
	}

	# Empty
	if ( $DATABASE_TYPE eq "" )
	{
		printStatus( "    Skipped.  No database to configure.\n" );
		printLog( "    Skipped.  No database to configure.\n" );
		return 0;
	}

	# Otherwise skip it.
	printStatus( "    Skipped.  Unsupported database type:  $DATABASE_TYPE.\n" );
	printLog( "    Skipped.  Unsupported database type:  $DATABASE_TYPE.\n" );
	return 0;
}





#
# @brief	Show a list of tables in the database.
#
# This function issues an appropriate command to the database to
# get a list of existing tables.
#
# Messages are output on errors.
#
# @param	$dbname
# 	The database name.
# @return
# 	A list of tables, or undef on error.
#
sub listDatabaseTables()
{
	my ($dbname) = @_;

	# Postgres
	if ( $DATABASE_TYPE eq "postgres" )
	{
		return Postgres_ListDatabaseTables( $dbname );
	}

	# Oracle
	if ( $DATABASE_TYPE eq "oracle" )
	{
		return Oracle_ListDatabaseTables( $dbname );
	}

	# MySQL
	if ( $DATABASE_TYPE eq "mysql" )
	{
		return MySQL_ListDatabaseTables( $dbname );
	}

	# Empty
	if ( $DATABASE_TYPE eq "" )
	{
		printStatus( "    Skipped.  No database to access.\n" );
		printLog( "    Skipped.  No database to access.\n" );
		return undef;
	}

	# Otherwise skip it.
	printStatus( "    Skipped.  Unsupported database type:  $DATABASE_TYPE.\n" );
	printLog( "    Skipped.  Unsupported database type:  $DATABASE_TYPE.\n" );
	return undef;
}





#
# @brief	Get the process IDs of the iRODS servers.
#
# @return	%serverPids	associative array of arrays of
# 				process ids.  Keys are server
# 				types from %servers.
#
sub getIrodsProcessIds()
{
	my @serverPids = ();

	$parentPid=getIrodsServerPid();
	my @serverPids = getFamilyProcessIds( $parentPid );

	return @serverPids;
}





#
# @brief	Start iRODS server.
#
# This function starts the iRODS server and confirms that it
# started.
#
# @return
# 	A numeric code indicating success or failure:
# 		0 = failure
# 		1 = started
# 		2 = already started
#
sub startIrods()
{
	# See if server is already started
	my @serverPids = getIrodsProcessIds( );

	if ( $#serverPids > 0 )
	{
		return 2;
	}

	($status,$output) = run( "$perl $irodsctl istart" );

	if ( $status != 0 )
	{
		printError( "Could not start iRODS server.\n" );
		printError( "    $output\n" );
		printLog( "\nCould not start iRODS server.\n" );
		printLog( "    $output\n" );
		return 0;
	}
	return 1;
}


#
# @brief	Get the iRODS Server PID (Parent of the others)
#
# This function iRODS Server PID, which is the parent of the others.
#
# @return
# 	The PID or "NotFound"
#
sub getIrodsServerPid()
{
	my $tmpDir="/usr/tmp";
	if (!-e $tmpDir)  {
	    $tmpDir="/tmp";
	}
	my $processFile   = $tmpDir . "/irodsServer" . "." . $currentPort;

	open( PIDFILE, "<$processFile" );
	my $line;
	my $parentPid="NotFound";
	foreach $line (<PIDFILE>)
	{
		my $i = index($line, " ");
		$parentPid=substr($line,0,$i);
	}
	close( PIDFILE );
	return ( $parentPid);
}




#
# @brief	Stop iRODS server.
#
# This function stops the iRODS server and confirms that it
# stopped.
#
# @return
# 	A numeric code indicating success or failure:
# 		0 = failure
# 		1 = stopped
# 		2 = already stopped
#
sub stopIrods()
{
	# Find and kill the server process IDs
	my $found = 0;

	$parentPid=getIrodsServerPid();
	my @pids = getFamilyProcessIds( $parentPid );

	foreach $pid (@pids)
	{
	    	$found = 1;
		kill( 'SIGINT', $pid );
	}
	return 2 if ( ! $found );	# Nothing running

	# Repeat to catch stragglers.  This time use kill -9.

	my @pids = getFamilyProcessIds( $parentPid );
	if ( $#pids >= 0 )
	{
		# make some short delay, let them die
		sleep( 1 ) ;
	}
	my @pids = getFamilyProcessIds( $parentPid );
	foreach $pid (@pids)
	{
	    	kill( 9, $pid );
	}

	# Report if there are any left.
	my $didNotDie = 0;
	@pids = getFamilyProcessIds( $parentPid );
	if ( $#pids >= 0 )
	{
	    	printError( "Could not stop all iRODS servers\n" );
		printLog( "Could not stop all iRODS servers\n" );
		foreach $pid (@pids)
		{
			printError( "    Process $pid\n" );
			printLog( "    Process $pid\n" );
		}		
		$didNotDie = 1;
	}
	return 0 if ( $didNotDie );	# Failed to kill all
	return 1;
}





#
# @brief	Start database server
#
# This function starts the database server and confirms that
# it started.  However, if the database is not under our control,
# this function does nothing.
#
# @return
# 	A numeric code indicating success or failure:
# 		0 = failure
# 		1 = started
# 		2 = already started
# 		3 = not under our control
#
sub startDatabase()
{
	return 3 if ( !$controlDatabase );	# DB not under our control

	if ( $DATABASE_TYPE eq "postgres" )
	{
		return Postgres_startDatabase( );
	}

	if ( $DATABASE_TYPE eq "oracle" )
	{
		return 3;
	}

	if ( $DATABASE_TYPE eq "mysql" )
	{
		return 3;
	}

	if ( $DATABASE_TYPE eq "" )
	{
		return 0;
	}

	# Don't know how to start.
	return 0;
}





#
# @brief	Stop database server
#
# This function stops the database server and confirms that
# it stopped.  However, if the database is not under our control,
# this function does nothing.
#
# @return
# 	A numeric code indicating success or failure:
# 		0 = failure
# 		1 = stopped
# 		2 = already stopped
# 		3 = not under our control
#
sub stopDatabase()
{
	return 3 if ( !$controlDatabase );	# DB not under our control

	if ( $DATABASE_TYPE eq "postgres" )
	{
		return Postgres_stopDatabase( );
	}

	if ( $DATABASE_TYPE eq "oracle" )
	{
		return 3;
	}

	if ( $DATABASE_TYPE eq "mysql" )
	{
		return 3;
	}

	if ( $DATABASE_TYPE eq "" )
	{
		return 0;
	}

	# Don't know how to stop.
	return 0;
}





#
# @brief	Run an iCommand and report an error, if any.
#
# This function runs the given command and collects its output.
# On an error, a message is output.
#
# @param	$command
# 	the iCommand to run
# @return
# 	A numeric code indicating success or failure:
# 		0 = failure
# 		1 = success
#
sub runIcommand($)
{
	my ($icommand) = @_;

	my ($status,$output) = run( $icommand );
	return 1 if ( $status == 0 );	# Success


	printError( "\nInstall problem:\n" );
	printError( "    iRODS command failed:\n" );
	printError( "        Command:  $icommand\n" );
	printError( "        ", $output );
	printLog( "icommand failed:\n" );
	printLog( "    ", $output );
	return 0;
}





#
# @brief	Make an iRODS directory, if it doesn't already exist.
#
# This function runs the iRODS command needed to create a directory.
# It checks that the directory doesn't exist first.
#
# @param	$directory
# 	the directory to create
# @return
# 	A numeric code indicating success or failure:
# 		0 = failure
# 		1 = success
# 		2 = already exists
#
sub imkdir($)
{
	my ($directory) = @_;

	# Check if the directory already exists.
	my ($status,$output) = run( "$ils $directory" );
	if ( $status == 0 )
	{
		# The 'ls' completed fine, which means the
		# directory already exists.
		printLog( "        Skipped.  Directory $directory already created.\n" );
		return 2;
	}

	# The 'ls' reported an error.  Make sure it is a
	# 'do not exist' error.  Otherwise it is something
	# more serious.
	if ( $output !~ /does not exist/i )
	{
		# Something more serious.
		printError( "\nInstall problem:\n" );
		printError( "    iRODS command failed:\n" );
		printError( "        Command:  $ils $directory\n" );
		printError( "        ", $output );
		printLog( "\nils failed:\n" );
		printLog( "    $output\n" );
		return 0;
	}

	# The directory doesn't exist.  Create it.
	return runIcommand( "$iadmin mkdir $directory" );
}





#
# @brief	Make an iRODS admin account, if it doesn't already exist.
#
# This function runs the iRODS command needed to create a user account.
# It checks that the account doesn't exist first.
#
# @param	$username
# 	the name of the user account
# @return
# 	A numeric code indicating success or failure:
# 		0 = failure
# 		1 = success
# 		2 = already exists
#
sub imkuser($)
{
	my ($username) = @_;

	# Check if the account already exists.
	# 	'iadmin lu' returns a list of accounts, one per line.
	#	Ignore leading and trailing white-space on the account name.
	my ($status,$output) = run( "$iadmin lu" );
	my $line;
	my @lines = split( "\n", $output );
	$usernameWithZone=$username . "#" . $ZONE_NAME;
	foreach $line (@lines)
	{
		if ( $line =~ /^[ \t]*$usernameWithZone[ \t]*$/ )
		{
			# The account already exists.
			printLog( "        Skipped.  Account '$username' already exists.\n" );
			return 2;
		}
	}

	# Create it.
	return runIcommand( "$iadmin mkuser $username rodsadmin" );
}


#
# @brief	Make an iRODS group, if it doesn't already exist.
#
# This function runs the iRODS command needed to create a user group.
# It checks that the group doesn't exist first.
#
# @param	$groupname
# 	the name of the user group
# @return
# 	A numeric code indicating success or failure:
# 		0 = failure
# 		1 = success
# 		2 = already exists
#
sub imkgroup($)
{
	my ($groupname) = @_;

	# Check if the account already exists.
	# 	'iadmin lu' returns a list of accounts, one per line.
	#	Ignore leading and trailing white-space on the group name.
	my ($status,$output) = run( "$iadmin lg" );
	my $line;
	my @lines = split( "\n", $output );
	foreach $line (@lines)
	{
		if ( $line =~ /^[ \t]*$groupname[ \t]*$/ )
		{
			# The group already exists.
			printLog( "        Skipped.  Group '$groupname' already exists.\n" );
			return 2;
		}
	}

	# Create it.
	return runIcommand( "$iadmin mkgroup $groupname" );
}





#
# @brief	Give ownership permissions on a directory, if not already set.
#
# This function runs the iRODS command needed to set a directory's owner.
# It checks that the ownership wasn't already set.
#
# @param	$username
# 	the name of the user account
# @param	$directory
# 	the directory to change
# @return
# 	A numeric code indicating success or failure:
# 		0 = failure
# 		1 = success
# 		2 = already set
# 
sub ichown($$)
{
	my ($username,$directory) = @_;

	# The original author wanted to check the ownership of the
	# directory first, but there was not iCommand to do that at
	# the time (now, ils -A does it).  But it does no harm to just
	# set it, even if it is redundant.
	my ($status,$output) = run( "$ichmod own $username $directory" );
	return 1 if ( $status == 0 );

	# An error.
	printError( "\nInstall problem:\n" );
	printError( "    iRODS command failed:\n" );
	printError( "        Command:  $ichmod own $username $directory\n" );
	printError( "        ", $output );
	printLog( "\nichmod failed:\n" );
	printLog( "    ", $output );
	return 0;
}





#
# @brief	Generate the MD5 scrambled form of a password.
#
# iRODS connections use MD5 scrambling.  This function uses
# the iRODS 'iadmin' command to generate the MD5 form of a
# password based upon a given scrambling key.
#
# @param	$password
# 	the password to scramble
# @return
# 	the scrambled password, or undef on failure
#
sub scramblePassword($)
{
	my ($password) = @_;

	# Generate the scrambled form of the database password.
	my ($status,$output) = run( "$iadmin spass '$password' $DB_KEY" );
	return undef if ( $status != 0 );	# Error

	$output =~ /Scrambled form is:(.*)$/;
	my $scrambled = $1;
	chomp( $scrambled );
	return $scrambled;
}





#
# @brief	Send a file of SQL commands to the database.
#
# The given SQL commands are passed to the database.  The
# command to do this varies with the type of database in use.
#
# @param	$databaseName
# 	the name of the database
# @param	$sqlFilename
# 	the name of the file
# @return
# 	A 2-tuple including a numeric code indicating success
# 	or failure, and the stdout/stderr of the command.
# 		0 = failure
# 		1 = success
#
sub execute_sql($$)
{
	my ($databaseName,$sqlFilename) = @_;

	# Postgres
	if ( $DATABASE_TYPE eq "postgres" )
	{
		return Postgres_sql( $databaseName, $sqlFilename );
	}

	# Oracle
	if ( $DATABASE_TYPE eq "oracle" )
	{
		return Oracle_sql( $databaseName, $sqlFilename );
	}

	# MySQL
	if ( $DATABASE_TYPE eq "mysql" )
	{
		return MySQL_sql( $databaseName, $sqlFilename );
	}

	# Empty
	if ( $DATABASE_TYPE eq "" )
	{
		return (0,"No database" );
	}

	# Otherwise cannot issue SQL.
	return (0,"Unknown database:  $DATABASE_TYPE");
}





#
# @brief	Exit the script abruptly.
#
# Print a final message, then close the log and exit.
#
# @param	$code	the exit code
#
sub cleanAndExit($)
{
	my ($code) = @_;

	# Try to close open iRODS connections.  Ignore failure.
	my $output = `$iexit full 2>&1`;

	# Try to shut down iRODS server.  Ignore failure.
	stopIrods( );

	printError( "\nAbort.\n" );

	printLog( "\nAbort.\n" );
	closeLog( );

	exit( $code );
}





#
# @brief	Print command-line help
#
# Print a usage message.
#
sub printUsage()
{
	my $oldVerbosity = isPrintVerbose( );
	setPrintVerbose( 1 );

	printNotice( "This script sets up the iRODS database.\n" );
	printNotice( "\n" );
	printNotice( "Usage is:\n" );
	printNotice( "    $scriptName [options]\n" );
	printNotice( "\n" );
	printNotice( "Help options:\n" );
	printNotice( "    --help      Show this help information\n" );
	printNotice( "\n" );
	printNotice( "Verbosity options:\n" );
	printNotice( "    --quiet     Suppress all messages\n" );
	printNotice( "    --verbose   Output all messages (default)\n" );
	printNotice( "    --noask     Don't ask questions, assume 'yes'\n" );
	printNotice( "\n" );
	printNotice( "Other options:\n" );
	printNotice( "    --force     Force database drop and reset\n" );
	printNotice( "\n" );

	setPrintVerbose( $oldVerbosity );
}















########################################################################
#
#  Postgres functions.
#
#  	These functions are all Postgres-specific.  They are called
#  	by the setup functions if the database under our control is
#  	Postgres.  All of them output error messages on failure,
#  	but none exit directly.
#


#
# @brief	Create an Alternate Database User (if requested).
#
# This function creates the additional postgres user if the specified
# name it is not the same as the OS usename.
#
sub Postgres_CreateAlternateDatabaseUser( )
{
	if ( $thisUser eq $DATABASE_ADMIN_NAME ) {
		# Typical case, using the OS username for the DB user.
		return;
	}
    
	# Create the postgres user.
	#   If this is an initial build/install of Postgres, no
	#   password will be needed (yet) but to avoid the possibility
	#   of getting stuck on a prompt, we include the password in 
	#   an input file.
	printStatus( "    Creating Postgres database user...\n" );
	printLog( "\n    Creating Postgres database user...\n" );
	my $tmpPassword = createTempFilePath( "create" );
	printToFile( $tmpPassword, "$DATABASE_ADMIN_PASSWORD\n" );
	chmod( 0600, $tmpPassword );
	($status,$output) = run( "$createuser -U $thisUser -s $DATABASE_ADMIN_NAME < $tmpPassword" );
	if ( $status != 0 ) {
		printStatus( "        Create user failed (which may be OK)...\n" );
		printLog( "\n        Create user failed (which may be OK)...\n" );
		printLog( "        $createuser \n" );
		printLog( "        $output" );
		printLog( "        status: $status\n");
	}
	unlink( $tmpPassword );
}

#
# @brief	Configure user accounts for a Postgres database.
#
# This function creates the user's .pgpass file so that Postgres commands
# can automatically use the right account name and password.
#
# These actions will not require a database restart.
#
# Messages are output on errors.
#
sub Postgres_ConfigureDatabaseUser()
{
	# Look for the user's .pgpass file in their home directory.
	# It may not exist yet.
	my $pgpass = File::Spec->catfile( $ENV{"HOME"}, ".pgpass" );

	# The new line for .pgpass:
	#              hostname:port:database:username:password
	my $newline = "*:$DATABASE_PORT:$DB_NAME:$DATABASE_ADMIN_NAME:$DATABASE_ADMIN_PASSWORD\n";


	# If the .pgpass file doesn't exist, make one with a line for the
	# iRODS database.  Then we're done.
	if ( ! -e $pgpass )
	{
		printStatus( "Creating user's .pgpass...\n" );
		printLog( "\nCreating user's .pgpass...\n" );
		printToFile( $pgpass, $newline );
		chmod( 0600, $pgpass );
		return;
	}


	# The user already has a .pgpass file.  It may have entries
	# for multiple databases, including those that are not for
	# iRODS.  We can't just delete it or overwrite it.  It also
	# may have an old entry for a prior iRODS install.
	#
	# So, if there is an old iRODS entry, delete it.
	# Otherwise, append an entry for this install.
	printStatus( "Updating user's .pgpass...\n" );
	printLog( "Updating user's .pgpass...\n" );

	open( PGPASSNEW, ">$pgpass.new" );
	chmod( 0600, $pgpass );
	open( PGPASS, "<$pgpass" );

	# Read each line and see if we needed to change it.
	foreach $line (<PGPASS>)
	{
		# Each line in the file has the form:
		# 	host:port:database:user:password

		if ( $line eq $newline )
		{
			# It's an old entry that exactly
			# matches what we're about to add.
			# No point in continuing with this.
			close( PGPASS );
			close( PGPASSNEW );
			printStatus( "    Skipped.  File already uptodate.\n" );
			printLog( "    Skipped.  File already uptodate.\n" );
			unlink( "$pgpass.new" );
			return;
		}

		my @fields = split( ":", $line );
		if ( $fields[2] ne $DB_NAME )
		{
			# Not an old entry for this same
			# database name.  Copy it as-is.
			print( PGPASSNEW $line );
		}
		else
		{
			# Otherwise it is an old entry for
			# the same database.  Don't
			# copy it to the new file.
			printLog( "    Deleting old $DB_NAME line:  $line\n" );
		}
	}
	close( PGPASS );
	print( PGPASSNEW $newline );
	close( PGPASSNEW );

	# Replace the old one with the new one.
	move( "$pgpass.new", $pgpass );
	chmod( 0600, $pgpass );
}





#
# @brief	Create a Postgres database.
#
# This function creates a Postgres database.  If the script is
# being run in FORCE mode, and there is an old database, drop
# it first.
#
# These actions could require a database restart.
#
# Messages are output on errors.
#
# @return
# 	A numeric code indicating success or failure:
# 		0 = failure
# 		1 = success
# 		2 = already created
#
sub Postgres_CreateDatabase()
{
	# Check if the database already exists.
	#	'psql' will list databases, or report an error
	#	if the one we want isn't there.
	printStatus( "Checking whether iCAT database exists...\n" );
	printLog( "\nChecking whether iCAT database exist...\n" );
	my $needCreate = 1;
	my ($status,$output) = run( "$psql -l $DB_NAME" );
	if ( $output =~ /List of databases/i )
	{
		# The command only shows a list of databases if
		# the database name on the command line exists.
		# Otherwise it reports a 'does not exist' error.
		#
		# Since we did get a list of databases, the
		# database exists.  Re-create it if forced too.
		if ( $force )
		{
			# In FORCE mode, we need to drop the database and
			# re-create it.
			printStatus( "Forced dropping of iCAT database...\n" );
			printLog( "Forced dropping of iCAT database...\n" );
			my $doDrop = 1;
			if ( ! $noAsk )
			{
				printNotice( "\nDropping the iCAT database tables will destroy all metadata about files\n" );
				printNotice( "stored by iRODS.  This cannot be undone.\n" );
				printNotice( "\n" );
				if ( askYesNo( "    Continue (yes/no)?  " ) == 0 )
				{
					printStatus( "    Skipped.  Drop canceled.\n" );
					$doDrop = 0;
				}
			}
			if ( $doDrop )
			{
				printStatus( "Dropping iCAT database...\n" );

				my $tmpPassword = createTempFilePath( "drop" );
				printToFile( $tmpPassword, "$DATABASE_ADMIN_PASSWORD\n" );
				chmod( 0600, $tmpPassword );
				my ($status,$output) = run( "$dropdb $DB_NAME < $tmpPassword" );
				unlink( $tmpPassword );

				if ( $status != 0 && $output !~ /does not exist/i )
				{
					printError( "\nInstall problem:\n" );
					printError( "    Could not drop iCAT database.\n" );
					printError( "        ", $output );
					printLog( "\nCould not drop iCAT database:\n" );
					printLog( "        ", $output );
					return 0;
				}
			}
		}
		else
		{
			# Already exists and not forced to drop it,
			# so we don't need to create it again.
			$needCreate = 0;
		}
	}


	# Create the database.
	#	While the user's .pgpass provides a password for
	#	use when accessing this database, the database
	#	doesn't exist yet.  So, .pgpass isn't involved and
	#	the user will be prompted for a password.  To avoid
	#	the prompt, we include the password in an input file.
	if ( $needCreate )
	{
		printStatus( "    Creating iCAT database...\n" );
		printLog( "\n    Creating iCAT database...\n" );
		my $tmpPassword = createTempFilePath( "create" );
		printToFile( $tmpPassword, "$DATABASE_ADMIN_PASSWORD\n" );
		chmod( 0600, $tmpPassword );
		($status,$output) = run( "$createdb $DB_NAME < $tmpPassword" );
		unlink( $tmpPassword );

		if ( $status != 0 )
		{
			# Supposedly we've already caught the case where
			# the database already exists above.  Just in case,
			# watch for the same error again.
			if( $output =~ /already exists/i )
			{
				# Already exists.  Odd.  The earlier check
				# should have avoided this.
				return 2;
			}

			printError( "\nInstall problem:\n" );
			printError( "    Could not create iCAT database.\n" );
			printError( "        ", $output );
			printLog( "\nCould not create iCAT database:\n" );
			printLog( "        ", $output );
			return 0;
		}
	}
	else
	{
		printStatus( "    Skipped creating iCAT database, it already exists.\n" );
		printLog( "    Skipped creating iCAT database, it already exists.\n" );
	}



	# Update the ODBC configuration files to include the database name.
	#	If Postgres was installed by the iRODS scripts, then
	#	either 'odbcinst.ini' or 'odbc.ini' has been created
	#	when the ODBC drivers were installed.  In both cases,
	#	the 'Database' field was left out because the install
	#	script didn't know the database name at the time.
	#
	#	If Postgres was previously installed, then 'odbcinst.ini'
	#	or 'odbc.ini' may have been created and reference another
	#	database.  Or the files may be empty or not exist.
	#
	#	In order for the ODBC drivers to work, and depending upon
	#	the driver installed, these files must include a section
	#	for 'PostgreSQL' that gives the database name.
	#
	# These files should already exist, but may not include the line
	# setting the database name.  Update or create the files.
	if ( $DATABASE_ODBC_TYPE =~ /unix/i )
	{
		printStatus( "Updating Postgres UNIX ODBC odbc.ini configuration...\n" );
		printLog( "\nUpdating Postgres UNIX ODBC odbc.ini configuration\n" );

		# Update 'odbc.ini'.
		my $ini = File::Spec->catfile( $databaseEtcDir, "odbc.ini" );
		if ( ! -e $ini )
		{
			# The file doesn't exist.  Create it and presume
			# that the driver is in the Postgres/lib directory
			# where it should be.
			#
			# Since the iRODS Postgres installation always
			# creates this file, that we are creating it now
			# probably means Postgres was installed (incompletely)
			# previously.  Chances are good that this will not
			# be sufficient and something else is wrong too.
			my $libPath = abs_path( File::Spec->catfile(
				$databaseLibDir, "libodbcpsql.so" ) );
			printToFile( $ini,
				"[PostgreSQL]\n" .
				"Driver=$libPath\n" .
				"Debug=0\n" .
				"CommLog=0\n" .
				"Servername=$thisHost\n" .
				"Database=$DB_NAME\n" .
				"ReadOnly=no\n" .
				"Ksqo=0\n" .
				"Port=$DATABASE_PORT\n" );
			$databaseRestartNeeded = 1;
		}
		else
		{
			# The file exists.  Either it was created by the
			# iRODS installation script, or Postgres was
			# previosly installed.  Make sure there is a
			# 'Database' name set in the file.
			$status = Postgres_updateODBC( $ini, "PostgreSQL",
				"Database", $DB_NAME );
			if ( $status == 0 )
			{
				# Problem.  Message already output.
				return 0;
			}
			if ( $status == 1 )
			{
				# ODBC config file was changed.
				$databaseRestartNeeded = 1;
			}
			else
			{
				printStatus( "    Skipped.  Configuration already uptodate.\n" );
				printLog( "    Skipped.  Configuration already uptodate.\n" );
			}
		}
	}
	elsif ( $DATABASE_ODBC_TYPE =~ /post/i )
	{
		printStatus( "Updating Postgres ODBC odbcinst.ini configuration...\n" );
		printLog( "\nUpdating Postgres ODBC odbcinst.ini configuration\n" );

		# Update 'odbcinst.ini'.
		my $ini = File::Spec->catfile( $databaseEtcDir, "odbcinst.ini" );
		if ( ! -e $ini )
		{
			# The file doesn't exist.  Create it.  We are
			# apparently using the Postgres ODBC driver, so
			# we don't need a 'Driver' line in the file.
			#
			# Since the iRODS Postgres installation always
			# creates this file, that we are creating it now
			# probably means Postgres was installed (incompletely)
			# previously.  Chances are good that this will not
			# be sufficient and something else is wrong too.
			printToFile( $ini,
				"[PostgreSQL]\n" .
				"Debug=0\n" .
				"CommLog=0\n" .
				"Servername=$thisHost\n" .
				"Database=$DB_NAME\n" .
				"ReadOnly=no\n" .
				"Ksqo=0\n" .
				"Port=$DATABASE_PORT\n" );
			$databaseRestartNeeded = 1;
		}
		else
		{
			# The file exists.  Either it was created by the
			# iRODS installation script, or Postgres was
			# previosly installed.  Make sure there is a
			# 'Database' name set in the file.
			$status = Postgres_updateODBC( $ini, "PostgreSQL",
				"Database", $DB_NAME );
			if ( $status == 0 )
			{
				# Problem.  Message already output.
				return 0;
			}
			if ( $status == 1 )
			{
				# ODBC config file was changed.
				$databaseRestartNeeded = 1;
			}
			else
			{
				printStatus( "    Skipped.  Configuration already uptodate.\n" );
				printLog( "    Skipped.  Configuration already uptodate.\n" );
			}
		}
	}


	# The user's .odbc.ini file can override the system files above,
	# which can be a problem.  Look for a section for Postgres and
	# delete it, leaving the rest of the file alone.
	printStatus( "Updating user's .odbc.ini...\n" );
	printLog( "Updating user's .odbc.ini...\n" );
	my $userODBC = File::Spec->catfile( $ENV{"HOME"}, ".odbc.ini" );
	if ( ! -e $userODBC )
	{
		printStatus( "    Skipped.  File doesn't exist.\n" );
		printLog( "    Skipped.  File doesn't exist.\n" );
	}
	else
	{
		# Edit the file to remove a [PostgreSQL] section.
		# Do this editing in-place so that the edited file
		# has the same permissions, ownership, hard links
		# to it, etc., as it had before.

		# Backup the current file.
		my $dt = getCurrentDateTime( );
		$dt =~ s/ /_/g;
		my $tmpfile = "$userODBC.$dt";
		if ( copy( $userODBC, $tmpfile ) != 1 )
		{
			printError( "\nInstall problem:\n" );
			printError( "    Cannot copy user's ODBC configuration file.\n" );
			printError( "        Temp file:  $tmpfile\n" );
			printError( "        Error:      $!\n" );
			printLog( "Cannot copy user's ODBC config file:  $tmpfile\n" );
			printLog( "Copy error:  $!\n" );
			return 0;
		}
		chmod( $tmpfile, 0600 );

		# Open the backup for reading and the original for writing.
		# This will truncate the original file, but we'll write
		# new lines back into it.
		open( CONFIGFILE,    "<$tmpfile" );
		open( NEWCONFIGFILE, ">$userODBC" );	# Truncate file
		my $inSection = 0;
		my $hasContent = 0;
		foreach $line ( <CONFIGFILE> )
		{
			$hasContent = 1;
			if ( $line =~ /^\[[ \t]*PostgreSQL/ )
			{
				$inSection = 1;
			}
			elsif ( $line =~ /^[ \t]*$/ || $line =~ /^\[/ )
			{
				$inSection = 0;
			}
			if ( ! $inSection )
			{
				print( NEWCONFIGFILE $line );
			}
		}

# New section added 8/21/08 to update ~/.odbc.ini to have the
# [PostgreSQL] section as this seems to be needed on some hosts.  Note
# that this may change the user's version of odbcpsql.so being used
# (but the previous ~/.odbc.ini file is saved).
		my $libPath = abs_path( File::Spec->catfile(
			      $databaseLibDir, "libodbcpsql.so" ) );

		print ( NEWCONFIGFILE "[PostgreSQL]\n" .
				"Driver=$libPath\n" .
				"Debug=0\n" .
				"CommLog=0\n" .
				"Servername=$thisHost\n" .
				"Database=$DB_NAME\n" .
				"ReadOnly=no\n" .
				"Ksqo=0\n" .
				"Port=$DATABASE_PORT\n" );
		$hasContent = 1;
# end new section

		close( NEWCONFIGFILE );
		close( CONFIGFILE );

		if ( $hasContent )
		{
			# Be sure edited file has the right permissions.
			chmod( $userODBC, 0600 );

			printStatus( "    Old file moved to $tmpfile\n" );
		}
		else
		{
			# The file is empty.  Just delete.
			unlink( $userODBC );
			unlink( $tmpfile );
			printStatus( "    Skipped.  Unused empty file deleted.\n" );
		}
	}
	return 1;
}


#
# @brief	'Create' an Oracle database.
#
# Stub that corresponds to the function creates a database, but for
# Oracle this is done by the DBA.
#
# @return
# 	A numeric code indicating success or failure:
# 		0 = failure
# 		1 = success
# 		2 = already created
#
sub Oracle_CreateDatabase()
{
    # If force mode, drop the tables (similar to dropping the db
    # in postgres.  The tables are then inserted later, if not present.
    if ( $force )
    {
	printStatus( "CreateDatabase: dropping tables...\n" );
	printLog( "CreateDatabase: dropping tables...\n" );
	my @sqlfiles = (
			"icatPurgeRecycleBin.sql",
			"icatDropCoreTables.sql",
			"icatDropSysTables.sql",
			"icatPurgeRecycleBin.sql");
	my $sqlfile;
	foreach $sqlfile (@sqlfiles)
	{
	    printLog( "    $sqlfile...\n" );
	    printStatus ( "    $sqlfile...\n" );
	    my $sqlPath = File::Spec->catfile( $serverSqlDir, $sqlfile );
	    my ($status,$output) = Oracle_sql( "unused", $sqlPath );
	    if ( $status != 0 )
	    {
		# Stop if any of the SQL scripts fail.
		printError( "\nInstall problem:\n" );
		printError( "    Could not drop the iCAT tables.\n" );
		printError( "        ", $output );
		printLog( "\nSQL failed:\n" );
		printLog( "    ", $output );
		cleanAndExit( 1 );
	    }
	}
    }
    else 
    {
	 printStatus( "CreateDatabase Skipped.  For Oracle, DBA creates the instance.\n" );
	 printLog( "CreateDatabase Skipped.  For Oracle, DBA creates the instance.\n" );
     }
    return 1;
}



#
# @brief	'Create' a MySQL database.
#
# Stub that corresponds to the function creates a database, but for
# MySQL this is done by the DBA.
#
# @return
# 	A numeric code indicating success or failure:
# 		0 = failure
# 		1 = success
# 		2 = already created
#
sub MySQL_CreateDatabase()
{
    # If force mode, drop the tables (similar to dropping the db
    # in postgres.  The tables are then inserted later, if not present.
    if ( $force )
    {
        printStatus( "CreateDatabase: dropping tables...\n" );
        printLog( "CreateDatabase: dropping tables...\n" );
        my @sqlfiles = (
                "icatPurgeRecycleBin.sql",
                "icatDropCoreTables.sql",
                "icatDropSysTables.sql",
                "icatPurgeRecycleBin.sql");
        my $sqlfile;
        foreach $sqlfile (@sqlfiles)
        {
            printLog( "    $sqlfile...\n" );
            printStatus ( "    $sqlfile...\n" );
            my $sqlPath = File::Spec->catfile( $serverSqlDir, $sqlfile );
            my ($status,$output) = MySQL_sql( $DB_NAME, $sqlPath );
            if ( $status != 0 )
            {
                # Stop if any of the SQL scripts fail.
                printError( "\nInstall problem:\n" );
                printError( "    Could not drop the iCAT tables.\n" );
                printError( "        ", $output );
                printLog( "\nSQL failed:\n" );
                printLog( "    ", $output );
                cleanAndExit( 1 );
            }
        }
    }
    else 
    {
        printStatus( "CreateDatabase Skipped.  For MySQL, DBA creates the instance.\n" );
        printLog( "CreateDatabase Skipped.  For MySQL, DBA creates the instance.\n" );
    }
    return 1;
}



#
# @brief	Show a list of tables in the database.
#
# This function issues an appropriate command to the database to
# get a list of existing tables.
#
# Messages are output on errors.
#
# @param	$dbname
# 	The database name.
# @return
# 	A list of tables, or undef on error.
#
sub Postgres_ListDatabaseTables($)
{
	my ($dbname) = @_;

	# Using psql, issue the \d command for a list of tables.
	my $tmpSql = createTempFilePath( "sql" );
	printToFile( $tmpSql, "\n\\d\n" );
	($status,$output) = execute_sql( $dbname, $tmpSql );
	unlink( $tmpSql );

	if ( $status == 0 )
	{
		return $output;
	}
	printLog( "    Cannot get list of tables from Postgres:\n" );
	printLog( "        ", $output );
	return undef;
}

#
# @brief	Show a list of tables in the database.
#
# This function issues an appropriate command to the database to
# get a list of existing tables.
#
# Messages are output on errors.
#
# @param	$dbname
# 	The database name (unused; see oracle_sql).
# @return
# 	A list of tables, or undef on error.
#
sub Oracle_ListDatabaseTables($)
{
	my ($dbname) = @_;

	# Using sqlplus, issue the oracle command for a list of tables.
	my $tmpSql = createTempFilePath( "sql" );
	printLog( "    Oracle_ListDB\n" );
	printToFile( $tmpSql, "select table_name from tabs;\n" );
	($status,$output) = execute_sql( $dbname, $tmpSql );
	unlink( $tmpSql );

	if ( $status == 0 )
	{
		return $output;
	}
	printLog( "    Cannot get list of tables from Oracle:\n" );
	printLog( "        ", $output );
	return undef;
}

#
# @brief	Show a list of tables in the database.
#
# This function issues an appropriate command to the database to
# get a list of existing tables.
#
# Messages are output on errors.
#
# @param	$dbname
# 	The database name (unused; see oracle_sql).
# @return
# 	A list of tables, or undef on error.
#
sub MySQL_ListDatabaseTables($)
{
	my ($dbname) = @_;

	# Using mysql, issue the SQL command for a list of tables.
	my $tmpSql = createTempFilePath( "sql" );
	printLog( "    MySQL_ListDB\n" );
	printToFile( $tmpSql, "show tables\n" );
	($status,$output) = execute_sql( $dbname, $tmpSql );
	unlink( $tmpSql );

	if ( $status == 0 )
	{
		return $output;
	}
	printLog( "    Cannot get list of tables from MySQL:\n" );
	printLog( "        ", $output );
	return undef;
}





#
# @brief	Update an ODBC configuration file to include a
# 		given name and value.
#
# This function edits an ODBC '.ini' file and insures that the given name
# and value are set in the given section.  The rest of the file is left
# untouched.
#
# @param	$filename
# 	The name of the ODBC file to change.
# @param	$section
# 	The section of the ODBC file to change.
# @param	$name
# 	The name to change or add to the section.
# @param	$value
# 	The value to set for that name.
# @return
# 	A numeric value indicating:
# 		0 = failure
# 		1 = success, file changed
# 		2 = success, file no changed (OK as-is)
#
sub Postgres_updateODBC()
{
	my ($filename,$section,$name,$value) = @_;

	# Edit the file in-place, thereby insuring that the
	# file maintains its permissions, ownership, hard links,
	# and other attributes.

	# Copy the original file to a temp file.
	my $tmpfile = createTempFilePath( "odbc" );
	if ( copy( $filename, $tmpfile ) != 1 )
	{
		printError( "\nInstall problem:\n" );
		printError( "    Cannot copy system's ODBC configuration file.\n" );
		printError( "        File:  $filename\n" );
		printError( "        Error: $!\n" );
		printLog( "Cannot copy user's ODBC config file:  $tmpfile\n" );
		printLog( "Copy error:  $!\n" );
		return 0;
	}
	chmod( 0600, $tmpfile );

	# Open the copy for reading and write to the original
	# file.  This will truncate the original file, then we'll
	# add lines back into it.
	open( CONFIGFILE,    "<$tmpfile" );
	open( NEWCONFIGFILE, ">$filename" );	# Truncate
	my $inSection = 0;
	my $nameFound = 0;
	my $fileChanged = 0;
	foreach $line ( <CONFIGFILE> )
	{
		if ( $line =~ /^\[[ \t]*$section/ )
		{
			# We're in the target section.  Flag it
			# and copy the section heading.
			$inSection = 1;
			print( NEWCONFIGFILE $line );
			next;
		}

		if ( $inSection && $line =~ /^$name/ )
		{
			# We've got a line that sets the target
			# value to change.
			$nameFound = 1;
			if ( $line !~ /$name=$value/ )
			{
				# The value isn't the target
				# value, so rewrite the line.
				print( NEWCONFIGFILE "$name=$value\n" );
				$fileChanged = 1;
			}
			else
			{
				# The value is already correct.
				# No need to change it.  Copy
				# the existing line.
				print( NEWCONFIGFILE $line );
			}
			next;
		}

		if ( $inSection && $line =~ /^[ \t]*$/ || $line =~ /^\[/ )
		{
			# We've hit the end of the target section
			# by getting a blank line or a new section.
			# If the target name hasn't been set yet,
			# then add it now.
			if ( ! $nameFound )
			{
				print( NEWCONFIGFILE "$name=$value\n" );
				$nameFound = 1;
				$fileChanged = 1;
			}

			# And output the line that ended the
			# target section (blank or a new section).
			$inSection = 0;
			print( NEWCONFIGFILE $line );
			next;
		}

		# The line is fine as-is.  Just copy it.
		print( NEWCONFIGFILE $line );
	}

	# If we hit the end of the file while still in the
	# target section, and the name hasn't been output yet,
	# then write it out.
	if ( $inSection && ! $nameFound )
	{
		print( NEWCONFIGFILE "$name=$value\n" );
		$fileChanged = 1;
	}
	close( NEWCONFIGFILE );
	close( CONFIGFILE );

	# Remove the old copy.
	unlink( $tmpfile );

	return 1 if ( $fileChanged );
	return 2;
}





#
# @brief	Configure security settings for a Postgres database.
#
# This function adjusts the default Postgres pg_hba.conf file to
# require MD5 scrambling of passwords.
#
# Messages are output on errors.
#
sub Postgres_ConfigureDatabaseSecurity()
{
	# The Postgres host list.
	my $pghbaconf = File::Spec->catfile( $databaseDataDir, "pg_hba.conf" );


	# Configure Postgres so that ODBC will need to use MD5-password
	# Check first to see if we've already modified the file
	# during a previous run of the script.
	printStatus( "Updating Postgres pg_hba.conf...\n" );
	printLog( "Updating Postgres pg_hba.conf...\n" );

	my $flagInsertedLines = "# iRODS connections:";

	my $newPghbaconf = createTempFilePath( "pghba" );
	my $pghbaAlreadyModified = 0;
	my $pghbaLocalFound = 0;
	open( NEWPGHBA,  ">$newPghbaconf" );
	open( ORIGPGHBA, "<$pghbaconf" );
	my $line;
	foreach $line ( <ORIGPGHBA> )
	{
		# Note if we've already modified this file.
		if ( $line =~ /^$flagInsertedLines/ )
		{
			$pghbaAlreadyModified = 1;
		}

		# Watch for a local line and make sure it is there.
		elsif ( $line =~ /^local/i )
		{
			$pghbaLocalFound = 1;
		}

		# Replace any use of the 'trust' method with 'md5'.
		if ( $line =~ /trust[ \t]*$/i )
		{
			$line =~ s/trust/md5/;
		}

		print( NEWPGHBA $line );
	}

	print( NEWPGHBA "\n$flagInsertedLines\n" );
	print( NEWPGHBA "#     Force use of md5 scrambling for all connections.\n" );


	# Legacy host list method:
	#   Previous scripts added one or more 'host' lines using host IP
	#   addresses.  Unfortunately, IP addresses change for hosts on
	#   DHCP, so this isn't reliable.  Instead, we'll use match-all
	#   entries below.
	#foreach $ip (keys %thisHostAddresses)
	#{
	#	print( NEWPGHBA
	#		"host    all         all         $ip  255.255.255.255   md5\n" );
	#}

	#  Instead:
	#	use "0.0.0.0/0" to match all IPv4 addresses.
	#	use "::/0" to match all IPv6 addresses.
	print( NEWPGHBA "host    all         all         0.0.0.0/0             md5\n" );
	print( NEWPGHBA "host    all         all         ::/0                  md5\n" );
	if ( ! $pghbaLocalFound )
	{
		print( NEWPGHBA
			"local   all         all                               md5\n" );
	}
	close( NEWPGHBA );
	close( ORIGPGHBA );

	if ( $pghbaAlreadyModified )
	{
		# We've already modified the file, so nevermind.
		unlink( $newPghbaconf );
		printStatus( "    Skipped.  File already uptodate.\n" );
		printLog( "    Skipped.  File already uptodate.\n" );
	}
	else
	{
		# Remove the old file and copy in the modified file.
		printLog( "    Removing old pg_hba.conf...\n" );
		unlink( $pghbaconf );
		printLog( "    Replacing with new pg_hba.conf...\n" );
		move( $newPghbaconf, $pghbaconf );
		$databaseRestartNeeded = 1;
	}
}





#
# @brief	Send a file of SQL commands to the Postgres database.
#
# This function runs 'psql' and passes it the given SQL.
#
# @param	$databaseName
# 	the name of the database
# @param	$sqlFilename
# 	the name of the file
# @return
# 	A 2-tuple including a numeric code indicating success
# 	or failure, and the stdout/stderr of the command.
# 		0 = failure
# 		1 = success
#
sub Postgres_sql($$)
{
	my ($databaseName,$sqlFilename) = @_;

	return run( "$psql $databaseName < $sqlFilename" );
}

#
# @brief	Send a file of SQL commands to the Oracle database.
#
# This function runs 'sqlplus' and passes it the given SQL.
#
# @param	$databaseName
# 	the name of the database  (Not used); instead 
#    $DATABASE_ADMIN_NAME and $DATABASE_ADMIN_PASSWORD are spliced together
# 	  in the form needed by sqlplus (note: should restructure the args in
#    the call tree for this, perhaps just let this and Postgres_sql set these.)
# @param	$sqlFilename
# 	the name of the file
# @return
# 	A 2-tuple including a numeric code indicating success
# 	or failure, and the stdout/stderr of the command.
# 		0 = failure
# 		1 = success
#
sub Oracle_sql($$)
{
	my ($databaseName,$sqlFilename) = @_;
	my $connectArg, $i;
	$i = index($DATABASE_ADMIN_NAME, "@");
	$connectArg = substr($DATABASE_ADMIN_NAME, 0, $i) . "/" . 
		 $DATABASE_ADMIN_PASSWORD .  substr($DATABASE_ADMIN_NAME, $i);
	return run( "$sqlplus '$connectArg' < $sqlFilename" );
}

#
# @brief	Send a file of SQL commands to the MySQL database.
#
# This function runs 'isql' and passes it the given SQL.
#
# @param	$databaseName
# 	the name of the database
# @param	$sqlFilename
# 	the name of the file
# @return
# 	A 2-tuple including a numeric code indicating success
# 	or failure, and the stdout/stderr of the command.
# 		0 = failure
# 		1 = success
#
sub MySQL_sql($$)
{
	my ($databaseName,$sqlFilename) = @_;
	return run( "$mysql < $sqlFilename" );
}


#
# @brief	Start Postgres server
#
# This function starts the Postgres server and confirms that
# it started.
#
# Messages are output on errors.
#
# @return
# 	A numeric code indicating success or failure:
# 		0 = failure
# 		1 = started
# 		2 = already started
#
sub Postgres_startDatabase()
{
	# Is Postgres running?
	my ($status,$output) = run( "$pgctl status" );
	return 2 if ($status == 0);	# Already started

	# Start it.
	my $logpath = File::Spec->catfile( $databaseLogDir, "pgsql.log" );
	($status,$output) = run( "$pgctl start -o '-i' -l $logpath" );
	if ( $status != 0 )
	{
		printError( "Could not start Postgres database server.\n" );
		printError( "    $output\n" );
		printLog( "\nCould not start Postgres database server.\n" );
		printLog( "    $output\n" );
		return 0;
	}

	# Give it time to start up
	sleep( $databaseStartStopDelay );
	return 1;
}





#
# @brief	Stop Postgres server
#
# This function stops the Postgres server and confirms that
# it stopped.
#
# Messages are output on errors.
#
# @return
# 	A numeric code indicating success or failure:
# 		0 = failure
# 		1 = stopped
# 		2 = already stopped
#
sub Postgres_stopDatabase()
{
	# Is Postgres running?
	my ($status,$output) = run( "$pgctl status" );
	return 2 if ($status != 0);	# Already stopped

	# Stoop it.
	my $logpath = File::Spec->catfile( $databaseLogDir, "pgsql.log" );
	($status,$output) = run( "$pgctl stop" );
	if ( $status != 0 )
	{
		printError( "Could not stop Postgres database server.\n" );
		printError( "    $output\n" );
		printLog( "\nCould not stop Postgres database server.\n" );
		printLog( "    $output\n" );
		return 0;
	}

	# Give it time to stop
	sleep( $databaseStartStopDelay );
	return 1;
}
