/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* clientLogin.c - client login
 *
 * Perform a series of calls to complete a client login; to
 * authenticate.
 */

// =-=-=-=-=-=-=-
// irods includes
#include "rodsClient.hpp"
#include "sslSockComm.hpp"

// =-=-=-=-=-=-=-
#include "irods_auth_object.hpp"
#include "irods_auth_factory.hpp"
#include "irods_auth_plugin.hpp"
#include "irods_auth_manager.hpp"
#include "irods_auth_constants.hpp"
#include "irods_native_auth_object.hpp"
#include "irods_pam_auth_object.hpp"
#include "authPluginRequest.hpp"

static char prevChallengeSignatureClient[200];

char *getSessionSignatureClientside() {
    return( prevChallengeSignatureClient );
}

void setSessionSignatureClientside( char* _sig ) {
    snprintf(
        prevChallengeSignatureClient,
        200,
        "%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x",
        ( unsigned char )_sig[0],
        ( unsigned char )_sig[1],
        ( unsigned char )_sig[2],
        ( unsigned char )_sig[3],
        ( unsigned char )_sig[4],
        ( unsigned char )_sig[5],
        ( unsigned char )_sig[6],
        ( unsigned char )_sig[7],
        ( unsigned char )_sig[8],
        ( unsigned char )_sig[9],
        ( unsigned char )_sig[10],
        ( unsigned char )_sig[11],
        ( unsigned char )_sig[12],
        ( unsigned char )_sig[13],
        ( unsigned char )_sig[14],
        ( unsigned char )_sig[15] );

} // setSessionSignatureClientside



int printError( rcComm_t *Conn, int status, char *routineName ) {
    char *mySubName;
    char *myName;
    rError_t *Err;
    rErrMsg_t *ErrMsg;
    int i, len;
    if ( Conn ) {
        if ( Conn->rError ) {
            Err = Conn->rError;
            len = Err->len;
            for ( i = 0; i < len; i++ ) {
                ErrMsg = Err->errMsg[i];
                fprintf( stderr, "Level %d: %s\n", i, ErrMsg->msg );
            }
        }
    }
    myName = rodsErrorName( status, &mySubName );
    fprintf( stderr, "%s failed with error %d %s %s\n", routineName,
             status, myName, mySubName );

    return ( 0 );
}

#ifdef GSI_AUTH
int clientLoginGsi( rcComm_t *Conn ) {
    int status;
    gsiAuthRequestOut_t *gsiAuthReqOut;
    char *myName;
    char *serverDN;

    status = igsiSetupCreds( Conn, NULL, NULL, &myName );

    if ( status ) {
        printError( Conn, status, "igsiSetupCreds" );
        return( status );
    }

    /*   printf("Client-side DN is:%s\n",myName); */

    status = rcGsiAuthRequest( Conn, &gsiAuthReqOut );
    if ( status ) {
        printError( Conn, status, "rcGsiAuthRequest" );
        return( status );
    }

    /*   printf("Server-side DN is:%s\n", gsiAuthReqOut->serverDN); */

    serverDN = getenv( "irodsServerDn" ); /* Use irodsServerDn if defined */
    if ( serverDN == NULL ) {
        serverDN = getenv( "SERVER_DN" ); /* NULL or the SERVER_DN string */
    }

    status = igsiEstablishContextClientside( Conn, serverDN, 0 );
    if ( status ) {
        printError( Conn, status, "igsiEstablishContextClientside" );
        return( status );
    }

    /* Now, check if it actually succeeded */
    status = rcGsiAuthRequest( Conn, &gsiAuthReqOut );
    if ( status ) {
        printf( "Error from iRODS Server:\n" );
        printError( Conn, status, "GSI Authentication" );
        return( status );
    }

    Conn->loggedIn = 1;

    return( 0 );
}
#endif

