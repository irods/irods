/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* clientLogin.c - client login
 *
 * Perform a series of calls to complete a client login; to
 * authenticate.
 */

// =-=-=-=-=-=-=-
// irods includes
#include "rcGlobalExtern.h"
#include "rodsClient.h"
#include "sslSockComm.h"

// =-=-=-=-=-=-=-
#include "irods_auth_object.hpp"
#include "irods_auth_factory.hpp"
#include "irods_auth_plugin.hpp"
#include "irods_auth_manager.hpp"
#include "irods_auth_constants.hpp"
#include "irods_native_auth_object.hpp"
#include "irods_pam_auth_object.hpp"
#include "authPluginRequest.h"
#include "irods_configuration_parser.hpp"
#include "irods_configuration_keywords.hpp"
#include "checksum.hpp"
#include "termiosUtil.hpp"


#include "irods_kvp_string_parser.hpp"
#include "irods_environment_properties.hpp"



#include <openssl/md5.h>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>

#include <errno.h>
#include <termios.h>

static char prevChallengeSignatureClient[200];

char *getSessionSignatureClientside() {
    return prevChallengeSignatureClient;
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
    char *mySubName = NULL;
    const char *myName = rodsErrorName( status, &mySubName );
    fprintf( stderr, "%s failed with error %d %s %s\n", routineName,
             status, myName, mySubName );
    free( mySubName );

    return 0;
}

int clientLoginPam( rcComm_t* Conn,
                    char*     password,
                    int       ttl ) {
    using namespace boost::filesystem;
    int status = 0;
    pamAuthRequestInp_t pamAuthReqInp;
    pamAuthRequestOut_t *pamAuthReqOut = NULL;
    int len = 0;
    char myPassword[MAX_PASSWORD_LEN + 2];
    char userName[NAME_LEN * 2];
    strncpy( userName, Conn->proxyUser.userName, NAME_LEN );
    if ( password[0] != '\0' ) {
        strncpy( myPassword, password, sizeof( myPassword ) );
    }
    else {
        irods::termiosUtil tiosutl(STDIN_FILENO);
        if ( !tiosutl.echoOff() )
        {
            printf( "WARNING: Error %d disabling echo mode. Password will be displayed in plaintext.\n", tiosutl.getError() );
        }

        printf( "Enter your current PAM (system) password:" );

        const char *fgets_return = fgets( myPassword, sizeof( myPassword ), stdin );
        if (fgets_return != myPassword || strlen(myPassword) < 2) {
            // We're here because we either got an error or end-of-file.
            myPassword[0] = '\0';
        }

        printf( "\n" );
        if( tiosutl.getValid() && !tiosutl.echoOn() )
        {
            printf( "Error reinstating echo mode.\n" );
        }
    }
    len = strlen( myPassword );
    if ( len > 0 && myPassword[len - 1] == '\n' ) {
        myPassword[len - 1] = '\0'; /* remove trailing \n */
    }

    /* since PAM requires a plain text password to be sent
       to the server, ask the server to encrypt the current
       communication socket. */
    status = sslStart( Conn );
    if ( status ) {
        printError( Conn, status, "sslStart" );
        return status;
    }

    memset( &pamAuthReqInp, 0, sizeof( pamAuthReqInp ) );
    pamAuthReqInp.pamPassword = myPassword;
    pamAuthReqInp.pamUser = userName;
    pamAuthReqInp.timeToLive = ttl;
    status = rcPamAuthRequest( Conn, &pamAuthReqInp, &pamAuthReqOut );
    if ( status ) {
        printError( Conn, status, "rcPamAuthRequest" );
        sslEnd( Conn );
        return status;
    }
    memset( myPassword, 0, sizeof( myPassword ) );
    rodsLog( LOG_NOTICE, "iRODS password set up for iCommand use: %s\n",
             pamAuthReqOut->irodsPamPassword );

    /* can turn off SSL now. Have to request the server to do so.
       Will also ignore any error returns, as future socket ops
       are probably unaffected. */
    sslEnd( Conn );

    status = obfSavePw( 0, 0, 0,  pamAuthReqOut->irodsPamPassword );
    return status;

}

/*
 Make a short-lived password.
 TTL is Time-to-live, in hours, typically in the few days range.
 */
