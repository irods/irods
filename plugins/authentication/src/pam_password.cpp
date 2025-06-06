/// \file

#include "irods/authentication_plugin_framework.hpp"

#include "irods/icatHighLevelRoutines.hpp"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_auth_constants.hpp"
#include "irods/irods_client_server_negotiation.hpp"
#include "irods/irods_logger.hpp"
#include "irods/irods_pam_auth_object.hpp"
#include "irods/rcConnect.h"

#include <boost/lexical_cast.hpp>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <sys/types.h>
#include <sys/wait.h>

#include <string>
#include <termios.h>
#include <unistd.h>

namespace
{
    namespace irods_auth = irods::authentication;
    using json = nlohmann::json;

    auto get_password_from_client_stdin() -> std::string
    {
        struct termios tty;
        tcgetattr(STDIN_FILENO, &tty);
        tcflag_t oldflag = tty.c_lflag;
        tty.c_lflag &= ~ECHO;
        int error = tcsetattr(STDIN_FILENO, TCSANOW, &tty);
        int errsv = errno;

        if (error) {
            printf("WARNING: Error %d disabling echo mode. Password will be displayed in plaintext.\n", errsv);
        }
        printf("Enter your current PAM password:");
        std::string password;
        getline(std::cin, password);
        printf("\n");
        tty.c_lflag = oldflag;
        if (tcsetattr(STDIN_FILENO, TCSANOW, &tty)) {
            printf("Error reinstating echo mode.\n");
        }

        return password;
    } // get_password_from_client_stdin

#ifdef RODS_SERVER
    auto run_pam_auth_check(const std::string& _username, const std::string& _password) -> void
    {
        // pipe between parent and child processes to which the password will be written
        // Do not attempt to close the pipe at any ol' scope exit. Rather, only close the pipe
        // when we are done with it or if an early exit occurs.
        int p2cp[2]{};
        if (pipe(p2cp) < 0) {
            THROW(SYS_PIPE_ERROR, "failed to open pipe");
        }

        int pid = fork();
        if (pid == -1) {
            irods::experimental::log::authentication::error("{}: fork() error: [errno=[{}]]", __func__, errno);
            close(p2cp[1]);
            THROW(SYS_FORK_ERROR, "failed to fork process");
        }

        // parent process
        if (pid) {
            if (write(p2cp[1], _password.c_str(), _password.size()) == -1) {
                int errsv = errno;
                irods::log(ERROR(errsv, "Error writing from parent to child."));
            }
            close(p2cp[1]);

            int ec = 0;
            waitpid(pid, &ec, 0);

            // anything other than 0 indicates that an error occurred.
            if (0 == ec) {
                return;
            }

            // what is 256?
            if (256 == ec) {
                THROW(PAM_AUTH_PASSWORD_FAILED, "pam auth check failed");
            }

            THROW(ec, "pam auth check failed");
        }

        // child process
        if (dup2(p2cp[0], STDIN_FILENO) == -1) {
            int errsv = errno;
            irods::log(ERROR(errsv, "Error duplicating the file descriptor."));
        }
        close(p2cp[1]);

        // From the docs...
        //
        // The exec() functions only return if an error has occurred.
        // The return value is -1, and errno is set to indicate the error.
        //
        // The list of arguments must be terminated by a NULL pointer, and, since these are
        // variadic functions, this pointer must be cast (char *) NULL.
        const auto binary = irods::get_irods_sbin_directory() / "irodsPamAuthCheck";
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
        execl(binary.c_str(), binary.c_str(), _username.c_str(), static_cast<char*>(nullptr));

        irods::experimental::log::authentication::error("{}: Returning default value [SYS_FORK_ERROR]", __func__);
        THROW(SYS_FORK_ERROR, fmt::format("execl failed [errno:{}]", errno));
    }  // run_pam_auth_check
#endif // #ifdef RODS_SERVER
} // anonymous namespace

namespace irods::authentication
{
    /// \brief Authentication plugin implementation for pam_password scheme.
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
    ///     // with the server, the plugin will generate a limited password and re-authenticate using that.
    ///     "a_ttl": 0,
    ///
    ///     // This option forces the plugin to prompt the user for a password which will be read from stdin. If the
    ///     // option is not present or is set to false, the password will be retrieved by some other means. It is
    ///     // possible that the plugin will still prompt for the user's password, but this is only after trying to
    ///     // get the password from the "a_pw" key or the .irodsA file.
    ///     "force_password_prompt": false,
    ///
    ///     // This option provides the user's PAM password to the plugin. This is useful for clients which do not use
    ///     // stdin. A password provided this way will be used over a previously-generated password stored in a
    ///     // .irodsA file and will re-authenticate with the PAM service and generate/record a new randomly-generated
    ///     // password. If "force_password_prompt" is present and set to true, this option is ignored. If this option
    ///     // is absent, a previously-generated password will be fetched from a .irodsA file or, failing that, the
    ///     // user's PAM password will be prompted from stdin. Note: This option treats the value for this option as
    ///     // the user's PAM password.
    ///     "a_pw": "<user's PAM password>",
    /// }
    /// \endcode
    class pam_password_authentication : public irods_auth::authentication_base
    {
    private:
      static constexpr const char* scheme_name = "pam_password";
      static constexpr const char* perform_native_auth = "perform_native_auth";