#ifdef KRB_AUTH
int clientLoginKrb( rcComm_t *Conn ) {
    int status;
    krbAuthRequestOut_t *krbAuthReqOut;
    char *myName = 0;
    char *serverName;
    static int KRB_debug = 0;

    status = ikrbSetupCreds( Conn, NULL, NULL, &myName );
    if ( status || NULL == myName ) { // JMC cppcheck - nullptr
        printError( Conn, status, "ikrbSetupCreds" );
        return( status );
    }

    if ( KRB_debug ) {
        printf( "Client-side Name:%s\n", myName );
    }
    if ( myName != 0 ) {
        free( myName );
    }

    status = rcKrbAuthRequest( Conn, &krbAuthReqOut );
    if ( status ) {
        printError( Conn, status, "rcKrbAuthRequest" );
        return( status );
    }

    if ( KRB_debug ) {
        printf( "Server-side Name provided:%s\n", krbAuthReqOut->serverName );
    }

    serverName = getenv( "irodsServerDn" ); /* Use irodsServerDn if defined */
    if ( serverName == NULL ) {
        serverName = getenv( "SERVER_DN" ); /* NULL or the SERVER_DN string */
    }
    if ( serverName == NULL ) {
        /* if not provided, use the server's claimed Name */
        serverName =  krbAuthReqOut->serverName;
    }

    if ( KRB_debug ) {
        printf( "Server Name to connect to:%s\n", serverName );
    }

    status = ikrbEstablishContextClientside( Conn, serverName, 0 );
    if ( status ) {
        printError( Conn, status, "ikrbEstablishContextClientside" );
        return( status );
    }

    /* Now, check if it actually succeeded */
    status = rcKrbAuthRequest( Conn, &krbAuthReqOut );
    if ( status ) {
        printf( "Error from iRODS Server:\n" );
        printError( Conn, status, "KRB Authentication" );
        return( status );
    }

    Conn->loggedIn = 1;

    return( 0 );
}
#endif

int clientLoginPam( rcComm_t* Conn,
                    char*     password,
                    int       ttl ) {
#ifdef PAM_AUTH
    int status;
    pamAuthRequestInp_t pamAuthReqInp;
    pamAuthRequestOut_t *pamAuthReqOut;
    int doStty = 0;
    int len;
    char myPassword[MAX_PASSWORD_LEN + 2];
    char userName[NAME_LEN * 2];
    strncpy( userName, Conn->proxyUser.userName, NAME_LEN );
    if ( password[0] != '\0' ) {
        strncpy( myPassword, password, sizeof( myPassword ) );
    }
    else {
        path p( "/bin/stty" );
        if ( exists( p ) ) {
            system( "/bin/stty -echo 2> /dev/null" );
            doStty = 1;
        }
        printf( "Enter your current PAM (system) password:" );
        fgets( myPassword, sizeof( myPassword ), stdin );
        if ( doStty ) {
            system( "/bin/stty echo 2> /dev/null" );
            printf( "\n" );
        }
    }
    len = strlen( myPassword );
    if ( myPassword[len - 1] == '\n' ) {
        myPassword[len - 1] = '\0'; /* remove trailing \n */
    }

#ifdef USE_SSL
    /* since PAM requires a plain text password to be sent
       to the server, ask the server to encrypt the current
       communication socket. */
    status = sslStart( Conn );
    if ( status ) {
        printError( Conn, status, "sslStart" );
        return( status );
    }
#else
    rodsLog( LOG_ERROR, "iRODS doesn't include SSL support, required for PAM authentication." );
    return SSL_NOT_BUILT_INTO_CLIENT;
#endif /* USE_SSL */

    memset( &pamAuthReqInp, 0, sizeof( pamAuthReqInp ) );
    pamAuthReqInp.pamPassword = myPassword;
    pamAuthReqInp.pamUser = userName;
    pamAuthReqInp.timeToLive = ttl;
    status = rcPamAuthRequest( Conn, &pamAuthReqInp, &pamAuthReqOut );
    if ( status ) {
        printError( Conn, status, "rcPamAuthRequest" );
#ifdef USE_SSL
        sslEnd( Conn );
#endif
        return( status );
    }
    memset( myPassword, 0, sizeof( myPassword ) );
    rodsLog( LOG_NOTICE, "iRODS password set up for i-command use: %s\n",
             pamAuthReqOut->irodsPamPassword );

#ifdef USE_SSL
    /* can turn off SSL now. Have to request the server to do so.
       Will also ignore any error returns, as future socket ops
       are probably unaffected. */
    sslEnd( Conn );
#endif /* USE_SSL */

    status = obfSavePw( 0, 0, 0,  pamAuthReqOut->irodsPamPassword );
    return( status );
#else
    return( PAM_AUTH_NOT_BUILT_INTO_CLIENT );
#endif

}

