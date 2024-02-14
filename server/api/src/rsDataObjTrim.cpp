#include "irods/dataObjTrim.h"
#include "irods/dataObjUnlink.h"
#include "irods/dataObjOpr.hpp"
#include "irods/rcMisc.h"
#include "irods/rodsLog.h"
#include "irods/objMetaOpr.hpp"
#include "irods/specColl.hpp"
#include "irods/rsDataObjTrim.hpp"
#include "irods/icatDefines.h"
#include "irods/getRemoteZoneResc.h"
#include "irods/rsDataObjUnlink.hpp"
#include "irods/rsDataObjOpen.hpp"

// =-=-=-=-=-=-=-
#include "irods/irods_resource_redirect.hpp"
#include "irods/irods_hierarchy_parser.hpp"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_linked_list_iterator.hpp"
#include "irods/logical_locking.hpp"

#include <tuple>
#include <type_traits> // For std::remove_cv_t
#include <utility>

namespace
{
    namespace ill = irods::logical_locking;

    dataObjInfo_t convert_physical_object_to_dataObjInfo_t(const irods::physical_object obj) {
        dataObjInfo_t info{};

        info.dataSize = obj.size();
        info.dataId = obj.id();
        info.collId = obj.coll_id();
        info.replNum = obj.repl_num();
        info.replStatus = obj.replica_status();
        info.dataMapId = obj.map_id();
        info.rescId = obj.resc_id();

        rstrcpy(info.objPath, obj.name().c_str(), sizeof(info.objPath));
        rstrcpy(info.version, obj.version().c_str(), sizeof(info.version));
        rstrcpy(info.dataType, obj.type_name().c_str(), sizeof(info.dataType));
        rstrcpy(info.rescName, obj.resc_name().c_str(), sizeof(info.rescName));
        rstrcpy(info.filePath, obj.path().c_str(), sizeof(info.filePath));
        rstrcpy(info.dataOwnerName, obj.owner_name().c_str(), sizeof(info.dataOwnerName));
        rstrcpy(info.dataOwnerZone, obj.owner_zone().c_str(), sizeof(info.dataOwnerZone));
        rstrcpy(info.statusString, obj.status().c_str(), sizeof(info.statusString));
        rstrcpy(info.chksum, obj.checksum().c_str(), sizeof(info.chksum));
        rstrcpy(info.dataExpiry, obj.expiry_ts().c_str(), sizeof(info.dataExpiry));
        rstrcpy(info.dataMode, obj.mode().c_str(), sizeof(info.dataMode));
        rstrcpy(info.dataComments, obj.r_comment().c_str(), sizeof(info.dataComments));
        rstrcpy(info.dataCreate, obj.create_ts().c_str(), sizeof(info.dataCreate));
        rstrcpy(info.dataModify, obj.modify_ts().c_str(), sizeof(info.dataModify));
        rstrcpy(info.rescHier, obj.resc_hier().c_str(), sizeof(info.rescHier));

        return info;
    }

    auto get_time_of_expiration(const char* _age_in_minutes) -> std::uint64_t
    {
        if (!_age_in_minutes) {
            return 0;
        }

        const auto now = std::time(nullptr);
        if (-1 == now) {
            THROW(SYS_INTERNAL_ERR, fmt::format("{}: Error occurred getting current time.", __func__));
        }

        try {
            const auto age_in_minutes = std::stoll(_age_in_minutes);
            if (age_in_minutes < 0) {
                THROW(
                    SYS_INVALID_INPUT_PARAM,
                    fmt::format("{}: {} value [{}] must be a non-negative value.", __func__, AGE_KW, _age_in_minutes));
            }

            constexpr decltype(age_in_minutes) seconds_in_one_minute = 60;
            const auto age_in_seconds = age_in_minutes * seconds_in_one_minute;
            if (0 != age_in_minutes && age_in_seconds / age_in_minutes != seconds_in_one_minute) {
                // If the provided time was so large that representing it in seconds caused the value to wrap, just
                // return 0 because the replicas would have to be older than epoch time 0 in order to be old enough to
                // be trimmed. iRODS itself has not existed that long, so none of the replicas are old enough to be
                // trimmed based on the input.
                return 0;
            }

            // If the provided time is larger than the current epoch time, just return 0 because iRODS itself has not
            // existed that long, so none of the replicas are old enough to be trimmed based on the input.
            if (std::cmp_less(now, age_in_seconds)) {
                return 0;
            }

            // The expiration time is the desired minimum age of the replicas to trim subtracted from the current time.
            // Replicas which have a last-modified time less than this expiration time will be trimmed. Based on the
            // preceding checks above, this value is guaranteed to be non-negative.
            return now - age_in_seconds;
        }
        catch (const std::invalid_argument& e) {
            THROW(SYS_INVALID_INPUT_PARAM,
                  fmt::format("{}: {} value [{}] is invalid: [{}]", __func__, AGE_KW, _age_in_minutes, e.what()));
        }
        catch (const std::out_of_range&) {
            THROW(SYS_INVALID_INPUT_PARAM,
                  fmt::format("{}: {} value [{}] is out of range.", __func__, AGE_KW, _age_in_minutes));
        }
    } // get_time_of_expiration

