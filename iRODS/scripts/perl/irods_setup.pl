#
# Perl

#
# Create the database tables and finish the iRODS installation
#
# Usage is:
#       perl irods_setup.pl [options]
#
use File::Spec;
use File::Copy;
use Cwd;
use Cwd "abs_path";
use Config;
use File::Basename;
use File::Path;

$version{"irods_setup.pl"} = "Feb 2015";

# =-=-=-=-=-=-=-
# detect the running environment (usually the service account user)
$scriptfullpath = abs_path(__FILE__);
$scripttoplevel = dirname(dirname(dirname(dirname($scriptfullpath))));

# =-=-=-=-=-=-=-
# for testing later...
# set flag determining if this is an automated cloud resource server
$cloudResourceInstall = 0;
if( scalar(@ARGV) == 1) {
        $cloudResourceInstall = 1;
}
# set flag to determine if this is an iCAT installation or not
$icatInstall = 0;
if( scalar(@ARGV) > 1 ) {
        $icatInstall = 1;
}

########################################################################
#
# Confirm execution from the top-level iRODS directory.
#
$IRODS_HOME = cwd( );   # Might not be actual iRODS home.  Fixed below.

my $perlScriptsDir = File::Spec->catdir( $IRODS_HOME, "scripts", "perl" );

# iRODS configuration directory
$configDir = `perl $perlScriptsDir/irods_get_config_dir.pl`;

