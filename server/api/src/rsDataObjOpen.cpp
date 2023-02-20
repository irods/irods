#include "irods/apiNumber.h"
#include "irods/dataObjClose.h"
#include "irods/dataObjCreate.h"
#include "irods/dataObjCreateAndStat.h"
#include "irods/dataObjInpOut.h"
#include "irods/dataObjLock.h"
#include "irods/dataObjOpen.h"
#include "irods/dataObjOpenAndStat.h"
#include "irods/dataObjOpr.hpp"
#include "irods/dataObjRepl.h"
#include "irods/dataObjUnlink.h"
#include "irods/fileCreate.h"
#include "irods/fileOpen.h"
#include "irods/getRemoteZoneResc.h"
#include "irods/getRescQuota.h"
#include "irods/icatHighLevelRoutines.hpp"
#include "irods/irods_exception.hpp"
#include "irods/irods_get_l1desc.hpp"
#include "irods/irods_linked_list_iterator.hpp"
#include "irods/irods_resource_types.hpp"
#include "irods/objDesc.hpp"
#include "irods/objInfo.h"
#include "irods/objMetaOpr.hpp"
#include "irods/miscServerFunct.hpp"
#include "irods/physPath.hpp"
#include "irods/rcGlobalExtern.h"
#include "irods/rcMisc.h"
#include "irods/regDataObj.h"
#include "irods/regReplica.h"
#include "irods/resource.hpp"
#include "irods/rodsErrorTable.h"
#include "irods/rodsLog.h"
#include "irods/rsDataObjClose.hpp"
#include "irods/rsDataObjCreate.hpp"
#include "irods/rsDataObjOpen.hpp"
#include "irods/rsDataObjRepl.hpp"
#include "irods/rsDataObjUnlink.hpp"
#include "irods/rsFileCreate.hpp"
#include "irods/rsFileOpen.hpp"
#include "irods/rsGetRescQuota.hpp"
#include "irods/rsGlobalExtern.hpp"
#include "irods/rsModDataObjMeta.hpp"
#include "irods/rsObjStat.hpp"
#include "irods/rs_register_physical_path.hpp"
#include "irods/rsRegDataObj.hpp"
#include "irods/rsRegReplica.hpp"
#include "irods/rsSubStructFileCreate.hpp"
#include "irods/rsSubStructFileOpen.hpp"
#include "irods/rsUnregDataObj.hpp"
#include "irods/specColl.hpp"
#include "irods/subStructFileCreate.h"
#include "irods/subStructFileOpen.h"

// =-=-=-=-=-=-=-
#include "irods/finalize_utilities.hpp"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_hierarchy_parser.hpp"
#include "irods/irods_log.hpp"
#include "irods/irods_resource_backport.hpp"
#include "irods/irods_resource_redirect.hpp"
#include "irods/irods_server_api_call.hpp"
#include "irods/irods_server_properties.hpp"
#include "irods/irods_stacktrace.hpp"
#include "irods/key_value_proxy.hpp"
#include "irods/replica_access_table.hpp"
#include "irods/replica_state_table.hpp"
#include "irods/scoped_privileged_client.hpp"

#define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
#include "irods/filesystem.hpp"

#define IRODS_QUERY_ENABLE_SERVER_SIDE_API
#include "irods/irods_query.hpp"

#define IRODS_REPLICA_ENABLE_SERVER_SIDE_API
#include "irods/data_object_proxy.hpp"

#include "irods/logical_locking.hpp"

#include <fmt/format.h>

#include <chrono>
#include <stdexcept>

#include <sys/types.h>
#include <unistd.h>

namespace
{
    // clang-format off
    namespace ill           = irods::logical_locking;
    namespace fs            = irods::experimental::filesystem;
    namespace id            = irods::experimental::data_object;
    namespace ir            = irods::experimental::replica;
    namespace rat           = irods::experimental::replica_access_table;
    namespace rst           = irods::replica_state_table;

    using replica_proxy     = irods::experimental::replica::replica_proxy<DataObjInfo>;
    using data_object_proxy = irods::experimental::data_object::data_object_proxy<DataObjInfo>;
    using log_api           = irods::experimental::log::api;
    // clang-format on

    constexpr auto minimum_valid_file_descriptor = 3;

    // Instructs how "update_replica_access_table" should update the
    // replica access table.
    enum class update_operation
    {
        create,
        update
    };

    auto update_replica_access_table(rsComm_t& _conn,
                                     update_operation _op,
                                     int _l1desc_index,
                                     const dataObjInp_t& _input) -> int
    {
        const fs::path p = _input.objPath;
        const irods::experimental::key_value_proxy kvp{_input.condInput};

        rat::data_id_type data_id;
        rat::replica_number_type replica_number;

        try {
            const auto gql = fmt::format("select DATA_ID, DATA_REPL_NUM "
                                         "where"
                                         " COLL_NAME = '{}' and"
                                         " DATA_NAME = '{}' and"
                                         " DATA_RESC_HIER = '{}'",
                                         p.parent_path().c_str(),
                                         p.object_name().c_str(),
                                         kvp.at(RESC_HIER_STR_KW).value());

            for (auto&& row : irods::query{&_conn, gql}) {
                data_id = std::stoull(row[0]);
                replica_number = std::stoul(row[1]);
            }
        }
        catch (const std::out_of_range&) {
            irods::log(LOG_NOTICE, fmt::format("[{}:{}] - Could not convert string to integer", __FUNCTION__, __LINE__));

            return SYS_INTERNAL_ERR;
        }

        auto& l1desc = L1desc[_l1desc_index];

        try {
            if (update_operation::create == _op) {
                l1desc.replica_token = rat::create_new_entry(data_id, replica_number, getpid());
            }
            else {
                auto token = kvp.at(REPLICA_TOKEN_KW).value();
                rat::append_pid(token.data(), data_id, replica_number, getpid());
                l1desc.replica_token = token;
            }

            irods::log(LOG_DEBUG, fmt::format(
                "[{}:{}] - [id=[{}], repl_num=[{}], replica_token=[{}]]",
                __FUNCTION__, __LINE__, data_id, replica_number, l1desc.replica_token));

            return 0;
        }
        catch (const rat::replica_access_table_error& e) {
            log_api::error(e.what());

            return SYS_INTERNAL_ERR;
        }
    } // update_replica_access_table

    void enable_creation_of_additional_replicas(rsComm_t& _comm)
    {
        // rxDataObjOpen has the freedom to create replicas on demand. To enable this,
        // it must always set the following flag. This special flag instructs rsPhyPathReg
        // to register a new replica if an existing replica already exists.
        irods::experimental::key_value_proxy{_comm.session_props}[REG_REPL_KW] = "";
    } // enable_creation_of_additional_replicas

