#include "irods/sslSockComm.h"

#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_client_server_negotiation.hpp"
#include "irods/packStruct.h"
#include "irods/rcGlobalExtern.h"
#include "irods/rcMisc.h"
#include "irods/rodsClient.h"

#include "irods/irods_logger.hpp"

#include <cstdio>

#include <openssl/decoder.h>
#include <openssl/evp.h>

static SSL_CTX* sslInit(const char* certfile, const char* keyfile);
static SSL* sslInitSocket(SSL_CTX* ctx, int sock);
static void sslLogError(char* msg);
static int sslLoadDHParams(SSL_CTX* ctx, const char* file);
static int sslVerifyCallback(int ok, X509_STORE_CTX* store);
static int sslPostConnectionCheck(SSL* ssl, char* peer);

namespace
{
    using log_network = irods::experimental::log::network;
} // anonymous namespace

int
sslStart( rcComm_t *rcComm ) {
    int status;
    sslStartInp_t sslStartInp;

    if ( rcComm == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    if ( rcComm->ssl_on ) {
        /* SSL connection turned on ... return */
        return 0;
    }

    /* ask the server if we can start SSL */
    memset( &sslStartInp, 0, sizeof( sslStartInp ) );
    status = rcSslStart( rcComm, &sslStartInp );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "sslStart: server refused our request to start SSL" );
        return status;
    }

    /* we have the go-ahead ... set up SSL on our side of the socket */
    rcComm->ssl_ctx = sslInit( NULL, NULL );
    if ( rcComm->ssl_ctx == NULL ) {
        rodsLog( LOG_ERROR, "sslStart: couldn't initialize SSL context" );
        return SSL_INIT_ERROR;
    }

    rcComm->ssl = sslInitSocket( rcComm->ssl_ctx, rcComm->sock );
    if ( rcComm->ssl == NULL ) {
        rodsLog( LOG_ERROR, "sslStart: couldn't initialize SSL socket" );
        SSL_CTX_free( rcComm->ssl_ctx );
        rcComm->ssl_ctx = NULL;
        return SSL_INIT_ERROR;
    }

    status = SSL_set_tlsext_host_name( rcComm->ssl, rcComm->host );
    if ( status != 1 ) {
        sslLogError( "sslStart: error in SSL_set_tlsext_host_name" );
        SSL_free( rcComm->ssl );
        rcComm->ssl = NULL;
        SSL_CTX_free( rcComm->ssl_ctx );
        rcComm->ssl_ctx = NULL;
        return SSL_INIT_ERROR;
    }

    status = SSL_connect( rcComm->ssl );
    if ( status < 1 ) {
        sslLogError( "sslStart: error in SSL_connect" );
        SSL_free( rcComm->ssl );
        rcComm->ssl = NULL;
        SSL_CTX_free( rcComm->ssl_ctx );
        rcComm->ssl_ctx = NULL;
        return SSL_HANDSHAKE_ERROR;
    }

    rcComm->ssl_on = 1;

    if ( !sslPostConnectionCheck( rcComm->ssl, rcComm->host ) ) {
        rodsLog( LOG_ERROR, "sslStart: post connection certificate check failed" );
        sslEnd( rcComm );
        return SSL_CERT_ERROR;
    }

    snprintf( rcComm->negotiation_results, sizeof( rcComm->negotiation_results ),
              "%s", irods::CS_NEG_USE_SSL.c_str() );
    return 0;
}

