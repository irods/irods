/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "rodsServer.hpp"
#include "sharedmemory.hpp"
#include "cache.hpp"
#include "resource.hpp"
#include "initServer.hpp"
#include "miscServerFunct.hpp"

#include <syslog.h>

#include <pthread.h>

#ifndef windows_platform
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#ifdef windows_platform
#include "irodsntutil.hpp"
#endif

// =-=-=-=-=-=-=-
//
#include "irods_exception.hpp"
#include "irods_server_state.hpp"
#include "irods_client_server_negotiation.hpp"
#include "irods_network_factory.hpp"
#include "irods_server_properties.hpp"
#include "irods_server_control_plane.hpp"
#include "readServerConfig.hpp"
#include "initServer.hpp"
#include "procLog.h"
#include "rsGlobalExtern.hpp"
#include "rsGlobal.hpp"	/* server global */

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>
using namespace boost::filesystem;

#include "sockCommNetworkInterface.hpp"



uint ServerBootTime;
int SvrSock;

agentProc_t *ConnectedAgentHead = NULL;
agentProc_t *ConnReqHead = NULL;
agentProc_t *SpawnReqHead = NULL;
agentProc_t *BadReqHead = NULL;


#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
boost::mutex		  ConnectedAgentMutex;
boost::mutex		  BadReqMutex;
boost::thread*		  ReadWorkerThread[NUM_READ_WORKER_THR];
boost::thread*		  SpawnManagerThread;
boost::thread*		  PurgeLockFileThread; // JMC - backport 4612

boost::mutex		  ReadReqCondMutex;
boost::mutex		  SpawnReqCondMutex;
boost::condition_variable ReadReqCond;
boost::condition_variable SpawnReqCond;

std::vector<std::string> setExecArg( const char *commandArgv );

namespace {
// We incorporate the cache salt into the rule engine's named_mutex and shared memory object.
// This prevents (most of the time) an orphaned mutex from halting server standup. Issue most often seen
// when a running iRODS installation is uncleanly killed (leaving the file system object used to implement
// boost::named_mutex e.g. in /var/run/shm) and then the iRODS user account is recreated, yielding a different
// UID. The new iRODS user account is then unable to unlock or remove the existing mutex, blocking the server.
    irods::error createAndSetRECacheSalt() {
        // Should only ever set the cache salt once
        irods::server_properties &server_properties = irods::server_properties::getInstance();
        std::string existing_salt;
        irods::error ret = server_properties.get_property<std::string>( RE_CACHE_SALT_KW, existing_salt );
        if ( ret.ok() ) {
            rodsLog( LOG_ERROR, "createAndSetRECacheSalt: salt already set [%s]", existing_salt.c_str() );
            return ERROR( SYS_ALREADY_INITIALIZED, "createAndSetRECacheSalt: cache salt already set" );
        }

        irods::buffer_crypt::array_t buf;
        ret = irods::buffer_crypt::generate_key( buf, RE_CACHE_SALT_NUM_RANDOM_BYTES );
        if ( !ret.ok() ) {
            rodsLog( LOG_ERROR, "createAndSetRECacheSalt: failed to generate random bytes" );
            return PASS( ret );
        }

        std::string cache_salt_random;
        ret = irods::buffer_crypt::hex_encode( buf, cache_salt_random );
        if ( !ret.ok() ) {
            rodsLog( LOG_ERROR, "createAndSetRECacheSalt: failed to hex encode random bytes" );
            return PASS( ret );
        }

        std::stringstream cache_salt;
        cache_salt << "pid"
                   << static_cast<intmax_t>( getpid() )
                   << "_"
                   << cache_salt_random;

        ret = server_properties.set_property<std::string>( RE_CACHE_SALT_KW, cache_salt.str() );
        if ( !ret.ok() ) {
            rodsLog( LOG_ERROR, "createAndSetRECacheSalt: failed to set server_properties" );
            return PASS( ret );
        }

        int ret_int = setenv( SP_RE_CACHE_SALT, cache_salt.str().c_str(), 1 );
        if ( 0 != ret_int ) {
            rodsLog( LOG_ERROR, "createAndSetRECacheSalt: failed to set environment variable" );
            return ERROR( SYS_SETENV_ERR, "createAndSetRECacheSalt: failed to set environment variable" );
        }

        return SUCCESS();
    }
}