/*
 Make a short-lived password.
 TTL is Time-to-live, in hours, typically in the few days range.
 */
int clientLoginTTL( rcComm_t *Conn, int ttl ) {
    getLimitedPasswordInp_t getLimitedPasswordInp;
    getLimitedPasswordOut_t *getLimitedPasswordOut;
    char hashBuf[100];
    unsigned char digest[100];
    char limitedPw[100];
    int status;

    char userPassword[MAX_PASSWORD_LEN + 10] = "";

    status = obfGetPw( userPassword );
    if ( status ) {
        memset( userPassword, 0, sizeof( userPassword ) );
        return( status );
    }

    status = obfSavePw( 0, 0, 0,  "   " ); /* clear out the permanent password */

    getLimitedPasswordInp.ttl = ttl;
    getLimitedPasswordInp.unused1 = "";

    status = rcGetLimitedPassword( Conn,
                                   &getLimitedPasswordInp,
                                   &getLimitedPasswordOut );
    if ( status ) {
        printError( Conn, status, "rcGetLimitedPassword" );
        memset( userPassword, 0, sizeof( userPassword ) );
        return( status );
    }

    /* calcuate the limited password, which is a hash of the user's main pw and
       the returned stringToHashWith. */
    memset( hashBuf, 0, sizeof( hashBuf ) );
    strncpy( hashBuf, getLimitedPasswordOut->stringToHashWith, 100 );
    strncat( hashBuf, userPassword, 100 );

    obfMakeOneWayHash(
        HASH_TYPE_DEFAULT,
        ( unsigned char* )hashBuf,
        100,
        digest );

    hashToStr( digest, limitedPw );

    status = obfSavePw( 0, 0, 0,  limitedPw );

    memset( hashBuf, 0, sizeof( hashBuf ) );
    memset( userPassword, 0, sizeof( userPassword ) );

    return( 0 );
}