    auto leaf_resource_is_bundleresc(const std::string_view _hierarchy)
    {
        std::string resc_class{};
        const irods::error prop_err = irods::get_resource_property<std::string>(
            resc_mgr.hier_to_leaf_id(_hierarchy), irods::RESOURCE_CLASS, resc_class);
        return prop_err.ok() && irods::RESOURCE_CLASS_BUNDLE == resc_class;
    } // leaf_resource_is_bundleresc

    auto create_new_replica(rsComm_t& _comm, dataObjInp_t& _inp, DataObjInfo* _existing_replica_list) -> int
    {
        auto cond_input = irods::experimental::make_key_value_proxy(_inp.condInput);
        const auto hierarchy = cond_input.at(RESC_HIER_STR_KW).value();

        const auto remove_reg_repl_keyword_from_session_props = irods::at_scope_exit{[&_comm] {
            irods::experimental::key_value_proxy{_comm.session_props}.erase(REG_REPL_KW);
        }};

        try {
            const auto brand_new_data_object = nullptr == _existing_replica_list;

            // The creation of additional replicas should only be enabled if it is known that a new
            // replica for an existing data object will be created. Otherwise, it does not need to
            // be enabled and informs the registration API about how the registration should occur.
            if (!brand_new_data_object) {
                enable_creation_of_additional_replicas(_comm);
            }

            const auto creating_new_replica = [&]() -> bool
            {
                return brand_new_data_object ||
                       !id::find_replica(id::make_data_object_proxy(*_existing_replica_list), hierarchy);
            }();

            // This is the case where a create was requested but the replica already exists
            // and the operation is supposed to turn into an overwrite of the existing data object.
            // Return a value of 0 here and the caller will continue with the open operation.
            if (!creating_new_replica) {
                irods::log(LOG_DEBUG, fmt::format(
                    "[{}:{}] - switching to overwrite an existing replica [path=[{}], hierarchy=[{}]]",
                    __FUNCTION__, __LINE__, _inp.objPath, hierarchy));
                cond_input[DEST_RESC_NAME_KW] = irods::hierarchy_parser{hierarchy.data()}.first_resc();
                cond_input[OPEN_TYPE_KW] = std::to_string(OPEN_FOR_WRITE_TYPE);

                return 0;
            }

            irods::log(LOG_DEBUG, fmt::format(
                "[{}:{}] - creating a new replica [path=[{}], hierarchy=[{}]]",
                __FUNCTION__, __LINE__, _inp.objPath, hierarchy));

            const auto special_collection_type = irods::get_special_collection_type_for_data_object(_comm, _inp);
            if (special_collection_type < 0) {
                return special_collection_type;
            }

            switch (special_collection_type) {
                case NO_SPEC_COLL:
                    // This is not a special collection - continue down the normal path
                    break;

                case LINKED_COLL:
                    // Linked collection should have been translated by this point - return error.
                    return SYS_COLL_LINK_PATH_ERR;

                default:
                    // This is a special collection so it has special creation logic
                    return irods::data_object_create_in_special_collection(&_comm, _inp);
            }

            // This conjures a brand new replica using information from the input parameters. This is ultimately
            // superceded by the structures used in the registration API and database operation, but is necessary
            // here to inform the database about the replica we intend to create.
            auto [new_replica, new_replica_lm] = ir::make_replica_proxy();
            new_replica.logical_path(_inp.objPath);
            new_replica.replica_status(INTERMEDIATE_REPLICA);
            new_replica.hierarchy(hierarchy);
            new_replica.resource_id(resc_mgr.hier_to_leaf_id(new_replica.hierarchy()));
            new_replica.resource(irods::hierarchy_parser{new_replica.hierarchy().data()}.first_resc());
            new_replica.mode(std::to_string(_inp.createMode));
            new_replica.type(cond_input.contains(DATA_TYPE_KW) ? cond_input.at(DATA_TYPE_KW).value() : GENERIC_DT_STR);

            if (cond_input.contains(DATA_ID_KW)) {
                new_replica.data_id(std::atoll(cond_input.at(DATA_ID_KW).value().data()));
            }

            if (cond_input.contains(FILE_PATH_KW)) {
                new_replica.physical_path(cond_input.at(FILE_PATH_KW).value());
            }

            cond_input[OPEN_TYPE_KW] = std::to_string(CREATE_TYPE);
            const int l1_index = irods::populate_L1desc_with_inp(_inp, *new_replica.get(), _inp.dataSize);

            if (l1_index < minimum_valid_file_descriptor) {
                if (l1_index < 0) {
                    irods::log(LOG_ERROR, fmt::format(
                        "[{}:{}] - failed to allocate L1 descriptor "
                        "[error_code=[{}], path=[{}], hierarchy=[{}]",
                        __FUNCTION__, __LINE__, l1_index, _inp.objPath, hierarchy));

                    return l1_index;
                }

                irods::log(LOG_ERROR, fmt::format(
                    "[{}:{}] - L1 descriptor out of range "
                    "[fd=[{}], path=[{}], hierarchy=[{}]",
                    __FUNCTION__, __LINE__, l1_index, _inp.objPath, hierarchy));

                return SYS_FILE_DESC_OUT_OF_RANGE;
            }

            auto& l1desc = L1desc[l1_index];

            if (const int ec = getFilePathName(&_comm, l1desc.dataObjInfo, l1desc.dataObjInp); ec < 0) {
                freeL1desc(l1_index);

                irods::log(LOG_ERROR, fmt::format(
                    "[{}] - failed to get file path name "
                    "[error_code=[{}], path=[{}], hierarchy=[{}]",
                    __FUNCTION__, __LINE__, ec, _inp.objPath, hierarchy));

                return ec;
            }

            auto l1_cond_input = irods::experimental::make_key_value_proxy(l1desc.dataObjInp->condInput);
            l1_cond_input[REGISTER_AS_INTERMEDIATE_KW] = "";
            l1_cond_input[FILE_PATH_KW] = l1desc.dataObjInfo->filePath;
            l1_cond_input[DATA_SIZE_KW] = std::to_string(0);

            const auto admin_op = l1_cond_input.contains(ADMIN_KW);

            // We need to lock the data object before creating the new replica. Otherwise,
            // multiple replicas can be created on the same data object simultaneously in an
            // uncoordinated fashion. The following steps must happen:
            //   1. Insert existing data object into RST
            //   2. Write-lock data object
            //   3. Create new replica (starts in intermediate status)
            //   4. Insert new replica into RST
            auto object_locked = false;
            if (!brand_new_data_object) {
                const auto replica_token = cond_input.contains(REPLICA_TOKEN_KW) ? cond_input.at(REPLICA_TOKEN_KW).value().data() : "";
                if (const auto ret = ill::try_lock(*_existing_replica_list, ill::lock_type::write, hierarchy, replica_token); ret < 0) {
                    freeL1desc(l1_index);
                    return ret;
                }

                const auto obj = id::make_data_object_proxy(*_existing_replica_list);
                if (const auto insert_ec = rst::insert(obj); insert_ec < 0) {
                    freeL1desc(l1_index);
                    return insert_ec;
                }

                if (const int lock_ec = ill::lock_and_publish(_comm, {obj.data_id(), admin_op}, ill::lock_type::write); lock_ec < 0) {
                    irods::log(LOG_ERROR, fmt::format(
                        "[{}:{}] - Failed to lock data object on create "
                        "[error_code=[{}], path=[{}], hierarchy=[{}]",
                        __FUNCTION__, __LINE__, lock_ec, _inp.objPath, hierarchy));

                    if (const int unlock_ec = ill::unlock_and_publish(_comm, {obj.data_id(), admin_op}, ill::restore_status); unlock_ec < 0) {
                        irods::log(LOG_ERROR, fmt::format("[{}:{}] - failed to unlock data object [error_code={}]", __FUNCTION__, __LINE__, unlock_ec));
                    }

                    freeL1desc(l1_index);

                    return lock_ec;
                }

                object_locked = true;
            }

            // Registers the physical path as a replica of the given data object information (which may or may not be
            // a new data object). The returned information is then used as the data object information in the open
            // L1 descriptor as some system metadata generated in the database operation were not available when the
            // data object information was populated above.
            char* info_json_str{};
            const auto free_buf = irods::at_scope_exit{[&info_json_str] {
                std::free(info_json_str);
                info_json_str = nullptr;
            }};

            if (const int ec = rs_register_physical_path(&_comm, l1desc.dataObjInp, &info_json_str); ec < 0 || !info_json_str) {
                irods::log(LOG_ERROR, fmt::format(
                    "[{}:{}] - failed to register physical path "
                    "[error_code=[{}], path=[{}], hierarchy=[{}]",
                    __FUNCTION__, __LINE__, ec, _inp.objPath, hierarchy));

                if (object_locked) {
                    if (const int unlock_ec = ill::unlock_and_publish(_comm, {_existing_replica_list->dataId, admin_op}, ill::restore_status); unlock_ec < 0) {
                        irods::log(LOG_ERROR, fmt::format("[{}:{}] - failed to unlock data object [error_code={}]", __FUNCTION__, __LINE__, unlock_ec));
                    }

                    // Remove the RST entry because the unlock interface does not automatically remove the RST entries.
                    // This is important because the agent for this connection may be re-used for another open operation
                    // which would expect the RST to be cleaned up.
                    if (rst::contains(_existing_replica_list->dataId)) {
                        rst::erase(_existing_replica_list->dataId);
                    }
                }

                freeL1desc(l1_index);

                return ec;
            }

            // Extract the JSON output from the registration API. If the output is not an expected value, a JSON
            // parsing error could occur, so the usual teardown needs to occur here. This is done before the
            // attendant data object info is constructed, so this is a limbo state!
            nlohmann::json info_json;
            try {
                info_json = nlohmann::json::parse(info_json_str);
            }
            catch (const nlohmann::json::exception& e) {
                irods::log(LOG_ERROR, fmt::format(
                    "[{}:{}] - failed to parse JSON output from registration "
                    "[error_code=[{}], path=[{}], hierarchy=[{}]",
                    __FUNCTION__, __LINE__, e.what(), _inp.objPath, hierarchy));

                if (object_locked) {
                    if (const int unlock_ec = ill::unlock_and_publish(_comm, {_existing_replica_list->dataId, admin_op}, ill::restore_status); unlock_ec < 0) {
                        irods::log(LOG_ERROR, fmt::format("[{}:{}] - failed to unlock data object [error_code={}]", __FUNCTION__, __LINE__, unlock_ec));
                    }
                }

                // TODO: Need to get at the data ID somehow in order to unlock the object before unlinking in all cases
                // Unregister replica from catalog
                unregDataObj_t unlink_inp{};
                unlink_inp.dataObjInfo = l1desc.dataObjInfo;
                if (const auto unreg_ec = rsUnregDataObj(&_comm, &unlink_inp); unreg_ec < 0) {
                    irods::log(LOG_ERROR, fmt::format(
                        "[{}:{}] - failed to unregister replica "
                        "[error_code=[{}], path=[{}], hierarchy=[{}]]",
                        __FUNCTION__, __LINE__, unreg_ec,
                        l1desc.dataObjInfo->objPath,
                        l1desc.dataObjInfo->rescHier));
                }

                if (object_locked && rst::contains(_existing_replica_list->dataId)) {
                    rst::erase(_existing_replica_list->dataId);
                }

                freeL1desc(l1_index);

                return JSON_VALIDATION_ERROR;
            }

            // Replace the missing information from the registered replica in the data object information
            // for the open L1 descriptor so that the open operation can refer to the data ID and other
            // bits generated by the database operation. Ownership of the data object information is given
            // to the open L1 descriptor.
            auto [json_generated_replica, lm] = ir::make_replica_proxy(_inp.objPath, info_json);
            freeDataObjInfo(l1desc.dataObjInfo);
            l1desc.dataObjInfo = lm.release();

            // TODO: There is a gap in the C++ standard where structured bindings cannot be captured
            // see for more information: https://stackoverflow.com/questions/46114214/lambda-implicit-capture-fails-with-variable-declared-from-structured-binding
            auto registered_replica = ir::make_replica_proxy(*l1desc.dataObjInfo);

            // Resource hierarchy information is not held in R_DATA_MAIN
            registered_replica.hierarchy(hierarchy);
            registered_replica.resource(irods::hierarchy_parser{registered_replica.hierarchy().data()}.last_resc());

            // Once the replica is registered successfully, make sure the RST entry and L1 descriptor
            // are cleaned up properly in the event of errors. Each return statement should assign the
            // return value to ec so that clean_up executes (or not) appropriately.
            int ec = 0;
            const auto clean_up = irods::at_scope_exit{[&]
                {
                    if (ec < 0) {
                        if (object_locked) {
                            if (const int unlock_ec = ill::unlock_and_publish(_comm, {registered_replica, admin_op, 0}, ill::restore_status); unlock_ec < 0) {
                                irods::log(LOG_ERROR, fmt::format("[{}:{}] - failed to unlock data object [error_code={}]", __FUNCTION__, __LINE__, unlock_ec));
                            }
                        }

                        // Unlink physical file if there is a valid L3 descriptor
                        if (l1desc.l3descInx > 2) {
                            if (const auto unlink_ec = l3Unlink(&_comm, registered_replica.get()); unlink_ec < 0) {
                                irods::log(LOG_ERROR, fmt::format(
                                    "[{}:{}] - failed to physically unlink replica "
                                    "[error_code=[{}], physical path=[{}]]",
                                    __FUNCTION__, __LINE__, unlink_ec, registered_replica.physical_path()));
                            }
                        }

                        // Unregister replica from catalog
                        unregDataObj_t unlink_inp{};
                        unlink_inp.dataObjInfo = registered_replica.get();
                        if (const auto unreg_ec = rsUnregDataObj(&_comm, &unlink_inp); unreg_ec < 0) {
                            irods::log(LOG_ERROR, fmt::format(
                                "[{}:{}] - failed to unregister replica "
                                "[error_code=[{}], path=[{}], hierarchy=[{}]]",
                                __FUNCTION__, __LINE__, unreg_ec,
                                registered_replica.logical_path(),
                                registered_replica.hierarchy()));
                        }

                        if (rst::contains(registered_replica.data_id())) {
                            rst::erase(registered_replica.data_id());
                        }

                        freeL1desc(l1_index);
                    }
                }
            };

            // Insert the newly registered replica into the RST with the other replicas which were inserted and locked before
            if (const auto insert_ec = rst::insert(registered_replica); insert_ec < 0) {
                return ec = insert_ec;
            }

            // The condInput that cares about the KEY_VALUE_PASSTHROUGH_KW is held in the dataObjInfo of the
            // open L1 descriptor, not the dataObjInp. We copy it here so that it holds the needed value.
            if (cond_input.contains(KEY_VALUE_PASSTHROUGH_KW)) {
                auto info_cond_input = irods::experimental::make_key_value_proxy(L1desc[l1_index].dataObjInfo->condInput);
                info_cond_input[KEY_VALUE_PASSTHROUGH_KW] = cond_input.at(KEY_VALUE_PASSTHROUGH_KW).value();
            }

            // Return if the caller has requested that no physical open occur.
            if (cond_input.contains(NO_OPEN_FLAG_KW)) {
                return l1_index;
            }

            const auto l3_index = irods::create_physical_file_for_replica(_comm, *l1desc.dataObjInp, *l1desc.dataObjInfo);
            if (l3_index < 0) {
                irods::log(LOG_ERROR, fmt::format(
                    "[{}:{}] - l3Create failed "
                    "[error_code=[{}], path=[{}], hierarchy=[{}], physical_path=[{}]]",
                    __FUNCTION__, __LINE__,
                    l3_index, _inp.objPath, hierarchy, registered_replica.physical_path()));

                if (const int unlock_ec = ill::unlock_and_publish(_comm, {registered_replica, admin_op, 0}, STALE_REPLICA, ill::restore_status); unlock_ec < 0) {
                    irods::log(LOG_ERROR, fmt::format("[{}:{}] - failed to unlock data object [error_code={}]", __FUNCTION__, __LINE__, unlock_ec));
                }

                return ec = l3_index;
            }

            L1desc[l1_index].l3descInx = l3_index;

            constexpr auto uo = update_operation::create;
            if (const auto rat_ec = update_replica_access_table(_comm, uo, l1_index, _inp); rat_ec < 0) {
                irods::log(LOG_ERROR, fmt::format(
                    "[{}:{}] - [error occurred while updating replica access table] "
                    "[error_code=[{}], path=[{}], hierarchy=[{}]]",
                    __FUNCTION__, __LINE__,
                    rat_ec, _inp.objPath, hierarchy));

                if (const int close_ec = irods::close_replica_and_unlock_data_object(_comm, l1_index, STALE_REPLICA); close_ec < 0) {
                    irods::log(LOG_ERROR, fmt::format(
                        "[{}:{}] - Failed to unlock data object "
                        "[error_code=[{}], path=[{}], hierarchy=[{}]]",
                        __FUNCTION__, __LINE__, close_ec, _inp.objPath, hierarchy));
                }

                return ec = rat_ec;
            }

            return l1_index;
        }
        catch (const irods::exception& e) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - [{}] [error_code=[{}], path=[{}], hierarchy=[{}]",
                __FUNCTION__, __LINE__, e.client_display_what(),
                e.code(), _inp.objPath, hierarchy));

