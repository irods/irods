/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* See authResponse.h for a description of this API call.*/

#include "authRequest.h"
#include "authResponse.h"
#include "authCheck.h"
#include "miscServerFunct.h"


/* Set requireServerAuth to 1 to fail authentications from
   un-authenticated Servers (for example, if the LocalZoneSID 
   is not set) */
#define requireServerAuth 0 

int
rsAuthResponse (rsComm_t *rsComm, authResponseInp_t *authResponseInp)
{
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

   status = getAndConnRcatHostNoLogin (rsComm, SLAVE_RCAT, 
    rsComm->proxyUser.rodsZone, &rodsServerHost);
   if (status < 0) {
      return(status);
   }

   memset (&authCheckInp, 0, sizeof (authCheckInp)); 
   authCheckInp.challenge = bufp;
   authCheckInp.response = authResponseInp->response;
   authCheckInp.username = authResponseInp->username;

   if (rodsServerHost->localFlag == LOCAL_HOST) {
      status = rsAuthCheck (rsComm, &authCheckInp, &authCheckOut);
   } else {
      status = rcAuthCheck (rodsServerHost->conn, &authCheckInp, &authCheckOut);
      /* not likely we need this connection again */
      rcDisconnect(rodsServerHost->conn);
      rodsServerHost->conn = NULL;
   }
   if (status < 0) {
      rodsLog (LOG_NOTICE,
            "rsAuthResponse: rxAuthCheck failed, status = %d", status);
      return (status);
   }

   if (rodsServerHost->localFlag != LOCAL_HOST) {
      if (authCheckOut->serverResponse == NULL) {
	 rodsLog(LOG_NOTICE, "Warning, cannot authenticate remote server, no serverResponse field");
	 if (requireServerAuth) {
	    rodsLog(LOG_NOTICE, "Authentication disallowed, no serverResponse field");
	    return(REMOTE_SERVER_AUTH_NOT_PROVIDED);
	 }
      }
      else {
	 char *cp;
	 int OK, len, i;
	 if (*authCheckOut->serverResponse == '\0') {
	    rodsLog(LOG_NOTICE, "Warning, cannot authenticate remote server, serverResponse field is empty");
	    if (requireServerAuth) { 
	       rodsLog(LOG_NOTICE, "Authentication disallowed, empty serverResponse");
	       return(REMOTE_SERVER_AUTH_EMPTY);  
	    }
	 }
	 else {
	    char username2[NAME_LEN+2];
	    char userZone[NAME_LEN+2];
	    memset(md5Buf, 0, sizeof(md5Buf));
	    strncpy(md5Buf, authCheckInp.challenge, CHALLENGE_LEN);
	    parseUserName(authResponseInp->username, username2, userZone);
	    getZoneServerId(userZone, serverId);
	    len = strlen(serverId);
	    if (len <= 0) {
	       rodsLog (LOG_NOTICE, "rsAuthResponse: Warning, cannot authenticate the remote server, no RemoteZoneSID defined in server.config", status);
	       if (requireServerAuth) {
		  rodsLog(LOG_NOTICE, "Authentication disallowed, no RemoteZoneSID defined");
		  return(REMOTE_SERVER_SID_NOT_DEFINED);  
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
		  rodsLog(LOG_NOTICE, "Server response incorrect, authentication disallowed");
		  return(REMOTE_SERVER_AUTHENTICATION_FAILURE);
	       }
	    }
	 }
      }
   }

   /* Set the clientUser zone if it is null. */
   if (strlen(rsComm->clientUser.rodsZone)==0) {
      zoneInfo_t *tmpZoneInfo;
      status = getLocalZoneInfo (&tmpZoneInfo);
      if (status < 0) {
	 free (authCheckOut);
	 return status;
      }
      strncpy(rsComm->clientUser.rodsZone,
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
        else if (authCheckOut->privLevel == LOCAL_PRIV_USER_AUTH)
            authCheckOut->privLevel = REMOTE_PRIV_USER_AUTH;

	/* adjust client user */
	if (strcmp (rsComm->proxyUser.userName, rsComm->clientUser.userName) 
	  == 0) {
            authCheckOut->clientPrivLevel = authCheckOut->privLevel;
	} else {
	    zoneInfo_t *tmpZoneInfo;
	    status = getLocalZoneInfo (&tmpZoneInfo);
            if (status < 0) {
                free (authCheckOut);
                return status;
            }

	    if (strcmp (tmpZoneInfo->zoneName, rsComm->clientUser.rodsZone)
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
   } else if (strcmp (rsComm->proxyUser.userName, rsComm->clientUser.userName)
    == 0) {
        authCheckOut->clientPrivLevel = authCheckOut->privLevel;
   }

   status = chkProxyUserPriv (rsComm, authCheckOut->privLevel);

   if (status < 0) {
      free (authCheckOut);
      return status;
   } 

   rodsLog(LOG_NOTICE,
    "rsAuthResponse set proxy authFlag to %d, client authFlag to %d, user:%s proxy:%s client:%s",
          authCheckOut->privLevel,
          authCheckOut->clientPrivLevel,
          authCheckInp.username,
          rsComm->proxyUser.userName,
          rsComm->clientUser.userName);

   if (strcmp (rsComm->proxyUser.userName, rsComm->clientUser.userName) != 0) {
      rsComm->proxyUser.authInfo.authFlag = authCheckOut->privLevel;
      rsComm->clientUser.authInfo.authFlag = authCheckOut->clientPrivLevel;
   } else {	/* proxyUser and clientUser are the same */
      rsComm->proxyUser.authInfo.authFlag =
      rsComm->clientUser.authInfo.authFlag = authCheckOut->privLevel;
   } 

   /*** Added by RAJA Nov 16 2010 **/
   if (authCheckOut->serverResponse != NULL) free(authCheckOut->serverResponse);
   /*** Added by RAJA Nov 16 2010 **/
   free (authCheckOut);

   return (status);
} 

int
chkProxyUserPriv (rsComm_t *rsComm, int proxyUserPriv)
{
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