    auto get_minimum_replica_count(const char* copies_kw) -> std::int32_t
    {
        if (!copies_kw) {
            return DEF_MIN_COPY_CNT;
        }
        try {
            const auto minimum_replica_count = std::stoi(copies_kw);
            if (minimum_replica_count <= 0) {
                THROW(SYS_INVALID_INPUT_PARAM,
                      fmt::format("{}: {} value [{}] must be a positive value.", __func__, COPIES_KW, copies_kw));
            }
            return minimum_replica_count;
        }
        catch (const std::invalid_argument& e) {
            THROW(SYS_INVALID_INPUT_PARAM,
                  fmt::format("{}: {} value [{}] is invalid: [{}]", __func__, COPIES_KW, copies_kw, e.what()));
        }
        catch (const std::out_of_range&) {
            THROW(SYS_INVALID_INPUT_PARAM,
                  fmt::format("{}: {} value [{}] is out of range.", __func__, COPIES_KW, copies_kw));
        }
    } // get_minimum_replica_count

    auto get_data_object_info(rsComm_t *rsComm, dataObjInp_t& dataObjInp) -> std::tuple<irods::file_object_ptr, DataObjInfo*>
    {
        DataObjInfo* info{};

        try {
            if (!getValByKey(&dataObjInp.condInput, RESC_HIER_STR_KW)) {
                auto result = irods::resolve_resource_hierarchy(irods::UNLINK_OPERATION, rsComm, dataObjInp, &info);
                auto file_obj = std::get<irods::file_object_ptr>(result);
                return {file_obj, info};
            }

            irods::file_object_ptr file_obj( new irods::file_object() );
            irods::error fac_err = irods::file_object_factory(rsComm, &dataObjInp, file_obj, &info);
            if (!fac_err.ok()) {
                THROW(fac_err.code(), "file_object_factory failed");
            }

            return {file_obj, info};
        }
        catch (...) {
            freeAllDataObjInfo(info);
            throw;
        }
    } // get_data_object_info