int clientLoginTTL( rcComm_t *Conn, int ttl ) {

    char userPassword[MAX_PASSWORD_LEN + 10];
    if ( int status = obfGetPw( userPassword ) ) {
        memset( userPassword, 0, sizeof( userPassword ) );
        return status;
    }

    getLimitedPasswordInp_t getLimitedPasswordInp;
    getLimitedPasswordInp.ttl = ttl;
    getLimitedPasswordInp.unused1 = "";

    getLimitedPasswordOut_t *getLimitedPasswordOut;
    if ( int status = rcGetLimitedPassword( Conn,
                &getLimitedPasswordInp,
                &getLimitedPasswordOut ) ) {
        printError( Conn, status, "rcGetLimitedPassword" );
        memset( userPassword, 0, sizeof( userPassword ) );
        return status;
    }

    /* calcuate the limited password, which is a hash of the user's main pw and
       the returned stringToHashWith. */
    char hashBuf[101];
    memset( hashBuf, 0, sizeof( hashBuf ) );
    strncpy( hashBuf, getLimitedPasswordOut->stringToHashWith, 100 );
    strncat( hashBuf, userPassword, 100 );
    memset( userPassword, 0, sizeof( userPassword ) );

    unsigned char digest[100];
    obfMakeOneWayHash(
        HASH_TYPE_DEFAULT,
        ( unsigned char* )hashBuf,
        100,
        digest );
    memset( hashBuf, 0, sizeof( hashBuf ) );

    char limitedPw[100];
    hashToStr( digest, limitedPw );

    return obfSavePw( 0, 0, 0,  limitedPw );
}


/* The following 3 functions are low level ssl initialization code also used by sslSockComm,
 * here to enable raw socket communications for the openid plugin, not using rcComm structures.
 */
static int
sslVerifyCallback( int ok, X509_STORE_CTX *store ) {
    /* log any verification problems, even if we'll still accept the cert */
    if ( !ok ) {
        char data[256];
        auto *cert = X509_STORE_CTX_get_current_cert( store );
        int  depth = X509_STORE_CTX_get_error_depth( store );
        int  err = X509_STORE_CTX_get_error( store );

        rodsLog( LOG_NOTICE, "sslVerifyCallback: problem with certificate at depth: %i", depth );
        X509_NAME_oneline( X509_get_issuer_name( cert ), data, 256 );
        rodsLog( LOG_NOTICE, "sslVerifyCallback:   issuer = %s", data );
        X509_NAME_oneline( X509_get_subject_name( cert ), data, 256 );
        rodsLog( LOG_NOTICE, "sslVerifyCallback:   subject = %s", data );
        rodsLog( LOG_NOTICE, "sslVerifyCallback:   err %i:%s", err,
                 X509_verify_cert_error_string( err ) );
    }
    return ok;
}

static SSL_CTX*
sslInit( char *certfile, char *keyfile ) {
    static int init_done = 0;
    rodsEnv env;
    int status = getRodsEnv( &env );
    if ( status < 0 ) {
        rodsLog(
            LOG_ERROR,
            "sslInit - failed in getRodsEnv : %d",
            status );
        return NULL;
    }
    if ( !init_done ) {
        SSL_library_init();
        SSL_load_error_strings();
        init_done = 1;
    }

    /* in our test programs we set up a null signal
       handler for SIGPIPE */
    /* signal(SIGPIPE, sslSigpipeHandler); */
    SSL_CTX* ctx = SSL_CTX_new( SSLv23_method() );

    SSL_CTX_set_options( ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_SINGLE_DH_USE );

    /* load our keys and certificates if provided */
    if ( certfile ) {
        if ( SSL_CTX_use_certificate_chain_file( ctx, certfile ) != 1 ) {
            rodsLog( LOG_ERROR,"sslInit: couldn't read certificate chain file" );
            SSL_CTX_free( ctx );
            return NULL;
        }
        else {
            if ( SSL_CTX_use_PrivateKey_file( ctx, keyfile, SSL_FILETYPE_PEM ) != 1 ) {
                rodsLog( LOG_ERROR,"sslInit: couldn't read key file" );
                SSL_CTX_free( ctx );
                return NULL;
            }
        }
    }

    /* set up CA paths and files here */
    const char *ca_path = strcmp( env.irodsSSLCACertificatePath, "" ) ? env.irodsSSLCACertificatePath : NULL;
    const char *ca_file = strcmp( env.irodsSSLCACertificateFile, "" ) ? env.irodsSSLCACertificateFile : NULL;
    if ( ca_path || ca_file ) {
        if ( SSL_CTX_load_verify_locations( ctx, ca_file, ca_path ) != 1 ) {
            rodsLog( LOG_ERROR,"sslInit: error loading CA certificate locations" );
        }
    }
    if ( SSL_CTX_set_default_verify_paths( ctx ) != 1 ) {
        rodsLog( LOG_ERROR,"sslInit: error loading default CA certificate locations" );
    }

    /* Set up the default certificate verification */
    /* if "none" is specified, we won't stop the SSL handshake
       due to certificate error, but will log messages from
       the verification callback */
    const char* verify_server = env.irodsSSLVerifyServer;
    if ( verify_server && !strcmp( verify_server, "none" ) ) {
        SSL_CTX_set_verify( ctx, SSL_VERIFY_NONE, sslVerifyCallback );
    }
    else {
        SSL_CTX_set_verify( ctx, SSL_VERIFY_PEER, sslVerifyCallback );
    }
    /* default depth is nine ... leave this here in case it needs modification */
    SSL_CTX_set_verify_depth( ctx, 9 );

    /* ciphers */
    if ( SSL_CTX_set_cipher_list( ctx, SSL_CIPHER_LIST ) != 1 ) {
        rodsLog( LOG_ERROR,"sslInit: couldn't set the cipher list (no valid ciphers)" );
        SSL_CTX_free( ctx );
        return NULL;
    }

    return ctx;
}

