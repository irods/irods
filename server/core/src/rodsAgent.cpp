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
#include "irods_environment_properties.hpp"
#include "irods_server_properties.hpp"
#include "irods_server_api_table.hpp"
#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"
#include "irods_threads.hpp"
#include "irods_re_plugin.hpp"
#include "irods_re_serialization.hpp"
#include "procLog.h"
#include "initServer.hpp"

#include "sockCommNetworkInterface.hpp"
#include "sslSockComm.h"

/* #define SERVER_DEBUG 1   */

#include "sys/socket.h"
#include "sys/un.h"
#include "sys/wait.h"

ssize_t receiveSocketFromSocket( int readFd, int *socket) {
    struct msghdr msg;
    memset(&msg, 0, sizeof(msg));
    ssize_t n;

    char message_buf[1024];
    struct iovec io = { .iov_base = message_buf, .iov_len = sizeof(message_buf) };
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;

    char control_buf[1024];
    msg.msg_control = control_buf;
    msg.msg_controllen = sizeof(control_buf);

    struct cmsghdr *cmptr;

    if ( ( n = recvmsg( readFd, &msg, MSG_WAITALL ) ) <= 0) {
        return n;
    }
    cmptr = CMSG_FIRSTHDR( &msg );
    unsigned char* data = CMSG_DATA(cmptr);
    int theSocket = *((int*) data);
    *socket = theSocket;

    return n;
}

int receiveDataFromServer( int inSocket ) {
    int status;
    char in_buf[1024];
    memset( in_buf, 0, 1024 );
    bool data_complete = false;

    recv( inSocket, &in_buf, 1024, 0 );

    sockaddr_un tmp_socket_addr;
    memset( &tmp_socket_addr, 0, sizeof(tmp_socket_addr) );
    tmp_socket_addr.sun_family = AF_UNIX;
    strcpy( tmp_socket_addr.sun_path, in_buf );
    unsigned int len = sizeof(tmp_socket_addr);

    int tmp_socket, conn_tmp_socket;
    tmp_socket = socket( AF_UNIX, SOCK_STREAM, 0 );

    // Delete socket if it already exists
    unlink( tmp_socket_addr.sun_path );

    if ( bind( tmp_socket, (struct sockaddr*) &tmp_socket_addr, len ) == -1 ) {
        rodsLog(LOG_ERROR, "[%s:%d] Unable to bind socket in receiveDataFromServer", __FUNCTION__, __LINE__);
        return -1;
    }

    if ( listen( tmp_socket, 5) == -1 ) {
        rodsLog(LOG_ERROR, "[%s:%d] Failed to set up socket for listening in receiveDataFromServer", __FUNCTION__, __LINE__);
        return -1;
    }

    // Send acknowledgement that socket has been created
    char ack_buffer[256];
    len = snprintf( ack_buffer, 256, "OK" );
    send ( inSocket, ack_buffer, len, 0 );

    conn_tmp_socket = accept( tmp_socket, (struct sockaddr*) &tmp_socket_addr, &len);
    if ( conn_tmp_socket == -1 ) {
        rodsLog(LOG_ERROR, "[%s:%d] Failed to accept client socket in receiveDataFromServer", __FUNCTION__, __LINE__);
        return -1;
    }

    while (!data_complete) {
        memset( in_buf, 0, 1024 );
        recv( conn_tmp_socket, &in_buf, 1024, 0 );

        if (strlen(in_buf) == 0) {
            return -1;
        }

        char* tokenized_strings = strtok(in_buf, ";");

        while (tokenized_strings != NULL) {
            std::string tmpStr = tokenized_strings;

            if ( tmpStr == "end_of_vars" ) {
                data_complete = true;

                // Send acknowledgement that all data has been received
                send ( conn_tmp_socket, ack_buffer, strlen(ack_buffer) + 1, 0 );

                break;
            }

            unsigned long i = 0;
            for ( auto& a : tmpStr) {
                if (a == '=') {
                    break;
                }
                ++i;
            }

            if (i == tmpStr.size()) {
                // No equal sign was found
                continue;
            }

            std::string lhs = tmpStr.substr(0, i);
            std::string rhs = tmpStr.substr(i+1, tmpStr.size());

            status = setenv( lhs.c_str(), rhs.c_str(), 1 );

            tokenized_strings = strtok(NULL, ";");
        }
    }

    int newSocket; 
    receiveSocketFromSocket( conn_tmp_socket, &newSocket );

    char socket_buf[16];
    snprintf(socket_buf, 16, "%d", newSocket);

    len = snprintf( ack_buffer, 256, "%d", getpid() );
    send ( conn_tmp_socket, ack_buffer, len, 0 );

    status = setenv( "spNewSock", socket_buf, 1 );

    close( conn_tmp_socket );
    close( tmp_socket );

    return status;
}

