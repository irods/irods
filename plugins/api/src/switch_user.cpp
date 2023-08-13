#include "irods/irods_pack_table.hpp"
#include "irods/plugins/api/api_plugin_number.h"
#include "irods/plugins/api/switch_user_types.h"
#include "irods/rodsDef.h"
#include "irods/rcConnect.h"
#include "irods/rodsPackInstruct.h"
#include "irods/rcMisc.h"
#include "irods/client_api_allowlist.hpp"

#include "irods/apiHandler.hpp"

#include <functional>

#ifdef RODS_SERVER

//
// Server-side Implementation
//

// clang-format off
#include "irods/switch_user.h"

#include "irods/fileOpr.hpp" // For FD_INUSE.
#include "irods/initServer.hpp" // For close_all_l1_descriptors.
#include "irods/irods_logger.hpp"
#include "irods/irods_rs_comm_query.hpp"
#include "irods/irods_server_properties.hpp"
#include "irods/key_value_proxy.hpp"
#include "irods/rodsErrorTable.h"
#include "irods/rsGlobalExtern.hpp" // For L1desc array.
#include "irods/rsTicketAdmin.hpp"
#include "irods/server_utilities.hpp"
#include "irods/stringOpr.h"
#include "irods/version.hpp"

#define IRODS_USER_ADMINISTRATION_ENABLE_SERVER_SIDE_API
#include "irods/user_administration.hpp"

#include <cstring>
#include <span>
#include <string>
#include <string_view>
// clang-format on

// These compile-time assertion guarantee that we always keep these data structures in sync.
static_assert(sizeof(SwitchUserInput::username) == sizeof(UserInfo::userName));
static_assert(sizeof(SwitchUserInput::zone) == sizeof(UserInfo::rodsZone));

// This global variable is required so that the API plugin can access RcComm connections
// created as a result of previous API calls (i.e. connections created due to redirection).
extern zoneInfo* ZoneInfoHead;

namespace
{
    //
    // Namespace and Type aliases
    //

    namespace adm = irods::experimental::administration;

    using log_api = irods::experimental::log::api;

    //
    // Function Prototypes
    //

    auto call_switch_user(irods::api_entry*, RsComm*, const SwitchUserInput*) -> int;

    auto rs_switch_user(RsComm*, const SwitchUserInput*) -> int;

    auto check_input(const SwitchUserInput*) -> int;

    auto update_user_info(UserInfo&, const char*, const char*, const char*) -> void;

    auto update_privilege_level(AuthInfo&, adm::user_type, const std::string_view, const std::string_view) -> void;

    auto update_server_to_server_connections(RsComm&, const SwitchUserInput&) -> void;

    auto terminate_ticket_session(RsComm&) -> void;

    //
    // Function Implementations
    //

    auto call_switch_user(irods::api_entry* _api, RsComm* _comm, const SwitchUserInput* _input) -> int
    {
        return _api->call_handler<const SwitchUserInput*>(_comm, _input);
    } // call_switch_user

    auto check_input(const SwitchUserInput* _input) -> int
    {
        if (!_input) {
            log_api::error("Invalid input argument: received a null pointer");
            return SYS_INVALID_INPUT_PARAM;
        }

        if (is_non_empty_string(_input->username, sizeof(SwitchUserInput::username)) == 0) {
            log_api::error("Invalid username argument: empty string");
            return SYS_INVALID_INPUT_PARAM;
        }

        if (is_non_empty_string(_input->zone, sizeof(SwitchUserInput::zone)) == 0) {
            log_api::error("Invalid zone argument: empty string");
            return SYS_INVALID_INPUT_PARAM;
        }

        return 0;
    } // check_input

    auto update_user_info(UserInfo& _user, const char* _new_username, const char* _new_zone, const char* _new_user_type)
        -> void
    {
        std::strncpy(_user.userName, _new_username, sizeof(UserInfo::userName));
        std::strncpy(_user.rodsZone, _new_zone, sizeof(UserInfo::rodsZone));
        std::strncpy(_user.userType, _new_user_type, sizeof(UserInfo::userType));
    } // update_user_info