int
sslEnd( rcComm_t *rcComm ) {
    int status;
    sslEndInp_t sslEndInp;

    if ( rcComm == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    if ( !rcComm->ssl_on ) {
        /* no SSL connection turned on ... return */
        return 0;
    }

    memset( &sslEndInp, 0, sizeof( sslEndInp ) );
    status = rcSslEnd( rcComm, &sslEndInp );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "sslEnd: server refused our request to stop SSL" );
        return status;
    }

    /* shut down the SSL connection. First SSL_shutdown() sends "close notify" */
    status = SSL_shutdown( rcComm->ssl );
    if ( status == 0 ) {
        /* do second phase of shutdown */
        status = SSL_shutdown( rcComm->ssl );
    }
    if ( status != 1 ) {
        sslLogError( "sslEnd: error shutting down the SSL connection" );
        return SSL_SHUTDOWN_ERROR;
    }

    /* clean up the SSL state */
    SSL_free( rcComm->ssl );
    rcComm->ssl = NULL;
    SSL_CTX_free( rcComm->ssl_ctx );
    rcComm->ssl_ctx = NULL;
    rcComm->ssl_on = 0;

    snprintf( rcComm->negotiation_results, sizeof( rcComm->negotiation_results ),
              "%s", irods::CS_NEG_USE_TCP.c_str() );
    rodsLog( LOG_DEBUG, "sslShutdown: shut down SSL connection" );


    return 0;
}

int sslAccept(rsComm_t* rsComm)
{
    try {
        const auto tls_config = irods::get_server_property<nlohmann::json>(irods::KW_CFG_TLS_SERVER);

        const auto& certificate_chain_file =
            tls_config.at(irods::KW_CFG_TLS_CERTIFICATE_CHAIN_FILE).get_ref<const std::string&>();
        const auto& certificate_key_file =
            tls_config.at(irods::KW_CFG_TLS_CERTIFICATE_KEY_FILE).get_ref<const std::string&>();
        rsComm->ssl_ctx = sslInit(certificate_chain_file.c_str(), certificate_key_file.c_str());
        if (!rsComm->ssl_ctx) {
            log_network::error("couldn't initialize SSL context");
            return SSL_INIT_ERROR;
        }

        const auto& dh_params_file = tls_config.at(irods::KW_CFG_TLS_DH_PARAMS_FILE).get_ref<const std::string&>();
        if (const auto ec = sslLoadDHParams(rsComm->ssl_ctx, dh_params_file.c_str()); ec < 0) {
            SSL_CTX_free(rsComm->ssl_ctx);
            rsComm->ssl_ctx = nullptr;
            log_network::error("error setting Diffie-Hellman parameters");
            return SSL_INIT_ERROR;
        }
    }
    catch (const irods::exception& e) {
        SSL_CTX_free(rsComm->ssl_ctx);
        log_network::error("Error getting TLS configuration: {}", e.client_display_what());
        return CONFIGURATION_ERROR;
    }
    catch (const std::exception& e) {
        SSL_CTX_free(rsComm->ssl_ctx);
        log_network::error("Error getting TLS configuration: {}", e.what());
        return CONFIGURATION_ERROR;
    }

    rsComm->ssl = sslInitSocket( rsComm->ssl_ctx, rsComm->sock );
    if ( rsComm->ssl == NULL ) {
        rodsLog( LOG_ERROR, "sslAccept: couldn't initialize SSL socket" );
        SSL_CTX_free( rsComm->ssl_ctx );
        rsComm->ssl_ctx = NULL;
        return SSL_INIT_ERROR;
    }

    const auto status = SSL_accept(rsComm->ssl);
    if ( status < 1 ) {
        sslLogError( "sslAccept: error calling SSL_accept" );
        return SSL_HANDSHAKE_ERROR;
    }

    rsComm->ssl_on = 1;
    snprintf( rsComm->negotiation_results, sizeof( rsComm->negotiation_results ),
              "%s", irods::CS_NEG_USE_SSL.c_str() );

    rodsLog( LOG_DEBUG, "sslAccept: accepted SSL connection" );

    return 0;
}

