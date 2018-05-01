// =-=-=-=-=-=-=-
// irods includes
#include "rodsDef.h"
#include "msParam.h"
#include "rcConnect.h"
#include "authRequest.h"
#include "authResponse.h"
#include "authCheck.h"
#include "miscServerFunct.hpp"
#include "authPluginRequest.h"
#include "authenticate.h"
#include "rsAuthCheck.hpp"
#include "rsAuthRequest.hpp"

// =-=-=-=-=-=-=-
#include "irods_auth_plugin.hpp"
#include "irods_auth_constants.hpp"
#include "irods_native_auth_object.hpp"
#include "irods_stacktrace.hpp"
#include "irods_kvp_string_parser.hpp"

// =-=-=-=-=-=-=-
// stl includes
#include <sstream>
#include <string>
#include <iostream>
#include <termios.h>
#include <unistd.h>

#include <openssl/md5.h>

int get64RandomBytes( char *buf );
void setSessionSignatureClientside( char* _sig );
void _rsSetAuthRequestGetChallenge( const char* _c );

static irods::error check_proxy_user_privileges(
    rsComm_t *rsComm,
    int proxyUserPriv ) {
    irods::error result = SUCCESS();

    if ( strcmp( rsComm->proxyUser.userName, rsComm->clientUser.userName ) != 0 ) {

        /* remote privileged user can only do things on behalf of users from
         * the same zone */
        result = ASSERT_ERROR( proxyUserPriv >= LOCAL_PRIV_USER_AUTH ||
                               ( proxyUserPriv >= REMOTE_PRIV_USER_AUTH &&
                                 strcmp( rsComm->proxyUser.rodsZone, rsComm->clientUser.rodsZone ) == 0 ),
                               SYS_PROXYUSER_NO_PRIV,
                               "Proxyuser: \"%s\" with %d no priv to auth clientUser: \"%s\".",
                               rsComm->proxyUser.userName, proxyUserPriv, rsComm->clientUser.userName );
    }

    return result;
}

// =-=-=-=-=-=-=-
// NOTE:: this needs to become a property
// Set requireServerAuth to 1 to fail authentications from
// un-authenticated Servers (for example, if the LocalZoneSID
// is not set)
#ifdef RODS_SERVER
const int requireServerAuth = 1;
#endif

// =-=-=-=-=-=-=-
// NOTE:: this needs to become a property
// If set, then SIDs are always required, errors will be return if a SID
// is not locally set for a remote server
#ifdef RODS_SERVER
const int requireSIDs = 0;
#endif

// =-=-=-=-=-=-=-
// given the client connection and context string, set up the
// native auth object with relevant informaiton: user, zone, etc
irods::error native_auth_client_start(
    irods::plugin_context& _ctx,
    rcComm_t*                    _comm,
    const char* ) {
    irods::error result = SUCCESS();
    irods::error ret;

    // =-=-=-=-=-=-=-
    // validate incoming parameters
    ret = _ctx.valid< irods::native_auth_object >();
    if ( ( result = ASSERT_PASS( ret, "Invalid plugin context." ) ).ok() ) {

        if ( ( result = ASSERT_ERROR( _comm, SYS_INVALID_INPUT_PARAM, "Null rcConn_t pointer." ) ).ok() ) {

            // =-=-=-=-=-=-=-
            // get the native auth object
            irods::native_auth_object_ptr ptr = boost::dynamic_pointer_cast<irods::native_auth_object >( _ctx.fco() );

            // =-=-=-=-=-=-=-
            // set the user name from the conn
            ptr->user_name( _comm->proxyUser.userName );

            // =-=-=-=-=-=-=-
            // set the zone name from the conn
            ptr->zone_name( _comm->proxyUser.rodsZone );
        }
    }

    return result;

} // native_auth_client_start

