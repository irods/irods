/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* See krbAuthRequest.h for a description of this API call.*/

#include "krbAuthRequest.h"
#include "authResponse.h"
#include "genQuery.h"
#include "rsGlobalExtern.h"

static int krbAuthReqStatus=0;
static int krbAuthReqError=0;
static char krbAuthReqErrorMsg[1000];

int
rsKrbAuthRequest (rsComm_t *rsComm, krbAuthRequestOut_t **krbAuthRequestOut)
{
    krbAuthRequestOut_t *result;
    int status;

    if (krbAuthReqStatus==1) {
       krbAuthReqStatus=0;
       if (krbAuthReqError != 0) {
	  rodsLogAndErrorMsg( LOG_NOTICE, &rsComm->rError, krbAuthReqError,
			      krbAuthReqErrorMsg);
       }
       return krbAuthReqError;
    }

    *krbAuthRequestOut = (krbAuthRequestOut_t*)malloc(sizeof(krbAuthRequestOut_t));
    memset((char *)*krbAuthRequestOut, 0, sizeof(krbAuthRequestOut_t));

    result = *krbAuthRequestOut;

#if defined(KRB_AUTH)
    status = ikrbSetupCreds(NULL, rsComm, KerberosName,
    			    &result->serverName);
    if (status==0) {
        rsComm->gsiRequest=2;
    }
    return(status);
#else
    status = KRB_NOT_BUILT_INTO_SERVER;
    rodsLog (LOG_ERROR,
	    "rsKrbAuthRequest failed KRB_NOT_BUILT_INTO_SERVER, status = %d",
	     status);
    return (status);
#endif

}

int ikrbServersideAuth(rsComm_t *rsComm) {
   int status;
#if defined(KRB_AUTH)
   char clientName[500];
   genQueryInp_t genQueryInp;
   genQueryOut_t *genQueryOut;
   char condition1[MAX_NAME_LEN];
   char condition2[MAX_NAME_LEN*2];
   char *tResult;
   int privLevel;
   int clientPrivLevel;

   krbAuthReqStatus=1;

   status = ikrbEstablishContextServerside(rsComm, clientName, 
					    500);
#ifdef KRB_DEBUG
   if (status==0) printf("clientName:%s\n",clientName);
#endif

   if (status) {
      krbAuthReqError = KRB_QUERY_INTERNAL_ERROR;
      return(status);
   }


   memset (&genQueryInp, 0, sizeof (genQueryInp_t));

   if (strlen(rsComm->clientUser.userName)>0) {  

      snprintf (condition1, MAX_NAME_LEN, "='%s'", 
		rsComm->clientUser.userName);
      addInxVal (&genQueryInp.sqlCondInp, COL_USER_NAME, condition1);

      snprintf (condition2, MAX_NAME_LEN*2, "='%s'",
		clientName);
      addInxVal (&genQueryInp.sqlCondInp, COL_USER_DN, condition2);

/*    The user's principal name should be sufficient
      snprintf (condition2, MAX_NAME_LEN*2, "='local'");
      addInxVal (&genQueryInp.sqlCondInp, COL_ZONE_TYPE, condition2); */

      addInxIval (&genQueryInp.selectInp, COL_USER_ID, 1);
      addInxIval (&genQueryInp.selectInp, COL_USER_TYPE, 1);

      genQueryInp.maxRows = 2;

      status = rsGenQuery (rsComm, &genQueryInp, &genQueryOut);
   }
   else {
      krbAuthReqError = KRB_QUERY_INTERNAL_ERROR;
      return(KRB_QUERY_INTERNAL_ERROR);
   }

   if (status == CAT_NO_ROWS_FOUND) {
      krbAuthReqError = KRB_USER_DN_NOT_FOUND;
      return(KRB_USER_DN_NOT_FOUND);
   }
   if (status < 0) {
      rodsLog (LOG_NOTICE,
	       "ikrbServersideAuth: rsGenQuery failed, status = %d", status);
      snprintf(krbAuthReqErrorMsg, 1000, 
	       "ikrbServersideAuth: rsGenQuery failed, status = %d", status);
      krbAuthReqError = status;
      return (status);
   }

   if (genQueryOut->rowCnt !=1 || genQueryOut->attriCnt != 2) {
      krbAuthReqError = KRB_NAME_MATCHES_MULTIPLE_USERS;
      return(KRB_NAME_MATCHES_MULTIPLE_USERS);
   }

#ifdef KRB_DEBUG
   printf("Results=%d\n",genQueryOut->rowCnt);
#endif

   tResult = genQueryOut->sqlResult[0].value;
#ifdef KRB_DEBUG
   printf("0:%s\n",tResult);
#endif
   tResult = genQueryOut->sqlResult[1].value;
#ifdef KRB_DEBUG
   printf("1:%s\n",tResult);
#endif
   privLevel = LOCAL_USER_AUTH;
   clientPrivLevel = LOCAL_USER_AUTH;

   if (strcmp(tResult, "rodsadmin") == 0) {
      privLevel = LOCAL_PRIV_USER_AUTH;
      clientPrivLevel = LOCAL_PRIV_USER_AUTH;
   }

   status = chkProxyUserPriv (rsComm, privLevel);

   if (status < 0) {
      krbAuthReqError = status;
      return status;
   }

   rsComm->proxyUser.authInfo.authFlag = privLevel;
   rsComm->clientUser.authInfo.authFlag = clientPrivLevel;

   return status;
#else
    status = KRB_NOT_BUILT_INTO_SERVER;
    rodsLog (LOG_ERROR,
	    "ikrbServersideAuth failed KRB_NOT_BUILT_INTO_SERVER, status = %d",
	     status);
    return (status);
#endif
}

