/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* clientLogin.c - client login
 *
 * Perform a series of calls to complete a client login; to
 * authenticate.
 */

// =-=-=-=-=-=-=-
// irods includs
#include "rodsClient.h"
#include "sslSockComm.h"

// =-=-=-=-=-=-=-
// eirods includs
#include "eirods_auth_object.h"
#include "eirods_auth_factory.h"
#include "eirods_auth_plugin.h"
#include "eirods_auth_manager.h"
#include "eirods_auth_constants.h"
#include "eirods_kvp_string_parser.h"

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
    if (status || NULL == myName ) { // JMC cppcheck - nullptr
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

int clientLoginPam( rcComm_t* Conn, 
        char*     password, 
        int       ttl ) {
#ifdef PAM_AUTH
    int status;
    pamAuthRequestInp_t pamAuthReqInp;
    pamAuthRequestOut_t *pamAuthReqOut;
    int doStty=0;
    int len;
    char myPassword[MAX_PASSWORD_LEN+2];
    char userName[NAME_LEN*2];
    strncpy(userName, Conn->proxyUser.userName, NAME_LEN);
    if (password[0]!='\0') {
        strncpy(myPassword, password, sizeof(myPassword));
    }
    else {
        path p ("/bin/stty");
        if (exists(p)) {
            system("/bin/stty -echo 2> /dev/null");
            doStty=1;
        }
        printf("Enter your current PAM (system) password:");
        fgets(myPassword, sizeof(myPassword), stdin);
        if (doStty) {
            system("/bin/stty echo 2> /dev/null");
            printf("\n");
        }
    }
    len = strlen(myPassword);
    if (myPassword[len-1]=='\n') {
        myPassword[len-1]='\0'; /* remove trailing \n */
    }

#ifdef USE_SSL
    /* since PAM requires a plain text password to be sent
       to the server, ask the server to encrypt the current 
       communication socket. */
    status = sslStart(Conn);
    if (status) {
        printError(Conn, status, "sslStart");
        return(status);
    }
#else
    rodsLog(LOG_ERROR, "iRODS doesn't include SSL support, required for PAM authentication.");
    return SSL_NOT_BUILT_INTO_CLIENT;
#endif /* USE_SSL */

    memset (&pamAuthReqInp, 0, sizeof (pamAuthReqInp));
    pamAuthReqInp.pamPassword = myPassword;
    pamAuthReqInp.pamUser = userName;
    pamAuthReqInp.timeToLive = ttl;
    status = rcPamAuthRequest(Conn, &pamAuthReqInp, &pamAuthReqOut);
    if (status) {
        printError(Conn, status, "rcPamAuthRequest");
#ifdef USE_SSL
        sslEnd(Conn);
#endif
        return(status);
    }
    memset(myPassword, 0, sizeof(myPassword));
    rodsLog(LOG_NOTICE, "iRODS password set up for i-command use: %s\n", 
            pamAuthReqOut->irodsPamPassword);

#ifdef USE_SSL
    /* can turn off SSL now. Have to request the server to do so. 
       Will also ignore any error returns, as future socket ops
       are probably unaffected. */
    sslEnd(Conn);
#endif /* USE_SSL */

    status = obfSavePw(0, 0, 0,  pamAuthReqOut->irodsPamPassword);
    return(status);
#else
    return(PAM_AUTH_NOT_BUILT_INTO_CLIENT);
#endif

    }

/// =-=-=-=-=-=-=-
/// @brief clientLogin provides the interface for authenticaion
///        plugins as well as defining the protocol or template
///        authenticaion will follow
int clientLogin(
    rcComm_t* _conn ) {
printf( "XXXX - clientLogin :: START" );
fflush( stdout );
    // =-=-=-=-=-=-=-
    // check out conn pointer
    if( !_conn ) {
        return SYS_INVALID_INPUT_PARAM;
    }

    // =-=-=-=-=-=-=-
    // get the rods environment so we can determine the
    // flavor of authenticaiton desired by the user -
    // check the environment vairable first then the rods
    // env if that was null
    std::string auth_scheme=eirods::AUTH_NATIVE_SCHEME;
    if( ProcessType == CLIENT_PT ) {
        // =-=-=-=-=-=-=-
        // check the environment variable first
        char* auth_env_var = getenv("irodsAuthScheme");
        if( !auth_env_var ) {
            rodsEnv rods_env;
            int status = getRodsEnv( &rods_env );
            if( !status ) {
                printf( "failed to acquire rods environment" );
                return SYS_INVALID_INPUT_PARAM;
            }

            if( strlen( rods_env.rodsAuthScheme ) > 0 ) {
                auth_scheme = rods_env.rodsAuthScheme;
           
            }
             
        } else {
            auth_scheme = auth_env_var;
        
        }
         
    } // if client side auth

printf( "XXXX - clientLogin :: auth scheme [%s]\n", auth_scheme.c_str() );
fflush( stdout );

    // =-=-=-=-=-=-=-
    // construct an auth object given the scheme
    eirods::auth_object_ptr auth_obj;
    eirods::error ret = eirods::auth_factory( 
                            auth_scheme, 
                            _conn->rError,
                            auth_obj );
    if( !ret.ok() ){
        eirods::log( PASS( ret ) );
        return ret.code();
    }

printf( "XXXX - clientLogin :: created auth object\n" );
fflush( stdout );
    // =-=-=-=-=-=-=-
    // resolve an auth plugin given the auth object
    eirods::plugin_ptr ptr;
    ret = auth_obj->resolve( 
              eirods::AUTH_INTERFACE,
              ptr );
    if( !ret.ok() ){
        eirods::log( PASS( ret ) );
        return ret.code();
    }
    eirods::auth_ptr auth_plugin = boost::dynamic_pointer_cast< eirods::auth >( ptr );

printf( "XXXX - clientLogin :: resolved auth plugin\n" );
fflush( stdout );

    // =-=-=-=-=-=-=-
    // call client side init - 'establish creds'
    ret = auth_plugin->call( 
              eirods::AUTH_CLIENT_START, 
              auth_obj );
    if( !ret.ok() ){
        eirods::log( PASS( ret ) );
        return ret.code();
    }
    
    // =-=-=-=-=-=-=-
    // send an authentication request to the server
    // we overload the challenge variable in order
    // to not change the interface which would break
    // federation.  we will check the scheme if the
    // challenge is not null on the server side to 
    // load the correct plugin
    authRequestOut_t* auth_request = new authRequestOut_t;
    auth_request->challenge = new char[ CHALLENGE_LEN+2 ];
    strncpy( 
        auth_request->challenge, 
        auth_scheme.c_str(), 
        CHALLENGE_LEN );
printf( "XXXX - clientLogin :: auth challenge ( scheme ) [%s]\n", auth_request->challenge );
fflush( stdout );
    int status = rcAuthRequest(
                 _conn, 
                 &auth_request );
    if( status ) {
        printError( 
            _conn, 
            status, 
            "rcAuthRequest" );
        delete [] auth_request->challenge;
        delete [] auth_request;
        return status;
    }


    // =-=-=-=-=-=-=-
    // establish auth context client side
printf( "XXXX - clientLogin :: calling est ctx" );
fflush( stdout );
    authResponseInp_t auth_response;
    auth_response.response = new char[ RESPONSE_LEN+2 ];
    auth_response.username = new char[ LONG_NAME_LEN ];
    ret = auth_plugin->call< 
              const char*,
              const char*,
              authRequestOut_t*,
              authResponseInp_t*,
              char* >( 
                  eirods::AUTH_ESTABLISH_CONTEXT, 
                  auth_obj,
                  _conn->proxyUser.userName,
                  _conn->proxyUser.rodsZone,
                  auth_request,
                  &auth_response,
                  prevChallengeSignitureClient );
    if( !ret.ok() ){
        eirods::log( PASS( ret ) );
        delete [] auth_response.response;
        delete [] auth_response.username; 
        delete [] auth_request->challenge;
        delete auth_request;
        return ret.code();
    }
printf( "XXXX - clientLogin :: calling auth response - username [%s]", auth_response.username );
fflush( stdout );

    // =-=-=-=-=-=-=-
    // send the auth response to the agent
    status = rcAuthResponse( 
                 _conn, 
                 &auth_response );
    if( status ) {
        printError( 
            _conn, 
            status, 
            "rcAuthResponse" );
        delete [] auth_response.response;
        delete [] auth_response.username; 
        delete [] auth_request->challenge;
        delete auth_request;
        return status;
    }

    // =-=-=-=-=-=-=-
    // set the flag stating we are logged in
    _conn->loggedIn = 1;

    // =-=-=-=-=-=-=-
    // win!
    delete [] auth_response.response;
    delete [] auth_response.username; 
    delete [] auth_request->challenge;
    delete auth_request;
    return 0;

} // clientLogin

static 
int OLD_clientLogin(rcComm_t *Conn) 
{   
    int status, len, i;
    authRequestOut_t *authReqOut;
    authResponseInp_t authRespIn;
    char md5Buf[CHALLENGE_LEN+MAX_PASSWORD_LEN+2];
    char digest[RESPONSE_LEN+2];
    char userNameAndZone[NAME_LEN*2];
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

#ifdef PAM_AUTH
    /* Even in PAM mode, we do the regular login here using the
       generated iRODS password that has been set up */
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
    if (status || NULL == authReqOut ) { // JMC cppcheck - nullptr
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

        path p ("/bin/stty");
        if (exists(p)) {
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
    if (authReqOut->challenge != NULL) {
        free(authReqOut->challenge);
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
   if (status || NULL == authReqOut ) { // JMC cppcheck - nullptr
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
   //if (authReqOut != NULL) { // JMC cppcheck - redundant nullptr check
      if (authReqOut->challenge != NULL) {
         free(authReqOut->challenge);
      }
      free(authReqOut);
   //}

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
