// =-=-=-=-=-=-=-
// irods includes
#include "rodsDef.h"
#include "msParam.hpp"
#include "reGlobalsExtern.hpp"
#include "rcConnect.hpp"
#include "sockComm.hpp"

// =-=-=-=-=-=-=-
#include "irods_network_plugin.hpp"
#include "irods_network_constants.hpp"
#include "irods_ssl_object.hpp"
#include "irods_stacktrace.hpp"
#include "irods_buffer_encryption.hpp"
#include "sockCommNetworkInterface.hpp"

// =-=-=-=-=-=-=-
// stl includes
#include <sstream>
#include <string>
#include <iostream>

// =-=-=-=-=-=-=-
// ssl includes
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <openssl/err.h>

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

// =-=-=-=-=-=-=-
//
#define SSL_CIPHER_LIST "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH"

// =-=-=-=-=-=-=-
// key for ssl shared secret property
const std::string SHARED_KEY( "ssl_network_plugin_shared_key" );

// =-=-=-=-=-=-=-
//
static void ssl_log_error(
    const char *msg ) {
    unsigned long err;
    char buf[512];

    while ( ( err = ERR_get_error() ) ) {
        ERR_error_string_n( err, buf, 512 );
        rodsLog( LOG_ERROR, "%s. SSL error: %s", msg, buf );
    }

} // ssl_log_error

// =-=-=-=-=-=-=-
//
static void ssl_build_error_string(
    std::string& _str ) {
    unsigned long err;
    char buf[512];

    while ( ( err = ERR_get_error() ) ) {
        ERR_error_string_n( err, buf, 512 );
        _str += " | ";
        _str += buf;
    }

} //ssl_build_error_string

/* This function returns a set of built-in Diffie-Hellman
   parameters. We could read the parameters from a file instead,
   but this is a reasonably strong prime. The parameters come from
   the openssl distribution's 'apps' sub-directory. Comment from
   the source file is:

   These are the 2048 bit DH parameters from "Assigned Number for SKIP Protocols"
   (http://www.skip-vpn.org/spec/numbers.html).
   See there for how they were generated. */

static DH* ssl_get_dh2048() {
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

} // ssl_get_dh2048

// =-=-=-=-=-=-=-
//
static int ssl_load_hd_params(
    SSL_CTX* ctx,
    char*    file ) {
    DH*  dhparams = 0;
    BIO* bio      = 0;

    if ( file ) {
        bio = BIO_new_file( file, "r" );
        if ( bio ) {
            dhparams = PEM_read_bio_DHparams( bio, NULL, NULL, NULL );
            BIO_free( bio );
        }
    }

    if ( dhparams == NULL ) {
        ssl_log_error( "ssl_load_hd_params: can't load DH parameter file. Falling back to built-ins." );
        dhparams = ssl_get_dh2048();
        if ( dhparams == NULL ) {
            rodsLog( LOG_ERROR, "ssl_load_hd_params: can't load built-in DH params" );
            return -1;
        }
    }

    if ( SSL_CTX_set_tmp_dh( ctx, dhparams ) < 0 ) {
        ssl_log_error( "ssl_load_hd_params: couldn't set DH parameters" );
        return -1;
    }
    return 0;
}

// =-=-=-=-=-=-=-
//
static int ssl_verify_callback(
    int             ok,
    X509_STORE_CTX* store ) {
    char data[256];

    /* log any verification problems, even if we'll still accept the cert */
    if ( !ok ) {
        X509 *cert = X509_STORE_CTX_get_current_cert( store );
        int  depth = X509_STORE_CTX_get_error_depth( store );
        int  err   = X509_STORE_CTX_get_error( store );

        rodsLog( LOG_NOTICE, "ssl_verify_callback: problem with certificate at depth: %i", depth );
        X509_NAME_oneline( X509_get_issuer_name( cert ), data, 256 );
        rodsLog( LOG_NOTICE, "ssl_verify_callback:   issuer = %s", data );
        X509_NAME_oneline( X509_get_subject_name( cert ), data, 256 );
        rodsLog( LOG_NOTICE, "ssl_verify_callback:   subject = %s", data );
        rodsLog( LOG_NOTICE, "ssl_verify_callback:   err %i:%s", err,
                 X509_verify_cert_error_string( err ) );
    }

    return ok;
}


