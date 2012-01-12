#
# Perl

#
# Prompt and create iRODS install configuration files.
#
# Usage is:
#	perl irodsprompt.pl [options]
#
# This script runs through a series of prompts for the user to
# select installation options.  This information is used to
# create or modify configuration files prior to an install.
#

use File::Spec;
use File::Copy;
use Cwd;
use Cwd "abs_path";
use Config;

$version{"irodsprompt.pl"} = "September 2011";





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
require File::Spec->catfile( $perlScriptsDir, "utils_prompt.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_config.pl" );

# Determine the execution environment.
my $thisUser   = getCurrentUser( );
my $thisHost   = getCurrentHostName( );

# Paths to other files
my $installPostgresConfig = File::Spec->catfile( $configDir, "installPostgres.config" );
my $installPostgresConfigTemplate = File::Spec->catfile( $configDir, "installPostgres.config.template" );

my $isUpgrade="";



########################################################################
#
# Check script usage.
#
setPrintVerbose( 1 );

# Don't allow root to run this.
if ( $< == 0 )
{
	printError( "Usage error:\n" );
	printError( "    This script should *not* be run as root.\n" );
	exit( 1 );
}

foreach $arg (@ARGV)
{
	# Options
	if ( $arg =~ /^-?-?h(elp)?$/ )		# Help / usage
	{
		printUsage( );
		exit( 0 );
	}
	if ( $arg =~ /--upgrade/ )	# upgrade mode
	{
		$isUpgrade="upgrade";
	}

	if ($isUpgrade eq "" ) {
	    printError( "Unknown command:  $arg\n" );
	    printUsage( );
	    exit( 1 );
	}
}





########################################################################
#
# Defaults
#	All of the following defaults may be overwritten by values in
#	an existing iRODS configuration file.  During prompts, these
#	values, or those from the configuration file, provide default
#	choices.  An "undef" value omits a default and requires that
#	the user enter something.
#

# Default values used when there isn't an existing irods.config,
# or when irods.config has empty values and prompting finds that
# values are needed.
$DEFAULT_irodsAccount		= "rods";
$DEFAULT_irodsPassword  	= "rods";
$DEFAULT_irodsPort		= "1247";
$DEFAULT_svrPortRangeStart	= "20000";
$DEFAULT_svrPortRangeEnd	= "20199";
$DEFAULT_irodsDbName		= "ICAT";
$DEFAULT_irodsDbKey		= "123";
$DEFAULT_irodsResourceName	= "demoResc";
$DEFAULT_irodsResourceDir	= "$IRODS_HOME/Vault";
$DEFAULT_irodsZone		= "tempZone";
# No default remote iCAT host

$DEFAULT_databaseServerType	= "postgres";
$DEFAULT_databaseServerOdbcType	= "unix";
$DEFAULT_databaseServerPort	= "5432";
$DEFAULT_databaseServerAccount	= $thisUser;
# No default database password
# No default database directory

# Set the initial iRODS configuration to the defaults
$irodsAccount		= $DEFAULT_irodsAccount;	# Prompt.
$irodsPassword		= $DEFAULT_irodsPassword;	# Prompt.
$irodsPort		= $DEFAULT_irodsPort;		# Prompt [advanced].
$svrPortRangeStart	= undef;		        # Prompt [advanced].
$svrPortRangeEnd	= undef;		        # Prompt [advanced].
$irodsDbName		= $DEFAULT_irodsDbName;		# Prompt [advanced].
$irodsDbKey		= $DEFAULT_irodsDbKey;		# Prompt [advanced].
$irodsResourceName	= $DEFAULT_irodsResourceName;	# Prompt [advanced].
$irodsResourceDir	= $DEFAULT_irodsResourceDir;	# Prompt [advanced].
$irodsZone		= $DEFAULT_irodsZone;		# Prompt [advanced].
$catalogServerHost	= undef;			# Prompt.

# Set the initial database configuration to the defaults
$databaseServerType	= $DEFAULT_databaseServerType;	# Prompt.
$databaseServerOdbcType	= $DEFAULT_databaseServerOdbcType; # Prompt.
$databaseServerPath	= undef;			# Prompt.
$databaseServerLib	= undef;			# Prompt.
$databaseServerExclusive = 1;				# Prompt.

$databaseServerHost	= "localhost";			# Prompt.
$databaseServerPort	= $DEFAULT_databaseServerPort;	# Prompt [advanced].
$databaseServerAccount	= $DEFAULT_databaseServerAccount;# Prompt.
$databaseServerPassword	= undef;			# Prompt.

# Postgres and ODBC versions
$postgresSource         = '';				# Prompt.
$odbcSource             = '';				# Prompt.


# GSI
$gsiAuth		= 0;
$globusLocation         = undef;
$gsiInstallType         = undef;

# Audit extension
$auditExt               = 0;

# Actions to take
$installDataServer	= 1;				# Prompt.
$installCatalogServer	= 1;				# Prompt.
$installDatabaseServer	= 1;				# Prompt.
$priorDatabaseExists	= 0;				# Prompt.
$deleteDatabaseData	= 0;				# Prompt.







########################################################################
#
# Prompt
#

# Intro
printTitle(
	"iRODS configuration setup\n",
	"----------------------------------------------------------------\n" );

printNotice(
	"This script prompts you for key iRODS configuration options.\n",
	"Default values (if any) are shown in square brackets [ ] at each\n",
	"prompt.  Press return to use the default, or enter a new value.\n",
	"\n",
	"\n" );

