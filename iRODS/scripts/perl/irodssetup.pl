#
# Perl

#
# Build and set up iRODS.
#
# Usage is:
#	perl irodssetup.pl [options]
#
# This script automatically builds everything you need for iRODS
# and sets up a default configuration.  It prompts for any information
# it needs.
#

use File::Spec;
use File::Copy;
use Cwd;
use Cwd "abs_path";
use Config;

$version{"irodssetup.pl"} = "September 2011";





#
# Design Notes:
#	This script is a front-end to other scripts.  Its purpose is
#	to provide an "easy" prompt-based way to build iRODS.  It
#	does not prompt for every possible configuration choice - only
#	those essential to the build.  Advanced users may edit the
#	configuration files directly instead.
#
#	The bulk of this script is prompts.  The real work amounts
#	to running:
#		scripts/installPostgres
#			Install Postgres, if needed.
#		scripts/configure
#			Configure the build of iRODS.
#		make
#			Build iRODS.
#		scripts/finishSetup
#			Configure the database and test.
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

# Check commandline
my $isUpgrade="";
my $arg;
foreach $arg (@ARGV)
{
    if ( $arg =~ /-?-?up/ )	# irodsupgrade, not irodssetup
    {
	$isUpgrade="upgrade";
	printf("Upgrade mode\n");
    }
}

# Load support scripts.
my $perlScriptsDir = File::Spec->catdir( $IRODS_HOME, "scripts", "perl" );
require File::Spec->catfile( $perlScriptsDir, "utils_paths.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_print.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_file.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_platform.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_config.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_prompt.pl" );

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
my $makePath   = getCurrentMake( );

my $thisUserID = $<;
my $thisHost   = getCurrentHostName( );

# Paths to other scripts
my $irodsPrompt     = File::Spec->catfile( $IRODS_HOME, "scripts", "irodsprompt" );
my $installPostgres = File::Spec->catfile( $IRODS_HOME, "scripts", "installPostgres" );
my $configure       = File::Spec->catfile( $IRODS_HOME, "scripts", "configure" );
my $setupFinish     = File::Spec->catfile( $IRODS_HOME, "scripts", "finishSetup" );

# Paths to other files
my $installPostgresConfig = File::Spec->catfile( $configDir, "installPostgres.config" );
my $makeLog               = File::Spec->catfile( $logDir, "installMake.log" );
my $installPostgresLog    = File::Spec->catfile( $logDir, "installPostgres.log" );
my $finishSetupLog        = File::Spec->catfile( $logDir, "finishSetup.log" );





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
	exit( 1 );
}





########################################################################
#
# Install
#


if ( $isUpgrade ne "") {
    printNotice("\nNote: When upgrading from 2.5 to 3.0, you should run\n");
    printNotice("psg-patch-v2.5tov3.0.sql via psql (for Postgres)\n");
    printNotice("or ora-patch-v2.5tov3.0.sql via sqlplus (for Oracle)\n");
    printNotice("or mys-patch-v2.5tov3.0.sql via mysql (for MySQL)\n");
    printNotice("before running this script.\n");
    printNotice("If you have an older iRODS, you need to run the\n");
    printNotice("other patch scripts in sequence.\n\n");

    my $answer = promptYesNo(
			     "Have you run one of those?",
		"yes" );
    if ( $answer == 0 )
    {
	printError( "Abort.\n" );
	exit( 0 );
    }
}

# Prompt for information.
#	Asks lots of questions and sets config files
promptUser( );


printSubtitle(
	"Build and configure\n",
	"-------------------\n" );


# Run the installation.
#	Runs 'irodsctl' to stop servers, if any
prepare( );


# Install database for the catalog server.
#	Runs 'installPostgres'
if ($isUpgrade eq "") {
    installDatabase( );
}


# Configure iRODS
#	Runs 'configure'
configureIrods( );


# Build iRODS
#	Runs 'make'
buildIrods( );


# Finish setting up iRODS
#	Runs 'setupFinish'
finishSetup( );



