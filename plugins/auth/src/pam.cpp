#include "irods/authentication_plugin_framework.hpp"

#define USE_SSL 1
#include "irods/sslSockComm.h"

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
    namespace irods_auth = irods::experimental::auth;
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
        char new_password[MAX_PASSWORD_LEN + 2]{};
        strncpy(new_password, password.c_str(), MAX_PASSWORD_LEN);
        printf("\n");
        tty.c_lflag = oldflag;
        if (tcsetattr(STDIN_FILENO, TCSANOW, &tty)) {
            printf("Error reinstating echo mode.\n");
        }

        return new_password;
    } // get_password_from_client_stdin

#ifdef RODS_SERVER
    auto run_pam_auth_check(const std::string& _username, const std::string& _password) -> void
    {
        // TODO why do we need a separate executable?
#ifndef PAM_AUTH_CHECK_PROG
#define PAM_AUTH_CHECK_PROG  "./irodsPamAuthCheck"
#endif

        // pipe between parent and child processes to which the password will be written
        // Do not attempt to close the pipe at any ol' scope exit. Rather, only close the pipe
        // when we are done with it or if an early exit occurs.
        int p2cp[2]{};
        if (pipe(p2cp) < 0) {
            THROW(SYS_PIPE_ERROR, "failed to open pipe");
        }

        int pid = fork();
        if (pid == -1) {
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
        execl(PAM_AUTH_CHECK_PROG, PAM_AUTH_CHECK_PROG, _username.c_str(), (char*)NULL);

        THROW(SYS_FORK_ERROR, fmt::format("execl failed [errno:{}]", errno));
    } // run_pam_auth_check
#endif // #ifdef RODS_SERVER
} // anonymous namespace

namespace irods
{
    class pam_authentication : public irods_auth::authentication_base {
    private:
        static constexpr char* perform_native_auth = "perform_native_auth";

    public:
        pam_authentication()
        {
            add_operation(AUTH_CLIENT_AUTH_REQUEST,  OPERATION(rcComm_t, pam_auth_client_request));
            add_operation(perform_native_auth,       OPERATION(rcComm_t, pam_auth_client_perform_native_auth));
#ifdef RODS_SERVER
            add_operation(AUTH_AGENT_AUTH_REQUEST,   OPERATION(rsComm_t, pam_auth_agent_request));
#endif
        } // ctor