    public:
        pam_password_authentication()
        {
            // clang-format off
            add_operation(AUTH_CLIENT_AUTH_REQUEST, OPERATION(rcComm_t, pam_password_auth_client_request));
            add_operation(perform_native_auth,      OPERATION(rcComm_t, pam_password_auth_client_perform_native_auth));
#ifdef RODS_SERVER
            add_operation(AUTH_AGENT_AUTH_REQUEST,  OPERATION(rsComm_t, pam_password_auth_agent_request));
#endif
            // clang-format on
        } // ctor

    private:
        json auth_client_start(rcComm_t& comm, const json& req)
        {
            if (irods::CS_NEG_USE_SSL != static_cast<char*>(comm.negotiation_results)) {
                THROW(SYS_NOT_ALLOWED,
                      "Client communications with this server are not secure and this authentication plugin is "
                      "configured to require TLS communication. Authentication is not allowed unless this server "
                      "is configured to require TLS in order to prevent leaking sensitive user information.");
            }

            json resp{req};
            resp["user_name"] = comm.proxyUser.userName;
            resp["zone_name"] = comm.proxyUser.rodsZone;

            // The force_password_prompt keyword does not check for an existing password but
            // instead forcibly prompts the user for their password. This is useful when a
            // client like iinit wants to "reset" the user's authentication "session" but in
            // general other clients want to use the already-authenticated "session."
            const auto force_prompt = req.find(irods_auth::force_password_prompt);
            if (req.end() != force_prompt && force_prompt->get<bool>()) {
                resp[irods::AUTH_PASSWORD_KEY] = get_password_from_client_stdin();
            }
            // If the client has provided a password in the request, use that.
            else if (!resp.contains(irods::AUTH_PASSWORD_KEY)) {
                // obfGetPw returns 0 if the password is retrieved successfully. Therefore,
                // we do NOT need a password in this case. This being the case, we conclude
                // that the user has already been authenticated via PAM with the server. We
                // proceed with steps for native authentication which will use the stored
                // password. This is the legacy behavior for the PAM authentication plugin.
                if (0 == obfGetPw(nullptr)) {
                    resp[irods_auth::next_operation] = perform_native_auth;
                    return resp;
                }

                // There's no password stored, so we need to prompt the client for it.
                resp[irods::AUTH_PASSWORD_KEY] = get_password_from_client_stdin();
            }

            // Need to perform the PAM authentication steps before we authenticate with the
            // iRODS server. Take the stored password and check it with PAM. After this checks
            // out, the native authentication steps will be performed.
            resp[irods_auth::next_operation] = AUTH_CLIENT_AUTH_REQUEST;

            return resp;
        } // auth_client_start

        json pam_password_auth_client_request(rcComm_t& comm, const json& req)
        {
            if (irods::CS_NEG_USE_SSL != static_cast<char*>(comm.negotiation_results)) {
                THROW(SYS_NOT_ALLOWED,
                      "Client communications with this server are not secure and this authentication plugin is "
                      "configured to require TLS communication. Authentication is not allowed unless this server "
                      "is configured to require TLS in order to prevent leaking sensitive user information.");
            }

            json svr_req{req};

            svr_req[irods_auth::next_operation] = AUTH_AGENT_AUTH_REQUEST;

            auto resp = irods_auth::request(comm, svr_req);

            irods_auth::throw_if_request_message_is_missing_key(resp, {"request_result"});

            // Save the PAM password-based generated password so that the client remains
            // authenticated with the server while the password is still valid.
            const auto& pw = resp.at("request_result").get_ref<const std::string&>();
            if (const int ec = obfSavePw(0, 0, 0, pw.data()); ec < 0) {
                THROW(ec, "failed to save obfuscated password");
            }

            // Now that the password has been vetted with PAM and found to be good, the newly
            // generated password in the server can be used to authenticate with the iRODS
            // server.
            resp[irods_auth::next_operation] = perform_native_auth;

            return resp;
        } // pam_password_auth_client_request