// =-=-=-=-=-=-=-
// establish context - take the auth request results and massage them
// for the auth response call
irods::error native_auth_establish_context(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;

    // =-=-=-=-=-=-=-
    // validate incoming parameters
    ret = _ctx.valid< irods::native_auth_object >();
    if ( ( result = ASSERT_PASS( ret, "Invalid plugin context." ) ).ok() ) {

        // =-=-=-=-=-=-=-
        // build a buffer for the challenge hash
        char md5_buf[ CHALLENGE_LEN + MAX_PASSWORD_LEN + 2 ];
        memset( md5_buf, 0, sizeof( md5_buf ) );

        // =-=-=-=-=-=-=-
        // get the native auth object
        irods::native_auth_object_ptr ptr = boost::dynamic_pointer_cast<irods::native_auth_object >( _ctx.fco() );

        // =-=-=-=-=-=-=-
        // copy the challenge into the md5 buffer
        strncpy( md5_buf, ptr->request_result().c_str(), CHALLENGE_LEN );

        // =-=-=-=-=-=-=-
        // Save a representation of some of the challenge string for use
        // as a session signiture
        setSessionSignatureClientside( md5_buf );

        // =-=-=-=-=-=-=-
        // determine if a password challenge is needed,
        // are we anonymous or not?
        int need_password = 0;
        if ( strncmp( ANONYMOUS_USER, ptr->user_name().c_str(), NAME_LEN ) == 0 ) {

            // =-=-=-=-=-=-=-
            // its an anonymous user - set the flag
            md5_buf[CHALLENGE_LEN + 1] = '\0';
            need_password = 0;

        }
        else {
            // =-=-=-=-=-=-=-
            // determine if a password is already in place
            need_password = obfGetPw( md5_buf + CHALLENGE_LEN );
        }

        // =-=-=-=-=-=-=-
        // prompt for a password if necessary
        if ( 0 != need_password ) {
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
            memset( &tty, 0, sizeof( tty ) );
            tcgetattr( STDIN_FILENO, &tty );
            tcflag_t oldflag = tty.c_lflag;
            tty.c_lflag &= ~ECHO;
            int error = tcsetattr( STDIN_FILENO, TCSANOW, &tty );
            int errsv = errno;
#endif
            if ( error ) {
                printf( "WARNING: Error %d disabling echo mode. Password will be displayed in plaintext.", errsv );
            }
            printf( "Enter your current iRODS password:" );
            std::string password = "";
            getline( std::cin, password );
            strncpy( md5_buf + CHALLENGE_LEN, password.c_str(), MAX_PASSWORD_LEN );
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
        } // if need_password

        // =-=-=-=-=-=-=-
        // create a md5 hash of the challenge
        MD5_CTX context;
        MD5_Init( &context );
        MD5_Update( &context, ( unsigned char* )md5_buf, CHALLENGE_LEN + MAX_PASSWORD_LEN );

        char digest[ RESPONSE_LEN + 2 ];
        MD5_Final( ( unsigned char* )digest, &context );

        // =-=-=-=-=-=-=-
        // make sure 'string' doesn't end early -
        // scrub out any errant terminating chars
        // by incrementing their value by one
        for ( int i = 0; i < RESPONSE_LEN; ++i ) {
            if ( digest[ i ] == '\0' ) {
                digest[ i ]++;
            }
        }

        // =-=-=-=-=-=-=-
        // cache the digest for the response
        ptr->digest( std::string( digest, RESPONSE_LEN ) );
    }

    return result;

} // native_auth_establish_context

// =-=-=-=-=-=-=-
// handle a client-side auth request call
irods::error native_auth_client_request(
    irods::plugin_context& _ctx,
    rcComm_t*                   _comm ) {

    if ( !_ctx.valid< irods::native_auth_object >().ok() ) {
        return ERROR( SYS_INVALID_INPUT_PARAM, "Invalid plugin context." );
    }

    authRequestOut_t* auth_request = nullptr;
    int status = rcAuthRequest( _comm, &auth_request );
    if ( status < 0 ) {
        if ( auth_request ) {
            free( auth_request->challenge );
            free( auth_request );
        }
        return ERROR( status, "Call to rcAuthRequest failed." );
    }
    else if ( !auth_request ) {
        return ERROR( SYS_NULL_INPUT, "Call to rcAuthRequest resulted in a null authRequest." );
    }
    else if ( !auth_request->challenge ) {
        free( auth_request );
        return ERROR( 0, "Challenge attribute is blank." );
    }

    irods::native_auth_object_ptr ptr = boost::dynamic_pointer_cast<irods::native_auth_object >( _ctx.fco() );
    ptr->request_result( std::string( auth_request->challenge, CHALLENGE_LEN ) );

    free( auth_request->challenge );
    free( auth_request );

    return SUCCESS();

} // native_auth_client_request


