#ifndef IRODS_SERVER_SIDE_COPY_DELEGATION_HPP
#define IRODS_SERVER_SIDE_COPY_DELEGATION_HPP

#include "irods/rcConnect.h"
#include "irods/dataObjInpOut.h"
#include "irods/rodsConnect.h"

#include <string_view>

namespace irods::server_side_copy
{
    // Only the server which owns the destination's primary L1 descriptor can
    // authorize secondary writes. The marker stops a delegated API call from
    // forwarding itself again.
    inline constexpr char delegated_keyword[] = "multi_1247_copy_delegated";

    auto supports_api_port_copy(RsComm& comm, rodsServerHost_t* host) -> bool;

    auto destination_for_delegation(RsComm& comm, std::string_view source_hierarchy,
                                    std::string_view destination_hierarchy,
                                    rodsLong_t size, const DataObjInp& input)
        -> rodsServerHost_t*;
} // namespace irods::server_side_copy

#endif
