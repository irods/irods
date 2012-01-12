/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* clientLogin.c - client login
 *
 * Perform a series of calls to complete a client login; to
 * authenticate.
 */

#include "rodsClient.h"

static char prevChallengeSignitureClient[200];

char *getSessionSignitureClientside() {
   return(prevChallengeSignitureClient);
}


int printError(rcComm_t *Conn, int status, char *routineName) {
   char *mySubName;
   char *myName;
   rError_t *Err;
   rErrMsg_t *ErrMsg;
   int i, len;
   if (Conn) {
      if (Conn->rError) {
	 Err = Conn->rError;
	 len = Err->len;
	 for (i=0;i<len;i++) {
	    ErrMsg = Err->errMsg[i];
	    fprintf(stderr, "Level %d: %s\n",i, ErrMsg->msg);
	 }
      }
   }
   myName = rodsErrorName(status, &mySubName);
   fprintf(stderr, "%s failed with error %d %s %s\n", routineName,
	    status, myName, mySubName);

   return (0);
}

#ifdef GSI_AUTH
int clientLoginGsi(rcComm_t *Conn) 
{
   int status;
   gsiAuthRequestOut_t *gsiAuthReqOut;
   char *myName;
   char *serverDN;

   status = igsiSetupCreds(Conn, NULL, NULL, &myName);

   if (status) {
      printError(Conn, status, "igsiSetupCreds");
      return(status);
   }

   /*   printf("Client-side DN is:%s\n",myName); */

   status = rcGsiAuthRequest(Conn, &gsiAuthReqOut);
   if (status) {
      printError(Conn, status, "rcGsiAuthRequest");
      return(status);
   }

   /*   printf("Server-side DN is:%s\n", gsiAuthReqOut->serverDN); */

   serverDN = getenv("irodsServerDn"); /* Use irodsServerDn if defined */
   if (serverDN == NULL) {
      serverDN = getenv("SERVER_DN");  /* NULL or the SERVER_DN string */
   }

   status = igsiEstablishContextClientside(Conn, serverDN, 0);
   if (status) {
      printError(Conn, status, "igsiEstablishContextClientside");
      return(status);
   }

   /* Now, check if it actually succeeded */
   status = rcGsiAuthRequest(Conn, &gsiAuthReqOut);
   if (status) {
      printf("Error from iRODS Server:\n");
      printError(Conn, status, "GSI Authentication");
      return(status);
   }

   Conn->loggedIn = 1;

   return(0);
}
#endif

#ifdef KRB_AUTH
int clientLoginKrb(rcComm_t *Conn) 
{
   int status;
   krbAuthRequestOut_t *krbAuthReqOut;
   char *myName=0;
   char *serverName;
   static int KRB_debug=0;

   status = ikrbSetupCreds(Conn, NULL, NULL, &myName);
   if (status) {
      printError(Conn, status, "ikrbSetupCreds");
      return(status);
   }

   if (KRB_debug) {
      printf("Client-side Name:%s\n",myName); 
   }
   if (myName != 0) {
      free(myName);
   }

   status = rcKrbAuthRequest(Conn, &krbAuthReqOut);
   if (status) {
      printError(Conn, status, "rcKrbAuthRequest");
      return(status);
   }

   if (KRB_debug) {
      printf("Server-side Name provided:%s\n", krbAuthReqOut->serverName);
   }

   serverName = getenv("irodsServerDn"); /* Use irodsServerDn if defined */
   if (serverName == NULL) {
      serverName = getenv("SERVER_DN");  /* NULL or the SERVER_DN string */
   }
   if (serverName == NULL) {
      /* if not provided, use the server's claimed Name */
      serverName=  krbAuthReqOut->serverName; 
   }

   if (KRB_debug) {
      printf("Server Name to connect to:%s\n", serverName);
   }

   status = ikrbEstablishContextClientside(Conn, serverName, 0);
   if (status) {
      printError(Conn, status, "ikrbEstablishContextClientside");
      return(status);
   }

   /* Now, check if it actually succeeded */
   status = rcKrbAuthRequest(Conn, &krbAuthReqOut);
   if (status) {
      printf("Error from iRODS Server:\n");
      printError(Conn, status, "KRB Authentication");
      return(status);
   }

   Conn->loggedIn = 1;

   return(0);
}
#endif

