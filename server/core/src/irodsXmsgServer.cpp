/*** Copyright (c), The Regents of the University of California
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* irodsXmsgServer.cpp - The irods xmsg server */
#include "reconstants.hpp"
#include "irodsXmsgServer.hpp"
#include "xmsgLib.hpp"
#include "irods_exception.hpp"
#include "irods_server_properties.hpp"
#include "irods_client_server_negotiation.hpp"
#include "irods_network_factory.hpp"
#include "irods_re_plugin.hpp"
#include "irods_signal.hpp"
#include "sockCommNetworkInterface.hpp"
#include "initServer.hpp"
#include "miscServerFunct.hpp"

int loopCnt = -1; /* make it -1 to run infinitely */


int
main( int argc, char **argv ) {
    int c;
    int runMode = SERVER;
    int flagval = 0;
    char *logDir = nullptr;
    char *tmpStr;
    int logFd;

    ProcessType = XMSG_SERVER_PT;

    irods::error ret = setRECacheSaltFromEnv();
    if ( !ret.ok() ) {
        rodsLog( LOG_ERROR, "irodsXmsgServer::main: Failed to set RE cache mutex name\n%s", ret.result().c_str() );
        exit( 1 );
    }

    irods::re_plugin_globals.reset(new irods::global_re_plugin_mgr);

    signal( SIGINT, signalExit );
    signal( SIGHUP, signalExit );
    signal( SIGTERM, signalExit );
    signal( SIGUSR1, signalExit );
    signal( SIGPIPE, rsPipeSignalHandler );

    /* Handle option to log sql commands */
    tmpStr = getenv( SP_LOG_SQL );
    if ( tmpStr != nullptr ) {
        rodsLogSqlReq( 1 );
    }

    /* Set the logging level */
    tmpStr = getenv( SP_LOG_LEVEL );
    if ( tmpStr != nullptr ) {
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
        rodsLog( LOG_ERROR, "xmsgServerMain: initAgent error. status = %d",
                 status );
        return status;
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
        sendVersion( net_obj, SYS_AGENT_INIT_ERR, 0, nullptr, 0 );
        cleanupAndExit( ret.code() );

    }
    else {
        // =-=-=-=-=-=-=-
        // copy negotiation results to comm for action by network objects
        snprintf( rsComm.negotiation_results, sizeof( rsComm.negotiation_results ),
                  "%s", neg_results.c_str() );
        //rsComm.ssl_do_accept = 1;

    }

    /* open  a socket and listen for connection */
    try {
        int xmsgPort = irods::get_server_property<const int>(irods::CFG_XMSG_PORT);
        svrComm.sock = sockOpenForInConn(
                        &svrComm,
                        &xmsgPort,
                        nullptr,
                        SOCK_STREAM );
    } catch ( const irods::exception& e ) {
        irods::log( irods::error(e) );
        return e.code();
    }

    if ( svrComm.sock < 0 ) {
        rodsLog( LOG_NOTICE, "xmsgServerMain: sockOpenForInConn error. status = %d",
                 svrComm.sock );
        exit( 1 );
    }

    listen( svrComm.sock, MAX_LISTEN_QUE );

    FD_ZERO( &sockMask );

    rodsLog( LOG_NOTICE, "xmsgServer version %s is up", RODS_REL_VERSION );

    while ( 1 ) {       /* infinite loop */
        FD_SET( svrComm.sock, &sockMask );
        while ( ( numSock = select( svrComm.sock + 1, &sockMask,
                                    ( fd_set * ) nullptr, ( fd_set * ) nullptr, ( struct timeval * ) nullptr ) ) < 0 ) {

            if ( errno == EINTR ) {
                rodsLog( LOG_NOTICE, "xmsgServerMain: select() interrupted" );
                FD_SET( svrComm.sock, &sockMask );
                continue;
            }
            else {
                rodsLog( LOG_NOTICE, "xmsgServerMain: select() error, errno = %d", errno );
                return -1;
            }
        }

        const int newSock = rsAcceptConn( &svrComm );

        if ( newSock < 0 ) {
            rodsLog( LOG_NOTICE,
                     "xmsgServerMain: acceptConn () error, errno = %d", errno );
            continue;
        }

        addReqToQue( newSock );

        if ( loopCnt > 0 ) {
            loopCnt--;
            if ( loopCnt == 0 ) {
                return 0;
            }
        }

    }
    return 0;
}
