#
#
# This Perl script controls the iRODS servers.
#
# Usage is:
#       perl irodsctl.pl [options] [commands]
#
# Help options:
#       --help      Show a list of options and commands
#
# Verbosity options:
#       --quiet     Suppress all messages
#       --verbose   Output all messages (default)
#
# Commands:
#   See the printUsage function.
#

use File::Spec;
use File::Path;
use File::Copy;
use File::Basename;
use Cwd;
use Cwd "abs_path";
use Config;

$version{"irodsctl.pl"} = "Feb 2015";


$scriptfullpath = abs_path(__FILE__);
$scripttoplevel = dirname(dirname(dirname(dirname($scriptfullpath))));


########################################################################
#
# Confirm execution from the top-level iRODS directory.
#
$IRODS_HOME = cwd( );   # Might not be actual iRODS home.  Fixed below.

my $perlScriptsDir = File::Spec->catdir( $IRODS_HOME, "scripts", "perl" );

# Where is the configuration directory for iRODS?  This is where
# support scripts are kept.

$configDir = `perl $perlScriptsDir/irods_get_config_dir.pl`;

if ( ! -e $configDir )
{
        # Configuration directory does not exist.  Perhaps this
        # script was run from the config or install subdirectories.
        # Look up one directory.
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

# Determine the execution environment.
my $thisUserID = $<;
my $thisHost   = getCurrentHostName( );

# Set the number of seconds to sleep after starting or stopping the
# iRODS servers.  This gives them time to start or stop before
# we do anything more.  The actual number here is a guess.  Different
# CPU speeds, host loads, or database versions may make this too little
# or too much.
my $iRODSStartStopDelay = 6;  # Seconds





########################################################################
#
# Load configuration files
#

# binary installation
if ( -e "$scripttoplevel/packaging/binary_installation.flag" )
{
    load_server_config("/etc/irods/server_config.json");
    if ( -e "/etc/irods/database_config.json" )
    {
        load_database_config("/etc/irods/database_config.json");
    }
}
# run-in-place
else
{
    load_server_config("$scripttoplevel/iRODS/server/config/server_config.json");
    if ( -e "$scripttoplevel/iRODS/server/config/database_config.json" )
    {
        load_database_config("$scripttoplevel/iRODS/server/config/database_config.json");
    }
}


########################################################################
#
# Set common paths.
#
# Directory containing the servers
$serverBinDir = File::Spec->catdir( $IRODS_HOME, "server", "bin" );

# The iRODS server
$irodsServer  = File::Spec->catfile( $serverBinDir, "irodsServer" );

# Directory containing server configuration 'server_config.json'.
$irodsServerConfigDir = File::Spec->catdir( $IRODS_HOME, "server", "config" );

# Directory for the server log.
$irodsLogDir    = File::Spec->catdir( $IRODS_HOME, "server", "log" );

# Postgres bin dir.
$postgresBinDir  = File::Spec->catdir( $POSTGRES_HOME, "bin" );

# iRODS Server names
%servers = (
        # Nice name             process name
        "iRODS agents" =>       "(irodsAge)(nt)?",
        "iRODS rule servers" => "(irodsReS)(erver)?",
        "iRODS servers" =>      "(irodsSer)(ver)?"

        # Process names need to be able to match on just 8 characters,
        # due to limitations of SysV /bin/ps (such as Solaris) which
        # only shows the 1st 8 characters.
);






########################################################################
#
# Overrides for sysadmins that edit this file.
#
# While we encourage you to use the defaults, you can override them
# here and they'll be used the next time the iRODS servers are started
# using this script.
#

$ENV{'irodsHomeDir'}      = $IRODS_HOME;
$ENV{'irodsConfigDir'}      = $irodsServerConfigDir;
if ($irodsEnvFile)              { $ENV{'IRODS_ENVIRNOMENT_FILE'} = $irodsEnvFile; }
if ($irodsPort)                 { $ENV{'IRODS_PORT'}          = $irodsPort; }
if ($spLogLevel)                { $ENV{'spLogLevel'}          = $spLogLevel; }
if ($spLogSql)                  { $ENV{'spLogSql'}            = $spLogSql; }
if ($SVR_PORT_RANGE_START)      { $ENV{'svrPortRangeStart'}   = $SVR_PORT_RANGE_START; }
if ($SVR_PORT_RANGE_END)        { $ENV{'svrPortRangeEnd'}     = $SVR_PORT_RANGE_END; }
if ($svrPortRangeStart)         { $ENV{'svrPortRangeStart'}   = $svrPortRangeStart; }
if ($svrPortRangeEnd)           { $ENV{'svrPortRangeEnd'}     = $svrPortRangeEnd; }
if ($reServerOption)            { $ENV{'reServerOption'}      = $reServerOption; }
if ($irodsReconnect)            { $ENV{'irodsReconnect'}    = $irodsReconnect; }
if ($RETESTFLAG)                { $ENV{'RETESTFLAG'}          = $RETESTFLAG; }
if ($GLOBALALLRULEEXECFLAG)    { $ENV{'GLOBALALLRULEEXECFLAG'} = $GLOBALALLRULEEXECFLAG; }
if ($PREPOSTPROCFORGENQUERYFLAG)    { $ENV{'PREPOSTPROCFORGENQUERYFLAG'} = $PREPOSTPROCFORGENQUERYFLAG; }
if ($GLOBALREAUDITFLAG)         { $ENV{'GLOBALREAUDITFLAG'}   = $GLOBALREAUDITFLAG; }
if ($GLOBALREDEBUGFLAG)         { $ENV{'GLOBALREDEBUGFLAG'}   = $GLOBALREDEBUGFLAG; }
if ($LOGFILE_INT)               { $ENV{'logfileInt'}          = $LOGFILE_INT; }



########################################################################
#
# Check usage.
#       Command-line options select whether to be verbose or not.
#
#       Command-line commands tell us what to do.  The main commands
#       start and stop the servers.
#
$scriptName = $0;
setPrintVerbose( 1 );

# Make sure we are not being run as root.
if ( $thisUserID == 0 && not defined $ENV{'irodsServiceUser'})
{
        printError( "Usage error:\n" );
        printError( "    This script should *not* be run as root.\n" );
        exit( 1 );
}

$numberCommands = 0;
$doTestExitValue=0;
$doStartExitValue=0;
foreach $arg (@ARGV)
{
        # Options
        if ( $arg =~ /^-?-?h(elp)$/ )           # Help / usage
        {
                printUsage( );
                exit( 0 );
        }
        if ( $arg =~ /^-?-?q(uiet)$/ )          # Suppress output
        {
                setPrintVerbose( 0 );
                next;
        }
        if ( $arg =~ /^-?-?v(erbose)$/ )        # Enable output
        {
                setPrintVerbose( 1 );
                next;
        }



        # Commands
        if ( $arg =~ /^-?-?(start)$/i )         # Start database and iRODS
        {
                $numberCommands++;
                printSubtitle( "Starting iRODS server...\n" );
                $startVal = startIrods( );
                if ($startVal eq "0") {$doStartExitValue++;}
                next;
        }
        if ( $arg =~ /^-?-?(stop)$/i )          # Stop database and iRODS
        {
                $numberCommands++;
                printSubtitle( "Stopping iRODS server...\n" );
                my $status = stopIrods( );
                next;
        }
        if ( $arg =~ /^-?-?(restart)$/i )       # Restart database and iRODS
        {
                $numberCommands++;
                printSubtitle( "Stopping iRODS server...\n" );
                stopIrods( );   # OK to fail.  Server might not be started
                printSubtitle( "Starting iRODS server...\n" );
                startIrods( );
                next;
        }
        if ( $arg =~ /^-?-?(stat(us)?)$/i )     # Status of iRODS and database
        {
                $numberCommands++;
                doStatus( );
                next;
        }
        if ( $arg =~ /^-?-?testWithoutConfirmation$/ )  # Run iRODS tests
# Undocumented command that does not warn user or prompts;
# used for batch-mode testing.
        {
                $numberCommands++;
                doTest( );
                next;
        }
        if ( $arg =~ /^-?-?devtesty$/ ) # Run iRODS tests
# Undocumented command that does not warn user or prompts;
# used for batch-mode testing.
        {
                $numberCommands++;
                doTest( );
                next;
        }
        if ( $arg =~ /^-?-?devtest$/ )  # Run iRODS developer tests
        {
                $numberCommands++;
                printNotice(
    "Warning, this test is normally only run by iRODS developers as it\n");
                printNotice(
    "performs some risky operations and requires certain default settings.\n");
                printNotice (
    "We do not recommend running this on production systems.  Basic\n");
                printNotice(
    "testing, such as iput and iget, is generally all that is needed.");
                printNotice( "\n" );
                if ( askYesNo( "    Run the test (yes/no)?  " ) == 1 ) {
                    doTest( );
                }
                next;
        }
        if ( $arg =~ /^-?-?loadtest$/ ) # Run concurrent-tests
        {
            $numberCommands++;
            doLoadTest( );
            next;
        }

        printError( "Unknown command:  $arg\n" );
        printUsage( );
        exit( 1 );
}

# Tell the user how to use the script if they didn't provide
# any commands.
if ( $numberCommands == 0 )
{
        printUsage( );
        exit( 1 );
}

# Done!
if ( $doTestExitValue != 0)
{
        exit( $doTestExitValue );
}
if ( $doStartExitValue != 0)
{
        exit( $doStartExitValue );
}
exit( 0 );





########################################################################
#
# Commands
#       Do the job.
#

#
# @brief        Print server status.
#
# Get a process list and look for the iRODS servers and,
# optionally, the database server.  Report them if found.
#
sub doStatus
{
        # Always print status messages, even if verbosity
        # has been disabled.
        my $verbosity = isPrintVerbose( );
        setPrintVerbose( 1 );

        my %serverPids = getIrodsProcessIds( );
        if ( (scalar keys %serverPids) == 0 )
        {
                printSubtitle( "iRODS servers:\n" );
                printStatus( "No servers running\n" );
        }
        else
        {
                foreach $serverType (keys %serverPids)
                {
                        my @pids = @{$serverPids{$serverType}};
                        printSubtitle( $serverType . ":\n" );
                        foreach $pid (@pids)
                        {
                                printStatus( "Process $pid\n" );
                        }
                }
        }

        # Report on our PIDs
        my @pids = getOurIrodsServerPids();
        my $foundOne=0;
        printSubtitle( "iRODS Servers associated with this instance, port $IRODS_PORT:\n" );
        foreach $pid (@pids)
        {
                printStatus( "Process $pid\n" );
                $foundOne=1;
        }
        if ($foundOne==0) {
                printStatus( "None\n" );
        }

        setPrintVerbose( $verbosity );
}




#
# @brief        Test installation.
#
# Run the developer iRODS tests.
#
sub doTest
{
        # TGR - Jan 2015 - move to server_config.json
        # hard-code default password - only run on new installations
        $IRODS_ADMIN_PASSWORD = "rods";

        # get password first if needed (no server on this host)
        if ($IRODS_ADMIN_PASSWORD eq "") {
            print "Please enter your irods password: ";
            my $answer = <STDIN>;
            chomp( $answer );
            $IRODS_ADMIN_PASSWORD = $answer;
        }
        # Start the servers first, if needed.
        my %serverPids = getIrodsProcessIds( );
        if ( (scalar keys %serverPids) == 0 )
        {
                printSubtitle( "Starting servers...\n" );
                printStatus( "Starting iRODS server...\n" );
                startIrods( ) || exit( 1 );
        }

        # Always verbose during testing.
        my $verbosity = isPrintVerbose( );
        setPrintVerbose( 1 );

        # Test iCommands
        printSubtitle( "\nTesting iCommands...\n" );
        doTestIcommands( );

        # Test irules
#       printSubtitle( "\nTesting irules...\n" );
#       doTestIrules( );

        # Check if this host is ICAT-enabled.
        # Note that the tests assume i-commands are in the path so we can too.
        # Need to re-iinit first for svr to svr connections, non-ICAT hosts.
        my $output  = `$iinit $IRODS_ADMIN_PASSWORD 2>&1`;
        my $outEnv = `ienv | grep irods_host | tail -1`;
        my $outMisc = `imiscsvrinfo`;
        my $myHostName = getCurrentHostName( );
        if ( $outEnv =~ /$myHostName/ &&
             $outMisc =~ /RCAT_ENABLED/) {
            # Test iCAT
            printSubtitle( "\nTesting iCAT...\n" );
            doTestIcat( );
        }
        else {
            printNotice( "\nSkipping iCAT tests since the iCAT-enabled server\n");
            printNotice( "is on a remote host.  The iCAT tests can only be\n" );
            printNotice( "run locally.\n" );

        }

        # Pound test
#       printSubtitle( "\nTesting via concurrent-test (many iput/iget)...\n" );
#       doTestPound( );
        return(0);

        printNotice( "\nDone.\n" );

        setPrintVerbose( $verbosity );
}

#
# @brief        Test installation.
#
# Run the concurrent/load/pound tests.
#
sub doLoadTest
{
        # Always verbose during testing.
        my $verbosity = isPrintVerbose( );
        setPrintVerbose( 1 );

        printStatus(
"This test takes about 10 minutes and requires about 3 GB of space at\n" .
"the peak.  It will get and put files and set and check user-defined\n" .
"metadata, running multiple processes doing so at the same time.  This\n" .
"will put a substantial load on your computer and network (if the\n" .
"resource is remote).  It will use 'demoResc' which can be changed by\n" .
"editing clients/concurrent-test/ResourcesToTest.\n\n");

        printStatus( getCurrentDateTime( ) .
                     " Starting concurrent-tests\n");

        my $startDir = cwd( );
        chdir( $startDir . "/clients/concurrent-test" );
# old form: my $output = `./poundtests.sh 1 `;
#

# 100 MB files to trigger parallel threads:
        `./poundtests.sh 1 100000 >> /dev/tty`;
# small files to be quicker and use less space:
#       `./poundtests.sh 1 25000 >> /dev/tty`;

        if ( $? != 0 ) {
            printError( "Failed\n" );
        }
        else {
            printStatus( getCurrentDateTime( ) .
                         " All concurrent-tests were successful\n");
        }
        chdir( $startDir);

        setPrintVerbose( $verbosity );
}


# run the concurrent-test
sub doTestPound
{
        my $startDir = cwd( );
        chdir( $startDir . "/clients/concurrent-test" );
        my $output = `./poundtests.sh 1`;
        if ( $? != 0 ) {
            printError( "Failed\n" );
            $doTestExitValue++;
        }
        else {
            printStatus( "All concurrent-tests were successful.\n" );
        }
        chdir( $startDir);
}


#
# @brief        Test the irules.
#
# Test the irules in rules3.0
#
sub doTestIrules
{
        # Create input for the iCommands test.
        #       The test script asks for two items of user input:
        #               The path to the iCommands.
        #               The user password.
        #
        #       An empty answer for the first one assumes that the
        #       commands are on the PATH environment variable.
        #       Earlier, setupEnvironment() has done this.
        #
        #       The user's password is added now.  To keep things
        #       a bit secure, the temp file is chmod-ed to keep
        #       it closed.  It's deleted when the tests are done.
        my $program     = File::Spec->catfile( $icommandsTestDir, "testallrules.pl" );
        my $passwordTmp = File::Spec->catfile( $icommandsTestDir, "irods_test_$$.tmp" );

# Use the same technique to get the hostname as testiCommands.pl
# (instead of hostname( )) so that this name will be the same on all
# hosts (was a problem on NMI AIX (perhaps others)).
        my $hostname2 = hostname();
        if ( $hostname2 =~ '.' ) {
        @words = split( /\./, $hostname2 );
        $hostname2  = $words[0];
}
        my $logFile     = File::Spec->catfile( $icommandsTestDir, "testallrules_" . $hostname2 . ".log" );
        my $outputFile  = File::Spec->catfile( $icommandsTestDir, "testallrules_" . $hostname2 . ".txt" );

        printToFile( $passwordTmp, "\n$IRODS_ADMIN_PASSWORD\n" );
        chmod( 0600, $passwordTmp );


        # Run the irules test.
        #       The test writes a "testSurvey" to the current
        #       directory, along with some temp files.  To
        #       avoid cluttering up wherever this script lives,
        #       we move to a temp directory first and run the
        #       script there.
        my $startDir = cwd( );
        chdir( $icommandsTestDir );
        my $output = `$perl $program < $passwordTmp 2>&1`;
        unlink( $passwordTmp );
        chdir( $startDir );
        printToFile( $outputFile, $output );


        # Count failed tests:
        #       The log lists all successful and failed tests.
        #       Failures are too cryptic to repeat here, but we
        #       can count the failures and let the user know
        #       there's more information in the log file.
        my $failsFound = 0;
        my $lineCount = 0;
        open( LOG, "<$logFile" );
        foreach $line ( <LOG> )
        {
                if ( $line =~ /error code/ )
                {
                        ++$failsFound;
                }
                ++$lineCount;
        }
        close( LOG );

        if ( $failsFound )
        {
                printError( "$failsFound tests failed.  Check log files for details:\n" );
                printError( "    Log:     $logFile\n" );
                printError( "    Output:  $outputFile\n" );
                $doTestExitValue++;
        }
        else
        {
            if ( $lineCount ) {
                printStatus( "All testallrules.pl tests were successful.\n" );
                printStatus( "Check log files for details:\n" );
                printStatus( "    Log:     $logFile\n" );
                printStatus( "    Output:  $outputFile\n" );
            }
            else {
                printError( "Test failure.  Log file is empty\n" );
                printError( "    Log:     $logFile\n" );
                printError( "    Output:  $outputFile\n" );
                $doTestExitValue++;
            }
        }
}


#
# @brief        Test the iCommands.
#
# Test the iCommands.
#
sub doTestIcommands
{
        # Create input for the iCommands test.
        #       The test script asks for two items of user input:
        #               The path to the iCommands.
        #               The user password.
        #
        #       An empty answer for the first one assumes that the
        #       commands are on the PATH environment variable.
        #       Earlier, setupEnvironment() has done this.
        #
        #       The user's password is added now.  To keep things
        #       a bit secure, the temp file is chmod-ed to keep
        #       it closed.  It's deleted when the tests are done.
        my $program     = File::Spec->catfile( $icommandsTestDir, "testiCommands.pl" );
        my $username = $ENV{LOGNAME} || $ENV{USER} || getpwuid($<);
        my $tmpDir   = File::Spec->catdir( File::Spec->tmpdir( ), $username );
        mkpath($tmpDir,0,01777);
        my $passwordTmp = File::Spec->catfile( $tmpDir, "irods_test_$$.tmp" );

# Use the same technique to get the hostname as testiCommands.pl
# (instead of hostname( )) so that this name will be the same on all
# hosts (was a problem on NMI AIX (perhaps others)).
        my $hostname2 = hostname();
        if ( $hostname2 =~ '.' ) {
        @words = split( /\./, $hostname2 );
        $hostname2  = $words[0];
}
        my $logFile     = File::Spec->catfile( $tmpDir, "testSurvey_" . $hostname2 . ".log" );
        my $outputFile  = File::Spec->catfile( $tmpDir, "testSurvey_" . $hostname2 . ".txt" );

        printToFile( $passwordTmp, "\n$IRODS_ADMIN_PASSWORD\n" );
        chmod( 0600, $passwordTmp );


        # Run the iCommands test.
        #       The test writes a "testSurvey" to the current
        #       directory, along with some temp files.  To
        #       avoid cluttering up wherever this script lives,
        #       we move to a temp directory first and run the
        #       script there.
        my $startDir = cwd( );
        chdir( $tmpDir );
        my $output = `$perl $program < $passwordTmp 2>&1`;
        unlink( $passwordTmp );
        chdir( $startDir );
        printToFile( $outputFile, $output );


        # Count failed tests:
        #       The log lists all successful and failed tests.
        #       Failures are too cryptic to repeat here, but we
        #       can count the failures and let the user know
        #       there's more information in the log file.
        my $failsFound = 0;
        my $lineCount = 0;
        open( LOG, "<$logFile" );
        foreach $line ( <LOG> )
        {
                if ( $line =~ /error code/ )
                {
                        ++$failsFound;
                }
                ++$lineCount;
        }
        close( LOG );

        if ( $failsFound )
        {
                printError( "$failsFound tests failed.  Check log files for details:\n" );
                printError( "    Log:     $logFile\n" );
                printError( "    Output:  $outputFile\n" );
                $doTestExitValue++;
        }
        else
        {
            if ( $lineCount ) {
                printStatus( "All testiCommands.pl tests were successful.\n" );
                printStatus( "Check log files for details:\n" );
                printStatus( "    Log:     $logFile\n" );
                printStatus( "    Output:  $outputFile\n" );
            }
            else {
                printError( "Test failure.  Log file is empty\n" );
                printError( "    Log:     $logFile\n" );
                printError( "    Output:  $outputFile\n" );
                $doTestExitValue++;
            }
        }
}





#
# @brief        Test the iCAT.
#
# Test the iCAT queries.
#
sub doTestIcat
{
        # Update environment PATH for run-in-place tests
        $ENV{PATH} = "$ENV{PATH}:$scripttoplevel/iRODS/clients/icommands/bin";
        $ENV{PATH} = "$ENV{PATH}:$scripttoplevel/iRODS/server/test/bin";
        $ENV{PATH} = "$ENV{PATH}:$scripttoplevel/plugins/database";

        # Connect to iRODS.
        my $output  = `$iinit $IRODS_ADMIN_PASSWORD 2>&1`;
        if ( $? != 0 )
        {
                printError( "Cannot connect to iRODS.\n" );
                printError( "    ", $output );
                exit( 1 );
        }


        # Create a temporary file
        my $username = $ENV{LOGNAME} || $ENV{USER} || getpwuid($<);
        my $tmpDir   = File::Spec->catdir( File::Spec->tmpdir( ), $username );
        mkpath($tmpDir);
        my $outputFile   = File::Spec->catfile( $tmpDir, "icatSurvey_" . hostname( ) . ".txt" );

        # Enable CATSQL debug mode in Server by changing user's env file
        $homeDir=$ENV{'HOME'};
        $userEnvFile=$ENV{'IRODS_ENVIRONMENT_FILE'};
        if ($userEnvFile eq "") {
            $userEnvFile=  $homeDir . "/.irods/irods_environment.json";
        }
        $originalEnvText = `cat $userEnvFile`;
        #appendToFile( $userEnvFile,
        #             "\nirods_debug CATSQL\n" );

        my %debug_val = ( "irods_debug", "CATSQL" );
    $status = update_json_configuration_file( $userEnvFile, %debug_val );


        # Run tests
        #       While the output could be put anywhere, the 'checkIcatLog'
        #       script presumes the output is in the same directory as the
        #       test scripts.  So we have to put it there.
        my $icatTestLog = File::Spec->catfile( $serverTestBinDir, "icatTest.log" );
        my $icatMiscTestLog = File::Spec->catfile( $serverTestBinDir, "icatMiscTest.log" );
        my $moveTestLog = File::Spec->catfile( $serverTestBinDir, "moveTest.log" );
        my $icatTicketTestLog = File::Spec->catfile( $serverTestBinDir, "icatTicketTest.log" );
        my $quotaTestLog = File::Spec->catfile( $serverTestBinDir, "quotaTest.log" );
        my $specNameTestLog = File::Spec->catfile( $serverTestBinDir, "specNameTest.log" );

        my $startDir = cwd( );
        chdir( $serverTestBinDir );
        $icatFailure=0;
        $output = `$perl icatTest.pl 2>&1`;
  $icatTestStatus="OK";
        if ( $? != 0 )
  {
    $icatFailure=1;
    $icatTestStatus="FAILURE";
  }
        printToFile( $icatTestLog, $output);

        $output = `$perl icatMiscTest.pl 2>&1`;
  $icatMiscTestStatus="OK";
        if ( $? != 0 )
  {
    $icatFailure=1;
    $icatMiscTestStatus="FAILURE";
  }
        printToFile( $icatMiscTestLog, $output );

        $output=`$perl moveTest.pl 2>&1`;
  $moveTestStatus="OK";
        if ( $? != 0 )
  {
    $icatFailure=1;
    $moveTestStatus="FAILURE";
  }
        printToFile( $moveTestLog, $output);

        $output = `$perl icatTicketTest.pl 2>&1`;
  $icatTicketTestStatus="OK";
        if ( $? != 0 )
  {
    $icatFailure=1;
    $icatTicketTestStatus="FAILURE";
  }
        printToFile( $icatTicketTestLog, $output );

        $output = `$perl quotaTest.pl 2>&1`;
  $quotaTestStatus="OK";
        if ( $? != 0 )
  {
    $icatFailure=1;
    $quotaTestStatus="FAILURE";
  }
        printToFile( $quotaTestLog, $output );

        $output = `$perl specNameTest.pl 2>&1`;
  $specNameTestStatus="OK";
        if ( $? != 0 )
  {
    $icatFailure=1;
    $specNameTestStatus="FAILURE";
  }
        printToFile( $specNameTestLog, $output );

        # Restore the iRODS environment
        printToFile("$userEnvFile", $originalEnvText);

        chdir( $startDir );

        if ($icatFailure) {
            printError( "One or more iCAT tests failed.\n" );
            $doTestExitValue++;
        }
        else {
            printStatus("All iCAT tests were successful.\n");
            # clean up after iCAT tests - they don't clean up the Vault themselves
            system("rm -rf $IRODS_HOME/Vault/home/rods/TestFile*");
        }
        printStatus( "Check log files for details:\n" );
        printStatus( "    Logs:     $icatTestLog        $icatTestStatus\n" );
        printStatus( "              $icatMiscTestLog    $icatMiscTestStatus\n" );
        printStatus( "              $moveTestLog        $moveTestStatus\n" );
        printStatus( "              $icatTicketTestLog  $icatTicketTestStatus\n" );
        printStatus( "              $quotaTestLog       $quotaTestStatus\n" );
        printStatus( "              $specNameTestLog    $specNameTestStatus\n" );
}




########################################################################
#
# Functions
#

#
# @brief        Set environment variables for running commands.
#
# The command and library paths are set to point to database and
# iRODS commands.  Environment variables for the database are set
# to indicate it's host, port, etc.
#
sub setupEnvironment
{
        # Execution path.  Add ".", iRODS commands, and database commands
        my $oldPath = $ENV{'PATH'};
        my $newPath = ".:$icommandsBinDir";
        $ENV{'PATH'} = "$newPath:$oldPath";

}





#
# @brief        Start iRODS server
#
# @return       0 = failed
#               1 = started
#
sub startIrods
{
        # Make sure the server is available
        my $status;
        if ( ! -e $irodsServer )
        {
                printError( "Configuration problem:\n" );
                printError( "    The 'irodsServer' application could not be found.  Have the\n" );
                printError( "    iRODS servers been compiled?\n" );
                exit( 1 );
        }
        if ( ! -e $irodsLogDir )
        {
                printError( "Configuration problem:\n" );
                printError( "    The server log directory, $irodsLogDir\n" );
                printError( "    does not exist.  Please create it, make it writable, and retry.\n" );
                exit( 1 );
        }
        if ( ! -w $irodsLogDir )
        {
                printError( "Configuration problem:\n" );
                printError( "    The server log directory, $irodsLogDir\n" );
                printError( "    is not writable.  Please chmod it and retry.\n" );
                exit( 1 );
        }

        # Test for iRODS port in use
        my $portTestLimit = 5;
        my $waitSeconds = 1;
        while ($waitSeconds < $portTestLimit){
                my $porttest = `netstat -tlpen 2> /dev/null | grep ":$IRODS_PORT" | awk '{print \$9}'`;
                chomp($porttest);
                if ($porttest ne ""){
                        print("($waitSeconds) Waiting for process bound to port $IRODS_PORT ... [$porttest]\n");
                        sleep($waitSeconds);
                        $waitSeconds = $waitSeconds * 2;
                }
                else{
                        last;
                }
        }
        if ($waitSeconds >= $portTestLimit){
                printError("Port $IRODS_PORT In Use ... Not Starting iRODS Server\n");
                exit( 1 );
        }


        # Prepare
        my $startingDir = cwd( );
        chdir( $serverBinDir );
        umask( 077 );
        # Start the server
        my $syslogStat = `grep IRODS_SYSLOG $configDir/config.mk 2> /dev/null | grep -v \'#'`;
        if ($syslogStat) {
#           syslog enabled, need to start differently
            my $status = system("$irodsServer&");
        }
        else {
            my $output = `$irodsServer 2>&1`;
            my $status = $?;
        }
        if ( $status )
        {
                printError( "iRODS server failed to start\n" );
                printError( "    $output\n" );
                return 0;
        }

        chdir( $startingDir );

        # Sleep a bit to give the server time to start and possibly exit
        sleep( $iRODSStartStopDelay );

        # Check that it actually started
        my %serverPids = getIrodsProcessIds( );
        if ( (scalar keys %serverPids) == 0 )
        {
                printError( "iRODS server failed to start.\n" );
                return 0;
        }

        return 1;
}





#
# @brief        Stop iRODS server
#
# This function lists all processes that appear to be iRODS servers,
# then tries to kill them.  This can fail if the processes are hung
# and refuse to die, or if they are owned by somebody else.
#
# @return       0 = failed
#               1 = stopped or nothing to stop
#
sub stopIrods
{
        # Find and kill the server process IDs
        my @pids = getOurIrodsServerPids();
        my $found = 0;
        $num = @pids;
        print ( "Found $num processes:\n" );
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
        my @pids = getOurIrodsServerPids();
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
        # remove shared memory mutex and semaphore
        # this will be handled more cleanly when servers can be gracefully shutdown
        system( "rm -f /var/run/shm/*re_cache_*iRODS*" ); # ubuntu
        system( "rm -f /dev/shm/*re_cache_*iRODS*" );     # centos, suse, arch

        # Report if there are any left.
        my $didNotDie = 0;
        my @pids = getOurIrodsServerPids();

        if ( $#pids >= 0 )
        {
                printError( "    Some servers could not be killed.  They may be owned\n" );
                printError( "    by another user or there could be a problem.\n" );
                return 0;
        }
        return 1;
}


#
# @brief        Get Our iRODS Server Process IDs
#
# Find the server processes that are part of this installation.
# Use the state file to find the process ID of the server and
# find all it's children.
#
# @return       0 = failed
#               1 = started
#               2 = already started
#               3 = not under our control
#
sub getOurIrodsServerPids
{
    my $tmpDir="/usr/tmp";
    if (!-e $tmpDir)  {
        $tmpDir="/tmp";
    }
    my $processFile   = $tmpDir . "/irodsServer" . "." . $IRODS_PORT;
    open( PIDFILE, "<$processFile" );
    my $line;
    my $parentPid;
    foreach $line (<PIDFILE>)
    {
        my $i = index($line, " ");
        $parentPid=substr($line,0,$i);
    }
    close( PIDFILE );

    my @serverPids = getFamilyProcessIds( $parentPid );

    return @serverPids;
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
        my %serverPids = ();
        foreach $serverType (keys %servers)
        {
                my $processName = $servers{$serverType};
                my @pids = getProcessIds( $processName );
                next if ( $#pids < 0 );
                $serverPids{$serverType} = [ @pids ];
        }
        return %serverPids;
}



#
# @brief        Print command-line help
#
sub printUsage
{
        my $oldVerbosity = isPrintVerbose( );
        setPrintVerbose( 1 );

        printNotice( "This script controls the iRODS servers.\n" );
        printNotice( "\n" );
        printNotice( "Usage is:\n" );
        printNotice( "    $scriptName [options] [commands]\n" );
        printNotice( "\n" );
        printNotice( "Help options:\n" );
        printNotice( "    --help        Show this help information\n" );
        printNotice( "\n" );
        printNotice( "Verbosity options:\n" );
        printNotice( "    --quiet       Suppress all messages\n" );
        printNotice( "    --verbose     Output all messages (default)\n" );
        printNotice( "\n" );
        printNotice( "General Commands:\n" );
        printNotice( "    start         Start the iRODS servers\n" );
        printNotice( "    stop          Stop the iRODS servers\n" );
        printNotice( "    restart       Restart the iRODS servers\n" );
        printNotice( "    status        Show the status of iRODS servers\n" );
        printNotice( "    devtest       Run a developer test suite\n" );
        printNotice( "    loadtest      Run a concurrency (load/pound) test suite\n" );

        setPrintVerbose( $oldVerbosity );
}
