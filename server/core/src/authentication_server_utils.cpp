#include "irods/authentication_server_utils.hpp"

#include "irods/authentication_client_utils.hpp"
#include "irods/irods_exception.hpp"
#include "irods/irods_rs_comm_query.hpp"
#include "irods/rcConnect.h"
#include "irods/rodsConnect.h"
#include "irods/rodsErrorTable.h"

#define IRODS_USER_ADMINISTRATION_ENABLE_SERVER_SIDE_API
#include "irods/user_administration.hpp"

#include <fmt/format.h>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <string>

namespace irods::authentication
{
    auto generate_session_token() -> std::string
    {
        std::string uuid;
        uuid.reserve(session_token_length);
        uuid = to_string(boost::uuids::random_generator{}());
        return uuid;
    } // generate_session_token

    // The implementation for this function is based on that found in native authentication.
    auto set_privileges_in_rs_comm(RsComm& _comm, const std::string& _user_name, const std::string& _zone_name) -> void
    {
        namespace adm = irods::experimental::administration;

        zoneInfo_t* local_zone_info{};
        if (const auto ec = getLocalZoneInfo(&local_zone_info); ec < 0) {
            THROW(ec, "getLocalZoneInfo failed.");
        }
        // First make sure the proxy and client user's zone information is populated. If not, set it to local zone.
        if ('\0' == _comm.proxyUser.rodsZone[0]) {
            std::strncpy(_comm.proxyUser.rodsZone, local_zone_info->zoneName, NAME_LEN);
        }
        if ('\0' == _comm.clientUser.rodsZone[0]) {
            std::strncpy(_comm.clientUser.rodsZone, local_zone_info->zoneName, NAME_LEN);
        }
        // Get the user type of the user whose password was just verified.
        const auto user = adm::user{_user_name, _zone_name};
        const auto user_type = adm::server::type(_comm, user);
        if (!user_type) {
            THROW(CAT_INVALID_USER_TYPE, fmt::format("Failed to get user type for [{}#{}].", user.name, user.zone));
        }
        // Set the privilege level based on the returned user type and whether user is local to this zone.
        int user_privilege_level = NO_USER_AUTH;
        switch (*user_type) {
            case adm::user_type::rodsadmin:
                user_privilege_level =
                    (user.zone != local_zone_info->zoneName) ? REMOTE_PRIV_USER_AUTH : LOCAL_PRIV_USER_AUTH;
                break;
            case adm::user_type::groupadmin:
                [[fallthrough]];
            case adm::user_type::rodsuser:
                user_privilege_level = (user.zone != local_zone_info->zoneName) ? REMOTE_USER_AUTH : LOCAL_USER_AUTH;
                break;
            default:
                THROW(CAT_INVALID_USER_TYPE,
                      fmt::format("User [{}#{}] has invalid user type [{}].",
                                  user.name,
                                  user.zone,
                                  static_cast<int>(*user_type)));
        }
        // Now set client user privilege level. If the user is acting on behalf of itself, just use the same
        // privilege level for both the proxy and client users.
        int client_user_privilege_level = NO_USER_AUTH;
        if (0 == strcmp(_comm.proxyUser.userName, _comm.clientUser.userName) &&
            0 == strcmp(_comm.proxyUser.rodsZone, _comm.clientUser.rodsZone))
        {
            client_user_privilege_level = user_privilege_level;
        }
        else {
            const auto client_user = adm::user{_comm.clientUser.userName, _comm.clientUser.rodsZone};
            const auto client_user_type = adm::server::type(_comm, client_user);
            if (!client_user_type) {
                THROW(CAT_INVALID_USER_TYPE,
                      fmt::format(
                          "Failed to get user type for client user [{}#{}].", client_user.name, client_user.zone));
            }
            switch (*client_user_type) {
                case adm::user_type::rodsadmin:
                    client_user_privilege_level =
                        (client_user.zone != local_zone_info->zoneName) ? REMOTE_PRIV_USER_AUTH : LOCAL_PRIV_USER_AUTH;
                    break;
                case adm::user_type::groupadmin:
                    [[fallthrough]];
                case adm::user_type::rodsuser:
                    client_user_privilege_level =
                        (client_user.zone != local_zone_info->zoneName) ? REMOTE_USER_AUTH : LOCAL_USER_AUTH;
                    break;
                default:
                    THROW(CAT_INVALID_USER_TYPE,
                          fmt::format("Client user [{}#{}] has invalid user type [{}].",
                                      client_user.name,
                                      client_user.zone,
                                      static_cast<int>(*client_user_type)));
            }
        }
        irods::throw_on_insufficient_privilege_for_proxy_user(_comm, user_privilege_level);
        _comm.proxyUser.authInfo.authFlag = user_privilege_level;
        _comm.clientUser.authInfo.authFlag = client_user_privilege_level;
    } // set_privileges_in_rs_comm
} // namespace irods::authentication