if ($isUpgrade eq "upgrade" ) {
    if ( !-e $irodsConfig ) {
	printNotice(
	     "For an upgrade, the easiest way to get all the settings is to copy the\n",
	     "config/irods.config file from your previous installation to this\n",
             "current source tree.  Don't enter 'yes' to exit now so you can do the\n",
             "copy and then rerun this.\n",
	     "\n" );
	my $answer = promptYesNo(
		"Continue without a previous irods.config file",
		"yes" );

	if ( $answer == 0 )
	{
		# Skip prompting and exit with error to exit parent
		exit( 1 );
	}
    }
}
if ( -e $irodsConfig )
{
	# An irods.config file already exists.  Ask if the
	# user would like to use it as-is or change it via
	# prompts.
	printNotice(
		"A prior iRODS configuration file was found.  This script can\n",
		"prompt you for changes, or use the same configuration again.\n",
		"You can also change parameters by editing config/irods.config\n",
		"and restarting this script.\n",
		"\n" );
	my $answer = promptYesNo(
		"Use the existing iRODS configuration without changes",
		"yes" );

	if ( $answer == 1 )
	{
		# Skip prompting and re-use the configuration files.
		exit( 0 );
	}
	printNotice( "\n\n" );

	# Load the irods.config file and use it for defaults
	# for all prompts.  Don't validate since at this point
	# it may have incomplete information - such as a DB
	# directory that doesn't exist until we run that part
	# of the install.
	if ( loadIrodsConfigNoValidate() == 0 )
	{
		# Configuration failed to load.  An error message
		# has already been output.
		exit( 1 );
	}


	# Map irods.config values to local variables.
	$irodsAccount      = $IRODS_ADMIN_NAME;
	$irodsPassword     = $IRODS_ADMIN_PASSWORD;
	$catalogServerHost = $IRODS_ICAT_HOST;
	$irodsPort         = $IRODS_PORT;
	$svrPortRangeStart = $SVR_PORT_RANGE_START;
	$svrPortRangeEnd   = $SVR_PORT_RANGE_END;

	$irodsDbName       = $DB_NAME;
	$irodsDbKey        = $DB_KEY;
	$irodsResourceName = $RESOURCE_NAME;
	$irodsResourceDir  = $RESOURCE_DIR;
	$irodsZone         = $ZONE_NAME;

	$gsiAuth           = $GSI_AUTH;
	$globusLocation    = $GLOBUS_LOCATION;
	$gsiInstallType    = $GSI_INSTALL_TYPE;

	$auditExt          = $AUDIT_EXT;

	$databaseServerType      = $DATABASE_TYPE;
	$databaseServerOdbcType  = $DATABASE_ODBC_TYPE;
	$databaseServerPath      = $DATABASE_HOME;
	$databaseServerLib       = $DATABASE_LIB;
	$databaseServerExclusive = $DATABASE_EXCLUSIVE_TO_IRODS;

	$databaseServerHost      = $DATABASE_HOST;
	$databaseServerPort      = $DATABASE_PORT;
	$databaseServerAccount   = $DATABASE_ADMIN_NAME;
	$databaseServerPassword  = $DATABASE_ADMIN_PASSWORD;

	$installCatalogServer = 1;
	if ( $catalogServerHost ne "" )
	{
		# An iCAT host has been given, which means this
		# host won't have an iCAT.
		$installCatalogServer = 0;
	}
	$installDataServer = 1;
	if ( $irodsDbName eq "" || $irodsResourceName eq "" ||
		$irodsResourceDir eq "" || $irodsZone eq "" )
	{
		# Some essential information about an iRODS
		# server install is missing, so it appears
		# that we should not install an iRODS data
		# server.
		$installDataServer = 0;
	}

	$installDatabaseServer = 1;
	if ( $databaseServerType =~ /oracle/i )
	{
		# Configured for Oracle, so we definately
		# are not installing the database here.
		$installDatabaseServer = 0;
	}
}


# Standard or advanced?
printNotice(
	"For flexibility, iRODS has a lot of configuration options.  Often\n",
	"the standard settings are sufficient, but if you need more control\n",
	"enter yes and additional questions will be asked.\n\n" );
$advanced = promptYesNo(
	"Include additional prompts for advanced settings",
	"no" );
printSubtitle( "\n\n" );


# Prompt
promptForIrodsConfiguration( );
printSubtitle( "\n\n" );

promptForDatabaseConfiguration( );
printSubtitle( "\n\n" );

promptForIrodsConfigurationPart2( ); 
printSubtitle( "\n\n" );

# Adjust paths to be absolute
if ( defined( $databaseServerPath ) && $databaseServerPath ne "" )
{
	$databaseServerPath = File::Spec->rel2abs( $databaseServerPath );
}
if ( defined( $irodsResourceDir ) && $irodsResourceDir ne "" )
{
	$irodsResourceDir   = File::Spec->rel2abs( $irodsResourceDir );
}

if ( promptForConfirmation( ) == 0 )
{
	printError( "Abort.\n" );
	exit( 1 );
}

# Set configuration files
configureNewPostgresDatabase( );
configureIrods( );
printNotice( "Saved.\n" );


if ( $installDataServer && $installCatalogServer &&
	!$installDatabaseServer )
{
# I'm removing this for now as it is wrong in some cases (perhaps all cases).
# If the old irodsctl controls the DBMS, the first step ('Prepare') will
# stop the DBMS.  No need to tell the user to start it just to stop it
# right after that.

#	printNotice(
#		"\n",
#		"Please be sure that the $databaseServerType DBMS on '$databaseServerHost'\n",
#		"is started and ready before continuing.\n" );
}
if ( $installDataServer && !$installCatalogServer )
{
	printNotice(
		"\n",
		"Please be sure that the iRODS iCAT-enabled server on '$catalogServerHost'\n",
		"is started and ready before continuing.\n" );
}


exit( 0 );





########################################################################

