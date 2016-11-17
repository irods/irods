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
#include <syslog.h>
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

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>
using namespace boost::filesystem;

time_t LastRescUpdateTime;
char *CurLogfileName = NULL;    /* the path of the current logfile */
static time_t LogfileLastChkTime = 0;

void
cleanupAndExit( int status ) {
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

void
signalExit( int ) {
#if 0
    rodsLog( LOG_NOTICE,
             "caught a signal and exiting\n" );
#endif
    cleanupAndExit( SYS_CAUGHT_SIGNAL );
}

char *
getLogDir() {
    char *myDir;

    if ( ( myDir = ( char * ) getenv( "irodsLogDir" ) ) != ( char * ) NULL ) {
        return myDir;
    }
    return DEF_LOG_DIR;
}

int get_log_file_rotation_time() {
    char* rotation_time_str = getenv(LOGFILE_INT);
    if ( rotation_time_str ) {
        const int rotation_time = atoi(rotation_time_str);
        if( rotation_time > 1 ) {
            return rotation_time;
        }
    }

    try {
        const int rotation_time = irods::get_advanced_setting<const int>(irods::DEFAULT_LOG_ROTATION_IN_DAYS);
        if(rotation_time > 1) {
            return rotation_time;
        }

    }
    catch( irods::exception& _e ) {
    }

    return DEF_LOGFILE_INT;

} // get_log_file_rotation_time

void
getLogfileName( char **logFile, const char *logDir, const char *logFileName ) {
    time_t myTime;
    struct tm *mytm;
    char *logfilePattern; // JMC - backport 4793
    //char *logfileIntStr;
    //int logfileInt;
    int tm_mday = 1;
    char logfileSuffix[MAX_NAME_LEN]; // JMC - backport 4793
    char myLogDir[MAX_NAME_LEN];

    /* Put together the full pathname of the logFile */

    if ( logDir == NULL ) {
        snprintf( myLogDir, MAX_NAME_LEN, "%-s", getLogDir() );
    }
    else {
        snprintf( myLogDir, MAX_NAME_LEN, "%-s", logDir );
    }
    *logFile = ( char * ) malloc( strlen( myLogDir ) + strlen( logFileName ) + 24 );

    LogfileLastChkTime = myTime = time( 0 );
    mytm = localtime( &myTime );
    const int rotation_time = get_log_file_rotation_time(); /*
    if ( ( logfileIntStr = getenv( LOGFILE_INT ) ) == NULL ||
            ( logfileInt = atoi( logfileIntStr ) ) < 1 ) {
        logfileInt = DEF_LOGFILE_INT;
    }*/

    tm_mday = ( mytm->tm_mday / rotation_time ) * rotation_time + 1;
    if ( tm_mday > mytm->tm_mday ) {
        tm_mday -= rotation_time;
    }
    // =-=-=-=-=-=-=-
    // JMC - backport 4793
    if ( ( logfilePattern = getenv( LOGFILE_PATTERN ) ) == NULL ) {
        logfilePattern = DEF_LOGFILE_PATTERN;
    }
    mytm->tm_mday = tm_mday;
    strftime( logfileSuffix, MAX_NAME_LEN, logfilePattern, mytm );
    sprintf( *logFile, "%-s/%-s.%-s", myLogDir, logFileName, logfileSuffix );
    // =-=-=-=-=-=-=-
}

int
logFileOpen( int runMode, const char *logDir, const char *logFileName ) {
    char *logFile = NULL;
#ifdef SYSLOG
    int logFd = 0;
#else
    int logFd;
#endif

    if ( runMode == SINGLE_PASS && logDir == NULL ) {
        return 1;
    }

    if ( logFileName == NULL ) {
        fprintf( stderr, "logFileOpen: NULL input logFileName\n" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    getLogfileName( &logFile, logDir, logFileName );
    if ( NULL == logFile ) { // JMC cppcheck - nullptr
        fprintf( stderr, "logFileOpen: unable to open log file" );
        return -1;
    }

#ifndef SYSLOG
    logFd = open( logFile, O_CREAT | O_WRONLY | O_APPEND, 0666 );
#endif
    if ( logFd < 0 ) {
        fprintf( stderr, "logFileOpen: Unable to open %s. errno = %d\n",
                 logFile, errno );
        free( logFile );
        return -1 * errno;
    }

    free( logFile );
    return logFd;
}

void
daemonize( int runMode, int logFd ) {
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

    close( 0 );
    close( 1 );
    close( 2 );

    ( void ) dup2( logFd, 0 );
    ( void ) dup2( logFd, 1 );
    ( void ) dup2( logFd, 2 );
    close( logFd );
}

extern int msiAdmClearAppRuleStruct( ruleExecInfo_t *rei );

int usage( char *prog );

int
main( int argc, char **argv ) {
    int status;
    int c;
    int runMode = SERVER;
    int flagval = 0;
    char *logDir = NULL;
    char *tmpStr;
    int logFd;
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
#ifdef SYSLOG
        int j = atoi( tmpStr );
        rodsLogSqlReq( j );
#else
        rodsLogSqlReq( 1 );
#endif
    }

    /* Set the logging level */
    tmpStr = getenv( SP_LOG_LEVEL );
    if ( tmpStr != NULL ) {
        int i;
        i = atoi( tmpStr );
        rodsLogLevel( i );
    }
    else {
        rodsLogLevel( LOG_NOTICE ); /* default */
    }

#ifdef SYSLOG
    /* Open a connection to syslog */
    openlog( "rodsReServer", LOG_ODELAY | LOG_PID, LOG_DAEMON );
#endif

    while ( ( c = getopt( argc, argv, "sSvD:j:t:" ) ) != EOF ) {
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
        case 'D':   /* user specified a log directory */
            logDir = strdup( optarg );
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

    if ( ( logFd = logFileOpen( runMode, logDir, RULE_EXEC_LOGFILE ) ) < 0 ) {
        return 1;
    }

    daemonize( runMode, logFd );

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
        reServerMain( logDir );
    }

    return 0;
}

int usage( char *prog ) {
    fprintf( stderr, "Usage: %s [-sSv] [j jobID] [-t jobType] [-D logDir] \n", prog );
    fprintf( stderr, "-s   Run like a client process - no fork\n" );
    fprintf( stderr, "-S   Run like a daemon server process - forked\n" );
    fprintf( stderr, "-j jobID   Run a single job\n" );
    fprintf( stderr, "-t jobType An integer for job type. \n" );
    return 0;
}

int
chkLogfileName( const char *logDir, const char *logFileName ) {
    time_t myTime;
    char *logFile = NULL;
    int i;

    myTime = time( 0 );
    if ( myTime < LogfileLastChkTime + LOGFILE_CHK_INT ) {
        /* not time yet */
        return 0;
    }

    getLogfileName( &logFile, logDir, logFileName );

    if ( CurLogfileName != NULL && strcmp( CurLogfileName, logFile ) == 0 ) {
        free( logFile );
        return 0;
    }

    /* open the logfile */

    if ( ( i = open( logFile, O_CREAT | O_RDWR, 0644 ) ) < 0 ) {
        fprintf( stderr, "Unable to open logFile %s\n", logFile );
        free( logFile );
        return -1;
    }
    else {
        lseek( i, 0, SEEK_END );
    }

    if ( CurLogfileName != NULL ) {
        free( CurLogfileName );
    }

    CurLogfileName = logFile;

    close( 0 );
    close( 1 );
    close( 2 );
    ( void ) dup2( i, 0 );
    ( void ) dup2( i, 1 );
    ( void ) dup2( i, 2 );
    ( void ) close( i );

    return 0;
}



void
reServerMain( char* logDir ) {

    genQueryOut_t *genQueryOut = NULL;
    time_t endTime;
    int runCnt;
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
                  rodsSleep(1, 0);
                  continue;
              }

            int status = clientLogin( rc_comm );
            if (status < 0) {
                rodsLog(LOG_ERROR, "clientLogin failed %d", status);
                rodsSleep(1, 0);
                continue;
            }

#ifndef SYSLOG
            chkLogfileName( logDir, RULE_EXEC_LOGFILE );
#endif
            rodsLog(
                LOG_DEBUG,
                "reServerMain: checking the queue for jobs" );
            status = getReInfo( rc_comm, &genQueryOut );
            if ( status < 0 ) {
                if ( status != CAT_NO_ROWS_FOUND ) {
                    rodsLog( LOG_ERROR,
                             "reServerMain: getReInfo error. status = %d", status );
                }
                else {   // JMC - backport 4520
                    repeatedQueryErrorCount++;
                }
                rcDisconnect( rc_comm );
                reSvrSleep( );
                continue;
            }
            else {   // JMC - backport 4520
                repeatedQueryErrorCount = 0;
            }

            endTime = time( NULL ) + RE_SERVER_EXEC_TIME;
            runCnt = runQueuedRuleExec( rc_comm, &reExec, &genQueryOut, endTime, 0 );

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
            if ( runCnt > 0 ||
                    ( genQueryOut != NULL && genQueryOut->continueInx > 0 ) ) {
                rcDisconnect( rc_comm );
                continue;
            }
            else {
                /* nothing got run */
                reSvrSleep( );
            }

            rcDisconnect( rc_comm );

        } // while

        rodsLog(
            LOG_NOTICE,
            "rule engine is exiting" );

    }
    catch ( irods::exception& e_ ) {
        std::cerr << e_.what() << std::endl;
        return;
    }

} // reServerMain

int reSvrSleep( ) {
    int re_sleep_time;
    try {
        re_sleep_time = irods::get_advanced_setting<const int>(irods::CFG_RE_SERVER_SLEEP_TIME);
    } catch ( const irods::exception& e ) {
        irods::log(e);
        re_sleep_time = RE_SERVER_SLEEP_TIME;
    }

    rodsSleep( re_sleep_time, 0 );

    return 0;
}
