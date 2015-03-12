/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* rodsAgent.cpp - The main code for rodsAgent
 */

#include <syslog.h>
#include "rodsAgent.hpp"
#include "reconstants.hpp"
#include "rsApiHandler.hpp"
#include "icatHighLevelRoutines.hpp"
#include "miscServerFunct.hpp"
#include "rsGlobal.hpp"   /* server global */
#ifdef windows_platform
#include "rsLog.hpp"
static void NtAgentSetEnvsFromArgs( int ac, char **av );
#endif

// =-=-=-=-=-=-=-
#include "irods_dynamic_cast.hpp"
#include "irods_signal.hpp"
#include "irods_client_server_negotiation.hpp"
#include "irods_network_factory.hpp"
#include "irods_auth_object.hpp"
#include "irods_auth_factory.hpp"
#include "irods_auth_manager.hpp"
#include "irods_auth_plugin.hpp"
#include "irods_auth_constants.hpp"
#include "irods_server_properties.hpp"
#include "irods_server_api_table.hpp"
#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"
#include "irods_threads.hpp"
#include "procLog.h"
#include "initServer.hpp"

#include "readServerConfig.hpp"
#include "sockCommNetworkInterface.hpp"

int getAgentProcCnt() {
    return 0;
}

int getAgentProcPIDs(
    std::vector<int>& _pids ) {
    _pids.clear();
    return 0;
}


/* #define SERVER_DEBUG 1   */
int
main( int, char ** ) {

    int status;
    rsComm_t rsComm;
    char *tmpStr;

    ProcessType = AGENT_PT;

    // capture server properties
    irods::server_properties& props = irods::server_properties::getInstance();
    irods::error result = props.capture_if_needed();
    if ( !result.ok() ) {
        irods::log( PASSMSG( "failed to read server configuration", result ) );
    }

#ifdef windows_platform
    iRODSNtAgentInit( argc, argv );
#endif

#ifndef windows_platform
    signal( SIGINT, signalExit );
    signal( SIGHUP, signalExit );
    signal( SIGTERM, signalExit );
    /* set to SIG_DFL as recommended by andy.salnikov so that system()
     * call returns real values instead of 1 */
    signal( SIGCHLD, SIG_DFL );
    signal( SIGUSR1, signalExit );
    signal( SIGPIPE, SIG_IGN );

    // register irods signal handlers
    register_handlers();
#endif

#ifndef windows_platform
#ifdef SERVER_DEBUG
    if ( isPath( "/tmp/rodsdebug" ) ) {
        sleep( 20 );
    }
#endif
#endif

#ifdef SYS_TIMING
    rodsLogLevel( LOG_NOTICE );
    printSysTiming( "irodsAgent", "exec", 1 );
#endif

    memset( &rsComm, 0, sizeof( rsComm ) );
    rsComm.thread_ctx = ( thread_context* )malloc( sizeof( thread_context ) );

    status = initRsCommWithStartupPack( &rsComm, NULL );

    // =-=-=-=-=-=-=-
    // manufacture a network object for comms
    irods::network_object_ptr net_obj;
    irods::error ret = irods::network_factory( &rsComm, net_obj );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
    }

    if ( status < 0 ) {
        sendVersion( net_obj, status, 0, NULL, 0 );
        cleanupAndExit( status );
    }

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
    openlog( "rodsAgent", LOG_ODELAY | LOG_PID, LOG_DAEMON );
#endif
    status = getRodsEnv( &rsComm.myEnv );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "agentMain :: getRodsEnv failed" );
        sendVersion( net_obj, SYS_AGENT_INIT_ERR, 0, NULL, 0 );
        cleanupAndExit( status );
    }

    ret = setRECacheSaltFromEnv();
    if ( !ret.ok() ) {
        rodsLog( LOG_ERROR, "rodsAgent::main: Failed to set RE cache mutex name\n%s", ret.result().c_str() );
        exit( 1 );
    }

    // =-=-=-=-=-=-=-
    // load server side pluggable api entries
    irods::api_entry_table&  RsApiTable   = irods::get_server_api_table();
    irods::pack_entry_table& ApiPackTable = irods::get_pack_table();
    ret = irods::init_api_table(
              RsApiTable,
              ApiPackTable,
              false );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return 1;
    }

    // =-=-=-=-=-=-=-
    // load client side pluggable api entries
    irods::api_entry_table&  RcApiTable = irods::get_client_api_table();
    ret = irods::init_api_table(
              RcApiTable,
              ApiPackTable,
              true );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return 1;
    }