# @brief	Prompt for the iRODS configuration
#
# Through a series of prompts, collect information for an iRODS
# configuration.  Results are returned through global variables.
#
sub promptForIrodsConfiguration( )
{
	# Intro
	if ( $advanced )
	{
		printSubtitle(
			"iRODS configuration (advanced)\n",
			"------------------------------\n" );
	}
	else
	{
		printSubtitle(
			"iRODS configuration\n",
			"-------------------\n" );

	}

	printNotice(
		"iRODS consists of clients (e.g. i-commands) with at least one iRODS\n",
		"server.  One server must include the iRODS metadata catalog (iCAT).\n",
		"\n",
		"For the initial installation, you would normally build the server with\n",
		"the iCAT (an iCAT-Enabled Server, IES), along with the i-commands.\n",
		"\n",
		"After that, you might want to build another Server to support another\n",
		"storage resource on another computer (where you are running this now).\n",
		"You would then build the iRODS server non-ICAT, and configure it with\n",
		"the IES host name (the servers connect to the IES for ICAT operations).\n",
		"\n",
		"If you already have iRODS installed (an IES), you may skip building\n",
		"the iRODS server and iCAT, and just build the command-line tools.\n",

		"\n" );


	# Build the iRODS data server?
	$installDataServer = promptYesNo(
		"Build an iRODS server",
		(($installDataServer == 1) ? "yes" : "no") );
	if ( $installDataServer == 0 )
	{
		# Don't install the iRODS server.  Just the iCommands.
		$irodsAccount		= undef;
		$irodsPassword		= undef;
		$catalogServerHost	= undef;
		$irodsPort		= undef;
		$svrPortRangeStart      = undef;
		$svrPortRangeEnd        = undef;

		$irodsDbName		= undef;
		$irodsDbKey		= undef;
		$irodsResourceName	= undef;
		$irodsResourceDir	= undef;
		$irodsZone		= undef;

		$installCatalogServer	= 0;
		$installDatabaseServer	= 0;
		$priorDatabaseExists	= 0;
		$deleteDatabaseData	= 0;

		$databaseServerType	= undef;
		$databaseServerOdbcType	= undef;
		$databaseServerPath	= undef;
		$databaseServerExclusive = 0;

		$databaseServerHost	= undef;
		$databaseServerPort	= undef;
		$databaseServerAccount	= undef;
		$databaseServerPassword	= undef;
		return;
	}


	# Include the iCAT metadata catalog?
	$installCatalogServer = promptYesNo(
		"Make this Server ICAT-Enabled",
		(($installCatalogServer == 1) ? "yes" : "no") );
	if ( $installCatalogServer == 0 )
	{
		# Don't include the iCAT catalog.  Get the iCAT host, etc.
		$installDatabaseServer	= 0;
		$priorDatabaseExists	= 0;
		$deleteDatabaseData	= 0;

		$databaseServerType	= undef;
		$databaseServerOdbcType	= undef;
		$databaseServerPath	= undef;
		$databaseServerExclusive = 0;

		$databaseServerHost	= undef;
		$databaseServerPort	= undef;
		$databaseServerAccount	= undef;
		$databaseServerPassword	= undef;

		printNotice(
			"\n",
			"When an iCAT-enabled server is already available, the non-ICAT\n",
			"server needs to know the host name of the iCAT-enabled server.\n",
			"\n" );
		$catalogServerHost = promptHostName(
			"Host running iCAT-enabled iRODS server",
			$catalogServerHost );

# Resource name
		if ($isUpgrade eq "") {
		    printNotice(
			"\n",
			"A name is needed for the storage resource that will be on this host,\n",
			"and it needs to be different from other defined resource names.\n",
			"\n" );
		    $irodsResourceName = promptString(
						  "Resource name", 
			((!defined($irodsResourceName)||$irodsResourceName eq $DEFAULT_irodsResourceName) ?
				"demoResc2" : $irodsResourceName) );

# Resource directory
		    printNotice(
			"\n",
			"This resource will store iRODS data in a directory on this host.\n",
			"\n" );
		    $irodsResourceDir = promptString(
			"Resource storage area directory",
			((!defined($irodsResourceDir)||$irodsResourceDir eq "") ?
				$DEFAULT_irodsResourceDir : $irodsResourceDir) );
		}

		# iRODS account name and password.
		printNotice(
			"\n",
			"The irodsServer will authenticate to the other servers via\n",
			"an existing iRODS administrator account.\n",
			"\n" );
		$irodsAccount = promptIdentifier(
			 "Existing iRODS admin login name",
			((!defined($irodsAccount)||$irodsAccount eq "") ?
				$DEFAULT_irodsAccount : $irodsAccount) );

		$irodsPassword = promptIdentifier(
			 "Password",
			((!defined($irodsPassword)||$irodsPassword eq "") ?
				$DEFAULT_irodsPassword : $irodsPassword) );
		# iRODS zone
		printNotice(
		     "\n",
		     "Enter the zone as defined in your ICAT\n",
		      "\n" );
		$irodsZone = promptString(
				"iRODS zone name",
				((!defined($irodsZone)||$irodsZone eq "") ?
				 $DEFAULT_irodsZone : $irodsZone) );
	}
	else
	{
		# Include the iCAT.

	        # iRODS zone
		if ($isUpgrade eq "") {
		    printNotice(
			"\n",
			"Each set of distributed servers (perhaps hundreds, world-wide),\n",
			"supported by one ICAT-enabled server is an iRODS 'zone' and has a\n",
			"unique name.  This name appears at the beginning of collection names.\n",
			"In the future, zones will interoperate but for now each is\n",
			"independent.\n",
			"\n",
			"If you are reusing an existing ICAT database, please enter the\n",
			"existing zone name and irods account below.\n",
			"\n" );
		}
		else {
		    printNotice(
			"\n",
			"Please enter the existing zone name and irods account below.\n",
			"\n" );
		}
		$irodsZone = promptString(
			"iRODS zone name",
			((!defined($irodsZone)||$irodsZone eq "") ?
				$DEFAULT_irodsZone : $irodsZone) );


		$catalogServerHost = undef;	# No external host needed

		# iRODS account name and password.
		if ($isUpgrade eq "") {
		    printNotice(
			"\n",
			"The build process will create a new iRODS administrator account\n",
			"for managing the iRODS system (unless there is one already).\n",
			"\n" );
		}

		$irodsAccount = promptIdentifier(
			 "iRODS login name",
			((!defined($irodsAccount)||$irodsAccount eq "") ?
				$DEFAULT_irodsAccount : $irodsAccount) );

		$irodsPassword = promptIdentifier(
			"Password",
			((!defined($irodsPassword)||$irodsPassword eq "") ?
				$DEFAULT_irodsPassword : $irodsPassword) );
	}

	if ( $advanced )
	{
		# iRODS port
		printNotice(
			"\n",
			"[Advanced option]\n",
			"You can use a different port for the iRODS servers as long as all\n",
			"servers in your zone use the same one.\n",
			"\n" );
		$irodsPort = promptInteger(
			"Port",
			((!defined($irodsPort)||$irodsPort eq "") ?
				$DEFAULT_irodsPort : $irodsPort) );

		# port start and range
		printNotice(
			"\n",
			"[Advanced option]\n",
			"You can specify which dynamic port numbers to use (start and end)\n",
		        "in case you need to open these thru a firewall.\n",
			"\n" );
		$svrPortRangeStart = promptInteger(
			"Starting Server Port",
			((!defined($svrPortRangeStart)||$svrPortRangeStart eq "") ?
				$DEFAULT_svrPortRangeStart : $svrPortRangeStart) );
		$svrPortRangeEnd = promptInteger(
			"Ending Server Port",
			((!defined($svrPortRangeEnd)||$svrPortRangeEnd eq "") ?
				$DEFAULT_svrPortRangeEnd : $svrPortRangeEnd) );


		if ( $installCatalogServer == 1 ) {

		    # iRODS database name
		    printNotice(
			"\n",
			"[Advanced option]\n",
			"Sites that share a single database system (DBMS) supporting\n".
			"multiple iCAT databases (catalogs) need to use a different database\n",
			"name for each catalog.  Currently, this applies to Postgres but not\n",
			"Oracle.  For Oracle, this name is unused.\n",
			"\n" );
		    $irodsDbName = promptString(
			"iRODS database name",
			((!defined($irodsDbName)||$irodsDbName eq "") ?
				$DEFAULT_irodsDbName : $irodsDbName) );


		    # iRODS database key
		    printNotice(
			"\n",
			"[Advanced option]\n",
			"iRODS scrambles passwords stored in various files.\n",
			"The following ascii-string key is used to scramble the DB password.\n",
			"\n" );
		    $irodsDbKey = promptString(
			"iRODS DB password scramble key",
			((!defined($irodsDbKey)||$irodsDbKey eq "") ?
				$DEFAULT_irodsDbKey : $irodsDbKey) );


		    # iRODS resource name and directory
		    printNotice(
			"\n",
			"[Advanced options]\n",
			"iRODS stores data (file contents) into storage 'resources'.\n",
			"Each resource has a name, a host name (this host for this one), and\n",
			"a local directory under which the data is stored.\n",
			"\n" );
		    if ($isUpgrade eq "") {
			$irodsResourceName = promptString(
			  "Resource name",
			  ((!defined($irodsResourceName)||$irodsResourceName eq "") ?
			  $DEFAULT_irodsResourceName : $irodsResourceName) );
			$irodsResourceDir = promptString(
			  "Directory",
			  ((!defined($irodsResourceDir)||$irodsResourceDir eq "") ?
			  $DEFAULT_irodsResourceDir : $irodsResourceDir) );
		    }
		}
	}
	else
	{
		# Fill in with defaults if no prior value exists.

		# If there was no prior irods.config, then all of these
		# values were left at their defaults and this code is
		# redundant.
		#
		# But if there was a prior irods.config file, then we
		# don't know what these are set to and whether they
		# are consistent with the above prompted choices.
		# Since the user didn't ask for Advanced prompts,
		# we need to restore the defaults for missing values.

		$irodsPort = ((!defined($irodsPort)||$irodsPort eq "") ?
			$DEFAULT_irodsPort : $irodsPort);
		$svrPortRangeStart = ((!defined($svrPortRangeStart)||$svrPortRangeStart eq "") ?
			"" : $svrPortRangeStart);
		$svrPortRangeEnd = ((!defined($svrPortRangeEnd)||$svrPortRangeEnd eq "") ?
			"" : $svrPortRangeEnd);
		$irodsZone = ((!defined($irodsZone)||$irodsZone eq "") ?
			$DEFAULT_irodsZone : $irodsZone);
		$irodsDbName = ((!defined($irodsDbName)||$irodsDbName eq "") ?
			$DEFAULT_irodsDbName : $irodsDbName);
		$irodsDbKey = ((!defined($irodsDbKey)||$irodsDbKey eq "") ?
			$DEFAULT_irodsDbKey : $irodsDbKey);
		$irodsResourceName = ((!defined($irodsResourceName)||$irodsResourceName eq "") ?
			$DEFAULT_irodsResourceName : $irodsResourceName);
		$irodsResourceDir = ((!defined($irodsResourceDir)||$irodsResourceDir eq "") ?
			$DEFAULT_irodsResourceDir : $irodsResourceDir);
	}
}


