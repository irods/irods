// =-=-=-=-=-=-=-
// irods includes
#define USE_SSL 1
#include "sslSockComm.hpp"

#include "rodsDef.h"
#include "msParam.hpp"
#include "reGlobalsExtern.hpp"
#include "rcConnect.hpp"
#include "authRequest.hpp"
#include "authResponse.hpp"
#include "authCheck.hpp"
#include "miscServerFunct.hpp"
#include "authPluginRequest.hpp"
#include "icatHighLevelRoutines.hpp"

// =-=-=-=-=-=-=-
#include "irods_auth_plugin.hpp"
#include "irods_auth_constants.hpp"
#include "irods_pam_auth_object.hpp"
#include "irods_stacktrace.hpp"
#include "irods_kvp_string_parser.hpp"
#include "irods_client_server_negotiation.hpp"

// =-=-=-=-=-=-=-
// boost includes
#include "boost/lexical_cast.hpp"

// =-=-=-=-=-=-=-
// stl includes
#include <sstream>
#include <string>
#include <iostream>
#include <termios.h>
#include <unistd.h>

// =-=-=-=-=-=-=-
// system includes
#include <sys/types.h>
#include <sys/wait.h>


int get64RandomBytes( char *buf );

extern "C" {
    // =-=-=-=-=-=-=-
    // NOTE:: this needs to become a property
    // Set requireServerAuth to 1 to fail authentications from
    // un-authenticated Servers (for example, if the LocalZoneSID
    // is not set)
    const int requireServerAuth = 0;

    // =-=-=-=-=-=-=-
    // establish context - take the auth request results and massage them
    // for the auth response call
    irods::error pam_auth_client_start(
        irods::auth_plugin_context& _ctx,
        rcComm_t*                    _comm,
        const char*                  _context ) {
        irods::error result = SUCCESS();
        irods::error ret;

        // =-=-=-=-=-=-=-
        // validate incoming parameters
        ret = _ctx.valid< irods::pam_auth_object >();
        if ( ( result = ASSERT_PASS( ret, "Invalid plugin context." ) ).ok() ) {
            if ( ( result = ASSERT_ERROR( _comm, SYS_INVALID_INPUT_PARAM, "Null comm pointer." ) ).ok() ) {
                if ( ( result = ASSERT_ERROR( _context, SYS_INVALID_INPUT_PARAM, "Null context pointer." ) ).ok() ) {

                    // =-=-=-=-=-=-=-
                    // simply cache the context string for a rainy day...
                    // or to pass to the auth client call later.
                    irods::pam_auth_object_ptr ptr = boost::dynamic_pointer_cast<irods::pam_auth_object >( _ctx.fco() );
                    ptr->context( _context );

                    // =-=-=-=-=-=-=-
                    // parse the kvp out of the _resp->username string
                    irods::kvp_map_t kvp;
                    irods::error ret = irods::parse_kvp_string( _context, kvp );
                    if ( ( result = ASSERT_PASS( ret, "Failed to parse the key-value pairs." ) ).ok() ) {

                        std::string password = kvp[ irods::AUTH_PASSWORD_KEY ];
                        std::string ttl_str  = kvp[ irods::AUTH_TTL_KEY ];

                        // =-=-=-=-=-=-=-
                        // prompt for a password if necessary
                        char new_password[ MAX_PASSWORD_LEN + 2 ];
                        if ( password.empty() ) {
#ifdef WIN32
                            HANDLE hStdin = GetStdHandle( STD_INPUT_HANDLE );
                            DWORD mode;
                            GetConsoleMode( hStdin, &mode );
                            DWORD lastMode = mode;
                            mode &= ~ENABLE_ECHO_INPUT;
                            BOOL error = !SetConsoleMode( hStdin, mode );
                            int errsv = -1;
#else
                            struct termios tty;
                            tcgetattr( STDIN_FILENO, &tty );
                            tcflag_t oldflag = tty.c_lflag;
                            tty.c_lflag &= ~ECHO;
                            int error = tcsetattr( STDIN_FILENO, TCSANOW, &tty );
                            int errsv = errno;
#endif
                            if ( error ) {
                                printf( "WARNING: Error %d disabling echo mode. Password will be displayed in plaintext.", errsv );
                            }
                            printf( "Enter your current PAM password:" );
                            std::string password = "";
                            getline( std::cin, password );
                            strncpy( new_password, password.c_str(), MAX_PASSWORD_LEN );
#ifdef WIN32
                            if ( !SetConsoleMode( hStdin, lastMode ) ) {
                                printf( "Error reinstating echo mode." );
                            }
#else
                            tty.c_lflag = oldflag;
                            if ( tcsetattr( STDIN_FILENO, TCSANOW, &tty ) ) {
                                printf( "Error reinstating echo mode." );
                            }
#endif

                            // =-=-=-=-=-=-=-
                            // rebuilt and reset context string
                            std::string context = irods::AUTH_TTL_KEY        +
                                                  irods::kvp_association()  +
                                                  ttl_str                    +
                                                  irods::kvp_delimiter()    +
                                                  irods::AUTH_PASSWORD_KEY  +
                                                  irods::kvp_association()  +
                                                  new_password;
                            ptr->context( context );

                        }


                        // =-=-=-=-=-=-=-
                        // set the user name from the conn
                        ptr->user_name( _comm->proxyUser.userName );

                        // =-=-=-=-=-=-=-
                        // set the zone name from the conn
                        ptr->zone_name( _comm->proxyUser.rodsZone );
                    }
                }
            }
        }

        return result;

    } // pam_auth_client_start

    // =-=-=-=-=-=-=-
    // handle an agent-side auth request call
    irods::error pam_auth_client_request(
        irods::auth_plugin_context& _ctx,
        rcComm_t*                    _comm ) {
        // =-=-=-=-=-=-=-
        // validate incoming parameters
        if ( !_ctx.valid< irods::pam_auth_object >().ok() ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "invalid plugin context" );

        }
        else if ( !_comm ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "null comm ptr" );

        }

        // =-=-=-=-=-=-=-
        // get the auth object
        irods::pam_auth_object_ptr ptr = boost::dynamic_pointer_cast <
                                         irods::pam_auth_object > ( _ctx.fco() );
        // =-=-=-=-=-=-=-
        // get the context string
        std::string context = ptr->context( );
        if ( context.empty() ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "empty plugin context string" );
        }

        // =-=-=-=-=-=-=-
        // append the auth scheme and user name
        context += irods::kvp_delimiter()   +
                   irods::AUTH_USER_KEY      +
                   irods::kvp_association() +
                   ptr->user_name();

        // =-=-=-=-=-=-=-
        // error check string size against MAX_NAME_LEN
        if ( context.size() > MAX_NAME_LEN ) {
            return ERROR(
                       -1,
                       "context string > max name len" );
        }

        // =-=-=-=-=-=-=-
        // copy the context to the req in struct
        authPluginReqInp_t req_in;
        strncpy(
            req_in.context_,
            context.c_str(),
            context.size() + 1 );

        // =-=-=-=-=-=-=-
        // copy the auth scheme to the req in struct
        strncpy(
            req_in.auth_scheme_,
            irods::AUTH_PAM_SCHEME.c_str(),
            irods::AUTH_PAM_SCHEME.size() + 1 );

        // =-=-=-=-=-=-=-
        // check to see if SSL is currently in place
        bool using_ssl = ( irods::CS_NEG_USE_SSL == _comm->negotiation_results );

        // =-=-=-=-=-=-=-
        // warm up SSL if it is not already in use
        if ( !using_ssl ) {
            int err = sslStart( _comm );
            if ( err ) {
                return ERROR( err, "failed to enable ssl" );
            }
        }

        // =-=-=-=-=-=-=-
        // make the call to our auth request
        authPluginReqOut_t* req_out = 0;
        int status = rcAuthPluginRequest( _comm, &req_in, &req_out );

        // =-=-=-=-=-=-=-
        // shut down SSL if it was not already in use
        if ( !using_ssl )  {
            sslEnd( _comm );
        }

        // =-=-=-=-=-=-=-
        // handle errors and exit
        if ( status < 0 ) {
            return ERROR( status, "call to rcAuthRequest failed." );
        }
        else {
            // =-=-=-=-=-=-=-
            // copy over the resulting irods pam pasword
            // and cache the result in our auth object
            ptr->request_result( req_out->result_ );
            status = obfSavePw( 0, 0, 0, req_out->result_ );
            free( req_out );
            return SUCCESS();

        }

    } // pam_auth_client_request

    /// =-=-=-=-=-=-=-
    /// @brief function to run the local exec which will
    ///        actually do the auth check for us