    auto get_list_of_replicas_to_trim(const DataObjInp& _inp,
                                      const irods::file_object_ptr& _obj,
                                      const DataObjInfo& _info) -> std::vector<irods::physical_object>
    {
        const auto minimum_replica_count = get_minimum_replica_count(getValByKey(&_inp.condInput, COPIES_KW));
        const auto& replica_list = _obj->replicas();
        auto remaining_replica_count = replica_list.size();
        if (std::cmp_less_equal(remaining_replica_count, minimum_replica_count)) {
            // If the number of existing replicas is already at the minimum required count, we should return an empty
            // list so that no replicas are trimmed.
            return {};
        }

        const auto good_replica_count = std::count_if(replica_list.begin(), replica_list.end(), [](const auto& repl) {
            return GOOD_REPLICA == repl.replica_status();
        });
        constexpr decltype(good_replica_count) minimum_good_replica_count = 1;

        const auto* age_kw = getValByKey(&_inp.condInput, AGE_KW);
        const auto expiration_time = get_time_of_expiration(age_kw);
        const auto expired = [expiration_time](const irods::physical_object& _replica) {
            // Any replica with a last-modified time that is EARLIER than the expiration time (that is, less than) is
            // considered expired.
            return std::cmp_less(std::stoul(_replica.modify_ts()), expiration_time);
        };

        // If a specific replica number is specified, only trim that one!
        if (const char* repl_num = getValByKey(&_inp.condInput, REPL_NUM_KW); repl_num) {
            try {
                const auto num = std::stoi(repl_num);

                const auto repl = std::find_if(replica_list.begin(), replica_list.end(), [num](const auto& repl) {
                    return num == repl.repl_num();
                });

                if (repl == replica_list.end()) {
                    THROW(SYS_REPLICA_DOES_NOT_EXIST, "target replica does not exist");
                }

                if (const auto ret = ill::try_lock(_info, ill::lock_type::write, num); ret < 0) {
                    const auto msg = fmt::format(
                        "trim not allowed because data object is locked "
                        "[path=[{}], hierarchy=[{}]]",
                        _obj->logical_path(), repl->resc_hier());

                    THROW(ret, msg);
                }

                if (age_kw && !expired(*repl)) {
                    THROW(USER_INCOMPATIBLE_PARAMS, "target replica is not old enough for removal");
                }

                if (good_replica_count <= minimum_good_replica_count && GOOD_REPLICA == repl->replica_status()) {
                    THROW(USER_INCOMPATIBLE_PARAMS, "cannot remove the last good replica");
                }

                return {*repl};
            }
            catch (const std::invalid_argument& e) {
                THROW(USER_INVALID_REPLICA_INPUT,
                      fmt::format("{}: {} value [{}] is invalid: [{}]", __func__, REPL_NUM_KW, repl_num, e.what()));
            }
            catch (const std::out_of_range& e) {
                THROW(USER_INVALID_REPLICA_INPUT,
                      fmt::format("{}: {} value [{}] is out of range.", __func__, REPL_NUM_KW, repl_num));
            }
        }

        // Do not try the lock for a replica specified by RESC_NAME_KW because it could
        // be referring to a root resource which would not be a particular replica but
        // potentially a number of different replicas.
        if (const auto ret = ill::try_lock(_info, ill::lock_type::write); ret < 0) {
            const auto msg = fmt::format(
                "trim not allowed because data object is locked [path=[{}]]",
                _obj->logical_path());

            THROW(ret, msg);
        }

        const char* resc_name = getValByKey(&_inp.condInput, RESC_NAME_KW);
        const auto matches_target_resource = [resc_name](const irods::physical_object& _replica) {
            return resc_name && irods::hierarchy_parser{_replica.resc_hier()}.first_resc() == resc_name;
        };

        std::vector<irods::physical_object> trim_list;

        // Walk list and add stale replicas to the list
        for (const auto& obj : replica_list) {
            if (std::cmp_less_equal(replica_list.size() - trim_list.size(), minimum_replica_count)) {
                // Once the size of the list of replicas is reduced by the size of the list of replicas to be trimmed
                // down to the minimum number of replicas to keep, there is nothing left to do.
                return trim_list;
            }
            if (STALE_REPLICA != obj.replica_status()) {
                // Only considering stale replicas at this stage.
                continue;
            }
            if (age_kw && !expired(obj)) {
                // Skip replicas that are not expired if the AGE_KW was provided.
                continue;
            }
            if (resc_name && !matches_target_resource(obj)) {
                // Skip replicas which are not on the specified target resource.
                continue;
            }
            trim_list.push_back(obj);
        }

        // If we have not reached the minimum count, walk list again and add good replicas
        std::remove_cv_t<decltype(good_replica_count)> good_replicas_to_be_trimmed = 0;
        for (const auto& obj : replica_list) {
            if (std::cmp_less_equal(good_replica_count - good_replicas_to_be_trimmed, minimum_good_replica_count)) {
                // Stop adding to the trim list as soon as the number of good replicas that will remain after the
                // number of good replicas to trim have been trimmed is less than the minimum good replica count (1).
                return trim_list;
            }
            if (std::cmp_less_equal(replica_list.size() - trim_list.size(), minimum_replica_count)) {
                // Once the size of the list of replicas is reduced by the size of the list of replicas to be trimmed
                // down to the minimum number of replicas to keep, there is nothing left to do.
                return trim_list;
            }
            if (GOOD_REPLICA != obj.replica_status()) {
                // Only considering good replicas at this stage.
                continue;
            }
            if (age_kw && !expired(obj)) {
                // Skip replicas that are not expired if the AGE_KW was provided.
                continue;
            }
            if (resc_name && !matches_target_resource(obj)) {
                // Skip replicas which are not on the specified target resource.
                continue;
            }
            trim_list.push_back(obj);
            good_replicas_to_be_trimmed++;
        }

        return trim_list;
    } // get_list_of_replicas_to_trim
}