            return e.code();
        }
        catch (const std::exception& e) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - [{}] [path=[{}], hierarchy=[{}]",
                __FUNCTION__, __LINE__, e.what(), _inp.objPath, hierarchy));

            return SYS_INTERNAL_ERR;
        }
        catch (...) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - unknown error has occurred. [path=[{}], hierarchy=[{}]",
                __FUNCTION__, __LINE__, _inp.objPath, hierarchy));

            return SYS_UNKNOWN_ERROR;
        }
    } // create_new_replica

    int stage_bundled_data_to_cache_directory(rsComm_t * rsComm, dataObjInfo_t **subfileObjInfoHead)
    {
        dataObjInfo_t *dataObjInfoHead = *subfileObjInfoHead;
        char* cacheRescName{};
        int status = unbunAndStageBunfileObj(
                        rsComm,
                        dataObjInfoHead->filePath,
                        &cacheRescName);
        if ( status < 0 ) {
            return status;
        }

        /* query the bundle dataObj */
        dataObjInp_t dataObjInp{};
        addKeyVal( &dataObjInp.condInput, RESC_NAME_KW, cacheRescName );
        rstrcpy( dataObjInp.objPath, dataObjInfoHead->objPath, MAX_NAME_LEN );

        dataObjInfo_t* cacheObjInfo{};
        status = getDataObjInfo( rsComm, &dataObjInp, &cacheObjInfo, NULL, 0 );
        clearKeyVal( &dataObjInp.condInput );
        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "%s: getDataObjInfo of subfile %s failed.stat=%d",
                     __FUNCTION__, dataObjInp.objPath, status );
            return status;
        }
        /* que the cache copy at the top */
        queueDataObjInfo(subfileObjInfoHead, cacheObjInfo, 0, 1);
        return status;
    } // stage_bundled_data_to_cache_directory

    int l3Open(rsComm_t *rsComm, int l1descInx)
    {
        dataObjInfo_t* dataObjInfo = L1desc[l1descInx].dataObjInfo;
        if (!dataObjInfo) {
            return SYS_INTERNAL_NULL_INPUT_ERR;
        }

        std::string location{};
        irods::error ret = irods::get_loc_for_hier_string( dataObjInfo->rescHier, location );
        if ( !ret.ok() ) {
            irods::log(LOG_ERROR, fmt::format(
                "{} - failed in get_loc_for_hier_string:[{}]; ec:[{}]",
                __FUNCTION__, ret.result(), ret.code()));
            return ret.code();
        }

        if ( getStructFileType( dataObjInfo->specColl ) >= 0 ) {
            subFile_t subFile{};
            rstrcpy( subFile.subFilePath, dataObjInfo->subPath, MAX_NAME_LEN );
            rstrcpy( subFile.addr.hostAddr, location.c_str(), NAME_LEN );
            subFile.specColl = dataObjInfo->specColl;
            subFile.mode = getFileMode( L1desc[l1descInx].dataObjInp );
            subFile.flags = getFileFlags( l1descInx );
            return rsSubStructFileOpen( rsComm, &subFile );
        }

        fileOpenInp_t fileOpenInp{};
        rstrcpy( fileOpenInp.resc_name_, dataObjInfo->rescName, MAX_NAME_LEN );
        rstrcpy( fileOpenInp.resc_hier_, dataObjInfo->rescHier, MAX_NAME_LEN );
        rstrcpy( fileOpenInp.objPath,    dataObjInfo->objPath, MAX_NAME_LEN );
        rstrcpy( fileOpenInp.addr.hostAddr,  location.c_str(), NAME_LEN );
        rstrcpy( fileOpenInp.fileName, dataObjInfo->filePath, MAX_NAME_LEN );
        fileOpenInp.mode = getFileMode(L1desc[l1descInx].dataObjInp);
        fileOpenInp.flags = getFileFlags(l1descInx);
        rstrcpy( fileOpenInp.in_pdmo, dataObjInfo->in_pdmo, MAX_NAME_LEN );

        copyKeyVal(&dataObjInfo->condInput, &fileOpenInp.condInput);

        const int l3descInx = rsFileOpen(rsComm, &fileOpenInp);
        clearKeyVal( &fileOpenInp.condInput );
        return l3descInx;
    } // l3Open

    auto open_replica(RsComm& _comm, DataObjInp& _inp, replica_proxy& _replica) -> int
    {
        copyKeyVal(&_inp.condInput, _replica.cond_input().get());
        const auto hierarchy = _replica.cond_input().at(RESC_HIER_STR_KW).value();

        // If the size is known (e.g. copy/repl/phymv) the size should be updated
        // in the calling routine. Here, we do not know, so set to special value -1.
        constexpr rodsLong_t unknown_target_size_for_replica = -1;

        const int l1_index = irods::populate_L1desc_with_inp(_inp, *_replica.get(), unknown_target_size_for_replica);

        if (l1_index < minimum_valid_file_descriptor) {
            if (l1_index < 0) {
                irods::log(LOG_ERROR, fmt::format(
                    "[{}:{}] - failed to allocate L1 descriptor "
                    "[error_code=[{}], path=[{}], hierarchy=[{}]",
                    __FUNCTION__, __LINE__, l1_index, _inp.objPath, hierarchy));

                return l1_index;
            }

            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - L1 descriptor out of range "
                "[fd=[{}], path=[{}], hierarchy=[{}]",
                __FUNCTION__, __LINE__, l1_index, _inp.objPath, hierarchy));

            return SYS_FILE_DESC_OUT_OF_RANGE;
        }

        const auto open_for_write = getWriteFlag(_inp.openFlags);
        if (open_for_write) {
            // TODO: is this necessary
            L1desc[l1_index].replStatus = INTERMEDIATE_REPLICA;
        }

        L1desc[l1_index].openType = open_for_write ? OPEN_FOR_WRITE_TYPE : OPEN_FOR_READ_TYPE;

        if (_replica.cond_input().contains(NO_OPEN_FLAG_KW)) {
            return l1_index;
        }

        const int l3_index = l3Open(&_comm, l1_index);
        if (l3_index <= 0) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - failed to physically open replica "
                "[error_code=[{}], path=[{}], hierarchy=[{}]]",
                __FUNCTION__, __LINE__, l3_index, _inp.objPath, _replica.hierarchy()));

            freeL1desc(l1_index);

            return l3_index;
        }

        auto& l1desc = L1desc[l1_index];
        l1desc.l3descInx = l3_index;

        // Return before truncating the file in the catalog - the catalog may not even have a entry for
        // the special collection items, so these are not being considered for updating at this point,
        // especially because special collections are not supported by RST.
        if (_replica.special_collection_info() || leaf_resource_is_bundleresc(_replica.hierarchy())) {
            return l1_index;
        }

        // Update the catalog to reflect the truncation (i.e. set the data size to zero and erase
        // the checksum). It is important that the catalog reflect truncation immediately because
        // operations following the open may depend on the size and checksum of the replica.
        //
        // TODO: optimization... do not touch the catalog -- update the structure and use this in lock_data_object
        if (l1desc.dataObjInp->openFlags & O_TRUNC) {
            if (const auto access_mode = (l1desc.dataObjInp->openFlags & O_ACCMODE);
                access_mode == O_WRONLY || access_mode == O_RDWR)
            {
                const auto admin_operation = _replica.cond_input().contains(ADMIN_KW);

                auto replica = ir::make_replica_proxy(*l1desc.dataObjInfo);

                replica.size(0);
                replica.checksum("");

                rst::update(replica.data_id(), replica.replica_number(), {{"data_size", "0"}, {"data_checksum", ""}});

                if (const auto [ret, ec] = rst::publish::to_catalog(_comm, {replica, admin_operation}); ec < 0) {
                    return ec;
                }

                // Update the replica proxy information.
                _replica.size(0);
                _replica.checksum("");

                // Update the L1 descriptor information.
                l1desc.dataSize = 0;
            }
        }

        return l1_index;
    } // open_replica

    auto get_data_object_info_for_open(RsComm& _comm, DataObjInp& _inp) -> std::tuple<DataObjInfo*, std::string>
    {
        auto cond_input = irods::experimental::make_key_value_proxy(_inp.condInput);

        std::string hierarchy_for_open{};
        if (cond_input.contains(RESC_HIER_STR_KW)) {
            hierarchy_for_open = cond_input.at(RESC_HIER_STR_KW).value().data();
        }
        // If the client specified a leaf resource, then discover the hierarchy and
        // store it in the keyValPair_t. This instructs the iRODS server to create
        // the replica at the specified resource if it does not exist.
        else if (cond_input.contains(LEAF_RESOURCE_NAME_KW)) {
            auto leaf = cond_input.at(LEAF_RESOURCE_NAME_KW).value();
            bool is_coord_resc = false;

            if (const auto err = resc_mgr.is_coordinating_resource(leaf.data(), is_coord_resc); !err.ok()) {
                THROW(err.code(), err.result());
            }

            // Leaf resources cannot be coordinating resources. This essentially checks
            // if the resource has any child resources which is exactly what we're interested in.
            if (is_coord_resc) {
                THROW(USER_INVALID_RESC_INPUT, fmt::format("[{}] is not a leaf resource.", leaf));
            }

            if (const auto err = resc_mgr.get_hier_to_root_for_resc(leaf.data(), hierarchy_for_open); !err.ok()) {
                THROW(err.code(), err.result());
            }
        }

        // Get replica information for data object, resolving hierarchy if necessary
        dataObjInfo_t* info_head{};

        if (hierarchy_for_open.empty()) {
            try {
                std::string operation;

                // NOLINTNEXTLINE(hicpp-signed-bitwise)
                if (0 != (_inp.openFlags & O_CREAT)) {
                    operation = irods::CREATE_OPERATION;
                }
                else if (0 != getWriteFlag(_inp.openFlags)) {
                    operation = irods::WRITE_OPERATION;
                }
                else {
                    operation = irods::OPEN_OPERATION;
                }

                irods::file_object_ptr file_obj;
                std::tie(file_obj, hierarchy_for_open) =
                    irods::resolve_resource_hierarchy(operation, &_comm, _inp, &info_head);
            }
            catch (const irods::exception& e) {
                // If the data object does not exist, then the exception will contain
                // an error code of CAT_NO_ROWS_FOUND.
                if (e.code() == CAT_NO_ROWS_FOUND) {
                    THROW(OBJ_PATH_DOES_NOT_EXIST, fmt::format(
                        "Data object or replica does not exist [error_code={}, path={}].",
                        e.code(), _inp.objPath));
                }

                throw;
            }
        }
        else {
            irods::file_object_ptr file_obj{new irods::file_object()};
            irods::error fac_err = irods::file_object_factory(&_comm, &_inp, file_obj, &info_head);
            if (!fac_err.ok() && CAT_NO_ROWS_FOUND != fac_err.code()) {
                irods::log(fac_err);
            }
        }

        if (!cond_input.contains(RESC_HIER_STR_KW)) {
            cond_input[RESC_HIER_STR_KW] = hierarchy_for_open;
        }

        cond_input[SELECTED_HIERARCHY_KW] = hierarchy_for_open;

        return {info_head, hierarchy_for_open};
    } // get_data_object_info_for_open

    auto apply_static_pep_data_obj_open_pre(RsComm& _comm, DataObjInp& _inp, DataObjInfo** _info_head) -> int
    {
        ruleExecInfo_t rei;
        initReiWithDataObjInp( &rei, &_comm, &_inp );
        rei.doi = *_info_head;

        // make resource properties available as rule session variables
        irods::get_resc_properties_as_kvp(rei.doi->rescHier, rei.condInputData);

        int status = applyRule( "acPreprocForDataObjOpen", NULL, &rei, NO_SAVE_REI );
        clearKeyVal(rei.condInputData);
        free(rei.condInputData);

        if (status < 0) {
            if (rei.status < 0) {
                status = rei.status;
            }

            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - acPreprocForDataObjOpen failed "
                "[error_code=[{}], path=[{}]]",
                __FUNCTION__, __LINE__, status, _inp.objPath));

            return status;
        }

        *_info_head = rei.doi;
        return rei.status;
    } // apply_static_pep_data_obj_open_pre

    auto open_special_replica(RsComm& _comm, DataObjInp& _inp, DataObjInfo* _info) -> int
    {
        auto cond_input = irods::experimental::key_value_proxy(_inp.condInput);

        const std::string_view hierarchy = cond_input.at(RESC_HIER_STR_KW).value();

        try {
            if (const int ec = apply_static_pep_data_obj_open_pre(_comm, _inp, &_info); ec < 0) {
                return ec;
            }

            auto replica = ir::make_replica_proxy(*_info);

            return open_replica(_comm, _inp, replica);
        }
        catch (const irods::exception& e) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - [{}] [error_code=[{}], path=[{}], hierarchy=[{}]",
                __FUNCTION__, __LINE__, e.client_display_what(),
                e.code(), _inp.objPath, hierarchy));

            return e.code();
        }
        catch (const std::exception& e) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - [{}] [path=[{}], hierarchy=[{}]",
                __FUNCTION__, __LINE__, e.what(), _inp.objPath, hierarchy));

            return SYS_INTERNAL_ERR;
        }
        catch (...) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - unknown error has occurred. [path=[{}], hierarchy=[{}]",
                __FUNCTION__, __LINE__, _inp.objPath, hierarchy));

            return SYS_UNKNOWN_ERROR;
        }
    } // open_special_replica

    auto remote_open(rodsServerHost& _server_host, DataObjInp& _inp) -> int
    {
        OpenStat* stat{};
        const auto free_stat = irods::at_scope_exit{[&stat] { if (stat) std::free(stat); }};

        const int remoteL1descInx = rcDataObjOpenAndStat(_server_host.conn, &_inp, &stat);
        if (remoteL1descInx < 0) {
            return remoteL1descInx;
        }

        return allocAndSetL1descForZoneOpr(remoteL1descInx, &_inp, &_server_host, stat);
    } // remote_open

    int rsDataObjOpen_impl(rsComm_t *rsComm, dataObjInp_t *dataObjInp)
    {
        rodsServerHost_t* rodsServerHost{};
        const int remoteFlag = getAndConnRemoteZone(rsComm, dataObjInp, &rodsServerHost, REMOTE_OPEN);
        if (remoteFlag < 0) {
            return remoteFlag;
        }
        else if (REMOTE_HOST == remoteFlag) {
            return remote_open(*rodsServerHost, *dataObjInp);
        }

        DataObjInfo* info_head{};
        std::string hierarchy{};

        try {
            std::tie(info_head, hierarchy) = get_data_object_info_for_open(*rsComm, *dataObjInp);
        }
        catch (const irods::exception& e) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - [{}] [error_code=[{}], path=[{}], hierarchy=[{}]",
                __FUNCTION__, __LINE__, e.client_display_what(),
                e.code(), dataObjInp->objPath, hierarchy));

            return e.code();
        }
        catch (const std::exception& e) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - [{}] [path=[{}], hierarchy=[{}]",
                __FUNCTION__, __LINE__, e.what(), dataObjInp->objPath, hierarchy));

            return SYS_INTERNAL_ERR;
        }
        catch (...) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - unknown error has occurred. [path=[{}], hierarchy=[{}]",
                __FUNCTION__, __LINE__, dataObjInp->objPath, hierarchy));

            return SYS_UNKNOWN_ERROR;
        }

        const auto free_info = irods::at_scope_exit{[&info_head] { if (info_head) freeAllDataObjInfo(info_head); }};

        try {
            if (dataObjInp->openFlags & O_CREAT) {
                // A return code of 0 indicates that the replica already exists and this is in fact
                // an overwrite. Otherwise, a valid L1 descriptor index or an error is returned.
                if (const auto ec = create_new_replica(*rsComm, *dataObjInp, info_head); 0 != ec) {
                    return ec;
                }
            }
        }
        catch (const irods::exception& e) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - [{}] [error_code=[{}], path=[{}], hierarchy=[{}]",
                __FUNCTION__, __LINE__, e.client_display_what(),
                e.code(), dataObjInp->objPath, hierarchy));

            return e.code();
        }
        catch (const std::exception& e) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - [{}] [path=[{}], hierarchy=[{}]",
                __FUNCTION__, __LINE__, e.what(), dataObjInp->objPath, hierarchy));

            return SYS_INTERNAL_ERR;
        }
        catch (...) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - unknown error has occurred. [path=[{}], hierarchy=[{}]",
                __FUNCTION__, __LINE__, dataObjInp->objPath, hierarchy));

            return SYS_UNKNOWN_ERROR;
        }

        if (!info_head) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - requested data object does not exist "
                "[path=[{}], hierarchy=[{}]]",
                __FUNCTION__, __LINE__, dataObjInp->objPath, hierarchy));

            return SYS_REPLICA_DOES_NOT_EXIST;
        }

        // We need to migrate bundled data to the cache directory before opening. Bundled data is not
        // considered for intermediate replicas as it is legacy behavior, so we simply stage the data
        // and open the replica in this case.
        if (leaf_resource_is_bundleresc(hierarchy)) {
            if (const int ec = stage_bundled_data_to_cache_directory(rsComm, &info_head); ec < 0) {
                return ec;
            }

            return open_special_replica(*rsComm, *dataObjInp, info_head);
        }

        // Special collections are not considered for intermediate replicas, so we simply open the replica.
        if (info_head->specColl) {
            return open_special_replica(*rsComm, *dataObjInp, info_head);
        }

        // If the selected replica is not found in the list of replicas, something has gone horribly
        // wrong and we should bail immediately. We need to reference the winning replica whereas in
        // the past the linked list was sorted with the winning replica at the head.
        auto obj = id::make_data_object_proxy(*info_head);

        irods::log(LOG_DEBUG, fmt::format(
            "[{}:{}] - path=[{}], hierarchy=[{}], replica count=[{}]",
            __FUNCTION__, __LINE__, obj.logical_path(), hierarchy, obj.replica_count()));

        auto maybe_replica = id::find_replica(obj, hierarchy);
        if (!maybe_replica) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - requested replica does not exist "
                "[path=[{}], hierarchy=[{}]]",
                __FUNCTION__, __LINE__, dataObjInp->objPath, hierarchy));

            return SYS_REPLICA_DOES_NOT_EXIST;
        }

        auto replica = *maybe_replica;

        irods::log(LOG_DEBUG, fmt::format(
            "[{}:{}] - [path=[{}], id=[{}], hierarchy=[{}], repl_num=[{}], status=[{}], data_status=[{}]]",
            __FUNCTION__, __LINE__,
            replica.logical_path(),
            replica.data_id(),
            replica.hierarchy(),
            replica.replica_number(),
            replica.replica_status(),
            replica.status()));

        const auto open_for_write = getWriteFlag(dataObjInp->openFlags);

        // If the selected replica is part of a locked data object or is in the intermediate state
        // (and no valid replica access token can be found), the open should be denied.
        const auto cond_input = irods::experimental::make_key_value_proxy(dataObjInp->condInput);
        const auto replica_token = cond_input.contains(REPLICA_TOKEN_KW) ? cond_input.at(REPLICA_TOKEN_KW).value().data() : "";
        const auto lock_type = open_for_write ? ill::lock_type::write : ill::lock_type::read;
        if (const auto ret = ill::try_lock(*info_head, lock_type, hierarchy, replica_token); ret < 0) {
            return ret;
        }

        const auto admin_op = irods::experimental::make_key_value_proxy(dataObjInp->condInput).contains(ADMIN_KW);

        auto locked = false;

        if (open_for_write) {
            // Insert the data object information into the replica state table before the replica status is
            // updated because the "before" state is supposed to represent the state of the data object before
            // it is modified (in this particular case, before its replica status is modified).
            if (const auto ec = rst::insert(obj); ec < 0) {
                if (rst::contains(replica.data_id())) {
                    rst::erase(replica.data_id());
                }

                return ec;
            }

            // If the replica is already locked, check_if_data_object_is_locked should have caught it above.
            // If the replica is in an intermediate state, check_if_data_object_is_locked determined that a
            // valid replica_token is present for this replica and so the open should be allowed to continue.
            // If the replica is at rest, this is the first open, so the data object needs to be locked.
            if (replica.at_rest()) {
                // Need to update the cached replica information so that the check below will update the
                // replica access table. This is needed for concurrent opens for write.
                replica.replica_status(INTERMEDIATE_REPLICA);

                if (const int lock_ec = ill::lock_and_publish(*rsComm, {replica, admin_op, 0}, ill::lock_type::write); lock_ec < 0) {
                    if (rst::contains(replica.data_id())) {
                        rst::erase(replica.data_id());
                    }

                    irods::log(LOG_ERROR, fmt::format(
                        "[{}:{}] - Failed to lock data object on create "
                        "[error_code=[{}], path=[{}], hierarchy=[{}]",
                        __FUNCTION__, __LINE__, lock_ec, dataObjInp->objPath, hierarchy));

                    return lock_ec;
                }
            }

            locked = true;
        }

        // The static pre-PEP for open is executed here. On failure, the data object is unlocked.
        if (const auto ec = apply_static_pep_data_obj_open_pre(*rsComm, *dataObjInp, &info_head); ec < 0) {
            const auto erase_rst_entry = irods::at_scope_exit{[&] {
                if (rst::contains(replica.data_id())) {
                    rst::erase(replica.data_id());
                }
            }};

            if (locked) {
                if (const int unlock_ec = ill::unlock_and_publish(*rsComm, {replica, admin_op, 0}, ill::restore_status); unlock_ec < 0) {
                    irods::log(LOG_ERROR, fmt::format(
                        "[{}:{}] - failed to unlock data object [ec=[{}], path=[{}]]",
                        __FUNCTION__, __LINE__, unlock_ec, replica.logical_path()));
                }
            }

            return ec;
        }

        const int l1descInx = open_replica(*rsComm, *dataObjInp, replica);
        if (l1descInx < 0) {
            const auto erase_rst_entry = irods::at_scope_exit{[&] {
                if (rst::contains(replica.data_id())) {
                    rst::erase(replica.data_id());
                }
            }};

            if (locked) {
                if (const int unlock_ec = ill::unlock_and_publish(*rsComm, {replica, admin_op, 0}, ill::restore_status); unlock_ec < 0) {
                    irods::log(LOG_ERROR, fmt::format(
                        "[{}:{}] - failed to unlock data object [ec=[{}], path=[{}]]",
                        __FUNCTION__, __LINE__, unlock_ec, replica.logical_path()));
                }
            }

            return l1descInx;
        }

        {
            auto& l1desc = L1desc[l1descInx];
            irods::log(LOG_DEBUG, fmt::format(
                "[{}:{}] - opened replica "
                "[fd=[{}], path=[{}], hierarchy=[{}], replica_status=[{}], replica_token=[{}]] "
                "[id=[{}], path=[{}], hierarchy=[{}], replica_status=[{}]]",
                __FUNCTION__, __LINE__,
                l1descInx,
                l1desc.dataObjInfo->objPath,
                l1desc.dataObjInfo->rescHier,
                l1desc.dataObjInfo->replStatus,
                l1desc.replica_token,
                replica.data_id(),
                replica.logical_path(),
                replica.hierarchy(),
                replica.replica_status()));
        }

        if (INTERMEDIATE_REPLICA == replica.replica_status()) {
            try {
                // Replica tokens only apply to write operations against intermediate replicas.
                //
                // There is a case where the client wants to open an existing replica for writes
                // but does not have a replica token because the client is the first one to open
                // the replica. "update" should be used when the replica token already exists.
                const auto uo = rat::contains(replica.data_id(), replica.replica_number()) ? update_operation::update
                                                                                           : update_operation::create;
                if (const auto ec = update_replica_access_table(*rsComm, uo, l1descInx, *dataObjInp); ec < 0) {
                    const auto erase_rst_entry = irods::at_scope_exit{[&] {
                        if (rst::contains(replica.data_id())) {
                            rst::erase(replica.data_id());
                        }
                    }};

                    irods::log(LOG_ERROR, fmt::format(
                        "[{}:{}] - [error occurred while updating replica access table] "
                        "[error_code=[{}], path=[{}], hierarchy=[{}]",
                        __FUNCTION__, __LINE__,
                        ec, dataObjInp->objPath, hierarchy));

                    if (const int ec = irods::close_replica_and_unlock_data_object(*rsComm, l1descInx, ill::restore_status); ec < 0) {
                        irods::log(LOG_ERROR, fmt::format(
                            "[{}:{}] - Failed to unlock data object "
                            "[error_code=[{}], path=[{}], hierarchy=[{}]",
                            __FUNCTION__, __LINE__, ec, dataObjInp->objPath, hierarchy));
                    }

                    return ec;
                }
            }
            catch (const irods::exception& e) {
                irods::log(LOG_ERROR, fmt::format(
                    "[{}:{}] - [{}] [error_code=[{}], path=[{}], hierarchy=[{}]",
                    __FUNCTION__, __LINE__, e.client_display_what(),
                    e.code(), dataObjInp->objPath, hierarchy));

                if (const int ec = irods::close_replica_and_unlock_data_object(*rsComm, l1descInx, ill::restore_status); ec < 0) {
                    irods::log(LOG_ERROR, fmt::format(
                        "[{}:{}] - Failed to unlock data object "
                        "[error_code=[{}], path=[{}], hierarchy=[{}]",
                        __FUNCTION__, __LINE__, ec, dataObjInp->objPath, hierarchy));
                }

                return e.code();
            }
            catch (const std::exception& e) {
                irods::log(LOG_ERROR, fmt::format(
                    "[{}:{}] - [{}] [path=[{}], hierarchy=[{}]",
                    __FUNCTION__, __LINE__, e.what(), dataObjInp->objPath, hierarchy));

                if (const int ec = irods::close_replica_and_unlock_data_object(*rsComm, l1descInx, ill::restore_status); ec < 0) {
                    irods::log(LOG_ERROR, fmt::format(
                        "[{}:{}] - Failed to unlock data object "
                        "[error_code=[{}], path=[{}], hierarchy=[{}]",
                        __FUNCTION__, __LINE__, ec, dataObjInp->objPath, hierarchy));
                }

                return SYS_INTERNAL_ERR;
            }
            catch (...) {
                irods::log(LOG_ERROR, fmt::format(
                    "[{}:{}] - unknown error has occurred. [path=[{}], hierarchy=[{}]",
                    __FUNCTION__, __LINE__, dataObjInp->objPath, hierarchy));

                if (const int ec = irods::close_replica_and_unlock_data_object(*rsComm, l1descInx, ill::restore_status); ec < 0) {
                    irods::log(LOG_ERROR, fmt::format(
                        "[{}:{}] - Failed to unlock data object "
                        "[error_code=[{}], path=[{}], hierarchy=[{}]",
                        __FUNCTION__, __LINE__, ec, dataObjInp->objPath, hierarchy));
                }

                return SYS_UNKNOWN_ERROR;
            }

            {
                auto& l1desc = L1desc[l1descInx];
                irods::log(LOG_DEBUG, fmt::format(
                    "[{}:{}] - opened replica "
                    "[fd=[{}], path=[{}], hierarchy=[{}], replica_status=[{}], replica_token=[{}]] "
                    "[id=[{}], path=[{}], hierarchy=[{}], replica_status=[{}]]",
                    __FUNCTION__, __LINE__,
                    l1descInx,
                    l1desc.dataObjInfo->objPath,
                    l1desc.dataObjInfo->rescHier,
                    l1desc.dataObjInfo->replStatus,
                    l1desc.replica_token,
                    replica.data_id(),
                    replica.logical_path(),
                    replica.hierarchy(),
                    replica.replica_status()));
            }
        }

        return l1descInx;
    } // rsDataObjOpen_impl
} // anonymous namespace