# @brief	Prompt for the iRODS configuration part 2
#
# Like promptForIrodsConfiguration but is done later.
# These are options that apply to client and server but
# are not simple.  
#
# Currently, this is just GSI.
# 
#
sub promptForIrodsConfigurationPart2( )
{
   my $gTypes;
	printNotice(
		"\n",
		"iRODS can make use of the Grid Security Infrastructure (GSI)\n",
		"authentication system in addition to the iRODS secure\n",
		"password system (challenge/response, no plain-text).\n",
		"In most cases, the iRODS password system is sufficient but\n",
		"if you are using GSI for other applications, you might want\n",
		"to include GSI in iRODS.  Both the clients and servers need\n",
		"to be built with GSI and then users can select it by setting\n",
		"irodsAuthScheme=GSI in their .irodsEnv files (or still use\n",
		"the iRODS password system if they want).\n",
		"\n" );
	# GSI ?
	$gsiAuth = promptYesNo(
		"Include GSI",
		(($gsiAuth == 1) ? "yes" : "no") );
	if ( $gsiAuth == 1 ) {
		 printNotice(
				 "\n",
		       "The GLOBUS_LOCATION and the 'install type' is needed to find include\n",
		       "and library files.  GLOBUS_LOCATION specifies the directory where\n",
		       "Globus is installed (see Globus documentation).  The 'install type' is\n",
		       "which 'flavor' of installation you want to use.  For this, use the,\n",
		       "exact name of one of the subdirectories under GLOBUS_LOCATION/include.\n",
		       "\n",
		       "You also need to set up your Globus GSI environment before running\n",
		       "this.\n",
			"\n" );
		if (!defined($globusLocation)||$globusLocation eq "") {
		    $globusLocation = $ENV{"GLOBUS_LOCATION"};
		}
		$globusLocation = promptString(
			"GLOBUS_LOCATION",
			((!defined($globusLocation)||$globusLocation eq "") ?
				"" : $globusLocation) );
		if (!-e $globusLocation) {
		    printError("Warning, $globusLocation does not exist, build will fail.\n");
		    $gTypes = "";
		}
		else {
		    my $gpathInc = "$globusLocation" . "/include";
		    $gTypes = `ls -C $gpathInc`;
		}
		printNotice("\nAvailable types appear to be: $gTypes\n");
		$gsiInstallType = promptString(
			"GSI Install Type to use",
			((!defined($gsiInstallType)||$gsiInstallType eq "") ?
				"" : $gsiInstallType) );

		if (!-e $globusLocation . "/include/" . $gsiInstallType) {
		    printError("Warning, $globusLocation/include/$gsiInstallType does not exist, build will fail.\n");
		}
	}

	# NCCS Audit ?
	printNotice(
		"\n",
		"NCCS Auditing extensions (SQL based) can be installed if\n",
		"desired.  See the README.txt file in server/icat/auditExtensions\n",
		"for more information on this.\n",
		"\n" );
	$auditExt = promptYesNo(
		"Include the NCCS Auditing extensions",
		(($auditExt == 1) ? "yes" : "no") );

}


