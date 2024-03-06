#include "irods/rs_replica_truncate.hpp"

#include "irods/getRemoteZoneResc.h"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_error.hpp"
#include "irods/irods_exception.hpp"
#include "irods/irods_file_object.hpp"
#include "irods/irods_logger.hpp"
#include "irods/irods_resource_backport.hpp"
#include "irods/irods_resource_redirect.hpp"
#include "irods/irods_rs_comm_query.hpp"
#include "irods/logical_locking.hpp"
#include "irods/replica_truncate.h"
#include "irods/rodsConnect.h"
#include "irods/rsFileTruncate.hpp"
#include "irods/rsModDataObjMeta.hpp"

#define IRODS_REPLICA_ENABLE_SERVER_SIDE_API
#include "irods/data_object_proxy.hpp"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <ctime>
#include <string>
#include <tuple> // For std::tie.

#include <string.h> // For strdup.

namespace
{
    namespace ill = irods::logical_locking;
    using log_api = irods::experimental::log::api;

    auto make_json_output(const std::string& _message, const DataObjInfo* _replica = nullptr) -> nlohmann::json
    {
        constexpr auto json_null = nlohmann::json::value_t::null;

        nlohmann::json output;

        output["message"] = _message;

        if (_replica) {
            output["resource_hierarchy"] = _replica->rescHier;
        }
        else {
            output["resource_hierarchy"] = json_null;
        }

        if (_replica) {
            output["replica_number"] = _replica->replNum;
        }
        else {
            output["replica_number"] = json_null;
        }

        return output;
    } // make_json_output

    auto get_data_object_info_and_resolve_hierarchy(RsComm* _comm,
                                                    DataObjInp* _inp,
                                                    DataObjInfo** _doi_out,
                                                    std::string& _hierarchy_out) -> int
    {
        if (!_comm || !_inp || !_doi_out) {
            return SYS_INTERNAL_NULL_INPUT_ERR;
        }

        *_doi_out = nullptr;

        if (const auto* provided_resc_hier = getValByKey(&_inp->condInput, RESC_HIER_STR_KW); provided_resc_hier) {
            _hierarchy_out = provided_resc_hier;
        }

        DataObjInfo* info_head = nullptr;

        if (_hierarchy_out.empty()) {
            try {
                irods::file_object_ptr file_obj;
                std::tie(file_obj, _hierarchy_out) =
                    irods::resolve_resource_hierarchy(irods::WRITE_OPERATION, _comm, *_inp, &info_head);
            }
            catch (const irods::exception& e) {
                if (info_head) {
                    freeAllDataObjInfo(info_head);
                }

                // If the data object does not exist, then the exception will contain error code of CAT_NO_ROWS_FOUND.
                if (e.code() == CAT_NO_ROWS_FOUND) {
                    return OBJ_PATH_DOES_NOT_EXIST;
                }

                return static_cast<int>(e.code());
            }

            addKeyVal(&_inp->condInput, RESC_HIER_STR_KW, _hierarchy_out.c_str());
        }
        else {
            // If the resource hierarchy has already been resolved, we still need the data object information. The
            // file_object_factory is used here in order to ensure that the same information is used for both cases.
            irods::file_object_ptr file_obj{new irods::file_object{}};
            if (const auto err = irods::file_object_factory(_comm, _inp, file_obj, &info_head); !err.ok()) {
                if (info_head) {
                    freeAllDataObjInfo(info_head);
                }

                return static_cast<int>(err.code());
            }
        }

        // If for whatever reason the function calls above did not produce an error but did not return a pointer to the
        // data object information, throw an error here.
        if (!info_head) {
            return SYS_INTERNAL_NULL_INPUT_ERR;
        }

        // An error should be returned if the resolved hierarchy had a locked or intermediate replica. However, not all
        // code paths resolve a resource hierarchy and it is inexpensive to check, regardless.
        if (const auto ec = ill::try_lock(*info_head, ill::lock_type::write, _hierarchy_out); ec < 0) {
            return ec;
        }

        *_doi_out = info_head;

        return 0;
    } // get_data_object_info_and_resolve_hierarchy

    auto truncate_replica(RsComm* _comm, const DataObjInp* _inp, const DataObjInfo* _replica) -> int
    {
        if (!_comm || !_inp || !_replica) {
            return SYS_INTERNAL_NULL_INPUT_ERR;
        }

        std::string location;
        if (const auto err = irods::get_loc_for_hier_string(_replica->rescHier, location); !err.ok()) {
            return static_cast<int>(err.code());
        }

        fileOpenInp_t file_open_inp{};
        std::strncpy(file_open_inp.fileName, _replica->filePath, MAX_NAME_LEN);
        std::strncpy(file_open_inp.resc_hier_, _replica->rescHier, MAX_NAME_LEN);
        std::strncpy(file_open_inp.addr.hostAddr, location.c_str(), NAME_LEN);
        file_open_inp.dataSize = _inp->dataSize;
        return rsFileTruncate(_comm, &file_open_inp);
    } // truncate_replica

