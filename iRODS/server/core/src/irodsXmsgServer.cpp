/*** Copyright (c), The Regents of the University of California            ***   *** For more information please refer to files in the COPYRIGHT directory ***/

/* irodsXmsgServer.c - The irods xmsg server
 */

#include "reconstants.hpp"
#include "irodsXmsgServer.hpp"
#include "xmsgLib.hpp"
#include "rsGlobal.hpp"
#include "miscServerFunct.hpp"
#include "irods_server_properties.hpp"
#include "readServerConfig.hpp"
#include "irods_client_server_negotiation.hpp"
#include "irods_network_factory.hpp"
#include "irods_signal.hpp"

int loopCnt = -1; /* make it -1 to run infinitel */


int
main( int argc, char **argv ) {
    int c;
    int runMode = SERVER;
    int flagval = 0;
    char *logDir = NULL;
    char *tmpStr;
    int logFd;
    bool run_server_as_root = false;

    ProcessType = XMSG_SERVER_PT;

    irods::server_properties::getInstance().get_property<bool>( RUN_SERVER_AS_ROOT_KW, run_server_as_root );

#ifndef windows_platform
    if ( run_server_as_root ) {
        if ( initServiceUser() < 0 ) {
            exit( 1 );
        }
    }
#endif


#ifndef _WIN32
    signal( SIGINT, signalExit );
    signal( SIGHUP, signalExit );
    signal( SIGTERM, signalExit );
    signal( SIGUSR1, signalExit );
    signal( SIGPIPE, rsPipSigalHandler );

#endif

    /* Handle option to log sql commands */
    tmpStr = getenv( SP_LOG_SQL );
    if ( tmpStr != NULL ) {
        rodsLogSqlReq( 1 );
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

    while ( ( c = getopt( argc, argv, "sSc:vD:" ) ) != EOF ) {
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
        case 'c':
            loopCnt = atoi( optarg );
            break;
        case 'D':   /* user specified a log directory */
            logDir = strdup( optarg );
            break;
        default:
            usage( argv[0] );
            exit( 1 );
        }
    }

    if ( ( logFd = logFileOpen( runMode, logDir, XMSG_SVR_LOGFILE ) ) < 0 ) {
        exit( 1 );
    }

    daemonize( runMode, logFd );


    xmsgServerMain();
    sleep( 5 );
    exit( 0 );
}

int usage( char *prog ) {
    fprintf( stderr, "Usage: %s [-scva] [-D logDir] \n", prog );
    return 0;
}

int
xmsgServerMain() {
    int status = 0;
    rsComm_t rsComm;
    rsComm_t svrComm;	/* rsComm is connection to icat, svrComm is the
			 * server's listening socket */
    fd_set sockMask;
    int numSock;
    int newSock;

    initThreadEnv();
    initXmsgHashQue();

    status = startXmsgThreads();

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "xmsgServerMain: startXmsgThreads error. status = %d", status );
        return status;
    }

    status = initRsComm( &rsComm );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "xmsgServerMain: initRsComm error. status = %d",
                 status );
        return status;
    }

    status = initRsComm( &svrComm );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "xmsgServerMain: initRsComm error. status = %d",
                 status );
        return status;
    }

    status = initAgent( RULE_ENGINE_NO_CACHE, &rsComm );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "xmsgServerMain: initServer error. status = %d",
                 status );
        return ( status );
    }


    // =-=-=-=-=-=-=-
    // handle negotiations with the client regarding TLS if requested
    irods::error ret;

    // =-=-=-=-=-=-=-
    // manufacture a network object for comms
    irods::network_object_ptr net_obj;
    ret = irods::network_factory( &rsComm, net_obj );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    std::string neg_results;
    ret = irods::client_server_negotiation_for_server( net_obj, neg_results );
    if ( !ret.ok() || neg_results == irods::CS_NEG_FAILURE ) {
        irods::log( PASS( ret ) );
        // =-=-=-=-=-=-=-
        // send a 'we failed to negotiate' message here??
        // or use the error stack rule engine thingie
        irods::log( PASS( ret ) );
        sendVersion( net_obj, SYS_AGENT_INIT_ERR, 0, NULL, 0 );
        unregister_handlers();
        cleanupAndExit( ret.code() );

    }
    else {
        // =-=-=-=-=-=-=-
        // copy negotiation results to comm for action by network objects
        strncpy( rsComm.negotiation_results, neg_results.c_str(), MAX_NAME_LEN );
        //rsComm.ssl_do_accept = 1;

    }


    /* open  a socket an listen for connection */
    svrComm.sock = sockOpenForInConn( &svrComm, &svrComm.myEnv.xmsgPort, NULL,
                                      SOCK_STREAM );

    if ( svrComm.sock < 0 ) {
        rodsLog( LOG_NOTICE, "serverMain: sockOpenForInConn error. status = %d",
                 svrComm.sock );
        exit( 1 );
    }

    listen( svrComm.sock, MAX_LISTEN_QUE );

    FD_ZERO( &sockMask );

    rodsLog( LOG_NOTICE, "rodsServer version %s is up", RODS_REL_VERSION );

    while ( 1 ) {       /* infinite loop */
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

        newSock = rsAcceptConn( &svrComm );

        if ( newSock < 0 ) {
            rodsLog( LOG_NOTICE,
                     "serverMain: acceptConn () error, errno = %d", errno );
            continue;
        }


        addReqToQue( newSock );

        if ( loopCnt > 0 ) {
            loopCnt--;
            if ( loopCnt == 0 ) {
                return( 0 );
            }
        }


    }
    /* RAJA removed June 13, 2088 to avoid compiler warning in solaris
    return status;
    */
    return 0;
}

