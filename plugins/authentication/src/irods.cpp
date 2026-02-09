#include "irods/authentication_plugin_framework.hpp"

#include "irods/authenticate.h"
#include "irods/authentication_client_utils.hpp"
#include "irods/irods_auth_constants.hpp"
#include "irods/irods_exception.hpp"
#include "irods/irods_stacktrace.hpp"
#include "irods/msParam.h"
#include "irods/rcConnect.h"
#include "irods/rodsDef.h"

#ifdef RODS_SERVER
#  include "irods/authentication_server_utils.hpp"
#  include "irods/icatHighLevelRoutines.hpp"
#  include "irods/irods_client_server_negotiation.hpp"
#  include "irods/irods_logger.hpp"
#  define IRODS_USER_ADMINISTRATION_ENABLE_SERVER_SIDE_API
#  include "irods/user_administration.hpp"
#endif // RODS_SERVER

#include <fmt/format.h>

#include <unistd.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>

namespace
{
#ifdef RODS_SERVER
    using log_auth = irods::experimental::log::authentication;
#endif // RODS_SERVER
    using json = nlohmann::json;
    namespace irods_auth = irods::authentication;

#ifdef RODS_SERVER
    auto auth_scheme_requires_secure_communications(const std::string& _auth_scheme) -> bool
    {
        const auto config_path = irods::configuration_parser::key_path_t{
            irods::KW_CFG_PLUGIN_CONFIGURATION, "authentication", _auth_scheme, "insecure_mode"};
        try {
            // Return the negation of the configuration's value because the configuration is "insecure_mode", but this
            // function is returning whether secure communications are required. So, if insecure_mode is set to true, we
            // should return false for this function; and vice-versa.
            return !irods::get_server_property<const bool>(config_path);
        }
        catch (const irods::exception& e) {
            if (KEY_NOT_FOUND == e.code()) {
                // If the plugin configuration is not set, default to requiring secure communications.
                return true;
            }
            // Re-throw for any other error.
            throw;
        }
        catch (const json::exception& e) {
            THROW(CONFIGURATION_ERROR,
                  fmt::format("Error occurred while attempting to get the value of server configuration [{}]: {}",
                              fmt::join(config_path, "."),
                              e.what()));
        }
    } // auth_scheme_requires_secure_communications

    auto throw_if_secure_communications_required() -> void
    {
        if (auth_scheme_requires_secure_communications("irods")) {
            THROW(SYS_NOT_ALLOWED,
                  "Client communications with this server are not secure and this authentication plugin is "
                  "configured to require TLS/SSL communication. Authentication is not allowed unless this server "
                  "is configured to require TLS/SSL in order to prevent leaking sensitive user information.");
        }
    } // throw_if_secure_communications_required

    auto log_warning_for_insecure_mode() -> void
    {
        log_auth::warn("Client communications with this server are not secure, and sensitive user information is "
                       "being communicated over the network in an unencrypted manner. Configure this server to "
                       "require TLS/SSL to prevent security leaks.");
    } // log_warning_for_insecure_mode
#endif // RODS_SERVER
} // anonymous namespace

namespace irods::authentication
{
    class irods_authentication : public irods_auth::authentication_base
    {
      private:
        // Operation names
        static constexpr const char* client_init_auth_with_server = "client_init_auth_with_server";
        static constexpr const char* client_prepare_auth_check = "client_prepare_auth_check";
        static constexpr const char* client_auth_with_password = "client_auth_with_password";
        static constexpr const char* client_auth_with_session_token = "client_auth_with_session_token";
        static constexpr const char* server_prepare_auth_check = "server_prepare_auth_check";
        static constexpr const char* server_auth_with_password = "server_auth_with_password";
        static constexpr const char* server_auth_with_session_token = "server_auth_with_session_token";

        // Other keys / constants
        static constexpr const char* password_kw = "password";
        static constexpr const char* session_token_kw = "session_token";
        static constexpr const char* user_name_kw = "user_name";
        static constexpr const char* zone_name_kw = "zone_name";