# @brief	Prompt for the database configuration
#
# Through a series of prompts, collect information for a database
# configuration.  Results are returned through global variables.
#
sub promptForDatabaseConfiguration()
{
	# Install Postgres?
	# Database type?
	# Database host name?
	# Database account name?
	# Database account password?
	# Path to database files

	# Database features
	if ( $installCatalogServer == 0 )
	{
		# No catalog server, so no database needed.
		# No further prompts.
		return;
	}


	# Intro
	if ( $advanced )
	{
		printSubtitle(
			"Database configuration (advanced)\n",
			"---------------------------------\n" );
	}
	else
	{
		printSubtitle(
			"Database configuration\n",
			"----------------------\n" );
	}

	if ( $isUpgrade eq "" ) {
	    printNotice(
		"The iCAT catalog uses a DBMS (database management system) to store\n",
		"state information in a database.  You have two choices:\n",
		"\n",
		"    1.  Download and build a new Postgres DBMS system.\n",
		"    2.  Use an existing Postgres, MySQL, or Oracle DBMS and database.\n",
		"\n" );
	}

	# The loop below walks through three prompts for the
	# above two cases.  Normally, we ask about 1, then
	# 2.
	#
	# If $databaseServerType is already set to "oracle", it
	# can only be because that was chosen in a prior existing
	# irods.config file.  It's a good bet that the user wants
	# to keep this choice, so do the prompts here in a different
	# order for this case to give a fast path for Oracle.
	my $showPrompt = ($databaseServerType =~ /oracle|mysql/i) ? 3 : 1;
	if ( $isUpgrade ne "" ) {
	    $showPrompt = 3;
	}
	my $oldDatabaseServerType = $databaseServerType;
	my $dontPrompt3Again = 0;
	while ( 1 )
	{
		if ( $showPrompt == 1 )
		{
			# New Postgres?
			$installDatabaseServer = promptYesNo(
				"Download and build a new Postgres DBMS",
				(($installDatabaseServer == 0) ? "no" : "yes") );
			if ( $installDatabaseServer == 1 )
			{
				# Definitive answer.  It's a new
				# Postgres install.  Prompt for it,
				# then we're done.
				$databaseServerType = "postgres";
				$databaseServerExclusive = 1;
				promptForNewPostgresConfiguration( 0 );	# not an upgrade
				return;
			}

			# Not that choice.  Try the next one.
			$showPrompt = 2;
		}

# No longer allow upgrade, this script tries to do too much already.
		if ( $showPrompt == 2 )
		{
#			# Upgrade existing Postgres?
#			my $upgrade = promptYesNo( 
#				"Upgrade an existing Postgres DBMS",
#				(($databaseServerType ne "" && $databaseServerType ne "postgres") ?
#					"no" : "yes" ) );
#			if ( $upgrade == 1 )
#			{
				# Definitive answer.  It's an upgrade
				# of an existing Postgres install.
				# Prompt for it, then we're done.
				#
				# A Postgres upgrade is actually the
				# same as installing a new Postgres.
				# In either case, the installation
				# prompts before overwriting prior
				# data.  We present this as a
				# separate "upgrade" choice for clarity.
#				$installDatabaseServer = 1;
#				$databaseServerType = "postgres";
#				$databaseServerExclusive = 1;
#				promptForNewPostgresConfiguration( 1 );	# upgrade
#				return;
#			}

			# Not that choice.  Try the next one.
			$showPrompt = 3;
		}

		if ( $showPrompt == 3 )
		{
			# What existing database should be used?
			if ( $dontPrompt3Again == 0 )
			{
				if ( $databaseServerType !~ /oracle|mysql/i )
				{
					# Explain if a prior install
					# hasn't already indicated
					# that Oracle is the choice.
					printNotice(
						"\n",
						"To use an existing database, the iCAT needs to know what type\n",
						"of database to use, where the database is running, and how to\n",
						"access it.  If it is a Postgres database, it must be running\n",
						"on this host.\n",
						"\n" );
				}
				$databaseServerType = promptString(
					"Database type (postgres or mysql or oracle)",
					(($databaseServerType ne "") ?
					$databaseServerType : $DEFAULT_databaseServerType) );
			}
			if ( $databaseServerType =~ /^p(ostgres)?/i )
			{
				# Definitive answer.  It's
				# a use of an existing
				# Postgres install.  Prompt
				# for it, then we're done.
				$databaseServerType = "postgres";
				if ( $oldDatabaseServerType !~ /$databaseServerType/i )
				{
					# The database choice changed,
					# probably from Oracle to
					# Postgres.
					#
					# The prior database parameters
					# are almost certainly wrong.
					# Undefine them or set them to
					# defaults (which are for Postgres).
					$databaseServerPath = undef;
					$databaseServerHost = undef;
					$databaseServerPort = $DEFAULT_databaseServerPort;
					$databaseServerAccount = $DEFAULT_databaseServerAccount;
					$databaseServerPassword = undef;
					$databaseServerOdbcType	= $DEFAULT_databaseServerOdbcType;

					# Because the prior DB was
					# probably oracle, the order of
					# these prompts was adjusted to
					# make the DB type prompt first.
					# Now, with an answer of 'postgres',
					# we don't know if the user wants
					# a new install or to use an
					# existing install.  So, start
					# the prompts over.
					$oldDatabaseServerType = "postgres";
					$installDatabaseServer = 1;
					$databaseServerExclusive = 1;
					$showPrompt = 1;

					# And next time we get here, don't
					# prompt for the DB type again.
					# This only happens if the original
					# config file said "Oracle".  The
					# first prompt asked if they wanted
					# to stick with Oracle, and the user
					# said "Postgres".  And then we
					# back to prompt 1, where they say
					# no, then 2, where they say no.
					# Which brings us back here.  And
					# now the user has already answered
					# this prompt, so don't ask again.
					#
					# This is a very rare case.
					$dontPrompt3Again = 1;
					next;
				}
				if ( $dontPrompt3Again == 1 )
				{
					printNotice(
						"\n",
						"Using an existing Postgres database.\n",
						"\n" );
				}
				$databaseServerExclusive = 0;
				promptForExistingPostgresDatabase( );
				return;
			}
			elsif ( $databaseServerType =~ /^o(racle)?/i )
			{
				# Definitive answer.  It's
				# a use of an existing
				# Oracle install.  Prompt
				# for it, then we're done.
				$databaseServerType = "oracle";
				if ( $oldDatabaseServerType !~ /$databaseServerType/i )
				{
					# The database choice changed,
					# probably from Postgres to
					# Oracle.
					#
					# The prior database parameters
					# are almost certainly wrong.
					$databaseServerPath = undef;
					$databaseServerHost = undef;
					$databaseServerPort = undef;
					$databaseServerAccount = undef;
					$databaseServerPassword = undef;
				}
				$databaseServerHost = $thisHost;
				$databaseServerExclusive = 0;
				promptForExistingOracleDatabase( );
				return;
			}
 			elsif ( $databaseServerType =~ /^m(ysql)?/i )
 			{
				# Definitive answer.  It's
				# a use of an existing
				# MySQL install.  Prompt
				# for it, then we're done.
				$databaseServerType = "mysql";
				if ( $oldDatabaseServerType !~ /$databaseServerType/i )
				{
					# The database choice changed,
					# probably from Postgres to
					# Oracle.
					#
					# The prior database parameters
					# are almost certainly wrong.
					$databaseServerPath = undef;
					$databaseServerHost = undef;
					$databaseServerPort = undef;
					$databaseServerAccount = undef;
					$databaseServerPassword = undef;
				}
				$databaseServerExclusive = 0;
 				promptForExistingMysqlDatabase( );
 				return;
 			}
			printError(
				"    iRODS currently only works with Postgres, MySQL, or Oracle\n",
				"    database management systems.  Please select one of these.\n\n" );

			# Not that choice.  Try the first one
			# again.
			$showPrompt = 1;
		}
	}
}





