/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "rodsServer.hpp"
#include "resource.hpp"
#include "miscServerFunct.hpp"

#include <syslog.h>

#ifndef SINGLE_SVR_THR
#include <pthread.h>
#endif
#ifndef windows_platform
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#ifdef windows_platform
#include "irodsntutil.hpp"
#endif

// =-=-=-=-=-=-=-
#include "irods_client_server_negotiation.hpp"
#include "irods_network_factory.hpp"
#include "irods_server_properties.hpp"
#include "readServerConfig.hpp"



uint ServerBootTime;
int SvrSock;

agentProc_t *ConnectedAgentHead = NULL;
agentProc_t *ConnReqHead = NULL;
agentProc_t *SpawnReqHead = NULL;
agentProc_t *BadReqHead = NULL;

#ifndef SINGLE_SVR_THR
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
boost::mutex		  ConnectedAgentMutex;
boost::mutex		  BadReqMutex;
boost::thread*		  ReadWorkerThread[NUM_READ_WORKER_THR];
boost::thread*		  SpawnManagerThread;
boost::thread*		  PurgeLockFileThread; // JMC - backport 4612
#endif

boost::mutex		  ReadReqCondMutex;
boost::mutex		  SpawnReqCondMutex;
boost::condition_variable ReadReqCond;
boost::condition_variable SpawnReqCond;

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
    bool run_server_as_root = false;


    ProcessType = SERVER_PT;	/* I am a server */

    // capture server properties
    irods::error result = irods::server_properties::getInstance().capture();
    if ( !result.ok() ) {
        irods::log( PASSMSG( "failed to read server configuration", result ) );
    }

    irods::server_properties::getInstance().get_property<bool>( RUN_SERVER_AS_ROOT_KW, run_server_as_root );

#ifndef windows_platform
    if ( run_server_as_root ) {
        if ( initServiceUser() < 0 ) {
            exit( 1 );
        }
    }
#endif


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
    free( logFile );

    if ( LogFd < 0 ) {
        rodsLog( LOG_NOTICE, "logFileOpen: Unable to open %s. errno = %d",
                 logFile, errno );
        return ( -1 );
    }

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
    return ( 0 );
#else
    return ( LogFd );
#endif
}

int
serverMain( char *logDir ) {
    int status;
    rsComm_t svrComm;
    fd_set sockMask;
    int numSock;
    int newSock;
    int loopCnt = 0;
    int acceptErrCnt = 0;
#ifdef SYS_TIMING
    int connCnt = 0;
#endif


    status = initServerMain( &svrComm );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "serverMain: initServerMain error. status = %d",
                 status );
        exit( 1 );
    }
#ifndef SINGLE_SVR_THR
    startProcConnReqThreads();
#if RODS_CAT // JMC - backport 4612
    PurgeLockFileThread = new boost::thread( purgeLockFileWorkerTask );
#endif /* RODS_CAT */
#endif /* SINGLE_SVR_THR */

    FD_ZERO( &sockMask );

    SvrSock = svrComm.sock;
    while ( 1 ) {		/* infinite loop */
        FD_SET( svrComm.sock, &sockMask );
        while ( ( numSock = select( svrComm.sock + 1, &sockMask,
                                    ( fd_set * ) NULL, ( fd_set * ) NULL, ( struct timeval * ) NULL ) ) < 0 ) {

            if ( errno == EINTR ) {
                rodsLog( LOG_NOTICE, "serverMain: select() interrupted" );
                FD_SET( svrComm.sock, &sockMask );
                continue;
            }
            else {
                rodsLog( LOG_NOTICE, "serverMain: select() error, errno = %d",
                         errno );
                return ( -1 );
            }
        }

        procChildren( &ConnectedAgentHead );
#ifdef SYS_TIMING
        initSysTiming( "irodsServer", "recv connection", 0 );
#endif

        newSock = rsAcceptConn( &svrComm );

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

#ifdef SYS_TIMING
        connCnt ++;
        rodsLog( LOG_NOTICE, "irodsServer: agent proc count = %d, connCnt = %d",
                 getAgentProcCnt( ConnectedAgentHead ), connCnt );
#endif

#ifdef SINGLE_SVR_THR
        procSingleConnReq( ConnReqHead );
        ConnReqHead = NULL;
#endif

#ifndef windows_platform
        loopCnt++;
        if ( loopCnt >= LOGFILE_CHK_CNT ) {
            chkLogfileName( logDir, RODS_LOGFILE );
            loopCnt = 0;
        }
#endif
    }		/* infinite loop */

    /* not reached - return (status); */
    return( 0 ); /* to avoid warning */
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
    while ( ( childPid = waitpid( -1, &status, WNOHANG | WUNTRACED ) ) > 0 ) {
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

    return ( 0 );
}