    auto update_system_metadata_for_data_object(RsComm* _comm, const DataObjInp* _inp, const DataObjInfo* _info) -> int
    {
        ModDataObjMetaInp mod_data_obj_meta_inp{};

        auto kvp = KeyValPair{};
        const auto clear_kvp = irods::at_scope_exit{[&kvp] { clearKeyVal(&kvp); }};
        auto reg_param = irods::experimental::make_key_value_proxy(kvp);

        reg_param[DATA_SIZE_KW] = std::to_string(_inp->dataSize);
        reg_param[CHKSUM_KW] = "";
        reg_param[OPEN_TYPE_KW] = std::to_string(OPEN_FOR_WRITE_TYPE);
        reg_param[ALL_REPL_STATUS_KW] = "";
        reg_param[DATA_MODIFY_KW] = std::to_string(time(nullptr));
        if (getValByKey(&_inp->condInput, ADMIN_KW)) {
            reg_param[ADMIN_KW] = "";
        }

        // We must const_cast here because rsModDataObjMeta does not accept a constant input. Therefore, the input must
        // be non-const, so the members inside the input structure must also be non-const.
        mod_data_obj_meta_inp.dataObjInfo = const_cast<DataObjInfo*>(_info);
        mod_data_obj_meta_inp.regParam = reg_param.get();
        return rsModDataObjMeta(_comm, &mod_data_obj_meta_inp);
    } // update_system_metadata_for_data_object
} // anonymous namespace

