#include "irods_rs_comm_query.hpp"

namespace irods {

bool is_privileged_client(const rsComm_t& _comm) noexcept
{
    return LOCAL_PRIV_USER_AUTH == _comm.clientUser.authInfo.authFlag;
}

bool is_privileged_proxy(const rsComm_t& _comm) noexcept
{
    return LOCAL_PRIV_USER_AUTH == _comm.proxyUser.authInfo.authFlag;
}

} // namespace irods

