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

    int get_time_of_expiration(const char* age_kw) {
        if (!age_kw) {
            return 0;
        }
        try {
            const auto age_in_minutes = std::atoi(age_kw);
            if ( age_in_minutes > 0 ) {
                const auto now = time(0);
                const auto age_in_seconds = age_in_minutes * 60;
                return now - age_in_seconds;
            }
            return 0;
        }
        catch (const std::exception&) {
            return 0;
        }
    }

    unsigned long get_minimum_replica_count(const char* copies_kw) {
        if (!copies_kw) {
            return DEF_MIN_COPY_CNT;
        }
        try {
            const auto minimum_replica_count = std::stoul(copies_kw);
            if (0 == minimum_replica_count) {
                return DEF_MIN_COPY_CNT;
            }
            return minimum_replica_count;
        }
        catch (const std::invalid_argument&) {
            return DEF_MIN_COPY_CNT;
        }
        catch (const std::out_of_range&) {
            return DEF_MIN_COPY_CNT;
        }
    }

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

    auto get_list_of_replicas_to_trim(
        dataObjInp_t& _inp,
        const irods::file_object_ptr _obj,
        const DataObjInfo& _info)
    {
        const auto& list = _obj->replicas();

        std::vector<irods::physical_object> trim_list;

        const unsigned long good_replica_count = std::count_if(list.begin(), list.end(),
            [](const auto& repl) {
                return GOOD_REPLICA == repl.replica_status();
            });

        const auto minimum_replica_count = get_minimum_replica_count(getValByKey(&_inp.condInput, COPIES_KW));
        const auto expiration = get_time_of_expiration(getValByKey(&_inp.condInput, AGE_KW));
        const auto expired = [&expiration](const irods::physical_object& obj) {
            return expiration && std::atoi(obj.modify_ts().c_str()) > expiration;
        };

        // If a specific replica number is specified, only trim that one!
        const char* repl_num = getValByKey(&_inp.condInput, REPL_NUM_KW);
        if (repl_num) {
            try {
                const auto num = std::stoi(repl_num);

                const auto repl = std::find_if(list.begin(), list.end(),
                    [&num](const auto& repl) {
                        return num == repl.repl_num();
                    });

                if (repl == list.end()) {
                    THROW(SYS_REPLICA_DOES_NOT_EXIST, "target replica does not exist");
                }

                if (const auto ret = ill::try_lock(_info, ill::lock_type::write, num); ret < 0) {
                    const auto msg = fmt::format(
                        "trim not allowed because data object is locked "
                        "[path=[{}], hierarchy=[{}]]",
                        _obj->logical_path(), repl->resc_hier());

                    THROW(ret, msg);
                }

                if (expired(*repl)) {
                    THROW(USER_INCOMPATIBLE_PARAMS, "target replica is not old enough for removal");
                }

                if (good_replica_count <= minimum_replica_count && GOOD_REPLICA == repl->replica_status()) {
                    THROW(USER_INCOMPATIBLE_PARAMS, "cannot remove the last good replica");
                }

                trim_list.push_back(*repl);
                return trim_list;
            }
            catch (const std::invalid_argument& e) {
                irods::log(LOG_ERROR, e.what());
                THROW(USER_INVALID_REPLICA_INPUT, "invalid replica number requested");
            }
            catch (const std::out_of_range& e) {
                irods::log(LOG_ERROR, e.what());
                THROW(USER_INVALID_REPLICA_INPUT, "invalid replica number requested");
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
        const auto matches_target_resource = [&resc_name](const irods::physical_object& obj) {
            return resc_name && irods::hierarchy_parser{obj.resc_hier()}.first_resc() == resc_name;
        };

        // Walk list and add stale replicas to the list
        for (const auto& obj : list) {
            if ((obj.replica_status() & 0x0F) == STALE_REPLICA) {
                if (expired(obj) || (resc_name && !matches_target_resource(obj))) {
                    continue;
                }
                trim_list.push_back(obj);
            }
        }
        if (good_replica_count <= minimum_replica_count) {
            return trim_list;
        }

        // If we have not reached the minimum count, walk list again and add good replicas
        unsigned long good_replicas_to_be_trimmed = 0;
        for (const auto& obj : list) {
            if ((obj.replica_status() & 0x0F) == GOOD_REPLICA) {
                if (expired(obj) || (resc_name && !matches_target_resource(obj))) {
                    continue;
                }
                if (good_replica_count - good_replicas_to_be_trimmed <= minimum_replica_count) {
                    return trim_list;
                }
                trim_list.push_back(obj);
                good_replicas_to_be_trimmed++;
            }
        }
        return trim_list;
    }
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
    irods::at_scope_exit at_scope_exit{[&] {
        // itrim -N
        if (getValByKey(&dataObjInp->condInput, COPIES_KW)) {
            addRErrorMsg(&rsComm->rError,
                         DEPRECATED_PARAMETER,
                         "Specifying a minimum number of replicas to keep is deprecated.");
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

