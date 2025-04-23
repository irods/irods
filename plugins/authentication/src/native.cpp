/// \file

#include "irods/authentication_plugin_framework.hpp"

#include "irods/authCheck.h"
#include "irods/authPluginRequest.h"
#include "irods/authRequest.h"
#include "irods/authResponse.h"
#include "irods/authenticate.h"
#include "irods/base64.hpp"
#include "irods/checksum.h" // for hashToStr
#include "irods/getLimitedPassword.h"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_auth_constants.hpp"
#include "irods/irods_auth_plugin.hpp"
#include "irods/irods_logger.hpp"
#include "irods/irods_stacktrace.hpp"
#include "irods/miscServerFunct.hpp"
#include "irods/msParam.h"
#include "irods/obf.h"
#include "irods/rcConnect.h"
#include "irods/rodsDef.h"

#ifdef RODS_SERVER
#include "irods/irods_rs_comm_query.hpp"
#include "irods/rsAuthCheck.hpp"
#include "irods/rsAuthRequest.hpp"
#endif // RODS_SERVER

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/md5.h>

#include <termios.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

void setSessionSignatureClientside(char* _sig);
#ifdef RODS_SERVER
int get64RandomBytes(char* buf);
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c, cert-dcl51-cpp)
void _rsSetAuthRequestGetChallenge(const char* _buf);
#endif

namespace
{
    using json = nlohmann::json;
    using log_auth = irods::experimental::log::authentication;
    namespace irods_auth = irods::authentication;

    // Implementation based on clientLoginTTL, which is now deprecated.
    auto record_limited_password(RcComm& _comm, int ttl, const char* _password) -> void
    {
        getLimitedPasswordInp_t inp{};
        inp.ttl = ttl;

        getLimitedPasswordOut_t* out{};
        if (const int err = rcGetLimitedPassword(&_comm, &inp, &out); 0 != err) {
            THROW(err, fmt::format("{}: rcGetLimitedPassword failed with error [{}]", __func__, err));
        }

        // Calculate the limited password, which is a hash of the user's password and the returned stringToHashWith.
        constexpr auto hash_length = 100;
        std::array<char, hash_length + 1> hash_buf{};

        std::strncpy(hash_buf.data(), static_cast<char*>(out->stringToHashWith), hash_length);
        // NOLINTNEXTLINE(cppcoreguidelines-no-malloc, cppcoreguidelines-owning-memory)
        std::free(out); // Free the output as it is no longer needed.
        std::strncat(hash_buf.data(), _password, hash_length);

        std::array<unsigned char, hash_length> digest{};
        obfMakeOneWayHash(HASH_TYPE_DEFAULT,
                          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                          reinterpret_cast<unsigned char*>(hash_buf.data()),
                          hash_length,
                          digest.data());

        // Clear out the buffer since it exists on the stack and contains a password.
        std::memset(hash_buf.data(), 0, hash_buf.size());

        std::array<char, hash_length> limited_password{};
        hashToStr(digest.data(), limited_password.data());

        if (const auto err = obfSavePw(0, 0, 0, limited_password.data()); err < 0) {
            THROW(err, fmt::format("{}: Failed to save limited password to irodsA file. error: [{}]", __func__, err));
        }
    } // record_limited_password

    auto get_password_from_client_stdin() -> std::string
    {
        termios tty{};
        tcgetattr(STDIN_FILENO, &tty);
        const tcflag_t oldflag = tty.c_lflag;
        // NOLINTNEXTLINE(hicpp-signed-bitwise)
        tty.c_lflag &= ~ECHO;
        if (const int error = tcsetattr(STDIN_FILENO, TCSANOW, &tty); 0 != error) {
            fmt::print("WARNING: Error {} disabling echo mode. "
                       "Password will be displayed in plaintext.\n",
                       errno);
        }
        fmt::print("Enter your current iRODS password:");
        std::string password{};
        getline(std::cin, password);
        fmt::print("\n");
        tty.c_lflag = oldflag;
        if (0 != tcsetattr(STDIN_FILENO, TCSANOW, &tty)) {
            fmt::print("Error reinstating echo mode.");
        }

        return password;
    } // get_password_from_client_stdin

    auto forcibly_prompt_for_password(const nlohmann::json& _req) -> bool
    {
        const auto force_prompt = _req.find(irods_auth::force_password_prompt);
        return _req.end() != force_prompt && force_prompt->get<bool>();
    } // forcibly_prompt_for_password
} // anonymous namespace