# Done!
printSubtitle(
	"\n",
	"Done!\n",
	"-----\n" );

if ( $installDataServer )
{
	printNotice("For special network interfaces, you may need to\n");
	printNotice("modify server/config/irodsHost; see comments within it.\n");
}

printNotice( "\nTo use the iRODS command-line tools, update your PATH:\n" );
printNotice( "    For csh users:\n" );
printNotice( "        set path=($icommandsBinDir \$path)\n" );
printNotice( "    For sh or bash users:\n" );
printNotice( "        PATH=$icommandsBinDir:\$PATH\n" );

printNotice("\nIf you wish to set the ports to use, set the environment variable\n");
printNotice("'svrPortRangeStart' or edit the svrPortRangeStart line in irodsctl.pl.\n");
printNotice("See the 'Specifying Ports' page on the irods web site for more.\n");

if ( $installDataServer )
{
	printNotice( "\nTo start and stop the servers, use 'irodsctl':\n" );
	printNotice( "    irodsctl start\n" );
	printNotice( "    irodsctl stop\n" );
	printNotice( "    irodsctl restart\n" );
	printNotice( "Add '--help' for a list of commands.\n" );

	printNotice(
		    "\n",
		    "Please see the iRODS documentation for additional notes on how\n",
		    "to manage the servers and adjust the configuration.\n" );
}
else {
	printNotice("\nFor i-commands-only builds like this, irodssetup does not\n");
	printNotice("attempt to create your ~/.irods/.irodsEnv file, so you need to\n");
	printNotice("create or update it by hand yourself. See the iRODS web site page\n" );
	printNotice("on the User Environment.  You may wish to copy .irodsEnv from\n");
	printNotice("another host.\n" );
}

exit( 0 );





########################################################################