int initRsCommFromServerSocket( rsComm_t *rsComm, int socket ) {
    char in_buf[1024];
    bool data_complete = false;
    
    while (!data_complete) {
        memset( in_buf, 0, 1024 );
        recv( socket, &in_buf, 1024, 0 );

        if (strlen(in_buf) == 0) {
            return -1;
        }

        char* tokenized_strings = strtok(in_buf, ";");

        while (tokenized_strings != NULL) {
            std::string tmpStr = tokenized_strings;

            if ( tmpStr == "end_of_vars" ) {
                data_complete = true;
                break;
            }

            unsigned long i = 0;
            for ( auto& a : tmpStr) {
                if (a == '=') {
                    break;
                }
                ++i;
            }

            if (i == tmpStr.size()) {
                // No equal sign was found
                continue;
            }

            std::string lhs = tmpStr.substr(0, i);
            std::string rhs = tmpStr.substr(i+1, tmpStr.size());


            if ( lhs == SP_CONNECT_CNT ) {
                rsComm->connectCnt = std::stoi( rhs );
            } else if ( lhs == SP_PROTOCOL ) {
                rsComm->irodsProt = (irodsProt_t) std::stoi( rhs );
            } else if ( lhs == SP_RECONN_FLAG ) {
                rsComm->reconnFlag = std::stoi( rhs );
            } else if ( lhs == SP_PROXY_USER ) {
                rstrcpy ( rsComm->proxyUser.userName, rhs.c_str(), NAME_LEN );

                if ( rhs == PUBLIC_USER_NAME ) {
                    rsComm->proxyUser.authInfo.authFlag = PUBLIC_USER_AUTH;
                }
            } else if ( lhs == SP_PROXY_RODS_ZONE ) { 
                rstrcpy( rsComm->proxyUser.rodsZone, rhs.c_str(), NAME_LEN );
            } else if ( lhs == SP_CLIENT_USER ) {
                rstrcpy( rsComm->clientUser.userName, rhs.c_str(), NAME_LEN );

                if ( rhs == PUBLIC_USER_NAME ) {
                    rsComm->clientUser.authInfo.authFlag = PUBLIC_USER_AUTH;
                }
            } else if ( lhs == SP_CLIENT_RODS_ZONE ) {
                rstrcpy( rsComm->clientUser.rodsZone, rhs.c_str(), NAME_LEN );
            } else if ( lhs == SP_REL_VERSION ) {
                rstrcpy( rsComm->cliVersion.relVersion, rhs.c_str(), NAME_LEN );
            } else if ( lhs == SP_API_VERSION ) {
                rstrcpy( rsComm->cliVersion.apiVersion, rhs.c_str(), NAME_LEN );
            } else if ( lhs == SP_OPTION ) {
                rstrcpy( rsComm->option, rhs.c_str(), LONG_NAME_LEN );
            } else if ( lhs == SP_RE_CACHE_SALT ) {
                // SP_RE_CACHE_SALT gets saved in an environment variable
                setenv(lhs.c_str(), rhs.c_str(), 1);
            }

            tokenized_strings = strtok(NULL, ";");
        }
    }

    int newSocket; 
    receiveSocketFromSocket( socket, &newSocket );
    rsComm->sock = newSocket;

    if ( rsComm->sock != 0 ) {

        /* remove error messages from xmsLog */
        setLocalAddr( rsComm->sock, &rsComm->localAddr );
        setRemoteAddr( rsComm->sock, &rsComm->remoteAddr );
    }

    char* tmpStr = inet_ntoa( rsComm->remoteAddr.sin_addr );
    if ( tmpStr == NULL || *tmpStr == '\0' ) {
        tmpStr = "UNKNOWN";
    }
    rstrcpy( rsComm->clientAddr, tmpStr, NAME_LEN );

    return 0;
}