#ifdef RODS_SERVER
// =-=-=-=-=-=-=-
// handle an agent-side auth request call
irods::error native_auth_agent_request(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;

    // =-=-=-=-=-=-=-
    // validate incoming parameters
    ret = _ctx.valid< irods::native_auth_object >();
    if ( ( result = ASSERT_PASS( ret, "Invalid plugin context." ) ).ok() ) {

        if ( ( result = ASSERT_ERROR( _ctx.comm(), SYS_INVALID_INPUT_PARAM, "Null comm pointer." ) ).ok() ) {

            // =-=-=-=-=-=-=-
            // generate a random buffer and copy it to the challenge
            char buf[ CHALLENGE_LEN + 2 ];
            get64RandomBytes( buf );

            // =-=-=-=-=-=-=-
            // get the auth object
            irods::native_auth_object_ptr ptr = boost::dynamic_pointer_cast<irods::native_auth_object >( _ctx.fco() );

            // =-=-=-=-=-=-=-
            // cache the challenge
            ptr->request_result( buf );

            // =-=-=-=-=-=-=-
            // cache the challenge in the server for later usage
            _rsSetAuthRequestGetChallenge( buf );

            if ( _ctx.comm()->auth_scheme != nullptr ) {
                free( _ctx.comm()->auth_scheme );
            }
            _ctx.comm()->auth_scheme = strdup( irods::AUTH_NATIVE_SCHEME.c_str() );
        }
    }

    // =-=-=-=-=-=-=-
    // win!
    return SUCCESS();

} // native_auth_agent_request
#endif


// =-=-=-=-=-=-=-
// handle a client-side auth request call
irods::error native_auth_client_response(
    irods::plugin_context& _ctx,
    rcComm_t*                    _comm ) {
    irods::error result = SUCCESS();
    irods::error ret;

    // =-=-=-=-=-=-=-
    // validate incoming parameters
    ret = _ctx.valid< irods::native_auth_object >();
    if ( ( result = ASSERT_PASS( ret, "Invalid plugin context." ) ).ok() ) {
        if ( ( result = ASSERT_ERROR( _comm, SYS_INVALID_INPUT_PARAM, "Null rcComm_t pointer." ) ).ok() ) {

            // =-=-=-=-=-=-=-
            // get the auth object
            irods::native_auth_object_ptr ptr = boost::dynamic_pointer_cast<irods::native_auth_object >( _ctx.fco() );

            // =-=-=-=-=-=-=-
            // build the response string
            char response[ RESPONSE_LEN + 2 ];
            snprintf( response, RESPONSE_LEN + 2, "%s", ptr->digest().c_str() );

            // =-=-=-=-=-=-=-
            // build the username#zonename string
            std::string user_name = ptr->user_name() + "#"              + ptr->zone_name();
            char username[ MAX_NAME_LEN ];
            snprintf( username, MAX_NAME_LEN, "%s", user_name.c_str() );

            authResponseInp_t auth_response;
            auth_response.response = response;
            auth_response.username = username;
            int status = rcAuthResponse( _comm, &auth_response );
            result = ASSERT_ERROR( status >= 0, status, "Call to rcAuthResponseFailed." );
        }
    }
    return result;
} // native_auth_client_response


// TODO -This function really needs breaking into bite sized bits - harry

