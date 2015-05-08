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
#include "rsGlobal.hpp"
#include "rsIcatOpr.hpp"
#include <syslog.h>
#include "miscServerFunct.hpp"
#include "reconstants.hpp"
#include "initServer.hpp"
#include "irods_server_state.hpp"
#include "irods_exception.hpp"
#include "irods_server_properties.hpp"
#include "readServerConfig.hpp"
#include "filesystem.hpp"


//#include "irods_server_control_plane.hpp"

// =-=-=-=-=-=-=-
// irods includes
#include "irods_get_full_path_for_config_file.hpp"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>
using namespace boost::filesystem;


int getAgentProcCnt() {
    return 0;
}

int getAgentProcPIDs(
    std::vector<int>& _pids ) {
    _pids.clear();
    return 0;
}

extern int msiAdmClearAppRuleStruct( ruleExecInfo_t *rei );

int usage( char *prog );

int
main( int argc, char **argv ) {
    int status;
    int c;
    rsComm_t rsComm;
    int runMode = SERVER;
    int flagval = 0;
    char *logDir = NULL;
    char *tmpStr;
    int logFd;
    char *ruleExecId = NULL;
    int jobType = 0;

    ProcessType = RE_SERVER_PT;

#ifndef _WIN32
    signal( SIGINT, signalExit );
    signal( SIGHUP, signalExit );
    signal( SIGTERM, signalExit );
    signal( SIGUSR1, signalExit );
    signal( SIGPIPE, rsPipeSignalHandler );
    /* XXXXX switched to SIG_DFL for embedded python. child process
     * went away. But probably have to call waitpid.
     * signal(SIGCHLD, SIG_IGN); */
    signal( SIGCHLD, SIG_DFL );
#endif

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
            exit( 1 );
        }
    }

    status = initRsComm( &rsComm );

    if ( status < 0 ) {
        cleanupAndExit( status );
    }

    if ( ( logFd = logFileOpen( runMode, logDir, RULE_EXEC_LOGFILE ) ) < 0 ) {
        exit( 1 );
    }

    daemonize( runMode, logFd );

    irods::error ret = setRECacheSaltFromEnv();
    if ( !ret.ok() ) {
        rodsLog( LOG_ERROR, "irodsReServer::main: Failed to set RE cache mutex name\n%s", ret.result().c_str() );
        exit( 1 );
    }

    status = initAgent( RULE_ENGINE_INIT_CACHE, &rsComm );
    if ( status < 0 ) {
        cleanupAndExit( status );
    }

    if ( ruleExecId != NULL ) {
        status = reServerSingleExec( &rsComm, ruleExecId, jobType );
        if ( status >= 0 ) {
            exit( 0 );
        }
        else {
            exit( 1 );
        }
    }
    else {
        reServerMain( &rsComm, logDir );
    }
    cleanupAndExit( status );

    exit( 0 );
}

int usage( char *prog ) {
    fprintf( stderr, "Usage: %s [-sSv] [j jobID] [-t jobType] [-D logDir] \n", prog );
    fprintf( stderr, "-s   Run like a client process - no fork\n" );
    fprintf( stderr, "-S   Run like a daemon server process - forked\n" );
    fprintf( stderr, "-j jobID   Run a single job\n" );
    fprintf( stderr, "-t jobType An integer for job type. \n" );
    return 0;
}