      public:
        irods_authentication()
        {
            add_operation(client_init_auth_with_server, OPERATION(RcComm, client_init_auth_with_server_op));
            add_operation(client_prepare_auth_check, OPERATION(RcComm, client_prepare_auth_check_op));
            add_operation(client_auth_with_password, OPERATION(RcComm, client_auth_with_password_op));
            add_operation(client_auth_with_session_token, OPERATION(RcComm, client_auth_with_session_token_op));
#ifdef RODS_SERVER
            add_operation(server_prepare_auth_check, OPERATION(RsComm, server_prepare_auth_check_op));
            add_operation(server_auth_with_password, OPERATION(RsComm, server_auth_with_password_op));
            add_operation(server_auth_with_session_token, OPERATION(RsComm, server_auth_with_session_token_op));
#endif
        } // ctor

      private:
        auto auth_client_start(rcComm_t& comm, const json& req) -> nlohmann::json override
        {
            json resp(req);
            resp[user_name_kw] = comm.proxyUser.userName;
            resp[zone_name_kw] = comm.proxyUser.rodsZone;
            resp[irods_auth::next_operation] = client_init_auth_with_server;
            return resp;
        } // auth_client_start

        static auto client_init_auth_with_server_op(RcComm& _comm, const nlohmann::json& _request) -> nlohmann::json
        {
            nlohmann::json svr_req(_request);
            svr_req[irods_auth::next_operation] = server_prepare_auth_check;
            auto resp = irods_auth::request(_comm, svr_req);
            resp[irods_auth::next_operation] = client_prepare_auth_check;
            return resp;
        } // client_init_auth_with_server_op

        static auto client_prepare_auth_check_op([[maybe_unused]] RcComm& _comm, const nlohmann::json& _request)
            -> nlohmann::json
        {
            irods_auth::throw_if_request_message_is_missing_key(_request, {user_name_kw, zone_name_kw});
            nlohmann::json resp(_request);
            const auto force_prompt = _request.find(irods_auth::force_password_prompt);
            if (_request.end() != force_prompt && force_prompt->get<bool>()) {
                fmt::print("Enter your iRODS password:");
                resp[password_kw] = irods::authentication::get_password_from_client_stdin();
                resp[irods_auth::next_operation] = client_auth_with_password;
                return resp;
            }
            // The anonymous user does not require a session token or password to authenticate.
            if (ANONYMOUS_USER == _request.at(user_name_kw).get_ref<const std::string&>()) {
                resp[password_kw] = "";
                resp[irods_auth::next_operation] = client_auth_with_password;
                return resp;
            }
            // If a password is provided by the client to the plugin, authenticate with that.
            if (const auto provided_password = _request.find(password_kw); _request.end() != provided_password) {
                resp[irods_auth::next_operation] = client_auth_with_password;
                return resp;
            }
            // If a session token is provided by the client to the plugin, authenticate with that.
            if (const auto provided_session_token = _request.find(session_token_kw);
                _request.end() != provided_session_token)
            {
                resp[irods_auth::next_operation] = client_auth_with_session_token;
                return resp;
            }
            // If a session token exists in the well-known session token file location(s), authenticate with that.
            // If this fails due to an expired session token, the caller will need to try again after clearing the
            // session token file, or with the force_password_prompt option.
            if (const auto discovered_session_token = irods_auth::read_session_token_from_file();
                !discovered_session_token.empty())
            {
                resp[session_token_kw] = discovered_session_token;
                resp[irods_auth::next_operation] = client_auth_with_session_token;
                return resp;
            }
            // If no session token was provided, no session token is found in the local file, no password is provided,
            // AND the user is not anonymous, get the password from stdin. This is the last resort.
            fmt::print("Enter your iRODS password:");
            resp[password_kw] = irods_auth::get_password_from_client_stdin();
            resp[irods_auth::next_operation] = client_auth_with_password;
            return resp;
        } // client_prepare_auth_check_op

        static auto client_auth_with_password_op(RcComm& _comm, const nlohmann::json& _request) -> nlohmann::json
        {
            irods_auth::throw_if_request_message_is_missing_key(_request, {user_name_kw, zone_name_kw, password_kw});
            nlohmann::json svr_req(_request);
            svr_req[irods_auth::next_operation] = server_auth_with_password;
            auto resp = irods_auth::request(_comm, svr_req);
            resp[irods_auth::next_operation] = client_auth_with_session_token;
            return resp;
        } // client_auth_with_password_op