# @brief	Prompt for a new Postgres database configuration
#
# Through a series of prompts, collect information for a new
# installation of Postgres.  Results are returned through global variables.
#
# @param	$upgrade	with a 1 value, present prompts as if this
# 				is an upgrade, otherwise present prompts
# 				as if this is a new install
#
sub promptForNewPostgresConfiguration( $ )
{
	my ($upgrade) = @_;

	$databaseServerHost = 'localhost';	# Always

	# Intro
	if ( $upgrade == 0 )
	{
		printNotice(
			"\n",
			"A new copy of Postgres will be automatically downloaded, built,\n",
			"and installed into a new directory of your choice.\n",
			"\n" );
	}
	else
	{
		printNotice(
			"\n",
			"To upgrade Postgres, a new copy of Postgres will be automatically\n",
			"downloaded, built, and installed into an existing Postgres\n",
			"directory.\n",
			"\n" );
	}



	# Get the installation directory.

	# For a new install, the user should select a new directory.
	# But if they select one that already has Postgres, ask if
	# they want to upgrade.

	# For an upgrade, the user should select a directory that
	# already has Postgres.  But if they don't, the confirm a
	# switch to a new install.

	# Keep asking until its clear what to do.
	
	$priorDatabaseExists = 0;
	while ( 1 )
	{
		# Prompt for the directory
		if ( $upgrade == 0 )
		{
			$tmpPath = $databaseServerPath;
			$tmpPath =~ s\/pgsql\\g; # remove /pgsql if it is there,
						 # as it seems to get added later but
						 # what we need as default here is without.
			$databaseServerPath = promptString(
				"New Postgres directory",
				$tmpPath );
		}
		else
		{
			$databaseServerPath = promptString(
				"Existing Postgres directory",
				$databaseServerPath );
		}

		# Check if there is a Postgres install there
		my $dataDir = File::Spec->catdir( $databaseServerPath,
			"pgsql", "data" );
		if ( -e $dataDir )
		{
			# The directory exists.
			if ( $upgrade == 0 )
			{
				# But the user said they wanted a
				# new install.  Confirm.
				printNotice(
					"\n",
					"Postgres is already installed in that directory.\n");
# old version				"Postgres is already installed in that directory.  You can\n",
#					"upgrade it to a new version, or select a different directory.\n",

#				my $switchUpgrade = promptYesNo(
#					"Upgrade an existing Postgres DBMS",
#					"yes" );
				my $switchUpgrade = 0;
				if ( $switchUpgrade == 0 )
				{
					# They don't want to upgrade.
					# Then ask them for another directory.
					printNotice(
						"\n",
						"Please select another directory for a new installation\n",
						"of Postgres or control-C and restart this script.\n",
						"\n" );
					next;
				}
				# Otherwise they're fine with an upgrade.
				$upgrade = 1;
			}

			# The directory exists and they want an upgrade.
			$priorDatabaseExists = 1;

			# Ask if they'd like to keep the data
			printNotice(
				"\n",
				"You can reuse the existing Postgres databases or delete it first.\n",
				"Deleting the data will remove all prior data, content and accounts.\n",
				"This cannot be undone.\n",
				"\n" );
			$deleteDatabaseData = promptYesNo(
				"Permanently delete prior IRODS data (Postgres data, ICAT)",
				"no" );
			last;
		}

		# The directory doesn't exist.
		$priorDatabaseExists = 0;
		if ( $upgrade == 1 )
		{
			# But they asked for an upgrade.  See if they'd
			# like to do a new install insteasd.
			printNotice(
				"\n",
				"The directory does not contain an installation of Postgres\n",
				"to upgrade.\n",
				"\n" );
			my $switchUpgrade = promptYesNo(
				"Install a new copy of Postgres there",
				"yes" );
			if ( $switchUpgrade == 0 )
			{
				# They don't want to install.
				# Then ask them for another directory.
				printNotice(
					"\n",
					"Please select another directory.\n",
					"\n" );
				next;
			}
			# Otherwise they're fine with a new install.
			$upgrade = 0;
		}
		last;
	}


	if ( $priorDatabaseExists == 1 && $deleteDatabaseData == 0 )
	{
		# Since there is a prior database and we aren't deleting it,
		# the password the user gives must match that in the prior
		# database
		printNotice(
			"\n",
			"This install will use the prior Postgres installation's data\n",
			"and accounts.\n",
			"\n" );
		$databaseServerAccount = promptIdentifier(
			"Existing database login name",
			((!defined($databaseServerAccount)||$databaseServerAccount eq "") ?
				$DEFAULT_databaseServerAccount : $databaseServerAccount) );
		$databaseServerPassword = promptIdentifier(
			"Password",
			$databaseServerPassword );
	}
	else
	{
		# Either there is no prior database, or we are deleting it.
		# In this case, the password is new.
		printNotice(
			"\n",
			"Installation of a new database will create an adminstrator account.\n",
			"\n" );
		$databaseServerAccount = promptIdentifier(
			"New database login name",
			((!defined($databaseServerAccount)||$databaseServerAccount eq "") ?
				$DEFAULT_databaseServerAccount : $databaseServerAccount) );
		$databaseServerPassword = promptIdentifier(
			"Password",
			$databaseServerPassword );
	}


	# Previous version did not, but we now prompt for the pg and odbc versions
	# to install.

        # Get default postgres settings
	if (-e $installPostgresConfig) {
	    require $installPostgresConfig
	}
	else {
	    require $installPostgresConfigTemplate
	}

        # Prompt for pg version
	$postgresSource = $POSTGRES_SOURCE;

	printNotice(
		    "\n",
		    "Multiple versions of PostgreSQL are available from their web site.\n",
		    "We recommend the default, or newer, version.",
		    "\n" );
	$postgresSource = promptString(
			"PostgreSQL version",
			$postgresSource );

        # Prompt for ODBC version
	$odbcSource = $ODBC_SOURCE;

	printNotice(
		    "\n",
		    "Different versions of ODBC are also available.\n",
		    "We recommend the default, but others may work well too.",
		    "\n" );
	$odbcSource = promptString(
			"ODBC version",
			$odbcSource );


	if ( $advanced )
	{
		# Database port
		printNotice(
			"\n",
			"[Advanced option]\n",
			"For sites with multiple databases, Postgres can be configured\n",
			"to use a custom port.\n",
			"\n" );
		$databaseServerPort = promptInteger(
			"Port",
			((!defined($databaseServerPort)||$databaseServerPort eq "") ?
				$DEFAULT_databaseServerPort : $databaseServerPort) );
	}
	else
	{
		# Fill in with defaults if no prior value exists.

		# If there was no prior irods.config, then the
		# database port is already set to the default.
		#
		# But if there was a prior irods.config file, and
		# it was built with a different set of choices,
		# and the user didn't ask for Advanced prompts,
		# it is possible the database port isn't set.
		# So, set it to the default.
		$databaseServerPort = ((!defined($databaseServerPort)||$databaseServerPort eq "") ?
			$DEFAULT_databaseServerPort : $databaseServerPort);
	}

	if ( $upgrade )
	{
		# Ask if we should control that database
		printNotice(
			"\n",
			"iRODS can be configured to start and stop the Postgres database\n",
			"along with the iRODS servers.  Enable this only if your database\n",
			"is not being used for anything else besides iRODS.\n",
			"\n" );
		$databaseServerExclusive = promptYesNo(
			"Start and stop the database along with iRODS",
			(($databaseServerExclusive==1)?"yes":"no") );
	}
	else
	{
		$databaseServerExclusive = 1;
	}
}





# @brief	Prompt for the configuration of an existing Postgres database
#
# Through a series of prompts, collect information for an existing Postgres
# installation.  Results are returned through global variables.
#
sub promptForExistingPostgresDatabase( )
{
	$databaseServerHost = 'localhost';	# Always
	if ( $databaseServerOdbcType eq "" )
	{
		$databaseServerOdbcType = $DEFAULT_databaseServerOdbcType;
	}

	# Intro
	printNotice(
		"\n",
		"The iRODS build needs access to the include and library files\n",
		"in the existing Postgres installation.\n",
		"\n" );

	# Where is Postgres?
	$databaseServerPath = promptString(
		"Existing Postgres directory",
		$databaseServerPath );


	# What type of ODBC is it using?
	while ( 1 )
	{
		printNotice(
			"\n",
			"To access Postgres, iRODS needs to know which type of ODBC is\n",
			"available.  iRODS prior to 1.0 used the 'postgres' ODBC.\n",
			"For iRODS 1.0 and beyond, the 'unix' ODBC is preferred.\n",
			"\n" );

		$databaseServerOdbcType = promptString(
			"ODBC type (unix or postgres)",
			$databaseServerOdbcType );
		if ( $databaseServerOdbcType =~ /^u(nix)?/i )
		{
			$databaseServerOdbcType = "unix";
			last;
		}
		if ( $databaseServerOdbcType =~ /^p(ostgres)?/i )
		{
			$databaseServerOdbcType = "postgres";
			last;
		}
		printError(
			"    Sorry, but iRODS only works with UNIX or Postgres ODBC\n",
			"    drivers.  Please select one of these two.\n" );
	}


	# Account name and password
	printNotice(
		"\n",
		"iRODS will need to use an existing database account.\n",
		"\n" );
	$databaseServerAccount = promptIdentifier(
		 "Existing database login name",
		((!defined($databaseServerAccount)||$databaseServerAccount eq "") ?
			$DEFAULT_databaseServerAccount : $databaseServerAccount) );
	$databaseServerPassword = promptIdentifier(
		"Password",
		$databaseServerPassword );


	if ( $advanced )
	{
		# Port.
		printNotice(
			"\n",
			"[Advanced option]\n",
			"For sites with multiple databases, iRODS can use a Postgres\n",
			"database configured to use a custom port.\n",
			"\n" );
		$databaseServerPort = promptInteger(
			"Port",
			$databaseServerPort );
	}
	else
	{
		# Fill in with defaults if no prior value exists.

		# If there was no prior irods.config, then the
		# database port is already set to the default.
		#
		# But if there was a prior irods.config file, and
		# it was built with a different set of choices,
		# and the user didn't ask for Advanced prompts,
		# it is possible the database port isn't set.
		# So, set it to the default.
		$databaseServerPort = ((!defined($databaseServerPort)||$databaseServerPort eq "") ?
			$DEFAULT_databaseServerPort : $databaseServerPort);
	}

	# Ask if we should control that database
	printNotice(
		"\n",
		"iRODS can be configured to start and stop the Postgres database\n",
		"along with the iRODS servers via the 'irodsctl' control script.\n",
		"Most likely, you'll want to enable this if you are using your\n",
		"Postgres primarily for iRODS.\n",
		"\n" );
	$databaseServerExclusive = promptYesNo(
		"Start and stop the database along with iRODS",
		"yes" );
}