void
reServerMain( rsComm_t *rsComm, char* logDir ) {
    int status = 0;
    genQueryOut_t *genQueryOut = NULL;
    time_t endTime;
    int runCnt;
    reExec_t reExec;
    int repeatedQueryErrorCount = 0; // JMC - backport 4520

    initReExec( rsComm, &reExec );
    LastRescUpdateTime = time( NULL );

    try {
        while ( true ) { 

#ifndef windows_platform
#ifndef SYSLOG
            chkLogfileName( logDir, RULE_EXEC_LOGFILE );
#endif
#endif
            rodsLog(
                LOG_NOTICE,
                "reServerMain: checking the queue for jobs" );
            chkAndResetRule();
            status = getReInfo( rsComm, &genQueryOut );
            if ( status < 0 ) {
                if ( status != CAT_NO_ROWS_FOUND ) {
                    rodsLog( LOG_ERROR,
                             "reServerMain: getReInfo error. status = %d", status );
                }
                else {   // JMC - backport 4520
                    repeatedQueryErrorCount++;
                }
                reSvrSleep( rsComm );
                continue;
            }
            else {   // JMC - backport 4520
                repeatedQueryErrorCount = 0;
            }
            endTime = time( NULL ) + RE_SERVER_EXEC_TIME;
            runCnt = runQueuedRuleExec( rsComm, &reExec, &genQueryOut, endTime, 0 );
            if ( runCnt > 0 ||
                    ( genQueryOut != NULL && genQueryOut->continueInx > 0 ) ) {
                /* need to refresh */
                svrCloseQueryOut( rsComm, genQueryOut );
                freeGenQueryOut( &genQueryOut );
                status = getReInfo( rsComm, &genQueryOut );
                if ( status < 0 ) {
                    reSvrSleep( rsComm );
                    continue;
                }
            }

            /* run the failed job */

            runCnt =
                runQueuedRuleExec( rsComm, &reExec, &genQueryOut, endTime,
                                   RE_FAILED_STATUS );
            svrCloseQueryOut( rsComm, genQueryOut );
            freeGenQueryOut( &genQueryOut );
            if ( runCnt > 0 ||
                    ( genQueryOut != NULL && genQueryOut->continueInx > 0 ) ) {
                continue;
            }
            else {
                /* nothing got run */
                reSvrSleep( rsComm );
            }
        }

        rodsLog(
            LOG_NOTICE,
            "rule engine is exiting" );

    }
    catch ( irods::exception& e_ ) {
        const char* what = e_.what();
        std::cerr << what << std::endl;
        return;

    }

}

int
reSvrSleep( rsComm_t *rsComm ) {
    rodsServerHost_t *rodsServerHost = NULL;
    irods::server_properties& props = irods::server_properties::getInstance();
    irods::error ret = props.capture_if_needed();
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    std::string zone_name;
    ret = props.get_property <
          std::string > (
              irods::CFG_ZONE_NAME,
              zone_name );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();

    }

    int status = disconnRcatHost( MASTER_RCAT, zone_name.c_str() );
    if ( status == LOCAL_HOST ) {
#ifdef RODS_CAT
        status = disconnectRcat();
        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "reSvrSleep: disconnectRcat error. status = %d", status );
        }
#endif
    }
    rodsSleep( RE_SERVER_SLEEP_TIME, 0 );

    status = getAndConnRcatHost( rsComm, MASTER_RCAT, zone_name.c_str(), &rodsServerHost );
    if ( status == LOCAL_HOST ) {
#ifdef RODS_CAT
        status = connectRcat();
        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "reSvrSleep: connectRcat error. status = %d", status );
        }
#endif
    }
    return status;
}