namespace irods::authentication
{
    /// \brief Authentication plugin implementation for native scheme.
    ///
    /// Authentication plugins implement a number of operations to perform the steps required to authenticate with
    /// an iRODS server. As described in \p irods::authentication::authenticate_client, the operations use JSON
    /// payloads to receive request information and return information in responses. The keys of the JSON payload
    /// recognized by this authentication scheme will be described here. All of these are optional and the values for
    /// the default behavior are shown. All options pertain to client-side plugin behavior.
    /// \code{.js}
    /// {
    ///     // This option instructs the plugin to authenticate the user with a time-limited password. The value is an
    ///     // integer which is given to the server. The server interprets the value in hours. After authenticating
    ///     // with the server using a "real" password, the plugin will generate a limited password and re-authenticate
    ///     // using that. It is recommended that this option be used with the "record_auth_file" option set to true so
    ///     // that the limited password can be used again by the client.
    ///     "a_ttl": 0,
    ///
    ///     // This option forces the plugin to prompt the user for a password which will be read from stdin. If the
    ///     // option is not present or is set to false, the password will be retrieved by some other means. It is
    ///     // possible that the plugin will still prompt for the user's password, but this is only after trying to
    ///     // get the password from the "a_pw" key or the .irodsA file.
    ///     "force_password_prompt": false,
    ///
    ///     // This option provides the user's iRODS password to the plugin. This is useful for clients which do not use
    ///     // stdin and may not use or have have access to a .irodsA file. A password provided this way will be used
    ///     // over a password stored in an .irodsA file. If "force_password_prompt" is present and set to true, this
    ///     // option is ignored. If this option is absent, the password will be fetched from a .irodsA file or, failing
    ///     // that, from stdin.
    ///     "a_pw": "<iRODS password>",
    ///
    ///     // This option instructs the client-side plugin to record the user's password to a .irodsA file. If this
    ///     // option is not present or is set to false, the password will not be recorded to a .irodsA file and the
    ///     // caller will need to provide the user's password again to authenticate with another connection.
    ///     "record_auth_file": false
    /// }
    /// \endcode
    class native_authentication : public irods_auth::authentication_base {
      private:
        static constexpr const char* scheme_name = "native";
        static constexpr const char* record_authentication_file = "record_authentication_file";
        static constexpr const char* complete_authentication = "complete_authentication";

      public:
        native_authentication()
        {
            // NOLINTBEGIN(readability-identifier-length)
            add_operation(AUTH_ESTABLISH_CONTEXT,    OPERATION(rcComm_t, native_auth_establish_context));
            add_operation(AUTH_CLIENT_AUTH_REQUEST,  OPERATION(rcComm_t, native_auth_client_request));
            add_operation(AUTH_CLIENT_AUTH_RESPONSE, OPERATION(rcComm_t, native_auth_client_response));
            add_operation(record_authentication_file, OPERATION(rcComm_t, record_authentication_file_op));
            add_operation(complete_authentication, OPERATION(rcComm_t, complete_authentication_op));
#ifdef RODS_SERVER
            add_operation(AUTH_AGENT_START,          OPERATION(rsComm_t, native_auth_agent_start));
            add_operation(AUTH_AGENT_AUTH_REQUEST,   OPERATION(rsComm_t, native_auth_agent_request));
            add_operation(AUTH_AGENT_AUTH_RESPONSE,  OPERATION(rsComm_t, native_auth_agent_response));
            add_operation(AUTH_AGENT_AUTH_VERIFY,    OPERATION(rsComm_t, native_auth_agent_verify));
#endif
            // NOLINTEND(readability-identifier-length)
        } // ctor

    private:
        json auth_client_start(rcComm_t& comm, const json& req)
        {
            json resp{req};
            resp[irods_auth::next_operation] = AUTH_CLIENT_AUTH_REQUEST;
            resp["user_name"] = comm.proxyUser.userName;
            resp["zone_name"] = comm.proxyUser.rodsZone;

            return resp;
        } // auth_client_start