# @brief	Prompt for the iRODS configuration
#
# Run the prompt script to collect information about the install.
# The script saves its results into irods.config and
# installPostgres.config.
#
# Status and error messages are output directly.
# On a failure, this function exits the script.
#
sub promptUser( )
{
	# Run the prompt script.  The script sets irods.config and
	# installPostgres.config based upon user choices.
	if ( $isUpgrade ne "") {
	    system( "$irodsPrompt --upgrade");
	}
	else {
	    system( $irodsPrompt );
	}
	if ( $? ne 0 )
	{
		# There is no log file from the prompt script.
		# Its errors are printed out directly.

		# But if there is a blank in the path name (that is,
		# within a folder name), 'system' will fail and no
		# messages are displayed.  Blanks are common on Macs,
		# but can be done on other OSes too.  So check for
		# this and print a message.  We could modify the
		# $irodsPrompt path to have '\ ' instead of ' ' but
		# even with that other errors occur later.  So it is
		# better to just avoid it.  A message, tho, is needed.

		if ($irodsPrompt =~ / / ) {
			printError(
"There is a blank in one of the folder names above 'iRODS' which will
cause various errors.  Please move iRODS to another folder or rename
the containing folder so as to have no blanks.\n");
		}

		exit( 1 );
	}
	printNotice( "\n\n" );

	my $answer = promptYesNo(
		"Start iRODS build",
		"yes" );
	if ( $answer == 0 )
	{
		printError( "Abort.\n" );
		exit( 0 );
	}
	printNotice( "\n\n" );

	# Load irods.config without validation.  At this point the
	# configuration file may reference a non-existant database
	# directory, etc., because we haven't installed it yet.  So,
	# we can't validate.  That'll happen later on.
	if ( loadIrodsConfigNoValidate( ) == 0 )
	{
		# Configuration failed to load.  An error message
		# has already been output.
		exit( 1 );
	}

	# Set some global flags based upon the configuration file
	$installDataServer = 0;
	if ( $IRODS_ADMIN_NAME ne "" )
	{
		# There is an account name.  Install an iRODS server.
		$installDataServer = 1;
	}

	$installDatabaseServer = 0;
	$deleteDatabaseData = 0;
	if ( $DATABASE_TYPE =~ /postgres/i && -e $installPostgresConfig )
	{
		# There is a database choice and installPostgres.config
		# exists.  Install a new database.
		$installDatabaseServer = 1;
	}
}





# @brief	Prepare to install
#
# Stop the servers, if there are any.  Ignore errors.
#
sub prepare( )
{
	# On a fresh install, there won't be any servers running.
	# But on a re-install, there might be.  Stop them.
	#
	# Now we do tell users were running irodsctl stop since it is
	# more confusing to users if we hide what's going on.
	# We still say we are "Preparing" but give a message about 
	# the 'irodsctl stop'.
	#
	# Ignore error messages.  If this is the first install, then
	# 'irodsctl' won't work because configuration files won't be
	# right yet.  That's fine because it probably means there are
	# no servers to stop anyway.
	# 
	# But especially with recent changes, there may be other
	# irodsservers running.  If on another port, this is OK, but
	# on a fresh install or dedicated host re-install, it is not.
	# So check, and warn the user if some are running.
	#
	printSubtitle( "\nPreparing...\n" );

	# Don't do any of this if we're only building clients.
	return if ( $installDataServer == 0 ); 


	printNotice( "    Running 'irodsctl --quiet stop' in case previous is running.\n" );
	`$irodsctl --quiet stop 2>&1`;
	# Ignore errors.

	# Sleep a bit and warn user if irods servers are running
	sleep(2);
	$ps_opt = "-el";
        if ($thisOS =~ /Darwin/i ) {
	    $ps_opt = "-x";
	}
	$psOut1 = `ps $ps_opt | grep irodsServer | grep -v defunct | grep -v grep`;
	$psOut2 = `ps $ps_opt | grep irodsReServer | grep -v defunct | grep -v grep`;
	$psOut3 = `ps $ps_opt | grep irodsAgent | grep -v defunct | grep -v grep`;
	$psOut =  $psOut1 . $psOut2 . $psOut3;
	if (length($psOut)) {
	    printError( "\nWarning, some irods processes are running\n" );
	    printError( "Will ignore, but if you are trying to run one only irods system,\n");
	    printError( "you might need to kill them and re-run irodssetup.\n");
	    printError( "ps $ps_opt | grep ...  results:\n$psOut\n" );
	}
}




# @brief	Install the database, if needed
#
# This function checks if the user has asked for a Postgres
# database to be installed.  If so, it updates the install script's
# configuration file with the user's choices, then runs
# the install script.
#
# Status and error messages are output directly.
# On a failure, this function exits the script.
#
sub installDatabase( )
{
	return if ( !$installDatabaseServer );

	printSubtitle( "\nInstalling Postgres database...\n" );

	# Run the Postgres install script.
	#
	# Note that since we don't need to redirect the script's
	# output to a variable, we don't need a subshell to run
	# the script.  So, we use system() instead of exec() or
	# backquotes.  This skips the subshell, sends output to
	# the user's screen, and is more efficient.
	#
	# Arguments passed to the script:
	# 	--noask
	# 		Don't prompt the user for anything.
	# 		We have already collected the data the
	# 		script needs and added it to the Postgres
	# 		config file.
	#
	# 	--noheader
	# 		Don't output a header message.  We already
	# 		have and the script's own header would just
	# 		add clutter.
	#
	# 	--indent
	# 		Indent the script's own messages so that
	# 		they look nice together with this script's
	# 		messages.
	#
	my @options = ( "--noask", "--noheader", "--indent" );
	system( $installPostgres, @options );
	if ( $? ne 0 )
	{
		printError( "Installation failed.  Please see the log file for details:\n" );
		printError( "    $installPostgresLog\n" );
		exit( 1 );
	}

	# The Postgres install script automatically transfers its settings
	# into the irods.config script used as a starting point by the
	# configure script run next.
}





# @brief	Configure iRODS
#
# Use the user's choices to intialize the 'irods.config' file
# and choose options for the 'configure' script, then run that
# script.
#
# Status and error messages are output directly.
# On a failure, this function exits the script.
#
sub configureIrods( )
{
	printSubtitle( "\nConfiguring iRODS...\n" );

	# Run the configure script.
	#
	# See the note earlier about using system() instead of exec() or
	# backquotes.
	#
	# Arguments passed to the script:
	# 	--noheader
	# 		Don't output a header message.  We already
	# 		have and the script's own header would just
	# 		add clutter.
	#
	# 	--indent
	# 		Indent the script's own messages so that
	# 		the look nice together with this script's
	# 		messages.
	#
	# The configure script reads irods.config for its
	# initial options.
	#
	system( $configure,
		"--noheader",		# Don't output a header message
		"--indent" );		# Indent everything
	if ( $? ne 0 )
	{
		printError( "Configure failed.\n" );
		# There is no log file from the configure script.
		# Its errors are printed out directly.
		exit( 1 );
	}
}





# @brief	Compile iRODS
#
# After configuration, the makefiles have been set up with
# the user's choices.  Run make to compile iRODS.
#
# Compile everything.  Route the make output to the log file
# so that it doesn't spew lots of detail at the user.
#
# Status and error messages are output directly.
# On a failure, this function exits the script.
#
sub buildIrods( )
{
	printSubtitle( "\nCompiling iRODS...\n" );

	unlink( $makeLog );

	my $totalSteps = 2;
	if ( $installDataServer )
	{
		$totalSteps++;
	}
	my $currentStep = 0;

	# Build the library and icommands
	++$currentStep;
	printSubtitle( "\n        Step $currentStep of $totalSteps:  Compiling library and i-commands...\n" );
	`$makePath icommands >> $makeLog 2>&1`;
	if ( $? ne 0 )
	{
		printError( "Compilation failed.  Please see the log file for details:\n" );
		printError( "    $makeLog\n" );
		exit( 1 );
	}

	if ( $installDataServer )
	{
		# Build the server
		++$currentStep;
		printSubtitle( "\n        Step $currentStep of $totalSteps:  Compiling iRODS server...\n" );
		`$makePath server >> $makeLog 2>&1`;
		if ( $? ne 0 )
		{
			printError( "Compilation failed.  Please see the log file for details:\n" );
			printError( "    $makeLog\n" );
			exit( 1 );
		}
	}

	# Build the tests
	++$currentStep;
	printSubtitle( "\n        Step $currentStep of $totalSteps:  Compiling tests...\n" );
	`$makePath test >> $makeLog 2>&1`;
	if ( $? ne 0 )
	{
		printError( "Compilation failed.  Please see the log file for details:\n" );
		printError( "    $makeLog\n" );
		exit( 1 );
	}
}




# @brief	Finish setting up iRODS
#
# After building iRODS, its time to install the iRODS tables
# and adjust configuration files.
#
# Status and error messages are output directly.
# On a failure, this function exits the script.
#
sub finishSetup( )
{
	return if ( $installDataServer == 0 );


	printSubtitle( "\nSetting up iRODS...\n" );

	# Run the completion script.
	#
	# See the note earlier about using system() instead of exec() or
	# backquotes.
	#
	# Arguments passed to the script:
	# 	--noask
	# 		Don't prompt the user for anything.
	# 		We have already collected the data the
	# 		script needs and added it to the Postgres
	# 		config file.
	#
	# 	--noheader
	# 		Don't output a header message.  We already
	# 		have and the script's own header would just
	# 		add clutter.
	#
	# 	--indent
	# 		Indent the script's own messages so that
	# 		the look nice together with this script's
	# 		messages.
	# 	--upgrade
	# 		Finish in Upgrade mode.
	#
	if ($isUpgrade eq "") {
	    system( $setupFinish,
		"--noask",
		"--noheader",
		"--indent" );
	}
	else {
	    system( $setupFinish,
		"--noask",
		"--noheader",
		"--indent",
		"--upgrade" );
	}

	if ( $? ne 0 )
	{
		printError( "Set up failed.  Please see the log file for details:\n" );
		printError( "    $finishSetupLog\n" );
		printError( "The server/log/rodsLog* file may also contain useful information.\n" );
		exit( 1 );
	}
}
