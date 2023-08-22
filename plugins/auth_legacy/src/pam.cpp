// =-=-=-=-=-=-=-
// irods includes
#define USE_SSL 1
#include "irods/sslSockComm.h"

#include "irods/rodsDef.h"
#include "irods/msParam.h"
#include "irods/rcConnect.h"
#include "irods/authRequest.h"
#include "irods/authResponse.h"
#include "irods/authCheck.h"
#include "irods/miscServerFunct.hpp"
#include "irods/authPluginRequest.h"
#include "irods/icatHighLevelRoutines.hpp"

// =-=-=-=-=-=-=-
#include "irods/irods_auth_plugin.hpp"
#include "irods/irods_auth_constants.hpp"
#include "irods/irods_pam_auth_object.hpp"
#include "irods/irods_stacktrace.hpp"
#include "irods/irods_kvp_string_parser.hpp"
#include "irods/irods_client_server_negotiation.hpp"

// =-=-=-=-=-=-=-
// boost includes
#include <boost/lexical_cast.hpp>

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

// establish context - take the auth request results and massage them
// for the auth response call
irods::error pam_auth_client_start(irods::plugin_context& _ctx,
                                   rcComm_t*              _comm,
                                   const char*            _context)
{
    if (const auto err = _ctx.valid<irods::pam_auth_object>(); !err.ok()) {
        return PASSMSG("Invalid plugin context.", err);
    }

    if (!_comm) {
        return ERROR(SYS_INVALID_INPUT_PARAM, "Null comm pointer.");
    }

    if (!_context) {
        return ERROR(SYS_INVALID_INPUT_PARAM, "Null context pointer.");
    }

    // parse the kvp out of the _resp->username string
    irods::kvp_map_t kvp;
    if (const auto err = irods::parse_escaped_kvp_string(_context, kvp); !err.ok()) {
        return PASSMSG("Failed to parse the key-value pairs.", err);
    }

    // simply cache the context string for a rainy day... or to pass to the auth client call later.
    irods::pam_auth_object_ptr ptr = boost::dynamic_pointer_cast<irods::pam_auth_object>(_ctx.fco());
    ptr->context(_context);

    std::string password = kvp[ irods::AUTH_PASSWORD_KEY ];
    std::string ttl_str  = kvp[ irods::AUTH_TTL_KEY ];

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
        printf( "\n" );
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

        // rebuilt and reset context string
        irods::kvp_map_t ctx_map;
        ctx_map[irods::AUTH_TTL_KEY] = ttl_str;
        ctx_map[irods::AUTH_PASSWORD_KEY] = new_password;
        std::string ctx_str = irods::escaped_kvp_string(
                ctx_map);
        ptr->context( ctx_str );

    }

    ptr->user_name( _comm->proxyUser.userName );
    ptr->zone_name( _comm->proxyUser.rodsZone );

    return SUCCESS();
} // pam_auth_client_start

// =-=-=-=-=-=-=-
// handle an agent-side auth request call
irods::error pam_auth_client_request(
    irods::plugin_context& _ctx,
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
    // expand the context string then append the auth scheme
    // and user name, then re-encode into a string
    irods::kvp_map_t ctx_map;
    irods::error ret = irods::parse_escaped_kvp_string(
                           context,
                           ctx_map);
    if( !ret.ok() ) {
        return PASS(ret);
    }

    ctx_map[irods::AUTH_USER_KEY]=ptr->user_name();
    std::string ctx_str = irods::escaped_kvp_string(
                              ctx_map);

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
        ctx_str.c_str(),
        ctx_str.size() + 1 );

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
        // copy over the resulting irods pam password
        // and cache the result in our auth object
        ptr->request_result( req_out->result_ );
        status = obfSavePw( 0, 0, 0, req_out->result_ );
        free( req_out );
        if (status < 0) {
            return ERROR(status, "obfSavePw failed");
        }
        return SUCCESS();

    }

} // pam_auth_client_request

/// =-=-=-=-=-=-=-
/// @brief function to run the local exec which will
///        actually do the auth check for us
#ifndef PAM_AUTH_CHECK_PROG
#define PAM_AUTH_CHECK_PROG  "./irodsPamAuthCheck"
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