int
runIrodsAgent( sockaddr_un agent_addr ) {
    int status;
    rsComm_t rsComm;

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

#ifdef SERVER_DEBUG
    if ( isPath( "/tmp/rodsdebug" ) ) {
rods::get_server_property<const std::string>( RE_CACHE_SALT_KW)        sleep( 20 );
    }
#endif

    int listen_socket, conn_socket;
    struct sockaddr_un client_addr;
    unsigned int len = sizeof(agent_addr);

    listen_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if ( listen_socket == -1 ) {
        rodsLog(LOG_ERROR, "[%s:%d] Unable to create socket in agent process factory", __FUNCTION__, __LINE__);
        return -1;
    }

    // Delete socket if it already exists
    unlink( agent_addr.sun_path );

    if ( bind( listen_socket, (struct sockaddr*) &agent_addr, len ) == -1 ) {
        rodsLog(LOG_ERROR, "[%s:%d] Unable to bind socket in agent process factory", __FUNCTION__, __LINE__);
        return -1;
    }

    if ( listen( listen_socket, 5) == -1 ) {
        rodsLog(LOG_ERROR, "[%s:%d] Failed to set up socket for listening in agent process factory", __FUNCTION__, __LINE__);
        return -1;
    }

    raise(SIGSTOP);

    conn_socket = accept( listen_socket, (struct sockaddr*) &client_addr, &len);
    if ( conn_socket == -1 ) {
        rodsLog(LOG_ERROR, "[%s:%d] Failed to accept client socket in agent process factory", __FUNCTION__, __LINE__);
        return -1;
    }

    while ( true ) {
        pid_t child_pid = fork();

        if (child_pid == 0) {
            // Child process - reload properties and receive data from server process
            irods::environment_properties::instance().capture();
            irods::server_properties::instance().capture();

            // Child process - Need to init the rsComm object  and break loop
            status = receiveDataFromServer( conn_socket );

            irods::error ret2 = setRECacheSaltFromEnv();
            if ( !ret2.ok() ) {
                rodsLog( LOG_ERROR, "rodsAgent::main: Failed to set RE cache mutex name\n%s", ret2.result().c_str() );
                exit( 1 );
            }

            break;
        } else if (child_pid > 0) {
            // Parent process - want to go right back to paused state
            raise(SIGSTOP);

            int reaped_pid;
            int child_status;
            while( ( reaped_pid = waitpid( -1, &child_status, WNOHANG ) ) > 0 ) {
                rodsLog( LOG_NOTICE, "Agent process [%d] exited with status [%d]", reaped_pid, child_status );
            }
        } else {
            rodsLog( LOG_ERROR, "fork() failed in rodsAgent process factory" );
            close( conn_socket );
            close( listen_socket );
            return -1;
        }
    }
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

    irods::re_serialization::serialization_map_t m = irods::re_serialization::get_serialization_map();
    irods::re_plugin_globals.reset(new irods::global_re_plugin_mgr);

    status = getRodsEnv( &rsComm.myEnv );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "agentMain :: getRodsEnv failed" );
        sendVersion( net_obj, SYS_AGENT_INIT_ERR, 0, NULL, 0 );
        cleanupAndExit( status );
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
              false );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return 1;
    }

    std::string svc_role;
    ret = get_catalog_service_role(svc_role);
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }
    if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
        if ( strstr( rsComm.myEnv.rodsDebug, "CAT" ) != NULL ) {
            chlDebug( rsComm.myEnv.rodsDebug );
        }
    }

    status = initAgent( RULE_ENGINE_TRY_CACHE, &rsComm );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "agentMain :: initAgent failed: %d", status );
        sendVersion( net_obj, SYS_AGENT_INIT_ERR, 0, NULL, 0 );
        cleanupAndExit( status );
    }

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
            sendVersion( net_obj, SERVER_NEGOTIATION_ERROR, 0, NULL, 0 );
            cleanupAndExit( ret.code() );

        }
        else {
            // =-=-=-=-=-=-=-
            // copy negotiation results to comm for action by network objects
            snprintf( rsComm.negotiation_results, sizeof( rsComm.negotiation_results ), "%s", neg_results.c_str() );

        }
    }

    /* send the server version and status as part of the protocol. Put
     * rsComm.reconnPort as the status */
    ret = sendVersion( net_obj, status, rsComm.reconnPort,
                       rsComm.reconnAddr, rsComm.cookie );

    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        sendVersion( net_obj, SYS_AGENT_INIT_ERR, 0, NULL, 0 );
        cleanupAndExit( status );
    }

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
    rodsLog( LOG_NOTICE, "Agent [%d] exiting with status = %d", getpid(), status );
    return status;
}

static void set_rule_engine_globals(
    rsComm_t* _comm ) {

    irods::set_server_property<std::string>(irods::CLIENT_USER_NAME_KW, _comm->clientUser.userName);
    irods::set_server_property<std::string>(irods::CLIENT_USER_ZONE_KW, _comm->clientUser.rodsZone);
    irods::set_server_property<int>(irods::CLIENT_USER_PRIV_KW, _comm->clientUser.authInfo.authFlag);
    irods::set_server_property<std::string>(irods::PROXY_USER_NAME_KW, _comm->proxyUser.userName);
    irods::set_server_property<std::string>(irods::PROXY_USER_ZONE_KW, _comm->proxyUser.rodsZone);
    irods::set_server_property<int>(irods::PROXY_USER_PRIV_KW, _comm->clientUser.authInfo.authFlag);

} // set_rule_engine_globals

int agentMain(
    rsComm_t *rsComm ) {
    if ( !rsComm ) {
        return SYS_INTERNAL_NULL_INPUT_ERR;

    }

    int status = 0;

    // =-=-=-=-=-=-=-
    // compiler backwards compatibility hack
    // see header file for more details
    irods::dynamic_cast_hack();

    irods::error result = SUCCESS();
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
                ret = auth_plugin->call < const char* > ( rsComm, irods::AUTH_AGENT_START, auth_obj, foo );
                result = ASSERT_PASS( ret, "Failed during auth plugin agent start for scheme: \"%s\".", rsComm->auth_scheme );
            }

            // =-=-=-=-=-=-=-
            // add the user info to the server properties for
            // reach by the operation wrapper for access by the
            // dynamic policy enforcement points
            try {
                set_rule_engine_globals( rsComm );
            } catch ( const irods::exception& e ) {
                rodsLog( LOG_ERROR, "set_rule_engine_globals failed:\n%s", e.what());
            }
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