if ( ! -d $configDir )
{
        # Configuration directory does not exist.  Perhaps this
        # script was run from the scripts or scripts/perl subdirectories.
        # Look up one directory.
        $IRODS_HOME = File::Spec->catdir( $IRODS_HOME, File::Spec->updir( ));
        $configDir  = File::Spec->catdir( $IRODS_HOME, "config" );
        if ( ! -d $configDir )
        {
                $IRODS_HOME = File::Spec->catdir( $IRODS_HOME, File::Spec->updir( ));
                $configDir  = File::Spec->catdir( $IRODS_HOME, "config" );
                if ( ! -d $configDir )
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

push @INC, "/etc/irods";
push @INC, $configDir;
require File::Spec->catfile( $perlScriptsDir, "utils_paths.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_print.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_file.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_platform.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_config.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_prompt.pl" );
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
my $logFile = File::Spec->catfile( $logDir, "irods_setup.log" );

# iRODS Server names (don't change these names without also changing
# code later in this file that depends upon them).
%servers = (
        # Nice name             process name
        "iRODS agents" =>       "(irodsAge)(nt)?",
        "iRODS rule servers" => "(irodsReS)(erver)?",
        "iRODS servers" =>      "(irodsSer)(ver)?"

        # Process names need to be able to match on just 8 characters,
        # due to limitations of SysV /bin/ps (such as Solaris) which
        # only shows the first 8 characters.
);





########################################################################
#
# Check script usage.
#

# Initialize global flags.
my $noAsk = 0;
my $noHeader = 0;
my $isUpgrade = 0;
setPrintVerbose( 1 );

########################################################################
#
# Confirm that prior setup stages have probably run.
#

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
# Load and validate server configuration files.
#

# binary installation
if ( -e "$scripttoplevel/packaging/binary_installation.flag" )
{
    load_server_config("/etc/irods/server_config.json");
    if ( 1 == $icatInstall )
    {
        load_database_config("/etc/irods/database_config.json");
    }
}
# run-in-place
else
{
    load_server_config("$scripttoplevel/iRODS/server/config/server_config.json");
    if ( 1 == $icatInstall )
    {
        load_database_config("$scripttoplevel/iRODS/server/config/database_config.json");
    }
}


# Make sure the home directory is set and valid.  If not, the installation
# is probably being run out of order or a prior stage failed.
if ( $IRODS_HOME eq "" || ! -e $IRODS_HOME )
{
        printError( "Usage error:\n" );
        printError( "    The IRODS_HOME setting is empty or incorrect.\n" );
        exit( 1 );
}

########################################################################
#
# Open and initialize the setup log file.
#
if ( -e $logFile )
{
        unlink( $logFile );
}
openLog( $logFile );

# Because the log file contains echoed account names and passwords, we
# change permissions on it so that only the current user can read it.
chmod( 0600, $logFile );

printLog( "iRODS setup\n" );
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

# =-=-=-=-=-=-=-
# JMC :: if arguments are 0, we assume this is a RESOURCE installation.
if( 1 == $icatInstall )
{
        $DATABASE_TYPE           = $ARGV[0];
        $DATABASE_HOST           = $ARGV[1];
        $DATABASE_PORT           = $ARGV[2];
        $DATABASE_ADMIN_NAME     = $ARGV[3];
        $DATABASE_ADMIN_PASSWORD = $ARGV[4];
}
# =-=-=-=-=-=-=-
# TGR :: for a resource server, prompt for icat admin password
else
{
    if ( 1 == $cloudResourceInstall )
    {
        # get the password from argv
        print "\n";
        print "Reading [$IRODS_ADMIN_NAME] password from input...\n";
        print "\n";
        $IRODS_ADMIN_PASSWORD = $ARGV[0];
        chomp $IRODS_ADMIN_PASSWORD;
    }
    else {
        # call out to external shell script so live password can be hidden
        # and never shown to the user and never written to disk or visible to
        # the unix process listing (ps)
        print "\n";
        print "The following password will not be written to disk\n";
        print "or made visible to any process other than this setup script.\n";
        print "\n";
        print "  iCAT server's admin username: $IRODS_ADMIN_NAME\n";
        print "  iCAT server's admin password: ";
        if( $RUNINPLACE == 1 ) {
            $IRODS_ADMIN_PASSWORD = `$scripttoplevel/packaging/get_icat_server_password.sh`;
        } else {
          $IRODS_ADMIN_PASSWORD = `/var/lib/irods/packaging/get_icat_server_password.sh`;
        }
        print "\n";
        print "\n";
    }
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
        $totalSteps  = 4;
        if ($isUpgrade) {$totalSteps=3;}
        $currentStep = 0;

        # Ignore the iCAT host name, if any, if we
        # are using a database.
        $IRODS_ICAT_HOST = "";

        # 1.  Set up the user account for the database.
        if (!$isUpgrade) {
            configureDatabaseUser( );
        }

        # 2.  Create the database schema and tables.
        if (!$isUpgrade) {
            createDatabaseAndTables( );
            testDatabase( );
        }

        # 3.  Configure
        configureIrodsServer( );

        # 4.  Configure the iRODS user account.
        configureIrodsUser( );
}

# Done.
printLog( "\nDone.\n" );
printLog( getCurrentDateTime( ) . "\n" );
closeLog( );
if ( ! $noHeader )
{
        printSubtitle( "\nDone.  Additional detailed information is in the log file:\n" );
        printStatus( "$logFile\n" );
}
exit( 0 );










########################################################################
#
# Setup steps.
#
#       All of these functions execute individual steps in the
#       database setup.  They all output error messages themselves
#       and may exit the script directly.
#

#
# @brief        Prepare to initialize.
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

}






#
# @brief        Configure database user.
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
# @brief        Create database schema and tables.
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
        #       This step is skipped if the database already exists, and
        #       we're not in FORCE mode.
        #
        #       On a fresh database, the user's account doesn't have a
        #       password yet and database creation doesn't prompt for one.
        #       But if the database already exists, or if this script ran
        #       before and failed part way through, then the user's
        #       account may now have a password and they'll be prompted.
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

        # Create the tables.
        #       The iCAT SQL files issue a number of instructions to
        #       create tables and initialize state.  If this script
        #       has been run before, then these tables will already be
        #       there.  Running the SQL instructions a second time
        #       will corrupt the database.  So, we need to check first.
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
                # iRODS table names mostly start with R_
                if ( $line =~ /r_/i )
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
                        "icatSysTables.sql",
                        "icatSysInserts.sql",
                        "icatSetupValues.sql");

                my $alreadyCreated = 0;
                my $sqlfile;
                printStatus( "    Inserting iCAT tables...\n" );
                printLog( "    Inserting iCAT tables...\n" );
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

                # Now apply the site-defined iCAT tables, if any
                my $sqlPath = File::Spec->catfile( $extendedIcatDir, "icatExtTables.sql" );
                if (-e $sqlPath) {
                    printStatus( "    Inserting iCAT Extension tables...\n" );
                    printLog( "    Inserting iCAT Extension tables...\n" );
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
                    printStatus( "    Inserting iCAT Extension table rows...\n" );
                    printLog( "    Inserting iCAT Extension table rows...\n" );
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

        }
        if ( $tablesAlreadyCreated )
        {
                printStatus( "    Skipped.  Tables already created.\n" );
                printLog( "    Skipped.  Tables already created.\n" );
        }

}