// =-=-=-=-=-=-=-
//
static SSL_CTX* ssl_init_context(
    char *certfile,
    char *keyfile ) {
    static int init_done = 0;
    SSL_CTX *ctx = 0;
    char *ca_path = 0;
    char *ca_file = 0;
    char *verify_server = 0;

    rodsEnv env;
    int status = getRodsEnv( &env );
    if ( status < 0 ) {
        rodsLog(
            LOG_ERROR,
            "ssl_init_context - failed in getRodsEnv : %d",
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
    ctx = SSL_CTX_new( SSLv23_method() );

    SSL_CTX_set_options( ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_SINGLE_DH_USE );

    /* load our keys and certificates if provided */
    if ( certfile ) {
        if ( SSL_CTX_use_certificate_chain_file( ctx, certfile ) != 1 ) {
            ssl_log_error( "sslInit: couldn't read certificate chain file" );
            SSL_CTX_free( ctx );
            return NULL;
        }
        else {
            if ( SSL_CTX_use_PrivateKey_file( ctx, keyfile, SSL_FILETYPE_PEM ) != 1 ) {
                ssl_log_error( "sslInit: couldn't read key file" );
                SSL_CTX_free( ctx );
                return NULL;
            }
        }
    }

    /* set up CA paths and files here */
    ca_path = env.irodsSSLCACertificatePath;
    ca_file = env.irodsSSLCACertificateFile;
    if ( strlen( ca_path ) > 0 || strlen( ca_file ) > 0 ) {
        if ( SSL_CTX_load_verify_locations( ctx, ca_file, ca_path ) != 1 ) {
            ssl_log_error( "sslInit: error loading CA certificate locations" );
        }
    }
    if ( SSL_CTX_set_default_verify_paths( ctx ) != 1 ) {
        ssl_log_error( "sslInit: error loading default CA certificate locations" );
    }

    /* Set up the default certificate verification */
    /* if "none" is specified, we won't stop the SSL handshake
       due to certificate error, but will log messages from
       the verification callback */
    verify_server = env.irodsSSLVerifyServer;
    if ( strlen( verify_server ) > 0 && !strcmp( verify_server, "none" ) ) {
        SSL_CTX_set_verify( ctx, SSL_VERIFY_NONE, ssl_verify_callback );
    }
    else {
        SSL_CTX_set_verify( ctx, SSL_VERIFY_PEER, ssl_verify_callback );
    }
    /* default depth is nine ... leave this here in case it needs modification */
    SSL_CTX_set_verify_depth( ctx, 9 );

    /* ciphers */
    if ( SSL_CTX_set_cipher_list( ctx, SSL_CIPHER_LIST ) != 1 ) {
        ssl_log_error( "sslInit: couldn't set the cipher list (no valid ciphers)" );
        SSL_CTX_free( ctx );
        return NULL;
    }

    return ctx;

} // ssl_init_context

// =-=-=-=-=-=-=-
//
static SSL* ssl_init_socket(
    SSL_CTX* _ctx,
    int      _socket_handle ) {

    BIO* bio = BIO_new_socket(
                   _socket_handle,
                   BIO_NOCLOSE );
    if ( bio == NULL ) {
        ssl_log_error( "sslInitSocket: BIO allocation error" );
        return NULL;
    }

    SSL* ssl = SSL_new( _ctx );
    if ( ssl == NULL ) {
        ssl_log_error( "sslInitSocket: couldn't create a new SSL socket" );
        BIO_free( bio );
        return NULL;
    }
    SSL_set_bio( ssl, bio, bio );

    return ssl;

} // ssl_init_socket

static int ssl_post_connection_check(
    SSL *ssl,
    const char *peer ) {
    char *verify_server = 0;
    X509 *cert = 0;
    int match = 0;
    STACK_OF( GENERAL_NAMES ) *names = 0;
    GENERAL_NAME *name = 0;
    int num_names = 0, i = 0;
    char *namestr = 0;
    char cn[256];

    rodsEnv env;
    int status = getRodsEnv( &env );
    if ( status < 0 ) {
        rodsLog(
            LOG_ERROR,
            "ssl_init_context - failed in getRodsEnv : %d",
            status );
        return 0;

    }

    verify_server = env.irodsSSLVerifyServer;
    if ( strlen( verify_server ) > 0 && strcmp( verify_server, "hostname" ) ) {
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
            const char *tmp = strchr( peer, '.' );
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

} // ssl_post_connection_check

extern "C" {
    // =-=-=-=-=-=-=-
    // 1. Define plugin Version Variable, used in plugin
    //    creation when the factory function is called.
    //    -- currently only 1.0 is supported.
    double PLUGIN_INTERFACE_VERSION = 1.0;

    // =-=-=-=-=-=-=-
    // local function to read a buffer from a socket
    irods::error ssl_socket_read(
        int             _socket,
        void*           _buffer,
        int             _length,
        int&            _bytes_read,
        struct timeval* _time_value,
        SSL*            _ssl ) {
        irods::error result = SUCCESS();

        // =-=-=-=-=-=-=-
        // check incoming pointers
        if ( ( result = ASSERT_ERROR( _buffer && _ssl, SYS_INVALID_INPUT_PARAM, "Null buffer or ssl pointer." ) ).ok() ) {

            // =-=-=-=-=-=-=-
            // Initialize the file descriptor set
            fd_set set;
            FD_ZERO( &set );
            FD_SET( _socket, &set );

            // =-=-=-=-=-=-=-
            // local copy of time value?
            struct timeval timeout;
            if ( _time_value != NULL ) {
                timeout = ( *_time_value );
            }

            // =-=-=-=-=-=-=-
            // local working variables
            int   len_to_read = _length;
            char* read_ptr    = static_cast<char*>( _buffer );

            // =-=-=-=-=-=-=-
            // reset bytes read
            _bytes_read = 0;

            // =-=-=-=-=-=-=-
            // loop while there is data to read
            while ( result.ok() && len_to_read > 0 ) {

                // =-=-=-=-=-=-=-
                // do a time out managed select of the socket fd
                if ( SSL_pending( _ssl ) == 0 && NULL != _time_value ) {
                    int status = select( _socket + 1, &set, NULL, NULL, &timeout );
                    if ( status == 0 ) {
                        // =-=-=-=-=-=-=-
                        // the select has timed out
                        if ( ( _length - len_to_read ) > 0 ) {
                            result = ERROR( _length - len_to_read, "failed to read requested number of bytes" );
                        }
                        else {
                            result =  ERROR( SYS_SOCK_READ_TIMEDOUT, "socket timeout error" );
                        }

                    }
                    else if ( status < 0 ) {

                        // =-=-=-=-=-=-=-
                        // keep trying on interrupt or just error out
                        int err_status = SYS_SOCK_READ_ERR - errno;
                        result = ASSERT_ERROR( errno != EINTR, err_status, "Error on select." );

                    } // else

                } // if tv

                // =-=-=-=-=-=-=-
                // select has been done, finally do the read
                int num_bytes = SSL_read( _ssl, ( void * ) read_ptr, len_to_read );

                // =-=-=-=-=-=-=-
                // error trapping the read
                if ( SSL_get_error( _ssl, num_bytes ) != SSL_ERROR_NONE ) {
                    // =-=-=-=-=-=-=-
                    // gracefully handle an interrupt
                    if ( EINTR == errno ) {
                        errno     = 0;
                        num_bytes = 0;
                    }
                    else {
                        result = ERROR( _length - len_to_read, "Failed to in SSL read." );
                    }
                }

                // =-=-=-=-=-=-=-
                // all has gone well, do byte book keeping
                len_to_read -= num_bytes;
                read_ptr    += num_bytes;
                _bytes_read += num_bytes;


            } // while

        } // if assert_error

        // =-=-=-=-=-=-=-
        // and were done? report length not read
        // return CODE( _length - len_to_read );
        return result;

    } // ssl_socket_read

    // =-=-=-=-=-=-=-
    // local function to write a buffer to a socket
    irods::error ssl_socket_write(
        void* _buffer,
        int   _length,
        int&  _bytes_written,
        SSL*  _ssl ) {
        irods::error result = SUCCESS();

        // =-=-=-=-=-=-=-
        // check incoming pointers
        if ( ( result = ASSERT_ERROR( _buffer && _ssl, SYS_INVALID_INPUT_PARAM, "Buffer or ssl pointer are null." ) ).ok() ) {

            // =-=-=-=-=-=-=-
            // local variables for write
            int   len_to_write = _length;
            char* write_ptr    = static_cast<char*>( _buffer );

            // =-=-=-=-=-=-=-
            // reset bytes written
            _bytes_written = 0;

            // =-=-=-=-=-=-=-
            // loop while there is data to read
            while ( result.ok() && len_to_write > 0 ) {
                int num_bytes = SSL_write( _ssl, static_cast<void*>( write_ptr ), len_to_write );

                // =-=-=-=-=-=-=-
                // error trapping the write
                if ( SSL_get_error( _ssl, num_bytes ) != SSL_ERROR_NONE ) {
                    // =-=-=-=-=-=-=-
                    // gracefully handle an interrupt
                    if ( errno == EINTR ) {
                        errno     = 0;
                        num_bytes = 0;

                    }
                    else {
                        result = ERROR( _length - len_to_write, "Failed to write to SSL" );
                    }
                }

                // =-=-=-=-=-=-=-
                // increment working variables
                len_to_write   -= num_bytes;
                write_ptr      += num_bytes;
                _bytes_written += num_bytes;

            }
        }

        // =-=-=-=-=-=-=-
        // and were done? report length not written
        // return CODE( _length - len_to_write );
        return result;

    } // ssl_socket_write

    // =-=-=-=-=-=-=-
    //
    irods::error ssl_read_msg_header(
        irods::plugin_context& _ctx,
        void*                   _buffer,
        struct timeval*         _time_val ) {
        irods::error result = SUCCESS();

        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid< irods::ssl_object >();
        if ( ( result = ASSERT_PASS( ret, "Invalid SSL plugin context." ) ).ok() ) {

            // =-=-=-=-=-=-=-
            // extract the useful bits from the context
            irods::ssl_object_ptr ssl_obj = boost::dynamic_pointer_cast< irods::ssl_object >( _ctx.fco() );
            int socket_handle = ssl_obj->socket_handle();

            // =-=-=-=-=-=-=-
            // read the header length packet */
            int header_length = 0;
            int bytes_read    = 0;
            ret = ssl_socket_read( socket_handle, static_cast<void*>( &header_length ), sizeof( int ), bytes_read,
                                   _time_val, ssl_obj->ssl() );
            if ( !ret.ok() || bytes_read != sizeof( header_length ) ) {

                int status = 0;
                if ( bytes_read < 0 ) {
                    status =  bytes_read - errno;
                }
                else {
                    status = SYS_HEADER_READ_LEN_ERR - errno;
                }
                std::stringstream msg;
                msg << "read "
                    << bytes_read
                    << " expected " << sizeof( header_length );
                return ERROR( status, msg.str() );
            }

            // =-=-=-=-=-=-=-
            // convert from network to host byte order
            header_length = ntohl( header_length );

            // =-=-=-=-=-=-=-
            // check head length against expected size range
            if ( ( result = ASSERT_ERROR( header_length <= MAX_NAME_LEN && header_length > 0, SYS_HEADER_READ_LEN_ERR,
                                          "Header length is out of range: %d expected >= 0 and < %d.", header_length, MAX_NAME_LEN ) ).ok() ) {

                // =-=-=-=-=-=-=-
                // now read the actual header
                ret = ssl_socket_read( socket_handle, _buffer, header_length, bytes_read, _time_val, ssl_obj->ssl() );
                if ( !ret.ok() ||
                        bytes_read != header_length ) {
                    int status = 0;
                    if ( bytes_read < 0 ) {
                        status = bytes_read - errno;
                    }
                    else {
                        status = SYS_HEADER_READ_LEN_ERR - errno;
                    }
                    std::stringstream msg;
                    msg << "read "
                        << bytes_read
                        << " expected " << header_length;
                    return ERROR( status, msg.str() );

                }

                // =-=-=-=-=-=-=-
                // log debug information if appropriate
                if ( getRodsLogLevel() >= LOG_DEBUG3 ) {
                    printf( "received header: len = %d\n%s\n",
                            header_length,
                            static_cast<char*>( _buffer ) );
                }
            }
        }

        return result;

    } // ssl_read_msg_header

    // =-=-=-=-=-=-=-
    //
    irods::error ssl_client_stop( irods::plugin_context& _ctx ) {
        irods::error result = SUCCESS();

        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid< irods::ssl_object >();
        if ( ( result = ASSERT_PASS( ret, "Invalid SSL plugin context." ) ).ok() ) {

            // =-=-=-=-=-=-=-
            // extract the useful bits from the context
            irods::ssl_object_ptr ssl_obj = boost::dynamic_pointer_cast< irods::ssl_object >( _ctx.fco() );
            SSL*     ssl = ssl_obj->ssl();
            SSL_CTX* ctx = ssl_obj->ssl_ctx();

            /* shut down the SSL connection. First SSL_shutdown() sends "close notify" */
            int status = SSL_shutdown( ssl );
            if ( status == 0 ) {
                /* do second phase of shutdown */
                status = SSL_shutdown( ssl );
            }

            std::string err_str = "error shutting down the SSL connection";
            ssl_build_error_string( err_str );
            if ( ( result = ASSERT_ERROR( status == 1, SSL_SHUTDOWN_ERROR, err_str.c_str() ) ).ok() ) {

                /* clean up the SSL state */
                SSL_free( ssl );
                SSL_CTX_free( ctx );

                ssl_obj->ssl( 0 );
                ssl_obj->ssl_ctx( 0 );
            }
        }

        return result;

    } // ssl_client_stop

    // =-=-=-=-=-=-=-
    //
    irods::error ssl_client_start(
        irods::plugin_context& _ctx,
        rodsEnv*                _env ) {
        irods::error result = SUCCESS();

        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid< irods::ssl_object >();
        if ( ( result = ASSERT_PASS( ret, "Invalid SSL plugin context." ) ).ok() ) {

            // =-=-=-=-=-=-=-
            // extract the useful bits from the context
            irods::ssl_object_ptr ssl_obj = boost::dynamic_pointer_cast< irods::ssl_object >( _ctx.fco() );

            // =-=-=-=-=-=-=-
            // set up SSL on our side of the socket
            SSL_CTX* ctx = ssl_init_context( NULL, NULL );
            std::string err_str = "failed to initialize SSL context";
            ssl_build_error_string( err_str );
            if ( ( result = ASSERT_ERROR( ctx, SSL_INIT_ERROR, err_str.c_str() ) ).ok() ) {

                // =-=-=-=-=-=-=-
                //
                SSL* ssl = ssl_init_socket( ctx, ssl_obj->socket_handle() );
                std::string err_str = "couldn't initialize SSL socket";
                ssl_build_error_string( err_str );
                if ( !( result = ASSERT_ERROR( ssl, SSL_INIT_ERROR, err_str.c_str() ) ).ok() ) {
                    SSL_CTX_free( ctx );
                }
                else {

                    // =-=-=-=-=-=-=-
                    //
                    int status = SSL_connect( ssl );
                    std::string err_str = "error in SSL_connect";
                    ssl_build_error_string( err_str );
                    if ( !( result = ASSERT_ERROR( status >= 1, SSL_HANDSHAKE_ERROR, err_str.c_str() ) ).ok() ) {
                        SSL_free( ssl );
                        SSL_CTX_free( ctx );
                    }
                    else {

                        // =-=-=-=-=-=-=-
                        //
                        int status = ssl_post_connection_check( ssl, ssl_obj->host().c_str() );
                        std::string err_str = "post connection certificate check failed";
                        ssl_build_error_string( err_str );
                        if ( !( result = ASSERT_ERROR( status, SSL_CERT_ERROR, err_str.c_str() ) ).ok() ) {
                            ssl_client_stop( _ctx );
                        }
                        else {

                            ssl_obj->ssl( ssl );
                            ssl_obj->ssl_ctx( ctx );

                            // =-=-=-=-=-=-=-
                            // check to see if a key has already been placed
                            // in the property map
                            irods::buffer_crypt::array_t key;
                            ret = _ctx.prop_map().get< irods::buffer_crypt::array_t >( SHARED_KEY, key );
                            if ( !ret.ok() ) {
                                // =-=-=-=-=-=-=-
                                // if no key exists then ship a new key and set the
                                // property
                                ret = irods::buffer_crypt::generate_key( key, _env->rodsEncryptionKeySize );
                                if ( !ret.ok() ) {
                                    irods::log( PASS( ret ) );
                                }

                                ret = _ctx.prop_map().set< irods::buffer_crypt::array_t >( SHARED_KEY, key );
                                if ( !ret.ok() ) {
                                    irods::log( PASS( ret ) );
                                }
                            }

                            if ( ( result = ASSERT_ERROR( _ctx.prop_map().has_entry( SHARED_KEY ),
                                                          -1, "irodsEncryption error. Failed to generate key." ) ).ok() ) {
                                // =-=-=-=-=-=-=-
                                // send a message to the agent containing the client
                                // size encryption environment variables
                                msgHeader_t msg_header;
                                memset( &msg_header, 0, sizeof( msg_header ) );
                                memcpy( msg_header.type, _env->rodsEncryptionAlgorithm, HEADER_TYPE_LEN );
                                msg_header.msgLen   = _env->rodsEncryptionKeySize;
                                msg_header.errorLen = _env->rodsEncryptionSaltSize;
                                msg_header.bsLen    = _env->rodsEncryptionNumHashRounds;

                                // =-=-=-=-=-=-=-
                                // error check the encryption envrionment
                                if ( ( result = ASSERT_ERROR( 0 != msg_header.msgLen && 0 != msg_header.errorLen && 0 != msg_header.bsLen,
                                                              -1, "irodsEncryption error. Key size, salt size or num hash rounds is 0." ) ).ok() ) {

                                    if ( ( result = ASSERT_ERROR( EVP_get_cipherbyname( msg_header.type ), -1, "irodsEncryptionAlgorithm \"%s\" is invalid.",
                                                                  msg_header.type ) ).ok() ) {

                                        // =-=-=-=-=-=-=-
                                        // use a message header to contain the encryption environment
                                        ret = writeMsgHeader( ssl_obj, &msg_header );
                                        if ( ( result = ASSERT_PASS( ret, "writeMsgHeader failed." ) ).ok() ) {

                                            // =-=-=-=-=-=-=-
                                            // send a message to the agent containing the shared secret
                                            bytesBuf_t key_bbuf;
                                            key_bbuf.len = key.size();
                                            key_bbuf.buf = &key[0];
                                            char msg_type[] = { "SHARED_SECRET" };
                                            ret = sendRodsMsg( ssl_obj, msg_type, &key_bbuf, 0, 0, 0, XML_PROT );
                                            if ( ( result = ASSERT_PASS( ret, "writeMsgHeader failed." ) ).ok() ) {

                                                // =-=-=-=-=-=-=-
                                                // set the key and env for this ssl object
                                                ssl_obj->shared_secret( key );
                                                ssl_obj->key_size( _env->rodsEncryptionKeySize );
                                                ssl_obj->salt_size( _env->rodsEncryptionSaltSize );
                                                ssl_obj->num_hash_rounds( _env->rodsEncryptionNumHashRounds );
                                                ssl_obj->encryption_algorithm( _env->rodsEncryptionAlgorithm );
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        return result;

    } // ssl_client_start

    // =-=-=-=-=-=-=-
    //
    irods::error ssl_agent_start(
        irods::plugin_context& _ctx ) {
        irods::error result = SUCCESS();
        rodsEnv env;
        int status = getRodsEnv( &env );
        if ( status < 0 ) {
            rodsLog(
                LOG_ERROR,
                "ssl_init_context - failed in getRodsEnv : %d",
                status );
            return ERROR(
                       status,
                       "failed in getRodsEnv" );

        }

        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid< irods::ssl_object >();
        if ( ( result = ASSERT_PASS( ret, "Invalid SSL plugin context." ) ).ok() ) {

            // =-=-=-=-=-=-=-
            // extract the useful bits from the context
            irods::ssl_object_ptr ssl_obj = boost::dynamic_pointer_cast< irods::ssl_object >( _ctx.fco() );

            // =-=-=-=-=-=-=-
            // set up the context using a certificate file and separate
            // keyfile passed through environment variables
            SSL_CTX* ctx = ssl_init_context( env.irodsSSLCertificateChainFile,
                                             env.irodsSSLCertificateKeyFile );
            std::string err_str = "couldn't initialize SSL context";
            ssl_build_error_string( err_str );
            if ( ( result = ASSERT_ERROR( ctx, SSL_INIT_ERROR, err_str.c_str() ) ).ok() ) {

                int status = ssl_load_hd_params( ctx, env.irodsSSLDHParamsFile );
                std::string err_str = "error setting Diffie-Hellman parameters";
                ssl_build_error_string( err_str );
                if ( !( result = ASSERT_ERROR( status >= 0, SSL_INIT_ERROR, err_str.c_str() ) ).ok() ) {
                    SSL_CTX_free( ctx );
                }
                else {

                    SSL* ssl = ssl_init_socket( ctx, ssl_obj->socket_handle() );
                    std::string err_str = "couldn't initialize SSL socket";
                    ssl_build_error_string( err_str );
                    if ( !( result = ASSERT_ERROR( ssl, SSL_INIT_ERROR, err_str.c_str() ) ).ok() ) {
                        SSL_CTX_free( ctx );
                    }
                    else {

                        status = SSL_accept( ssl );
                        std::string err_str = "error calling SSL_accept";
                        ssl_build_error_string( err_str );
                        if ( ( result = ASSERT_ERROR( status >= 1, SSL_HANDSHAKE_ERROR, err_str.c_str() ) ).ok() ) {

                            ssl_obj->ssl( ssl );
                            ssl_obj->ssl_ctx( ctx );

                            rodsLog( LOG_DEBUG, "sslAccept: accepted SSL connection" );

                            // =-=-=-=-=-=-=-
                            // message header variables
                            struct timeval tv;
                            tv.tv_sec = READ_VERSION_TOUT_SEC;
                            tv.tv_usec = 0;
                            msgHeader_t msg_header;

                            // =-=-=-=-=-=-=-
                            // wait for a message header containing the encryption environment
                            bzero( &msg_header, sizeof( msg_header ) );
                            ret = readMsgHeader( ssl_obj, &msg_header, &tv );
                            if ( ( result = ASSERT_PASS( ret, "Read message header failed." ) ).ok() ) {

                                // =-=-=-=-=-=-=-
                                // set encryption parameters
                                ssl_obj->key_size( msg_header.msgLen );
                                ssl_obj->salt_size( msg_header.errorLen );
                                ssl_obj->num_hash_rounds( msg_header.bsLen );
                                ssl_obj->encryption_algorithm( msg_header.type );

                                // =-=-=-=-=-=-=-
                                // wait for a message header containing a shared secret
                                bzero( &msg_header, sizeof( msg_header ) );
                                ret = readMsgHeader( ssl_obj, &msg_header, &tv );
                                if ( ( result = ASSERT_PASS( ret, "Read message header failed." ) ).ok() ) {

                                    // =-=-=-=-=-=-=-
                                    // call inteface to read message body
                                    bytesBuf_t msg_buf;
                                    ret = readMsgBody( ssl_obj, &msg_header, &msg_buf, 0, 0, XML_PROT, NULL );
                                    if ( ( result = ASSERT_PASS( ret, "Read message body failed." ) ).ok() ) {

                                        // =-=-=-=-=-=-=-
                                        // we cannot check to see if the key property has been set,
                                        // as the resource servers connect to the icat and init the
                                        // the key first, so we need to repave it with the client
                                        // connection.  leaving this here for debugging if necessary
                                        //std::string key;
                                        //ret = _ctx.prop_map().get< std::string >( SHARED_KEY, key );
                                        //if( ret.ok() ) {
                                        //    std::stringstream msg;
                                        //    return ERROR( -1, "shared secret already exists" );
                                        //}

                                        // =-=-=-=-=-=-=-
                                        // set the incoming shared secret
                                        unsigned char* secret_ptr = static_cast< unsigned char* >( msg_buf.buf );
                                        irods::buffer_crypt::array_t key;
                                        key.assign(
                                            secret_ptr,
                                            &secret_ptr[ msg_buf.len ] );

                                        ssl_obj->shared_secret( key );
                                        ret = _ctx.prop_map().set< irods::buffer_crypt::array_t >( SHARED_KEY, key );
                                        result = ASSERT_PASS( ret, "Shared key property not found." );
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        return result;

    } // ssl_agent_start

    // =-=-=-=-=-=-=-
    //
    irods::error ssl_agent_stop(
        irods::plugin_context& _ctx ) {
        irods::error result = SUCCESS();

        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid< irods::ssl_object >();
        if ( ( result = ASSERT_PASS( ret, "Invalid SSL plugin context." ) ).ok() ) {

            // =-=-=-=-=-=-=-
            // extract the useful bits from the context
            irods::ssl_object_ptr ssl_obj = boost::dynamic_pointer_cast< irods::ssl_object >( _ctx.fco() );
            SSL*     ssl = ssl_obj->ssl();
            SSL_CTX* ctx = ssl_obj->ssl_ctx();

            // =-=-=-=-=-=-=-
            // shut down the SSL connection. Might need to call SSL_shutdown()
            // twice to allow the protocol to notify and then complete
            // the shutdown.
            int status = SSL_shutdown( ssl_obj->ssl() );
            if ( status == 0 ) {
                // =-=-=-=-=-=-=-
                // second phase of shutdown
                status = SSL_shutdown( ssl_obj->ssl() );
            }
            std::string err_str = "error completing shutdown of SSL connection";
            ssl_build_error_string( err_str );
            if ( ( result = ASSERT_ERROR( status == 1, SSL_SHUTDOWN_ERROR, err_str.c_str() ) ).ok() ) {

                // =-=-=-=-=-=-=-
                // clean up the SSL state
                SSL_free( ssl );
                SSL_CTX_free( ctx );
                ssl_obj->ssl( 0 );
                ssl_obj->ssl_ctx( 0 );

                rodsLog( LOG_DEBUG, "sslShutdown: shut down SSL connection" );
            }
        }

        return result;

    } // ssl_agent_stop

    // =-=-=-=-=-=-=-
    //
    irods::error ssl_write_msg_header(
        irods::plugin_context& _ctx,
        bytesBuf_t*             _header ) {
        irods::error result = SUCCESS();

        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid< irods::ssl_object >();
        if ( ( result = ASSERT_PASS( ret, "Invalid SSL plugin context." ) ).ok() ) {

            // =-=-=-=-=-=-=-
            // log debug information if appropriate
            if ( getRodsLogLevel() >= LOG_DEBUG3 ) {
                printf( "sending header: len = %d\n%s\n", _header->len, ( char * ) _header->buf );
            }

            // =-=-=-=-=-=-=-
            // extract the useful bits from the context
            irods::ssl_object_ptr ssl_obj = boost::dynamic_pointer_cast< irods::ssl_object >( _ctx.fco() );

            // =-=-=-=-=-=-=-
            // convert host byte order to network byte order
            int header_length = htonl( _header->len );

            // =-=-=-=-=-=-=-
            // write the length of the header to the socket
            int bytes_written = 0;
            ret = ssl_socket_write( &header_length, sizeof( header_length ), bytes_written, ssl_obj->ssl() );
            int status = SYS_HEADER_WRITE_LEN_ERR - errno;
            if ( ( result = ASSERT_ERROR( ret.ok() && bytes_written == sizeof( header_length ), status, "Wrote %d expected %d.",
                                          bytes_written, header_length ) ).ok() ) {

                // =-=-=-=-=-=-=-
                // now send the actual header
                ret = ssl_socket_write( _header->buf, _header->len, bytes_written, ssl_obj->ssl() );
                status = SYS_HEADER_WRITE_LEN_ERR - errno;
                result = ASSERT_ERROR( ret.ok() && bytes_written == _header->len, status, "Wrote %d expected %d.",
                                       bytes_written, _header->len );
            }
        }

        return result;

    } // ssl_write_msg_header

    // =-=-=-=-=-=-=-
    //
    irods::error ssl_send_rods_msg(
        irods::plugin_context& _ctx,
        char*                   _msg_type,
        bytesBuf_t*             _msg_buf,
        bytesBuf_t*             _stream_bbuf,
        bytesBuf_t*             _error_buf,
        int                     _int_info,
        irodsProt_t             _protocol ) {
        irods::error result = SUCCESS();

        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid< irods::ssl_object >();
        if ( ( result = ASSERT_PASS( ret, "Invalid SSL plugin context." ) ).ok() ) {

            // =-=-=-=-=-=-=-
            // check the params
            if ( ( result = ASSERT_ERROR( _msg_type, SYS_INVALID_INPUT_PARAM, "Null msg type." ) ).ok() ) {

                // =-=-=-=-=-=-=-
                // extract the useful bits from the context
                irods::ssl_object_ptr ssl_obj = boost::dynamic_pointer_cast< irods::ssl_object >( _ctx.fco() );

                // =-=-=-=-=-=-=-
                // initialize a new header
                msgHeader_t msg_header;
                memset( &msg_header, 0, sizeof( msg_header ) );

                snprintf( msg_header.type, HEADER_TYPE_LEN, "%s", _msg_type );
                msg_header.intInfo = _int_info;

                // =-=-=-=-=-=-=-
                // initialize buffer lengths
                if ( _msg_buf ) {
                    msg_header.msgLen = _msg_buf->len;
                }
                if ( _stream_bbuf ) {
                    msg_header.bsLen = _stream_bbuf->len;
                }
                if ( _error_buf ) {
                    msg_header.errorLen = _error_buf->len;
                }

                // =-=-=-=-=-=-=-
                // send the header
                irods::network_object_ptr net_obj = boost::dynamic_pointer_cast< irods::network_object >( _ctx.fco() );
                ret = writeMsgHeader( net_obj, &msg_header );
                if ( ( result = ASSERT_PASS( ret, "Write message header failed." ) ).ok() ) {

                    // =-=-=-=-=-=-=-
                    // send the message buffer
                    int bytes_written = 0;
                    if ( msg_header.msgLen > 0 ) {
                        if ( XML_PROT == _protocol &&
                                getRodsLogLevel() >= LOG_DEBUG3 ) {
                            printf( "sending msg: \n%s\n", ( char* ) _msg_buf->buf );
                        }
                        ret = ssl_socket_write( _msg_buf->buf, _msg_buf->len, bytes_written, ssl_obj->ssl() );
                        result = ASSERT_PASS( ret, "Failed writing SSL message to socket." );
                    } // if msgLen > 0

                    if ( result.ok() ) {

                        // =-=-=-=-=-=-=-
                        // send the error buffer
                        if ( msg_header.errorLen > 0 ) {
                            if ( XML_PROT == _protocol &&
                                    getRodsLogLevel() >= LOG_DEBUG3 ) {
                                printf( "sending msg: \n%s\n", ( char* ) _error_buf->buf );

                            }

                            ret = ssl_socket_write( _error_buf->buf, _error_buf->len, bytes_written, ssl_obj->ssl() );
                            result = ASSERT_PASS( ret, "Failed writing SSL message to socket." );
                        } // if errorLen > 0

                        if ( result.ok() ) {

                            // =-=-=-=-=-=-=-
                            // send the stream buffer
                            if ( msg_header.bsLen > 0 ) {
                                if ( XML_PROT == _protocol &&
                                        getRodsLogLevel() >= LOG_DEBUG3 ) {
                                    printf( "sending msg: \n%s\n", ( char* ) _stream_bbuf->buf );
                                }

                                ret = ssl_socket_write( _stream_bbuf->buf, _stream_bbuf->len, bytes_written, ssl_obj->ssl() );
                                result = ASSERT_PASS( ret, "Failed writing SSL message to socket." );

                            } // if bsLen > 0
                        }
                    }
                }
            }
        }

        return result;

    } // ssl_send_rods_msg

    // =-=-=-=-=-=-=-
    // helper fcn to read a bytes buf
    irods::error read_bytes_buf(
        int             _socket_handle,
        int             _length,
        bytesBuf_t*     _buffer,
        irodsProt_t     _protocol,
        struct timeval* _time_val,
        SSL*            _ssl ) {
        irods::error result = SUCCESS();

        // =-=-=-=-=-=-=-
        // trap input buffer ptr
        if ( ( result = ASSERT_ERROR( _buffer, SYS_READ_MSG_BODY_INPUT_ERR, "Null buffer pointer." ) ).ok() ) {

            // =-=-=-=-=-=-=-
            // read buffer
            int bytes_read = 0;
            irods::error ret = ssl_socket_read( _socket_handle, _buffer->buf, _length, bytes_read, _time_val, _ssl );
            _buffer->len = bytes_read;

            // =-=-=-=-=-=-=-
            // log transaction if requested
            if ( _protocol == XML_PROT &&
                    getRodsLogLevel() >= LOG_DEBUG3 ) {
                printf( "received msg: \n%s\n",
                        ( char* ) _buffer->buf );
            }

            // =-=-=-=-=-=-=-
            // trap failed read
            if ( !( result = ASSERT_ERROR( ret.ok() && bytes_read == _length, SYS_READ_MSG_BODY_LEN_ERR,
                                           "Read %d expected %d.", bytes_read, _length ) ).ok() ) {
                free( _buffer->buf );
            }
        }

        return result;

    } // read_bytes_buf

    // =-=-=-=-=-=-=-
    // read a message body off of the socket
    irods::error ssl_read_msg_body(
        irods::plugin_context& _ctx,
        msgHeader_t*            _header,
        bytesBuf_t*             _input_struct_buf,
        bytesBuf_t*             _bs_buf,
        bytesBuf_t*             _error_buf,
        irodsProt_t             _protocol,
        struct timeval*         _time_val ) {
        irods::error result = SUCCESS();

        // =-=-=-=-=-=-=-
        // check the context
        irods::error ret = _ctx.valid< irods::ssl_object >();
        if ( ( result = ASSERT_PASS( ret, "Invalid SSL plugin context." ) ).ok() ) {

            // =-=-=-=-=-=-=-
            // extract the useful bits from the context
            irods::ssl_object_ptr ssl_obj = boost::dynamic_pointer_cast< irods::ssl_object >( _ctx.fco() );
            int socket_handle = ssl_obj->socket_handle();

            // =-=-=-=-=-=-=-
            // trap header ptr
            if ( ( result = ASSERT_ERROR( _header, SYS_READ_MSG_BODY_INPUT_ERR, "Null header pointer." ) ).ok() ) {

                // =-=-=-=-=-=-=-
                // reset error buf - assumed by the client code
                // NOTE :: do not reset bs buf as it can be reused
                //         on the client side
                if ( _error_buf ) {
                    memset( _error_buf, 0, sizeof( bytesBuf_t ) );
                }

                // =-=-=-=-=-=-=-
                // read input buffer
                if ( 0 != _input_struct_buf ) {
                    if ( _header->msgLen > 0 ) {
                        _input_struct_buf->buf = malloc( _header->msgLen + 1 );
                        ret = read_bytes_buf( socket_handle, _header->msgLen, _input_struct_buf, _protocol, _time_val, ssl_obj->ssl() );
                        result = ASSERT_PASS( ret, "Failed reading from SSL buffer." );

                    }
                    else {
                        // =-=-=-=-=-=-=-
                        // ensure msg len is 0 as this can cause issues
                        // in the agent
                        _input_struct_buf->len = 0;

                    }

                } // input buffer

                if ( result.ok() ) {

                    // =-=-=-=-=-=-=-
                    // read error buffer
                    if ( 0 != _error_buf ) {
                        if ( _header->errorLen > 0 ) {
                            _error_buf->buf = malloc( _header->errorLen + 1 );
                            ret = read_bytes_buf( socket_handle, _header->errorLen, _error_buf, _protocol, _time_val, ssl_obj->ssl() );
                            result = ASSERT_PASS( ret, "Failed reading from SSL buffer." );

                        }
                        else {
                            _error_buf->len = 0;

                        }

                    } // error buffer

                    if ( result.ok() ) {

                        // =-=-=-=-=-=-=-
                        // read bs buffer
                        if ( 0 != _bs_buf ) {
                            if ( _header->bsLen > 0 ) {
                                // do not repave bs buf as it can be
                                // reused by the client
                                if ( _bs_buf->buf == NULL ) {
                                    _bs_buf->buf = malloc( _header->bsLen + 1 );

                                }
                                else if ( _header->bsLen > _bs_buf->len ) {
                                    free( _bs_buf->buf );
                                    _bs_buf->buf = malloc( _header->bsLen + 1 );

                                }

                                ret = read_bytes_buf( socket_handle, _header->bsLen, _bs_buf, _protocol, _time_val, ssl_obj->ssl() );
                                result = ASSERT_PASS( ret, "Failed reading from SSL buffer." );
                            }
                            else {
                                _bs_buf->len = 0;

                            }

                        } // bs buffer
                    }
                }
            }
        }

        return result;

    } // ssl_read_msg_body

    // =-=-=-=-=-=-=-
    // stub for ops that the ssl plug does
    // not need to support - accept etc
    irods::error ssl_success_stub(
        irods::plugin_context& ) {
        return SUCCESS();

    } // ssl_success_stub


    // =-=-=-=-=-=-=-
    // derive a new ssl network plugin from
    // the network plugin base class for handling
    // ssl communications
    class ssl_network_plugin : public irods::network {
        public:
            ssl_network_plugin(
                const std::string& _nm,
                const std::string& _ctx ) :
                irods::network(
                    _nm,
                    _ctx ) {
            } // ctor

            ~ssl_network_plugin() {
            }

    }; // class ssl_network_plugin



    // =-=-=-=-=-=-=-
    // factory function to provide instance of the plugin
    irods::network* plugin_factory(
        const std::string& _inst_name,
        const std::string& _context ) {
        // =-=-=-=-=-=-=-
        // create a ssl network object
        ssl_network_plugin* ssl = new ssl_network_plugin(
            _inst_name,
            _context );

        // =-=-=-=-=-=-=-
        // fill in the operation table mapping call
        // names to function names
        ssl->add_operation( irods::NETWORK_OP_CLIENT_START, "ssl_client_start" );
        ssl->add_operation( irods::NETWORK_OP_CLIENT_STOP,  "ssl_client_stop" );
        ssl->add_operation( irods::NETWORK_OP_AGENT_START,  "ssl_agent_start" );
        ssl->add_operation( irods::NETWORK_OP_AGENT_STOP,   "ssl_agent_stop" );
        ssl->add_operation( irods::NETWORK_OP_READ_HEADER,  "ssl_read_msg_header" );
        ssl->add_operation( irods::NETWORK_OP_READ_BODY,    "ssl_read_msg_body" );
        ssl->add_operation( irods::NETWORK_OP_WRITE_HEADER, "ssl_write_msg_header" );
        ssl->add_operation( irods::NETWORK_OP_WRITE_BODY,   "ssl_send_rods_msg" );

        irods::network* net = dynamic_cast< irods::network* >( ssl );

        return net;

    } // plugin_factory

}; // extern "C"