#ifndef windows_platform   /* all UNIX */
int
main( int argc, char **argv )
#else   /* Windows */
int irodsWinMain( int argc, char **argv )
#endif
{
    int c;
    int uFlag = 0;
    char tmpStr1[100], tmpStr2[100];
    char *logDir = NULL;
    char *tmpStr;

    ProcessType = SERVER_PT;	/* I am a server */

    // capture server properties
    irods::error result = irods::server_properties::getInstance().capture();
    if ( !result.ok() ) {
        irods::log( PASSMSG( "failed to read server configuration", result ) );
    }

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
    openlog( "rodsServer", LOG_ODELAY | LOG_PID, LOG_DAEMON );
#endif

    ServerBootTime = time( 0 );
    while ( ( c = getopt( argc, argv, "uvVqsh" ) ) != EOF ) {
        switch ( c ) {
        case 'u':		/* user command level. without serverized */
            uFlag = 1;
            break;
        case 'D':   /* user specified a log directory */
            logDir = strdup( optarg );
            break;
        case 'v':		/* verbose Logging */
            snprintf( tmpStr1, 100, "%s=%d", SP_LOG_LEVEL, LOG_NOTICE );
            putenv( tmpStr1 );
            rodsLogLevel( LOG_NOTICE );
            break;
        case 'V':		/* very Verbose */
            snprintf( tmpStr1, 100, "%s=%d", SP_LOG_LEVEL, LOG_DEBUG1 );
            putenv( tmpStr1 );
            rodsLogLevel( LOG_DEBUG1 );
            break;
        case 'q':           /* quiet (only errors and above) */
            snprintf( tmpStr1, 100, "%s=%d", SP_LOG_LEVEL, LOG_ERROR );
            putenv( tmpStr1 );
            rodsLogLevel( LOG_ERROR );
            break;
        case 's':		/* log SQL commands */
            snprintf( tmpStr2, 100, "%s=%d", SP_LOG_SQL, 1 );
            putenv( tmpStr2 );
            break;
        case 'h':		/* help */
            usage( argv[0] );
            exit( 0 );
        default:
            usage( argv[0] );
            exit( 1 );
        }
    }

    if ( uFlag == 0 ) {
        if ( serverize( logDir ) < 0 ) {
            exit( 1 );
        }
    }

    /* start of irodsReServer has been moved to serverMain */
#ifndef _WIN32
    signal( SIGTTIN, SIG_IGN );
    signal( SIGTTOU, SIG_IGN );
    signal( SIGCHLD, SIG_DFL );	/* SIG_IGN causes autoreap. wait get nothing */
    signal( SIGPIPE, SIG_IGN );
#ifdef osx_platform
    signal( SIGINT, ( sig_t ) serverExit );
    signal( SIGHUP, ( sig_t )serverExit );
    signal( SIGTERM, ( sig_t ) serverExit );
#else
    signal( SIGINT, serverExit );
    signal( SIGHUP, serverExit );
    signal( SIGTERM, serverExit );
#endif
#endif
    /* log the server timeout */

#ifndef _WIN32
    /* Initialize ServerTimeOut */
    if ( getenv( "ServerTimeOut" ) != NULL ) {
        int serverTimeOut;
        serverTimeOut = atoi( getenv( "ServerTimeOut" ) );
        if ( serverTimeOut < MIN_AGENT_TIMEOUT_TIME ) {
            rodsLog( LOG_NOTICE,
                     "ServerTimeOut %d is less than min of %d",
                     serverTimeOut, MIN_AGENT_TIMEOUT_TIME );
            serverTimeOut = MIN_AGENT_TIMEOUT_TIME;
        }
        rodsLog( LOG_NOTICE,
                 "ServerTimeOut has been set to %d seconds",
                 serverTimeOut );
    }
#endif

    serverMain( logDir );
    exit( 0 );
}

int
serverize( char *logDir ) {
    char *logFile = NULL;

#ifdef windows_platform
    if ( iRODSNtServerRunningConsoleMode() ) {
        return;
    }
#endif

    getLogfileName( &logFile, logDir, RODS_LOGFILE );

#ifndef windows_platform
#ifdef SYSLOG
    LogFd = 0;
#else
    LogFd = open( logFile, O_CREAT | O_WRONLY | O_APPEND, 0644 );
#endif
#else
    LogFd = iRODSNt_open( logFile, O_CREAT | O_APPEND | O_WRONLY, 1 );
#endif

    if ( LogFd < 0 ) {
        rodsLog( LOG_NOTICE, "logFileOpen: Unable to open %s. errno = %d",
                 logFile, errno );
        free( logFile );
        return -1;
    }

    free( logFile );
#ifndef windows_platform
    if ( fork() ) {	/* parent */
        exit( 0 );
    }
    else {	/* child */
        if ( setsid() < 0 ) {
            rodsLog( LOG_NOTICE,
                     "serverize: setsid failed, errno = %d\n", errno );
            exit( 1 );
        }


//sleep( 60 );


#ifndef SYSLOG
        ( void ) dup2( LogFd, 0 );
        ( void ) dup2( LogFd, 1 );
        ( void ) dup2( LogFd, 2 );
        close( LogFd );
        LogFd = 2;
#endif
    }
#else
    _close( LogFd );
#endif

#ifdef SYSLOG
    return 0;
#else
    return LogFd;
#endif
}