        json native_auth_establish_context(rcComm_t& _comm, const json& req)
        {
            irods_auth::throw_if_request_message_is_missing_key(
                req, {"user_name", "zone_name", "request_result"}
            );

            json resp{req};

            auto request_result = req.at("request_result").get<std::string>();
            request_result.resize(CHALLENGE_LEN);

            // build a buffer for the challenge hash
            // Buffer structure:
            //  [64+1] challenge string (generated by server and sent back here)
            //  [50+1] obfuscated user password
            char md5_buf[CHALLENGE_LEN + MAX_PASSWORD_LEN + 2]{};
            strncpy(md5_buf, request_result.c_str(), CHALLENGE_LEN);

            // Save a representation of some of the challenge string for use as a session signature
            setSessionSignatureClientside(md5_buf);

            // Attach the leading bytes of the md5_buf buffer to the RcComm.
            //
            // This is important because setSessionSignatureClientside assumes client applications
            // only ever manage a single iRODS connection. This assumption breaks C/C++ application's
            // ability to modify passwords when multiple iRODS connections are under management.
            //
            // However, instead of replacing the original call, we leave it in place to avoid breaking
            // backwards compatibility.
            set_session_signature_client_side(&_comm, md5_buf, sizeof(md5_buf));

            // Store the user's password in here so that it can be restored later. It needs to be removed from the
            // request payload sent to the server in case secure communications (TLS) have not been enabled.
            [[maybe_unused]] std::string client_provided_password;

            // Determine if a password is needed.
            const auto password_key_iter = resp.find(irods::AUTH_PASSWORD_KEY);

            // This if-ladder could no doubt be made better. However, it correctly implements the means of obtaining a
            // password to use for authentication in priority order. The case for forcing a password prompt and the
            // fallback behavior of prompting the user for a password when no password was otherwise provided are
            // identical, and this makes clang-tidy upset. The forced password prompt takes priority over the other
            // cases, and the other cases take priority over the default case. We will ignore "bugprone-branch-clone"
            // here because while the branches are clones, they should not be collapsed.

            // NOLINTBEGIN(bugprone-branch-clone)

            // Anonymous user does not need (or have) a password.
            if (req.at("user_name").get_ref<const std::string&>() == ANONYMOUS_USER) {
                md5_buf[CHALLENGE_LEN + 1] = '\0';
            }
            // If the client wants to forcibly prompt for a password, get the password from stdin.
            else if (forcibly_prompt_for_password(req)) {
                client_provided_password = get_password_from_client_stdin();
            }
            // If the client has provided a password in the request, use that. Historically, we have given preference to
            // the .irodsA file, but we want to honor explicit provision of the password in the request now.
            else if (password_key_iter != resp.end()) {
                const auto& password = password_key_iter->get_ref<const std::string&>();
                if (password.length() > MAX_PASSWORD_LEN) {
                    THROW(PASSWORD_EXCEEDS_MAX_SIZE,
                          fmt::format("Provided password exceeds maximum length [{}].", MAX_PASSWORD_LEN));
                }
                // We do not need to remove the password from the request payload because no server communication
                // occurs in this operation.
                client_provided_password = password;
            }
            // If an .irodsA file exists, use the password from there. The array size matches the buffer in obfGetPw.
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
            else if (std::array<char, MAX_PASSWORD_LEN + 10> password{}; 0 == obfGetPw(password.data())) {
                client_provided_password = password.data();
                // Clear out the buffer since it exists on the stack and contains a password.
                password.fill('\0');
            }
            // If the password cannot be derived any other way, prompt from stdin.
            else {
                client_provided_password = get_password_from_client_stdin();
            }

            // NOLINTEND(bugprone-branch-clone)

            if (!client_provided_password.empty()) {
                std::strncpy(
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    static_cast<char*>(md5_buf) + CHALLENGE_LEN,
                    client_provided_password.c_str(),
                    MAX_PASSWORD_LEN);
            }

            // create a md5 hash of the challenge

            // Initialize the digest context for the derived digest implementation. Must use _ex or _ex2 to avoid
            // automatically resetting the context with EVP_MD_CTX_reset.
            EVP_MD* message_digest = EVP_MD_fetch(nullptr, "MD5", nullptr);
            EVP_MD_CTX* context = EVP_MD_CTX_new();
            const auto free_context = irods::at_scope_exit{[&context] { EVP_MD_CTX_free(context); }};
            if (0 == EVP_DigestInit_ex2(context, message_digest, nullptr)) {
                EVP_MD_free(message_digest);
                const auto ssl_error_code = ERR_get_error();
                const auto msg =
                    fmt::format("{}: Failed to initialize digest. error code: [{}]", __func__, ssl_error_code);
                THROW(DIGEST_INIT_FAILED, msg);
            }
            EVP_MD_free(message_digest);

            // Hash the specified buffer of bytes and store the results in the context.
            if (0 == EVP_DigestUpdate(context, md5_buf, CHALLENGE_LEN + MAX_PASSWORD_LEN)) {
                const auto ssl_error_code = ERR_get_error();
                const auto msg =
                    fmt::format("{}: Failed to calculate digest. error code: [{}]", __func__, ssl_error_code);
                THROW(DIGEST_UPDATE_FAILED, msg);
            }

            // Finally, retrieve the digest value from the context and place it into a buffer. Use the _ex function here
            // so that the digest context is not automatically cleaned up with EVP_MD_CTX_reset.
            char digest[RESPONSE_LEN + 2]{};
            if (0 == EVP_DigestFinal_ex(context, reinterpret_cast<unsigned char*>(digest), nullptr)) {
                const auto ssl_error_code = ERR_get_error();
                const auto msg =
                    fmt::format("{}: Failed to finalize digest. error code: [{}]", __func__, ssl_error_code);
                THROW(DIGEST_FINAL_FAILED, msg);
            }

            // make sure 'string' doesn't end early - scrub out any errant terminating chars
            // by incrementing their value by one
            for (int i = 0; i < RESPONSE_LEN; ++i) {
                if (digest[i] == '\0') {
                    digest[i]++;
                }
            }

            unsigned char out[RESPONSE_LEN*2];
            unsigned long out_len{RESPONSE_LEN*2};
            auto err = base64_encode(reinterpret_cast<unsigned char*>(digest), RESPONSE_LEN, out, &out_len);
            if(err < 0) {
                THROW(err, "base64 encoding of digest failed.");
            }

            // If a password was saved and not provided in the request, put it in the payload here.
            if (resp.end() == password_key_iter && !client_provided_password.empty()) {
                resp[irods::AUTH_PASSWORD_KEY] = client_provided_password;
            }

            resp["digest"] = std::string{reinterpret_cast<char*>(out), out_len};
            resp[irods_auth::next_operation] = AUTH_CLIENT_AUTH_RESPONSE;

            return resp;
        } // native_auth_establish_context