int rsDataObjOpen(rsComm_t *rsComm, dataObjInp_t *dataObjInp)
{
    namespace fs = irods::experimental::filesystem;

    if (!dataObjInp) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if (has_trailing_path_separator(dataObjInp->objPath)) {
        return USER_INPUT_PATH_ERR;
    }

    if ((dataObjInp->openFlags & O_ACCMODE) == O_RDONLY && (dataObjInp->openFlags & O_TRUNC)) {
        return USER_INCOMPATIBLE_OPEN_FLAGS;
    }

    const auto data_object_exists = fs::server::exists(*rsComm, dataObjInp->objPath);
    const auto fd = rsDataObjOpen_impl(rsComm, dataObjInp);

    // Do not update the collection mtime if the logical path refers to a remote
    // zone. There is a known limitation in the iRODS protocol which does not allow
    // for further communication over the server port with the remote server in
    // parallel transfer situations once initiated. Further, if the remote zone
    // cares about updating the mtime, the remote API endpoint called should have
    // updated the collection mtime already, so it is unnecessary.
    if (const auto zone_hint = fs::zone_name(fs::path{dataObjInp->objPath});
        zone_hint && !isLocalZone(zone_hint->data())) {
        return fd;
    }

    // Update the parent collection's mtime.
    if (fd >= minimum_valid_file_descriptor && !data_object_exists) {
        const auto parent_path = fs::path{dataObjInp->objPath}.parent_path();

        if (fs::server::is_collection_registered(*rsComm, parent_path)) {
            using std::chrono::system_clock;
            using std::chrono::time_point_cast;

            const auto mtime = time_point_cast<fs::object_time_type::duration>(system_clock::now());

            try {
                irods::experimental::scoped_privileged_client spc{*rsComm};
                fs::server::last_write_time(*rsComm, parent_path, mtime);
            }
            catch (const fs::filesystem_error& e) {
                log_api::error(e.what());
                return e.code().value();
            }
        }
    }

    return fd;
}