    auto update_privilege_level(
        AuthInfo& _auth_info,
        adm::user_type _user_type,
        const std::string_view _user_zone,
        const std::string_view _local_zone) -> void
    {
        // Set the appropriate privilege level based on whether the user is local or remote to this zone.
        if (_user_zone == _local_zone) {
            _auth_info.authFlag = (_user_type == adm::user_type::rodsadmin) ? LOCAL_PRIV_USER_AUTH : LOCAL_USER_AUTH;
        }
        else {
            _auth_info.authFlag = (_user_type == adm::user_type::rodsadmin) ? REMOTE_PRIV_USER_AUTH : REMOTE_USER_AUTH;
        }
    } // update_privilege_level

    auto update_server_to_server_connections(const SwitchUserInput& _input) -> void
    {
        const auto switch_user_or_disconnect = [&](rodsServerHost& _host) {
            if (!_host.conn) {
                log_api::trace("No connection to remote host [{}].", _host.hostName->name);
                return;
            }

            // Disconnect if the client requested that all server-to-server connections be closed or
            // if the remote server's version is earlier than 4.3.1.
            //
            // Remember, only iRODS 4.3.1 and later support the switch user API plugin.
            if (!getValByKey(&_input.options, KW_KEEP_SVR_TO_SVR_CONNECTIONS) ||
                *irods::to_version(_host.conn->svrVersion->relVersion) < irods::version{4, 3, 1})
            {
                log_api::trace("Closing server-to-server connection to remote host [{}].", _host.hostName->name);
                rcDisconnect(_host.conn);
                _host.conn = nullptr;
                return;
            }

            // Create a copy of the SwitchUserInput and update it for connections created due to server
            // redirection.
            SwitchUserInput input_copy{};
            std::strncpy(input_copy.username, _input.username, sizeof(SwitchUserInput::username) - 1);
            std::strncpy(input_copy.zone, _input.zone, sizeof(SwitchUserInput::zone) - 1);
            copyKeyVal(&_input.options, &input_copy.options);

            // Server-to-server connections are proxy connections created by the server. That means the
            // identity of the proxy user matches that of the local iRODS administrator. The proxy user
            // for these connections MUST NOT be updated.
            rmKeyVal(&input_copy.options, KW_SWITCH_PROXY_USER);

            // At this point, we know the remote server supports this API plugin.
            if (const auto ec = rc_switch_user(_host.conn, &input_copy); ec != 0) {
                log_api::error(
                    "rc_switch_user failed on remote host [{}] with error_code [{}]. Disconnecting from remote host.",
                    _host.hostName->name,
                    ec);
                rcDisconnect(_host.conn);
                _host.conn = nullptr;
            }
        };

        for (auto* zone_ptr = ZoneInfoHead; zone_ptr; zone_ptr = zone_ptr->next) {
            for (auto* host_ptr = zone_ptr->primaryServerHost; host_ptr; host_ptr = host_ptr->next) {
                switch_user_or_disconnect(*host_ptr);
            }

            for (auto* host_ptr = zone_ptr->secondaryServerHost; host_ptr; host_ptr = host_ptr->next) {
                switch_user_or_disconnect(*host_ptr);
            }
        }
    } // update_server_to_server_connections

    auto terminate_ticket_session(RsComm& _comm) -> void
    {
        log_api::trace("Terminating ticket session.");

        TicketAdminInput input{};
        input.arg1 = "session";
        input.arg2 = "";
        input.arg3 = "";

        if (const auto ec = rsTicketAdmin(&_comm, &input); ec < 0) {
            log_api::error("rsTicketAdmin failed with error_code [{}].", ec);
        }
    } // terminate_ticket_session