#
# @brief        Test database communications.
#
# After setting up the database, make sure we can talk to it.
#
# Output error messages and exit on problems.
#
sub testDatabase()
{
        # Make sure communications are working.
        #       This simple test issues a few SQL statements
        #       to the database, testing that the connection
        #       works.  iRODS is uninvolved at this point.
        printStatus( "Testing database communications...\n" );
        printLog( "\nTesting database communications with test_cll...\n" );

        my $test_cll = File::Spec->catfile( $serverTestCLLBinDir, "test_cll" );

        $exec_str = "$test_cll $DATABASE_ADMIN_NAME '$DATABASE_ADMIN_PASSWORD'";
        my ($status,$output) = run( "$exec_str" );

        # scrub the password before logging and displaying
        $output =~ s/(.*,pass=)(.*)/$1XXXXX/;
        $output =~ s/(.*password=)(.*)/$1XXXXX/;
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
# @brief        Configure iRODS.
#
# Update the iRODS server_config.json file.
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

        # Update/set the iRODS server configuration.
        #       Tell iRODS the database host (often this host),
        #       the database administrator's name, and their MD5
        #       scrambled database password.
        #
        #       If this script is run more than once, then the file
        #       may already be updated.  It's so small there is
        #       little point in checking for this first.  Just
        #       overwrite it with the correct values.

        my $host = ($IRODS_ICAT_HOST eq "") ? "localhost" : $IRODS_ICAT_HOST;





        # binary installation
        if ( -e "$scripttoplevel/packaging/binary_installation.flag" )
        {
            $serverConfigFile = "/etc/irods/server_config.json";
            if ( -e "/etc/irods/database_config.json" )
            {
                $databaseConfigFile = "/etc/irods/database_config.json";
            }
        }
        # run-in-place
        else
        {
            $serverConfigFile = "$scripttoplevel/iRODS/server/config/server_config.json";
            if ( -e "$scripttoplevel/iRODS/server/config/database_config.json" )
            {
                $databaseConfigFile = "$scripttoplevel/iRODS/server/config/database_config.json";
            }
        }

        my %svr_variables = (
        "icat_host",        $host,
        "zone_name",        $ZONE_NAME,
        "zone_port",        $IRODS_PORT + 0, # convert to integer
        "zone_user",        $IRODS_ADMIN_NAME,
        "zone_auth_scheme", "native" );
        printStatus( "    Updating $serverConfigFile....\n" );
        printLog( "    Updating $serverConfigFile....\n" );
        printLog( "        icat_host        = $host\n" );
        printLog( "        zone_name        = $ZONE_NAME\n" );
        printLog( "        zone_port        = $IRODS_PORT\n" );
        printLog( "        zone_user        = $IRODS_ADMIN_NAME\n" );
        printLog( "        zone_auth_scheme = native\n" );
        $status = update_json_configuration_file(
                  $serverConfigFile,
                  %svr_variables );
        if ( $status == 0 )
        {
                printError( "\nInstall problem:\n" );
                printError( "    Updating of iRODS $serverConfigFile failed.\n" );
                printError( "        ", $output );
                printLog( "\nCannot update variables in $serverConfigFile.\n" );
                printLog( "    ", $output );
                cleanAndExit( 1 );
        }
        chmod( 0600, $serverConfigFile );

        if ( $IRODS_ICAT_HOST eq "" )
        {
            my %db_variables = (
                    "catalog_database_type",  $DATABASE_TYPE,
                    "db_username",  $DATABASE_ADMIN_NAME,
                    "db_password",  $DATABASE_ADMIN_PASSWORD );
            printStatus( "    Updating $databaseConfigFile....\n" );
            printLog( "    Updating $databaseConfigFile....\n" );
            printLog( "        catalog_database_type = $DATABASE_TYPE\n" );
            printLog( "        db_username = $DATABASE_ADMIN_NAME\n" );
            printLog( "        db_password = XXXXX\n" );
            $status = update_json_configuration_file(
                    $databaseConfigFile,
                    %db_variables );
            if ( $status == 0 )
            {
                    printError( "\nInstall problem:\n" );
                    printError( "    Updating of iRODS $databaseConfigFile failed.\n" );
                    printError( "        ", $output );
                    printLog( "\nCannot update variables in $databaseConfigFile.\n" );
                    printLog( "    ", $output );
                    cleanAndExit( 1 );
            }
            chmod( 0600, $databaseConfigFile );
        }

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
                # will not attempt to create irods_environment.json (and prompt and
                # hang).  The irods_environment.json is created properly later.
                $ENV{"IRODS_HOST"}=$thisHost;
                $ENV{"IRODS_PORT"}=$IRODS_PORT;
                $ENV{"IRODS_USER_NAME"}=$IRODS_ADMIN_NAME;
                $ENV{"IRODS_ZONE"}=$ZONE_NAME;

                printStatus( "Running 'iinit' to enable server to server connections...\n" );
                printLog( "Running 'iinit' to enable server to server connections...\n" );
                my ($status,$output) = run( "$iinit $IRODS_ADMIN_PASSWORD" );

                printStatus( "Using ICAT-enabled server on '$IRODS_ICAT_HOST'\n" );
                printLog( "Using ICAT-enabled server on '$IRODS_ICAT_HOST'\n" );

                # Unset environment variables temporarily set above.
                delete $ENV{"IRODS_HOST"};
                delete $ENV{"IRODS_PORT"};
                delete $ENV{"IRODS_USER_NAME"};
                delete $ENV{"IRODS_ZONE"};
                return;
        }


        if ( $somethingFailed )
        {
                cleanAndExit( 1 );
        }

}