int rsDataObjTrim(
    rsComm_t *rsComm,
    dataObjInp_t *dataObjInp) {

    if (!dataObjInp) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    // Deprecation messages must be handled by doing the following.
    // The native rule engine may erase all messages in the rError array.
    // The only way to guarantee that messages are received by the client
    // is to add them to the rError array when the function returns.
    const auto add_warning_messages_for_deprecated_parameters = irods::at_scope_exit{[&] {
        if (getValByKey(&dataObjInp->condInput, COPIES_KW)) {
            addRErrorMsg(&rsComm->rError,
                         DEPRECATED_PARAMETER,
                         "Specifying a minimum number of replicas to keep is deprecated.");
        }
        if (getValByKey(&dataObjInp->condInput, AGE_KW)) {
            addRErrorMsg(
                &rsComm->rError, DEPRECATED_PARAMETER, "Specifying a minimum age of replicas to trim is deprecated.");
        }
    }};

    // -S and -n are incompatible...
    const char* resc_name = getValByKey(&dataObjInp->condInput, RESC_NAME_KW);
    const char* repl_num = getValByKey(&dataObjInp->condInput, REPL_NUM_KW);
    if (resc_name && repl_num) {
        return USER_INCOMPATIBLE_PARAMS;
    }
    // TODO: If !repl_num && !resc_name, use default resource.

    rodsServerHost_t *rodsServerHost;
    specCollCache_t *specCollCache{};

    resolveLinkedPath( rsComm, dataObjInp->objPath, &specCollCache, &dataObjInp->condInput );
    int remoteFlag = getAndConnRemoteZone( rsComm, dataObjInp, &rodsServerHost, REMOTE_OPEN );

    if ( remoteFlag < 0 ) {
        return remoteFlag;
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        return rcDataObjTrim( rodsServerHost->conn, dataObjInp );
    }

    int retVal = 0;
    try {
        // Temporarily remove REPL_NUM_KW to ensure we are returned all replicas in the list
        std::string repl_num_str{};
        if (repl_num) {
            repl_num_str = repl_num;
            rmKeyVal(&dataObjInp->condInput, REPL_NUM_KW);
        }

        const auto [file_obj, info] = get_data_object_info(rsComm, *dataObjInp);
        irods::at_scope_exit free_data_obj_infos{[p = info] { freeAllDataObjInfo(const_cast<DataObjInfo*>(p)); }};

        if (!info) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - DataObjInfo is null without an error from API",
                __FUNCTION__, __LINE__));

            return SYS_INTERNAL_ERR;
        }

        if (!repl_num_str.empty()) {
            addKeyVal(&dataObjInp->condInput, REPL_NUM_KW, repl_num_str.c_str());
        }

        const auto trim_list = get_list_of_replicas_to_trim(*dataObjInp, file_obj, *info);

        for (const auto& obj : trim_list) {
            if (getValByKey(&dataObjInp->condInput, DRYRUN_KW)) {
                retVal = 1;
                continue;
            }

            dataObjInfo_t tmp = convert_physical_object_to_dataObjInfo_t(obj);
            int status = dataObjUnlinkS(rsComm, dataObjInp, &tmp);
            if ( status < 0 ) {
                if ( retVal == 0 ) {
                    retVal = status;
                }
            }
            else {
                retVal = 1;
            }
        }
    }
    catch (const std::invalid_argument& e) {
        irods::log(LOG_ERROR, e.what());
        return USER_INCOMPATIBLE_PARAMS;
    }
    catch (const irods::exception& e) {
        irods::log(LOG_NOTICE, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, e.client_display_what()));
        return e.code();
    }
    return retVal;
}