        json native_auth_client_request(rcComm_t& comm, const json& req)
        {
            json svr_req{req};

            // Do not transmit the password in the request to the server if it was supplied by the client application.
            // This plugin does not require secure communications, so we cannot transmit the password in the clear.
            // Copy the password contents and remove the key from the request payload. It will be restored.
            std::string client_provided_password_copy;
            if (const auto password_iter = svr_req.find(irods::AUTH_PASSWORD_KEY); password_iter != svr_req.end()) {
                client_provided_password_copy = password_iter->get<std::string>();
                svr_req.erase(password_iter);
            }

            svr_req[irods_auth::next_operation] = AUTH_AGENT_AUTH_REQUEST;
            auto resp = irods_auth::request(comm, svr_req);

            resp[irods_auth::next_operation] = AUTH_ESTABLISH_CONTEXT;

            // If a password was provided in the request payload, restore it here.
            if (!client_provided_password_copy.empty()) {
                resp[irods::AUTH_PASSWORD_KEY] = client_provided_password_copy;
            }

            return resp;
        } // native_auth_client_request

        json native_auth_client_response(rcComm_t& comm, const json& req)
        {
            irods_auth::throw_if_request_message_is_missing_key(
                req, {"digest", "user_name", "zone_name"}
            );

            json svr_req{req};

            // Do not transmit the password in the request to the server if it was supplied by the client application.
            // This plugin does not require secure communications, so we cannot transmit the password in the clear.
            // Copy the password contents and remove the key from the request payload. It will be restored if needed.
            std::string client_provided_password_copy;
            if (const auto password_iter = svr_req.find(irods::AUTH_PASSWORD_KEY); password_iter != svr_req.end()) {
                client_provided_password_copy = password_iter->get<std::string>();
                svr_req.erase(password_iter);
            }

            svr_req[irods_auth::next_operation] = AUTH_AGENT_AUTH_RESPONSE;
            auto resp = irods_auth::request(comm, svr_req);

            // If the client requested recording the auth file, that will be done here. Otherwise, authentication is
            // complete.
            if (const auto record_auth_file_iter = req.find(irods_auth::record_auth_file);
                req.end() != record_auth_file_iter && record_auth_file_iter->get<bool>())
            {
                resp[irods::AUTH_PASSWORD_KEY] = client_provided_password_copy;
                resp[irods_auth::next_operation] = record_authentication_file;
            }
            else {
                resp[irods_auth::next_operation] = complete_authentication;
            }

            return resp;
        } // native_auth_client_response