        json pam_password_auth_client_perform_native_auth(rcComm_t& comm, const json& req)
        {
            // This operation is basically just running the entire native authentication flow
            // because this is how the PAM authentication plugin has worked historically. This
            // is done in order to minimize communications with the PAM server as iRODS does
            // not use proper "sessions".
            const auto ctx = nlohmann::json{{irods_auth::scheme_name, "native"}};
            if (const auto err = irods_auth::authenticate_client(comm, ctx); err < 0) {
                THROW(err, "pam_password: Failed to authenticate with generated native password.");
            }

            // If everything completes successfully, the flow is completed and we can
            // consider the user "logged in". Again, the entire native authentication flow
            // was run and so we trust the result.
            comm.loggedIn = 1;
            return nlohmann::json{{irods_auth::next_operation, irods_auth::flow_complete}};
        } // pam_password_auth_client_perform_native_auth

#ifdef RODS_SERVER
        json pam_password_auth_agent_request(rsComm_t& comm, const json& req)
        {
            using log_auth = irods::experimental::log::authentication;

            if (irods::CS_NEG_USE_SSL != static_cast<char*>(comm.negotiation_results)) {
                THROW(SYS_NOT_ALLOWED,
                      "Client communications with this server are not secure and this authentication plugin is "
                      "configured to require TLS communication. Authentication is not allowed unless this server "
                      "is configured to require TLS in order to prevent leaking sensitive user information.");
            }

            const std::vector<std::string_view> required_keys{"user_name", "zone_name", irods::AUTH_PASSWORD_KEY};
            irods_auth::throw_if_request_message_is_missing_key(req, required_keys);

            rodsServerHost_t* host = nullptr;

            log_auth::trace("connecting to catalog provider");

            if (const int ec = getAndConnRcatHost(&comm, PRIMARY_RCAT, comm.clientUser.rodsZone, &host); ec < 0) {
                THROW(ec, "getAndConnRcatHost failed.");
            }

            if (LOCAL_HOST != host->localFlag) {
                log_auth::trace("redirecting call to CSP");

                if (irods::CS_NEG_USE_SSL != static_cast<char*>(host->conn->negotiation_results)) {
                    THROW(SYS_NOT_ALLOWED,
                          "Server-to-server communications with the catalog service provider server are not secure and "
                          "this authentication plugin is configured to require TLS communication. Authentication "
                          "is not allowed unless this server is configured to require TLS in order to prevent "
                          "leaking sensitive user information.");
                }
                return irods_auth::request(*host->conn, req);
            }

            json resp{req};

            // This operation asserts that the key exists in the request above, so we know that it is there.
            const auto password_iter = resp.find(irods::AUTH_PASSWORD_KEY);
            const auto password = password_iter->get<std::string>();

            // Erase the password key as soon as it is no longer needed to avoid sending it back in the response.
            resp.erase(password_iter);

            log_auth::trace("getting TTL param");

            // Check TTL parameters before checking the password so that we avoid unnecessary communication with the PAM
            // service. If we cannot authenticate with the iRODS server because of a bad TTL input, there is no reason
            // to check the PAM credentials.
            int ttl = 0;
            if (const auto ttl_iter = req.find(irods::AUTH_TTL_KEY); ttl_iter != req.end()) {
                if (const auto& ttl_str = ttl_iter->get_ref<const std::string&>(); !ttl_str.empty()) {
                    try {
                        ttl = boost::lexical_cast<int>(ttl_str);
                    }
                    catch (const boost::bad_lexical_cast& e) {
                        THROW(SYS_INVALID_INPUT_PARAM, fmt::format("invalid TTL [{}]", ttl_str));
                    }
                }
            }

            const auto& username = req.at("user_name").get_ref<const std::string&>();

            log_auth::trace("performing PAM auth check for [{}]", username);

            run_pam_auth_check(username, password);

            log_auth::trace("PAM auth check done", username);

            // Plus 1 for null terminator.
            std::array<char, MAX_PASSWORD_LEN + 1> password_out{};
            char* password_ptr = password_out.data();
            const int ec =
                chlUpdateIrodsPamPassword(&comm, username.c_str(), ttl, nullptr, &password_ptr, password_out.size());
            if (ec < 0) {
                THROW(ec, "failed updating iRODS pam password");
            }

            resp["request_result"] = password_out.data();

            if (comm.auth_scheme) {
                free(comm.auth_scheme);
            }

            comm.auth_scheme = strdup(scheme_name);

            return resp;
        } // pam_password_auth_agent_request
#endif    // #ifdef RODS_SERVER
    };    // class pam_password_authentication
} // namespace irods::authentication

// clang-format off
extern "C"
auto plugin_factory([[maybe_unused]] const std::string& _instance_name,
                    [[maybe_unused]] const std::string& _context) -> irods_auth::pam_password_authentication*
{
    return new irods_auth::pam_password_authentication{}; // NOLINT(cppcoreguidelines-owning-memory)
} // plugin_factory
// clang-format on