int
sslShutdown( rsComm_t *rsComm ) {
    int status;

    /* shut down the SSL connection. Might need to call SSL_shutdown()
       twice to allow the protocol to notify and then complete
       the shutdown. */
    status = SSL_shutdown( rsComm->ssl );
    if ( status == 0 ) {
        /* second phase of shutdown */
        status = SSL_shutdown( rsComm->ssl );
    }
    if ( status != 1 ) {
        sslLogError( "sslShutdown: error completing shutdown of SSL connection" );
        return SSL_SHUTDOWN_ERROR;
    }

    /* clean up the SSL state */
    SSL_free( rsComm->ssl );
    rsComm->ssl = NULL;
    SSL_CTX_free( rsComm->ssl_ctx );
    rsComm->ssl_ctx = NULL;
    rsComm->ssl_on = 0;

    snprintf( rsComm->negotiation_results, sizeof( rsComm->negotiation_results ),
              "%s", irods::CS_NEG_USE_TCP.c_str() );
    rodsLog( LOG_DEBUG, "sslShutdown: shut down SSL connection" );

    return 0;
}


int
sslReadMsgHeader( int sock, msgHeader_t *myHeader, struct timeval *tv, SSL *ssl ) {
    int nbytes;
    int myLen;
    char tmpBuf[MAX_NAME_LEN];
    msgHeader_t *outHeader;
    int status;

    /* read the header length packet */

    nbytes = sslRead( sock, ( void * ) &myLen, sizeof( myLen ),
                      NULL, tv, ssl );
    if ( nbytes != sizeof( myLen ) ) {
        if ( nbytes < 0 ) {
            status = nbytes - errno;
        }
        else {
            status = SYS_HEADER_READ_LEN_ERR - errno;
        }
        rodsLog( LOG_ERROR,
                 "sslReadMsgHeader:header read- read %d bytes, expect %d, status = %d",
                 nbytes, sizeof( myLen ), status );
        return status;
    }

    myLen =  ntohl( myLen );
    if ( myLen > MAX_NAME_LEN || myLen <= 0 ) {
        rodsLog( LOG_ERROR,
                 "sslReadMsgHeader: header length %d out of range",
                 myLen );
        return SYS_HEADER_READ_LEN_ERR;
    }

    nbytes = sslRead( sock, ( void * ) tmpBuf, myLen, NULL, tv, ssl );

    if ( nbytes != myLen ) {
        if ( nbytes < 0 ) {
            status = nbytes - errno;
        }
        else {
            status = SYS_HEADER_READ_LEN_ERR - errno;
        }
        rodsLog( LOG_ERROR,
                 "sslReadMsgHeader:header read- read %d bytes, expect %d, status = %d",
                 nbytes, myLen, status );
        return status;
    }

    if ( getRodsLogLevel() >= LOG_DEBUG8 ) {
        printf( "received header: len = %d\n%s\n", myLen, tmpBuf );
    }

    /* always use XML_PROT for the startup pack */
    status = unpack_struct( ( void * ) tmpBuf, ( void ** )( static_cast<void *>( &outHeader ) ),
                           "MsgHeader_PI", RodsPackTable, XML_PROT, nullptr);

    if ( status < 0 ) {
        rodsLogError( LOG_ERROR,  status,
                      "sslReadMsgHeader:unpackStruct error. status = %d",
                      status );
        return status;
    }

    *myHeader = *outHeader;

    free( outHeader );

    return 0;
}

