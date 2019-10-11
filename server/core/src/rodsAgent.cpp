/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* rodsAgent.cpp - The main code for rodsAgent
 */

#include "rodsAgent.hpp"
#include "reconstants.hpp"
#include "rsApiHandler.hpp"
#include "icatHighLevelRoutines.hpp"
#include "miscServerFunct.hpp"
#include "irods_socket_information.hpp"
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
#include "irods_server_state.hpp"
#include "irods_threads.hpp"
#include "irods_re_plugin.hpp"
#include "irods_re_serialization.hpp"
#include "irods_logger.hpp"
#include "irods_at_scope_exit.hpp"
#include "procLog.h"
#include "initServer.hpp"

#include "sockCommNetworkInterface.hpp"
#include "sslSockComm.h"

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

static void set_agent_process_name(const InformationRequiredToSafelyRenameProcess& info, const int socket_fd) {
    try {
        std::string remote_address = socket_fd_to_remote_address(socket_fd);
        if (remote_address.size() > 0) {
            const std::string desired_name = "irodsServer: " + remote_address;
            const auto l_desired = desired_name.size();
            if (l_desired <= info.argv0_size) {
                strncpy(info.argv0, desired_name.c_str(), info.argv0_size);
            }
        }
    } catch ( const irods::exception& e ) {
        rodsLog(LOG_ERROR, "set_agent_process_name: failed to get remote address of socket\n%s", e.what());
    }
}

int receiveDataFromServer( int conn_tmp_socket ) {
    ssize_t num_bytes;
    char in_buf[1024];
    memset( in_buf, 0, 1024 );
    bool data_complete = false;

    char ack_buffer[256]{};
    snprintf( ack_buffer, 256, "OK" );

    while (!data_complete) {
        memset( in_buf, 0, 1024 );
        num_bytes = recv( conn_tmp_socket, &in_buf, 1024, 0 );

        if ( num_bytes < 0 ) {
            rodsLog( LOG_ERROR, "Error receiving data from rodsServer, errno = [%d][%s]", errno, strerror( errno ) );
            return SYS_SOCK_READ_ERR;
        } else if ( num_bytes == 0 ) {
            rodsLog( LOG_ERROR, "Received 0 bytes from rodsServer" );
            return SYS_SOCK_READ_ERR;
        }

        char* tokenized_strings = strtok(in_buf, ";");

        while (tokenized_strings != NULL) {
            std::string tmpStr = tokenized_strings;

            if ( tmpStr == "end_of_vars" ) {
                data_complete = true;

                // Send acknowledgement that all data has been received
                num_bytes = send ( conn_tmp_socket, ack_buffer, strlen(ack_buffer) + 1, 0 );
                if ( num_bytes < 0 ) {
                    rodsLog( LOG_ERROR, "Error sending acknowledgment to rodsServer, errno = [%d][%s]", errno, strerror( errno ) );
                    return SYS_SOCK_READ_ERR;
                }

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

            int status{setenv(lhs.c_str(), rhs.c_str(), 1)};
            if (status < 0) {
                irods::log(ERROR(SYS_INTERNAL_ERR,
                                 (boost::format("setenv([%s],[%s],1) failed with errno = [%d][%s]") %
                                  lhs.c_str() % rhs.c_str() % errno % strerror(errno)).str().c_str()));
            }
            tokenized_strings = strtok(NULL, ";");
        }
    }

    int newSocket;
    num_bytes = receiveSocketFromSocket( conn_tmp_socket, &newSocket );
    if ( num_bytes < 0 ) {
        rodsLog( LOG_ERROR, "Error receiving socket from rodsServer, errno = [%d]", errno, strerror( errno ) );
        return SYS_SOCK_READ_ERR;
    } else if ( num_bytes == 0 ) {
        rodsLog( LOG_ERROR, "Received 0 bytes from rodsServer" );
        return SYS_SOCK_READ_ERR;
    }

    char socket_buf[16];
    snprintf(socket_buf, 16, "%d", newSocket);

    unsigned int len = snprintf( ack_buffer, 256, "%d", getpid() );
    num_bytes = send ( conn_tmp_socket, ack_buffer, len, 0 );
    if ( num_bytes < 0 ) {
        rodsLog( LOG_ERROR, "Error sending agent pid to rodsServer, errno = [%d]", errno, strerror( errno ) );
        return SYS_SOCK_READ_ERR;
    }

    int status{setenv(SP_NEW_SOCK, socket_buf, 1)};
    if (status < 0) {
        irods::log(ERROR(SYS_INTERNAL_ERR,
                         (boost::format("setenv([%s],[%s],1) failed with errno = [%d][%s]") %
                          SP_NEW_SOCK % socket_buf % errno % strerror(errno)).str().c_str()));
    }
    status = close(conn_tmp_socket);
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "close(conn_tmp_socket) failed with errno = [%d]: %s", errno, strerror( errno ) );
    }

    return status;
}