/// =-=-=-=-=-=-=-
/// @brief clientLogin provides the interface for authentication
///        plugins as well as defining the protocol or template
///        Authentication will follow
int clientLogin(
    rcComm_t*   _comm,
    const char* _context,
    const char* _scheme_override ) {
    // =-=-=-=-=-=-=-
    // check out conn pointer
    if ( !_comm ) {
        return SYS_INVALID_INPUT_PARAM;
    }

    // =-=-=-=-=-=-=-
    // get the rods environment so we can determine the
    // flavor of authentication desired by the user -
    // check the environment variable first then the rods
    // env if that was null
    std::string auth_scheme = irods::AUTH_NATIVE_SCHEME;
    if ( ProcessType == CLIENT_PT ) {
        // =-=-=-=-=-=-=-
        // the caller may want to override the env var
        // or irods env file configuration ( PAM )
        if ( _scheme_override && strlen( _scheme_override ) > 0 ) {
            auth_scheme = _scheme_override;

        }
        else {
            // =-=-=-=-=-=-=-
            // check the environment variable first
            char* auth_env_var = getenv( "irodsAuthScheme" );
            if ( !auth_env_var ) {
                rodsEnv rods_env;
                if ( getRodsEnv( &rods_env ) ) {
                    if ( strlen( rods_env.rodsAuthScheme ) > 0 ) {
                        auth_scheme = rods_env.rodsAuthScheme;

                    }
                }

            }
            else {
                auth_scheme = auth_env_var;

            }

            // =-=-=-=-=-=-=-
            // ensure scheme is lower case for comparison
            std::string lower_scheme = auth_scheme;
            std::transform(
                auth_scheme.begin(),
                auth_scheme.end(),
                auth_scheme.begin(),
                ::tolower );

            // =-=-=-=-=-=-=-
            // filter out the pam auth as it is an extra special
            // case and only sent in as an override.
            // everyone other scheme behaves as normal
            if ( irods::AUTH_PAM_SCHEME == auth_scheme ) {
                auth_scheme = irods::AUTH_NATIVE_SCHEME;
            }

        } // if _scheme_override

    } // if client side auth


    // =-=-=-=-=-=-=-
    // construct an auth object given the scheme
    irods::auth_object_ptr auth_obj;
    irods::error ret = irods::auth_factory(
                           auth_scheme,
                           _comm->rError,
                           auth_obj );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve an auth plugin given the auth object
    irods::plugin_ptr ptr;
    ret = auth_obj->resolve(
              irods::AUTH_INTERFACE,
              ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }
    irods::auth_ptr auth_plugin = boost::dynamic_pointer_cast< irods::auth >( ptr );

    // =-=-=-=-=-=-=-
    // call client side init
    ret = auth_plugin->call <
          rcComm_t*,
          const char* > (
              irods::AUTH_CLIENT_START,
              auth_obj,
              _comm,
              _context );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // send an authentication request to the server
    ret = auth_plugin->call <
          rcComm_t* > (
              irods::AUTH_CLIENT_AUTH_REQUEST,
              auth_obj,
              _comm );
    if ( !ret.ok() ) {
        printError(
            _comm,
            ret.code(),
            ( char* )ret.result().c_str() );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // establish auth context client side
    ret = auth_plugin->call(
              irods::AUTH_ESTABLISH_CONTEXT,
              auth_obj );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // send the auth response to the agent
    ret = auth_plugin->call <
          rcComm_t* > (
              irods::AUTH_CLIENT_AUTH_RESPONSE,
              auth_obj,
              _comm );
    if ( !ret.ok() ) {
        printError(
            _comm,
            ret.code(),
            ( char* )ret.result().c_str() );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // set the flag stating we are logged in
    _comm->loggedIn = 1;

    // =-=-=-=-=-=-=-
    // win!
    return 0;

} // clientLogin

int
clientLoginWithPassword( rcComm_t *Conn, char* password ) {
    int status, len, i;
    authRequestOut_t *authReqOut;
    authResponseInp_t authRespIn;
    char md5Buf[CHALLENGE_LEN + MAX_PASSWORD_LEN + 2];
    char digest[RESPONSE_LEN + 2];
    char userNameAndZone[NAME_LEN * 2];
    MD5_CTX context;

    if ( Conn->loggedIn == 1 ) {
        /* already logged in */
        return ( 0 );
    }
    status = rcAuthRequest( Conn, &authReqOut );
    if ( status || NULL == authReqOut ) { // JMC cppcheck - nullptr
        printError( Conn, status, "rcAuthRequest" );
        return( status );
    }

    memset( md5Buf, 0, sizeof( md5Buf ) );
    strncpy( md5Buf, authReqOut->challenge, CHALLENGE_LEN );

    len = strlen( password );
    sprintf( md5Buf + CHALLENGE_LEN, "%s", password );
    md5Buf[CHALLENGE_LEN + len] = '\0'; /* remove trailing \n */

    MD5Init( &context );
    MD5Update( &context, ( unsigned char* )md5Buf, CHALLENGE_LEN + MAX_PASSWORD_LEN );
    MD5Final( ( unsigned char* )digest, &context );
    for ( i = 0; i < RESPONSE_LEN; i++ ) {
        if ( digest[i] == '\0' ) {
            digest[i]++;
        }  /* make sure 'string' doesn't
					    end early*/
    }

    /* free the array and structure allocated by the rcAuthRequest */
    //if (authReqOut != NULL) { // JMC cppcheck - redundant nullptr check
    if ( authReqOut->challenge != NULL ) {
        free( authReqOut->challenge );
    }
    free( authReqOut );
    //}

    authRespIn.response = digest;
    /* the authentication is always for the proxyUser. */
    authRespIn.username = Conn->proxyUser.userName;
    strncpy( userNameAndZone, Conn->proxyUser.userName, NAME_LEN );
    strncat( userNameAndZone, "#", NAME_LEN );
    strncat( userNameAndZone, Conn->proxyUser.rodsZone, NAME_LEN * 2 );
    authRespIn.username = userNameAndZone;
    status = rcAuthResponse( Conn, &authRespIn );

    if ( status ) {
        printError( Conn, status, "rcAuthResponse" );
        return( status );
    }
    Conn->loggedIn = 1;

    return( 0 );
}