int
sslReadMsgBody( int sock, msgHeader_t *myHeader, bytesBuf_t *inputStructBBuf,
                bytesBuf_t *bsBBuf, bytesBuf_t *errorBBuf, irodsProt_t irodsProt,
                struct timeval *tv, SSL *ssl ) {
    int nbytes;
    int bytesRead;

    if ( myHeader == NULL ) {
        return SYS_READ_MSG_BODY_INPUT_ERR;
    }
    if ( inputStructBBuf != NULL ) {
        memset( inputStructBBuf, 0, sizeof( bytesBuf_t ) );
    }

    /* Don't memset bsBBuf because bsBBuf can be reused on the client side */

    if ( errorBBuf != NULL ) {
        memset( errorBBuf, 0, sizeof( bytesBuf_t ) );
    }

    if ( myHeader->msgLen > 0 ) {
        if ( inputStructBBuf == NULL ) {
            return SYS_READ_MSG_BODY_INPUT_ERR;
        }

        inputStructBBuf->buf = malloc( myHeader->msgLen );

        nbytes = sslRead( sock, inputStructBBuf->buf, myHeader->msgLen,
                          NULL, tv, ssl );

        if (irodsProt == XML_PROT && getRodsLogLevel() >= LOG_DEBUG8) {
            const auto* buf = static_cast<char*>(inputStructBBuf->buf);

            if (!may_contain_sensitive_data(buf, inputStructBBuf->len)) {
                std::printf("received msg: \n%s\n", buf);
            }
        }

        if ( nbytes != myHeader->msgLen ) {
            rodsLog( LOG_NOTICE,
                     "sslReadMsgBody: inputStruct read error, read %d bytes, expect %d",
                     nbytes, myHeader->msgLen );
            free( inputStructBBuf->buf );
            return SYS_HEADER_READ_LEN_ERR;
        }
        inputStructBBuf->len = myHeader->msgLen;
    }

    if ( myHeader->errorLen > 0 ) {
        if ( errorBBuf == NULL ) {
            return SYS_READ_MSG_BODY_INPUT_ERR;
        }

        errorBBuf->buf = malloc( myHeader->errorLen );

        nbytes = sslRead( sock, errorBBuf->buf, myHeader->errorLen,
                          NULL, tv, ssl );

        if (irodsProt == XML_PROT && getRodsLogLevel() >= LOG_DEBUG8) {
            const auto* buf = static_cast<char*>(errorBBuf->buf);

            if (!may_contain_sensitive_data(buf, errorBBuf->len)) {
                std::printf("received error msg: \n%s\n", buf);
            }
        }

        if ( nbytes != myHeader->errorLen ) {
            rodsLog( LOG_NOTICE,
                     "sslReadMsgBody: errorBbuf read error, read %d bytes, expect %d, errno = %d",
                     nbytes, myHeader->msgLen, errno );
            free( errorBBuf->buf );
            return SYS_READ_MSG_BODY_LEN_ERR - errno;
        }
        errorBBuf->len = myHeader->errorLen;
    }

    if ( myHeader->bsLen > 0 ) {
        if ( bsBBuf == NULL ) {
            return SYS_READ_MSG_BODY_INPUT_ERR;
        }

        if ( bsBBuf->buf == NULL ) {
            bsBBuf->buf = malloc( myHeader->bsLen );
        }
        else if ( myHeader->bsLen > bsBBuf->len ) {
            free( bsBBuf->buf );
            bsBBuf->buf = malloc( myHeader->bsLen );
        }

        nbytes = sslRead( sock, bsBBuf->buf, myHeader->bsLen,
                          &bytesRead, tv, ssl );

        if ( nbytes != myHeader->bsLen ) {
            rodsLog( LOG_NOTICE,
                     "sslReadMsgBody: bsBBuf read error, read %d bytes, expect %d, errno = %d",
                     nbytes, myHeader->bsLen, errno );
            free( bsBBuf->buf );
            return SYS_READ_MSG_BODY_INPUT_ERR - errno;
        }
        bsBBuf->len = myHeader->bsLen;
    }

    return 0;
}