# @brief	Prompt for the configuration of an existing Oracle database
#
# Through a series of prompts, collect information for an existing Oracle
# installation.  Results are returned through global variables.
#
sub promptForExistingMysqlDatabase( )
{
	$databaseServerOdbcType  = "unix";	# Already set.  No prompt.
	$deleteDatabaseData      = 0;		# Never.  No prompt.
	$databaseServerExclusive = 0;		# Never.  No prompt.

	# Intro
	printNotice(
		"\n",
		"iRODS uses ODBC for access to MySQL databases.\n",
		"The iRODS build needs access to the include and\n",
        "library files in the existing unixODBC installation.\n",
		"\n" );

	# Where is ODBC?
	$databaseServerPath = promptString( "Existing unixODBC directory", $databaseServerPath );

	# Server host can be "localhost" or any other host name
	printNotice(
		"\n",
		"ODBC should have been configured by administrator already\n",
		"to provide access to database via the 'Data Source'.\n",
		"\n" );
	$irodsDbName = promptString( "ODBC Data Source name", $irodsDbName );


	# Account name and password
	printNotice(
		"\n",
		"iRODS will need to use an existing database account.\n",
		"\n" );
	$databaseServerAccount = promptIdentifier(
		 "Existing database login name",
		((!defined($databaseServerAccount)||$databaseServerAccount eq "") ?
			$DEFAULT_databaseServerAccount : $databaseServerAccount) );
	$databaseServerPassword = promptIdentifier(
		"Password",
		$databaseServerPassword );

}