#ifdef RODS_SERVER
// =-=-=-=-=-=-=-
// handle an agent-side auth request call
irods::error native_auth_agent_response(
    irods::plugin_context& _ctx,
    authResponseInp_t*           _resp ) {
    irods::error ret = SUCCESS();

    // =-=-=-=-=-=-=-
    // validate incoming parameters
    ret = _ctx.valid();
    if ( !ret.ok() ) {
        return PASSMSG( "Invalid plugin context.", ret );
    }

    if ( nullptr == _resp ) {
        return ERROR( SYS_INVALID_INPUT_PARAM, "Invalid response or comm pointers." );
    }

    int status;
    char *bufp;
    authCheckInp_t authCheckInp;
    authCheckOut_t *authCheckOut = nullptr;
    rodsServerHost_t *rodsServerHost;

    char digest[RESPONSE_LEN + 2];
    char md5Buf[CHALLENGE_LEN + MAX_PASSWORD_LEN + 2];
    char serverId[MAX_PASSWORD_LEN + 2];

    bufp = _rsAuthRequestGetChallenge();

    /* need to do NoLogin because it could get into inf loop for cross
     * zone auth */

    status = getAndConnRcatHostNoLogin( _ctx.comm(), MASTER_RCAT,
                                        _ctx.comm()->proxyUser.rodsZone, &rodsServerHost );
    if ( status < 0 ) {
        return ERROR( status, "Connecting to rcat host failed." );
    }
    memset( &authCheckInp, 0, sizeof( authCheckInp ) );
    authCheckInp.challenge = bufp;
    //null-terminate the response for rsAuthCheck
    _resp->response = ( char * )realloc( _resp->response, RESPONSE_LEN + 1 );
    _resp->response[ RESPONSE_LEN ] = 0;
    authCheckInp.response = _resp->response;
    authCheckInp.username = _resp->username;

    if ( LOCAL_HOST == rodsServerHost->localFlag ) {
        status = rsAuthCheck( _ctx.comm(), &authCheckInp, &authCheckOut );
    }
    else {
        status = rcAuthCheck( rodsServerHost->conn, &authCheckInp, &authCheckOut );
        /* not likely we need this connection again */
        rcDisconnect( rodsServerHost->conn );
        rodsServerHost->conn = nullptr;
    }

    if ( status >= 0 && nullptr != authCheckOut ) {
        if ( rodsServerHost->localFlag != LOCAL_HOST ) {
            if ( authCheckOut->serverResponse == nullptr ) {
                rodsLog( LOG_NOTICE, "Warning, cannot authenticate remote server, no serverResponse field" );
                if ( requireServerAuth ) {
                    ret = ERROR( REMOTE_SERVER_AUTH_NOT_PROVIDED, "Authentication disallowed. no serverResponse field." );
                }
            }
            else {
                char *cp;
                int OK, len, i;
                if ( *authCheckOut->serverResponse == '\0' ) {
                    rodsLog( LOG_NOTICE, "Warning, cannot authenticate remote server, serverResponse field is empty" );
                    if ( requireServerAuth ) {
                        ret = ERROR( REMOTE_SERVER_AUTH_EMPTY, "Authentication disallowed, empty serverResponse." );
                    }
                }
                else {
                    char username2[NAME_LEN + 2];
                    char userZone[NAME_LEN + 2];
                    memset( md5Buf, 0, sizeof( md5Buf ) );
                    strncpy( md5Buf, authCheckInp.challenge, CHALLENGE_LEN );
                    parseUserName( _resp->username, username2, userZone );
                    getZoneServerId( userZone, serverId );
                    len = strlen( serverId );
                    if ( len <= 0 ) {
                        rodsLog( LOG_NOTICE, "rsAuthResponse: Warning, cannot authenticate the remote server, no RemoteZoneSID defined in server_config.json", status );
                        if ( requireServerAuth ) {
                            ret = ERROR( REMOTE_SERVER_SID_NOT_DEFINED, "Authentication disallowed, no RemoteZoneSID defined" );
                        }
                        if ( requireSIDs ) {
                            ret = ERROR( REMOTE_SERVER_SID_NOT_DEFINED, "Authentication disallowed, no RemoteZoneSID defined" );
                        }
                    }
                    else {
                        strncpy( md5Buf + CHALLENGE_LEN, serverId, len );
                        obfMakeOneWayHash(
                            HASH_TYPE_DEFAULT,
                            ( unsigned char* )md5Buf,
                            CHALLENGE_LEN + MAX_PASSWORD_LEN,
                            ( unsigned char* )digest );

                        for ( i = 0; i < RESPONSE_LEN; i++ ) {
                            if ( digest[i] == '\0' ) {
                                digest[i]++;
                            }  /* make sure 'string' doesn't end early*/
                        }
                        cp = authCheckOut->serverResponse;
                        OK = 1;
                        for ( i = 0; i < RESPONSE_LEN; i++ ) {
                            if ( *cp++ != digest[i] ) {
                                OK = 0;
                            }
                        }
                        rodsLog( LOG_DEBUG, "serverResponse is OK/Not: %d", OK );
                        if ( 0 == OK ) {
                            ret = ERROR( REMOTE_SERVER_AUTHENTICATION_FAILURE, "Authentication disallowed, server response incorrect." );
                        }
                    }
                }
            }
        }

        /* Set the clientUser zone if it is null. */
        if ( ret.ok() && 0 == strlen( _ctx.comm()->clientUser.rodsZone ) ) {
            zoneInfo_t *tmpZoneInfo;
            status = getLocalZoneInfo( &tmpZoneInfo );
            if ( status < 0 ) {
                ret = ERROR( status, "getLocalZoneInfo failed." ); 
            }
            else {
                strncpy( _ctx.comm()->clientUser.rodsZone, tmpZoneInfo->zoneName, NAME_LEN );
            }
        }

        /* have to modify privLevel if the icat is a foreign icat because
         * a local user in a foreign zone is not a local user in this zone
         * and vice versa for a remote user
         */
        if ( ret.ok() && rodsServerHost->rcatEnabled == REMOTE_ICAT ) {
            /* proxy is easy because rodsServerHost is based on proxy user */
            if ( authCheckOut->privLevel == LOCAL_PRIV_USER_AUTH ) {
                authCheckOut->privLevel = REMOTE_PRIV_USER_AUTH;
            }
            else if ( authCheckOut->privLevel == LOCAL_USER_AUTH ) {
                authCheckOut->privLevel = REMOTE_USER_AUTH;
            }

            /* adjust client user */
            if ( 0 == strcmp( _ctx.comm()->proxyUser.userName,  _ctx.comm()->clientUser.userName ) ) {
                authCheckOut->clientPrivLevel = authCheckOut->privLevel;
            }
            else {
                zoneInfo_t *tmpZoneInfo;
                status = getLocalZoneInfo( &tmpZoneInfo );
                if ( status < 0 ) {
                    ret = ERROR( status, "getLocalZoneInfo failed." );
                }
                else {
                    if ( 0 == strcmp( tmpZoneInfo->zoneName,  _ctx.comm()->clientUser.rodsZone ) ) {
                        /* client is from local zone */
                        if ( REMOTE_PRIV_USER_AUTH == authCheckOut->clientPrivLevel ) {
                            authCheckOut->clientPrivLevel = LOCAL_PRIV_USER_AUTH;
                        }
                        else if ( REMOTE_USER_AUTH == authCheckOut->clientPrivLevel ) {
                            authCheckOut->clientPrivLevel = LOCAL_USER_AUTH;
                        }
                    }
                    else {
                        /* client is from remote zone */
                        if ( LOCAL_PRIV_USER_AUTH == authCheckOut->clientPrivLevel ) {
                            authCheckOut->clientPrivLevel = REMOTE_USER_AUTH;
                        }
                        else if ( LOCAL_USER_AUTH == authCheckOut->clientPrivLevel ) {
                            authCheckOut->clientPrivLevel = REMOTE_USER_AUTH;
                        }
                    }
                }
            }
        }
        else if ( 0 == strcmp( _ctx.comm()->proxyUser.userName,  _ctx.comm()->clientUser.userName ) ) {
            authCheckOut->clientPrivLevel = authCheckOut->privLevel;
        }

        if ( ret.ok() ) {
            ret = check_proxy_user_privileges( _ctx.comm(), authCheckOut->privLevel );
            if ( !ret.ok() ) {
                ret = PASSMSG( "Check proxy user priviledges failed.", ret );
            }
            else {
                rodsLog( LOG_DEBUG,
                         "rsAuthResponse set proxy authFlag to %d, client authFlag to %d, user:%s proxy:%s client:%s",
                         authCheckOut->privLevel,
                         authCheckOut->clientPrivLevel,
                         authCheckInp.username,
                         _ctx.comm()->proxyUser.userName,
                         _ctx.comm()->clientUser.userName );

                if ( strcmp( _ctx.comm()->proxyUser.userName,  _ctx.comm()->clientUser.userName ) != 0 ) {
                    _ctx.comm()->proxyUser.authInfo.authFlag = authCheckOut->privLevel;
                    _ctx.comm()->clientUser.authInfo.authFlag = authCheckOut->clientPrivLevel;
                }
                else {          /* proxyUser and clientUser are the same */
                    _ctx.comm()->proxyUser.authInfo.authFlag =
                        _ctx.comm()->clientUser.authInfo.authFlag = authCheckOut->privLevel;
                }
            }
        }
    }
    else {
        ret = ERROR( status, "rcAuthCheck failed." );
    }

    if ( authCheckOut != nullptr ) {
        if ( authCheckOut->serverResponse != nullptr ) {
            free( authCheckOut->serverResponse );
        }
        free( authCheckOut );
    }
    return ret;
} // native_auth_agent_response