        static auto record_authentication_file_op(RcComm& _comm, const nlohmann::json& _req) -> nlohmann::json
        {
            // Get the password from the request payload, if available. If it is not there, this is not necessarily an
            // error. In that case, just complete the authentication flow without recording an authentication file.
            const std::string* password{};
            const auto password_iter = _req.find(irods::AUTH_PASSWORD_KEY);
            if (password_iter != _req.end()) {
                password = password_iter->get_ptr<const std::string*>();
            }
            if (nullptr == password || password->empty()) {
                return nlohmann::json{{irods_auth::next_operation, complete_authentication}};
            }

            // If TTL was provided, get and record the limited password.
            if (const auto ttl_iter = _req.find(irods::AUTH_TTL_KEY); ttl_iter != _req.end()) {
                if (const auto& ttl_str = ttl_iter->get_ref<const std::string&>(); !ttl_str.empty()) {
                    try {
                        // Specifically check for TTL of 0 here because 0 means "no TTL" for native authentication and
                        // is a valid value as far as this authentication plugin is concerned. However, the mechanism
                        // for getting the limited password considers non-positive values invalid. Therefore, we drop
                        // through to the "else" case for 0 and pass non-zero values to the limited password machinery
                        // to allow it to perform the value checking.
                        if (const auto ttl = std::stoi(ttl_str); 0 != ttl) {
                            record_limited_password(_comm, ttl, password->c_str());
                            // Now that the limited password is recorded in the auth file, we need to authenticate with
                            // the limited password to ensure that things are working. Call the start operation - the
                            // limited password recorded in the irodsA file will be used.
                            return nlohmann::json{
                                {"scheme", scheme_name}, {irods_auth::next_operation, AUTH_CLIENT_START}};
                        }
                    }
                    catch (const std::invalid_argument& e) {
                        THROW(SYS_INVALID_INPUT_PARAM, fmt::format("Invalid TTL [{}]: {}", ttl_str, e.what()));
                    }
                    catch (const std::out_of_range& e) {
                        THROW(SYS_INVALID_INPUT_PARAM, fmt::format("Invalid TTL [{}]: {}", ttl_str, e.what()));
                    }
                }
            }

            // If TTL was not provided or has a value of 0, we are not using TTL. Save the irodsA file traditionally.
            if (const auto err = obfSavePw(0, 0, 0, password->c_str()); err < 0) {
                THROW(err, "Failed to record authentication file for native authentication.");
            }
            return nlohmann::json{{irods_auth::next_operation, complete_authentication}};
        } // record_authentication_file_op

        static auto complete_authentication_op(RcComm& _comm, [[maybe_unused]] const nlohmann::json& _req)
            -> nlohmann::json
        {
            _comm.loggedIn = 1;
            return nlohmann::json{{irods_auth::next_operation, irods_auth::flow_complete}};
        } // complete_authentication_op

#ifdef RODS_SERVER
        json native_auth_agent_request(rsComm_t& comm, const json& req)
        {
            json resp{req};

            char buf[CHALLENGE_LEN + 2]{};
            get64RandomBytes( buf );

            resp["request_result"] = buf;

            _rsSetAuthRequestGetChallenge(buf);

            if (comm.auth_scheme) {
                free(comm.auth_scheme);
            }

            comm.auth_scheme = strdup(scheme_name);

            return resp;
        } // native_auth_agent_request

