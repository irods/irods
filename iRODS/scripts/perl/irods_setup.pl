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

# set flag to determine if this is an iCAT installation or not
$icatInstall = 0;
if( scalar(@ARGV) > 0 ) {
        $icatInstall = 1;
}

########################################################################
#
# Confirm execution from the top-level iRODS directory.
#

my $perlScriptsDir = File::Spec->catdir( $scripttoplevel, "iRODS", "scripts", "perl" );
my $pythonScriptsDir = File::Spec->catdir( $scripttoplevel, "scripts" );

# iRODS configuration directory
$configDir = ( -e "$scripttoplevel/packaging/binary_installation.flag" ) ?
    "/etc/irods" :
    File::Spec->catdir( "$scripttoplevel", "iRODS", "config" );


$userIrodsDir = File::Spec->catfile( $ENV{"HOME"}, ".irods" );
$userIrodsFile = File::Spec->catfile( $userIrodsDir, "irods_environment.json" );

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

require File::Spec->catfile( $perlScriptsDir, "utils_file.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_config.pl" );
require File::Spec->catfile( $perlScriptsDir, "utils_print.pl" );


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
my $thisHost   = `hostname -f`;
$thisHost =~ s/^\s+|\s+$//g;

# Name a log file.
my $logFile = File::Spec->catfile( $scripttoplevel, "log", "setup_log.txt" );

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

# What's the name of this script?
printLog( "\nScript:\n" );
printLog( "    Script:  $scriptName\n" );
printLog( "    CWD:  " . cwd( ) . "\n" );

# What version of perl?
printLog( "\nPerl:\n" );
printLog( "    Perl path:  $perl\n" );
printLog( "    Perl version:  $]\n" );

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
if( 0 == $icatInstall )
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

#This will and should be false if we are installing just the icommands
if ( $IRODS_ADMIN_NAME ne "" )
{
    if ( stopIrods() == 0 )
    {
        cleanAndExit( 1 );
    }

    if ( $DATABASE_TYPE ne "" )
    {
            # Ignore the iCAT host name, if any, if we
            # are using a database.
            $IRODS_ICAT_HOST = "";

            testDatabase();
    }

    $totalSteps  = 2;
    $currentStep = 0;

    # 1.  Configure
    configureIrodsServer( );

    # 2.  Configure the iRODS user account.
    configureIrodsUser( );

}

done();