int
sslWriteMsgHeader( msgHeader_t *myHeader, SSL *ssl ) {
    int nbytes;
    int status;
    int myLen;
    bytesBuf_t *headerBBuf = NULL;

    /* always use XML_PROT for the Header */
    status = pack_struct( ( void * ) myHeader, &headerBBuf,
                         "MsgHeader_PI", RodsPackTable, 0, XML_PROT, nullptr);

    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status,
                      "sslWriteMsgHeader: packStruct error, status = %d", status );
        return status;
    }

    if ( getRodsLogLevel() >= LOG_DEBUG8 ) {
        printf( "sending header: len = %d\n%s\n", headerBBuf->len,
                ( char * ) headerBBuf->buf );
    }

    myLen = htonl( headerBBuf->len );

    nbytes = sslWrite( ( void * ) &myLen, sizeof( myLen ), NULL, ssl );

    if ( nbytes != sizeof( myLen ) ) {
        rodsLog( LOG_ERROR,
                 "sslWriteMsgHeader: wrote %d bytes for myLen , expect %d, status = %d",
                 nbytes, sizeof( myLen ), SYS_HEADER_WRITE_LEN_ERR - errno );
        freeBBuf( headerBBuf );
        return SYS_HEADER_WRITE_LEN_ERR - errno;
    }

    /* now send the header */

    nbytes = sslWrite( headerBBuf->buf, headerBBuf->len, NULL, ssl );

    if ( headerBBuf->len != nbytes ) {
        rodsLog( LOG_ERROR,
                 "sslWriteMsgHeader: wrote %d bytes, expect %d, status = %d",
                 nbytes, headerBBuf->len, SYS_HEADER_WRITE_LEN_ERR - errno );
        freeBBuf( headerBBuf );
        return SYS_HEADER_WRITE_LEN_ERR - errno;
    }

    freeBBuf( headerBBuf );

    return 0;
}

int
sslSendRodsMsg( char *msgType, bytesBuf_t *msgBBuf,
                bytesBuf_t *byteStreamBBuf, bytesBuf_t *errorBBuf, int intInfo,
                irodsProt_t irodsProt, SSL *ssl ) {
    int status;
    msgHeader_t msgHeader;
    int bytesWritten;

    memset( &msgHeader, 0, sizeof( msgHeader ) );

    rstrcpy( msgHeader.type, msgType, HEADER_TYPE_LEN );

    msgHeader.msgLen = msgBBuf ? msgBBuf->len : 0;
    msgHeader.bsLen = byteStreamBBuf ? byteStreamBBuf->len : 0;
    msgHeader.errorLen = errorBBuf ? errorBBuf->len : 0;

    msgHeader.intInfo = intInfo;

    status = sslWriteMsgHeader( &msgHeader, ssl );

    if ( status < 0 ) {
        return status;
    }

    /* send the rest */

    if (msgBBuf && msgBBuf->len > 0) {
        if (irodsProt == XML_PROT && getRodsLogLevel() >= LOG_DEBUG8) {
            const auto* buf = static_cast<char*>(msgBBuf->buf);

            if (!may_contain_sensitive_data(buf, msgBBuf->len)) {
                std::printf("sending msg: \n%s\n", buf);
            }
        }
        status = sslWrite( msgBBuf->buf, msgBBuf->len, NULL, ssl );
        if ( status < 0 ) {
            return status;
        }
    }

    if (errorBBuf && errorBBuf->len > 0) {
        if (irodsProt == XML_PROT && getRodsLogLevel() >= LOG_DEBUG8) {
            const auto* buf = static_cast<char*>(errorBBuf->buf);

            if (!may_contain_sensitive_data(buf, errorBBuf->len)) {
                std::printf("sending error msg: \n%s\n", buf);
            }
        }
        status = sslWrite( errorBBuf->buf, errorBBuf->len,
                           NULL, ssl );
        if ( status < 0 ) {
            return status;
        }
    }
    if ( byteStreamBBuf && byteStreamBBuf->len > 0 ) {
        status = sslWrite( byteStreamBBuf->buf, byteStreamBBuf->len,
                           &bytesWritten, ssl );
        if ( status < 0 ) {
            return status;
        }
    }

    return 0;
}