int
serverMain( char *logDir ) {
    int loopCnt = 0;
    int acceptErrCnt = 0;

    irods::error ret = createAndSetRECacheSalt();
    if ( !ret.ok() ) {
        rodsLog( LOG_ERROR, "serverMain: createAndSetRECacheSalt error.\n%s", ret.result().c_str() );
        exit( 1 );
    }

    rsComm_t svrComm;
    int status = initServerMain( &svrComm );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "serverMain: initServerMain error. status = %d",
                 status );
        exit( 1 );
    }


    // =-=-=-=-=-=-=-
    // Launch the Control Plane
    try {
        irods::server_control_plane ctrl_plane(
            irods::CFG_SERVER_CONTROL_PLANE_PORT );

        startProcConnReqThreads();
#if RODS_CAT // JMC - backport 4612
        try {
            PurgeLockFileThread = new boost::thread( purgeLockFileWorkerTask );
        }
        catch ( const boost::thread_resource_error& ) {
            rodsLog( LOG_ERROR, "boost encountered a thread_resource_error during thread construction in serverMain." );
        }
#endif /* RODS_CAT */


        fd_set sockMask;
        FD_ZERO( &sockMask );
        SvrSock = svrComm.sock;

        irods::server_state& state = irods::server_state::instance();
        while ( true ) {
            std::string the_server_state = state();
            if ( irods::server_state::STOPPED == the_server_state ) {
                procChildren( &ConnectedAgentHead );
                rodsLog(
                    LOG_NOTICE,
                    "iRODS Server is exiting with state [%s].",
                    the_server_state.c_str() );
                break;

            }
            else if ( irods::server_state::PAUSED == the_server_state ) {
                procChildren( &ConnectedAgentHead );
                rodsSleep(
                    0,
                    irods::SERVER_CONTROL_POLLING_TIME_MILLI_SEC * 1000 );
                continue;

            }
            else {
                if ( irods::server_state::RUNNING != the_server_state ) {
                    rodsLog(
                        LOG_NOTICE,
                        "invalid iRODS server state [%s]",
                        the_server_state.c_str() );
                }

            }

            FD_SET( svrComm.sock, &sockMask );

            int numSock = 0;
            struct timeval time_out;
            time_out.tv_sec  = 0;
            time_out.tv_usec = irods::SERVER_CONTROL_POLLING_TIME_MILLI_SEC * 1000;
            while ( ( numSock = select(
                                    svrComm.sock + 1,
                                    &sockMask,
                                    ( fd_set * ) NULL,
                                    ( fd_set * ) NULL,
                                    &time_out ) ) < 0 ) {

                if ( errno == EINTR ) {
                    rodsLog( LOG_NOTICE, "serverMain: select() interrupted" );
                    FD_SET( svrComm.sock, &sockMask );
                    continue;
                }
                else {
                    rodsLog( LOG_NOTICE, "serverMain: select() error, errno = %d",
                             errno );
                    return -1;
                }
            }

            procChildren( &ConnectedAgentHead );

            if ( 0 == numSock ) {
                continue;

            }

            const int newSock = rsAcceptConn( &svrComm );
            if ( newSock < 0 ) {
                acceptErrCnt ++;
                if ( acceptErrCnt > MAX_ACCEPT_ERR_CNT ) {
                    rodsLog( LOG_ERROR,
                             "serverMain: Too many socket accept error. Exiting" );
                    break;
                }
                else {
                    rodsLog( LOG_NOTICE,
                             "serverMain: acceptConn () error, errno = %d", errno );
                    continue;
                }
            }
            else {
                acceptErrCnt = 0;
            }

            status = chkAgentProcCnt();
            if ( status < 0 ) {
                rodsLog( LOG_NOTICE,
                         "serverMain: chkAgentProcCnt failed status = %d", status );
                // =-=-=-=-=-=-=-
                // create network object to communicate to the network
                // plugin interface.  repave with newSock as that is the
                // operational socket at this point

                irods::network_object_ptr net_obj;
                irods::error ret = irods::network_factory( &svrComm, net_obj );
                if ( !ret.ok() ) {
                    irods::log( PASS( ret ) );
                }
                else {
                    ret = sendVersion( net_obj, status, 0, NULL, 0 );
                    if ( !ret.ok() ) {
                        irods::log( PASS( ret ) );
                    }
                }
                status = mySockClose( newSock );
                printf( "close status = %d\n", status );
                continue;
            }

            addConnReqToQue( &svrComm, newSock );

#ifndef windows_platform
            loopCnt++;
            if ( loopCnt >= LOGFILE_CHK_CNT ) {
                chkLogfileName( logDir, RODS_LOGFILE );
                loopCnt = 0;
            }
#endif
        }

        try {
            PurgeLockFileThread->join();
        }
        catch ( const boost::thread_resource_error& ) {
            rodsLog( LOG_ERROR, "boost encountered a thread_resource_error during join in serverMain." );
        }
        procChildren( &ConnectedAgentHead );
        stopProcConnReqThreads();

    }
    catch ( const irods::exception& e_ ) {
        const char* what = e_.what();
        std::cerr << what << std::endl;
        return e_.code();

    }

    rodsLog( LOG_NOTICE, "iRODS Server is done." );

    return 0;

}

void
#if defined(linux_platform) || defined(aix_platform) || defined(solaris_platform) || defined(linux_platform) || defined(osx_platform)
serverExit( int sig )
#else
serverExit()
#endif
{
#ifndef windows_platform
    rodsLog( LOG_NOTICE, "rodsServer caught signal %d, exiting", sig );
#else
    rodsLog( LOG_NOTICE, "rodsServer is exiting." );
#endif
    recordServerProcess( NULL ); /* unlink the process id file */
    exit( 1 );
}

void
usage( char *prog ) {
    printf( "Usage: %s [-uvVqs]\n", prog );
    printf( " -u  user command level, remain attached to the tty\n" );
    printf( " -v  verbose (LOG_NOTICE)\n" );
    printf( " -V  very verbose (LOG_DEBUG1)\n" );
    printf( " -q  quiet (LOG_ERROR)\n" );
    printf( " -s  log SQL commands\n" );

}

int
procChildren( agentProc_t **agentProcHead ) {
    int childPid;
    agentProc_t *tmpAgentProc;
    int status;


#ifndef _WIN32
    while ( ( childPid = waitpid( -1, &status, WNOHANG ) ) > 0 ) {
        tmpAgentProc = getAgentProcByPid( childPid, agentProcHead );
        if ( tmpAgentProc != NULL ) {
            rodsLog( LOG_NOTICE, "Agent process %d exited with status %d",
                     childPid, status );
            free( tmpAgentProc );
        }
        else {
            rodsLog( LOG_NOTICE,
                     "Agent process %d exited with status %d but not in queue",
                     childPid, status );
        }
        rmProcLog( childPid );
    }
#endif

    return 0;
}


