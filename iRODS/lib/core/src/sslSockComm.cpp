/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* sslSockComm.c - SSL socket communication routines
 */


#include "rodsClient.hpp"
#include "sslSockComm.hpp"
#include "irods_client_server_negotiation.hpp"

// =-=-=-=-=-=-=-
// work around for SSL Macro version issues
#ifdef sk_GENERAL_NAMES_num
#define GENERAL_NAMES_NUM sk_GENERAL_NAMES_num
#define GENERAL_NAMES_VALUE sk_GENERAL_NAMES_value
#define GENERAL_NAMES_FREE sk_GENERAL_NAMES_free
#else
#define GENERAL_NAMES_NUM sk_GENERAL_NAME_num
#define GENERAL_NAMES_VALUE sk_GENERAL_NAME_value
#define GENERAL_NAMES_FREE sk_GENERAL_NAME_free
#endif



/* module internal functions */
static SSL_CTX *sslInit( char *certfile, char *keyfile );
static SSL *sslInitSocket( SSL_CTX *ctx, int sock );
static void sslLogError( char *msg );
static DH *get_dh2048();
static int sslLoadDHParams( SSL_CTX *ctx, char *file );
static int sslVerifyCallback( int ok, X509_STORE_CTX *store );
static int sslPostConnectionCheck( SSL *ssl, char *peer );

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