int 
clientLogin(rcComm_t *Conn) 
{   
   int status, len, i;
   authRequestOut_t *authReqOut;
   authResponseInp_t authRespIn;
   char md5Buf[CHALLENGE_LEN+MAX_PASSWORD_LEN+2];
   char digest[RESPONSE_LEN+2];
   char userNameAndZone[NAME_LEN*2];
#ifndef USE_BOOST_FS
   struct stat statbuf;
#endif
   MD5_CTX context;
#ifdef OS_AUTH
   int doOsAuthentication = 0;
#endif

   if (Conn->loggedIn == 1) {
      /* already logged in */
      return (0);
   }

#ifdef GSI_AUTH
   if (ProcessType==CLIENT_PT) {
      char *getVar;
      getVar = getenv("irodsAuthScheme");
      if (getVar != NULL && strncmp("GSI",getVar,3)==0) {
	 status = clientLoginGsi(Conn);
	 return(status);
      }
   }
#endif

#ifdef KRB_AUTH
   if (ProcessType==CLIENT_PT) {
      char *getVar;
      getVar = getenv("irodsAuthScheme");
      if (getVar != NULL) {
	 if (strncmp("Kerberos",getVar,8)==0 ||
	     strncmp("kerberos",getVar,8)==0 ||
	     strncmp("KRB",getVar,3)==0) {
	    status = clientLoginKrb(Conn);
	    return(status);
	 }
      }
   }
#endif

#ifdef OS_AUTH
   if (ProcessType==CLIENT_PT) {
       char *getVar;
       getVar = getenv("irodsAuthScheme");
       if (getVar != NULL) {
           if (strncmp("OS",getVar,2)==0 ||
               strncmp("os",getVar,2)==0) {
               doOsAuthentication = 1;
           }
       }
   }
#endif

   status = rcAuthRequest(Conn, &authReqOut);
   if (status) {
      printError(Conn, status, "rcAuthRequest");
      return(status);
   }

   memset(md5Buf, 0, sizeof(md5Buf));
   strncpy(md5Buf, authReqOut->challenge, CHALLENGE_LEN);

/* Save a representation of some of the challenge string for use
   as a session signiture */
   snprintf(prevChallengeSignitureClient,200,"%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x",
	     (unsigned char)md5Buf[0], 
	     (unsigned char)md5Buf[1], 
	     (unsigned char)md5Buf[2], 
	     (unsigned char)md5Buf[3],
	     (unsigned char)md5Buf[4], 
	     (unsigned char)md5Buf[5], 
	     (unsigned char)md5Buf[6], 
	     (unsigned char)md5Buf[7],
	     (unsigned char)md5Buf[8], 
	     (unsigned char)md5Buf[9], 
	     (unsigned char)md5Buf[10], 
	     (unsigned char)md5Buf[11],
	     (unsigned char)md5Buf[12], 
	     (unsigned char)md5Buf[13], 
	     (unsigned char)md5Buf[14], 
	     (unsigned char)md5Buf[15]);


   if (strncmp(ANONYMOUS_USER, Conn->proxyUser.userName, NAME_LEN) == 0) {
      md5Buf[CHALLENGE_LEN+1]='\0';
      i = 0;
   }
#ifdef OS_AUTH
   else if (doOsAuthentication) {
       i = osauthGetAuth(authReqOut->challenge, Conn->proxyUser.userName, 
                         md5Buf+CHALLENGE_LEN, MAX_PASSWORD_LEN);
   }
#endif
   else {
      i = obfGetPw(md5Buf+CHALLENGE_LEN);
   }

   if (i != 0) {
	   int doStty=0;

#ifdef windows_platform
	  if (ProcessType != CLIENT_PT)
		return i;
#endif
      
#ifdef USE_BOOST_FS
      path p ("/bin/stty");
      if (exists(p)) {
#else
      if (stat ("/bin/stty", &statbuf) == 0) {
#endif
	 system("/bin/stty -echo 2> /dev/null");
	 doStty=1;
      }
      printf("Enter your current iRODS password:");
      fgets(md5Buf+CHALLENGE_LEN, MAX_PASSWORD_LEN, stdin);
      if (doStty) {
	 system("/bin/stty echo 2> /dev/null");
	 printf("\n");
      }
      len = strlen(md5Buf);
      md5Buf[len-1]='\0'; /* remove trailing \n */
   }
   MD5Init (&context);
   MD5Update (&context, (unsigned char*)md5Buf, CHALLENGE_LEN+MAX_PASSWORD_LEN);
   MD5Final ((unsigned char*)digest, &context);
   for (i=0;i<RESPONSE_LEN;i++) {
      if (digest[i]=='\0') digest[i]++;  /* make sure 'string' doesn't
					    end early*/
   }

   /* free the array and structure allocated by the rcAuthRequest */
   if (authReqOut != NULL) {
      if (authReqOut->challenge != NULL) {
         free(authReqOut->challenge);
      }
      free(authReqOut);
   }

   authRespIn.response=digest;
   /* the authentication is always for the proxyUser. */
   strncpy(userNameAndZone, Conn->proxyUser.userName, NAME_LEN);
   strncat(userNameAndZone, "#", NAME_LEN);
   strncat(userNameAndZone, Conn->proxyUser.rodsZone, NAME_LEN*2);
#ifdef OS_AUTH
   /* here we attach a special string to the username
      so that the server knows to do OS authentication */
   if (doOsAuthentication) {
       strncat(userNameAndZone, OS_AUTH_FLAG, NAME_LEN);
   }
#endif
   authRespIn.username = userNameAndZone;
   status = rcAuthResponse(Conn, &authRespIn);

   if (status) {
      printError(Conn, status, "rcAuthResponse");
      return(status);
   }
   Conn->loggedIn = 1;

   return(0);
}

int 
clientLoginWithPassword(rcComm_t *Conn, char* password) 
{   
   int status, len, i;
   authRequestOut_t *authReqOut;
   authResponseInp_t authRespIn;
   char md5Buf[CHALLENGE_LEN+MAX_PASSWORD_LEN+2];
   char digest[RESPONSE_LEN+2];
   char userNameAndZone[NAME_LEN*2];
   MD5_CTX context;

   if (Conn->loggedIn == 1) {
      /* already logged in */
      return (0);
   }
   status = rcAuthRequest(Conn, &authReqOut);
   if (status) {
      printError(Conn, status, "rcAuthRequest");
      return(status);
   }

   memset(md5Buf, 0, sizeof(md5Buf));
   strncpy(md5Buf, authReqOut->challenge, CHALLENGE_LEN);

   len = strlen(password);
   sprintf(md5Buf+CHALLENGE_LEN, "%s", password);
   md5Buf[CHALLENGE_LEN+len]='\0'; /* remove trailing \n */

   MD5Init (&context);
   MD5Update (&context, (unsigned char*)md5Buf, CHALLENGE_LEN+MAX_PASSWORD_LEN);
   MD5Final ((unsigned char*)digest, &context);
   for (i=0;i<RESPONSE_LEN;i++) {
      if (digest[i]=='\0') digest[i]++;  /* make sure 'string' doesn't
					    end early*/
   }

   /* free the array and structure allocated by the rcAuthRequest */
   if (authReqOut != NULL) {
      if (authReqOut->challenge != NULL) {
         free(authReqOut->challenge);
      }
      free(authReqOut);
   }

   authRespIn.response=digest;
   /* the authentication is always for the proxyUser. */
   authRespIn.username = Conn->proxyUser.userName;
   strncpy(userNameAndZone, Conn->proxyUser.userName, NAME_LEN);
   strncat(userNameAndZone, "#", NAME_LEN);
   strncat(userNameAndZone, Conn->proxyUser.rodsZone, NAME_LEN*2);
   authRespIn.username = userNameAndZone;
   status = rcAuthResponse(Conn, &authRespIn);

   if (status) {
      printError(Conn, status, "rcAuthResponse");
      return(status);
   }
   Conn->loggedIn = 1;

   return(0);
}
