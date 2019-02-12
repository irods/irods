/*-------------------------------------------------------------------------
 *
 * irodsReServer.cpp -- The iRODS Rule Execution server
 *
 *
 *-------------------------------------------------------------------------
 */

#include "irodsReServer.hpp"
#include "reServerLib.hpp"
#include "rsApiHandler.hpp"
#include "rsIcatOpr.hpp"
#include "miscServerFunct.hpp"
#include "reconstants.hpp"
#include "initServer.hpp"
#include "irods_server_state.hpp"
#include "irods_exception.hpp"
#include "irods_server_properties.hpp"
#include "objMetaOpr.hpp"
#include "genQuery.h"
#include "getRodsEnv.h"

// =-=-=-=-=-=-=-
// irods includes
#include "irods_get_full_path_for_config_file.hpp"
#include "irods_re_plugin.hpp"
#include "irods_logger.hpp"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>
using namespace boost::filesystem;

time_t LastRescUpdateTime;

void cleanupAndExit( int status )
{
#if 0
    rodsLog( LOG_NOTICE,
             "Agent exiting with status = %d", status );
#endif

    if ( status >= 0 ) {
        exit( 0 );
    }
    else {
        exit( 1 );
    }
}

void signalExit( int )
{
#if 0
    rodsLog( LOG_NOTICE,
             "caught a signal and exiting\n" );
#endif
    cleanupAndExit( SYS_CAUGHT_SIGNAL );
}

void daemonize(int runMode)
{
    if ( runMode == SINGLE_PASS ) {
        return;
    }

    if ( runMode == STANDALONE_SERVER ) {
        if ( fork() ) {
            exit( 0 );
        }

        if ( setsid() < 0 ) {
            fprintf( stderr, "daemonize" );
            perror( "cannot create a new session." );
            exit( 1 );
        }
    }

    close(0);
    close(1);
    close(2);

    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_RDWR);
}

extern int msiAdmClearAppRuleStruct( ruleExecInfo_t *rei );

int usage( char *prog );

namespace
{
    void init_logger()
    {
        using log = irods::experimental::log;

        log::init();
        irods::server_properties::instance().capture();
        log::server::set_level(log::get_level_from_config(irods::CFG_LOG_LEVEL_CATEGORY_SERVER_KW));
        log::set_server_type("delay_server");

        if (char hostname[HOST_NAME_MAX]{}; gethostname(hostname, sizeof(hostname)) == 0) {
            log::set_server_host(hostname);
        }
        else {
            // TODO Handle error.
        }
    }
} // anonymous namespace

int main( int argc, char **argv )
{
    using log = irods::experimental::log;

    init_logger();

    log::server::info("Initializing ...");

    int status;
    int c;
    int runMode = SERVER;
    int flagval = 0;
    char *tmpStr;
    char *ruleExecId = NULL;
    int jobType = 0;

    ProcessType = CLIENT_PT;

    signal( SIGINT, signalExit );
    signal( SIGHUP, signalExit );
    signal( SIGTERM, signalExit );
    signal( SIGUSR1, signalExit );
    signal( SIGPIPE, signalExit );
    /* XXXXX switched to SIG_DFL for embedded python. child process
     * went away. But probably have to call waitpid.
     * signal(SIGCHLD, SIG_IGN); */
    signal( SIGCHLD, SIG_DFL );

    /* Handle option to log sql commands */
    tmpStr = getenv( SP_LOG_SQL );
    if ( tmpStr != NULL ) {
        rodsLogSqlReq( 1 );
    }

    /* Set the logging level */
    tmpStr = getenv( SP_LOG_LEVEL );
    if ( tmpStr != NULL ) {
        rodsLogLevel( atoi( tmpStr ) );
    }
    else {
        rodsLogLevel( LOG_NOTICE ); /* default */
    }

    while ( ( c = getopt( argc, argv, "sSv:j:t:" ) ) != EOF ) {
        switch ( c ) {
        case 's':
            runMode = SINGLE_PASS;
            break;
        case 'S':
            runMode = STANDALONE_SERVER;
            break;
        case 'v':   /* verbose */
            flagval |= v_FLAG;
            break;
        case 'j':
            runMode = SINGLE_PASS;
            ruleExecId = strdup( optarg );
            break;
        case 't':
            jobType = atoi( optarg );
            break;
        default:
            usage( argv[0] );
            return 1;
        }
    }

    daemonize(runMode);

    if ( ruleExecId != NULL ) {
        status = reServerSingleExec( ruleExecId, jobType );
        if ( status >= 0 ) {
            return 0;
        }
        else {
            return 1;
        }
    }
    else {
        reServerMain();
    }

    return 0;
}