static SSL*
sslInitSocket( SSL_CTX *ctx, int sock ) {
    SSL *ssl;
    BIO *bio;
    bio = BIO_new_socket( sock, BIO_NOCLOSE );
    if ( bio == NULL ) {
        rodsLog( LOG_ERROR, "sslInitSocket: BIO allocation error" );
        return NULL;
    }
    ssl = SSL_new( ctx );
    if ( ssl == NULL ) {
        rodsLog( LOG_ERROR,"sslInitSocket: couldn't create a new SSL socket" );
        BIO_free( bio );
        return NULL;
    }
    SSL_set_bio( ssl, bio, bio );
    return ssl;
}

/* writes a message for the openid plugin
 */
int ssl_write_msg( SSL* ssl, const std::string& msg )
{
    int msg_len = msg.size();
    SSL_write( ssl, &msg_len, sizeof( msg_len ) );
    SSL_write( ssl, msg.c_str(), msg_len );
    return msg_len;
}

/* reads a message in the format defined by the openid plugin
 */
int ssl_read_msg( SSL* ssl, std::string& msg_out )
{
    const int READ_LEN = 256;
    char buffer[READ_LEN + 1];
    int n_bytes = 0;
    int total_bytes = 0;
    int data_len = 0;
    SSL_read( ssl, &data_len, sizeof( data_len ) );
    std::string msg;
    // read that many bytes into our buffer
    while ( total_bytes < data_len ) {
        memset( buffer, 0, READ_LEN + 1 );
        int bytes_remaining = data_len - total_bytes;
        if ( bytes_remaining < READ_LEN ) {
            // can read rest of data in one go
            n_bytes = SSL_read( ssl, buffer, bytes_remaining );
        }
        else {
            // read max bytes into buffer
            n_bytes = SSL_read( ssl, buffer, READ_LEN );
        }
        if ( n_bytes == -1 ) {
            // error reading
            break;
        }
        if ( n_bytes == 0 ) {
            // no more data
            break;
        }
        std::cout << "received " + std::to_string( n_bytes ) + " bytes: " + std::string( buffer ) << std::endl;
        msg.append( buffer );
        total_bytes += n_bytes;
    }
    msg_out = msg;
    return total_bytes;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* To enable easier debugging in apache webservers, each error condition is returning a unique arbitrary negative number.
 * These return values override any error code passed back from functions where the error originates, but this may be
 * changed in the future.
 */
int clientLoginOpenID(
        rcComm_t    *_comm,
        const char  *_context,
        int reprompt )
{
    if ( !_comm ) {
        rodsLog( LOG_ERROR, "clientLoginOpenID: null comm" );
        return -1;
    }
    if ( 1 == _comm->loggedIn ) {
        rodsLog( LOG_ERROR, "clientLoginOpenID: already logged in" );
        return 0;
    }
    const char *auth_scheme = "openid";
    // =-=-=-=-=-=-=-
    // construct an auth object given the scheme
    irods::auth_object_ptr auth_obj;
    irods::error ret = irods::auth_factory( auth_scheme, _comm->rError, auth_obj );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        //return ret.code();
        return -2;
    }

    // =-=-=-=-=-=-=-
    // resolve an auth plugin given the auth object
    irods::plugin_ptr ptr;
    ret = auth_obj->resolve( irods::AUTH_INTERFACE, ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        //return ret.code();
        return -3;
    }
    irods::auth_ptr auth_plugin = boost::dynamic_pointer_cast< irods::auth >( ptr );

    // =-=-=-=-=-=-=-
    // call client side init
    ret = auth_plugin->call <rcComm_t*, const char* > ( NULL, irods::AUTH_CLIENT_START, auth_obj, _comm, _context );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        //return ret.code();
        return -4;
    }

    // if reprompt, do a login flow which reprompts
    if ( reprompt ) {
        // =-=-=-=-=-=-=-
        // send an authentication request to the server
        ret = auth_plugin->call <rcComm_t* > ( NULL, irods::AUTH_CLIENT_AUTH_REQUEST, auth_obj, _comm );
        if ( !ret.ok() ) {
            printError(
                _comm,
                ret.code(),
                ( char* )ret.result().c_str() );
            return ret.code();
        }
    }
    else {
        irods::kvp_map_t ctx_map;
        std::string client_provider_cfg;
        try {
            client_provider_cfg = irods::get_environment_property<std::string&>("openid_provider");
        }
        catch ( const irods::error& e ) {
            if ( e.code() == KEY_NOT_FOUND ) {
                rodsLog( LOG_DEBUG, "openid_provider not defined" );
            }
            else {
                irods::log( e );
            }
            return e.code();
        }
        ctx_map["provider"] = client_provider_cfg;
        ctx_map["session_id"] = _context ? _context : "";
        ctx_map["a_user"] = _comm->proxyUser.userName;
        std::string context_string = irods::escaped_kvp_string( ctx_map );

        authPluginReqInp_t req_in;
        memset( &req_in, 0, sizeof( authPluginReqInp_t ) );
        if ( context_string.size() + 1 > MAX_NAME_LEN ) {
            rodsLog( LOG_ERROR, "context string exceeded max allowed length: %ld", (MAX_NAME_LEN - 1) );
            return -6;
        }
        strncpy( req_in.context_, context_string.c_str(), context_string.size() + 1 );
        // copy auth scheme to the req in
        std::string auth_scheme = "openid";
        strncpy( req_in.auth_scheme_, auth_scheme.c_str(), auth_scheme.size() + 1 );

        authPluginReqOut_t *req_out = NULL;
        int status = rcAuthPluginRequest( _comm, &req_in, &req_out );
        if ( status < 0 ) {
            return -7;
        }
        irods::kvp_map_t out_map;
        irods::parse_escaped_kvp_string( req_out->result_, out_map );
        if ( out_map.find( "port" ) == out_map.end()
            || out_map.find( "nonce" ) == out_map.end() ) {
            rodsLog( LOG_ERROR, "map missing values: %s", req_out->result_ );
            return -8;
        }
        int portno = std::stoi( out_map["port"] );
        std::string nonce = out_map["nonce"];
        std::cout 
            << "received comm info from server: port: [" + out_map["port"] + "], nonce: [" + out_map["nonce"] + "]"
            << std::endl;

        int sockfd;
        struct sockaddr_in serv_addr;
        struct hostent* server;
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if ( sockfd < 0 ) {
            perror( "socket" );
            return -9;
        }
        std::string irods_env_host;
        try {
            irods_env_host = irods::get_environment_property<std::string&>("irods_host");
        }
        catch ( const irods::exception& e ) {
            if ( e.code() == KEY_NOT_FOUND ) {
                rodsLog( LOG_DEBUG, "irods_host is not defined" );
            }
            else {
                irods::log( e );
            }
            return e.code();
        }
        server = gethostbyname( irods_env_host.c_str() ); // this only handles hostnames, not IP addresses.
        if ( server == NULL ) {
            fprintf( stderr, "No host found for host: %s\n", irods_env_host.c_str() );
            return -10;
        }
        memset( &serv_addr, 0, sizeof( serv_addr ) );
        serv_addr.sin_family = AF_INET;
        memcpy( server->h_addr, &serv_addr.sin_addr.s_addr, server->h_length );
        serv_addr.sin_port = htons( portno );
        if ( connect( sockfd, (struct sockaddr*)&serv_addr, sizeof( serv_addr ) ) < 0 ) {
            perror( "connect" );
            return -11;
        }
        // turn on ssl
        SSL_CTX *ctx = sslInit( NULL, NULL );
        if ( !ctx ) {
            rodsLog( LOG_ERROR, "could not initialize SSL context on client" );
            close( sockfd );
            return -12;
        }
        SSL* ssl = sslInitSocket( ctx, sockfd );
        if ( !ssl ) {
            rodsLog( LOG_ERROR, "could not initialize SSL socket on client" );
            ERR_print_errors_fp( stdout );
            SSL_CTX_free( ctx );
            close( sockfd );
            return -13;
        }
        status = SSL_connect( ssl );
        if ( status != 1 ) {
            rodsLog( LOG_ERROR, "ssl connect error" );
            SSL_free( ssl );
            SSL_CTX_free( ctx );
            close( sockfd );
            return -14;
        }
        // peer validation

        // write nonce to server to verify that we are the same client that the auth req came from
        ssl_write_msg( ssl, nonce );

        // read first 4 bytes (data length)
        std::string authorization_url_buf;
        if ( ssl_read_msg( ssl, authorization_url_buf ) < 0 ) {
            perror( "error reading url from socket" );
            return -15;
        }   

        SSL_free( ssl );
        SSL_CTX_free( ctx );
        close( sockfd );
        // finished reading authorization url
        // if the auth url is "SUCCESS", session is already authorized, no user action needed
        // debug issue with using empty message as url
        if ( authorization_url_buf.compare( "SUCCESS" ) == 0 ) {
            std::cout << "Session is valid" << std::endl;
        }
        else {
            // for non reprompt, we just ignore the auth url
            std::cout << "session information was invalid, received: ["
                << authorization_url_buf << "]" << std::endl;
            return -16;
        }
    }
    // =-=-=-=-=-=-=-
    // send the auth response to the agent
    ret = auth_plugin->call <rcComm_t* > ( NULL, irods::AUTH_CLIENT_AUTH_RESPONSE, auth_obj, _comm );
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

    if ( 1 == _comm->loggedIn ) {
        return 0;
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
            char* auth_env_var = getenv( irods::to_env( irods::CFG_IRODS_AUTHENTICATION_SCHEME_KW ).c_str() );
            if ( !auth_env_var ) {
                rodsEnv rods_env;
                if ( getRodsEnv( &rods_env ) >= 0 ) {
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
            std::transform( auth_scheme.begin(), auth_scheme.end(), auth_scheme.begin(), ::tolower );

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
    irods::error ret = irods::auth_factory( auth_scheme, _comm->rError, auth_obj );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // resolve an auth plugin given the auth object
    irods::plugin_ptr ptr;
    ret = auth_obj->resolve( irods::AUTH_INTERFACE, ptr );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }
    irods::auth_ptr auth_plugin = boost::dynamic_pointer_cast< irods::auth >( ptr );

    // =-=-=-=-=-=-=-
    // call client side init
    ret = auth_plugin->call <rcComm_t*, const char* > ( NULL, irods::AUTH_CLIENT_START, auth_obj, _comm, _context );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // send an authentication request to the server
    ret = auth_plugin->call <rcComm_t* > ( NULL, irods::AUTH_CLIENT_AUTH_REQUEST, auth_obj, _comm );
    if ( !ret.ok() ) {
        printError(
            _comm,
            ret.code(),
            ( char* )ret.result().c_str() );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // establish auth context client side
    ret = auth_plugin->call( NULL, irods::AUTH_ESTABLISH_CONTEXT, auth_obj );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // =-=-=-=-=-=-=-
    // send the auth response to the agent
    ret = auth_plugin->call <rcComm_t* > ( NULL, irods::AUTH_CLIENT_AUTH_RESPONSE, auth_obj, _comm );
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
    int status, len, i = 0;
    authRequestOut_t *authReqOut = NULL;
    authResponseInp_t authRespIn;
    char md5Buf[CHALLENGE_LEN + MAX_PASSWORD_LEN + 2];
    char digest[RESPONSE_LEN + 2];
    char userNameAndZone[NAME_LEN * 2 + 1];
    MD5_CTX context;
    if ( !password ) {
        printError( Conn, -1, "null password pointer" );
        return -1;
    }

    if ( Conn->loggedIn == 1 ) {
        /* already logged in */
        return 0;
    }
    status = rcAuthRequest( Conn, &authReqOut );
    if ( status || NULL == authReqOut ) { // JMC cppcheck - nullptr
        printError( Conn, status, "rcAuthRequest" );
        return status;
    }

    memset( md5Buf, 0, sizeof( md5Buf ) );
    strncpy( md5Buf, authReqOut->challenge, CHALLENGE_LEN );
    setSessionSignatureClientside( md5Buf );


    len = strlen( password );
    sprintf( md5Buf + CHALLENGE_LEN, "%s", password );
    md5Buf[CHALLENGE_LEN + len] = '\0'; /* remove trailing \n */

    MD5_Init( &context );
    MD5_Update( &context, ( unsigned char* )md5Buf, CHALLENGE_LEN + MAX_PASSWORD_LEN );
    MD5_Final( ( unsigned char* )digest, &context );
    for ( i = 0; i < RESPONSE_LEN; i++ ) {
        if ( digest[i] == '\0' ) {
            digest[i]++;
        }  /* make sure 'string' doesn't end early*/
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
        return status;
    }
    Conn->loggedIn = 1;

    return 0;
}
