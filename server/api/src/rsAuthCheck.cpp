/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* See authCheck.h for a description of this API call.*/

#include "authRequest.h"
#include "authCheck.h"
#include "icatHighLevelRoutines.hpp"
#include "miscServerFunct.hpp"
#include "irods_configuration_keywords.hpp"
#include "rsAuthCheck.hpp"

// =-=-=-=-=-=-=-
// stl includes
#include <string>

// =-=-=-=-=-=-=-
#include "irods_kvp_string_parser.hpp"
#include "irods_auth_constants.hpp"

int rsAuthCheck(
        rsComm_t *rsComm,
        authCheckInp_t *authCheckInp,
        authCheckOut_t **authCheckOut ) {
    std::string svc_role;
    irods::error ret = get_catalog_service_role(svc_role);
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
        int status;
        int privLevel;
        int clientPrivLevel;
        authCheckOut_t *result;
        unsigned char *digest;
        char md5Buf[CHALLENGE_LEN + MAX_PASSWORD_LEN + 2];
        char ServerID[MAX_PASSWORD_LEN + 2];

        *authCheckOut = ( authCheckOut_t* )malloc( sizeof( authCheckOut_t ) );
        memset( ( char * )*authCheckOut, 0, sizeof( authCheckOut_t ) );

        // =-=-=-=-=-=-=-
        // the incoming response string might be a kvp string
        // holding the auth scheme as well as the response
        // try to parse it
        // NOTE :: incoming response string is longer than RESPONSE_LEN so we
        //         cannot directly assign it, it will need to be properly terminated
        //         and not contain any magic characters
        std::string scheme;
        std::string orig_resp = authCheckInp->response;
        std::string response  = authCheckInp->response;

        // due to backward compatibility reasons, an unsanitized binary string is sent
        // as the 'response' portion of the KVP which may cause the parser to fail.  we
        // evade the error and manually extract the reponse as necessary
        std::string::size_type schem_key_pos = response.find( irods::AUTH_SCHEME_KEY );
        bool have_auth_scheme_key = schem_key_pos != std::string::npos;
        if ( have_auth_scheme_key ) {
            irods::kvp_map_t kvp;
            irods::error ret = irods::parse_kvp_string( orig_resp, kvp );
            if ( kvp.end() != kvp.find( irods::AUTH_SCHEME_KEY ) ) {
                scheme = kvp[ irods::AUTH_SCHEME_KEY ];

                // =-=-=-=-=-=-=-
                // subset the 'response' string from the incoming kvp set
                size_t response_key_pos = response.find( irods::AUTH_RESPONSE_KEY );
                if ( response_key_pos != std::string::npos ) {
                    char *response_ptr = authCheckInp->response +
                                         ( response_key_pos +
                                           irods::AUTH_RESPONSE_KEY.length() +
                                           irods::KVP_DEF_ASSOCIATION.length() );
                    response.assign( response_ptr, RESPONSE_LEN + 2 );
                }

            }
        }
        else {
            response.assign( authCheckInp->response, RESPONSE_LEN );

        }

        status = chlCheckAuth(
                     rsComm,
                     scheme.c_str(),
                     authCheckInp->challenge,
                     const_cast< char* >( response.c_str() ),
                     authCheckInp->username,
                     &privLevel,
                     &clientPrivLevel );
        if ( status < 0 ) {
            rodsLog( LOG_NOTICE,
                     "rsAuthCheck: chlCheckAuth status = %d", status );
        }

        if ( status == 0 ) {
            int len, i;
            result = *authCheckOut;
            result->privLevel = privLevel;
            result->clientPrivLevel = clientPrivLevel;

            /* Add a hash to authenticate this server to the other server */
            memset( md5Buf, 0, sizeof( md5Buf ) );
            strncpy( md5Buf, authCheckInp->challenge, CHALLENGE_LEN );

            getZoneServerId( "", ServerID ); /* get our local zone SID */
            len = strlen( ServerID );
            digest = ( unsigned char* )malloc( RESPONSE_LEN + 2 );
            memset( digest, 0, RESPONSE_LEN + 2 );
            if ( len <= 0 ) {
                rodsLog( LOG_DEBUG,
                         "rsAuthCheck: Warning, cannot authenticate this server to remote server, no LocalZoneSID defined in server_config.json", status );
            }
            else {
                strncpy( md5Buf + CHALLENGE_LEN, ServerID, len );

                obfMakeOneWayHash(
                    HASH_TYPE_DEFAULT,
                    ( unsigned char* )md5Buf,
                    CHALLENGE_LEN + MAX_PASSWORD_LEN,
                    ( unsigned char* )digest );

                for ( i = 0; i < RESPONSE_LEN; i++ ) {
                    if ( digest[i] == '\0' ) {
                        digest[i]++;
                    }  /* make sure 'string' doesn't
                                                          end early */
                }
            }
            result->serverResponse = ( char* )digest;
        }

        return status;
    } else if( irods::CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
        /* this may be a gateway to the rcat host */
        rodsServerHost_t *rodsServerHost;
        int status;

        status = getAndConnRcatHostNoLogin( rsComm, MASTER_RCAT,
                                            rsComm->proxyUser.rodsZone, &rodsServerHost );

        if ( status < 0 ) {
            rodsLog( LOG_NOTICE,
                     "rsAuthCheck:getAndConnRcatHostNoLogin() failed. erro=%d", status );
            return status;
        }

        if ( rodsServerHost->localFlag == LOCAL_HOST ) {
            return SYS_NO_ICAT_SERVER_ERR;
        }
        else {
            status = rcAuthCheck( rodsServerHost->conn, authCheckInp, authCheckOut );
        }
        return status;
    } else {
        rodsLog(
            LOG_ERROR,
            "role not supported [%s]",
            svc_role.c_str() );
        return SYS_SERVICE_ROLE_NOT_SUPPORTED;
    }
}
