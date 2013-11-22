/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* See authCheck.h for a description of this API call.*/

#include "authRequest.h"
#include "authCheck.h"
#include "icatHighLevelRoutines.h"
#include "miscServerFunct.h"

// =-=-=-=-=-=-=-
// stl includes
#include <string>

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_kvp_string_parser.h"
#include "eirods_auth_constants.h"

int
rsAuthCheck (rsComm_t *rsComm, authCheckInp_t *authCheckInp, 
		authCheckOut_t **authCheckOut)
{
#ifdef RODS_CAT
    int status;
    int privLevel;
    int clientPrivLevel;
    authCheckOut_t *result;
    unsigned char *digest;
    char md5Buf[CHALLENGE_LEN+MAX_PASSWORD_LEN+2];
    MD5_CTX context;
    char ServerID[MAX_PASSWORD_LEN+2];

    *authCheckOut = (authCheckOut_t*)malloc(sizeof(authCheckOut_t));
    memset((char *)*authCheckOut, 0, sizeof(authCheckOut_t));

    // =-=-=-=-=-=-=-
    // the incoming response string might be a kvp string
    // holding the auth scheme as well as the response
    // try to parse it
    std::string orig_resp = authCheckInp->response;
    eirods::kvp_map_t kvp;
    eirods::error ret = eirods::parse_kvp_string(
            orig_resp,
            kvp );
    std::string scheme;
    std::string response = authCheckInp->response; 
    if( ret.ok() ) {
        if( kvp.end() != kvp.find( eirods::AUTH_SCHEME_KEY ) &&
            kvp.end() != kvp.find( eirods::AUTH_RESPONSE_KEY ) ) {
            response = kvp[ eirods::AUTH_RESPONSE_KEY ];
            scheme   = kvp[ eirods::AUTH_SCHEME_KEY   ];
        }   
    }
    status = chlCheckAuth(
                 rsComm, 
                 scheme.c_str(),
                 authCheckInp->challenge, 
                 const_cast< char* >( response.c_str() ),
                 authCheckInp->username,
                 &privLevel, 
                 &clientPrivLevel );
    if (status < 0) {
        rodsLog (LOG_NOTICE, 
                "rsAuthCheck: chlCheckAuth status = %d", status);
    }

    if (status == 0) {
        int len, i;
        result = *authCheckOut;
        result->privLevel = privLevel;
        result->clientPrivLevel = clientPrivLevel;

        /* Add a hash to authenticate this server to the other server */
        memset(md5Buf, 0, sizeof(md5Buf));
        strncpy(md5Buf, authCheckInp->challenge, CHALLENGE_LEN);

        getZoneServerId("", ServerID); /* get our local zone SID */
        len = strlen(ServerID);
        digest = ( unsigned char* )malloc(RESPONSE_LEN+2);
        if (len <=0) {
            rodsLog (LOG_DEBUG, 
                    "rsAuthCheck: Warning, cannot authenticate this server to remote server, no LocalZoneSID defined in server.config", status);
            memset(digest, 0, RESPONSE_LEN);
        }
        else {
            strncpy(md5Buf+CHALLENGE_LEN, ServerID, len);

            obfMakeOneWayHash(
                              HASH_TYPE_DEFAULT, 
                              (unsigned char*)md5Buf, 
                              CHALLENGE_LEN+MAX_PASSWORD_LEN,
                              (unsigned char*)digest);

            for (i=0;i<RESPONSE_LEN;i++) {
                if (digest[i]=='\0') digest[i]++;  /* make sure 'string' doesn't
                                                      end early */
            }
        }
        result->serverResponse = (char*)digest;
    }

    return (status);
#else
    /* this may be a gateway to the rcat host */
    rodsServerHost_t *rodsServerHost;
    int status;

    status = getAndConnRcatHostNoLogin (rsComm, MASTER_RCAT,
            rsComm->proxyUser.rodsZone, &rodsServerHost);

    if (status < 0) {
        rodsLog (LOG_NOTICE,
                "rsAuthCheck:getAndConnRcatHostNoLogin() failed. erro=%d", status);
        return (status);
    }

    if (rodsServerHost->localFlag == LOCAL_HOST) {
        return ( SYS_NO_ICAT_SERVER_ERR);
    } else {
        status = rcAuthCheck (rodsServerHost->conn, authCheckInp, authCheckOut);
    }
    return status;
#endif
} 