#
# @brief        Configure iRODS user.
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


        # Create a irods_environment.json file for the user, if needed.
        printStatus( "Updating iRODS user's ~/.irods/irods_environment.json...\n" );
        printLog( "Updating iRODS user's ~/.irods/irods_environment.json...\n" );
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

    # populate the irods environment for this server instance
    printToFile( $userIrodsFile,
        "{\n" .
        "    \"irods_host\": \"$thisHost\",\n" .
        "    \"irods_port\": $IRODS_PORT,\n" .
        "    \"irods_default_resource\": \"$RESOURCE_NAME\",\n" .
        "    \"irods_home\": \"/$ZONE_NAME/home/$IRODS_ADMIN_NAME\",\n" .
        "    \"irods_cwd\": \"/$ZONE_NAME/home/$IRODS_ADMIN_NAME\",\n" .
        "    \"irods_user_name\": \"$IRODS_ADMIN_NAME\",\n" .
        "    \"irods_zone\": \"$ZONE_NAME\",\n" .
        "    \"irods_client_server_negotiation\": \"request_server_negotiation\",\n" .
        "    \"irods_client_server_policy\": \"CS_NEG_REFUSE\",\n" .
        "    \"irods_encryption_key_size\": 32,\n" .
        "    \"irods_encryption_salt_size\": 8,\n" .
        "    \"irods_encryption_num_hash_rounds\": 16,\n" .
        "    \"irods_encryption_algorithm\": \"AES-256-CBC\",\n" .
        "    \"irods_default_hash_scheme\": \"SHA256\",\n" .
        "    \"irods_match_hash_policy\": \"compatible\",\n" .
        "    \"irods_server_control_plane_port\": 1248,\n" .
        "    \"irods_server_control_plane_key\": \"TEMPORARY__32byte_ctrl_plane_key\",\n" .
        "    \"irods_server_control_plane_encryption_num_hash_rounds\": 16,\n" .
        "    \"irods_server_control_plane_encryption_algorithm\": \"AES-256-CBC\"\n" .
        "}\n"
         );


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

        # Update admin password
        if ( $IRODS_ICAT_HOST eq "" )
        {
            printStatus( "Updating admin user...\n" );
            printLog( "\nUpdating admin user...\n" );
            my ($status,$output) = run( "$iadmin moduser $IRODS_ADMIN_NAME password $IRODS_ADMIN_PASSWORD" );
            if ( $status != 0 )
            {
                printError( "\nInstall problem:\n" );
                printError( "    Cannot update admin user [$IRODS_ADMIN_NAME] password:\n" );
                printError( "        ", $output );
                printLog( "\nCannot update admin user [$IRODS_ADMIN_NAME] password:\n" );
                printLog( "    ", $output );
                cleanAndExit( 1 );
            }
        }

        # Create the default resource, if needed.
        printStatus( "Creating default resource...\n" );
        printLog( "\nCreating default resource...\n" );

        # List existing resources first to see if it already exists.
        my ($status,$output) = run( "$iadmin lr" );
        if ( $status == 0 && index($output,$RESOURCE_NAME) >= 0 )
        {
                printStatus( "    Skipped.  Resource [$RESOURCE_NAME] already created.\n" );
                printLog( "    Skipped.  Resource [$RESOURCE_NAME] already created.\n" );
        }
        else
        {
                # Create vault path if it does not exist
                if (! -e "$RESOURCE_DIR") {
                        mkpath( "$RESOURCE_DIR", {error => \my $err} );
                        if (@$err) {
                            printError( "\nInstall problem:\n" );
                            printError( "    Cannot create default resource vault path:\n" );
                            foreach $e (@$err) {
                                while (($k, $v) = each (%$e)) {
                                    printError( "        '$k' => '$v'\n" );
                                }
                            }
                            printError( "\n" );
                            printLog( "\nInstall problem:\n" );
                            printLog( "    Cannot create default resource vault path:\n" );
                            foreach $e (@$err) {
                                while (($k, $v) = each (%$e)) {
                                    printLog( "        '$k' => '$v'\n" );
                                }
                            }
                            cleanAndExit( 1 );
                        }
                }
                # Create new default resource
                ($status,$output) = run( "$iadmin mkresc $RESOURCE_NAME unixfilesystem $thisHost:$RESOURCE_DIR \"\"" );
                if ( $status != 0 )
                {
                        printError( "\nInstall problem:\n" );
                        printError( "    Cannot create default resource [$RESOURCE_NAME] [$RESOURCE_DIR]:\n" );
                        printError( "        ", $output );
                        printLog( "\nCannot create default resource [$RESOURCE_NAME] [$RESOURCE_DIR]:\n" );
                        printLog( "    ", $output );
                        cleanAndExit( 1 );
                }
                else {
                        printStatus( "    ... Success [$RESOURCE_NAME] [$RESOURCE_DIR]\n" );
                        printLog(    "    ... Success [$RESOURCE_NAME] [$RESOURCE_DIR]\n" );
                }
        }


        # Test the new vault with an iput and iget
        printStatus( "Testing resource...\n" );
        printLog( "\nTesting resource...\n" );
        my $tmpPutFile = "irods_put_$thisHost.tmp";
        my $tmpGetFile = "irods_get.$thisHost.tmp";
        printToFile( $tmpPutFile, "This is a test file." );

        my ($status,$output) = run( "$iput $tmpPutFile" );
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

}





########################################################################
#
# Support functions.
#
#       All of these functions support activities in the setup steps.
#       Some output error messages directly, but none exit the script
#       on their own.
#