    auto rs_switch_user(RsComm* _comm, const SwitchUserInput* _input) -> int
    {
        try {
            // Only administrators are allowed to invoke this API.
            // We check the proxy user because it covers clients and server-to-server redirects.
            // Remember, the client and proxy user information is identical until there's a redirect.
            if (!irods::is_privileged_proxy(*_comm)) {
                log_api::error(
                    "Proxy user [{}] does not have permission to switch client user.", _comm->proxyUser.userName);
                return SYS_PROXYUSER_NO_PRIV;
            }

            const irods::experimental::key_value_proxy opts{_input->options};

            if (opts.contains(KW_CLOSE_OPEN_REPLICAS)) {
                close_all_l1_descriptors(*_comm);
            }
            else {
                // Verify there are no open L1 descriptors. Return an error if there are. This forces the
                // client to clean up behind themselves and keeps this API plugin from becoming overly complicated.
                for (const auto& l1d : std::span{L1desc}) {
                    if (FD_INUSE == l1d.inuseFlag) {
                        log_api::error("Switching to another user is not allowed while there are open L1 descriptors.");
                        return SYS_NOT_ALLOWED;
                    }
                }
            }

            // Return immediately if the client did not provide non-empty strings for the
            // username and zone.
            if (const auto ec = check_input(_input); ec != 0) {
                return ec;
            }

            const auto* local_zone = getLocalZoneName();

            if (!local_zone) {
                log_api::error("Could not get name of local zone.");
                return SYS_INTERNAL_ERR;
            }

            // This call covers the existence check too.
            const auto user_type = adm::server::type(*_comm, adm::user{_input->username, _input->zone});

            if (!user_type) {
                constexpr const char* msg = "User type for [{}#{}] could not be identified in local zone [{}].";
                log_api::error(msg, _input->username, _input->zone, local_zone);
                return CAT_INVALID_USER;
            }

            const auto* user_type_string = adm::to_c_str(*user_type);

            // Create a copy of the client information and modify it.
            // This protects the RsComm from partial updates. This works as long as UserInfo and the members
            // inside of it are PODs.
            auto client = _comm->clientUser;

            // Update the user identity associated with the RsComm.
            update_user_info(client, _input->username, _input->zone, user_type_string);
            update_privilege_level(client.authInfo, *user_type, _input->zone, local_zone);

            // Update the proxy user identity associated with the RsComm if requested by the client.
            if (opts.contains(KW_SWITCH_PROXY_USER)) {
                auto proxy = _comm->proxyUser;

                update_user_info(proxy, _input->username, _input->zone, user_type_string);
                update_privilege_level(proxy.authInfo, *user_type, _input->zone, local_zone);

                log_api::trace("Updating client user and proxy user information in RsComm.");

                // Update the client and proxy information stored in the RsComm.
                _comm->clientUser = client;
                _comm->proxyUser = proxy;
            }
            else {
                log_api::trace("Updating client user information in RsComm.");
                _comm->clientUser = client;
            }

            // iRODS agents do not disconnect from other nodes following a redirect. For long running agents,
            // this means the API plugin must invoke rc_switch_user on each connection or disconnect each
            // connection.
            update_server_to_server_connections(*_input);

            // If the connection had a ticket enabled on it, we must clear it to avoid security issues.
            // Failing to do so can result in the switched-to user gaining access to data they normally
            // wouldn't have access to.
            terminate_ticket_session(*_comm);

            return 0;
        }
        catch (const irods::exception& e) {
            log_api::error(e.what());
            return e.code();
        }
        catch (const std::exception& e) {
            log_api::error(e.what());
            return SYS_LIBRARY_ERROR;
        }
        catch (...) {
            log_api::error("An unknown error occurred while processing the request.");
            return SYS_UNKNOWN_ERROR;
        }
    } // rs_switch_user

    using operation = std::function<int(RsComm*, const SwitchUserInput*)>;
    const operation op = rs_switch_user;
    auto fn_ptr = reinterpret_cast<funcPtr>(call_switch_user);
} // anonymous namespace

#else // RODS_SERVER

//
// Client-side Implementation
//

namespace
{
    using operation = std::function<int(RsComm*, const SwitchUserInput*)>;
    const operation op{};
    funcPtr fn_ptr = nullptr; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
} // anonymous namespace

#endif // RODS_SERVER

// The plugin factory function must always be defined.
extern "C" auto plugin_factory(
    [[maybe_unused]] const std::string& _instance_name, // NOLINT(bugprone-easily-swappable-parameters)
    [[maybe_unused]] const std::string& _context) -> irods::api_entry*
{
#ifdef RODS_SERVER
    irods::client_api_allowlist::add(SWITCH_USER_APN);
#endif // RODS_SERVER

    // clang-format off
    irods::apidef_t def{
        SWITCH_USER_APN,            // API number
        RODS_API_VERSION,           // API version
        NO_USER_AUTH,               // Client auth
        NO_USER_AUTH,               // Proxy auth
        "SwitchUserInp_PI", 0,      // In PI / bs flag
        nullptr, 0,                 // Out PI / bs flag
        op,                         // Operation
        "api_switch_user",          // Operation name
        clearSwitchUserInput,       // Clear input function
        irods::clearOutStruct_noop, // Clear output function
        fn_ptr
    };
    // clang-format on

    auto* api = new irods::api_entry{def}; // NOLINT(cppcoreguidelines-owning-memory)

    api->in_pack_key = "SwitchUserInp_PI";
    api->in_pack_value = SwitchUserInp_PI;

    return api;
} // plugin_factory
