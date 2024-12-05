#include "irods/authPluginRequest.h"
#include "irods/authentication_plugin_framework.hpp"
#include "irods/checksum.h"
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

#include <openssl/md5.h>

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
    using log_auth = irods::experimental::log::authentication;

    auto authenticate_with_plugin_framework(RcComm& _comm, rodsEnv& _env, const json& _ctx) -> int
    {
        // If _comm already claims to be authenticated, there is nothing to do.
        if (1 == _comm.loggedIn) {
            return 0;
        }

        try {
            irods::experimental::auth::authenticate_client(_comm, _env, _ctx);

            log_auth::trace("Authenticated user [{}]", _comm.clientUser.userName);
        }
        catch (const irods::exception& e) {
            const auto ec = e.code();

            const std::string msg = fmt::format(
                "Error occurred while authenticating user [{}] [{}] [ec={}]",
                _comm.clientUser.userName, e.client_display_what(), ec);

            log_auth::info(msg);

            // NOLINTNEXTLINE(bugprone-narrowing-conversions,cppcoreguidelines-narrowing-conversions)
            allocate_if_necessary_and_add_rError_msg(&_comm.rError, ec, msg.c_str());

            return ec;
        }
        catch (const nlohmann::json::exception& e) {
            constexpr auto ec = SYS_LIBRARY_ERROR;

            const std::string msg = fmt::format(
                "JSON error occurred while authenticating user [{}] [{}]",
                _comm.clientUser.userName, e.what());

            log_auth::info(msg);

            allocate_if_necessary_and_add_rError_msg(&_comm.rError, ec, msg.c_str());

            return ec;
        }
        catch (const std::exception& e) {
            constexpr auto ec = SYS_INTERNAL_ERR;

            const std::string msg = fmt::format(
                "Error occurred while authenticating user [{}] [{}]",
                _comm.clientUser.userName, e.what());

            log_auth::info(msg);

            allocate_if_necessary_and_add_rError_msg(&_comm.rError, ec, msg.c_str());

            return ec;
        }
        catch (...) {
            constexpr auto ec = SYS_UNKNOWN_ERROR;

            const std::string msg = fmt::format(
                "Unknown error occurred while authenticating user [{}]",
                _comm.clientUser.userName);

            log_auth::info(msg);

            allocate_if_necessary_and_add_rError_msg(&_comm.rError, ec, msg.c_str());

            return ec;
        }

        return 0;
    } // authenticate_with_plugin_framework
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

    bool use_legacy_authentication;
    try {
        use_legacy_authentication = irods::experimental::auth::use_legacy_authentication(*_comm);
    }
    catch (const irods::exception& e) {
        const auto ec = e.code();
        const std::string msg = e.client_display_what();

        log_auth::info(msg);

        // NOLINTNEXTLINE(bugprone-narrowing-conversions,cppcoreguidelines-narrowing-conversions)
        allocate_if_necessary_and_add_rError_msg(&_comm->rError, ec, msg.c_str());

        return ec;
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
            char* auth_env_var = getenv( irods::to_env( irods::KW_CFG_IRODS_AUTHENTICATION_SCHEME ).c_str() );
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
            std::transform( auth_scheme.begin(), auth_scheme.end(), auth_scheme.begin(), [](unsigned char _ch) { return std::tolower(_ch); });

            // =-=-=-=-=-=-=-
            // filter out the pam auth as it is an extra special
            // case and only sent in as an override.
            // everyone other scheme behaves as normal
            if (use_legacy_authentication && irods::AUTH_PAM_SCHEME == auth_scheme) {
                auth_scheme = irods::AUTH_NATIVE_SCHEME;
            }
        } // if _scheme_override
    } // if client side auth

    if (!use_legacy_authentication) {
        try {
            // Use the authentication scheme string derived above in the client environment as
            // it may have been overridden or incorrectly formatted (i.e. not all lowercase).
            rodsEnv env{};
            if (const int ec = getRodsEnv(&env); ec < 0) {
                THROW(ec, fmt::format("getRodsEnv error. status = [{}]", ec));
            }

            if (auth_scheme != env.rodsAuthScheme) {
                std::strncpy(env.rodsAuthScheme, auth_scheme.data(), NAME_LEN);
            }

            return authenticate_with_plugin_framework(*_comm, env, _context ? json::parse(_context) : json{});
        }
        catch (const irods::exception& e) {
            const auto ec = e.code();
            const std::string msg = e.client_display_what();

            log_auth::info(msg);

            // NOLINTNEXTLINE(bugprone-narrowing-conversions,cppcoreguidelines-narrowing-conversions)
            allocate_if_necessary_and_add_rError_msg(&_comm->rError, ec, msg.c_str());

            return ec;
        }
        catch (const json::exception& e) {
            const auto ec = SYS_LIBRARY_ERROR;
            const std::string msg = fmt::format("JSON error occurred: [{}]", e.what());

            log_auth::info(msg);

            allocate_if_necessary_and_add_rError_msg(&_comm->rError, ec, msg.c_str());

            return ec;
        }
    }

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
        // NOLINTNEXTLINE(bugprone-narrowing-conversions,cppcoreguidelines-narrowing-conversions)
        allocate_if_necessary_and_add_rError_msg(&_comm->rError, ret.code(), ret.result().c_str());
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
        // NOLINTNEXTLINE(bugprone-narrowing-conversions,cppcoreguidelines-narrowing-conversions)
        allocate_if_necessary_and_add_rError_msg(&_comm->rError, ret.code(), ret.result().c_str());
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
        allocate_if_necessary_and_add_rError_msg(&Conn->rError, status, "rcAuthResponse");
        return status;
    }
    Conn->loggedIn = 1;

    return 0;
}