#ifdef RODS_SERVER
// =-=-=-=-=-=-=-
// handle an agent-side auth request call
irods::error pam_auth_agent_request(
    irods::plugin_context& _ctx ) {
    // =-=-=-=-=-=-=-
    // validate incoming parameters
    if ( !_ctx.valid< irods::pam_auth_object >().ok() ) {
        return ERROR( SYS_INVALID_INPUT_PARAM, "invalid plugin context" );
    }

    // =-=-=-=-=-=-=-
    // get the server host handle
    rodsServerHost_t* server_host = 0;
    int status =
        getAndConnRcatHost(_ctx.comm(), PRIMARY_RCAT, (const char*) _ctx.comm()->clientUser.rodsZone, &server_host);
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
            if ( _ctx.comm()->auth_scheme != NULL ) {
                free( _ctx.comm()->auth_scheme );
            }
            _ctx.comm()->auth_scheme = strdup( irods::AUTH_PAM_SCHEME.c_str() );
            return SUCCESS();

        }

    } // if !localhost

    // =-=-=-=-=-=-=-
    // parse the kvp out of the _resp->username string
    irods::kvp_map_t kvp;
    irods::error ret = irods::parse_escaped_kvp_string(
                           context,
                           kvp);
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

    // Request the resulting irods password after the handshake. Plus 1 for null terminator.
    std::array<char, MAX_PASSWORD_LEN + 1> password_out{};
    char* password_ptr = password_out.data();
    status =
        chlUpdateIrodsPamPassword(_ctx.comm(), user_name.c_str(), ttl, nullptr, &password_ptr, password_out.size());
    if (status < 0) {
        return ERROR(status, "failed updating iRODS pam password");
    }

    // =-=-=-=-=-=-=-
    // set the result for communication back to the client
    ptr->request_result(password_out.data());

    // =-=-=-=-=-=-=-
    // win!
    if ( _ctx.comm()->auth_scheme != NULL ) {
        free( _ctx.comm()->auth_scheme );
    }
    _ctx.comm()->auth_scheme = strdup( "pam" );
    return SUCCESS();

} // pam_auth_agent_request
#endif

// =-=-=-=-=-=-=-
// establish context - take the auth request results and massage them
// for the auth response call
irods::error pam_auth_establish_context(
    irods::plugin_context& _ctx ) {
    // =-=-=-=-=-=-=-
    // validate incoming parameters
    if ( !_ctx.valid< irods::pam_auth_object >().ok() ) {
        return ERROR(
                   SYS_INVALID_INPUT_PARAM,
                   "invalid plugin context" );

    }

    return SUCCESS();

} // pam_auth_establish_context

#ifdef RODS_SERVER
// =-=-=-=-=-=-=-
// stub for ops that the native plug does
// not need to support
irods::error pam_auth_agent_start(
    irods::plugin_context&,
    const char*) {
    return SUCCESS();

} // native_auth_success_stub

irods::error pam_auth_agent_response(
    irods::plugin_context& _ctx,
    authResponseInp_t*           _resp ) {
    return SUCCESS();
}

irods::error pam_auth_agent_verify(
    irods::plugin_context& ,
    const char* ,
    const char* ,
    const char* ) {
    return SUCCESS();

}
#endif

irods::error pam_auth_client_response(
    irods::plugin_context& _ctx,
    rcComm_t*                    _comm ) {
    return SUCCESS();
}

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
extern "C"
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
    using namespace irods;
    using namespace std;
    pam->add_operation(
        AUTH_ESTABLISH_CONTEXT,
        function<error(plugin_context&)>(
            pam_auth_establish_context ) );
    pam->add_operation(
        AUTH_CLIENT_START,
        function<error(plugin_context&,rcComm_t*,const char*)>(
            pam_auth_client_start ) );
    pam->add_operation(
        AUTH_CLIENT_AUTH_REQUEST,
        function<error(plugin_context&,rcComm_t*)>(
            pam_auth_client_request ) );
    pam->add_operation(
        AUTH_CLIENT_AUTH_RESPONSE,
        function<error(plugin_context&,rcComm_t*)>(
            pam_auth_client_response ) );
#ifdef RODS_SERVER
    pam->add_operation(
        AUTH_AGENT_START,
        function<error(plugin_context&,const char*)>(
            pam_auth_agent_start ) );
    pam->add_operation(
        AUTH_AGENT_AUTH_REQUEST,
        function<error(plugin_context&)>(
            pam_auth_agent_request )  );
    pam->add_operation(
        AUTH_AGENT_AUTH_RESPONSE,
        function<error(plugin_context&,authResponseInp_t*)>(
            pam_auth_agent_response ) );
    pam->add_operation(
        AUTH_AGENT_AUTH_VERIFY,
        function<error(plugin_context&,const char*,const char*,const char*)>(
            pam_auth_agent_verify ) );
#endif
    irods::auth* auth = dynamic_cast< irods::auth* >( pam );

    return auth;

} // plugin_factory