agentProc_t *
getAgentProcByPid( int childPid, agentProc_t **agentProcHead ) {
    agentProc_t *tmpAgentProc, *prevAgentProc;
    prevAgentProc = NULL;

#ifndef SINGLE_SVR_THR
    boost::unique_lock< boost::mutex > con_agent_lock( ConnectedAgentMutex );
#endif
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
#ifndef SINGLE_SVR_THR
    con_agent_lock.unlock();
#endif
    return ( tmpAgentProc );
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
#ifdef SYS_TIMING
        printSysTiming( "irodsAgent", "after fork", 0 );
        initSysTiming( "irodsAgent", "after fork", 1 );
#endif
        /* close any socket still in the queue */
#ifndef SINGLE_SVR_THR
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
#endif
        execAgent( newSock, startupPack );
    }
    else {			/* parent */
#ifdef SYS_TIMING
        printSysTiming( "irodsServer", "fork agent", 0 );
#endif
        queConnectedAgentProc( childPid, connReq, agentProcHead );
    }
#else
    childPid = execAgent( newSock, startupPack );
    queConnectedAgentProc( childPid, connReq, agentProcHead );
#endif

    return ( childPid );
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
    return ( 0 );
#endif
}

int
queConnectedAgentProc( int childPid, agentProc_t *connReq,
                       agentProc_t **agentProcHead ) {
    if ( connReq == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    connReq->pid = childPid;
#ifndef SINGLE_SVR_THR
    boost::unique_lock< boost::mutex > con_agent_lock( ConnectedAgentMutex );
#endif

    queAgentProc( connReq, agentProcHead, TOP_POS );

#ifndef SINGLE_SVR_THR
    con_agent_lock.unlock();
#endif

    return ( 0 );
}

int
getAgentProcCnt() {
    agentProc_t *tmpAagentProc;
    int count = 0;

#ifndef SINGLE_SVR_THR
    boost::unique_lock< boost::mutex > con_agent_lock( ConnectedAgentMutex );
#endif

    tmpAagentProc = ConnectedAgentHead;
    while ( tmpAagentProc != NULL ) {
        count++;
        tmpAagentProc = tmpAagentProc->next;
    }
#ifndef SINGLE_SVR_THR
    con_agent_lock.unlock();
#endif

    return count;
}

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

#ifndef SINGLE_SVR_THR
    boost::unique_lock< boost::mutex > con_agent_lock( ConnectedAgentMutex );
#endif
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
#ifndef SINGLE_SVR_THR
    con_agent_lock.unlock();
#endif
    return 0;
}