        static auto client_auth_with_session_token_op(RcComm& _comm, const nlohmann::json& _request) -> nlohmann::json
        {
            irods_auth::throw_if_request_message_is_missing_key(
                _request, {user_name_kw, zone_name_kw, session_token_kw});
            nlohmann::json svr_req(_request);
            svr_req[irods_auth::next_operation] = server_auth_with_session_token;
            auto resp = irods_auth::request(_comm, svr_req);
            if (const auto record_auth_file_iter = _request.find(irods_auth::record_auth_file);
                _request.end() != record_auth_file_iter && record_auth_file_iter->get<bool>())
            {
                irods_auth::write_session_token_to_file(_request.at(session_token_kw).get_ref<const std::string&>());
            }
            _comm.loggedIn = 1;
            resp[irods_auth::next_operation] = irods_auth::flow_complete;
            return resp;
        } // client_auth_with_session_token_op

#ifdef RODS_SERVER
        static auto server_prepare_auth_check_op(RsComm& _comm, const nlohmann::json& _request) -> nlohmann::json
        {
            nlohmann::json resp(_request);
            if (nullptr != _comm.auth_scheme) {
                // NOLINTNEXTLINE(cppcoreguidelines-no-malloc, cppcoreguidelines-owning-memory)
                std::free(_comm.auth_scheme);
            }
            _comm.auth_scheme = strdup("irods");
            // Make sure the connection is secured before proceeding. If the connection is not secure, a warning will be
            // displayed in the server log at the very least. If the plugin is not configured to allow for insecure
            // communications between the client and server, the authentication attempt is rejected outright.
            if (irods::CS_NEG_USE_SSL != static_cast<char*>(_comm.negotiation_results)) {
                throw_if_secure_communications_required();
                log_warning_for_insecure_mode();
            }
            return resp;
        } // server_prepare_auth_check_op