auto rs_replica_truncate(RsComm* _comm, DataObjInp* _inp, char** _out) -> int
{
    if (!_comm || !_inp || !_out) {
        static const auto msg = fmt::format("{}: Null input parameter.", __func__);
        log_api::info(msg);
        return USER__NULL_INPUT_ERR;
    }

    *_out = nullptr;

    // Do not allow DEST_RESC_NAME_KW to be used. It is confusing when RESC_NAME_KW is also supported and the API is
    // only dealing with one replica.
    if (const auto* destination_resource = getValByKey(&_inp->condInput, DEST_RESC_NAME_KW); destination_resource) {
        static const auto msg = fmt::format("{}: [{}] keyword not supported.", __func__, DEST_RESC_NAME_KW);
        log_api::info(msg);
        *_out = strdup(make_json_output(msg).dump().c_str());
        return SYS_INVALID_INPUT_PARAM;
    }

    // Do not allow DEST_RESC_HIER_STR_KW to be used. It is confusing when RESC_HIER_STR_KW is also supported and the
    // API is only dealing with one replica.
    if (const auto* destination_hierarchy = getValByKey(&_inp->condInput, DEST_RESC_HIER_STR_KW); destination_hierarchy)
    {
        const auto msg = fmt::format("{}: [{}] keyword not supported.", __func__, DEST_RESC_HIER_STR_KW);
        log_api::info(msg);
        *_out = strdup(make_json_output(msg).dump().c_str());
        return SYS_INVALID_INPUT_PARAM;
    }

    const auto* resource_name = getValByKey(&_inp->condInput, RESC_NAME_KW);
    const auto* replica_number = getValByKey(&_inp->condInput, REPL_NUM_KW);

    // Do not allow RESC_NAME_KW and REPL_NUM_KW to be used at the same time.
    if (resource_name && replica_number) {
        static const auto msg =
            fmt::format("{}: [{}] and [{}] keywords cannot be used together.", __func__, RESC_NAME_KW, REPL_NUM_KW);
        log_api::info(msg);
        *_out = strdup(make_json_output(msg).dump().c_str());
        return SYS_INVALID_INPUT_PARAM;
    }

    // Check for admin keyword and only allow its usage if the connected client user is privileged.
    if (getValByKey(&_inp->condInput, ADMIN_KW) && !irods::is_privileged_client(*_comm)) {
        const auto msg = fmt::format("{}: [{}] keyword used by non-admin user [{}#{}].",
                                     __func__,
                                     ADMIN_KW,
                                     _comm->clientUser.userName,
                                     _comm->clientUser.rodsZone);
        log_api::error(msg);
        *_out = strdup(make_json_output(msg).dump().c_str());
        return CAT_INSUFFICIENT_PRIVILEGE_LEVEL;
    }

    // Do not free this structure because it is a pointer to a global list of rodsServerHost structs which must
    // persist for the life of the agent.
    rodsServerHost* host_info = nullptr;
    const auto remoteFlag = getAndConnRemoteZone(_comm, _inp, &host_info, REMOTE_OPEN);
    if (remoteFlag < 0) {
        const auto msg =
            fmt::format("{}: getAndConnRemoteZone failed for [{}]: {}", __func__, _inp->objPath, remoteFlag);
        log_api::error(msg);
        *_out = strdup(make_json_output(msg).dump().c_str());
        return remoteFlag;
    }

    // Let the remote zone handle it.
    if (REMOTE_HOST == remoteFlag) {
        return rc_replica_truncate(host_info->conn, _inp, _out);
    }

    try {
        // Get the data object information and resolve the resource hierarchy. If an error occurs, this function
        // should throw. Cannot use structured bindings with these types.
        DataObjInfo* doi = nullptr;
        std::string resolved_hierarchy;
        const auto free_data_object_info = irods::at_scope_exit{[&doi] { freeAllDataObjInfo(doi); }};
        if (const auto ec = get_data_object_info_and_resolve_hierarchy(_comm, _inp, &doi, resolved_hierarchy); ec < 0) {
            const auto msg = fmt::format("{}: Error occurred resolving hierarchy or getting information for [{}]: {}",
                                         __func__,
                                         _inp->objPath,
                                         ec);
            log_api::error(msg);
            *_out = strdup(make_json_output(msg).dump().c_str());
            return ec;
        }

        // Find the replica in the list which resides on the resolved resource hierarchy. If it is not found, something
        // has gone horribly wrong and we should bail.
        const auto* replica = irods::experimental::data_object::find_replica(*doi, resolved_hierarchy);
        if (!replica) {
            const auto msg = fmt::format(
                "{}: [{}] has no replica on resolved hierarchy [{}].", __func__, _inp->objPath, resolved_hierarchy);
            log_api::error(msg);
            *_out = strdup(make_json_output(msg).dump().c_str());
            return SYS_REPLICA_DOES_NOT_EXIST;
        }

        // If the user specified a resource and it is not in the resolved hierarchy, this is considered an error.
        if (resource_name && !irods::hierarchy_parser{replica->rescHier}.resc_in_hier(resource_name)) {
            const auto msg = fmt::format(
                "{}: Hierarchy descending from specified resource name [{}] does not have a replica of [{}] "
                "or the replica is inaccessible at this time.",
                __func__,
                resource_name,
                _inp->objPath);
            log_api::error(msg);
            *_out = strdup(make_json_output(msg, replica).dump().c_str());
            return SYS_REPLICA_INACCESSIBLE;
        }

        // If the target replica is already of the specified size, there is nothing to do.
        if (replica->dataSize == _inp->dataSize) {
            const auto msg = fmt::format("{}: Replica of [{}] on [{}] already has size [{}].",
                                         __func__,
                                         _inp->objPath,
                                         resolved_hierarchy,
                                         _inp->dataSize);
            log_api::debug(msg);
            *_out = strdup(make_json_output(msg, replica).dump().c_str());
            return 0;
        }

        if (const auto ec = truncate_replica(_comm, _inp, replica); ec < 0) {
            const auto msg = fmt::format("{}: Error occurred while truncating replica of [{}] on [{}]: {}",
                                         __func__,
                                         replica->objPath,
                                         replica->rescHier,
                                         ec);
            log_api::debug(msg);
            *_out = strdup(make_json_output(msg, replica).dump().c_str());
            return ec;
        }

        if (const auto ec = update_system_metadata_for_data_object(_comm, _inp, replica); ec < 0) {
            const auto msg =
                fmt::format("{}: Error occurred while updating system metadata for replica of [{}] on [{}]: {}",
                            __func__,
                            replica->objPath,
                            replica->rescHier,
                            ec);
            log_api::debug(msg);
            *_out = strdup(make_json_output(msg, replica).dump().c_str());
            return ec;
        }
    }
    catch (const irods::exception& e) {
        log_api::error("{}: Failed to truncate [{}]: {}", __func__, _inp->objPath, e.client_display_what());
        return static_cast<int>(e.code());
    }
    catch (const std::exception& e) {
        log_api::error("{}: Exception occurred truncating [{}]: {}", __func__, _inp->objPath, e.what());
        return SYS_INTERNAL_ERR;
    }

    return 0;
} // rs_replica_truncate