int
initServer( rsComm_t *svrComm ) {
    int status;
    rodsServerHost_t *rodsServerHost = NULL;

#ifdef windows_platform
    status = startWinsock();
    if ( status != 0 ) {
        rodsLog( LOG_NOTICE, "initServer: startWinsock() failed. status=%d", status );
        return -1;
    }
#endif

    status = initServerInfo( svrComm );
    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "initServer: initServerInfo error, status = %d",
                 status );
        return ( status );
    }

    // JMC - legcay resources - printLocalResc ();
    resc_mgr.print_local_resources();

    printZoneInfo();

    status = getRcatHost( MASTER_RCAT, NULL, &rodsServerHost );

    if ( status < 0 || NULL == rodsServerHost ) { // JMC cppcheck - nullptr
        return ( status );
    }

    if ( rodsServerHost->localFlag == LOCAL_HOST ) {
#if RODS_CAT
        disconnectRcat( svrComm );
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

    return ( status );
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
            chmod( filePath, 0664 );
        }
    }
#endif
    return 0;
}

int
initServerMain( rsComm_t *svrComm ) {
    int status;
    rodsServerHost_t *reServerHost = NULL;
    rodsServerHost_t *xmsgServerHost = NULL;

    bzero( svrComm, sizeof( rsComm_t ) );

    status = getRodsEnv( &svrComm->myEnv );

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
    svrComm->sock = sockOpenForInConn( svrComm, &svrComm->myEnv.rodsPort,
                                       NULL, SOCK_STREAM );

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
    getReHost( &reServerHost );
    if ( reServerHost != NULL && reServerHost->localFlag == LOCAL_HOST ) {
        if ( RODS_FORK() == 0 ) { /* child */
            char *reServerOption = NULL;
            char *av[NAME_LEN];

            close( svrComm->sock );
            memset( av, 0, sizeof( av ) );
            reServerOption = getenv( "reServerOption" );
            setExecArg( reServerOption, av );
            rodsLog( LOG_NOTICE, "Starting irodsReServer" );
            av[0] = "irodsReServer";
            execv( av[0], av );
            exit( 1 );
        }
    }
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

    return status;
}

/* add incoming connection request to the bottom of the link list */

int
addConnReqToQue( rsComm_t *rsComm, int sock ) {
    agentProc_t *myConnReq;

#ifndef SINGLE_SVR_THR
    boost::unique_lock< boost::mutex > read_req_lock( ReadReqCondMutex );
#endif
    myConnReq = ( agentProc_t* )calloc( 1, sizeof( agentProc_t ) );

    myConnReq->sock = sock;
    myConnReq->remoteAddr = rsComm->remoteAddr;
    queAgentProc( myConnReq, &ConnReqHead, BOTTOM_POS );

#ifndef SINGLE_SVR_THR
    ReadReqCond.notify_all(); // NOTE:: check all vs one
    read_req_lock.unlock();
#endif

    return ( 0 );
}

int
initConnThreadEnv() {
    return ( 0 );
}

agentProc_t *
getConnReqFromQue() {
    agentProc_t *myConnReq = NULL;

    while ( myConnReq == NULL ) {
#ifndef SINGLE_SVR_THR
        boost::unique_lock<boost::mutex> read_req_lock( ReadReqCondMutex );
#endif
        if ( ConnReqHead != NULL ) {
            myConnReq = ConnReqHead;
            ConnReqHead = ConnReqHead->next;
#ifndef SINGLE_SVR_THR
            read_req_lock.unlock();
#endif
            break;
        }

#ifndef SINGLE_SVR_THR
        ReadReqCond.wait( read_req_lock );
#endif
        if ( ConnReqHead == NULL ) {
#ifndef SINGLE_SVR_THR
            read_req_lock.unlock();
#endif
            continue;
        }
        else {
            myConnReq = ConnReqHead;
            ConnReqHead = ConnReqHead->next;
#ifndef SINGLE_SVR_THR
            read_req_lock.unlock();
#endif
            break;
        }
    }

    return ( myConnReq );
}

int
startProcConnReqThreads() {
    int status = 0;
    int i;

    initConnThreadEnv();
    for ( i = 0; i < NUM_READ_WORKER_THR; i++ ) {
        ReadWorkerThread[i] = new boost::thread( readWorkerTask );
        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "pthread_create of readWorker %d failed, errno = %d",
                     i, errno );
        }
    }
    SpawnManagerThread = new boost::thread( spawnManagerTask );

    return ( status );
}