        static auto server_auth_with_password_op(RsComm& _comm, const nlohmann::json& _request) -> nlohmann::json
        {
            // Make sure the connection is secured before proceeding. If the connection is not secure, a warning will be
            // displayed in the server log at the very least. If the plugin is not configured to allow for insecure
            // communications between the client and server, the authentication attempt is rejected outright.
            if (irods::CS_NEG_USE_SSL != static_cast<char*>(_comm.negotiation_results)) {
                throw_if_secure_communications_required();
                log_warning_for_insecure_mode();
            }
            irods_auth::throw_if_request_message_is_missing_key(_request, {zone_name_kw, user_name_kw, password_kw});
            // Need to do NoLogin because it could get into inf loop for cross zone auth.
            rodsServerHost_t* host{};
            const auto& zone_name = _request.at(zone_name_kw).get_ref<const std::string&>();
            // Even though getAndConnRcatHostNoLogin does not modify the zone name parameter, it uses a pointer-to-char
            // parameter instead of pointer-to-const-char parameter, so we must cast the const away on the c_str() call
            // here. It is safe to do so - just ugly.
            // NOLINTBEGIN(cppcoreguidelines-pro-type-const-cast)
            if (const auto connect_err =
                    getAndConnRcatHostNoLogin(&_comm, PRIMARY_RCAT, const_cast<char*>(zone_name.c_str()), &host);
                connect_err < 0)
            {
                THROW(connect_err, fmt::format("Failed to connect to catalog service provider: [{}]", connect_err));
            }
            // NOLINTEND(cppcoreguidelines-pro-type-const-cast)
            // What follows in this operation requires access to database operations, so continue on the catalog
            // provider.
            if (LOCAL_HOST != host->localFlag) {
                // In addition to the client-server connection, the server-to-server connection which occurs between the
                // local server and the catalog service provider must be secured as well. If the connection is not
                // secure, a warning will be displayed in the server log at the very least. If the plugin is not
                // configured to allow for insecure communications between the client (in this case, also a server) and
                // server, the authentication attempt is rejected outright.
                if (irods::CS_NEG_USE_SSL != static_cast<char*>(_comm.negotiation_results)) {
                    throw_if_secure_communications_required();
                    log_warning_for_insecure_mode();
                }
                // Note: We should not disconnect this server-to-server connection because the connection is not owned
                // by this context. A set of server-to-server connections is maintained by the server agent and reused
                // by various APIs and operations as needed.
                return irods_auth::request(*host->conn, _request);
            }

            // Check the provided username / password combination.
            const auto& user_name = _request.at(user_name_kw).get_ref<const std::string&>();
            int valid = 0;
            const auto& password = _request.at(password_kw).get_ref<const std::string&>();
            const auto check_password_input =
                nlohmann::json{{"user_name", user_name}, {"zone_name", zone_name}, {"password", password}};
            log_auth::trace("{}: Checking password for user [{}#{}]", __func__, user_name, zone_name);
            if (const int check_password_ec = chl_check_password(&_comm, check_password_input.dump().c_str(), &valid);
                check_password_ec < 0)
            {
                THROW(check_password_ec,
                      fmt::format("Error occurred while checking password for user [{}#{}]: {}",
                                  user_name,
                                  zone_name,
                                  check_password_ec));
            }
            if (0 == valid) {
                log_auth::debug("{}: Password check for user [{}#{}] failed", __func__, user_name, zone_name);
                THROW(
                    AUTHENTICATION_ERROR, fmt::format("Authentication failed for user [{}#{}].", user_name, zone_name));
            }

            // If the password check passed, make sure the user has sufficient privileges to request a session token
            // which does not expire.
            const auto expires_iter = _request.find("session_token_expires");
            const auto expires = (expires_iter == _request.end() || expires_iter->get<bool>());
            if (!expires) {
                namespace adm = irods::experimental::administration;
                const auto user = adm::user{user_name, zone_name};
                const auto user_type = adm::server::type(_comm, user);
                if (!user_type) {
                    THROW(CAT_INVALID_USER_TYPE,
                          fmt::format("Failed to get user type for [{}#{}].", user.name, user.zone));
                }
                if (adm::user_type::rodsadmin != *user_type) {
                    THROW(SYS_NOT_ALLOWED, "Only rodsadmins may request session tokens which do not expire.");
                }
            }

            // Now create a session token.
            char* token{};
            const auto free_token = irods::at_scope_exit{[&token] {
                if (nullptr != token) {
                    // NOLINTNEXTLINE(cppcoreguidelines-no-malloc, cppcoreguidelines-owning-memory)
                    std::free(token);
                }
            }};
            const auto create_session_token_input = nlohmann::json{
                {"user_name", user_name}, {"zone_name", zone_name}, {"auth_scheme", "irods"}, {"expires", expires}};
            log_auth::trace("{}: Making session token for user [{}#{}]", __func__, user_name, zone_name);
            if (const int make_token_err =
                    chl_make_session_token(&_comm, create_session_token_input.dump().c_str(), &token);
                make_token_err < 0)
            {
                THROW(make_token_err,
                      fmt::format("Error occurred while generating session token for user [{}#{}]: {}",
                                  user_name,
                                  zone_name,
                                  make_token_err));
            }

            // Now clear expired session tokens. Failing to do so is not an error, so just log a warning and continue.
            const auto remove_expired_tokens_input =
                nlohmann::json{{"user_name", user_name}, {"zone_name", zone_name}, {"expired_only", true}};
            log_auth::trace("{}: Deleting expired session tokens for user [{}#{}]", __func__, user_name, zone_name);
            if (const int remove_token_err =
                    chl_remove_session_tokens(&_comm, remove_expired_tokens_input.dump().c_str());
                remove_token_err < 0)
            {
                log_auth::warn("Error occurred while clearing expired session tokens after authenticating user [{}#{}] "
                               "with password: {}",
                               user_name,
                               zone_name,
                               remove_token_err);
            }

            // Remove the password from the payload so that it's not sent back across the network unnecessarily, and add
            // the new session token so that the client side can use it to authenticate.
            nlohmann::json resp(_request);
            resp.erase(password_kw);
            resp[session_token_kw] = token;
            log_auth::trace("{}: check complete for user [{}#{}]", __func__, user_name, zone_name);
            return resp;
        } // server_auth_with_password_op

