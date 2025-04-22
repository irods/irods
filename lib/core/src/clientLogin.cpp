#include "irods/authPluginRequest.h"
#include "irods/authenticate.h"
#include "irods/authentication_plugin_framework.hpp"
#include "irods/checksum.h"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_auth_constants.hpp"
#include "irods/irods_auth_factory.hpp"
#include "irods/irods_auth_manager.hpp"
#include "irods/irods_auth_object.hpp"
#include "irods/irods_auth_plugin.hpp"
#include "irods/irods_configuration_keywords.hpp"
#include "irods/irods_configuration_parser.hpp"
#include "irods/irods_environment_properties.hpp"
#include "irods/irods_kvp_string_parser.hpp"
#include "irods/irods_logger.hpp"
#include "irods/irods_native_auth_object.hpp"
#include "irods/irods_pam_auth_object.hpp"
#include "irods/rcGlobalExtern.h"
#include "irods/rodsClient.h"
#include "irods/sslSockComm.h"
#include "irods/termiosUtil.hpp"

#include <openssl/evp.h>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>

#include <fmt/format.h>

#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <string_view>

#include <termios.h>

static char prevChallengeSignatureClient[200];

namespace
{
    using json = nlohmann::json;
} // anonymous namespace

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

auto set_session_signature_client_side(rcComm_t* _comm, const char* _buffer, std::size_t _buffer_size) -> int
{
    constexpr std::size_t required_size = 16;

    if (!_comm || !_buffer || _buffer_size < required_size) {
        return SYS_INVALID_INPUT_PARAM;
    }

    std::memset(_comm->session_signature, 0, sizeof(RcComm::session_signature));

    const std::string_view bytes{_buffer, required_size};
    const auto* end = fmt::format_to(_comm->session_signature, "{:02x}", fmt::join(bytes, ""));

    // If the difference in position is not double the original size, something went wrong.
    // The session signature is expected to be 32 bytes long (w/o the null byte). Each byte
    // in the original string will occupy 2 bytes in the signature.
    if ((required_size * 2) != (end - _comm->session_signature)) {
        return SYS_LIBRARY_ERROR;
    }

    return 0;
} // set_session_signature_client_side

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
        allocate_if_necessary_and_add_rError_msg(&Conn->rError, status, "sslStart");
        return status;
    }

    memset( &pamAuthReqInp, 0, sizeof( pamAuthReqInp ) );
    pamAuthReqInp.pamPassword = myPassword;
    pamAuthReqInp.pamUser = userName;
    pamAuthReqInp.timeToLive = ttl;
    status = rcPamAuthRequest( Conn, &pamAuthReqInp, &pamAuthReqOut );
    if ( status ) {
        allocate_if_necessary_and_add_rError_msg(&Conn->rError, status, "rcPamAuthRequest");
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
        const auto msg = fmt::format("rcGetLimitedPassword failed with error [{}]", status);
        allocate_if_necessary_and_add_rError_msg(&Conn->rError, status, msg.c_str());
        memset( userPassword, 0, sizeof( userPassword ) );
        return status;
    }

    /* calculate the limited password, which is a hash of the user's main pw and
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

/// =-=-=-=-=-=-=-
/// @brief clientLogin provides the interface for authentication
///        plugins as well as defining the protocol or template
///        Authentication will follow
int clientLogin(rcComm_t* _comm, const char* _context, const char* _scheme_override)
{
    if (!_comm) {
        return SYS_INVALID_INPUT_PARAM;
    }

    // If _comm already claims to be authenticated, there is nothing to do.
    if (1 == _comm->loggedIn) {
        return 0;
    }

    RodsEnvironment env{};
    if (const auto err = getRodsEnv(&env); 0 != err) {
        return err;
    }

    const auto ctx = (nullptr != _context) ? json::parse(_context) : nlohmann::json{};
    return irods::authentication::authenticate_client(*_comm, env, ctx);
} // clientLogin

int
clientLoginWithPassword( rcComm_t *Conn, char* password ) {
    int status, len, i = 0;
    authRequestOut_t *authReqOut = NULL;
    authResponseInp_t authRespIn;
    char md5Buf[CHALLENGE_LEN + MAX_PASSWORD_LEN + 2];
    char digest[RESPONSE_LEN + 2];
    char userNameAndZone[NAME_LEN * 2 + 1];
    if ( !password ) {
        allocate_if_necessary_and_add_rError_msg(&Conn->rError, -1, "null password pointer");
        return -1;
    }

    if ( Conn->loggedIn == 1 ) {
        /* already logged in */
        return 0;
    }
    status = rcAuthRequest( Conn, &authReqOut );
    if ( status || NULL == authReqOut ) { // JMC cppcheck - nullptr
        allocate_if_necessary_and_add_rError_msg(&Conn->rError, status, "rcAuthRequest");
        return status;
    }

    memset( md5Buf, 0, sizeof( md5Buf ) );
    strncpy( md5Buf, authReqOut->challenge, CHALLENGE_LEN );
    setSessionSignatureClientside( md5Buf );

    // Attach the leading bytes of the md5Buf buffer to the RcComm.
    //
    // This is important because setSessionSignatureClientside assumes client applications
    // only ever manage a single iRODS connection. This assumption breaks C/C++ application's
    // ability to modify passwords when multiple iRODS connections are under management.
    //
    // However, instead of replacing the original call, we leave it in place to avoid breaking
    // backwards compatibility.
    set_session_signature_client_side(Conn, md5Buf, sizeof(md5Buf));

    len = strlen( password );
    sprintf( md5Buf + CHALLENGE_LEN, "%s", password );
    md5Buf[CHALLENGE_LEN + len] = '\0'; /* remove trailing \n */

    // First, we need to fetch a digest implementation for the desired algorithm, MD5. This is done this way because
    // passing, for example, EVP_md5() directly to EVP_DigestInit has a performance penalty which is avoided by
    // fetching the digest implementation from the default provider. Anything fetched using EVP_MD_fetch should be
    // freed by EVP_MD_free.
    EVP_MD* message_digest = EVP_MD_fetch(nullptr, "MD5", nullptr);
    const auto free_md = irods::at_scope_exit{[&message_digest] { EVP_MD_free(message_digest); }};

    // Establish a context for the digest, ensuring that it is freed when we exit the function.
    auto* context = EVP_MD_CTX_new();
    const auto free_context = irods::at_scope_exit{[&context] { EVP_MD_CTX_free(context); }};

    // Initialize the digest context for the derived digest implementation. Must use _ex or _ex2 to avoid automatically
    // resetting the context with EVP_MD_CTX_reset.
    if (0 == EVP_DigestInit_ex2(context, message_digest, nullptr)) {
        return DIGEST_INIT_FAILED;
    }
    // Hash the specified buffer of bytes and store the results in the context.
    if (0 == EVP_DigestUpdate(context, reinterpret_cast<unsigned char*>(md5Buf), CHALLENGE_LEN + MAX_PASSWORD_LEN)) {
        return DIGEST_UPDATE_FAILED;
    }
    // Finally, retrieve the digest value from the context and place it into digest. And yes, a return value of 0 means
    // an error occurred. Use the _ex function here so that the digest context is not automatically cleaned up with
    // EVP_MD_CTX_reset.
    if (0 == EVP_DigestFinal_ex(context, reinterpret_cast<unsigned char*>(digest), nullptr)) {
        return DIGEST_FINAL_FAILED;
    }

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
        allocate_if_necessary_and_add_rError_msg(&Conn->rError, status, "rcAuthResponse");
        return status;
    }
    Conn->loggedIn = 1;

    return 0;
}
