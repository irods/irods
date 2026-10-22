#include "irods/server_side_copy_delegation.hpp"

#include "irods/get_library_features.h"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_resource_backport.hpp"
#include "irods/library_features.h"
#include "irods/miscServerFunct.hpp"

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <exception>
#include <string>

namespace irods::server_side_copy
{
    namespace
    {
        auto supports_feature(RsComm& comm, rodsServerHost_t* host,
                              const char* feature, long minimum) -> bool
        {
            if (!host) {
                return false;
            }
            if (host->localFlag == LOCAL_HOST) {
                return true; // The local server is running this implementation.
            }
            if (svrToSvrConnect(&comm, host) < 0 || !host->conn) {
                return false;
            }
            char* response{};
            const int ec = rc_get_library_features(host->conn, &response);
            const irods::at_scope_exit release_response{[&] { std::free(response); }};
            if (ec != 0 || !response) {
                return false;
            }
            try {
                const auto features = nlohmann::json::parse(response);
                return features.value(feature, 0L) >= minimum;
            }
            catch (const std::exception&) {
                return false;
            }
        }
    } // anonymous namespace

    auto supports_api_port_copy(RsComm& comm, rodsServerHost_t* host) -> bool
    {
        return supports_feature(comm, host,
                                "IRODS_LIBRARY_FEATURE_AUTHENTICATE_CLIENT",
                                IRODS_LIBRARY_FEATURE_AUTHENTICATE_CLIENT);
    }

    auto destination_for_delegation(RsComm& comm, std::string_view source_hierarchy,
                                    std::string_view destination_hierarchy,
                                    rodsLong_t size, const DataObjInp& input)
        -> rodsServerHost_t*
    {
        if (size <= 32 * 1024 * 1024 || input.numThreads == NO_THREADING ||
            input.specColl || getValByKey(&input.condInput, delegated_keyword) ||
            getValByKey(&input.condInput, TICKET_KW) ||
            getValByKey(&input.condInput, NO_CHK_COPY_LEN_KW) ||
            getValByKey(&input.condInput, NO_PARA_OP_KW) ||
            getValByKey(&input.condInput, ALL_KW) ||
            getValByKey(&input.condInput, STREAMING_KW) ||
            std::string_view{comm.clientUser.rodsZone} != comm.myEnv.rodsZone ||
            std::string_view{comm.clientUser.userName} == PUBLIC_USER_NAME ||
            source_hierarchy.empty() || destination_hierarchy.empty() ||
            source_hierarchy == destination_hierarchy) {
            return nullptr;
        }

        int source_flag{};
        int destination_flag{};
        rodsServerHost_t* source_host{};
        rodsServerHost_t* destination_host{};
        if (!irods::get_host_for_hier_string(std::string{source_hierarchy},
                                              source_flag, source_host).ok() ||
            !irods::get_host_for_hier_string(std::string{destination_hierarchy},
                                              destination_flag, destination_host).ok() ||
            !source_host || !destination_host || source_host == destination_host ||
            destination_flag == LOCAL_HOST) {
            return nullptr;
        }

        // Authenticate the byte-source as a server before forwarding the
        // request. Both APIs are present on unmodified 5.0+ byte peers; the
        // destination must also explicitly advertise the new coordinator.
        if (!supports_api_port_copy(comm, source_host) ||
            !supports_feature(comm, destination_host,
                              "IRODS_LIBRARY_FEATURE_SERVER_SIDE_MULTI_1247_COPY",
                              IRODS_LIBRARY_FEATURE_SERVER_SIDE_MULTI_1247_COPY)) {
            return nullptr;
        }

        return destination_host;
    }
} // namespace irods::server_side_copy