int
sslRead( int sock, void *buf, int len,
         int *bytesRead, struct timeval *tv, SSL *ssl ) {
    struct timeval timeout;

    /* Initialize the file descriptor set. */
    fd_set set;
    FD_ZERO( &set );
    FD_SET( sock, &set );
    if ( tv != NULL ) {
        timeout = *tv;
    }

    int toRead = len;
    char *tmpPtr = ( char * ) buf;

    if ( bytesRead != NULL ) {
        *bytesRead = 0;
    }

    while ( toRead > 0 ) {
        if ( SSL_pending( ssl ) == 0 && tv != NULL ) {
            const int status = select( sock + 1, &set, NULL, NULL, &timeout );
            if ( status == 0 ) {
                /* timedout */
                if ( len - toRead > 0 ) {
                    return len - toRead;
                }
                else {
                    return SYS_SOCK_READ_TIMEDOUT;
                }
            }
            else if ( status < 0 ) {
                if ( errno == EINTR ) {
                    continue;
                }
                else {
                    return SYS_SOCK_READ_ERR - errno;
                }
            }
        }
        int nbytes = SSL_read( ssl, ( void * ) tmpPtr, toRead );
        if ( SSL_get_error( ssl, nbytes ) != SSL_ERROR_NONE ) {
            if ( errno == EINTR ) {
                /* interrupted */
                errno = 0;
                nbytes = 0;
            }
            else {
                break;
            }
        }

        toRead -= nbytes;
        tmpPtr += nbytes;
        if ( bytesRead != NULL ) {
            *bytesRead += nbytes;
        }
    }
    return len - toRead;
}

int
sslWrite( void *buf, int len,
          int *bytesWritten, SSL *ssl ) {
    int nbytes;
    int toWrite;
    char *tmpPtr;

    toWrite = len;
    tmpPtr = ( char * ) buf;

    if ( bytesWritten != NULL ) {
        *bytesWritten = 0;
    }

    while ( toWrite > 0 ) {
        nbytes = SSL_write( ssl, ( void * ) tmpPtr, toWrite );
        if ( SSL_get_error( ssl, nbytes ) != SSL_ERROR_NONE ) {
            if ( errno == EINTR ) {
                /* interrupted */
                errno = 0;
                nbytes = 0;
            }
            else {
                break;
            }
        }
        toWrite -= nbytes;
        tmpPtr += nbytes;
        if ( bytesWritten != NULL ) {
            *bytesWritten += nbytes;
        }
    }
    return len - toWrite;
}

/* Module internal support functions */