        json native_auth_agent_response(rsComm_t& comm, const json& req)
        {
            irods_auth::throw_if_request_message_is_missing_key(
                req, {"digest", "zone_name", "user_name"}
            );

            // need to do NoLogin because it could get into inf loop for cross zone auth
            rodsServerHost_t *rodsServerHost;
            auto zone_name = req.at("zone_name").get<std::string>();
            int status =
                getAndConnRcatHostNoLogin(&comm, PRIMARY_RCAT, const_cast<char*>(zone_name.c_str()), &rodsServerHost);
            if ( status < 0 ) {
                THROW(status, "Connecting to rcat host failed.");
            }

            char* response = static_cast<char*>(malloc(RESPONSE_LEN + 1));
            std::memset(response, 0, RESPONSE_LEN + 1);
            const auto free_response = irods::at_scope_exit{[response] { free(response); }};

            response[RESPONSE_LEN] = 0;

            unsigned long out_len = RESPONSE_LEN;
            auto to_decode = req.at("digest").get<std::string>();
            auto err = base64_decode(reinterpret_cast<unsigned char*>(const_cast<char*>(to_decode.c_str())),
                                     to_decode.size(),
                                     reinterpret_cast<unsigned char*>(response),
                                     &out_len);
            if (err < 0) {
                THROW(err, "base64 decoding of digest failed.");
            }

            authCheckInp_t authCheckInp{};
            authCheckInp.challenge = _rsAuthRequestGetChallenge();
            authCheckInp.response = response;

            const std::string username = fmt::format(
                    "{}#{}", req.at("user_name").get_ref<const std::string&>(), zone_name);
            authCheckInp.username = const_cast<char*>(username.data());

            authCheckOut_t* authCheckOut = nullptr;
            if (LOCAL_HOST == rodsServerHost->localFlag) {
                status = rsAuthCheck(&comm, &authCheckInp, &authCheckOut);
            }
            else {
                status = rcAuthCheck(rodsServerHost->conn, &authCheckInp, &authCheckOut);
                /* not likely we need this connection again */
                rcDisconnect(rodsServerHost->conn);
                rodsServerHost->conn = nullptr;
            }

            if (status < 0 || !authCheckOut) {
                THROW(status, "rcAuthCheck failed.");
            }

            json resp{req};

            // Do we need to consider remote zones here?
            if (LOCAL_HOST != rodsServerHost->localFlag) {
                if (!authCheckOut->serverResponse) {
                    log_auth::info("Warning, cannot authenticate remote server, no serverResponse field");
                    THROW(REMOTE_SERVER_AUTH_NOT_PROVIDED, "Authentication disallowed. no serverResponse field.");
                }

                if (*authCheckOut->serverResponse == '\0') {
                    log_auth::info("Warning, cannot authenticate remote server, serverResponse field is empty");
                    THROW(REMOTE_SERVER_AUTH_EMPTY, "Authentication disallowed, empty serverResponse.");
                }

                char md5Buf[CHALLENGE_LEN + MAX_PASSWORD_LEN + 2]{};
                strncpy(md5Buf, authCheckInp.challenge, CHALLENGE_LEN);

                char userZone[NAME_LEN + 2]{};
                strncpy(userZone, zone_name.data(), NAME_LEN + 1);

                char serverId[MAX_PASSWORD_LEN + 2]{};
                getZoneServerId(userZone, serverId);

                if ('\0' == serverId[0]) {
                    log_auth::info("rsAuthResponse: Warning, cannot authenticate the remote server, no RemoteZoneSID defined in server_config.json");
                    THROW(REMOTE_SERVER_SID_NOT_DEFINED, "Authentication disallowed, no RemoteZoneSID defined");
                }

                strncpy(md5Buf + CHALLENGE_LEN, serverId, strlen(serverId));

                char digest[RESPONSE_LEN + 2]{};
                obfMakeOneWayHash(
                    HASH_TYPE_DEFAULT,
                    ( unsigned char* )md5Buf,
                    CHALLENGE_LEN + MAX_PASSWORD_LEN,
                    ( unsigned char* )digest );

                for (int i = 0; i < RESPONSE_LEN; i++) {
                    if (digest[i] == '\0') {
                        digest[i]++;
                    }  /* make sure 'string' doesn't end early*/
                }

                char* cp = authCheckOut->serverResponse;

                for (int i = 0; i < RESPONSE_LEN; i++) {
                    if ( *cp++ != digest[i] ) {
                        THROW(REMOTE_SERVER_AUTHENTICATION_FAILURE, "Authentication disallowed, server response incorrect.");
                    }
                }
            }

            /* Set the clientUser zone if it is null. */
            if ('\0' == comm.clientUser.rodsZone[0]) {
                zoneInfo_t* tmpZoneInfo{};
                status = getLocalZoneInfo( &tmpZoneInfo );
                if ( status < 0 ) {
                    THROW(status, "getLocalZoneInfo failed.");
                }
                else {
                    strncpy(comm.clientUser.rodsZone, tmpZoneInfo->zoneName, NAME_LEN);
                }
            }

            /* have to modify privLevel if the icat is a foreign icat because
             * a local user in a foreign zone is not a local user in this zone
             * and vice versa for a remote user
             */
            if (rodsServerHost->rcatEnabled == REMOTE_ICAT ) {
                /* proxy is easy because rodsServerHost is based on proxy user */
                if ( authCheckOut->privLevel == LOCAL_PRIV_USER_AUTH ) {
                    authCheckOut->privLevel = REMOTE_PRIV_USER_AUTH;
                }
                else if ( authCheckOut->privLevel == LOCAL_USER_AUTH ) {
                    authCheckOut->privLevel = REMOTE_USER_AUTH;
                }

                /* adjust client user */
                if ( 0 == strcmp(comm.proxyUser.userName, comm.clientUser.userName ) ) {
                    authCheckOut->clientPrivLevel = authCheckOut->privLevel;
                }
                else {
                    zoneInfo_t *tmpZoneInfo;
                    status = getLocalZoneInfo( &tmpZoneInfo );
                    if ( status < 0 ) {
                        THROW(status, "getLocalZoneInfo failed.");
                    }
                    else {
                        if ( 0 == strcmp( tmpZoneInfo->zoneName, comm.clientUser.rodsZone ) ) {
                            /* client is from local zone */
                            if ( REMOTE_PRIV_USER_AUTH == authCheckOut->clientPrivLevel ) {
                                authCheckOut->clientPrivLevel = LOCAL_PRIV_USER_AUTH;
                            }
                            else if ( REMOTE_USER_AUTH == authCheckOut->clientPrivLevel ) {
                                authCheckOut->clientPrivLevel = LOCAL_USER_AUTH;
                            }
                        }
                        else {
                            /* client is from remote zone */
                            if ( LOCAL_PRIV_USER_AUTH == authCheckOut->clientPrivLevel ) {
                                authCheckOut->clientPrivLevel = REMOTE_USER_AUTH;
                            }
                            else if ( LOCAL_USER_AUTH == authCheckOut->clientPrivLevel ) {
                                authCheckOut->clientPrivLevel = REMOTE_USER_AUTH;
                            }
                        }
                    }
                }
            }
            else if ( 0 == strcmp(comm.proxyUser.userName,  comm.clientUser.userName ) ) {
                authCheckOut->clientPrivLevel = authCheckOut->privLevel;
            }

            irods::throw_on_insufficient_privilege_for_proxy_user(comm, authCheckOut->privLevel);

            log_auth::debug(
                    "rsAuthResponse set proxy authFlag to {}, client authFlag to {}, user:{} proxy:{} client:{}",
                    authCheckOut->privLevel,
                    authCheckOut->clientPrivLevel,
                    authCheckInp.username,
                    comm.proxyUser.userName,
                    comm.clientUser.userName);

            if ( strcmp(comm.proxyUser.userName, comm.clientUser.userName ) != 0 ) {
                comm.proxyUser.authInfo.authFlag = authCheckOut->privLevel;
                comm.clientUser.authInfo.authFlag = authCheckOut->clientPrivLevel;
            }
            else {          /* proxyUser and clientUser are the same */
                comm.proxyUser.authInfo.authFlag =
                    comm.clientUser.authInfo.authFlag = authCheckOut->privLevel;
            }

            if ( authCheckOut != NULL ) {
                if ( authCheckOut->serverResponse != NULL ) {
                    free( authCheckOut->serverResponse );
                }
                free( authCheckOut );
            }

            return resp;
        } // native_auth_agent_response

        // =-=-=-=-=-=-=-
        // stub for ops that the native plug does
        // not need to support
        json native_auth_agent_verify(rsComm_t&, const json&)
        {
            return {};
        } // native_auth_agent_verify

        // =-=-=-=-=-=-=-
        // stub for ops that the native plug does
        // not need to support
        json native_auth_agent_start(rsComm_t&, const json&)
        {
            return {};
        } // native_auth_agent_start
#endif
    }; // class native_authentication
} // namespace irods::authentication

// clang-format off
extern "C"
auto plugin_factory([[maybe_unused]] const std::string& _instance_name,
                    [[maybe_unused]] const std::string& _context) -> irods_auth::native_authentication*
{
    return new irods_auth::native_authentication{}; // NOLINT(cppcoreguidelines-owning-memory)
} // plugin_factory
// clang-format on