agentProc_t *
getAgentProcByPid( int childPid, agentProc_t **agentProcHead ) {
    agentProc_t *tmpAgentProc, *prevAgentProc;
    prevAgentProc = NULL;


    boost::unique_lock< boost::mutex > con_agent_lock( ConnectedAgentMutex );

    tmpAgentProc = *agentProcHead;

    while ( tmpAgentProc != NULL ) {
        if ( childPid == tmpAgentProc->pid ) {
            if ( prevAgentProc == NULL ) {
                *agentProcHead = tmpAgentProc->next;
            }
            else {
                prevAgentProc->next = tmpAgentProc->next;
            }
            break;
        }
        prevAgentProc = tmpAgentProc;
        tmpAgentProc = tmpAgentProc->next;
    }

    con_agent_lock.unlock();

    return tmpAgentProc;
}


int
spawnAgent( agentProc_t *connReq, agentProc_t **agentProcHead ) {
    int childPid;
    int newSock;
    startupPack_t *startupPack;

    if ( connReq == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    newSock = connReq->sock;
    startupPack = &connReq->startupPack;

#ifndef windows_platform
    childPid = fork();	/* use fork instead of vfork because of multi-thread
			 * env */

    if ( childPid < 0 ) {
        return SYS_FORK_ERROR - errno;
    }
    else if ( childPid == 0 ) {	/* child */
        agentProc_t *tmpAgentProc;
        close( SvrSock );

        /* close any socket still in the queue */

        /* These queues may be inconsistent because of the multi-threading
             * of the parent. set sock to -1 if it has been closed */
        tmpAgentProc = ConnReqHead;
        while ( tmpAgentProc != NULL ) {
            if ( tmpAgentProc->sock == -1 ) {
                break;
            }
            close( tmpAgentProc->sock );
            tmpAgentProc->sock = -1;
            tmpAgentProc = tmpAgentProc->next;
        }
        tmpAgentProc = SpawnReqHead;
        while ( tmpAgentProc != NULL ) {
            if ( tmpAgentProc->sock == -1 ) {
                break;
            }
            close( tmpAgentProc->sock );
            tmpAgentProc->sock = -1;
            tmpAgentProc = tmpAgentProc->next;
        }

        execAgent( newSock, startupPack );
    }
    else {			/* parent */
        queConnectedAgentProc( childPid, connReq, agentProcHead );
    }
#else
    childPid = execAgent( newSock, startupPack );
    queConnectedAgentProc( childPid, connReq, agentProcHead );
#endif

    return childPid;
}

int
execAgent( int newSock, startupPack_t *startupPack ) {
#if windows_platform
    char *myArgv[3];
    char console_buf[20];
#else
    char *myArgv[2];
#endif
    int status;
    char buf[NAME_LEN];

    mySetenvInt( SP_NEW_SOCK, newSock );
    mySetenvInt( SP_PROTOCOL, startupPack->irodsProt );
    mySetenvInt( SP_RECONN_FLAG, startupPack->reconnFlag );
    mySetenvInt( SP_CONNECT_CNT, startupPack->connectCnt );
    mySetenvStr( SP_PROXY_USER, startupPack->proxyUser );
    mySetenvStr( SP_PROXY_RODS_ZONE, startupPack->proxyRodsZone );
    mySetenvStr( SP_CLIENT_USER, startupPack->clientUser );
    mySetenvStr( SP_CLIENT_RODS_ZONE, startupPack->clientRodsZone );
    mySetenvStr( SP_REL_VERSION, startupPack->relVersion );
    mySetenvStr( SP_API_VERSION, startupPack->apiVersion );

    // =-=-=-=-=-=-=-
    // if the client-server negotiation request is in the
    // option variable, set that env var and strip it out
    std::string opt_str( startupPack->option );
    size_t pos = opt_str.find( REQ_SVR_NEG );
    if ( std::string::npos != pos ) {
        std::string trunc_str = opt_str.substr( 0, pos );
        mySetenvStr( SP_OPTION,           trunc_str.c_str() );
        mySetenvStr( irods::RODS_CS_NEG, REQ_SVR_NEG );

    }
    else {
        mySetenvStr( SP_OPTION, startupPack->option );

    }

    mySetenvInt( SERVER_BOOT_TIME, ServerBootTime );


#ifdef windows_platform  /* windows */
    iRODSNtGetAgentExecutableWithPath( buf, AGENT_EXE );
    myArgv[0] = buf;
    if ( iRODSNtServerRunningConsoleMode() ) {
        strcpy( console_buf, "console" );
        myArgv[1] = console_buf;
        myArgv[2] = NULL;
    }
    else {
        myArgv[1] = NULL;
        myArgv[2] = NULL;
    }
#else
    rstrcpy( buf, AGENT_EXE, NAME_LEN );
    myArgv[0] = buf;
    myArgv[1] = NULL;
#endif

#if windows_platform  /* windows */
    return ( int )_spawnv( _P_NOWAIT, myArgv[0], myArgv );
#else
    status = execv( myArgv[0], myArgv );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "execAgent: execv error errno=%d", errno );
        exit( 1 );
    }
    return 0;
#endif
}