static SSL_CTX* sslInit(const char* certfile, const char* keyfile)
{
    rodsEnv env;
    int status = getRodsEnv( &env );
    if ( status < 0 ) {
        rodsLog(
            LOG_ERROR,
            "sslInit - failed in getRodsEnv : %d",
            status );
        return NULL;

    }

    /* in our test programs we set up a null signal
       handler for SIGPIPE */
    /* signal(SIGPIPE, sslSigpipeHandler); */

    SSL_CTX* ctx = SSL_CTX_new( SSLv23_method() );

    SSL_CTX_set_options( ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_SINGLE_DH_USE );

    /* load our keys and certificates if provided */
    if ( certfile ) {
        if ( SSL_CTX_use_certificate_chain_file( ctx, certfile ) != 1 ) {
            sslLogError( "sslInit: couldn't read certificate chain file" );
            SSL_CTX_free( ctx );
            return NULL;
        }
        else {
            if ( SSL_CTX_use_PrivateKey_file( ctx, keyfile, SSL_FILETYPE_PEM ) != 1 ) {
                sslLogError( "sslInit: couldn't read key file" );
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
            sslLogError( "sslInit: error loading CA certificate locations" );
        }
    }
    if ( SSL_CTX_set_default_verify_paths( ctx ) != 1 ) {
        sslLogError( "sslInit: error loading default CA certificate locations" );
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
        sslLogError( "sslInit: couldn't set the cipher list (no valid ciphers)" );
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
        sslLogError( "sslInitSocket: BIO allocation error" );
        return NULL;
    }
    ssl = SSL_new( ctx );
    if ( ssl == NULL ) {
        sslLogError( "sslInitSocket: couldn't create a new SSL socket" );
        BIO_free( bio );
        return NULL;
    }
    SSL_set_bio( ssl, bio, bio );

    return ssl;
}

static void
sslLogError( char *msg ) {
    unsigned long err;
    char buf[512];

    while ( ( err = ERR_get_error() ) ) {
        ERR_error_string_n( err, buf, 512 );
        rodsLog( LOG_ERROR, "%s. SSL error: %s", msg, buf );
    }
}

static auto sslLoadDHParams(SSL_CTX* ctx, const char* file) -> int
{
    // dhparams file is required for secure communications in iRODS. Bail out.
    if (!file) {
        return -1;
    }

    BIO* bio = BIO_new_file(file, "r");
    if (!bio) {
        return -1;
    }

    const auto free_bio = irods::at_scope_exit{[&bio] { BIO_free(bio); }};
    constexpr const char* format = "PEM";
    constexpr const char* structure = nullptr;
    constexpr const char* keytype = "DH";
    constexpr int selection = 0;

    EVP_PKEY* pkey = nullptr;
    OSSL_DECODER_CTX* dctx =
        OSSL_DECODER_CTX_new_for_pkey(&pkey, format, structure, keytype, selection, nullptr, nullptr);
    if (!dctx) {
        return -1;
    }
    const auto free_decoder_context = irods::at_scope_exit{[&dctx] { OSSL_DECODER_CTX_free(dctx); }};
    // pkey is created with the decoded data from the bio
    if (0 == OSSL_DECODER_from_bio(dctx, bio)) {
        return -1;
    }

    if (!pkey) {
        return -1;
    }

    if (SSL_CTX_set0_tmp_dh_pkey(ctx, pkey) < 0) {
        return -1;
    }

    return 0;
} // sslLoadDHParams

static int
sslVerifyCallback( int ok, X509_STORE_CTX *store ) {
    char data[256];

    /* log any verification problems, even if we'll still accept the cert */
    if ( !ok ) {
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

static int
sslPostConnectionCheck( SSL *ssl, char *peer ) {
    rodsEnv env;
    int status = getRodsEnv( &env );
    if ( status < 0 ) {
        rodsLog(
            LOG_ERROR,
            "sslPostConnectionCheck - failed in getRodsEnv : %d",
            status );
        return status;

    }

    int match = 0;
    char cn[256];

    const char* verify_server = env.irodsSSLVerifyServer;
    if ( verify_server && strcmp( verify_server, "hostname" ) ) {
        /* not being asked to verify that the peer hostname
           is in the certificate. */
        return 1;
    }

    auto* cert = SSL_get1_peer_certificate(ssl);
    if ( cert == NULL ) {
        /* no certificate presented */
        return 0;
    }

    if ( peer == NULL ) {
        /* no hostname passed to verify */
        X509_free( cert );
        return 0;
    }

    /* check if the peer name matches any of the subjectAltNames
       listed in the certificate */
    auto names = static_cast<STACK_OF(GENERAL_NAME)*>(X509_get_ext_d2i( cert, NID_subject_alt_name, NULL, NULL ));
    std::size_t num_names = sk_GENERAL_NAME_num( names );
    for ( std::size_t i = 0; i < num_names; i++ ) {
        auto* name = sk_GENERAL_NAME_value( names, i );
        if ( name->type == GEN_DNS ) {
            if ( !strcasecmp( reinterpret_cast<const char*>(ASN1_STRING_get0_data( name->d.dNSName )), peer ) ) {
                match = 1;
                break;
            }
        }
    }
    sk_GENERAL_NAME_free( names );

    /* if no match above, check the common name in the certificate */
    if ( !match &&
            ( X509_NAME_get_text_by_NID( X509_get_subject_name( cert ),
                                         NID_commonName, cn, 256 ) != -1 ) ) {
        cn[255] = 0;
        if ( !strcasecmp( cn, peer ) ) {
            match = 1;
        }
        else if ( cn[0] == '*' ) { /* wildcard domain */
            char *tmp = strchr( peer, '.' );
            if ( tmp && !strcasecmp( tmp, cn + 1 ) ) {
                match = 1;
            }
        }
    }

    X509_free( cert );

    if ( match ) {
        return 1;
    }
    else {
        return 0;
    }
}
