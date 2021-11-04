#ifndef IRODS_RS_COMM_QUERY_HPP
#define IRODS_RS_COMM_QUERY_HPP

#include "irods/rcConnect.h"

namespace irods {

bool is_privileged_client(const rsComm_t& _comm) noexcept;

bool is_privileged_proxy(const rsComm_t& _comm) noexcept;

/// \brief Throw when the proxy user does not have the right to act for the client user.
///
/// \param[in] _comm The communication struct containing the proxy and client user information.
/// \param[in] _proxy_priv The privilege level of the proxy user in the local zone.
///
/// \throws irods::exception In all but the following scenarios:\parblock
///     - The proxy user is a privileged user in the local zone
///     - The proxy user is acting on behalf of itself (i.e. the proxy user is the client user)
///     - The proxy user is a privileged user in a remote zone proxying for a remote client user
///       from the same zone
/// \endparblock
///
/// \since 4.3.0
void throw_on_insufficient_privilege_for_proxy_user(const rsComm_t& _comm, int _proxy_priv);

} // namespace irods

#endif // IRODS_RS_COMM_QUERY_HPP
