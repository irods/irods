// =-=-=-=-=-=-=-
// irods includes
#include "rodsDef.h"
#include "msParam.hpp"
#include "reGlobalsExtern.hpp"
#include "rcConnect.hpp"
#include "authRequest.hpp"
#include "authResponse.hpp"
#include "authCheck.hpp"
#include "miscServerFunct.hpp"
#include "authPluginRequest.hpp"
#include "authenticate.hpp"

// =-=-=-=-=-=-=-
#include "irods_auth_plugin.hpp"
#include "irods_auth_constants.hpp"
#include "irods_osauth_auth_object.hpp"
#include "irods_stacktrace.hpp"
#include "irods_kvp_string_parser.hpp"

// =-=-=-=-=-=-=-
// stl includes
#include <sstream>
#include <string>
#include <iostream>
#include <termios.h>
#include <unistd.h>

// =-=-=-=-=-=-=-
// local includes
#define OS_AUTH 1
#include "osauth.hpp"

int get64RandomBytes( char *buf );
void setSessionSignatureClientside( char* _sig );
void _rsSetAuthRequestGetChallenge( const char* _c );
static
int check_proxy_user_privileges(
    rsComm_t *rsComm,
    int proxyUserPriv ) {
    if ( strcmp( rsComm->proxyUser.userName, rsComm->clientUser.userName )
            == 0 ) {
        return 0;
    }

    /* remote privileged user can only do things on behalf of users from
     * the same zone */
    if ( proxyUserPriv >= LOCAL_PRIV_USER_AUTH ||
            ( proxyUserPriv >= REMOTE_PRIV_USER_AUTH &&
              strcmp( rsComm->proxyUser.rodsZone, rsComm->clientUser.rodsZone ) == 0 ) ) {
        return 0;
    }
    else {
        rodsLog( LOG_ERROR,
                 "rsAuthResponse: proxyuser %s with %d no priv to auth clientUser %s",
                 rsComm->proxyUser.userName,
                 proxyUserPriv,
                 rsComm->clientUser.userName );
        return SYS_PROXYUSER_NO_PRIV;
    }
}