int
queConnectedAgentProc( int childPid, agentProc_t *connReq,
                       agentProc_t **agentProcHead ) {
    if ( connReq == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    connReq->pid = childPid;

    boost::unique_lock< boost::mutex > con_agent_lock( ConnectedAgentMutex );

    queAgentProc( connReq, agentProcHead, TOP_POS );

    con_agent_lock.unlock();

    return 0;
}

int
getAgentProcCnt() {
    agentProc_t *tmpAagentProc;
    int count = 0;

    boost::unique_lock< boost::mutex > con_agent_lock( ConnectedAgentMutex );

    tmpAagentProc = ConnectedAgentHead;
    while ( tmpAagentProc != NULL ) {
        count++;
        tmpAagentProc = tmpAagentProc->next;
    }
    con_agent_lock.unlock();

    return count;
}

int getAgentProcPIDs(
    std::vector<int>& _pids ) {
    agentProc_t *tmp_proc = 0;
    int count = 0;

    boost::unique_lock< boost::mutex > con_agent_lock( ConnectedAgentMutex );

    tmp_proc = ConnectedAgentHead;
    while ( tmp_proc != NULL ) {
        count++;
        _pids.push_back( tmp_proc->pid );
        tmp_proc = tmp_proc->next;
    }
    con_agent_lock.unlock();

    return count;

} // getAgentProcPIDs

int
chkAgentProcCnt() {
    int count;

    if ( MaxConnections == NO_MAX_CONNECTION_LIMIT ) {
        return 0;
    }
    count = getAgentProcCnt();
    if ( count >= MaxConnections ) {
        chkConnectedAgentProcQue();
        count = getAgentProcCnt();
        if ( count >= MaxConnections ) {
            return SYS_MAX_CONNECT_COUNT_EXCEEDED;
        }
        else {
            return 0;
        }
    }
    else {
        return 0;
    }
}

int
chkConnectedAgentProcQue() {
    agentProc_t *tmpAgentProc, *prevAgentProc, *unmatchedAgentProc;
    prevAgentProc = NULL;

    boost::unique_lock< boost::mutex > con_agent_lock( ConnectedAgentMutex );
    tmpAgentProc = ConnectedAgentHead;

    while ( tmpAgentProc != NULL ) {
        char procPath[MAX_NAME_LEN];

        snprintf( procPath, MAX_NAME_LEN, "%s/%-d", ProcLogDir,
                  tmpAgentProc->pid );
        path p( procPath );
        if ( !exists( p ) ) {
            /* the agent proc is gone */
            unmatchedAgentProc = tmpAgentProc;
            rodsLog( LOG_DEBUG,
                     "Agent process %d in Connected queue but not in ProcLogDir",
                     tmpAgentProc->pid );
            if ( prevAgentProc == NULL ) {
                ConnectedAgentHead = tmpAgentProc->next;
            }
            else {
                prevAgentProc->next = tmpAgentProc->next;
            }
            tmpAgentProc = tmpAgentProc->next;
            free( unmatchedAgentProc );
        }
        else {
            prevAgentProc = tmpAgentProc;
            tmpAgentProc = tmpAgentProc->next;
        }
    }
    con_agent_lock.unlock();

    return 0;
}

int
initServer( rsComm_t *svrComm ) {
    int status;
    rodsServerHost_t *rodsServerHost = NULL;

#ifdef windows_platform
    if ( int status startWinsock() ) {
        rodsLog( LOG_NOTICE, "initServer: startWinsock() failed. status=%d", status );
        return -1;
    }
#endif

    status = initServerInfo( svrComm );
    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "initServer: initServerInfo error, status = %d",
                 status );
        return status;
    }

    // JMC - legcay resources - printLocalResc ();
    resc_mgr.print_local_resources();

    printZoneInfo();

    status = getRcatHost( MASTER_RCAT, NULL, &rodsServerHost );

    if ( status < 0 || NULL == rodsServerHost ) { // JMC cppcheck - nullptr
        return status;
    }

    if ( rodsServerHost->localFlag == LOCAL_HOST ) {
#if RODS_CAT
        disconnectRcat();
#endif
    }
    else {
        if ( rodsServerHost->conn != NULL ) {
            rcDisconnect( rodsServerHost->conn );
            rodsServerHost->conn = NULL;
        }
    }
    initConnectControl();

#if RODS_CAT // JMC - backport 4612
    purgeLockFileDir( 0 );
#endif

    return status;
}

/* record the server process number and other information into
   a well-known file.  If svrComm is Null and this has created a file
   before, just unlink the file. */
int
recordServerProcess( rsComm_t *svrComm ) {
#ifndef windows_platform
    int myPid;
    FILE *fd;
    DIR  *dirp;
    static char filePath[100] = "";
    char cwd[1000];
    char stateFile[] = "irodsServer";
    char *tmp;
    char *cp;

    if ( svrComm == NULL ) {
        if ( filePath[0] != '\0' ) {
            unlink( filePath );
        }
        return 0;
    }
    rodsEnv *myEnv = &( svrComm->myEnv );

    /* Use /usr/tmp if it exists, /tmp otherwise */
    dirp = opendir( "/usr/tmp" );
    if ( dirp != NULL ) {
        tmp = "/usr/tmp";
        ( void )closedir( dirp );
    }
    else {
        tmp = "/tmp";
    }

    sprintf( filePath, "%s/%s.%d", tmp, stateFile, myEnv->rodsPort );

    unlink( filePath );

    myPid = getpid();
    cp = getcwd( cwd, 1000 );
    if ( cp != NULL ) {
        fd = fopen( filePath, "w" );
        if ( fd != NULL ) {
            fprintf( fd, "%d %s\n", myPid, cwd );
            fclose( fd );
            int err_code = chmod( filePath, 0664 );
            if ( err_code != 0 ) {
                rodsLog( LOG_ERROR, "chmod failed in recordServerProcess on [%s] with error code %d", filePath, err_code );
            }
        }
    }
#endif
    return 0;
}