# Done.
sub done()
{
    printLog( "\nDone.\n" );
    closeLog( );
    if ( ! $noHeader )
    {
            printSubtitle( "\nDone.  Additional detailed information is in the log file:\n" );
            printStatus( "$logFile\n" );
    }
    exit( 0 );
}

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


        # Stop iRODS if it is running.
        printStatus( "Stopping iRODS server...\n" );
        printLog( "Stopping iRODS server...\n" );
        $currentPort = $IRODS_PORT;

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

        my $test_cll = File::Spec->catfile( $scripttoplevel, "iRODS", "server", "test", "bin", "test_cll" );

        my $output = `$test_cll $DATABASE_ADMIN_NAME '$DATABASE_ADMIN_PASSWORD' 2>&1`;

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
        printStatus( "Updating $serverConfigFile...\n" );
        printLog( "Updating $serverConfigFile...\n" );
        printLog( "    icat_host        = $host\n" );
        printLog( "    zone_name        = $ZONE_NAME\n" );
        printLog( "    zone_port        = $IRODS_PORT\n" );
        printLog( "    zone_user        = $IRODS_ADMIN_NAME\n" );
        printLog( "    zone_auth_scheme = native\n" );
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
            printStatus( "Updating $databaseConfigFile...\n" );
            printLog( "Updating $databaseConfigFile...\n" );
            printLog( "    catalog_database_type = $DATABASE_TYPE\n" );
            printLog( "    db_username = $DATABASE_ADMIN_NAME\n" );
            printLog( "    db_password = XXXXX\n" );
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
                $ENV{"IRODS_ZONE_NAME"}=$ZONE_NAME;

                printStatus( "Running 'iinit' to enable server to server connections...\n" );
                printLog( "Running 'iinit' to enable server to server connections...\n" );
                $output = `iinit $IRODS_ADMIN_PASSWORD`;

                printStatus( "Using ICAT-enabled server on '$IRODS_ICAT_HOST'\n" );
                printLog( "Using ICAT-enabled server on '$IRODS_ICAT_HOST'\n" );

                # Unset environment variables temporarily set above.
                delete $ENV{"IRODS_HOST"};
                delete $ENV{"IRODS_PORT"};
                delete $ENV{"IRODS_USER_NAME"};
                delete $ENV{"IRODS_ZONE_NAME"};
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
            my $output = `iinit $IRODS_ADMIN_PASSWORD`;
            if ( $? != 0 )
            {
                printError( "\nInstall problem:\n" );
                printError( "    Connection to iRODS server failed.\n" );
                printError( "        $output" );
                printLog( "\nCannot open connection to iRODS server.\n" );
                printLog( "    $output" );
                cleanAndExit( 1 );
            }
            return;
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
        "    \"irods_zone_name\": \"$ZONE_NAME\",\n" .
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
        "    \"irods_server_control_plane_encryption_algorithm\": \"AES-256-CBC\",\n" .
        "    \"irods_maximum_size_for_single_buffer_in_megabytes\": 32,\n" .
        "    \"irods_default_number_of_transfer_threads\": 4,\n" .
        "    \"irods_transfer_buffer_size_for_parallel_transfer_in_megabytes\": 4\n" .
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
        my $output = `iinit $IRODS_ADMIN_PASSWORD`;
        if ( $? != 0 )
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
            my $output = `iadmin moduser $IRODS_ADMIN_NAME password $IRODS_ADMIN_PASSWORD`;
            if ( $? != 0 )
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
        my $output = `iadmin lr`;
        if ( $? == 0 && index($output,$RESOURCE_NAME) >= 0 )
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
                $output = `iadmin mkresc $RESOURCE_NAME unixfilesystem $thisHost:$RESOURCE_DIR \"\"`;
                if ( $? != 0 )
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
        if ( ! -e $tmpPutFile ){
            printToFile( $tmpPutFile, "This is a test file." );
        }

        my $output = `iput -f $tmpPutFile`;
        if ( $? != 0 )
        {
                printError( "\nInstall problem:\n" );
                printError( "    Cannot put test file into iRODS:\n" );
                printError( "        ", $output );
                printLog( "\nCannot put test file into iRODS:\n" );
                printLog( "    ", $output );
                unlink( $tmpPutFile );
                cleanAndExit( 1 );
        }

        $output = `iget -f $tmpPutFile $tmpGetFile`;
        if ( $? != 0 )
        {
                printError( "\nInstall problem:\n" );
                printError( "    Cannot get test file from iRODS:\n" );
                printError( "        ", $output );
                printLog( "\nCannot get test file from iRODS:\n" );
                printLog( "    ", $output );
                unlink( $tmpPutFile );
                cleanAndExit( 1 );
        }

        $output = `diff $tmpPutFile $tmpGetFile`;
        if ( $? != 0 )
        {
                printError( "\nInstall problem:\n" );
                printError( "    Test file retrieved from iRODS doesn't match file written there:\n" );
                printError( "        ", $output );
                printLog( "\nGet file doesn't match put file:\n" );
                printLog( "    ", $output );
                unlink( $tmpPutFile );
                unlink( $tmpGetFile );
                cleanAndExit( 1 );
        }

        $output = `irm -f $tmpPutFile`;
        if ( $? != 0 )
        {
                printError( "\nInstall problem:\n" );
                printError( "    Cannot remove test file from iRODS:\n" );
                printError( "        ", $output );
                printLog( "\nCannot remove test file from iRODS:\n" );
                printLog( "    ", $output );
                unlink( $tmpPutFile );
                unlink( $tmpGetFile );
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
# @brief        Get the process IDs of the iRODS servers.
#
# @return       %serverPids     associative array of arrays of
#                               process ids.  Keys are server
#                               types from %servers.
#
sub getIrodsProcessIds()
{
        my $parentPid = getIrodsServerPid();
        if ( $parentPid == "NotFound" )
        {
            return ();
        }
        print "getIrodsProcessIds $parentPid\n";
        my @pids = `ps --ppid $parentPid o pid`;
        shift @pids;
        return @pids;
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

        $output = `python $pythonScriptsDir/irods_control.py start`;

        if ( $? != 0 )
        {
                printError( "Could not start iRODS server.\n" );
                printError( "    $output\n" );
                printLog( "\nCould not start iRODS server.\n" );
                printLog( "    $output\n" );
                return 0;
        }
        $output =~ s/^(.*\n){1}//; # remove duplicate first line of output
        chomp($output);
        if ( $output =~ "Validation Failed" )
        {
                printWarning( "$output\n" );
                printLog( "$output\n" );
        }
        else {
                printStatus( "$output\n" );
                printLog( "$output\n" );
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

        my $parentPid="NotFound";
        if (open( PIDFILE, '<', $processFile )) {
            my $line;
            foreach $line (<PIDFILE>)
            {
                    my $i = index($line, " ");
                    $parentPid=substr($line,0,$i);
            }
            close( PIDFILE );
        }
        return $parentPid;
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
        my @pids = getIrodsProcessIds();
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
                system( "python $pythonScriptsDir/terminate_irods_processes.py" );
                printStatus( "    There are no iRODS servers running.\n" );
                return 1;
        }

        # Repeat to catch stragglers.  This time use kill -9.
        my $pids=getIrodsProcessIds();
        shift @pids;
        my $found = 0;
        foreach $pid (@pids)
        {
                $found = 1;
                print( "\tKilling process id $pid\n" );
                kill( 9, $pid );
        }

    # no regard for PIDs
    # iRODS must kill all owned processes for packaging purposes
    system( "python $pythonScriptsDir/terminate_irods_processes.py" );

    return 1;
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
        my $output = `iexit full 2>&1`;

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