int usage( char *prog ) {
    fprintf( stderr, "Usage: %s [-sSv] [j jobID] [-t jobType] \n", prog );
    fprintf( stderr, "-s   Run like a client process - no fork\n" );
    fprintf( stderr, "-S   Run like a daemon server process - forked\n" );
    fprintf( stderr, "-j jobID   Run a single job\n" );
    fprintf( stderr, "-t jobType An integer for job type. \n" );
    return 0;
}

void reServerMain()
{
    using log = irods::experimental::log;

    log::server::info("Entering while loop ...");

    genQueryOut_t *genQueryOut = NULL;
    reExec_t reExec;
    int repeatedQueryErrorCount = 0;

    initReExec( &reExec );
    LastRescUpdateTime = time( NULL );

    int re_exec_time;
    try {
        re_exec_time = irods::get_advanced_setting<const int>(irods::CFG_RE_SERVER_EXEC_TIME);
    } catch ( const irods::exception& e ) {
        irods::log(e);
        re_exec_time = RE_SERVER_EXEC_TIME;
    }

    try {
        while ( true ) {
            rodsEnv env;
            _reloadRodsEnv(env);
            rErrMsg_t rcConnect_error_msg;
            memset(&rcConnect_error_msg, 0, sizeof(rcConnect_error_msg));
            rcComm_t* rc_comm = rcConnect(
                      env.rodsHost,
                      env.rodsPort,
                      env.rodsUserName,
                      env.rodsZone,
                      NO_RECONN, &rcConnect_error_msg );
            if (!rc_comm) {
                rodsLog(LOG_ERROR, "rcConnect failed %ji, %s", static_cast<intmax_t>(rcConnect_error_msg.status), rcConnect_error_msg.msg);
                reSvrSleep();
                continue;
            }

            int status = clientLogin( rc_comm );
            if (status < 0) {
                const int status_rcDisconnect = rcDisconnect(rc_comm);
                if (status_rcDisconnect < 0) {
                    rodsLog(LOG_ERROR, "reServerMain: rcDisconnect failed [%d]", status_rcDisconnect);
                }
                rodsLog(LOG_ERROR, "clientLogin failed %d", status);
                reSvrSleep();
                continue;
            }

            status = getReInfo( rc_comm, &genQueryOut );
            if ( status < 0 ) {
                if ( status != CAT_NO_ROWS_FOUND ) {
                    rodsLog(LOG_ERROR, "reServerMain: getReInfo error. status = %d", status );
                } else {   // JMC - backport 4520
                    repeatedQueryErrorCount++;
                }
                rcDisconnect( rc_comm );
                reSvrSleep( );
                continue;
            }
            else {   // JMC - backport 4520
                repeatedQueryErrorCount = 0;
            }

            const time_t endTime = time( NULL ) + re_exec_time;
            int runCnt = runQueuedRuleExec( rc_comm, &reExec, &genQueryOut, endTime, 0 );

            if ( runCnt > 0 ||
                    ( genQueryOut != NULL && genQueryOut->continueInx > 0 ) ) {
                /* need to refresh */
                closeQueryOut( rc_comm, genQueryOut );
                freeGenQueryOut( &genQueryOut );
                status = getReInfo( rc_comm, &genQueryOut );
                if ( status < 0 ) {
                    rcDisconnect( rc_comm );
                    reSvrSleep( );
                    continue;
                }
            }

            /* run the failed job */
            runCnt = runQueuedRuleExec(
                         rc_comm,
                         &reExec,
                         &genQueryOut,
                         endTime,
                         RE_FAILED_STATUS );
            closeQueryOut( rc_comm, genQueryOut );
            freeGenQueryOut( &genQueryOut );
            if ( runCnt > 0 || ( genQueryOut != NULL && genQueryOut->continueInx > 0 ) ) {
                rcDisconnect( rc_comm );
                reSvrSleep();
                continue;
            } else {
                /* nothing got run */
                reSvrSleep( );
            }

            rcDisconnect( rc_comm );
        } // while

        rodsLog(LOG_NOTICE, "rule engine is exiting");
    }
    catch (const irods::exception& e_) {
        rodsLog(LOG_ERROR, "rule engine is exiting because of irods::exception");
        std::cerr << e_.what() << std::endl;
        return;
    }
} // reServerMain

int reSvrSleep( ) {
    int re_sleep_time;

    try {
        re_sleep_time = irods::get_advanced_setting<const int>(irods::CFG_RE_SERVER_SLEEP_TIME);
    }
    catch ( const irods::exception& e ) {
        irods::log(e);
        re_sleep_time = RE_SERVER_SLEEP_TIME;
    }

    rodsSleep( re_sleep_time, 0 );

    return 0;
}