void
irodsAgentSignalExit( int ) {
    int reaped_pid, child_status;
    while( ( reaped_pid = waitpid( -1, &child_status, WNOHANG ) ) > 0 ) {
        rmProcLog( reaped_pid );
    }

    exit( 1 );
}

int
runIrodsAgentFactory( sockaddr_un agent_addr ) {
    int status{};
    rsComm_t rsComm;

    using log = irods::experimental::log;

    irods::server_properties::instance().capture();
    log::agent_factory::set_level(log::get_level_from_config(irods::CFG_LOG_LEVEL_CATEGORY_AGENT_FACTORY_KW));

    // Attach the error stack object to the logger and release it once this function returns.
    log::set_server_type("agent_factory");
    log::set_error_object(&rsComm.rError);

    log::agent_factory::info("Initializing ...");

    irods::at_scope_exit release_error_stack{[] {
        log::set_error_object(nullptr);
    }};

    log::agent_factory::trace("Configuring signals ...");

    signal( SIGINT, irodsAgentSignalExit );
    signal( SIGHUP, irodsAgentSignalExit );
    signal( SIGTERM, irodsAgentSignalExit );
    /* set to SIG_DFL as recommended by andy.salnikov so that system()
     * call returns real values instead of 1 */
    signal( SIGCHLD, SIG_DFL );
    signal( SIGUSR1, irodsAgentSignalExit );
    signal( SIGPIPE, SIG_IGN );

    // register irods signal handlers
    register_handlers();

    initProcLog();

    int listen_socket, conn_socket, conn_tmp_socket;
    struct sockaddr_un client_addr;
    unsigned int len = sizeof(agent_addr);

    listen_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if ( listen_socket < 0 ) {
        rodsLog( LOG_ERROR, "Unable to create socket in runIrodsAgent, errno = [%d]: %s", errno, strerror( errno ) );
        return SYS_SOCK_OPEN_ERR;
    }

    // Delete socket if it already exists
    unlink( agent_addr.sun_path );

    if ( bind( listen_socket, (struct sockaddr*) &agent_addr, len ) < 0 ) {
        rodsLog( LOG_ERROR, "Unable to bind socket in runIrodsAgent, errno [%d]: %s", errno, strerror( errno ) );
        return SYS_SOCK_BIND_ERR;
    }

    if ( listen( listen_socket, 5) < 0 ) {
        rodsLog( LOG_ERROR, "Unable to set up socket for listening in runIrodsAgent, errno [%d]: %s", errno, strerror( errno ) );
        return SYS_SOCK_LISTEN_ERR;
    }

    conn_socket = accept( listen_socket, (struct sockaddr*) &client_addr, &len);
    if ( conn_socket < 0 ) {
        rodsLog( LOG_ERROR, "Failed to accept client socket in runIrodsAgent, errno [%d]: %s", errno, strerror( errno ) );
        return SYS_SOCK_ACCEPT_ERR;
    }

    while ( true ) {
        // Reap any zombie processes from completed agents
        int reaped_pid, child_status;
        while ( ( reaped_pid = waitpid( -1, &child_status, WNOHANG ) ) > 0 ) {
            if (WIFEXITED(child_status)) {
                const int exit_status = WEXITSTATUS(child_status);
                const int log_level = exit_status == 0 ? LOG_DEBUG : LOG_ERROR;
                rodsLog( log_level, "Agent process [%d] exited with status [%d]", reaped_pid, exit_status );
            } else if (WIFSIGNALED(child_status)) {
                const int exit_signal = WTERMSIG(child_status);
                rodsLog( LOG_ERROR, "Agent process [%d] terminated by signal [%d]", reaped_pid, exit_signal );
            } else {
                rodsLog( LOG_ERROR, "Agent process [%d] terminated with unusual status [%d]", reaped_pid, child_status );
            }
            rmProcLog( reaped_pid );
        }

        fd_set read_socket;
        FD_ZERO( &read_socket );
        FD_SET( conn_socket, &read_socket);
        struct timeval time_out;
        time_out.tv_sec  = 0;
        time_out.tv_usec = 30 * 1000;
        const int ready = select(conn_socket + 1, &read_socket, nullptr, nullptr, &time_out);
        // Check the ready socket
        if ( ready == -1 && errno == EINTR ) {
            // Caught a signal, return to the select() call
            rodsLog( LOG_DEBUG, "select() was interrupted in the agent factory process, continuing..." );
            continue;
        } else if ( ready == -1 ) {
            // select() failed, quit
            rodsLog( LOG_ERROR, "select() failed with errno = [%d]: %s", errno, strerror( errno ) );
            return SYS_SOCK_SELECT_ERR;
        } else if (ready == 0) {
            continue;
        } else {
            // select returned, attempt to receive data
            // If 0 bytes are received, socket has been closed
            // If a socket address is on the line, create it and fork a child process
            char in_buf[1024]{};
            int tmp_socket{};
            const ssize_t bytes_received = recv( conn_socket, &in_buf, sizeof(in_buf), 0 );
            if ( bytes_received == -1 ) {
                rodsLog(LOG_ERROR, "Error receiving data from rodsServer, errno = [%d]: %s", errno, strerror( errno ) );
                return SYS_SOCK_READ_ERR;
            } else if ( bytes_received == 0 ) {
                // The socket peer has shut down
                rodsLog(LOG_NOTICE, "The rodsServer socket peer has shut down");
                return 0;
            } else {
                // Assume that we have received valid data over the socket connection
                // Set up the temporary (per-agent) sockets
                sockaddr_un tmp_socket_addr{};
                tmp_socket_addr.sun_family = AF_UNIX;
                strncpy( tmp_socket_addr.sun_path, in_buf, sizeof(tmp_socket_addr.sun_path) );
                unsigned int len = sizeof(tmp_socket_addr);

                tmp_socket = socket( AF_UNIX, SOCK_STREAM, 0 );

                // Delete socket if it already exists
                unlink( tmp_socket_addr.sun_path );

                if (-1 == bind(tmp_socket, (struct sockaddr*) &tmp_socket_addr, len) ) {
                    const auto err{ERROR(SYS_SOCK_BIND_ERR, "Unable to bind socket in receiveDataFromServer")};
                    irods::log(err);
                    return err.code();
                }

                if (-1 == listen(tmp_socket, 5)) {
                    const auto err{ERROR(SYS_SOCK_LISTEN_ERR, "Failed to set up socket for listening in receiveDataFromServer")};
                    irods::log(err);
                    return err.code();
                }

                // Send acknowledgement that socket has been created
                char ack_buffer[256]{};
                len = snprintf( ack_buffer, sizeof(ack_buffer), "OK" );
                const auto bytes_sent{send(conn_socket, ack_buffer, len, 0)};
                if (bytes_sent < 0) {
                    rodsLog(LOG_ERROR, "[%s] - Error sending acknowledgment to rodsServer, errno = [%d][%s]", __FUNCTION__, errno, strerror(errno));
                    return SYS_SOCK_READ_ERR;
                }

                // Wait for connection message from main server
                memset(in_buf, 0, sizeof(in_buf));
                recv(conn_socket, in_buf, sizeof(in_buf), 0);
                if (0 != std::string(in_buf).compare("connection_successful")) {
                    rodsLog(LOG_ERROR, "[%s:%d] - received failure message in connecting to socket from server", __FUNCTION__, __LINE__);
                    status = close( tmp_socket );
                    if (status < 0) {
                        rodsLog(LOG_ERROR, "close(tmp_socket) failed with errno = [%d]: %s", errno, strerror(errno));
                    }
                    continue;
                }

                conn_tmp_socket = accept( tmp_socket, (struct sockaddr*) &tmp_socket_addr, &len);
                if (-1 == conn_tmp_socket) {
                    const auto err{ERROR(SYS_SOCK_ACCEPT_ERR, "Failed to accept client socket in receiveDataFromServer")};
                    irods::log(err);
                    return err.code();
                }
            }

            // Data is ready on conn_socket, fork a child process to handle it
            log::agent_factory::trace("Spawning agent to handle request ...");
            pid_t child_pid = fork();
            if ( child_pid == 0 ) {
                log::set_server_type("agent");

                // Child process - reload properties and receive data from server process
                irods::environment_properties::instance().capture();

                status = receiveDataFromServer(conn_tmp_socket);
                if (status < 0) {
                    const auto err{ERROR(status, "Error in receiveDataFromServer")};
                    irods::log(err);
                    //return err.code();
                }

                irods::server_properties::instance().capture();

                log::agent::set_level(log::get_level_from_config(irods::CFG_LOG_LEVEL_CATEGORY_AGENT_KW));
                log::legacy::set_level(log::get_level_from_config(irods::CFG_LOG_LEVEL_CATEGORY_LEGACY_KW));
                log::resource::set_level(log::get_level_from_config(irods::CFG_LOG_LEVEL_CATEGORY_RESOURCE_KW));
                log::database::set_level(log::get_level_from_config(irods::CFG_LOG_LEVEL_CATEGORY_DATABASE_KW));
                log::authentication::set_level(log::get_level_from_config(irods::CFG_LOG_LEVEL_CATEGORY_AUTHENTICATION_KW));
                log::api::set_level(log::get_level_from_config(irods::CFG_LOG_LEVEL_CATEGORY_API_KW));
                log::microservice::set_level(log::get_level_from_config(irods::CFG_LOG_LEVEL_CATEGORY_MICROSERVICE_KW));
                log::network::set_level(log::get_level_from_config(irods::CFG_LOG_LEVEL_CATEGORY_NETWORK_KW));
                log::rule_engine::set_level(log::get_level_from_config(irods::CFG_LOG_LEVEL_CATEGORY_RULE_ENGINE_KW));

                log::agent::info("I'm an agent!");

                irods::error ret2 = setRECacheSaltFromEnv();
                if ( !ret2.ok() ) {
                    rodsLog( LOG_ERROR, "rodsAgent::main: Failed to set RE cache mutex name\n%s", ret2.result().c_str() );
                    return SYS_INTERNAL_ERR;
                }

                break;
            } else if ( child_pid > 0 ) {
                // Parent process - want to return to select() call
                status = close( conn_tmp_socket );
                if ( status < 0 ) {
                    rodsLog( LOG_ERROR, "close(conn_tmp_socket) failed with errno = [%d]: %s", errno, strerror( errno ) );
                }

                status = close( tmp_socket );
                if ( status < 0 ) {
                    rodsLog( LOG_ERROR, "close(tmp_socket) failed with errno = [%d]: %s", errno, strerror( errno ) );
                }

                continue;
            } else {
                rodsLog( LOG_ERROR, "fork() failed in rodsAgent process factory" );

                status = close( conn_socket );
                if ( status < 0 ) {
                    rodsLog( LOG_ERROR, "close(conn_socket) failed with errno = [%d]: %s", errno, strerror( errno ) );
                }

                status = close( listen_socket );
                if ( status < 0 ) {
                    rodsLog( LOG_ERROR, "close(listen_socket) failed with errno = [%d]: %s", errno, strerror( errno ) );
                }

                return SYS_FORK_ERROR;
            }
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

    irods::re_plugin_globals.reset(new irods::global_re_plugin_mgr);
    irods::re_plugin_globals->global_re_mgr.call_start_operations();

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
        status = chkAllowedUser( rsComm.clientUser.userName, rsComm.clientUser.rodsZone );

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

    const int log_level = status == 0 ? LOG_DEBUG : LOG_ERROR;
    rodsLog( log_level, "Agent [%d] exiting with status = %d", getpid(), status );
    return status;
}

static void set_rule_engine_globals( rsComm_t* _comm )
{
    irods::set_server_property<std::string>(irods::CLIENT_USER_NAME_KW, _comm->clientUser.userName);
    irods::set_server_property<std::string>(irods::CLIENT_USER_ZONE_KW, _comm->clientUser.rodsZone);
    irods::set_server_property<int>(irods::CLIENT_USER_PRIV_KW, _comm->clientUser.authInfo.authFlag);
    irods::set_server_property<std::string>(irods::PROXY_USER_NAME_KW, _comm->proxyUser.userName);
    irods::set_server_property<std::string>(irods::PROXY_USER_ZONE_KW, _comm->proxyUser.rodsZone);
    irods::set_server_property<int>(irods::PROXY_USER_PRIV_KW, _comm->clientUser.authInfo.authFlag);
} // set_rule_engine_globals

int agentMain( rsComm_t *rsComm )
{
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