void
readWorkerTask() {
    agentProc_t *myConnReq = NULL;
    startupPack_t *startupPack;
    int newSock = 0;
    int status  = 0;
    struct timeval tv;

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

    tv.tv_sec = READ_STARTUP_PACK_TOUT_SEC;
    tv.tv_usec = 0;
    while ( 1 ) {
        myConnReq = getConnReqFromQue();
        if ( myConnReq == NULL ) {
            /* someone else took care of it */
            continue;
        }

        newSock = myConnReq->sock;

        // =-=-=-=-=-=-=-
        // repave the socket handle with the new socket
        // for this connection
        net_obj->socket_handle( newSock );
        status = readStartupPack( net_obj, &startupPack, &tv );

        if ( status < 0 ) {
            rodsLog( LOG_ERROR, "readWorkerTask - readStartupPack failed. %d", status );
            sendVersion( net_obj, status, 0, NULL, 0 );
#ifndef SINGLE_SVR_THR
            boost::unique_lock<boost::mutex> bad_req_lock( BadReqMutex );
#endif
            queAgentProc( myConnReq, &BadReqHead, TOP_POS );
#ifndef SINGLE_SVR_THR
            bad_req_lock.unlock();
#endif
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
                }
            }

            myConnReq->startupPack = *startupPack;
            free( startupPack );
#ifndef SINGLE_SVR_THR
            boost::unique_lock< boost::mutex > spwn_req_lock( SpawnReqCondMutex );
#endif
            queAgentProc( myConnReq, &SpawnReqHead, BOTTOM_POS );
#ifndef SINGLE_SVR_THR
            SpawnReqCond.notify_all(); // NOTE:: look into notify_one vs notify_all
#endif
        } // else

    } // while 1

} // readWorkerTask

void
spawnManagerTask() {
    agentProc_t *mySpawnReq = NULL;
    int status;
    uint curTime;
    uint agentQueChkTime = 0;
    while ( 1 ) {
#ifndef SINGLE_SVR_THR
        boost::unique_lock<boost::mutex> spwn_req_lock( SpawnReqCondMutex );
        SpawnReqCond.wait( spwn_req_lock );
#endif
        while ( SpawnReqHead != NULL ) {
            mySpawnReq = SpawnReqHead;
            SpawnReqHead = mySpawnReq->next;
#ifndef SINGLE_SVR_THR
            spwn_req_lock.unlock();
#endif
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
#ifndef SINGLE_SVR_THR
            spwn_req_lock.lock();
#endif
        }
#ifndef SINGLE_SVR_THR
        spwn_req_lock.unlock();
#endif
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
#ifdef SYS_TIMING
    printSysTiming( "irodsServer", "read StartupPack", 0 );
#endif
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
#ifndef SINGLE_SVR_THR
    boost::unique_lock< boost::mutex > bad_req_lock( BadReqMutex );
#endif
    tmpConnReq = BadReqHead;
    while ( tmpConnReq != NULL ) {
        nextConnReq = tmpConnReq->next;
        free( tmpConnReq );
        tmpConnReq = nextConnReq;
    }
    BadReqHead = NULL;
#ifndef SINGLE_SVR_THR
    bad_req_lock.unlock();
#endif

    return 0;
}

// =-=-=-=-=-=-=-
// JMC - backport 4612
void
purgeLockFileWorkerTask() {
    int status;
    while ( 1 ) {
        rodsSleep( LOCK_FILE_PURGE_TIME, 0 );
        status = purgeLockFileDir( 1 );
        if ( status < 0 ) {
            rodsLogError( LOG_ERROR, status,
                          "purgeLockFileWorkerTask: purgeLockFileDir failed" );
        }
    }
}
// =-=-=-=-=-=-=-
