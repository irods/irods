#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_buffer_encryption.hpp"
#include "irods/irods_configuration_keywords.hpp"
#include "irods/irods_network_constants.hpp"
#include "irods/irods_network_plugin.hpp"
#include "irods/irods_server_properties.hpp"
#include "irods/irods_ssl_object.hpp"
#include "irods/irods_stacktrace.hpp"
#include "irods/msParam.h"
#include "irods/rcConnect.h"
#include "irods/rcMisc.h"
#include "irods/rodsDef.h"
#include "irods/sockComm.h"
#include "irods/sockCommNetworkInterface.hpp"

#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>
#include <iostream>

#include <openssl/decoder.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>

#include <fmt/format.h>

#define SSL_CIPHER_LIST "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH"

// =-=-=-=-=-=-=-
// key for ssl shared secret property
const std::string SHARED_KEY( "ssl_network_plugin_shared_key" );

static void ssl_log_error(
    const char *msg ) {
    unsigned long err;
    char buf[512];

    while ( ( err = ERR_get_error() ) ) {
        ERR_error_string_n( err, buf, 512 );
        rodsLog( LOG_ERROR, "%s. SSL error: %s", msg, buf );
    }

} // ssl_log_error

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

static auto ssl_load_dh_params(SSL_CTX* ctx, const char* file) -> int
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
} // ssl_load_dh_params

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
static SSL_CTX* ssl_init_context(const char* certfile, const char* keyfile)
{
    SSL_CTX *ctx = 0;
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
    const char *ca_path = strcmp( env.irodsSSLCACertificatePath, "" ) ? env.irodsSSLCACertificatePath : NULL;
    const char *ca_file = strcmp( env.irodsSSLCACertificateFile, "" ) ? env.irodsSSLCACertificateFile : NULL;
    if ( ca_path || ca_file ) {
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

    rodsEnv env;
    int status = getRodsEnv( &env );
    if ( status < 0 ) {
        rodsLog(
            LOG_ERROR,
            "ssl_init_context - failed in getRodsEnv : %d",
            status );
        return 0;

    }

    char *verify_server = env.irodsSSLVerifyServer;
    if ( strlen( verify_server ) > 0 && strcmp( verify_server, "hostname" ) ) {
        /* not being asked to verify that the peer hostname
           is in the certificate. */
        return 1;
    }

    X509* cert = SSL_get1_peer_certificate(ssl);
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
    bool match = false;
    auto* names = static_cast<STACK_OF(GENERAL_NAME)*>(X509_get_ext_d2i( cert, NID_subject_alt_name, NULL, NULL ));
    int num_names = sk_GENERAL_NAME_num( names );
    for ( int i = 0; i < num_names; i++ ) {
        auto* name = sk_GENERAL_NAME_value( names, i );
        if ( name && name->type == GEN_DNS ) {
            if ( !strcasecmp( reinterpret_cast<const char*>(ASN1_STRING_get0_data( name->d.dNSName )), peer ) ) {
                match = true;
                break;
            }
        }
    }
    sk_GENERAL_NAME_free( names );

    /* if no match above, check the common name in the certificate */
    char name_text[256];
    if ( !match &&
            ( X509_NAME_get_text_by_NID( X509_get_subject_name( cert ),
                                         NID_commonName, name_text, sizeof( name_text ) ) != -1 ) ) {
        if ( !strcasecmp( name_text, peer ) ) {
            match = true;
        }
        else if ( name_text[0] == '*' ) { /* wildcard domain */
            const char *tmp = strchr( peer, '.' );
            if ( tmp && !strcasecmp( tmp, name_text + 1 ) ) {
                match = true;
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

// local function to read a buffer from a socket
irods::error ssl_socket_read(
    int             _socket,
    void*           _buffer,
    int             _length,
    int&            _bytes_read,
    struct timeval* _time_value,
    SSL*            _ssl)
{
    // check incoming pointers
    if (!_buffer || !_ssl) {
        return ERROR(SYS_INVALID_INPUT_PARAM, "Null buffer or ssl pointer.");
    }

    fd_set set;
    // clang-format off
    struct timeval timeout{};
    // clang-format on
    int len_to_read = _length;
    char* read_ptr = static_cast<char*>(_buffer);
    _bytes_read = 0;
    irods::error result = SUCCESS();

    // loop while there is data to read
    while (result.ok() && len_to_read > 0) {
        // do a time out managed select of the socket fd
        if (SSL_pending(_ssl) == 0 && _time_value) {
            FD_ZERO(&set); // NOLINT
            FD_SET(_socket, &set); // NOLINT
            timeout = *_time_value;

            int status = select(_socket + 1, &set, nullptr, nullptr, &timeout);

            if ( status == 0 ) {
                // the select has timed out
                if ( ( _length - len_to_read ) > 0 ) {
                    result = ERROR(_length - len_to_read, "failed to read requested number of bytes");
                }
                else {
                    result = ERROR(SYS_SOCK_READ_TIMEDOUT, "socket timeout error");
                }
            }
            else if ( status < 0 ) {
                // keep trying on interrupt or just error out
                if (EINTR == errno) {
                    return ERROR(INTERRUPT_DETECTED, fmt::format("{} interrupted by signal", __func__));
                }

                int err_status = SYS_SOCK_READ_ERR - errno;
                return ERROR(err_status, "Error on select.");
            } // else
        } // if tv

        // select has been done, finally do the read
        int num_bytes = SSL_read(_ssl, (void*) read_ptr, len_to_read);

        // error trapping the read
        if ( SSL_get_error( _ssl, num_bytes ) != SSL_ERROR_NONE ) {
            // gracefully handle an interrupt
            if ( EINTR == errno ) {
                errno     = 0;
                num_bytes = 0;
            }
            else {
                result = ERROR( _length - len_to_read, "Failed to in SSL read." );
            }
        }

        // all has gone well, do byte book keeping
        len_to_read -= num_bytes;
        read_ptr += num_bytes; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        _bytes_read += num_bytes;
    } // while

    // and were done? report length not read
    // return CODE( _length - len_to_read );
    return result;
} // ssl_socket_read

// local function to write a buffer to a socket
irods::error ssl_socket_write(
    const void* _buffer,
    int         _length,
    int&        _bytes_written,
    SSL*        _ssl)
{
    // check incoming pointers
    if (!_buffer || !_ssl) {
        return ERROR(SYS_INVALID_INPUT_PARAM, "Buffer or ssl pointer are null.");
    }

    // local variables for write
    int   len_to_write = _length;
    const char* write_ptr    = static_cast<const char*>( _buffer );

    // reset bytes written
    _bytes_written = 0;

    // loop while there is data to read
    irods::error result = SUCCESS();
    while ( result.ok() && len_to_write > 0 ) {
        int num_bytes = SSL_write( _ssl, static_cast<const void*>( write_ptr ), len_to_write );

        // error trapping the write
        if ( SSL_get_error( _ssl, num_bytes ) != SSL_ERROR_NONE ) {
            // gracefully handle an interrupt
            if ( errno == EINTR ) {
                errno     = 0;
                num_bytes = 0;
            }
            else {
                result = ERROR( _length - len_to_write, "Failed to write to SSL" );
            }
        }

        // increment working variables
        len_to_write   -= num_bytes;
        write_ptr      += num_bytes;
        _bytes_written += num_bytes;
    }

    // and were done? report length not written
    // return CODE( _length - len_to_write );
    return result;
} // ssl_socket_write

irods::error ssl_read_msg_header(irods::plugin_context& _ctx,
                                 void*                  _buffer,
                                 struct timeval*        _time_val)
{
    if (const auto ret = _ctx.valid< irods::ssl_object >(); !ret.ok()) {
        return PASSMSG("Invalid SSL plugin context.", ret);
    }

    // =-=-=-=-=-=-=-
    // extract the useful bits from the context
    irods::ssl_object_ptr ssl_obj = boost::dynamic_pointer_cast< irods::ssl_object >( _ctx.fco() );
    int socket_handle = ssl_obj->socket_handle();

    // =-=-=-=-=-=-=-
    // read the header length packet */
    int header_length = 0;
    int bytes_read    = 0;
    auto ret = ssl_socket_read( socket_handle, static_cast<void*>( &header_length ), sizeof( int ), bytes_read,
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

    // convert from network to host byte order
    header_length = ntohl( header_length );

    // check head length against expected size range
    if (!(header_length <= MAX_NAME_LEN && header_length > 0)) {
        return ERROR(SYS_HEADER_READ_LEN_ERR, fmt::format(
                     "Header length is out of range: {} expected >= 0 and < {}.",
                     header_length, MAX_NAME_LEN));
    }

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

    // log debug information if appropriate
    if ( getRodsLogLevel() >= LOG_DEBUG8 ) {
        printf( "received header: len = %d\n%s\n",
                header_length,
                static_cast<char*>( _buffer ) );
    }

    return SUCCESS();
} // ssl_read_msg_header

irods::error ssl_client_stop(irods::plugin_context& _ctx,
                             rodsEnv*               _env)
{
    // check the context
    if (const auto ret = _ctx.valid<irods::ssl_object>(); !ret.ok()) {
        return PASSMSG("Invalid SSL plugin context.", ret);
    }

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
    if (status != 1) {
        return ERROR(SSL_SHUTDOWN_ERROR, err_str.c_str());
    }

    /* clean up the SSL state */
    SSL_free( ssl );
    SSL_CTX_free( ctx );

    ssl_obj->ssl( 0 );
    ssl_obj->ssl_ctx( 0 );

    return SUCCESS();

} // ssl_client_stop

irods::error ssl_client_start(irods::plugin_context& _ctx,
                              rodsEnv*               _env)
{
    // check the context
    if (const auto ret = _ctx.valid<irods::ssl_object>(); !ret.ok()) {
        return PASSMSG("Invalid SSL plugin context.", ret);
    }

    // extract the useful bits from the context
    irods::ssl_object_ptr ssl_obj = boost::dynamic_pointer_cast< irods::ssl_object >( _ctx.fco() );

    // set up SSL on our side of the socket
    SSL_CTX* ctx = ssl_init_context( NULL, NULL );
    if (!ctx) {
        std::string err_str = "failed to initialize SSL context";
        ssl_build_error_string( err_str );
        return ERROR(SSL_INIT_ERROR, err_str.c_str());
    }

    SSL* ssl = ssl_init_socket( ctx, ssl_obj->socket_handle() );
    if (!ssl) {
        std::string err_str = "couldn't initialize SSL socket";
        ssl_build_error_string( err_str );
        SSL_CTX_free( ctx );
        return ERROR(SSL_INIT_ERROR, err_str.c_str());
    }

    if (const auto ec = SSL_set_tlsext_host_name(ssl, ssl_obj->host().c_str()); ec != 1) {
        std::string err_str = "error in SSL_set_tlsext_host_name";
        ssl_build_error_string( err_str );
        SSL_free( ssl );
        SSL_CTX_free( ctx );
        return ERROR(SSL_INIT_ERROR, err_str.c_str());
    }

    if (const auto ec = SSL_connect(ssl); ec < 1) {
        std::string err_str = "error in SSL_connect";
        ssl_build_error_string( err_str );
        SSL_free( ssl );
        SSL_CTX_free( ctx );
        return ERROR(SSL_HANDSHAKE_ERROR, err_str.c_str());
    }

    ssl_obj->ssl( ssl );
    ssl_obj->ssl_ctx( ctx );

    if (const auto ec = ssl_post_connection_check(ssl, ssl_obj->host().c_str()); !ec) {
        std::string err_str = "post connection certificate check failed";
        ssl_build_error_string( err_str );
        ssl_client_stop( _ctx, _env );
        return ERROR(SSL_CERT_ERROR, err_str.c_str());
    }

    // check to see if a key has already been placed
    // in the property map
    irods::buffer_crypt::array_t key;
    if (const auto err = _ctx.prop_map().get<irods::buffer_crypt::array_t>(SHARED_KEY, key); !err.ok()) {
        // if no key exists then ship a new key and set the property
        if (const auto ret = irods::buffer_crypt::generate_key(key, _env->rodsEncryptionKeySize); !ret.ok()) {
            irods::log(PASS(ret));
        }

        if (const auto ret = _ctx.prop_map().set<irods::buffer_crypt::array_t>(SHARED_KEY, key); !ret.ok()) {
            irods::log(PASS(ret));
        }
    }

    if (!_ctx.prop_map().has_entry(SHARED_KEY)) {
        return ERROR(KEY_NOT_FOUND, "irodsEncryption error. Failed to generate key.");
    }

    // send a message to the agent containing the client
    // size encryption environment variables
    msgHeader_t msg_header;
    memset( &msg_header, 0, sizeof( msg_header ) );
    memcpy( msg_header.type, _env->rodsEncryptionAlgorithm, HEADER_TYPE_LEN );
    msg_header.msgLen   = _env->rodsEncryptionKeySize;
    msg_header.errorLen = _env->rodsEncryptionSaltSize;
    msg_header.bsLen    = _env->rodsEncryptionNumHashRounds;

    // error check the encryption environment
    if (!(0 != msg_header.msgLen && 0 != msg_header.errorLen && 0 != msg_header.bsLen)) {
        return ERROR(-1, "irodsEncryption error. Key size, salt size or num hash rounds is 0.");
    }

    if (!EVP_get_cipherbyname(msg_header.type)) {
        return ERROR(-1, fmt::format("irods_encryption_algorithm \"{}\" is invalid.", msg_header.type));
    }

    // use a message header to contain the encryption environment
    if (const auto err = writeMsgHeader(ssl_obj, &msg_header); !err.ok()) {
        return PASSMSG("writeMsgHeader failed.", err);
    }

    // send a message to the agent containing the shared secret
    bytesBuf_t key_bbuf;
    key_bbuf.len = key.size();
    key_bbuf.buf = &key[0];
    char msg_type[] = { "SHARED_SECRET" };
    if (const auto err = sendRodsMsg(ssl_obj, msg_type, &key_bbuf, 0, 0, 0, XML_PROT); !err.ok()) {
        return PASSMSG("writeMsgHeader failed.", err);
    }

    // set the key and env for this ssl object
    ssl_obj->shared_secret( key );
    ssl_obj->key_size( _env->rodsEncryptionKeySize );
    ssl_obj->salt_size( _env->rodsEncryptionSaltSize );
    ssl_obj->num_hash_rounds( _env->rodsEncryptionNumHashRounds );
    ssl_obj->encryption_algorithm( _env->rodsEncryptionAlgorithm );

    return SUCCESS();
} // ssl_client_start

irods::error ssl_agent_start(irods::plugin_context& _ctx)
{
    // check the context
    if (const auto err = _ctx.valid<irods::ssl_object>(); !err.ok()) {
        return PASSMSG("Invalid SSL plugin context.", err);
    }

    // extract the useful bits from the context
    irods::ssl_object_ptr ssl_obj = boost::dynamic_pointer_cast< irods::ssl_object >( _ctx.fco() );

    SSL_CTX* ctx{};
    try {
        const auto tls_config = irods::get_server_property<nlohmann::json>(irods::KW_CFG_TLS_SERVER);

        const auto& certificate_chain_file =
            tls_config.at(irods::KW_CFG_TLS_CERTIFICATE_CHAIN_FILE).get_ref<const std::string&>();
        const auto& certificate_key_file =
            tls_config.at(irods::KW_CFG_TLS_CERTIFICATE_KEY_FILE).get_ref<const std::string&>();
        ctx = ssl_init_context(certificate_chain_file.c_str(), certificate_key_file.c_str());
        if (!ctx) {
            std::string err_str = "couldn't initialize SSL context";
            ssl_build_error_string(err_str);
            return ERROR(SSL_INIT_ERROR, err_str);
        }

        const auto& dh_params_file = tls_config.at(irods::KW_CFG_TLS_DH_PARAMS_FILE).get_ref<const std::string&>();
        if (const auto ec = ssl_load_dh_params(ctx, dh_params_file.c_str()); ec < 0) {
            std::string err_str = "error setting Diffie-Hellman parameters";
            ssl_build_error_string(err_str);
            SSL_CTX_free(ctx);
            return ERROR(SSL_INIT_ERROR, err_str);
        }
    }
    catch (const irods::exception& e) {
        return ERROR(CONFIGURATION_ERROR, fmt::format("Error getting TLS configuration: {}", e.client_display_what()));
    }
    catch (const std::exception& e) {
        SSL_CTX_free(ctx);
        return ERROR(CONFIGURATION_ERROR, fmt::format("Error occurred getting TLS configuration: {}", e.what()));
    }

    SSL* ssl = ssl_init_socket( ctx, ssl_obj->socket_handle() );
    if (!ssl) {
        std::string err_str = "couldn't initialize SSL socket";
        ssl_build_error_string( err_str );
        SSL_CTX_free( ctx );
        return ERROR(SSL_INIT_ERROR, err_str.c_str());
    }

    if (const auto ec = SSL_accept(ssl); ec < 1) {
        std::string err_str = "error calling SSL_accept";
        ssl_build_error_string( err_str );
        return ERROR(SSL_HANDSHAKE_ERROR, err_str.c_str());
    }

    ssl_obj->ssl( ssl );
    ssl_obj->ssl_ctx( ctx );

    rodsLog( LOG_DEBUG, "sslAccept: accepted SSL connection" );

    // message header variables
    struct timeval tv;
    tv.tv_sec = READ_VERSION_TOUT_SEC;
    tv.tv_usec = 0;
    msgHeader_t msg_header;

    // wait for a message header containing the encryption environment
    std::memset(&msg_header, 0, sizeof(msg_header));
    if (const auto err = readMsgHeader(ssl_obj, &msg_header, &tv); !err.ok()) {
        return PASSMSG("Read message header failed.", err);
    }

    // set encryption parameters
    ssl_obj->key_size( msg_header.msgLen );
    ssl_obj->salt_size( msg_header.errorLen );
    ssl_obj->num_hash_rounds( msg_header.bsLen );
    ssl_obj->encryption_algorithm( msg_header.type );

    // wait for a message header containing a shared secret
    std::memset(&msg_header, 0, sizeof(msg_header));
    if (const auto err = readMsgHeader(ssl_obj, &msg_header, &tv); !err.ok()) {
        return PASSMSG("Read message header failed.", err);
    }

    // call interface to read message body
    bytesBuf_t msg_buf{};
    if (const auto err = readMsgBody(ssl_obj, &msg_header, &msg_buf, 0, 0, XML_PROT, NULL); !err.ok()) {
        return PASSMSG("Read message body failed.", err);
    }

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

    // set the incoming shared secret
    unsigned char* secret_ptr = static_cast< unsigned char* >( msg_buf.buf );
    irods::buffer_crypt::array_t key;
    key.assign( secret_ptr, &secret_ptr[ msg_buf.len ] );

    ssl_obj->shared_secret( key );
    if (const auto err = _ctx.prop_map().set<irods::buffer_crypt::array_t>(SHARED_KEY, key); !err.ok()) {
        return PASSMSG("Shared key property not found.", err);
    }

    return SUCCESS();
} // ssl_agent_start

irods::error ssl_agent_stop(irods::plugin_context& _ctx)
{
    // check the context
    if (const auto err = _ctx.valid<irods::ssl_object>(); !err.ok()) {
        return PASSMSG("Invalid SSL plugin context.", err);
    }

    // extract the useful bits from the context
    irods::ssl_object_ptr ssl_obj = boost::dynamic_pointer_cast< irods::ssl_object >( _ctx.fco() );
    SSL*     ssl = ssl_obj->ssl();
    SSL_CTX* ctx = ssl_obj->ssl_ctx();

    // shut down the SSL connection. Might need to call SSL_shutdown()
    // twice to allow the protocol to notify and then complete
    // the shutdown.
    if (auto ec = SSL_shutdown(ssl_obj->ssl()); ec != 1) {
        if (ec == 0) {
            // second phase of shutdown
            ec = SSL_shutdown(ssl_obj->ssl());
        }

        if (ec != 1) {
            std::string err_str = "error completing shutdown of SSL connection";
            ssl_build_error_string( err_str );
            return ERROR(SSL_SHUTDOWN_ERROR, err_str.c_str());
        }
    }

    // clean up the SSL state
    SSL_free( ssl );
    SSL_CTX_free( ctx );
    ssl_obj->ssl( 0 );
    ssl_obj->ssl_ctx( 0 );

    rodsLog( LOG_DEBUG, "sslShutdown: shut down SSL connection" );

    return SUCCESS();
} // ssl_agent_stop

irods::error ssl_write_msg_header(irods::plugin_context& _ctx,
                                  const bytesBuf_t*      _header)
{
    // check the context
    if (const auto err = _ctx.valid<irods::ssl_object>(); !err.ok()) {
        return PASSMSG("Invalid SSL plugin context.", err);
    }

    // log debug information if appropriate
    if ( getRodsLogLevel() >= LOG_DEBUG8 ) {
        printf( "sending header: len = %d\n%s\n", _header->len, ( const char * ) _header->buf );
    }

    // extract the useful bits from the context
    irods::ssl_object_ptr ssl_obj = boost::dynamic_pointer_cast< irods::ssl_object >( _ctx.fco() );

    // convert host byte order to network byte order
    int header_length = htonl( _header->len );

    // write the length of the header to the socket
    int bytes_written = 0;
    if (const auto err = ssl_socket_write(&header_length, sizeof(header_length), bytes_written, ssl_obj->ssl());
        !(err.ok() && bytes_written == sizeof(header_length))) {
        const auto ec = SYS_HEADER_WRITE_LEN_ERR - errno;
        return ERROR(ec, fmt::format("Wrote {} expected {}.", bytes_written, header_length));
    }

    // now send the actual header
    if (const auto err = ssl_socket_write(_header->buf, _header->len, bytes_written, ssl_obj->ssl());
        !(err.ok() && bytes_written == _header->len)) {
        const auto ec = SYS_HEADER_WRITE_LEN_ERR - errno;
        return ERROR(ec, fmt::format("Wrote {} expected {}.", bytes_written, _header->len));
    }

    return SUCCESS();
} // ssl_write_msg_header

irods::error ssl_send_rods_msg(
    irods::plugin_context& _ctx,
    const char*            _msg_type,
    const bytesBuf_t*      _msg_buf,
    const bytesBuf_t*      _stream_bbuf,
    const bytesBuf_t*      _error_buf,
    int                    _int_info,
    irodsProt_t            _protocol)
{
    // check the context
    if (const auto err = _ctx.valid<irods::ssl_object>(); !err.ok()) {
        return PASSMSG("Invalid SSL plugin context.", err);
    }

    // check the params
    if (!_msg_type) {
        return ERROR(SYS_INVALID_INPUT_PARAM, "Null msg type.");
    }

    // extract the useful bits from the context
    irods::ssl_object_ptr ssl_obj = boost::dynamic_pointer_cast< irods::ssl_object >( _ctx.fco() );

    // initialize a new header
    msgHeader_t msg_header;
    memset( &msg_header, 0, sizeof( msg_header ) );

    snprintf( msg_header.type, HEADER_TYPE_LEN, "%s", _msg_type );
    msg_header.intInfo = _int_info;

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

    // send the header
    irods::network_object_ptr net_obj = boost::dynamic_pointer_cast< irods::network_object >( _ctx.fco() );
    if (const auto err = writeMsgHeader(net_obj, &msg_header); !err.ok()) {
        return PASSMSG("Write message header failed.", err);
    }

    // send the message buffer
    int bytes_written = 0;
    if (_msg_buf && msg_header.msgLen > 0) {
        if (XML_PROT == _protocol && getRodsLogLevel() >= LOG_DEBUG8) {
            const auto* buf = static_cast<char*>(_msg_buf->buf);

            if (!may_contain_sensitive_data(buf, _msg_buf->len)) {
                std::printf("sending msg: \n%s\n", buf);
            }
        }

        if (const auto err = ssl_socket_write(_msg_buf->buf, _msg_buf->len, bytes_written, ssl_obj->ssl()); !err.ok()) {
            return PASSMSG("Failed writing SSL message to socket.", err);
        }
    } // if msgLen > 0

    // send the error buffer
    if (_error_buf && msg_header.errorLen > 0) {
        if (XML_PROT == _protocol && getRodsLogLevel() >= LOG_DEBUG8) {
            const auto* buf = static_cast<char*>(_error_buf->buf);

            if (!may_contain_sensitive_data(buf, _msg_buf->len)) {
                std::printf("sending msg: \n%s\n", buf);
            }
        }

        if (const auto err = ssl_socket_write(_error_buf->buf, _error_buf->len, bytes_written, ssl_obj->ssl()); !err.ok()) {
            return PASSMSG("Failed writing SSL message to socket.", err);
        }
    } // if errorLen > 0

    // send the stream buffer
    if (_stream_bbuf && msg_header.bsLen > 0) {
        if (XML_PROT == _protocol && getRodsLogLevel() >= LOG_DEBUG8) {
            const auto* buf = static_cast<char*>(_stream_bbuf->buf);

            if (!may_contain_sensitive_data(buf, _msg_buf->len)) {
                std::printf("sending msg: \n%s\n", buf);
            }
        }

        if (const auto err = ssl_socket_write(_stream_bbuf->buf, _stream_bbuf->len, bytes_written, ssl_obj->ssl()); !err.ok()) {
            return PASSMSG("Failed writing SSL message to socket.", err);
        }
    } // if bsLen > 0

    return SUCCESS();
} // ssl_send_rods_msg

// helper fcn to read a bytes buf
irods::error read_bytes_buf(
    int             _socket_handle,
    int             _length,
    bytesBuf_t*     _buffer,
    irodsProt_t     _protocol,
    struct timeval* _time_val,
    SSL*            _ssl)
{
    // trap input buffer ptr
    if (!_buffer) {
        return ERROR(SYS_READ_MSG_BODY_INPUT_ERR, "Null buffer pointer.");
    }

    // read buffer
    int bytes_read = 0;
    const irods::error err = ssl_socket_read(_socket_handle, _buffer->buf, _length, bytes_read, _time_val, _ssl);
    _buffer->len = bytes_read;

    // log transaction if requested
    if (_protocol == XML_PROT && getRodsLogLevel() >= LOG_DEBUG8) {
        const auto* buf = static_cast<char*>(_buffer->buf);

        if (!may_contain_sensitive_data(buf, _buffer->len)) {
            std::printf("received msg: \n%s\n", buf);
        }
    }

    // trap failed read
    if (!(err.ok() && bytes_read == _length)) {
        free(_buffer->buf);
        return ERROR(SYS_READ_MSG_BODY_LEN_ERR, fmt::format("Read {} expected {}.", bytes_read, _length));
    }

    return SUCCESS();
} // read_bytes_buf

// read a message body off of the socket
irods::error ssl_read_msg_body(
    irods::plugin_context& _ctx,
    msgHeader_t*            _header,
    bytesBuf_t*             _input_struct_buf,
    bytesBuf_t*             _bs_buf,
    bytesBuf_t*             _error_buf,
    irodsProt_t             _protocol,
    struct timeval*         _time_val)
{
    // check the context
    if (const auto err = _ctx.valid<irods::ssl_object>(); !err.ok()) {
        return PASSMSG("Invalid SSL plugin context.", err);
    }

    // extract the useful bits from the context
    irods::ssl_object_ptr ssl_obj = boost::dynamic_pointer_cast< irods::ssl_object >( _ctx.fco() );
    int socket_handle = ssl_obj->socket_handle();

    // trap header ptr
    if (!_header) {
        return ERROR(SYS_READ_MSG_BODY_INPUT_ERR, "Null header pointer.");
    }

    // reset error buf - assumed by the client code
    // NOTE :: do not reset bs buf as it can be reused
    //         on the client side
    if ( _error_buf ) {
        memset( _error_buf, 0, sizeof( bytesBuf_t ) );
    }

    // read input buffer
    if ( 0 != _input_struct_buf ) {
        if ( _header->msgLen > 0 ) {
            _input_struct_buf->buf = malloc( _header->msgLen + 1 );
            const auto ret = read_bytes_buf(socket_handle, _header->msgLen, _input_struct_buf, _protocol, _time_val, ssl_obj->ssl());
            if (!ret.ok()) {
                return PASSMSG("Failed reading from SSL buffer.", ret);
            }
        }
        else {
            // ensure msg len is 0 as this can cause issues in the agent
            _input_struct_buf->len = 0;
        }
    } // input buffer

    // read error buffer
    if ( 0 != _error_buf ) {
        if ( _header->errorLen > 0 ) {
            _error_buf->buf = malloc( _header->errorLen + 1 );
            const auto ret = read_bytes_buf(socket_handle, _header->errorLen, _error_buf, _protocol, _time_val, ssl_obj->ssl());
            if (!ret.ok()) {
                return PASSMSG("Failed reading from SSL buffer.", ret);
            }
        }
        else {
            _error_buf->len = 0;
        }
    } // error buffer

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

            const auto ret = read_bytes_buf(socket_handle, _header->bsLen, _bs_buf, _protocol, _time_val, ssl_obj->ssl());
            if (!ret.ok()) {
                return PASSMSG("Failed reading from SSL buffer.", ret);
            }
        }
        else {
            _bs_buf->len = 0;
        }
    } // bs buffer

    return SUCCESS();
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
extern "C"
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
    using namespace irods;
    using namespace std;
    ssl->add_operation(
        NETWORK_OP_CLIENT_START,
        function<error(plugin_context&,rodsEnv*)>(
            ssl_client_start ) );
    ssl->add_operation(
        NETWORK_OP_CLIENT_STOP,
        function<error(plugin_context&,rodsEnv*)>(
            ssl_client_stop ) );
    ssl->add_operation(
        NETWORK_OP_AGENT_START,
        function<error(plugin_context&)>(
            ssl_agent_start ) );
    ssl->add_operation(
        NETWORK_OP_AGENT_STOP,
        function<error(plugin_context&)>(
            ssl_agent_stop ) );
    ssl->add_operation(
        NETWORK_OP_READ_HEADER,
        function<error(plugin_context&,void*, struct timeval*)>(
            ssl_read_msg_header ) );
    ssl->add_operation(
        NETWORK_OP_READ_BODY,
        function<error(plugin_context&,msgHeader_t*,bytesBuf_t*,bytesBuf_t*,bytesBuf_t*,irodsProt_t,struct timeval*)>(
            ssl_read_msg_body ) );
    ssl->add_operation(
        NETWORK_OP_WRITE_HEADER,
        function<error(plugin_context&,const bytesBuf_t*)>(
            ssl_write_msg_header ) );
    ssl->add_operation(
        NETWORK_OP_WRITE_BODY,
        function<error(plugin_context&,const char*,const bytesBuf_t*,const bytesBuf_t*,const bytesBuf_t*,int,irodsProt_t)>(
            ssl_send_rods_msg ) );

    irods::network* net = dynamic_cast< irods::network* >( ssl );

    return net;

} // plugin_factory
