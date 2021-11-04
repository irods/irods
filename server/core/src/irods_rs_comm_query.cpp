#include "irods/irods_rs_comm_query.hpp"

#include "irods/irods_exception.hpp"
#include "irods/rodsErrorTable.h"

#include <fmt/format.h>

namespace irods {

bool is_privileged_client(const rsComm_t& _comm) noexcept
{
    return LOCAL_PRIV_USER_AUTH == _comm.clientUser.authInfo.authFlag;
}

bool is_privileged_proxy(const rsComm_t& _comm) noexcept
{
    return LOCAL_PRIV_USER_AUTH == _comm.proxyUser.authInfo.authFlag;
}

void throw_on_insufficient_privilege_for_proxy_user(const rsComm_t& _comm, int _proxy_priv)
{
    // If the proxy user is a rodsadmin in the local zone, it can do whatever it wants.
    if (_proxy_priv >= LOCAL_PRIV_USER_AUTH) {
        return;
    }

    // A user is allowed to authenticate itself.
    if (std::string_view{_comm.proxyUser.userName} == _comm.clientUser.userName) {
        return;
    }

    // A proxy user with remote priviliged user authorization can only authenticate client
    // users from the same zone.
    if (_proxy_priv >= REMOTE_PRIV_USER_AUTH &&
        std::string_view{_comm.proxyUser.rodsZone} == _comm.clientUser.rodsZone) {
        return;
    }

    // This is a proxy user attempting to authenticate a client user with insufficient
    // permissions to do so.
    THROW(SYS_PROXYUSER_NO_PRIV, fmt::format(
          "Proxy user [{}] is not allowed to authenticate client user [{}].",
          _comm.proxyUser.userName, _comm.clientUser.userName));
} // throw_on_insufficient_privilege_for_proxy_user

} // namespace irods