    private:
        json auth_client_start(rcComm_t& comm, const json& req)
        {
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
            else {
                // obfGetPw returns 0 if the password is retrieved successfully. Therefore,
                // we do NOT need a password in this case. This being the case, we conclude
                // that the user has already been authenticated via PAM with the server. We
                // proceed with steps for native authentication which will use the stored
                // password. This is the legacy behavior for the PAM authentication plugin.
                if (const bool need_password = obfGetPw(nullptr); !need_password) {
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

        json pam_auth_client_request(rcComm_t& comm, const json& req)
        {
            json svr_req{req};

            svr_req[irods_auth::next_operation] = AUTH_AGENT_AUTH_REQUEST;

            // Need to enable SSL here if it is not already being used because the PAM password
            // is sent to the server in the clear.
            const bool using_ssl = irods::CS_NEG_USE_SSL == comm.negotiation_results;
            const auto end_ssl_if_we_enabled_it = irods::at_scope_exit{[&comm, using_ssl] {
                if (!using_ssl)  {
                    sslEnd(&comm);
                }
            }};

            if (!using_ssl) {
                if (const int ec = sslStart(&comm); ec) {
                    THROW(ec, "failed to enable SSL");
                }
            }

            auto resp = irods_auth::request(comm, svr_req);

            irods_auth::throw_if_request_message_is_missing_key(
                resp, {"request_result"}
            );

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
        } // pam_auth_client_request

        json pam_auth_client_perform_native_auth(rcComm_t& comm, const json& req)
        {
            // This operation is basically just running the entire native authentication flow
            // because this is how the PAM authentication plugin has worked historically. This
            // is done in order to minimize communications with the PAM server as iRODS does
            // not use proper "sessions".
            json resp{req};

            // The authentication password needs to be removed from the request message as it
            // will send the password over the network without SSL being necessarily enabled.
            resp.erase(irods::AUTH_PASSWORD_KEY);

            static constexpr char* auth_scheme_native = "native";
            rodsEnv env{};
            std::strncpy(env.rodsAuthScheme, auth_scheme_native, NAME_LEN);
            irods_auth::authenticate_client(comm, env, json{});

            // If everything completes successfully, the flow is completed and we can
            // consider the user "logged in". Again, the entire native authentication flow
            // was run and so we trust the result.
            resp[irods_auth::next_operation] = irods_auth::flow_complete;

            comm.loggedIn = 1;

            return resp;
        } // pam_auth_client_perform_native_auth

#ifdef RODS_SERVER
        json pam_auth_agent_request(rsComm_t& comm, const json& req)
        {
            using log_auth = irods::experimental::log::authentication;

            const std::vector<std::string_view> required_keys{"user_name",
                                                              "zone_name",
                                                              irods::AUTH_PASSWORD_KEY};
            irods_auth::throw_if_request_message_is_missing_key(
                req, required_keys
            );

            rodsServerHost_t* host = nullptr;

            log_auth::trace("connecting to catalog provider");

            if (const int ec = getAndConnRcatHost(&comm, MASTER_RCAT, comm.clientUser.rodsZone, &host); ec < 0) {
                THROW(ec, "getAndConnRcatHost failed.");
            }

            if (LOCAL_HOST != host->localFlag) {
                const auto disconnect = irods::at_scope_exit{[host]
                    {
                        rcDisconnect(host->conn);
                        host->conn = nullptr;
                    }
                };

                log_auth::trace("redirecting call to CSP");

                if (const auto ec = sslStart(host->conn); ec) {
                    THROW(ec, "could not establish SSL connection");
                }

                const auto end_ssl = irods::at_scope_exit{[host] { sslEnd(host->conn); }};

                return irods_auth::request(*host->conn, req);
            }

            json resp{req};

            log_auth::trace("getting TTL param");

            int ttl = 0;
            if (req.contains(irods::AUTH_TTL_KEY)) {
                if (const auto& ttl_str = req.at(irods::AUTH_TTL_KEY).get_ref<const std::string&>();
                    !ttl_str.empty()) {
                    try {
                        ttl = boost::lexical_cast<int>(ttl_str);
                    }
                    catch (const boost::bad_lexical_cast& e) {
                        THROW(SYS_INVALID_INPUT_PARAM, fmt::format("invalid TTL [{}]", ttl_str));
                    }
                }
            }

            const auto username = req.at("user_name").get_ref<const std::string&>();
            const auto password = req.at(irods::AUTH_PASSWORD_KEY).get_ref<const std::string&>();

            log_auth::trace("performing PAM auth check for [{}]", username);

            run_pam_auth_check(username, password);

            log_auth::trace("PAM auth check done", username);

            char password_out[MAX_NAME_LEN]{};
            char* pw_ptr = &password_out[0];
            const int ec = chlUpdateIrodsPamPassword(&comm,
                                                     const_cast<char*>(username.c_str()),
                                                     ttl,
                                                     nullptr,
                                                     &pw_ptr);
            if (ec < 0) {
                THROW(ec, "failed updating iRODS pam password");
            }

            resp["request_result"] = password_out;

            if (comm.auth_scheme) {
                free(comm.auth_scheme);
            }

            static constexpr char* auth_scheme_pam = "pam";
            comm.auth_scheme = strdup(auth_scheme_pam);

            return resp;
        } // pam_auth_agent_request
#endif // #ifdef RODS_SERVER
    }; // class pam_authentication
} // namespace irods

extern "C"
irods::pam_authentication* plugin_factory(const std::string&, const std::string&)
{
    return new irods::pam_authentication{};
}