irods::error capture_rulesets(
    std::string& _res,
    std::string& _fnm,
    std::string& _dvm ) {
    typedef irods::configuration_parser::array_t  array_t;

    irods::server_properties& props = irods::server_properties::getInstance();
    irods::error ret = props.capture();

    array_t prop_arr;
    ret = props.get_property <
          array_t > (
              irods::CFG_RE_RULEBASE_SET_KW,
              prop_arr );

    std::string prop_str;
    if ( ret.ok() ) {
        for ( size_t i = 0;
                i < prop_arr.size();
                ++i ) {
            try {
                _res += boost::any_cast< std::string >(
                                prop_arr[i][ irods::CFG_FILENAME_KW ] );
            } catch ( boost::bad_any_cast& _e ) {
                rodsLog(
                    LOG_ERROR,
                    "failed to cast rule base file name entry to string" );
                continue;
            }
            _res += ",";
        }

        _res = _res.substr( 0, _res.size() - 1 );

    }

    ret = props.get_property <
          array_t > (
              irods::CFG_RE_FUNCTION_NAME_MAPPING_SET_KW,
              prop_arr );
    if ( ret.ok() ) {
        for ( size_t i = 0;
                i < prop_arr.size();
                ++i ) {
            try {
                _fnm += boost::any_cast< std::string >(
                                prop_arr[i][ irods::CFG_FILENAME_KW ] );
            } catch ( boost::bad_any_cast& _e ) {
                rodsLog(
                    LOG_ERROR,
                    "failed to cast rule function file name entry to string" );
                continue;
            }
            _fnm += ",";
        }

        _fnm = _fnm.substr( 0, _fnm.size() - 1 );

    }

    ret = props.get_property <
          array_t > (
              irods::CFG_RE_DATA_VARIABLE_MAPPING_SET_KW,
              prop_arr );
    if ( ret.ok() ) {
        for ( size_t i = 0;
                i < prop_arr.size();
                ++i ) {
            try {
                _dvm += boost::any_cast< std::string >(
                                prop_arr[i][ irods::CFG_FILENAME_KW ] );
            } catch ( boost::bad_any_cast& _e ) {
                rodsLog(
                    LOG_ERROR,
                    "failed to cast rule data variable file name entry to string" );
                continue;
            }
            _dvm += ",";
        }

        _dvm = _dvm.substr( 0, _dvm.size() - 1 );

    }

    return SUCCESS();

} // capture_rulesets

int
chkAndResetRule() {
    int status = 0;

    std::string re_str, fnm_str, dvm_str;
    irods::error ret = capture_rulesets(
                           re_str,
                           fnm_str,
                           dvm_str );
    if( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    /* get max timestamp */
    char fn[MAX_NAME_LEN];
    char r1[NAME_LEN], r2[RULE_SET_DEF_LENGTH], r3[RULE_SET_DEF_LENGTH];
    snprintf( r2, sizeof( r2 ), "%s", re_str.c_str() );
    uint mtime = 0;

    std::string re_full_path;
    while ( strlen( r2 ) > 0 ) {
        rSplitStr( r2, r1, NAME_LEN, r3, RULE_SET_DEF_LENGTH, ',' );
        getRuleBasePath( r1, fn );
        
        re_full_path = fn;
        path p( re_full_path );
        if ( !exists( p ) ) {
            status = UNIX_FILE_STAT_ERR - errno;
            rodsLog( LOG_ERROR,
                     "chkAndResetRule: unable to read rule config file %s, status = %d",
                     re_full_path.c_str(), status );
            return status;
        }

        const uint mt = ( uint ) last_write_time( p );
        if ( mt > mtime ) {
            mtime = mt;
        }
        snprintf( r2, sizeof( r2 ), "%s", r3 );
    }

    if ( CoreIrbTimeStamp == 0 ) {
        /* first time */
        CoreIrbTimeStamp = mtime;
        return 0;
    }

    if ( mtime > CoreIrbTimeStamp ) {
        /* file has been changed */
        rodsLog(
            LOG_NOTICE,
            "chkAndResetRule: reconf file %s has been changed. re-initializing",
            re_full_path.c_str() );
        CoreIrbTimeStamp = mtime;
        clearCoreRule();
        /* The shared memory cache may have already been updated, do not force reload */
        status = initRuleEngine(
                     RULE_ENGINE_TRY_CACHE,
                     NULL,
                     (char*)re_str.c_str(),
                     (char*)fnm_str.c_str(),
                     (char*)dvm_str.c_str() );
        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "chkAndResetRule: initRuleEngine error, status = %d", status );
        }
    }

    return status;

}
