/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

// =-=-=-=-=-=-=-
// irods includes
#define USE_SSL 1
#include "sslSockComm.h"

#include "rodsDef.h"
#include "msParam.h"
#include "reGlobalsExtern.h"
#include "rcConnect.h"
#include "authRequest.h"
#include "authResponse.h"
#include "authCheck.h"
#include "miscServerFunct.h"
#include "authPluginRequest.h"
#include "icatHighLevelRoutines.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_auth_plugin.h"
#include "eirods_auth_constants.h"
#include "eirods_pam_auth_object.h"
#include "eirods_stacktrace.h"
#include "eirods_kvp_string_parser.h"
#include "eirods_client_server_negotiation.h"

// =-=-=-=-=-=-=-
// boost includes
#include "boost/lexical_cast.hpp"

// =-=-=-=-=-=-=-
// stl includes
#include <sstream>
#include <string>
#include <iostream>

// =-=-=-=-=-=-=-
// system includes
#include <sys/types.h>
#include <sys/wait.h>


int get64RandomBytes(char *buf);

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
    eirods::error pam_auth_client_start(
        eirods::auth_plugin_context& _ctx,
        rcComm_t*                    _comm,
        const char*                  _context )
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        // =-=-=-=-=-=-=-
        // validate incoming parameters
        ret = _ctx.valid< eirods::pam_auth_object >();
        if((result = ASSERT_PASS(ret, "Invalid plugin context.")).ok() ) {
            if((result = ASSERT_ERROR(_comm, SYS_INVALID_INPUT_PARAM, "Null comm pointer." )).ok()) {
        
                // =-=-=-=-=-=-=-
                // simply cache the context string for a rainy day...
                // or to pass to the auth client call later.
                eirods::pam_auth_object_ptr ptr = boost::dynamic_pointer_cast<eirods::pam_auth_object >( _ctx.fco() );
                if( _context ) {
                    ptr->context( _context );
        
                }
         
                // =-=-=-=-=-=-=-
                // parse the kvp out of the _resp->username string
                eirods::kvp_map_t kvp;
                eirods::error ret = eirods::parse_kvp_string(_context, kvp );
                if((result = ASSERT_PASS(ret, "Failed to parse the key-value pairs.")).ok() ) {

                    std::string password = kvp[ eirods::AUTH_PASSWORD_KEY ];
                    std::string ttl_str  = kvp[ eirods::AUTH_TTL_KEY ];

                    // =-=-=-=-=-=-=-
                    // prompt for a password if necessary
                    char new_password[ MAX_PASSWORD_LEN + 2 ];
                    if( password.empty() ) {
                        int doStty=0;
                        path p ("/bin/stty");
                        if (exists(p)) {
                            system("/bin/stty -echo 2> /dev/null");
                            doStty=1;
                        }
                        printf("Enter your current PAM password:");
                        fgets( new_password, sizeof( new_password ), stdin );
                        if( doStty ) {
                            system("/bin/stty echo 2> /dev/null");
                            printf("\n");
                        }
            
                        int len = strlen( new_password );
                        new_password[len - 1]='\0'; // remove trailing \n 

                        // =-=-=-=-=-=-=-
                        // rebuilt and reset context string
                        std::string context = eirods::AUTH_TTL_KEY        + 
                            eirods::kvp_association()  +
                            ttl_str                    +
                            eirods::kvp_delimiter()    +
                            eirods::AUTH_PASSWORD_KEY  +
                            eirods::kvp_association()  +
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
        
        return result;
         
    } // pam_auth_client_start

    // =-=-=-=-=-=-=-
    // handle an agent-side auth request call 
    eirods::error pam_auth_client_request(
        eirods::auth_plugin_context& _ctx,
        rcComm_t*                    _comm ) {
        // =-=-=-=-=-=-=-
        // validate incoming parameters
        if( !_ctx.valid< eirods::pam_auth_object >().ok() ) {
            return ERROR( 
                SYS_INVALID_INPUT_PARAM,
                "invalid plugin context" );

        } else if( !_comm ) {
            return ERROR( 
                SYS_INVALID_INPUT_PARAM,
                "null comm ptr" );

        }
        
        // =-=-=-=-=-=-=-
        // get the auth object
        eirods::pam_auth_object_ptr ptr = boost::dynamic_pointer_cast< 
            eirods::pam_auth_object >( _ctx.fco() );
        // =-=-=-=-=-=-=-
        // get the context string
        std::string context = ptr->context( );
        if( context.empty() ) {
            return ERROR( 
                SYS_INVALID_INPUT_PARAM,
                "empty plugin context string" );
        }

        // =-=-=-=-=-=-=-
        // append the auth scheme and user name
        context += eirods::kvp_delimiter()   +
            eirods::AUTH_USER_KEY      +
            eirods::kvp_association() + 
            ptr->user_name();
                    
        // =-=-=-=-=-=-=-
        // error check string size against MAX_NAME_LEN
        if( context.size() > MAX_NAME_LEN ) {
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
            context.size()+1 );
                   
        // =-=-=-=-=-=-=-
        // copy the auth scheme to the req in struct
        strncpy( 
            req_in.auth_scheme_,
            eirods::AUTH_PAM_SCHEME.c_str(),
            eirods::AUTH_PAM_SCHEME.size()+1 );
        
        // =-=-=-=-=-=-=-
        // check to see if SSL is currently in place
        bool using_ssl = ( eirods::CS_NEG_USE_SSL == _comm->negotiation_results );
            
        // =-=-=-=-=-=-=-
        // warm up SSL if it is not already in use
        if( !using_ssl ) {
            int err = sslStart( _comm );
            if( err ) {
                return ERROR( 
                    err,
                    "failed to enable ssl" );
            }
        }

        // =-=-=-=-=-=-=-
        // make the call to our auth request
        authPluginReqOut_t* req_out = 0;
        int status = rcAuthPluginRequest( 
            _comm,
            &req_in,
            &req_out );
        
        // =-=-=-=-=-=-=-
        // shut down SSL if it was not already in use
        if( !using_ssl )  {
            sslEnd( _comm );
        }

        // =-=-=-=-=-=-=-
        // handle errors and exit
        if( status < 0 ) {
            return ERROR( 
                status,
                "call to rcAuthRequest failed." );
        
        } else {
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

        if (pipe(p2cp) < 0) {
            return(SYS_PIPE_ERROR);
        }
        pid = fork();
        if (pid == -1) {
            return(SYS_FORK_ERROR);
        }

        if (pid)  {
            /* 
               This is still the parent.  Write the message to the child and
               then wait for the exit and status.
            */   
            write( p2cp[1], _password.c_str(), _password.size() );
            close( p2cp[1] );
            waitpid( pid, &status, 0 );
            return status;
        } else {
            /* This is the child */
            close(0);          /* close current stdin */
            dup (p2cp[0]);     /* Make stdin come from read end of the pipe */
            close (p2cp[1]);
            i = execl(PAM_AUTH_CHECK_PROG, PAM_AUTH_CHECK_PROG, _username.c_str(),
                      (char *)NULL);
            perror("execl");
            printf("execl failed %d\n",i);
        }
        return(SYS_FORK_ERROR); /* avoid compiler warning */

    } // run_pam_auth_check

    // =-=-=-=-=-=-=-
    // handle an agent-side auth request call 
    eirods::error pam_auth_agent_request(
        eirods::auth_plugin_context& _ctx,
        rsComm_t*                    _comm ) {
        // =-=-=-=-=-=-=-
        // validate incoming parameters
        if( !_ctx.valid< eirods::pam_auth_object >().ok() ) {
            return ERROR( 
                SYS_INVALID_INPUT_PARAM,
                "invalid plugin context" );
        } else if( !_comm ) {
            return ERROR( 
                SYS_INVALID_INPUT_PARAM,
                "null comm ptr" );
        } 

        // =-=-=-=-=-=-=-
        // get the server host handle
        rodsServerHost_t* server_host = 0;
        int status = getAndConnRcatHost( 
            _comm, 
            MASTER_RCAT, 
            _comm->clientUser.rodsZone, 
            &server_host );
        if( status < 0 ) {
            return ERROR(
                status,
                "getAndConnRcatHost failed." );
        }
  
        // =-=-=-=-=-=-=-
        // simply cache the context string for a rainy day...
        // or to pass to the auth client call later.
        eirods::pam_auth_object_ptr ptr = boost::dynamic_pointer_cast< 
            eirods::pam_auth_object >( _ctx.fco() );
        std::string context = ptr->context( );

        // =-=-=-=-=-=-=-
        // if we are not the catalog server, redirect the call
        // to there
        if( server_host->localFlag != LOCAL_HOST) {
            // =-=-=-=-=-=-=-
            // protect the PAM plain text password by
            // using an SSL connection to the remote ICAT
            status = sslStart( server_host->conn );
            if( status ) {
                return ERROR(
                    status,
                    "could not establish SSL connection" );
            }

            // =-=-=-=-=-=-=-
            // manufacture structures for the redirected call
            authPluginReqOut_t* req_out = 0;
            authPluginReqInp_t  req_inp;
            strncpy( 
                req_inp.auth_scheme_,
                eirods::AUTH_PAM_SCHEME.c_str(),
                eirods::AUTH_PAM_SCHEME.size()+1 );
            strncpy( 
                req_inp.context_,
                context.c_str(),
                context.size()+1 );
                    
            // =-=-=-=-=-=-=-
            // make the redirected call
            status = rcAuthPluginRequest(
                server_host->conn, 
                &req_inp,
                &req_out );
           
            // =-=-=-=-=-=-=-
            // shut down ssl on the connection 
            sslEnd( server_host->conn );
            
            // =-=-=-=-=-=-=-
            // disconnect
            rcDisconnect( server_host->conn );
            server_host->conn = NULL;
            if( !req_out || status < 0 ) {
                return ERROR(
                    status,
                    "redirected rcAuthPluginRequest failed." );    
            } else { 
                // =-=-=-=-=-=-=-
                // set the result for communication back to the client
                ptr->request_result( req_out->result_ );
                return SUCCESS();

            }

        } // if !localhost

        // =-=-=-=-=-=-=-
        // parse the kvp out of the _resp->username string
        eirods::kvp_map_t kvp;
        eirods::error ret = eirods::parse_kvp_string(
            context,
            kvp );
        if( !ret.ok() ) {
            return PASS( ret );
        }

        if( kvp.find( eirods::AUTH_USER_KEY     ) == kvp.end() ||
            kvp.find( eirods::AUTH_TTL_KEY      ) == kvp.end() ||
            kvp.find( eirods::AUTH_PASSWORD_KEY ) == kvp.end() ) {
            return ERROR( 
                SYS_INVALID_INPUT_PARAM,
                "user or ttl or password key missing" );
        }

        std::string user_name = kvp[ eirods::AUTH_USER_KEY     ];
        std::string password  = kvp[ eirods::AUTH_PASSWORD_KEY ];
        std::string ttl_str   = kvp[ eirods::AUTH_TTL_KEY      ];
        int ttl = 0;
        if( !ttl_str.empty() ) {
            ttl = boost::lexical_cast<int>( ttl_str );
        }

        // =-=-=-=-=-=-=-
        // Normal mode, fork/exec setuid program to do the Pam check
        status = run_pam_auth_check( 
            user_name, 
            password );
        if( status == 256 ) {
            return ERROR(
                PAM_AUTH_PASSWORD_FAILED,
                "pam auth check failed" );
        } else if( status ) {
            return ERROR( 
                status,
                "pam auth check failed" );
        }
 
        // =-=-=-=-=-=-=-
        // request the resulting irods password after the handshake
        char password_out[ MAX_NAME_LEN ];
        char* pw_ptr = &password_out[0];
        status = chlUpdateIrodsPamPassword(
            _comm,
            const_cast< char* >( user_name.c_str() ),
            ttl,
            NULL,
            &pw_ptr );
        
        // =-=-=-=-=-=-=-
        // set the result for communication back to the client
        ptr->request_result( password_out );

        // =-=-=-=-=-=-=-
        // win!
        return SUCCESS();              
         
    } // pam_auth_agent_request

    // =-=-=-=-=-=-=-
    // establish context - take the auth request results and massage them
    // for the auth response call
    eirods::error pam_auth_establish_context(
        eirods::auth_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // validate incoming parameters
        if( !_ctx.valid< eirods::pam_auth_object >().ok() ) {
            return ERROR( 
                SYS_INVALID_INPUT_PARAM,
                "invalid plugin context" );
        
        }
 
        return SUCCESS();

    } // pam_auth_establish_context

    // =-=-=-=-=-=-=-
    // stub for ops that the native plug does 
    // not need to support 
    eirods::error pam_auth_success_stub( 
        eirods::auth_plugin_context& _ctx ) {
        return SUCCESS();

    } // pam_auth_success_stub

    // =-=-=-=-=-=-=-
    // derive a new pam_auth auth plugin from
    // the auth plugin base class for handling
    // native authentication
    class pam_auth_plugin : public eirods::auth {
    public:
        pam_auth_plugin( 
            const std::string& _nm, 
            const std::string& _ctx ) :
            eirods::auth( 
                _nm, 
                _ctx ) {
        } // ctor

        ~pam_auth_plugin() {
        }

    }; // class pam_auth_plugin

    // =-=-=-=-=-=-=-
    // factory function to provide instance of the plugin
    eirods::auth* plugin_factory( 
        const std::string& _inst_name, 
        const std::string& _context ) {
        // =-=-=-=-=-=-=-
        // create an auth object
        pam_auth_plugin* pam = new pam_auth_plugin( 
            _inst_name,
            _context );
        if( !pam ) {
            rodsLog( LOG_ERROR, "plugin_factory - failed to alloc pam_auth_plugin" );
            return 0;
        }
        
        // =-=-=-=-=-=-=-
        // fill in the operation table mapping call 
        // names to function names
        pam->add_operation( eirods::AUTH_CLIENT_START,         "pam_auth_client_start"   );
        pam->add_operation( eirods::AUTH_AGENT_START,          "pam_auth_success_stub"   );
        pam->add_operation( eirods::AUTH_ESTABLISH_CONTEXT,    "pam_auth_success_stub"   );
        pam->add_operation( eirods::AUTH_CLIENT_AUTH_REQUEST,  "pam_auth_client_request" );
        pam->add_operation( eirods::AUTH_AGENT_AUTH_REQUEST,   "pam_auth_agent_request"  );
        pam->add_operation( eirods::AUTH_CLIENT_AUTH_RESPONSE, "pam_auth_success_stub"   );
        pam->add_operation( eirods::AUTH_AGENT_AUTH_RESPONSE,  "pam_auth_success_stub"   );
        pam->add_operation( eirods::AUTH_AGENT_AUTH_VERIFY,    "pam_auth_success_stub"   );

        eirods::auth* auth = dynamic_cast< eirods::auth* >( pam );
        if( !auth ) {
            rodsLog( LOG_ERROR, "failed to dynamic cast to eirods::auth*" );
        }

        return auth;

    } // plugin_factory

}; // extern "C"