        static auto server_auth_with_session_token_op(RsComm& _comm, const nlohmann::json& _request) -> nlohmann::json
        {
            // Make sure the connection is secured before proceeding. If the connection is not secure, a warning will be
            // displayed in the server log at the very least. If the plugin is not configured to allow for insecure
            // communications between the client and server, the authentication attempt is rejected outright.
            if (irods::CS_NEG_USE_SSL != static_cast<char*>(_comm.negotiation_results)) {
                throw_if_secure_communications_required();
                log_warning_for_insecure_mode();
            }
            irods_auth::throw_if_request_message_is_missing_key(
                _request, {zone_name_kw, user_name_kw, session_token_kw});
            // Need to do NoLogin because it could get into inf loop for cross zone auth.
            rodsServerHost_t* host{};
            const auto& zone_name = _request.at(zone_name_kw).get_ref<const std::string&>();
            // Even though getAndConnRcatHostNoLogin does not modify the zone name parameter, it uses a pointer-to-char
            // parameter instead of pointer-to-const-char parameter, so we must cast the const away on the c_str() call
            // here. It is safe to do so - just ugly.
            // NOLINTBEGIN(cppcoreguidelines-pro-type-const-cast)
            if (const auto connect_err =
                    getAndConnRcatHostNoLogin(&_comm, PRIMARY_RCAT, const_cast<char*>(zone_name.c_str()), &host);
                connect_err < 0)
            {
                THROW(connect_err, fmt::format("Failed to connect to catalog service provider: [{}]", connect_err));
            }
            // NOLINTEND(cppcoreguidelines-pro-type-const-cast)
            // What follows in this operation requires access to database operations, so continue on the catalog
            // provider.
            if (LOCAL_HOST != host->localFlag) {
                // In addition to the client-server connection, the server-to-server connection which occurs between the
                // local server and the catalog service provider must be secured as well. If the connection is not
                // secure, a warning will be displayed in the server log at the very least. If the plugin is not
                // configured to allow for insecure communications between the client (in this case, also a server) and
                // server, the authentication attempt is rejected outright.
                if (irods::CS_NEG_USE_SSL != static_cast<char*>(_comm.negotiation_results)) {
                    throw_if_secure_communications_required();
                    log_warning_for_insecure_mode();
                }
                // Note: We should not disconnect this server-to-server connection because the connection is not owned
                // by this context. A set of server-to-server connections is maintained by the server agent and reused
                // by various APIs and operations as needed.
                return irods_auth::request(*host->conn, _request);
            }

            const auto& user_name = _request.at(user_name_kw).get_ref<const std::string&>();

            // Authenticate using the session token...
            const auto& session_token = _request.at(session_token_kw).get_ref<const std::string&>();
            int valid = 0;
            const auto json_input = nlohmann::json{{"user_name", user_name},
                                                   {"zone_name", zone_name},
                                                   {"session_token", session_token},
                                                   {"auth_scheme", "irods"}};
            if (const int check_token_err = chl_check_session_token(&_comm, json_input.dump().c_str(), &valid);
                check_token_err < 0)
            {
                THROW(check_token_err,
                      fmt::format("Error occurred while checking session token for user [{}#{}]: {}",
                                  user_name,
                                  zone_name,
                                  check_token_err));
            }
            if (0 == valid) {
                THROW(
                    AUTHENTICATION_ERROR, fmt::format("Authentication failed for user [{}#{}].", user_name, zone_name));
            }

            // Success! Now set user privilege information in the RsComm. This could be its own operation, but then we
            // would require the client to call the operation, so, just make it automatic as part of this operation.
            irods_auth::set_privileges_in_rs_comm(_comm, user_name, zone_name);

            // Remove the session token from the response payload so that it is not communicated across the network
            // unnecessarily. The session token was included with the request, so the client should already have it.
            nlohmann::json resp(_request);
            resp.erase(session_token_kw);
            return resp;
        } // server_auth_with_session_token_op
#endif
    }; // class irods_authentication
} // namespace irods::authentication

extern "C" auto plugin_factory([[maybe_unused]] const std::string& _instance_name,
                               [[maybe_unused]] const std::string& _context) -> irods_auth::irods_authentication*
{
    return new irods_auth::irods_authentication{}; // NOLINT(cppcoreguidelines-owning-memory)
} // plugin_factory