# @brief	Prompt for the configuration of an existing Oracle database
#
# Through a series of prompts, collect information for an existing Oracle
# installation.  Results are returned through global variables.
#
sub promptForExistingOracleDatabase( )
{
	$databaseServerType      = "oracle";	# Already set.  No prompt.
	$databaseServerOdbcType  = undef;	# Unused. No prompt.
	$deleteDatabaseData      = 0;		# Never.  No prompt.
	$databaseServerExclusive = 0;		# Never.  No prompt.
	$databaseServerPort      = undef;	# Unused. No prompt.


	printNotice(
		"\n",
		"The iRODS build needs access to the include and library files\n",
		"in the Oracle installation.\n",
		"\n" );
	# Where is Oracle?  If ORACLE_HOME is set, use it.
	$databaseServerPath = promptString(
		"Existing Oracle directory",
		((!defined($databaseServerPath)||$databaseServerPath eq "") ?
			$ENV{"ORACLE_HOME"} : $databaseServerPath) );

	# Which lib dir?
	$databaseServerLib = promptString(
		"Which library subdirectory under $databaseServerPath",
		((!defined($databaseServerLib)||$databaseServerLib eq "") ?
			"lib" : $databaseServerLib) );


	# Prompt for schema@instance and make sure the input has that form
	printNotice(
		"\n",
		"iRODS will need to use an existing database account.  Oracle\n",
		"account names have the form 'schema\@instance', such as\n",
		"'icat\@irods_rac'.\n",
		"\n" );
	while ( 1 )
	{
		$databaseServerAccount = promptString(
			 "Existing Oracle login name",
			 $databaseServerAccount );
		my @parts = split( '@', $databaseServerAccount );
		if ( $#parts != 1 )	# 2 parts required
		{
			printError(
				"    Sorry, but the account name must include the schema name,\n",
				"    an \@ sign, and the instance name.\n",
				"\n" );
			next;
		}
		if ( $parts[0] =~ /[^a-zA-Z0-9_\-\.]/ )
		{
			printError(
				"    Sorry, but the schema name can only include the characters\n",
				"    a-Z A-Z 0-9, dash, period, and underscore.\n",
				"\n" );
			next;
		}
		if ( $parts[1] =~ /[^a-zA-Z0-9_\-\.]/ )
		{
			printError(
				"    Sorry, but the instance name can only include the characters\n",
				"    a-Z A-Z 0-9, dash, period, and underscore.\n",
				"\n" );
			next;
		}
# Do not set $databaseServerHost below, it needs to be left as $thisHost,
# the icat-enabled irods server host name.
#		$databaseServerHost = $parts[1];
		last;
	}

	# Password
	$databaseServerPassword = promptIdentifier(
		"Password",
		$databaseServerPassword );
}








# @brief	Prompt for confirmation of the prompted for values
#
# The collected information is shown to the user and they are prompted
# to confirm that they look right.  0 is returned on no, 1 on yes.
#
sub promptForConfirmation( )
{
	printSubtitle(
		"Confirmation\n",
		"------------\n" );


	printNotice(
		"Please confirm your choices.\n\n",
		"    --------------------------------------------------------\n" );

	# iRODS server + iCAT
	if ( $installDataServer )
	{
		if ( !$installCatalogServer )
		{
			printNotice(
				"    Build iRODS data server without iCAT\n",
				"        iCAT host     '$catalogServerHost'\n" );
		}
		else
		{
			printNotice(
				"    Build iRODS data server + iCAT metadata catalog\n" );
		}
		if ( $advanced )
		{
			printNotice(
				"        directory     '$IRODS_HOME'\n",
				"        port          '$irodsPort'\n",
				"        start svrPort '$svrPortRangeStart'\n",
				"        end svrPort   '$svrPortRangeEnd'\n",
				"        account       '$irodsAccount'\n",
				"        password      '$irodsPassword'\n",
				"        zone          '$irodsZone'\n",
				"        db name       '$irodsDbName'\n",
				"        scramble key  '$irodsDbKey'\n",
				"        resource name '$irodsResourceName'\n",
				"        resource dir  '$irodsResourceDir'\n",
				"\n" );
		}
		else
		{
			printNotice(
				"        directory     '$IRODS_HOME'\n",
				"        account       '$irodsAccount'\n",
				"        password      '$irodsPassword'\n",
				"\n" );
		}
	}

	# Database
	if ( $installDatabaseServer || $databaseServerType ne "" )
	{
		if ( $installDatabaseServer )
		{
			my $msg = "";
			if ( $deleteDatabaseData )
			{
				$msg = "(delete prior data)";
			}
			elsif ( $priorDatabaseExists )
			{
				$msg = "(keep prior data)";
			}
			printNotice(
				"    Build Postgres $msg\n" );
		}
		elsif ( $databaseServerType eq "postgres" )
		{
			printNotice(
				"    Use existing Postgres\n" );
		}
		elsif ( $databaseServerType eq "oracle" )
		{
			printNotice(
				"    Use existing Oracle\n" );
		}
		elsif ( $databaseServerType eq "mysql" )
		{
			printNotice(
				"    Use existing MySQL/ODBC Data Source\n" );
		}

		printNotice(
			"        host          '$databaseServerHost'\n",
			($advanced ?
			"        port          '$databaseServerPort'\n" :
			""),
			"        directory     '$databaseServerPath'\n");
		if (defined( $databaseServerLib ) && $databaseServerLib ne "" ) {
		    printNotice(
			"        lib subdir    '$databaseServerLib'\n");
		}
		printNotice(
			"        account       '$databaseServerAccount'\n",
			"        password      '$databaseServerPassword'\n",
			"        pg version    '$postgresSource'\n",
			"        odbc version  '$odbcSource'\n" );
		my $msg = "";
		if ( !$databaseServerExclusive )
		{
			$msg = "do not";
		}
		printNotice(
			"        control       $msg start & stop along with iRODS servers\n",
			"\n" );
	}

   # GSI
	if ($gsiAuth == 0) {
		 printNotice(
			"    GSI not selected\n\n");
   } else {
		 printNotice (
			"    GSI enabled\n",
			"        GLOBUS_LOCATION  $globusLocation\n",
			"        gsiInstallType   $gsiInstallType\n\n");
	}

   # Audit Extension
	if ($auditExt == 0) {
		 printNotice(
			"    NCCS Audit Extensions not selected\n\n");
        } else {
		 printNotice (
			"    NCCS Audit Extensions will be enabled\n\n");
	}

	# Commands
	printNotice(
		"    Build iRODS command-line tools\n",
		"    --------------------------------------------------------\n" );


	printNotice(
		"\n" );


	return promptYesNo(
		"Save configuration (irods.config)",
		"yes" );
}





# @brief	Set the iRODS configuration file
#
# Use the current set of global variable values, update the irods.config
# file with the user's choices.
#
sub configureIrods( )
{
	# Copy the template if irods.config doesn't exist yet.
	my $status = copyTemplateIfNeeded( $irodsConfig );
	if ( $status == 0 )
	{
		printError( "\nConfiguration problem:\n" );
		printError( "    Cannot copy the iRODS configuration template file:\n" );
		printError( "        File:  $irodsConfig.template\n" );
		printError( "    Permissions problem?  Missing file?\n" );
		printError( "\n" );
		printError( "Abort.\n" );
		exit( 1 );
	}

	# Make sure the file is only readable by the user.
	chmod( 0600, $irodsConfig );

	# Use an existing database.  Set the configuration file
	# to point to the database.
	my %configure = (
		"IRODS_HOME",			$IRODS_HOME,
		"IRODS_PORT",			$irodsPort,
		"SVR_PORT_RANGE_START",		$svrPortRangeStart,
		"SVR_PORT_RANGE_END",		$svrPortRangeEnd,
		"IRODS_ADMIN_NAME",		$irodsAccount,
		"IRODS_ADMIN_PASSWORD",		$irodsPassword,
		"IRODS_ICAT_HOST",		$catalogServerHost,
		"DB_NAME",			$irodsDbName,
		"DB_KEY",			$irodsDbKey,
		"RESOURCE_NAME",		$irodsResourceName,
		"RESOURCE_DIR",			$irodsResourceDir,
		"ZONE_NAME",			$irodsZone,

		"GSI_AUTH",			$gsiAuth,
		"GLOBUS_LOCATION",		$globusLocation,
		"GSI_INSTALL_TYPE",		$gsiInstallType,

		"AUDIT_EXT",			$auditExt,

		"DATABASE_TYPE",		$databaseServerType,
		"DATABASE_ODBC_TYPE",		((!defined($databaseServerOdbcType)) ? "" : $databaseServerOdbcType),
		"DATABASE_EXCLUSIVE_TO_IRODS",	"$databaseServerExclusive",
		"DATABASE_HOME",		$databaseServerPath,
		"DATABASE_LIB", 		$databaseServerLib,
		"DATABASE_HOST",		$databaseServerHost,
		"DATABASE_PORT",		$databaseServerPort,
		"DATABASE_ADMIN_NAME",		$databaseServerAccount,
		"DATABASE_ADMIN_PASSWORD",	$databaseServerPassword );
	replaceVariablesInFile( $irodsConfig, "perl", 0, %configure );
}





# @brief	Set the Postgres configuration file
#
# Use the current set of global variable values, update the
# installPostgres.config file with the user's choices.
#
sub configureNewPostgresDatabase( )
{
	# If there is no new database to configure, make sure the
	# installPostgres.config file is gone.
	if ( $installDatabaseServer == 0 )
	{
		if ( -e $installPostgresConfig )
		{
			unlink( $installPostgresConfig );
		}
		return;
	}


	# Copy the template if installPostgres.config doesn't exist yet.
	my $status = copyTemplateIfNeeded( $installPostgresConfig );
	if ( $status == 0 )
	{
		printError( "\nConfiguration problem:\n" );
		printError( "    Cannot copy the Postgres configuration template file:\n" );
		printError( "        File:  $installPostgresConfig.template\n" );
		printError( "    Permissions problem?  Missing file?\n" );
		printError( "\n" );
		printError( "Abort.\n" );
		exit( 1 );
	}

	# Make sure the file is only readable by the user.
	chmod( 0600, $installPostgresConfig );

	# Change/add settings to the file to reflect the user's choices.
	my %configure = (
		"POSTGRES_SRC_DIR",		$databaseServerPath,
		"POSTGRES_HOME",		File::Spec->catdir( $databaseServerPath, "pgsql" ),
		"POSTGRES_PORT",		$databaseServerPort,
		"POSTGRES_ADMIN_NAME",		$databaseServerAccount,
		"POSTGRES_ADMIN_PASSWORD",	$databaseServerPassword );
	if ( $deleteDatabaseData ne "" )
	{
		$configure{"POSTGRES_FORCE_INIT"} = $deleteDatabaseData;
	}
	if ( $postgresSource ne "" )
	{
		$configure{"POSTGRES_SOURCE"} = $postgresSource;
	}
	if ( $postgresSource ne "" )
	{
		$configure{"ODBC_SOURCE"} = $odbcSource;
	}
	replaceVariablesInFile( $installPostgresConfig,
		"perl", 1, %configure );
}





# @brief	Print command-line help
#
sub printUsage
{
	my $oldVerbosity = isPrintVerbose( );
	setPrintVerbose( 1 );

	printNotice( "This script prompts for configuration choices when\n" );
	printNotice( "installing iRODS.\n" );
	printNotice( "\n" );
	printNotice( "Usage is:\n" );
	printNotice( "    $scriptName [options]\n" );
	printNotice( "\n" );
	printNotice( "Help options:\n" );
	printNotice( "    --help        Show this help information\n" );

	setPrintVerbose( $oldVerbosity );
}