int
initServerMain( rsComm_t *svrComm ) {
    memset( svrComm, 0, sizeof( *svrComm ) );
    int status = getRodsEnv( &svrComm->myEnv );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "initServerMain: getRodsEnv error. status = %d",
                 status );
        return status;
    }
    initAndClearProcLog();

    setRsCommFromRodsEnv( svrComm );

    status = initServer( svrComm );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "initServerMain: initServer error. status = %d",
                 status );
        exit( 1 );
    }

    irods::server_properties& props = irods::server_properties::getInstance();
    irods::error ret = props.capture_if_needed();
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    int zone_port = 0;
    ret = props.get_property <
          int > (
              irods::CFG_ZONE_PORT,
              zone_port );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();

    }

    svrComm->sock = sockOpenForInConn(
                        svrComm,
                        &zone_port,
                        NULL,
                        SOCK_STREAM );
    if ( svrComm->sock < 0 ) {
        rodsLog( LOG_ERROR,
                 "initServerMain: sockOpenForInConn error. status = %d",
                 svrComm->sock );
        return svrComm->sock;
    }

    listen( svrComm->sock, MAX_LISTEN_QUE );

    rodsLog( LOG_NOTICE,
             "rodsServer Release version %s - API Version %s is up",
             RODS_REL_VERSION, RODS_API_VERSION );

    /* Record port, pid, and cwd into a well-known file */
    recordServerProcess( svrComm );
    /* start the irodsReServer */
#ifndef windows_platform   /* no reServer for Windows yet */
    rodsServerHost_t *reServerHost = NULL;
    getReHost( &reServerHost );
    if ( reServerHost != NULL && reServerHost->localFlag == LOCAL_HOST ) {
        int re_pid = RODS_FORK();
        if ( re_pid == 0 ) {//RODS_FORK() == 0 ) { /* child */
            char *reServerOption = NULL;

            close( svrComm->sock );
            reServerOption = getenv( "reServerOption" );
            std::vector<std::string> args = setExecArg( reServerOption );
            std::vector<char *> av;
            av.push_back( "irodsReServer" );
            for ( std::vector<std::string>::iterator it = args.begin(); it != args.end(); it++ ) {
                av.push_back( strdup( it->c_str() ) );
            }
            av.push_back( NULL );
            rodsLog( LOG_NOTICE, "Starting irodsReServer" );
            execv( av[0], &av[0] );
            exit( 1 );
        }
        else {
            irods::server_properties &props =
                irods::server_properties::getInstance();
            props.set_property<int>( irods::RE_PID_KW, re_pid );
        }
    }
    else if ( unsigned char *shared = prepareServerSharedMemory() ) {
        copyCache( &shared, SHMMAX, &ruleEngineConfig );
        detachSharedMemory();
    }
    else {
        rodsLog( LOG_ERROR, "Cannot open shared memory." );
    }
    rodsServerHost_t *xmsgServerHost = NULL;
    getXmsgHost( &xmsgServerHost );
    if ( xmsgServerHost != NULL && xmsgServerHost->localFlag == LOCAL_HOST ) {
        if ( RODS_FORK() == 0 ) { /* child */
            char *av[NAME_LEN];

            close( svrComm->sock );
            memset( av, 0, sizeof( av ) );
            rodsLog( LOG_NOTICE, "Starting irodsXmsgServer" );
            av[0] = "irodsXmsgServer";
            execv( av[0], av );
            exit( 1 );
        }
    }
#endif

    return 0;
}

/* add incoming connection request to the bottom of the link list */

int
addConnReqToQue( rsComm_t *rsComm, int sock ) {
    agentProc_t *myConnReq;

    boost::unique_lock< boost::mutex > read_req_lock( ReadReqCondMutex );
    myConnReq = ( agentProc_t* )calloc( 1, sizeof( agentProc_t ) );

    myConnReq->sock = sock;
    myConnReq->remoteAddr = rsComm->remoteAddr;
    queAgentProc( myConnReq, &ConnReqHead, BOTTOM_POS );

    ReadReqCond.notify_all(); // NOTE:: check all vs one
    read_req_lock.unlock();

    return 0;
}

int
initConnThreadEnv() {
    return 0;
}

agentProc_t *
getConnReqFromQue() {
    agentProc_t *myConnReq = NULL;

    irods::server_state& state = irods::server_state::instance();
    while ( irods::server_state::STOPPED != state() && myConnReq == NULL ) {
        boost::unique_lock<boost::mutex> read_req_lock( ReadReqCondMutex );
        if ( ConnReqHead != NULL ) {
            myConnReq = ConnReqHead;
            ConnReqHead = ConnReqHead->next;
            read_req_lock.unlock();
            break;
        }

        ReadReqCond.wait( read_req_lock );
        if ( ConnReqHead == NULL ) {
            read_req_lock.unlock();
            continue;
        }
        else {
            myConnReq = ConnReqHead;
            ConnReqHead = ConnReqHead->next;
            read_req_lock.unlock();
            break;
        }
    }

    return myConnReq;
}