#if RODS_CAT
    if ( strstr( rsComm.myEnv.rodsDebug, "CAT" ) != NULL ) {
        chlDebug( rsComm.myEnv.rodsDebug );
    }
#endif

    status = initAgent( RULE_ENGINE_TRY_CACHE, &rsComm );



#ifdef SYS_TIMING
    printSysTiming( "irodsAgent", "initAgent", 0 );
#endif

    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "agentMain :: initAgent failed: %d", status );
        sendVersion( net_obj, SYS_AGENT_INIT_ERR, 0, NULL, 0 );
        cleanupAndExit( status );
    }

    /* move configConnectControl behind initAgent for now. need zoneName if
     * the user does not specify one in the input */
    initConnectControl();

    if ( rsComm.clientUser.userName[0] != '\0' ) {
        status = chkAllowedUser( rsComm.clientUser.userName,
                                 rsComm.clientUser.rodsZone );

        if ( status < 0 ) {
            sendVersion( net_obj, status, 0, NULL, 0 );
            cleanupAndExit( status );
        }
    }

    // =-=-=-=-=-=-=-
    // handle negotiations with the client regarding TLS if requested
    // this scope block makes valgrind happy
    {
        std::string neg_results;
        ret = irods::client_server_negotiation_for_server( net_obj, neg_results );
        if ( !ret.ok() || neg_results == irods::CS_NEG_FAILURE ) {
            irods::log( PASS( ret ) );
            // =-=-=-=-=-=-=-
            // send a 'we failed to negotiate' message here??
            // or use the error stack rule engine thingie
            irods::log( PASS( ret ) );
            sendVersion( net_obj, SYS_AGENT_INIT_ERR, 0, NULL, 0 );
            cleanupAndExit( ret.code() );

        }
        else {
            // =-=-=-=-=-=-=-
            // copy negotiation results to comm for action by network objects
            snprintf( rsComm.negotiation_results, sizeof( rsComm.negotiation_results ), "%s", neg_results.c_str() );
            //rsComm.ssl_do_accept = 1;

        }
    }

    /* send the server version and atatus as part of the protocol. Put
     * rsComm.reconnPort as the status */
    ret = sendVersion( net_obj, status, rsComm.reconnPort,
                       rsComm.reconnAddr, rsComm.cookie );

    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        sendVersion( net_obj, SYS_AGENT_INIT_ERR, 0, NULL, 0 );
        cleanupAndExit( status );
    }

#ifdef SYS_TIMING
    printSysTiming( "irodsAgent", "sendVersion", 0 );
#endif

    logAgentProc( &rsComm );

    // call initialization for network plugin as negotiated
    irods::network_object_ptr new_net_obj;
    ret = irods::network_factory( &rsComm, new_net_obj );
    if ( !ret.ok() ) {
        return ret.code();
    }

    ret = sockAgentStart( new_net_obj );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    new_net_obj->to_server( &rsComm );
    status = agentMain( &rsComm );

    // call initialization for network plugin as negotiated
    ret = sockAgentStop( new_net_obj );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    new_net_obj->to_server( &rsComm );
    cleanup();
    free( rsComm.thread_ctx );
    free( rsComm.auth_scheme );
    rodsLog( LOG_NOTICE, "Agent exiting with status = %d", status );
    return status;
}

static void set_rule_engine_globals(
    rsComm_t*                 _comm,
    irods::server_properties& _props ) {

    _props.set_property< std::string >(
        irods::CLIENT_USER_NAME_KW,
        _comm->clientUser.userName );
    _props.set_property< std::string >(
        irods::CLIENT_USER_ZONE_KW,
        _comm->clientUser.rodsZone );
    _props.set_property< int >(
        irods::CLIENT_USER_PRIV_KW,
        _comm->clientUser.authInfo.authFlag );

    _props.set_property< std::string >(
        irods::PROXY_USER_NAME_KW,
        _comm->proxyUser.userName );
    _props.set_property< std::string >(
        irods::PROXY_USER_ZONE_KW,
        _comm->proxyUser.rodsZone );
    _props.set_property< int >(
        irods::PROXY_USER_PRIV_KW,
        _comm->clientUser.authInfo.authFlag );

} // set_rule_engine_globals