#ifndef PAM_AUTH_CHECK_PROG
#define PAM_AUTH_CHECK_PROG  "./PamAuthCheck"
#endif
    int run_pam_auth_check(
        const std::string& _username,
        const std::string& _password ) {

        int p2cp[2]; /* parent to child pipe */
        int pid, i;
        int status;

        if ( pipe( p2cp ) < 0 ) {
            return SYS_PIPE_ERROR;
        }
        pid = fork();
        if ( pid == -1 ) {
            return SYS_FORK_ERROR;
        }

        if ( pid )  {
            /*
               This is still the parent.  Write the message to the child and
               then wait for the exit and status.
            */
            if ( write( p2cp[1], _password.c_str(), _password.size() ) == -1 ) {
                int errsv = errno;
                irods::log( ERROR( errsv, "Error writing from parent to child." ) );
            }
            close( p2cp[1] );
            waitpid( pid, &status, 0 );
            return status;
        }
        else {
            /* This is the child */
            if ( dup2( p2cp[0], STDIN_FILENO ) == -1 ) { /* Make stdin come from read end of the pipe */
                int errsv = errno;
                irods::log( ERROR( errsv, "Error duplicating the file descriptor." ) );
            }
            close( p2cp[1] );
            i = execl( PAM_AUTH_CHECK_PROG, PAM_AUTH_CHECK_PROG, _username.c_str(),
                       ( char * )NULL );
            perror( "execl" );
            printf( "execl failed %d\n", i );
        }
        return ( SYS_FORK_ERROR ); /* avoid compiler warning */

    } // run_pam_auth_check

    // =-=-=-=-=-=-=-
    // handle an agent-side auth request call
    irods::error pam_auth_agent_request(
        irods::auth_plugin_context& _ctx,
        rsComm_t*                    _comm ) {

        // =-=-=-=-=-=-=-
        // validate incoming parameters
        if ( !_ctx.valid< irods::pam_auth_object >().ok() ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "invalid plugin context" );
        }
        else if ( !_comm ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "null comm ptr" );
        }

        // =-=-=-=-=-=-=-
        // get the server host handle
        rodsServerHost_t* server_host = 0;
        int status = getAndConnRcatHost( _comm, MASTER_RCAT, ( const char* )_comm->clientUser.rodsZone, &server_host );
        if ( status < 0 ) {
            return ERROR( status, "getAndConnRcatHost failed." );
        }

        // =-=-=-=-=-=-=-
        // simply cache the context string for a rainy day...
        // or to pass to the auth client call later.
        irods::pam_auth_object_ptr ptr = boost::dynamic_pointer_cast <
                                         irods::pam_auth_object > ( _ctx.fco() );
        std::string context = ptr->context( );

        // =-=-=-=-=-=-=-
        // if we are not the catalog server, redirect the call
        // to there
        if ( server_host->localFlag != LOCAL_HOST ) {
            // =-=-=-=-=-=-=-
            // protect the PAM plain text password by
            // using an SSL connection to the remote ICAT
            status = sslStart( server_host->conn );
            if ( status ) {
                return ERROR( status, "could not establish SSL connection" );
            }

            // =-=-=-=-=-=-=-
            // manufacture structures for the redirected call
            authPluginReqOut_t* req_out = 0;
            authPluginReqInp_t  req_inp;
            strncpy( req_inp.auth_scheme_, irods::AUTH_PAM_SCHEME.c_str(), irods::AUTH_PAM_SCHEME.size() + 1 );
            strncpy( req_inp.context_, context.c_str(), context.size() + 1 );

            // =-=-=-=-=-=-=-
            // make the redirected call
            status = rcAuthPluginRequest( server_host->conn, &req_inp, &req_out );

            // =-=-=-=-=-=-=-
            // shut down ssl on the connection
            sslEnd( server_host->conn );

            // =-=-=-=-=-=-=-
            // disconnect
            rcDisconnect( server_host->conn );
            server_host->conn = NULL;
            if ( !req_out || status < 0 ) {
                return ERROR( status, "redirected rcAuthPluginRequest failed." );
            }
            else {
                // =-=-=-=-=-=-=-
                // set the result for communication back to the client
                ptr->request_result( req_out->result_ );
                if ( _comm->auth_scheme != NULL ) {
                    free( _comm->auth_scheme );
                }
                _comm->auth_scheme = strdup( irods::AUTH_PAM_SCHEME.c_str() );
                return SUCCESS();

            }

        } // if !localhost

        // =-=-=-=-=-=-=-
        // parse the kvp out of the _resp->username string
        irods::kvp_map_t kvp;
        irods::error ret = irods::parse_kvp_string(
                               context,
                               kvp );
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        if ( kvp.find( irods::AUTH_USER_KEY ) == kvp.end() ||
                kvp.find( irods::AUTH_TTL_KEY ) == kvp.end() ||
                kvp.find( irods::AUTH_PASSWORD_KEY ) == kvp.end() ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "user or ttl or password key missing" );
        }

        std::string user_name = kvp[ irods::AUTH_USER_KEY     ];
        std::string password  = kvp[ irods::AUTH_PASSWORD_KEY ];
        std::string ttl_str   = kvp[ irods::AUTH_TTL_KEY      ];
        int ttl = 0;
        if ( !ttl_str.empty() ) {
            ttl = boost::lexical_cast<int>( ttl_str );
        }

        // =-=-=-=-=-=-=-
        // Normal mode, fork/exec setuid program to do the Pam check
        status = run_pam_auth_check( user_name, password );
        if ( status == 256 ) {
            return ERROR( PAM_AUTH_PASSWORD_FAILED, "pam auth check failed" );
        }
        else if ( status ) {
            return ERROR( status, "pam auth check failed" );
        }

        // =-=-=-=-=-=-=-
        // request the resulting irods password after the handshake
        char password_out[ MAX_NAME_LEN ];
        char* pw_ptr = &password_out[0];
        status = chlUpdateIrodsPamPassword( _comm, const_cast< char* >( user_name.c_str() ), ttl, NULL, &pw_ptr );

        // =-=-=-=-=-=-=-
        // set the result for communication back to the client
        ptr->request_result( password_out );

        // =-=-=-=-=-=-=-
        // win!
        if ( _comm->auth_scheme != NULL ) {
            free( _comm->auth_scheme );
        }
        _comm->auth_scheme = strdup( "pam" );
        return SUCCESS();

    } // pam_auth_agent_request

    // =-=-=-=-=-=-=-
    // establish context - take the auth request results and massage them
    // for the auth response call
    irods::error pam_auth_establish_context(
        irods::auth_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // validate incoming parameters
        if ( !_ctx.valid< irods::pam_auth_object >().ok() ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "invalid plugin context" );

        }

        return SUCCESS();

    } // pam_auth_establish_context

    // =-=-=-=-=-=-=-
    // stub for ops that the native plug does
    // not need to support
    irods::error pam_auth_success_stub(
        irods::auth_plugin_context& ) {
        return SUCCESS();

    } // pam_auth_success_stub

    // =-=-=-=-=-=-=-
    // derive a new pam_auth auth plugin from
    // the auth plugin base class for handling
    // native authentication
    class pam_auth_plugin : public irods::auth {
        public:
            pam_auth_plugin(
                const std::string& _nm,
                const std::string& _ctx ) :
                irods::auth(
                    _nm,
                    _ctx ) {
            } // ctor

            ~pam_auth_plugin() {
            }

    }; // class pam_auth_plugin

    // =-=-=-=-=-=-=-
    // factory function to provide instance of the plugin
    irods::auth* plugin_factory(
        const std::string& _inst_name,
        const std::string& _context ) {
        // =-=-=-=-=-=-=-
        // create an auth object
        pam_auth_plugin* pam = new pam_auth_plugin(
            _inst_name,
            _context );

        // =-=-=-=-=-=-=-
        // fill in the operation table mapping call
        // names to function names
        pam->add_operation( irods::AUTH_CLIENT_START,         "pam_auth_client_start" );
        pam->add_operation( irods::AUTH_AGENT_START,          "pam_auth_success_stub" );
        pam->add_operation( irods::AUTH_ESTABLISH_CONTEXT,    "pam_auth_success_stub" );
        pam->add_operation( irods::AUTH_CLIENT_AUTH_REQUEST,  "pam_auth_client_request" );
        pam->add_operation( irods::AUTH_AGENT_AUTH_REQUEST,   "pam_auth_agent_request" );
        pam->add_operation( irods::AUTH_CLIENT_AUTH_RESPONSE, "pam_auth_success_stub" );
        pam->add_operation( irods::AUTH_AGENT_AUTH_RESPONSE,  "pam_auth_success_stub" );
        pam->add_operation( irods::AUTH_AGENT_AUTH_VERIFY,    "pam_auth_success_stub" );

        irods::auth* auth = dynamic_cast< irods::auth* >( pam );

        return auth;

    } // plugin_factory

}; // extern "C"
