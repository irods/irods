#ifndef IRODS_RS_COMM_QUERY_HPP
#define IRODS_RS_COMM_QUERY_HPP

#include "rcConnect.h"

namespace irods {

bool is_privileged_client(const rsComm_t& _comm) noexcept;

bool is_privileged_proxy(const rsComm_t& _comm) noexcept;

} // namespace irods

#endif // IRODS_RS_COMM_QUERY_HPP