int agentMain(
    rsComm_t *rsComm ) {
    if ( !rsComm ) {
        return SYS_INTERNAL_NULL_INPUT_ERR;

    }

    int status = 0;

    // =-=-=-=-=-=-=-
    // capture server properties
    irods::server_properties& props = irods::server_properties::getInstance();
    irods::error result = props.capture_if_needed();
    if ( !result.ok() ) {
        irods::log( PASSMSG( "failed to read server configuration", result ) );
    }

    // =-=-=-=-=-=-=-
    // compiler backwards compatibility hack
    // see header file for more details
    irods::dynamic_cast_hack();

    while ( result.ok() && status >= 0 ) {

        // set default to the native auth scheme here.
        if ( rsComm->auth_scheme == NULL ) {
            rsComm->auth_scheme = strdup( "native" );
        }
        // construct an auth object based on the scheme specified in the comm
        irods::auth_object_ptr auth_obj;
        irods::error ret = irods::auth_factory( rsComm->auth_scheme, &rsComm->rError, auth_obj );
        if ( ( result = ASSERT_PASS( ret, "Failed to factory an auth object for scheme: \"%s\".", rsComm->auth_scheme ) ).ok() ) {

            irods::plugin_ptr ptr;
            ret = auth_obj->resolve( irods::AUTH_INTERFACE, ptr );
            if ( ( result = ASSERT_PASS( ret, "Failed to resolve the auth plugin for scheme: \"%s\".",
                                         rsComm->auth_scheme ) ).ok() ) {

                irods::auth_ptr auth_plugin = boost::dynamic_pointer_cast< irods::auth >( ptr );

                // Call agent start
                char* foo = "";
                ret = auth_plugin->call <rsComm_t*, const char* > ( irods::AUTH_AGENT_START, auth_obj, rsComm, foo );
                result = ASSERT_PASS( ret, "Failed during auth plugin agent start for scheme: \"%s\".", rsComm->auth_scheme );
            }

            // =-=-=-=-=-=-=-
            // add the user info to the server properties for
            // reach by the operation wrapper for access by the
            // dynamic policy enforcement points
            set_rule_engine_globals( rsComm, props );
        }

        if ( result.ok() ) {
            if ( rsComm->ssl_do_accept ) {
                status = sslAccept( rsComm );
                if ( status < 0 ) {
                    rodsLog( LOG_ERROR, "sslAccept failed in agentMain with status %d", status );
                }
                rsComm->ssl_do_accept = 0;
            }
            if ( rsComm->ssl_do_shutdown ) {
                status = sslShutdown( rsComm );
                if ( status < 0 ) {
                    rodsLog( LOG_ERROR, "sslShutdown failed in agentMain with status %d", status );
                }
                rsComm->ssl_do_shutdown = 0;
            }

            status = readAndProcClientMsg( rsComm, READ_HEADER_TIMEOUT );
            if ( status < 0 ) {
                if ( status == DISCONN_STATUS ) {
                    status = 0;
                    break;
                }
            }
        }
    }

    if ( !result.ok() ) {
        irods::log( result );
        status = result.code();
        return status;
    }

    // =-=-=-=-=-=-=-
    // determine if we even need to connect, break the
    // infinite reconnect loop.
    if ( !resc_mgr.need_maintenance_operations() ) {
        return status;
    }

    // =-=-=-=-=-=-=-
    // find the icat host
    rodsServerHost_t *rodsServerHost = 0;
    status = getRcatHost( MASTER_RCAT, 0, &rodsServerHost );
    if ( status < 0 ) {
        irods::log( ERROR( status, "getRcatHost failed." ) );
        return status;
    }

    // =-=-=-=-=-=-=-
    // connect to the icat host
    status = svrToSvrConnect( rsComm, rodsServerHost );
    if ( status < 0 ) {
        irods::log( ERROR( status, "svrToSvrConnect failed." ) );
        return status;
    }

    // =-=-=-=-=-=-=-
    // call post disconnect maintenance operations before exit
    status = resc_mgr.call_maintenance_operations( rodsServerHost->conn );

    return status;
}





























