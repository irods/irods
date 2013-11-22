
// =-=-=-=-=-=-=-
// irods includes
#include "rodsDef.h"
#include "msParam.h"
#include "reGlobalsExtern.h"
#include "rcConnect.h"
#include "authRequest.h"
#include "authResponse.h"
#include "authCheck.h"
#include "miscServerFunct.h"
#include "authPluginRequest.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_auth_plugin.h"
#include "eirods_auth_constants.h"
#include "eirods_native_auth_object.h"
#include "eirods_stacktrace.h"
#include "eirods_kvp_string_parser.h"

// =-=-=-=-=-=-=-
// stl includes
#include <sstream>
#include <string>
#include <iostream>

int get64RandomBytes(char *buf);
void setSessionSignatureClientside( char* _sig );
void _rsSetAuthRequestGetChallenge( const char* _c );
static
int check_proxy_user_privelges(
    rsComm_t *rsComm, 
    int proxyUserPriv ) {
    if (strcmp (rsComm->proxyUser.userName, rsComm->clientUser.userName) 
      == 0) return 0;

    /* remote privileged user can only do things on behalf of users from
     * the same zone */
    if (proxyUserPriv >= LOCAL_PRIV_USER_AUTH ||
      (proxyUserPriv >= REMOTE_PRIV_USER_AUTH &&
      strcmp (rsComm->proxyUser.rodsZone,rsComm->clientUser.rodsZone) == 0)) {
	return 0;
    } else {
        rodsLog (LOG_ERROR,
         "rsAuthResponse: proxyuser %s with %d no priv to auth clientUser %s",
             rsComm->proxyUser.userName,
             proxyUserPriv,
             rsComm->clientUser.userName);
         return (SYS_PROXYUSER_NO_PRIV);
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
    // native auth object with relevant informaiton: user, zone, etc
    eirods::error native_auth_client_start(
        eirods::auth_plugin_context& _ctx,
        rcComm_t*                    _comm, 
        const char*                  _context ) {
        // =-=-=-=-=-=-=-
        // validate incoming parameters
        if( !_ctx.valid< eirods::native_auth_object >().ok() ) {
            return ERROR( 
                       SYS_INVALID_INPUT_PARAM,
                       "invalid plugin context" );
        
        } else if( !_comm ) {
            return ERROR( 
                       SYS_INVALID_INPUT_PARAM,
                       "null rcConn_t ptr" );
            
        }

        // =-=-=-=-=-=-=-
        // get the native auth object
        eirods::native_auth_object_ptr ptr = boost::dynamic_pointer_cast< 
                                                 eirods::native_auth_object >( 
                                                     _ctx.fco() );
        // =-=-=-=-=-=-=-
        // set the user name from the conn
        ptr->user_name( _comm->proxyUser.userName );
        
        // =-=-=-=-=-=-=-
        // set the zone name from the conn
        ptr->zone_name( _comm->proxyUser.rodsZone );

        return SUCCESS();

    } // native_auth_client_start
 
    // =-=-=-=-=-=-=-
    // establish context - take the auth request results and massage them
    // for the auth response call
    eirods::error native_auth_establish_context(
        eirods::auth_plugin_context& _ctx ) {
        // =-=-=-=-=-=-=-
        // validate incoming parameters
        if( !_ctx.valid< eirods::native_auth_object >().ok() ) {
            return ERROR( 
                       SYS_INVALID_INPUT_PARAM,
                       "invalid plugin context" );
            
        }
               
        // =-=-=-=-=-=-=-
        // build a buffer for the challenge hash
        char md5_buf[ CHALLENGE_LEN+MAX_PASSWORD_LEN+2 ];
        memset( 
            md5_buf, 
            0, 
            sizeof( md5_buf ) );
 
        // =-=-=-=-=-=-=-
        // get the native auth object
        eirods::native_auth_object_ptr ptr = boost::dynamic_pointer_cast< 
                                                 eirods::native_auth_object >( 
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
        if( strncmp(
                ANONYMOUS_USER, 
                ptr->user_name().c_str(),
                NAME_LEN ) == 0 ) {
            // =-=-=-=-=-=-=-
            // its an anonymous user - set the flag
            md5_buf[CHALLENGE_LEN+1]='\0';
            need_password = 0;

        } else {
            // =-=-=-=-=-=-=-
            // determine if a password is already in place
            need_password = obfGetPw( md5_buf+CHALLENGE_LEN );

        }

        // =-=-=-=-=-=-=-
        // prompt for a password if necessary
        if( 0 != need_password ) {
            int doStty=0;

            #ifdef windows_platform
            if (ProcessType != CLIENT_PT)
                return i;
            #endif

            path p ("/bin/stty");
            if (exists(p)) {
                system("/bin/stty -echo 2> /dev/null");
                doStty=1;
            }
            printf("Enter your current iRODS password:");
            fgets(md5_buf+CHALLENGE_LEN, MAX_PASSWORD_LEN, stdin);
            if (doStty) {
                system("/bin/stty echo 2> /dev/null");
                printf("\n");
            }
            
            int md5_len = strlen( md5_buf );
            md5_buf[md5_len-1]='\0'; /* remove trailing \n */

        } // if need_password

        // =-=-=-=-=-=-=-
        // create a md5 hash of the challenge
        MD5_CTX context;
        MD5Init( &context );
        MD5Update( 
            &context, 
            (unsigned char*)md5_buf, 
            CHALLENGE_LEN+MAX_PASSWORD_LEN );

        char digest[ RESPONSE_LEN+2 ];
        MD5Final( ( unsigned char* )digest, &context );

        // =-=-=-=-=-=-=-
        // make sure 'string' doesn'tend early -
        // scrub out any errant terminating chars
        // by incrementing their value by one
        for( int i = 0; i < RESPONSE_LEN; ++i ) {
            if( digest[ i ] == '\0' ) {
                digest[ i ]++;  
            }                                      
        }

        // =-=-=-=-=-=-=-
        // cache the digest for the response
        ptr->digest( digest );

        return SUCCESS();

    } // native_auth_establish_context

    // =-=-=-=-=-=-=-
    // handle an client-side auth request call 
    eirods::error native_auth_client_request(
        eirods::auth_plugin_context& _ctx,
        rcComm_t*                    _comm ) {
        // =-=-=-=-=-=-=-
        // validate incoming parameters
        if( !_ctx.valid< eirods::native_auth_object >().ok() ) {
            return ERROR( 
                       SYS_INVALID_INPUT_PARAM,
                       "invalid plugin context" );
        }  
        
        // =-=-=-=-=-=-=-
        // make the call to our auth request
        authRequestOut_t* auth_request = 0;
        int status = rcAuthRequest( 
                         _comm,
                         &auth_request );
        if( status < 0 ) {
            free( auth_request->challenge );
            free( auth_request );
            return ERROR( 
                       status,
                       "call to rcAuthRequest failed." );
        
        } else { 
            // =-=-=-=-=-=-=-
            // get the auth object
            eirods::native_auth_object_ptr ptr = boost::dynamic_pointer_cast< 
                                                  eirods::native_auth_object >( _ctx.fco() );
            // =-=-=-=-=-=-=-
            // cache the challenge
            ptr->request_result( auth_request->challenge );

            free( auth_request->challenge );
            free( auth_request );
            return SUCCESS();
        
        }

    } // native_auth_client_request

    // =-=-=-=-=-=-=-
    // handle an agent-side auth request call 
    eirods::error native_auth_agent_request(
        eirods::auth_plugin_context& _ctx,
        rsComm_t*                    _comm ) {
        // =-=-=-=-=-=-=-
        // validate incoming parameters
        if( !_ctx.valid< eirods::native_auth_object >().ok() ) {
            return ERROR( 
                       SYS_INVALID_INPUT_PARAM,
                       "invalid plugin context" );
        } else if( !_comm ) {
            return ERROR( 
                       SYS_INVALID_INPUT_PARAM,
                       "null comm ptr" );
        } 
        
        // =-=-=-=-=-=-=-
        // generate a random buffer and copy it to the challenge
        char buf[ CHALLENGE_LEN+2 ];
        get64RandomBytes( buf );
        
        // =-=-=-=-=-=-=-
        // get the auth object
        eirods::native_auth_object_ptr ptr = boost::dynamic_pointer_cast< 
                                              eirods::native_auth_object >( _ctx.fco() );
        // =-=-=-=-=-=-=-
        // cache the challenge
        ptr->request_result( buf );

        // =-=-=-=-=-=-=-
        // cache the challenge in the server for later usage
        _rsSetAuthRequestGetChallenge( buf );
        
        // =-=-=-=-=-=-=-
        // win!
        return SUCCESS(); 
         
    } // native_auth_agent_request

    // =-=-=-=-=-=-=-
    // handle a client-side auth request call 
    eirods::error native_auth_client_response(
        eirods::auth_plugin_context& _ctx,
        rcComm_t*                    _comm ) {
        // =-=-=-=-=-=-=-
        // validate incoming parameters
        if( !_ctx.valid< eirods::native_auth_object >().ok() ) {
            return ERROR( 
                    SYS_INVALID_INPUT_PARAM,
                    "invalid plugin context" );
        } else if( !_comm ) { 
            return ERROR( 
                    SYS_INVALID_INPUT_PARAM,
                    "null rcComm_t ptr" );
        }
        
        // =-=-=-=-=-=-=-
        // get the auth object
        eirods::native_auth_object_ptr ptr = boost::dynamic_pointer_cast< 
                                                 eirods::native_auth_object >( 
                                                     _ctx.fco() );
        // =-=-=-=-=-=-=-
        // build the response string
        char response[ RESPONSE_LEN+2 ];
        strncpy( 
            response,
            ptr->digest().c_str(),
            RESPONSE_LEN+2 );
               
        // =-=-=-=-=-=-=-
        // build the username#zonename string
        std::string user_name = ptr->user_name() +
                                "#"              +
                                ptr->zone_name();
        char username[ MAX_NAME_LEN ];
        strncpy(
            username, 
            user_name.c_str(),
            MAX_NAME_LEN );

        authResponseInp_t auth_response;
        auth_response.response = response;
        auth_response.username = username;
        int status = rcAuthResponse(
                         _comm,
                         &auth_response );
        if( status < 0 ) {
            return ERROR(
                       status,
                       "call to rcAuthResponse failed." );
        } else {
            return SUCCESS();

        }
           
    } // native_auth_client_response

    // =-=-=-=-=-=-=-
    // handle an agent-side auth request call 
    eirods::error native_auth_agent_response(
        eirods::auth_plugin_context& _ctx,
        rsComm_t*                    _comm,
        authResponseInp_t*           _resp ) {
        // =-=-=-=-=-=-=-
        // validate incoming parameters
        if( !_ctx.valid().ok() ) {
            return ERROR( 
                    SYS_INVALID_INPUT_PARAM,
                    "invalid plugin context" );
        } else if( !_resp ) {
            return ERROR( 
                    SYS_INVALID_INPUT_PARAM,
                    "null authResponseInp_t ptr" );
        } else if( !_comm ) {
            return ERROR( 
                    SYS_INVALID_INPUT_PARAM,
                    "null rsComm_t ptr" );
        }

        int status;
        char *bufp;
        authCheckInp_t authCheckInp;
        authCheckOut_t *authCheckOut = NULL;
        rodsServerHost_t *rodsServerHost;

        char digest[RESPONSE_LEN+2];
        char md5Buf[CHALLENGE_LEN+MAX_PASSWORD_LEN+2];
        char serverId[MAX_PASSWORD_LEN+2];
        MD5_CTX context;

        bufp = _rsAuthRequestGetChallenge();

        /* need to do NoLogin because it could get into inf loop for cross 
         * zone auth */

        status = getAndConnRcatHostNoLogin ( _comm, MASTER_RCAT, 
                _comm->proxyUser.rodsZone, &rodsServerHost);
        if (status < 0) {
            return ERROR(
                       status,
                       "getAndConnRcatHostNoLogin failed" );
        }

        memset (&authCheckInp, 0, sizeof (authCheckInp)); 
        authCheckInp.challenge = bufp;
        authCheckInp.response = _resp->response;
        authCheckInp.username = _resp->username;

        if (rodsServerHost->localFlag == LOCAL_HOST) {
            status = rsAuthCheck ( _comm, &authCheckInp, &authCheckOut);
        } else {
            status = rcAuthCheck (rodsServerHost->conn, &authCheckInp, &authCheckOut);
            /* not likely we need this connection again */
            rcDisconnect(rodsServerHost->conn);
            rodsServerHost->conn = NULL;
        }
        if (status < 0 || authCheckOut == NULL ) { // JMC cppcheck 
            return ERROR(
                    status,
                    "rxAuthCheck failed" );
        }

        if (rodsServerHost->localFlag != LOCAL_HOST) {
            if (authCheckOut->serverResponse == NULL) {
                rodsLog(LOG_NOTICE, "Warning, cannot authenticate remote server, no serverResponse field");
                if (requireServerAuth) {
                    return ERROR(
                               REMOTE_SERVER_AUTH_NOT_PROVIDED,
                               "Authentication disallowed, no serverResponse field" );
                }
            }
            else {
                char *cp;
                int OK, len, i;
                if (*authCheckOut->serverResponse == '\0') {
                    rodsLog(LOG_NOTICE, "Warning, cannot authenticate remote server, serverResponse field is empty");
                    if (requireServerAuth) { 
                        return ERROR(
                                   REMOTE_SERVER_AUTH_EMPTY,
                                   "Authentication disallowed, empty serverResponse" );  
                    }
                }
                else {
                    char username2[NAME_LEN+2];
                    char userZone[NAME_LEN+2];
                    memset(md5Buf, 0, sizeof(md5Buf));
                    strncpy(md5Buf, authCheckInp.challenge, CHALLENGE_LEN);
                    parseUserName(_resp->username, username2, userZone);
                    getZoneServerId(userZone, serverId);
                    len = strlen(serverId);
                    if (len <= 0) {
                        rodsLog (LOG_NOTICE, "rsAuthResponse: Warning, cannot authenticate the remote server, no RemoteZoneSID defined in server.config", status);
                        if (requireServerAuth) {
                            return ERROR(
                                       REMOTE_SERVER_SID_NOT_DEFINED,
                                       "Authentication disallowed, no RemoteZoneSID defined" );  
                        }
                    }
                    else { 
                        strncpy(md5Buf+CHALLENGE_LEN, serverId, len);
                        MD5Init (&context);
                        MD5Update (&context, (unsigned char*)md5Buf, 
                                CHALLENGE_LEN+MAX_PASSWORD_LEN);
                        MD5Final ((unsigned char*)digest, &context);
                        for (i=0;i<RESPONSE_LEN;i++) {
                            if (digest[i]=='\0') digest[i]++;  /* make sure 'string' doesn't
                                                                  end early*/
                        }
                        cp = authCheckOut->serverResponse;
                        OK=1;
                        for (i=0;i<RESPONSE_LEN;i++) {
                            if (*cp++ != digest[i]) OK=0;
                        }
                        rodsLog(LOG_DEBUG, "serverResponse is OK/Not: %d", OK);
                        if (OK==0) {
                            return ERROR(
                                       REMOTE_SERVER_AUTHENTICATION_FAILURE,
                                       "Server response incorrect, authentication disallowed" );
                        }
                    }
                }
            }
        }

        /* Set the clientUser zone if it is null. */
        if (strlen( _comm->clientUser.rodsZone)==0) {
            zoneInfo_t *tmpZoneInfo;
            status = getLocalZoneInfo (&tmpZoneInfo);
            if (status < 0) {
                free (authCheckOut);
                return ERROR( 
                           status,
                           "getLocalZoneInfo failed" );
            }
            strncpy( _comm->clientUser.rodsZone,
                    tmpZoneInfo->zoneName, NAME_LEN);
        }


        /* have to modify privLevel if the icat is a foreign icat because
         * a local user in a foreign zone is not a local user in this zone
         * and vice vera for a remote user
         */
        if (rodsServerHost->rcatEnabled == REMOTE_ICAT) {
            /* proxy is easy because rodsServerHost is based on proxy user */
            if (authCheckOut->privLevel == LOCAL_PRIV_USER_AUTH)
                authCheckOut->privLevel = REMOTE_PRIV_USER_AUTH;
            else if (authCheckOut->privLevel == LOCAL_USER_AUTH)
                authCheckOut->privLevel = REMOTE_USER_AUTH;

            /* adjust client user */
            if (strcmp ( _comm->proxyUser.userName,  _comm->clientUser.userName) 
                    == 0) {
                authCheckOut->clientPrivLevel = authCheckOut->privLevel;
            } else {
                zoneInfo_t *tmpZoneInfo;
                status = getLocalZoneInfo (&tmpZoneInfo);
                if (status < 0) {
                    free (authCheckOut);
                    return ERROR(
                               status,
                               "getLocalZoneInfo failed" );
                }

                if (strcmp (tmpZoneInfo->zoneName,  _comm->clientUser.rodsZone)
                        == 0) {
                    /* client is from local zone */
                    if (authCheckOut->clientPrivLevel == REMOTE_PRIV_USER_AUTH) {
                        authCheckOut->clientPrivLevel = LOCAL_PRIV_USER_AUTH;
                    } else if (authCheckOut->clientPrivLevel == REMOTE_USER_AUTH) {
                        authCheckOut->clientPrivLevel = LOCAL_USER_AUTH;
                    }
                } else {
                    /* client is from remote zone */
                    if (authCheckOut->clientPrivLevel == LOCAL_PRIV_USER_AUTH) {
                        authCheckOut->clientPrivLevel = REMOTE_USER_AUTH;
                    } else if (authCheckOut->clientPrivLevel == LOCAL_USER_AUTH) {
                        authCheckOut->clientPrivLevel = REMOTE_USER_AUTH;
                    }
                }
            }
        } else if (strcmp ( _comm->proxyUser.userName,  _comm->clientUser.userName)
                == 0) {
            authCheckOut->clientPrivLevel = authCheckOut->privLevel;
        }

        status = check_proxy_user_privelges( _comm, authCheckOut->privLevel );

        if (status < 0) {
            free (authCheckOut);
            return ERROR( 
                       status,
                       "check_proxy_user_privelges failed" );
        } 

        rodsLog(LOG_NOTICE,
                "rsAuthResponse set proxy authFlag to %d, client authFlag to %d, user:%s proxy:%s client:%s",
                authCheckOut->privLevel,
                authCheckOut->clientPrivLevel,
                authCheckInp.username,
                 _comm->proxyUser.userName,
                 _comm->clientUser.userName);

        if (strcmp ( _comm->proxyUser.userName,  _comm->clientUser.userName) != 0) {
             _comm->proxyUser.authInfo.authFlag = authCheckOut->privLevel;
             _comm->clientUser.authInfo.authFlag = authCheckOut->clientPrivLevel;
        } else {	/* proxyUser and clientUser are the same */
             _comm->proxyUser.authInfo.authFlag =
                 _comm->clientUser.authInfo.authFlag = authCheckOut->privLevel;
        } 

        /*** Added by RAJA Nov 16 2010 **/
        if (authCheckOut->serverResponse != NULL) free(authCheckOut->serverResponse);
        /*** Added by RAJA Nov 16 2010 **/
        free (authCheckOut);

        return SUCCESS();

    } // native_auth_agent_response

    // =-=-=-=-=-=-=-
    // stub for ops that the native plug does 
    // not need to support 
    eirods::error native_auth_success_stub( 
        eirods::auth_plugin_context& _ctx ) {
        return SUCCESS();

    } // native_auth_success_stub

    // =-=-=-=-=-=-=-
    // derive a new native_auth auth plugin from
    // the auth plugin base class for handling
    // native authentication
    class native_auth_plugin : public eirods::auth {
    public:
        native_auth_plugin( 
           const std::string& _nm, 
           const std::string& _ctx ) :
               eirods::auth( 
                   _nm, 
                   _ctx ) {
        } // ctor

        ~native_auth_plugin() {
        }

    }; // class native_auth_plugin

    // =-=-=-=-=-=-=-
    // factory function to provide instance of the plugin
    eirods::auth* plugin_factory( 
        const std::string& _inst_name, 
        const std::string& _context ) {
        // =-=-=-=-=-=-=-
        // create an auth object
        native_auth_plugin* nat = new native_auth_plugin( 
                                          _inst_name,
                                          _context );
        if( !nat ) {
            rodsLog( LOG_ERROR, "plugin_factory - failed to alloc native_auth_plugin" );
            return 0;
        }
        
        // =-=-=-=-=-=-=-
        // fill in the operation table mapping call 
        // names to function names
        nat->add_operation( eirods::AUTH_CLIENT_START,         "native_auth_client_start" );
        nat->add_operation( eirods::AUTH_AGENT_START,          "native_auth_success_stub" );
        nat->add_operation( eirods::AUTH_ESTABLISH_CONTEXT,    "native_auth_establish_context" );
        nat->add_operation( eirods::AUTH_CLIENT_AUTH_REQUEST,  "native_auth_client_request" );
        nat->add_operation( eirods::AUTH_AGENT_AUTH_REQUEST,   "native_auth_agent_request" );
        nat->add_operation( eirods::AUTH_CLIENT_AUTH_RESPONSE, "native_auth_client_response" );
        nat->add_operation( eirods::AUTH_AGENT_AUTH_RESPONSE,  "native_auth_agent_response" );
        nat->add_operation( eirods::AUTH_AGENT_AUTH_VERIFY,    "native_auth_success_stub" );

        eirods::auth* auth = dynamic_cast< eirods::auth* >( nat );
        if( !auth ) {
            rodsLog( LOG_ERROR, "failed to dynamic cast to eirods::auth*" );
        }

        return auth;

    } // plugin_factory

}; // extern "C"