int
sslAccept( rsComm_t *rsComm ) {
    int status;

    /* set up the context using a certificate file and separate
       keyfile passed through environment variables */
    rsComm->ssl_ctx = sslInit( getenv( "irodsSSLCertificateChainFile" ),
                               getenv( "irodsSSLCertificateKeyFile" ) );
    if ( rsComm->ssl_ctx == NULL ) {
        rodsLog( LOG_ERROR, "sslAccept: couldn't initialize SSL context" );
        return SSL_INIT_ERROR;
    }

    status = sslLoadDHParams( rsComm->ssl_ctx, getenv( "irodsSSLDHParamsFile" ) );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "sslAccept: error setting Diffie-Hellman parameters" );
        SSL_CTX_free( rsComm->ssl_ctx );
        rsComm->ssl_ctx = NULL;
        return SSL_INIT_ERROR;
    }

    rsComm->ssl = sslInitSocket( rsComm->ssl_ctx, rsComm->sock );
    if ( rsComm->ssl == NULL ) {
        rodsLog( LOG_ERROR, "sslAccept: couldn't initialize SSL socket" );
        SSL_CTX_free( rsComm->ssl_ctx );
        rsComm->ssl_ctx = NULL;
        return SSL_INIT_ERROR;
    }

    status = SSL_accept( rsComm->ssl );
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

    if ( getRodsLogLevel() >= LOG_DEBUG3 ) {
        printf( "received header: len = %d\n%s\n", myLen, tmpBuf );
    }

    /* always use XML_PROT for the startup pack */
    status = unpackStruct( ( void * ) tmpBuf, ( void ** )( static_cast<void *>( &outHeader ) ),
                           "MsgHeader_PI", RodsPackTable, XML_PROT );

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

        if ( irodsProt == XML_PROT && getRodsLogLevel() >= LOG_DEBUG3 ) {
            printf( "received msg: \n%s\n", ( char * ) inputStructBBuf->buf );
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

        if ( irodsProt == XML_PROT && getRodsLogLevel() >= LOG_DEBUG3 ) {
            printf( "received error msg: \n%s\n", ( char * ) errorBBuf->buf );
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
    status = packStruct( ( void * ) myHeader, &headerBBuf,
                         "MsgHeader_PI", RodsPackTable, 0, XML_PROT );

    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status,
                      "sslWriteMsgHeader: packStruct error, status = %d", status );
        return status;
    }

    if ( getRodsLogLevel() >= LOG_DEBUG3 ) {
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

    if ( msgBBuf && msgBBuf->len > 0 ) {
        if ( irodsProt == XML_PROT && getRodsLogLevel() >= LOG_DEBUG3 ) {
            printf( "sending msg: \n%s\n", ( char * ) msgBBuf->buf );
        }
        status = sslWrite( msgBBuf->buf, msgBBuf->len, NULL, ssl );
        if ( status < 0 ) {
            return status;
        }
    }

    if ( errorBBuf && errorBBuf->len > 0 ) {
        if ( irodsProt == XML_PROT && getRodsLogLevel() >= LOG_DEBUG3 ) {
            printf( "sending error msg: \n%s\n", ( char * ) errorBBuf->buf );
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

static SSL_CTX*
sslInit( char *certfile, char *keyfile ) {
    static int init_done = 0;
    SSL_CTX *ctx;
    char *ca_path;
    char *ca_file;
    char *verify_server;

    if ( !init_done ) {
        SSL_library_init();
        SSL_load_error_strings();
        init_done = 1;
    }
    /* in our test programs we set up a null signal
       handler for SIGPIPE */
    /* signal(SIGPIPE, sslSigpipeHandler); */

    ctx = SSL_CTX_new( SSLv23_method() );

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
    ca_path = getenv( "irodsSSLCACertificatePath" );
    ca_file = getenv( "irodsSSLCACertificateFile" );
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
    verify_server = getenv( "irodsSSLVerifyServer" );
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

/* This function returns a set of built-in Diffie-Hellman
   parameters. We could read the parameters from a file instead,
   but this is a reasonably strong prime. The parameters come from
   the openssl distribution's 'apps' sub-directory. Comment from
   the source file is:

   These are the 2048 bit DH parameters from "Assigned Number for SKIP Protocols"
   (http://www.skip-vpn.org/spec/numbers.html).
   See there for how they were generated. */

static DH*
get_dh2048() {
    static unsigned char dh2048_p[] = {
        0xF6, 0x42, 0x57, 0xB7, 0x08, 0x7F, 0x08, 0x17, 0x72, 0xA2, 0xBA, 0xD6,
        0xA9, 0x42, 0xF3, 0x05, 0xE8, 0xF9, 0x53, 0x11, 0x39, 0x4F, 0xB6, 0xF1,
        0x6E, 0xB9, 0x4B, 0x38, 0x20, 0xDA, 0x01, 0xA7, 0x56, 0xA3, 0x14, 0xE9,
        0x8F, 0x40, 0x55, 0xF3, 0xD0, 0x07, 0xC6, 0xCB, 0x43, 0xA9, 0x94, 0xAD,
        0xF7, 0x4C, 0x64, 0x86, 0x49, 0xF8, 0x0C, 0x83, 0xBD, 0x65, 0xE9, 0x17,
        0xD4, 0xA1, 0xD3, 0x50, 0xF8, 0xF5, 0x59, 0x5F, 0xDC, 0x76, 0x52, 0x4F,
        0x3D, 0x3D, 0x8D, 0xDB, 0xCE, 0x99, 0xE1, 0x57, 0x92, 0x59, 0xCD, 0xFD,
        0xB8, 0xAE, 0x74, 0x4F, 0xC5, 0xFC, 0x76, 0xBC, 0x83, 0xC5, 0x47, 0x30,
        0x61, 0xCE, 0x7C, 0xC9, 0x66, 0xFF, 0x15, 0xF9, 0xBB, 0xFD, 0x91, 0x5E,
        0xC7, 0x01, 0xAA, 0xD3, 0x5B, 0x9E, 0x8D, 0xA0, 0xA5, 0x72, 0x3A, 0xD4,
        0x1A, 0xF0, 0xBF, 0x46, 0x00, 0x58, 0x2B, 0xE5, 0xF4, 0x88, 0xFD, 0x58,
        0x4E, 0x49, 0xDB, 0xCD, 0x20, 0xB4, 0x9D, 0xE4, 0x91, 0x07, 0x36, 0x6B,
        0x33, 0x6C, 0x38, 0x0D, 0x45, 0x1D, 0x0F, 0x7C, 0x88, 0xB3, 0x1C, 0x7C,
        0x5B, 0x2D, 0x8E, 0xF6, 0xF3, 0xC9, 0x23, 0xC0, 0x43, 0xF0, 0xA5, 0x5B,
        0x18, 0x8D, 0x8E, 0xBB, 0x55, 0x8C, 0xB8, 0x5D, 0x38, 0xD3, 0x34, 0xFD,
        0x7C, 0x17, 0x57, 0x43, 0xA3, 0x1D, 0x18, 0x6C, 0xDE, 0x33, 0x21, 0x2C,
        0xB5, 0x2A, 0xFF, 0x3C, 0xE1, 0xB1, 0x29, 0x40, 0x18, 0x11, 0x8D, 0x7C,
        0x84, 0xA7, 0x0A, 0x72, 0xD6, 0x86, 0xC4, 0x03, 0x19, 0xC8, 0x07, 0x29,
        0x7A, 0xCA, 0x95, 0x0C, 0xD9, 0x96, 0x9F, 0xAB, 0xD0, 0x0A, 0x50, 0x9B,
        0x02, 0x46, 0xD3, 0x08, 0x3D, 0x66, 0xA4, 0x5D, 0x41, 0x9F, 0x9C, 0x7C,
        0xBD, 0x89, 0x4B, 0x22, 0x19, 0x26, 0xBA, 0xAB, 0xA2, 0x5E, 0xC3, 0x55,
        0xE9, 0x32, 0x0B, 0x3B,
    };
    static unsigned char dh2048_g[] = {
        0x02,
    };
    DH *dh;

    if ( ( dh = DH_new() ) == NULL ) {
        return NULL;
    }
    dh->p = BN_bin2bn( dh2048_p, sizeof( dh2048_p ), NULL );
    dh->g = BN_bin2bn( dh2048_g, sizeof( dh2048_g ), NULL );
    if ( ( dh->p == NULL ) || ( dh->g == NULL ) ) {
        DH_free( dh );
        return NULL;
    }
    return dh;
}

static int
sslLoadDHParams( SSL_CTX *ctx, char *file ) {
    DH *dhparams = NULL;
    BIO *bio;

    if ( file ) {
        bio = BIO_new_file( file, "r" );
        if ( bio ) {
            dhparams = PEM_read_bio_DHparams( bio, NULL, NULL, NULL );
            BIO_free( bio );
        }
    }

    if ( dhparams == NULL ) {
        sslLogError( "sslLoadDHParams: can't load DH parameter file. Falling back to built-ins." );
        dhparams = get_dh2048();
        if ( dhparams == NULL ) {
            rodsLog( LOG_ERROR, "sslLoadDHParams: can't load built-in DH params" );
            return -1;
        }
    }

    if ( SSL_CTX_set_tmp_dh( ctx, dhparams ) < 0 ) {
        sslLogError( "sslLoadDHParams: couldn't set DH parameters" );
        return -1;
    }
    return 0;
}

static int
sslVerifyCallback( int ok, X509_STORE_CTX *store ) {
    char data[256];

    /* log any verification problems, even if we'll still accept the cert */
    if ( !ok ) {
        X509 *cert = X509_STORE_CTX_get_current_cert( store );
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
    char *verify_server;
    X509 *cert;
    int match = 0;
    STACK_OF( GENERAL_NAMES ) *names;
    GENERAL_NAME *name;
    int num_names, i;
    char *namestr;
    char cn[256];

    verify_server = getenv( "irodsSSLVerifyServer" );
    if ( verify_server && strcmp( verify_server, "hostname" ) ) {
        /* not being asked to verify that the peer hostname
           is in the certificate. */
        return 1;
    }

    cert = SSL_get_peer_certificate( ssl );
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
    names = ( STACK_OF( GENERAL_NAMES )* )X509_get_ext_d2i( cert, NID_subject_alt_name, NULL, NULL );
    num_names = GENERAL_NAMES_NUM( names );
    for ( i = 0; i < num_names; i++ ) {
        name = ( GENERAL_NAME* )GENERAL_NAMES_VALUE( names, i );
        if ( name->type == GEN_DNS ) {
            namestr = ( char* )ASN1_STRING_data( name->d.dNSName );
            if ( !strcasecmp( namestr, peer ) ) {
                match = 1;
                break;
            }
        }
    }
    GENERAL_NAMES_FREE( names );

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