int
startProcConnReqThreads() {
    initConnThreadEnv();
    for ( int i = 0; i < NUM_READ_WORKER_THR; i++ ) {
        try {
            ReadWorkerThread[i] = new boost::thread( readWorkerTask );
        }
        catch ( const boost::thread_resource_error& ) {
            rodsLog( LOG_ERROR, "boost encountered a thread_resource_error during thread construction in startProcConnReqThreads." );
            return SYS_THREAD_RESOURCE_ERR;
        }
    }
    try {
        SpawnManagerThread = new boost::thread( spawnManagerTask );
    }
    catch ( const boost::thread_resource_error& ) {
        rodsLog( LOG_ERROR, "boost encountered a thread_resource_error during thread construction in startProcConnReqThreads." );
        return SYS_THREAD_RESOURCE_ERR;
    }

    return 0;
}

void
stopProcConnReqThreads() {

    SpawnReqCond.notify_all();
    try {
        SpawnManagerThread->join();
    }
    catch ( const boost::thread_resource_error& ) {
        rodsLog( LOG_ERROR, "boost encountered a thread_resource_error during join in stopProcConnReqThreads." );
    }

    for ( int i = 0; i < NUM_READ_WORKER_THR; i++ ) {
        ReadReqCond.notify_all();
        try {
            ReadWorkerThread[i]->join();
        }
        catch ( const boost::thread_resource_error& ) {
            rodsLog( LOG_ERROR, "boost encountered a thread_resource_error during join in stopProcConnReqThreads." );
        }
    }


}

void
readWorkerTask() {

    // =-=-=-=-=-=-=-
    // artificially create a conn object in order to
    // create a network object.  this is gratuitous
    // but necessary to maintain the consistent interface.
    rcComm_t            tmp_comm;
    bzero( &tmp_comm, sizeof( rcComm_t ) );

    irods::network_object_ptr net_obj;
    irods::error ret = irods::network_factory( &tmp_comm, net_obj );
    if ( !ret.ok() || !net_obj.get() ) {
        irods::log( PASS( ret ) );
        return;
    }

    irods::server_state& state = irods::server_state::instance();
    while ( irods::server_state::STOPPED != state() ) {
        agentProc_t *myConnReq = getConnReqFromQue();
        if ( myConnReq == NULL ) {
            /* someone else took care of it */
            continue;
        }
        int newSock = myConnReq->sock;

        // =-=-=-=-=-=-=-
        // repave the socket handle with the new socket
        // for this connection
        net_obj->socket_handle( newSock );
        startupPack_t *startupPack;
        struct timeval tv;
        tv.tv_sec = READ_STARTUP_PACK_TOUT_SEC;
        tv.tv_usec = 0;
        int status = readStartupPack( net_obj, &startupPack, &tv );

        if ( status < 0 ) {
            rodsLog( LOG_ERROR, "readWorkerTask - readStartupPack failed. %d", status );
            sendVersion( net_obj, status, 0, NULL, 0 );

            boost::unique_lock<boost::mutex> bad_req_lock( BadReqMutex );

            queAgentProc( myConnReq, &BadReqHead, TOP_POS );

            bad_req_lock.unlock();

            mySockClose( newSock );
        }
        else if ( startupPack->connectCnt > MAX_SVR_SVR_CONNECT_CNT ) {
            sendVersion( net_obj, SYS_EXCEED_CONNECT_CNT, 0, NULL, 0 );
            mySockClose( newSock );
            free( myConnReq );
        }
        else {
            if ( startupPack->clientUser[0] == '\0' ) {
                status = chkAllowedUser( startupPack->clientUser,
                                         startupPack->clientRodsZone );
                if ( status < 0 ) {
                    sendVersion( net_obj, status, 0, NULL, 0 );
                    mySockClose( newSock );
                    free( myConnReq );
                    continue;
                }
            }

            myConnReq->startupPack = *startupPack;
            free( startupPack );

            boost::unique_lock< boost::mutex > spwn_req_lock( SpawnReqCondMutex );

            queAgentProc( myConnReq, &SpawnReqHead, BOTTOM_POS );

            SpawnReqCond.notify_all(); // NOTE:: look into notify_one vs notify_all

        } // else

    } // while 1

} // readWorkerTask

void
spawnManagerTask() {
    agentProc_t *mySpawnReq = NULL;
    int status;
    uint curTime;
    uint agentQueChkTime = 0;

    irods::server_state& state = irods::server_state::instance();
    while ( irods::server_state::STOPPED != state() ) {

        boost::unique_lock<boost::mutex> spwn_req_lock( SpawnReqCondMutex );
        SpawnReqCond.wait( spwn_req_lock );

        while ( SpawnReqHead != NULL ) {
            mySpawnReq = SpawnReqHead;
            SpawnReqHead = mySpawnReq->next;

            spwn_req_lock.unlock();

            status = spawnAgent( mySpawnReq, &ConnectedAgentHead );
            close( mySpawnReq->sock );

            if ( status < 0 ) {
                rodsLog( LOG_NOTICE,
                         "spawnAgent error for puser=%s and cuser=%s from %s, stat=%d",
                         mySpawnReq->startupPack.proxyUser,
                         mySpawnReq->startupPack.clientUser,
                         inet_ntoa( mySpawnReq->remoteAddr.sin_addr ), status );
                free( mySpawnReq );
            }
            else {
                rodsLog( LOG_NOTICE,
                         "Agent process %d started for puser=%s and cuser=%s from %s",
                         mySpawnReq->pid, mySpawnReq->startupPack.proxyUser,
                         mySpawnReq->startupPack.clientUser,
                         inet_ntoa( mySpawnReq->remoteAddr.sin_addr ) );
            }

            spwn_req_lock.lock();

        }

        spwn_req_lock.unlock();

        curTime = time( 0 );
        if ( curTime > agentQueChkTime + AGENT_QUE_CHK_INT ) {
            agentQueChkTime = curTime;
            procBadReq();
        }
    }
}