#
# @brief        Create the database, if it doesn't exist already.
#
# This function creates the iRODS/iCAT database using the selected
# database type.
#
# Messages are output on errors.
#
# @return
#       A numeric code indicating success or failure:
#               0 = failure
#               1 = created
#               2 = already created
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
# @brief        Show a list of tables in the database.
#
# This function issues an appropriate command to the database to
# get a list of existing tables.
#
# Messages are output on errors.
#
# @param        $dbname
#       The database name.
# @return
#       A list of tables, or undef on error.
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
# @brief        Get the process IDs of the iRODS servers.
#
# @return       %serverPids     associative array of arrays of
#                               process ids.  Keys are server
#                               types from %servers.
#
sub getIrodsProcessIds()
{
        my @serverPids = ();

        $parentPid=getIrodsServerPid();
        my @serverPids = getFamilyProcessIds( $parentPid );

        return @serverPids;
}





#
# @brief        Start iRODS server.
#
# This function starts the iRODS server and confirms that it
# started.
#
# @return
#       A numeric code indicating success or failure:
#               0 = failure
#               1 = started
#               2 = already started
#
sub startIrods()
{
        # See if server is already started
        my @serverPids = getIrodsProcessIds( );

        if ( $#serverPids > 0 )
        {
                return 2;
        }

        ($status,$output) = run( "$perl $irodsctl start" );

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
# @brief        Get the iRODS Server PID (Parent of the others)
#
# This function iRODS Server PID, which is the parent of the others.
#
# @return
#       The PID or "NotFound"
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
# @brief        Stop iRODS server.
#
# This function stops the iRODS server and confirms that it
# stopped.
#
# @return
#       A numeric code indicating success or failure:
#               0 = failure
#               1 = stopped
#               2 = already stopped
#
sub stopIrods
{
        #
        # Design Notes:  The current version of this uses a file created
        #       by the irods server which records its PID and then finds
        #       the children.
        #
        # Find and kill the server process IDs
        $parentPid=getIrodsServerPid();
        my @pids = getFamilyProcessIds( $parentPid );
        my $found = 0;
        $num = @pids;
        print( "Found $num processes:\n" );
        foreach $pid (@pids)
        {
                $found = 1;
                print( "\tStopping process id $pid\n" );
                kill( 'SIGINT', $pid );
        }
        if ( ! $found )
        {
                system( "ps aux | grep \"^[_]\\?\$USER\" | grep \"irods[A|S|R|X]\" | awk '{print \$2}' | xargs kill -9 > /dev/null 2>&1" );
                printStatus( "    There are no iRODS servers running.\n" );
                return 1;
        }

        # Repeat to catch stragglers.  This time use kill -9.
        my @pids = getFamilyProcessIds( $parentPid );
        my $found = 0;
        foreach $pid (@pids)
        {
                $found = 1;
                print( "\tKilling process id $pid\n" );
                kill( 9, $pid );
        }

    # no regard for PIDs
    # iRODS must kill all owned processes for packaging purposes
    system( "ps aux | grep \"^[_]\\?\$USER\" | grep \"irods[A|S|R|X]\" | awk '{print \$2}' | xargs kill -9 > /dev/null 2>&1" );

        return 1;
}






#
# @brief        Run an iCommand and report an error, if any.
#
# This function runs the given command and collects its output.
# On an error, a message is output.
#
# @param        $command
#       the iCommand to run
# @return
#       A numeric code indicating success or failure:
#               0 = failure
#               1 = success
#
sub runIcommand($)
{
        my ($icommand) = @_;

        my ($status,$output) = run( $icommand );
        return 1 if ( $status == 0 );   # Success


        printError( "\nInstall problem:\n" );
        printError( "    iRODS command failed:\n" );
        printError( "        Command:  $icommand\n" );
        printError( "        ", $output );
        printLog( "icommand failed:\n" );
        printLog( "    ", $output );
        return 0;
}





#
# @brief        Make an iRODS directory, if it doesn't already exist.
#
# This function runs the iRODS command needed to create a directory.
# It checks that the directory doesn't exist first.
#
# @param        $directory
#       the directory to create
# @return
#       A numeric code indicating success or failure:
#               0 = failure
#               1 = success
#               2 = already exists
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
        return runIcommand( "$iadmin mkdir -f $directory" );
}





#
# @brief        Make an iRODS admin account, if it doesn't already exist.
#
# This function runs the iRODS command needed to create a user account.
# It checks that the account doesn't exist first.
#
# @param        $username
#       the name of the user account
# @return
#       A numeric code indicating success or failure:
#               0 = failure
#               1 = success
#               2 = already exists
#
sub imkuser($)
{
        my ($username) = @_;

        # Check if the account already exists.
        #       'iadmin lu' returns a list of accounts, one per line.
        #       Ignore leading and trailing white-space on the account name.
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
# @brief        Make an iRODS group, if it doesn't already exist.
#
# This function runs the iRODS command needed to create a user group.
# It checks that the group doesn't exist first.
#
# @param        $groupname
#       the name of the user group
# @return
#       A numeric code indicating success or failure:
#               0 = failure
#               1 = success
#               2 = already exists
#
sub imkgroup($)
{
        my ($groupname) = @_;

        # Check if the account already exists.
        #       'iadmin lu' returns a list of accounts, one per line.
        #       Ignore leading and trailing white-space on the group name.
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
# @brief        Give ownership permissions on a directory, if not already set.
#
# This function runs the iRODS command needed to set a directory's owner.
# It checks that the ownership wasn't already set.
#
# @param        $username
#       the name of the user account
# @param        $directory
#       the directory to change
# @return
#       A numeric code indicating success or failure:
#               0 = failure
#               1 = success
#               2 = already set
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
# @brief        Send a file of SQL commands to the database.
#
# The given SQL commands are passed to the database.  The
# command to do this varies with the type of database in use.
#
# @param        $databaseName
#       the name of the database
# @param        $sqlFilename
#       the name of the file
# @return
#       A 2-tuple including a numeric code indicating success
#       or failure, and the stdout/stderr of the command.
#               0 = failure
#               1 = success
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
# @brief        Exit the script abruptly.
#
# Print a final message, then close the log and exit.
#
# @param        $code   the exit code
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
# @brief        Print command-line help
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

        setPrintVerbose( $oldVerbosity );
}




########################################################################
#
#  Postgres functions.
#
#       These functions are all Postgres-specific.  They are called
#       by the setup functions if the database under our control is
#       Postgres.  All of them output error messages on failure,
#       but none exit directly.
#

#
# @brief        Configure user accounts for a Postgres database.
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
                #       host:port:database:user:password

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
# @brief        Create a Postgres database.
#
# This function creates a Postgres database.
#
# These actions could require a database restart.
#
# Messages are output on errors.
#
# @return
#       A numeric code indicating success or failure:
#               0 = failure
#               1 = success
#               2 = already created
#
sub Postgres_CreateDatabase()
{
        # Check if the database already exists.
        #       'psql' will list databases, or report an error
        #       if the one we want isn't there.
        printStatus( "Checking whether iCAT database exists...\n" );
        printLog( "\nChecking whether iCAT database exists...\n" );
        my $needCreate = 1;

        if( $RUNINPLACE == 1 )
        {
            $finding_psql="$scripttoplevel/plugins/database/packaging/find_bin_postgres.sh";
        }
        else
        {
            $finding_psql="$scripttoplevel/packaging/find_bin_postgres.sh";
        }
        $PSQL=`$finding_psql`;
        chomp $PSQL;
        if ($PSQL eq "FAIL") {
            printStatus("    Ran $finding_psql\n");
            printStatus("    Failed to find psql binary.\n");
            printLog("    Ran $finding_psql\n");
            printLog("    Failed to find psql binary.\n");
            cleanAndExit( 1 );
        }
        $PSQL=$PSQL . "/psql";
    if ($DATABASE_HOST eq "localhost") {
        $psqlcmd = "$PSQL -U $DATABASE_ADMIN_NAME -p $DATABASE_PORT -l $DB_NAME";
    }
    else {
        $psqlcmd = "$PSQL -U $DATABASE_ADMIN_NAME -p $DATABASE_PORT -h $DATABASE_HOST -l $DB_NAME";
    }
    printLog( "    Connecting with: $psqlcmd\n" );
    my ($status,$output) = run( $psqlcmd );
        if ( $output =~ /List of databases/i )
        {
                printStatus( "    [$DB_NAME] on [$DATABASE_HOST] found.\n");
                printLog( "    [$DB_NAME] on [$DATABASE_HOST] found.\n");
        }
        else
        {
                printError( "\nInstall problem:\n" );
                printError( "    Database [$DB_NAME] on [$DATABASE_HOST] cannot be found.\n" );
                printError( "    $output\n" );
                printLog( "\nInstall problem:\n" );
                printLog( "    Database [$DB_NAME] on [$DATABASE_HOST] cannot be found.\n" );
                printLog( "    $output\n" );
                cleanAndExit( 1 );
        }

        # Update the ODBC configuration files to include the database name.
        #       If Postgres was installed by the iRODS scripts, then
        #       either 'odbcinst.ini' or 'odbc.ini' has been created
        #       when the ODBC drivers were installed.  In both cases,
        #       the 'Database' field was left out because the install
        #       script didn't know the database name at the time.
        #
        #       If Postgres was previously installed, then 'odbcinst.ini'
        #       or 'odbc.ini' may have been created and reference another
        #       database.  Or the files may be empty or not exist.
        #
        #       In order for the ODBC drivers to work, and depending upon
        #       the driver installed, these files must include a section
        #       for 'PostgreSQL' that gives the database name.
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
                        my $libPath = abs_path( File::Spec->catfile($databaseLibDir, "libodbcpsql.so" ) );

                        printToFile( $ini,
                                "[postgres]\n" .
                                "Driver=$libPath\n" .
                                "Debug=0\n" .
                                "CommLog=0\n" .
                                "Servername=$DATABASE_HOST\n" .
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
                        $status = Postgres_updateODBC( $ini, "postgres",
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
                                "[postgres]\n" .
                                "Debug=0\n" .
                                "CommLog=0\n" .
                                "Servername=$DATABASE_HOST\n" .
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
                        $status = Postgres_updateODBC( $ini, "postgres",
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
                printStatus( "    Touching... $userODBC\n" );
                open( TOUCHFILE, ">>$userODBC" );
                close( TOUCHFILE );
        }
        if ( ! -e $userODBC )
        {
                printStatus( "    Skipped.  File doesn't exist. - $userODBC\n" );
                printLog( "    Skipped.  File doesn't exist.\n" );
        }
        else
        {
                # iRODS now supports a script to determine the path & lib name of the odbc driver
                 my $psqlOdbcLib;
                if ( $RUNINPLACE == 1 )
                {
                        $psqlOdbcLib = `$scripttoplevel/plugins/database/packaging/find_odbc_postgres.sh $scripttoplevel`;
                }
                else
                {
                        $psqlOdbcLib = `$scripttoplevel/packaging/find_odbc_postgres.sh`;
                }
                chomp($psqlOdbcLib);

                if ($psqlOdbcLib eq "")
                {
                        printError("\nInstall Problem:\n");
                        printError("    find_odbc_postgres.sh did not find odbc driver.\n");
                        printLog("\nInstall Problem:\n");
                        printLog("    find_odbc_postgres.sh did not find odbc driver.\n");

                        cleanAndExit( 1 );
                }

                open( NEWCONFIGFILE, ">$userODBC" );
                print( NEWCONFIGFILE "[postgres]\n" .
                                "Driver=$psqlOdbcLib\n" .
                                "Debug=0\n" .
                                "CommLog=0\n" .
                                "Servername=$DATABASE_HOST\n" .
                                "Database=$DB_NAME\n" .
                                "ReadOnly=no\n" .
                                "Ksqo=0\n" .
                                "Port=$DATABASE_PORT\n" );

                close( NEWCONFIGFILE );

                chmod( 0600, $userODBC );
        }
        return 1;
}


#
# @brief        'Create' an Oracle database.
#
# Stub that corresponds to the function creates a database, but for
# Oracle this is done by the DBA.
#
# @return
#       A numeric code indicating success or failure:
#               0 = failure
#               1 = success
#               2 = already created
#
sub Oracle_CreateDatabase()
{
        printStatus( "CreateDatabase Skipped.  For Oracle, DBA creates the instance.\n" );
        printLog( "CreateDatabase Skipped.  For Oracle, DBA creates the instance.\n" );

        # =-=-=-=-=-=-=-
        # configure the service accounts .odbc.ini file
        printStatus( "Updating the .odbc.ini...\n" );
        printLog( "Updating the .odbc.ini...\n" );
        my $userODBC = File::Spec->catfile( $ENV{"HOME"}, ".odbc.ini" );

        # iRODS now supports a script to determine the path & lib name of the odbc driver
        my $oracleOdbcLib;
        if ( $RUNINPLACE == 1 )
        {
                $oracleOdbcLib = `$scripttoplevel/plugins/database/packaging/find_odbc_oracle.sh`;
        }
        else
        {
                $oracleOdbcLib = `$scripttoplevel/packaging/find_odbc_oracle.sh`;
        }
        chomp($oracleOdbcLib);

        open( NEWCONFIGFILE, ">$userODBC" );
        print( NEWCONFIGFILE "[oracle]\n" .
            "Driver=$oracleOdbcLib\n" .
            "Database=$DB_NAME\n" .
            "Servername=$DATABASE_HOST\n" .
            "Port=$DATABASE_PORT\n" );

        close( NEWCONFIGFILE );

        chmod( 0600, $userODBC );
        return 1;

}



#
# @brief        'Create' a MySQL database.
#
# Stub that corresponds to the function creates a database, but for
# MySQL this is done by the DBA.
#
# @return
#       A numeric code indicating success or failure:
#               0 = failure
#               1 = success
#               2 = already created
#
sub MySQL_CreateDatabase()
{
        printStatus( "CreateDatabase Skipped.  For MySQL, DBA creates the instance.\n" );
        printLog( "CreateDatabase Skipped.  For MySQL, DBA creates the instance.\n" );

        # =-=-=-=-=-=-=-
        # configure the service accounts .odbc.ini file
        printStatus( "Updating the .odbc.ini...\n" );
        printLog( "Updating the .odbc.ini...\n" );
        my $userODBC = File::Spec->catfile( $ENV{"HOME"}, ".odbc.ini" );

    # iRODS now supports a script to determine the path & lib name of the odbc driver
    my $mysqlOdbcLib;
    if ( $RUNINPLACE == 1 )
    {
            $mysqlOdbcLib = `$scripttoplevel/plugins/database/packaging/find_odbc_mysql.sh`;
    }
    else
    {
            $mysqlOdbcLib = `$scripttoplevel/packaging/find_odbc_mysql.sh`;
    }
    chomp($mysqlOdbcLib);

    open( NEWCONFIGFILE, ">$userODBC" );
    print( NEWCONFIGFILE "[mysql]\n" .
            "Driver=$mysqlOdbcLib\n" .
            "Database=$DB_NAME\n" .
            "Server=$DATABASE_HOST\n" .
            "Option=2\n" .
            "Port=$DATABASE_PORT\n" );

    close( NEWCONFIGFILE );

    chmod( 0600, $userODBC );
    return 1;
}



#
# @brief        Show a list of tables in the database.
#
# This function issues an appropriate command to the database to
# get a list of existing tables.
#
# Messages are output on errors.
#
# @param        $dbname
#       The database name.
# @return
#       A list of tables, or undef on error.
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
# @brief        Show a list of tables in the database.
#
# This function issues an appropriate command to the database to
# get a list of existing tables.
#
# Messages are output on errors.
#
# @param        $dbname
#       The database name (unused; see oracle_sql).
# @return
#       A list of tables, or undef on error.
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
# @brief        Show a list of tables in the database.
#
# This function issues an appropriate command to the database to
# get a list of existing tables.
#
# Messages are output on errors.
#
# @param        $dbname
#       The database name (unused; see oracle_sql).
# @return
#       A list of tables, or undef on error.
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
# @brief        Update an ODBC configuration file to include a
#               given name and value.
#
# This function edits an ODBC '.ini' file and insures that the given name
# and value are set in the given section.  The rest of the file is left
# untouched.
#
# @param        $filename
#       The name of the ODBC file to change.
# @param        $section
#       The section of the ODBC file to change.
# @param        $name
#       The name to change or add to the section.
# @param        $value
#       The value to set for that name.
# @return
#       A numeric value indicating:
#               0 = failure
#               1 = success, file changed
#               2 = success, file no changed (OK as-is)
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
        open( NEWCONFIGFILE, ">$filename" );    # Truncate
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
# @brief        Send a file of SQL commands to the Postgres database.
#
# This function runs 'psql' and passes it the given SQL.
#
# @param        $databaseName
#       the name of the database
# @param        $sqlFilename
#       the name of the file
# @return
#       A 2-tuple including a numeric code indicating success
#       or failure, and the stdout/stderr of the command.
#               0 = failure
#               1 = success
#
sub Postgres_sql($$)
{
        my ($databaseName,$sqlFilename) = @_;

        $findbinscript = "find_bin_".$DATABASE_TYPE.".sh";
        if ( $RUNINPLACE == 1 )
        {
                $PSQL=`$scripttoplevel/plugins/database/packaging/$findbinscript`;
        }
        else
        {
                $PSQL=`$scripttoplevel/packaging/$findbinscript`;
        }
        chomp $PSQL;
        $PSQL=$PSQL . "/psql";

        if ($DATABASE_HOST eq "localhost") {
            return run( "$PSQL -U $DATABASE_ADMIN_NAME -p $DATABASE_PORT $databaseName < $sqlFilename" );
        }
        return run( "$PSQL -U $DATABASE_ADMIN_NAME -p $DATABASE_PORT -h $DATABASE_HOST $databaseName < $sqlFilename" );
}

#
# @brief        Send a file of SQL commands to the Oracle database.
#
# This function runs 'sqlplus' and passes it the given SQL.
#
# @param        $databaseName
#       the name of the database  (Not used); instead
#    $DATABASE_ADMIN_NAME and $DATABASE_ADMIN_PASSWORD are spliced together
#         in the form needed by sqlplus (note: should restructure the args in
#    the call tree for this, perhaps just let this and Postgres_sql set these.)
# @param        $sqlFilename
#       the name of the file
# @return
#       A 2-tuple including a numeric code indicating success
#       or failure, and the stdout/stderr of the command.
#               0 = failure
#               1 = success
#
sub Oracle_sql($$)
{
    my ($databaseName,$sqlFilename) = @_;
    my ($connectArg, $i);
    $i = index($DATABASE_ADMIN_NAME, "@");

    $dbadmin = substr($DATABASE_ADMIN_NAME, 0, $i);
    $tnsname = substr($DATABASE_ADMIN_NAME, $i+1 );
    $connectArg = $dbadmin . "/" .
              $DATABASE_ADMIN_PASSWORD . "@" .
              $tnsname;
    $sqlplus = "sqlplus";
    $exec_str = "$sqlplus '$connectArg' < $sqlFilename";
    ($code,$output) = run( "$exec_str" );

    return ($code,$output);
}

#
# @brief        Send a file of SQL commands to the MySQL database.
#
# This function runs 'isql' and passes it the given SQL.
#
# @param        $databaseName
#       the name of the database
# @param        $sqlFilename
#       the name of the file
# @return
#       A 2-tuple including a numeric code indicating success
#       or failure, and the stdout/stderr of the command.
#               0 = failure
#               1 = success
#
sub MySQL_sql($$)
{
        $findbinscript = "find_bin_".$DATABASE_TYPE.".sh";
        if ( $RUNINPLACE == 1 )
        {
                $MYSQL=`$scripttoplevel/plugins/database/packaging/$findbinscript`;
        }
        else
        {
                $MYSQL=`$scripttoplevel/packaging/$findbinscript`;
        }
        chomp $MYSQL;
        $MYSQL=$MYSQL . "/mysql";
        my ($databaseName,$sqlFilename) = @_;
        return run( "$MYSQL --user=$DATABASE_ADMIN_NAME --host=$DATABASE_HOST --port=$DATABASE_PORT --password=$DATABASE_ADMIN_PASSWORD $databaseName < $sqlFilename" );
}