extern "C" {
    // =-=-=-=-=-=-=-
    // NOTE:: this needs to become a property
    // Set requireServerAuth to 1 to fail authentications from
    // un-authenticated Servers (for example, if the LocalZoneSID
    // is not set)
    const int requireServerAuth = 0;

    // =-=-=-=-=-=-=-
    // given the client connection and context string, set up the
    // native auth object with relevant information: user, zone, etc
    irods::error osauth_auth_client_start(
        irods::auth_plugin_context& _ctx,
        rcComm_t*                   _comm,
        const char* ) {
        // =-=-=-=-=-=-=-
        // validate incoming parameters
        if ( !_ctx.valid< irods::osauth_auth_object >().ok() ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "invalid plugin context" );

        }
        else if ( !_comm ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "null rcConn_t ptr" );

        }

        // =-=-=-=-=-=-=-
        // get the native auth object
        irods::osauth_auth_object_ptr ptr = boost::dynamic_pointer_cast <
                                            irods::osauth_auth_object > (
                                                _ctx.fco() );
        // =-=-=-=-=-=-=-
        // set the user name from the conn
        ptr->user_name( _comm->proxyUser.userName );

        // =-=-=-=-=-=-=-
        // set the zone name from the conn
        ptr->zone_name( _comm->proxyUser.rodsZone );

        return SUCCESS();

    } // osauth_auth_client_start

    // =-=-=-=-=-=-=-
    // establish context - take the auth request results and massage them
    // for the auth response call
    irods::error osauth_auth_establish_context(
        irods::auth_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // validate incoming parameters
        if ( !_ctx.valid< irods::osauth_auth_object >().ok() ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "invalid plugin context" );

        }

        // =-=-=-=-=-=-=-
        // build a buffer for the challenge hash
        char md5_buf[ CHALLENGE_LEN + MAX_PASSWORD_LEN + 2 ];
        memset(
            md5_buf,
            0,
            sizeof( md5_buf ) );

        // =-=-=-=-=-=-=-
        // get the native auth object
        irods::osauth_auth_object_ptr ptr = boost::dynamic_pointer_cast <
                                            irods::osauth_auth_object > (
                                                _ctx.fco() );
        // =-=-=-=-=-=-=-
        // copy the challenge into the md5 buffer
        strncpy(
            md5_buf,
            ptr->request_result().c_str(),
            CHALLENGE_LEN );

        // =-=-=-=-=-=-=-
        // Save a representation of some of the challenge string for use
        // as a session signature
        setSessionSignatureClientside( md5_buf );

        // =-=-=-=-=-=-=-
        // determine if a password challenge is needed,
        // are we anonymous or not?
        int need_password = 0;
        if ( strncmp(
                    ANONYMOUS_USER,
                    ptr->user_name().c_str(),
                    NAME_LEN ) == 0 ) {
            // =-=-=-=-=-=-=-
            // its an anonymous user - set the flag
            md5_buf[CHALLENGE_LEN + 1] = '\0';
            need_password = 0;

        }
        else {
            // =-=-=-=-=-=-=-
            // do os authentication
            need_password = osauthGetAuth(
                                const_cast< char* >( ptr->request_result().c_str() ),
                                const_cast< char* >( ptr->user_name().c_str() ),
                                md5_buf + CHALLENGE_LEN,
                                MAX_PASSWORD_LEN );
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
        MD5Init( &context );
        MD5Update(
            &context,
            ( unsigned char* )md5_buf,
            CHALLENGE_LEN + MAX_PASSWORD_LEN );

        char digest[ RESPONSE_LEN + 2 ];
        MD5Final( ( unsigned char* )digest, &context );

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
        ptr->digest( digest );

        return SUCCESS();

    } // osauth_auth_establish_context

    // =-=-=-=-=-=-=-
    // handle a client-side auth request call
    irods::error osauth_auth_client_request(
        irods::auth_plugin_context& _ctx,
        rcComm_t*                    _comm ) {
        // =-=-=-=-=-=-=-
        // validate incoming parameters
        if ( !_ctx.valid< irods::osauth_auth_object >().ok() ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "invalid plugin context" );
        }

        // =-=-=-=-=-=-=-
        // copy the auth scheme to the req in struct
        authPluginReqInp_t req_in;
        strncpy(
            req_in.auth_scheme_,
            irods::AUTH_OSAUTH_SCHEME.c_str(),
            irods::AUTH_OSAUTH_SCHEME.size() + 1 );

        // =-=-=-=-=-=-=-
        // make the call to our auth request
        authPluginReqOut_t* req_out = 0;
        int status = rcAuthPluginRequest(
                         _comm,
                         &req_in,
                         &req_out );
        if ( status < 0 ) {
            free( req_out );
            return ERROR(
                       status,
                       "call to rcAuthRequest failed." );

        }
        else {
            // =-=-=-=-=-=-=-
            // get the auth object
            irods::osauth_auth_object_ptr ptr = boost::dynamic_pointer_cast <
                                                irods::osauth_auth_object > ( _ctx.fco() );
            // =-=-=-=-=-=-=-
            // cache the challenge
            ptr->request_result( req_out->result_ );
            free( req_out );
            return SUCCESS();

        }

    } // osauth_auth_client_request

    // =-=-=-=-=-=-=-
    // handle an agent-side auth request call
    irods::error osauth_auth_agent_request(
        irods::auth_plugin_context& _ctx,
        rsComm_t*                    _comm ) {

        // =-=-=-=-=-=-=-
        // validate incoming parameters
        if ( !_ctx.valid< irods::osauth_auth_object >().ok() ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "invalid plugin context" );
        }
        else if ( !_comm ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "null comm ptr" );
        }

        // =-=-=-=-=-=-=-
        // generate a random buffer and copy it to the challenge
        char buf[ CHALLENGE_LEN + 2 ];
        get64RandomBytes( buf );

        // =-=-=-=-=-=-=-
        // get the auth object
        irods::osauth_auth_object_ptr ptr = boost::dynamic_pointer_cast <
                                            irods::osauth_auth_object > ( _ctx.fco() );
        // =-=-=-=-=-=-=-
        // cache the challenge
        ptr->request_result( buf );

        // =-=-=-=-=-=-=-
        // cache the challenge in the server for later usage
        _rsSetAuthRequestGetChallenge( buf );

        if ( _comm->auth_scheme != NULL ) {
            free( _comm->auth_scheme );
        }
        _comm->auth_scheme = strdup( irods::AUTH_OSAUTH_SCHEME.c_str() );

        // =-=-=-=-=-=-=-
        // win!
        return SUCCESS();

    } // osauth_auth_agent_request

    // =-=-=-=-=-=-=-
    // handle a client-side auth request call
    irods::error osauth_auth_client_response(
        irods::auth_plugin_context& _ctx,
        rcComm_t*                    _comm ) {
        // =-=-=-=-=-=-=-
        // validate incoming parameters
        if ( !_ctx.valid< irods::osauth_auth_object >().ok() ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "invalid plugin context" );
        }
        else if ( !_comm ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "null rcComm_t ptr" );
        }

        // =-=-=-=-=-=-=-
        // get the auth object
        irods::osauth_auth_object_ptr ptr = boost::dynamic_pointer_cast <
                                            irods::osauth_auth_object > (
                                                _ctx.fco() );
        char response[ RESPONSE_LEN + 2 ];
        snprintf(
            response,
            RESPONSE_LEN + 2,
            "%s",
            ptr->digest().c_str() );

        // =-=-=-=-=-=-=-
        // build the username#zonename string
        std::string user_name = ptr->user_name() +
                                "#"              +
                                ptr->zone_name();
        char username[ MAX_NAME_LEN ];
        snprintf(
            username,
            MAX_NAME_LEN,
            "%s",
            user_name.c_str() );

        authResponseInp_t auth_response;
        auth_response.response = response;
        auth_response.username = username;
        int status = rcAuthResponse(
                         _comm,
                         &auth_response );
        if ( status < 0 ) {
            return ERROR(
                       status,
                       "call to rcAuthResponse failed." );
        }
        else {
            return SUCCESS();

        }

    } // osauth_auth_client_response

    // =-=-=-=-=-=-=-
    // handle an agent-side auth request call
    irods::error osauth_auth_agent_response(
        irods::auth_plugin_context& _ctx,
        rsComm_t*                    _comm,
        authResponseInp_t*           _resp ) {
        // =-=-=-=-=-=-=-
        // validate incoming parameters
        if ( !_ctx.valid().ok() ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "invalid plugin context" );
        }
        else if ( !_resp ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "null authResponseInp_t ptr" );
        }
        else if ( !_comm ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "null rsComm_t ptr" );
        }

        int status;
        char *bufp;
        authCheckInp_t authCheckInp;
        rodsServerHost_t *rodsServerHost;

        char digest[RESPONSE_LEN + 2];
        char md5Buf[CHALLENGE_LEN + MAX_PASSWORD_LEN + 2];
        char serverId[MAX_PASSWORD_LEN + 2];
        MD5_CTX context;

        bufp = _rsAuthRequestGetChallenge();

        // =-=-=-=-=-=-=-
        // need to do NoLogin because it could get into inf loop for cross
        // zone auth
        status = getAndConnRcatHostNoLogin(
                     _comm,
                     MASTER_RCAT,
                     _comm->proxyUser.rodsZone,
                     &rodsServerHost );
        if ( status < 0 ) {
            return ERROR(
                       status,
                       "getAndConnRcatHostNoLogin failed" );
        }

        memset( &authCheckInp, 0, sizeof( authCheckInp ) );
        authCheckInp.challenge = bufp;
        authCheckInp.username = _resp->username;

        std::string resp_str = irods::AUTH_SCHEME_KEY    +
                               irods::kvp_association()  +
                               irods::AUTH_OSAUTH_SCHEME +
                               irods::kvp_delimiter()    +
                               irods::AUTH_RESPONSE_KEY  +
                               irods::kvp_association()  +
                               _resp->response;
        authCheckInp.response = const_cast<char*>( resp_str.c_str() );

        authCheckOut_t *authCheckOut = NULL;
        if ( rodsServerHost->localFlag == LOCAL_HOST ) {
            status = rsAuthCheck( _comm, &authCheckInp, &authCheckOut );
        }
        else {
            status = rcAuthCheck( rodsServerHost->conn, &authCheckInp, &authCheckOut );
            /* not likely we need this connection again */
            rcDisconnect( rodsServerHost->conn );
            rodsServerHost->conn = NULL;
        }
        if ( status < 0 || authCheckOut == NULL ) { // JMC cppcheck
            if ( authCheckOut != NULL ) {
                free( authCheckOut->serverResponse );
            }
            free( authCheckOut );
            return ERROR(
                       status,
                       "rxAuthCheck failed" );
        }

        if ( rodsServerHost->localFlag != LOCAL_HOST ) {
            if ( authCheckOut->serverResponse == NULL ) {
                rodsLog( LOG_NOTICE, "Warning, cannot authenticate remote server, no serverResponse field" );
                if ( requireServerAuth ) {
                    free( authCheckOut );
                    return ERROR(
                               REMOTE_SERVER_AUTH_NOT_PROVIDED,
                               "Authentication disallowed, no serverResponse field" );
                }
            }
            else {
                char *cp;
                int OK, len, i;
                if ( *authCheckOut->serverResponse == '\0' ) {
                    rodsLog( LOG_NOTICE, "Warning, cannot authenticate remote server, serverResponse field is empty" );
                    if ( requireServerAuth ) {
                        free( authCheckOut->serverResponse );
                        free( authCheckOut );
                        return ERROR(
                                   REMOTE_SERVER_AUTH_EMPTY,
                                   "Authentication disallowed, empty serverResponse" );
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
                            free( authCheckOut->serverResponse );
                            free( authCheckOut );
                            return ERROR(
                                       REMOTE_SERVER_SID_NOT_DEFINED,
                                       "Authentication disallowed, no RemoteZoneSID defined" );
                        }
                    }
                    else {
                        strncpy( md5Buf + CHALLENGE_LEN, serverId, len );
                        MD5Init( &context );
                        MD5Update( &context, ( unsigned char* )md5Buf,
                                   CHALLENGE_LEN + MAX_PASSWORD_LEN );
                        MD5Final( ( unsigned char* )digest, &context );
                        for ( i = 0; i < RESPONSE_LEN; i++ ) {
                            if ( digest[i] == '\0' ) {
                                digest[i]++;
                            }  /* make sure 'string' doesn't
                                                                  end early*/
                        }
                        cp = authCheckOut->serverResponse;
                        OK = 1;
                        for ( i = 0; i < RESPONSE_LEN; i++ ) {
                            if ( *cp++ != digest[i] ) {
                                OK = 0;
                            }
                        }
                        rodsLog( LOG_DEBUG, "serverResponse is OK/Not: %d", OK );
                        if ( OK == 0 ) {
                            free( authCheckOut->serverResponse );
                            free( authCheckOut );
                            return ERROR(
                                       REMOTE_SERVER_AUTHENTICATION_FAILURE,
                                       "Server response incorrect, authentication disallowed" );
                        }
                    }
                }
            }
        }

        /* Set the clientUser zone if it is null. */
        if ( strlen( _comm->clientUser.rodsZone ) == 0 ) {
            zoneInfo_t *tmpZoneInfo;
            status = getLocalZoneInfo( &tmpZoneInfo );
            if ( status < 0 ) {
                free( authCheckOut->serverResponse );
                free( authCheckOut );
                return ERROR(
                           status,
                           "getLocalZoneInfo failed" );
            }
            strncpy( _comm->clientUser.rodsZone,
                     tmpZoneInfo->zoneName, NAME_LEN );
        }


        /* have to modify privLevel if the icat is a foreign icat because
         * a local user in a foreign zone is not a local user in this zone
         * and vice versa for a remote user
         */
        if ( rodsServerHost->rcatEnabled == REMOTE_ICAT ) {
            /* proxy is easy because rodsServerHost is based on proxy user */
            if ( authCheckOut->privLevel == LOCAL_PRIV_USER_AUTH ) {
                authCheckOut->privLevel = REMOTE_PRIV_USER_AUTH;
            }
            else if ( authCheckOut->privLevel == LOCAL_USER_AUTH ) {
                authCheckOut->privLevel = REMOTE_USER_AUTH;
            }

            /* adjust client user */
            if ( strcmp( _comm->proxyUser.userName,  _comm->clientUser.userName )
                    == 0 ) {
                authCheckOut->clientPrivLevel = authCheckOut->privLevel;
            }
            else {
                zoneInfo_t *tmpZoneInfo;
                status = getLocalZoneInfo( &tmpZoneInfo );
                if ( status < 0 ) {
                    free( authCheckOut->serverResponse );
                    free( authCheckOut );
                    return ERROR(
                               status,
                               "getLocalZoneInfo failed" );
                }

                if ( strcmp( tmpZoneInfo->zoneName,  _comm->clientUser.rodsZone )
                        == 0 ) {
                    /* client is from local zone */
                    if ( authCheckOut->clientPrivLevel == REMOTE_PRIV_USER_AUTH ) {
                        authCheckOut->clientPrivLevel = LOCAL_PRIV_USER_AUTH;
                    }
                    else if ( authCheckOut->clientPrivLevel == REMOTE_USER_AUTH ) {
                        authCheckOut->clientPrivLevel = LOCAL_USER_AUTH;
                    }
                }
                else {
                    /* client is from remote zone */
                    if ( authCheckOut->clientPrivLevel == LOCAL_PRIV_USER_AUTH ) {
                        authCheckOut->clientPrivLevel = REMOTE_USER_AUTH;
                    }
                    else if ( authCheckOut->clientPrivLevel == LOCAL_USER_AUTH ) {
                        authCheckOut->clientPrivLevel = REMOTE_USER_AUTH;
                    }
                }
            }
        }
        else if ( strcmp( _comm->proxyUser.userName,  _comm->clientUser.userName )
                  == 0 ) {
            authCheckOut->clientPrivLevel = authCheckOut->privLevel;
        }

        status = check_proxy_user_privileges( _comm, authCheckOut->privLevel );

        if ( status < 0 ) {
            free( authCheckOut->serverResponse );
            free( authCheckOut );
            return ERROR(
                       status,
                       "check_proxy_user_privileges failed" );
        }

        rodsLog( LOG_DEBUG,
                 "rsAuthResponse set proxy authFlag to %d, client authFlag to %d, user:%s proxy:%s client:%s",
                 authCheckOut->privLevel,
                 authCheckOut->clientPrivLevel,
                 authCheckInp.username,
                 _comm->proxyUser.userName,
                 _comm->clientUser.userName );

        if ( strcmp( _comm->proxyUser.userName,  _comm->clientUser.userName ) != 0 ) {
            _comm->proxyUser.authInfo.authFlag = authCheckOut->privLevel;
            _comm->clientUser.authInfo.authFlag = authCheckOut->clientPrivLevel;
        }
        else {	/* proxyUser and clientUser are the same */
            _comm->proxyUser.authInfo.authFlag =
                _comm->clientUser.authInfo.authFlag = authCheckOut->privLevel;
        }

        free( authCheckOut->serverResponse );
        free( authCheckOut );
        return SUCCESS();

    } // osauth_auth_agent_response

    // =-=-=-=-=-=-=-
    // operation to verify the response on the agent side
    irods::error osauth_auth_agent_auth_verify(
        irods::auth_plugin_context&,
        const char*                  _challenge,
        const char*                  _user_name,
        const char*                  _response ) {
        // =-=-=-=-=-=-=-
        // delegate auth verify to osauth lib
        int status = osauthVerifyResponse(
                         const_cast< char* >( _challenge ),
                         const_cast< char* >( _user_name ),
                         const_cast< char* >( _response ) );
        if ( status ) {
            return ERROR(
                       status,
                       "osauthVerifyResponse failed" );
        }
        else {
            return SUCCESS();

        }

    } // osauth_auth_agent_auth_verify


    // =-=-=-=-=-=-=-
    // stub for ops that the native plug does
    // not need to support
    irods::error osauth_auth_success_stub(
        irods::auth_plugin_context& ) {
        return SUCCESS();

    } // osauth_auth_success_stub

    // =-=-=-=-=-=-=-
    // derive a new osauth_auth auth plugin from
    // the auth plugin base class for handling
    // native authentication
    class osauth_auth_plugin : public irods::auth {
        public:
            osauth_auth_plugin(
                const std::string& _nm,
                const std::string& _ctx ) :
                irods::auth(
                    _nm,
                    _ctx ) {
            } // ctor

            ~osauth_auth_plugin() {
            }

    }; // class osauth_auth_plugin

    // =-=-=-=-=-=-=-
    // factory function to provide instance of the plugin
    irods::auth* plugin_factory(
        const std::string& _inst_name,
        const std::string& _context ) {
        // =-=-=-=-=-=-=-
        // create an auth object
        osauth_auth_plugin* nat = new osauth_auth_plugin(
            _inst_name,
            _context );

        // =-=-=-=-=-=-=-
        // fill in the operation table mapping call
        // names to function names
        nat->add_operation( irods::AUTH_CLIENT_START,         "osauth_auth_client_start" );
        nat->add_operation( irods::AUTH_AGENT_START,          "osauth_auth_success_stub" );
        nat->add_operation( irods::AUTH_ESTABLISH_CONTEXT,    "osauth_auth_establish_context" );
        nat->add_operation( irods::AUTH_CLIENT_AUTH_REQUEST,  "osauth_auth_client_request" );
        nat->add_operation( irods::AUTH_AGENT_AUTH_REQUEST,   "osauth_auth_agent_request" );
        nat->add_operation( irods::AUTH_CLIENT_AUTH_RESPONSE, "osauth_auth_client_response" );
        nat->add_operation( irods::AUTH_AGENT_AUTH_RESPONSE,  "osauth_auth_agent_response" );
        nat->add_operation( irods::AUTH_AGENT_AUTH_VERIFY,    "osauth_auth_agent_auth_verify" );

        irods::auth* auth = dynamic_cast< irods::auth* >( nat );

        return auth;

    } // plugin_factory

}; // extern "C"