int
procSingleConnReq( agentProc_t *connReq ) {
    startupPack_t *startupPack;
    int newSock;
    int status;

    if ( connReq == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    newSock = connReq->sock;

    // =-=-=-=-=-=-=-
    // artificially create a conn object in order to
    // create a network object.  this is gratuitous
    // but necessary to maintain the consistent interface.
    rcComm_t            tmp_comm;
    bzero( &tmp_comm, sizeof( rcComm_t ) );

    irods::network_object_ptr net_obj;
    irods::error ret = irods::network_factory( &tmp_comm, net_obj );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return -1;
    }

    net_obj->socket_handle( newSock );

    status = readStartupPack( net_obj, &startupPack, NULL );

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE, "readStartupPack error from %s, status = %d",
                 inet_ntoa( connReq->remoteAddr.sin_addr ), status );
        sendVersion( net_obj, status, 0, NULL, 0 );
        mySockClose( newSock );
        return status;
    }

    if ( startupPack->connectCnt > MAX_SVR_SVR_CONNECT_CNT ) {
        sendVersion( net_obj, SYS_EXCEED_CONNECT_CNT, 0, NULL, 0 );
        mySockClose( newSock );
        return SYS_EXCEED_CONNECT_CNT;
    }

    connReq->startupPack = *startupPack;
    free( startupPack );
    status = spawnAgent( connReq, &ConnectedAgentHead );

#ifndef windows_platform
    close( newSock );
#endif
    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "spawnAgent error for puser=%s and cuser=%s from %s, status = %d",
                 connReq->startupPack.proxyUser, connReq->startupPack.clientUser,
                 inet_ntoa( connReq->remoteAddr.sin_addr ), status );
    }
    else {
        rodsLog( LOG_NOTICE,
                 "Agent process %d started for puser=%s and cuser=%s from %s",
                 connReq->pid, connReq->startupPack.proxyUser,
                 connReq->startupPack.clientUser,
                 inet_ntoa( connReq->remoteAddr.sin_addr ) );
    }
    return status;
}

/* procBadReq - process bad request */
int
procBadReq() {
    agentProc_t *tmpConnReq, *nextConnReq;
    /* just free them for now */

    boost::unique_lock< boost::mutex > bad_req_lock( BadReqMutex );

    tmpConnReq = BadReqHead;
    while ( tmpConnReq != NULL ) {
        nextConnReq = tmpConnReq->next;
        free( tmpConnReq );
        tmpConnReq = nextConnReq;
    }
    BadReqHead = NULL;
    bad_req_lock.unlock();

    return 0;
}

// =-=-=-=-=-=-=-
// JMC - backport 4612
void
purgeLockFileWorkerTask() {
    size_t wait_time_ms = 0;
    const size_t purge_time_ms = LOCK_FILE_PURGE_TIME * 1000; // s to ms

    irods::server_state& state = irods::server_state::instance();
    while ( irods::server_state::STOPPED != state() ) {
        rodsSleep( 0, irods::SERVER_CONTROL_POLLING_TIME_MILLI_SEC * 1000 ); // microseconds
        wait_time_ms += irods::SERVER_CONTROL_POLLING_TIME_MILLI_SEC;

        if ( wait_time_ms >= purge_time_ms ) {
            wait_time_ms = 0;
            int status = purgeLockFileDir( 1 );
            if ( status < 0 ) {
                rodsLogError(
                    LOG_ERROR,
                    status,
                    "purgeLockFileWorkerTask: purgeLockFileDir failed" );
            }

        } // if

    } // while

} // purgeLockFileWorkerTask

std::vector<std::string>
setExecArg( const char *commandArgv ) {

    std::vector<std::string> arguments;
    if ( commandArgv != NULL ) {
        int len = 0;
        bool openQuote = false;
        const char* cur = commandArgv;
        for ( int i = 0; commandArgv[i] != '\0'; i++ ) {
            if ( commandArgv[i] == ' ' && !openQuote ) {
                if ( len > 0 ) {    /* end of a argv */
                    std::string( cur, len );
                    arguments.push_back( std::string( cur, len ) );
                    /* reset inx and pointer */
                    cur = &commandArgv[i + 1];
                    len = 0;
                }
                else {      /* skip over blanks */
                    cur = &commandArgv[i + 1];
                }
            }
            else if ( commandArgv[i] == '\'' || commandArgv[i] == '\"' ) {
                openQuote ^= true;
                if ( openQuote ) {
                    /* skip the quote */
                    cur = &commandArgv[i + 1];
                }
            }
            else {
                len ++;
            }
        }
        if ( len > 0 ) {    /* handle the last argv */
            arguments.push_back( std::string( cur, len ) );
        }
    }

    return arguments;
}
// =-=-=-=-=-=-=-