// =-=-=-=-=-=-=-
// stub for ops that the native plug does
// not need to support
irods::error native_auth_agent_verify(
    irods::plugin_context& ,
    const char* ,
    const char* ,
    const char* ) {
    return SUCCESS();

} // native_auth_agent_verify


// =-=-=-=-=-=-=-
// stub for ops that the native plug does
// not need to support
irods::error native_auth_agent_start(
    irods::plugin_context&,
    const char*) {
    return SUCCESS();

} // native_auth_success_stub
#endif

// =-=-=-=-=-=-=-
// derive a new native_auth auth plugin from
// the auth plugin base class for handling
// native authentication
class native_auth_plugin : public irods::auth {
    public:
        native_auth_plugin(
            const std::string& _nm,
            const std::string& _ctx ) :
            irods::auth(
                _nm,
                _ctx ) {
        } // ctor

        ~native_auth_plugin() override = default;

}; // class native_auth_plugin

// =-=-=-=-=-=-=-
// factory function to provide instance of the plugin
extern "C"
irods::auth* plugin_factory(
    const std::string& _inst_name,
    const std::string& _context ) {
    // =-=-=-=-=-=-=-
    // create an auth object
    auto  nat = new native_auth_plugin(
        _inst_name,
        _context );

    // =-=-=-=-=-=-=-
    // fill in the operation table mapping call
    // names to function names
    using namespace irods;
    using namespace std;
    nat->add_operation(
        AUTH_ESTABLISH_CONTEXT,
        function<error(plugin_context&)>(
            native_auth_establish_context ) );
    nat->add_operation<rcComm_t*,const char*>(
        AUTH_CLIENT_START,
        function<error(plugin_context&,rcComm_t*,const char*)>(
            native_auth_client_start ) );
    nat->add_operation<rcComm_t*>(
        AUTH_CLIENT_AUTH_REQUEST,
        function<error(plugin_context&,rcComm_t*)>(
            native_auth_client_request ) );
    nat->add_operation<rcComm_t*>(
        AUTH_CLIENT_AUTH_RESPONSE,
        function<error(plugin_context&,rcComm_t*)>(
            native_auth_client_response ) );
#ifdef RODS_SERVER
    nat->add_operation<const char*>(
        AUTH_AGENT_START,
        function<error(plugin_context&,const char*)>(
            native_auth_agent_start ) );
    nat->add_operation(
        AUTH_AGENT_AUTH_REQUEST,
        function<error(plugin_context&)>(
            native_auth_agent_request )  );
    nat->add_operation<authResponseInp_t*>(
        AUTH_AGENT_AUTH_RESPONSE,
        function<error(plugin_context&,authResponseInp_t*)>(
            native_auth_agent_response ) );
    nat->add_operation<const char*,const char*,const char*>(
        AUTH_AGENT_AUTH_VERIFY,
        function<error(plugin_context&,const char*,const char*,const char*)>(
            native_auth_agent_verify ) );
#endif
    irods::auth* auth = dynamic_cast< irods::auth* >( nat );

    return auth;

} // plugin_factory
