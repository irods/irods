/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* See dataObjTrim.h for a description of this API call.*/

#include "dataObjTrim.h"
#include "dataObjUnlink.h"
#include "dataObjOpr.hpp"
#include "rodsLog.h"
#include "objMetaOpr.hpp"
#include "specColl.hpp"
#include "rsDataObjTrim.hpp"
#include "icatDefines.h"
#include "getRemoteZoneResc.h"
#include "rsDataObjUnlink.hpp"
#include "rsDataObjOpen.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_redirect.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_at_scope_exit.hpp"
#include "irods_linked_list_iterator.hpp"

#include <tuple>

namespace {
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

    std::vector<irods::physical_object> get_replica_list(
        rsComm_t *rsComm,
        dataObjInp_t& dataObjInp) {
        if (!getValByKey(&dataObjInp.condInput, RESC_HIER_STR_KW)) {
            auto result = irods::resolve_resource_hierarchy(irods::UNLINK_OPERATION, rsComm, dataObjInp);
            auto file_obj = std::get<irods::file_object_ptr>(result);
            return file_obj->replicas();
        }
        else {
            irods::file_object_ptr file_obj( new irods::file_object() );
            irods::error fac_err = irods::file_object_factory(rsComm, &dataObjInp, file_obj);
            if (!fac_err.ok()) {
                THROW(fac_err.code(), "file_object_factory failed");
            }
            return file_obj->replicas();
        }
    }

    std::vector<irods::physical_object> get_list_of_replicas_to_trim(
        dataObjInp_t& _inp,
        const std::vector<irods::physical_object>& _list) {
        std::vector<irods::physical_object> trim_list;
        const unsigned long good_replica_count = std::count_if(_list.begin(), _list.end(),
            [](const auto& repl) {
                return (repl.replica_status() & 0x0F) == GOOD_REPLICA;
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
                const auto repl = std::find_if(_list.begin(), _list.end(),
                    [&num](const auto& repl) {
                        return num == repl.repl_num();
                    });
                if (repl == _list.end()) {
                    THROW(SYS_REPLICA_DOES_NOT_EXIST, "target replica does not exist");
                }
                if (expired(*repl)) {
                    THROW(USER_INCOMPATIBLE_PARAMS, "target replica is not old enough for removal");
                }
                if (good_replica_count <= minimum_replica_count && (repl->replica_status() & 0x0F) == GOOD_REPLICA) {
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

        const char* resc_name = getValByKey(&_inp.condInput, RESC_NAME_KW);
        const auto matches_target_resource = [&resc_name](const irods::physical_object& obj) {
            return resc_name && irods::hierarchy_parser{obj.resc_hier()}.first_resc() == resc_name;
        };

        // Walk list and add stale replicas to the list
        for (const auto& obj : _list) {
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
        for (const auto& obj : _list) {
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
        const std::vector<irods::physical_object> repl_list = get_replica_list(rsComm, *dataObjInp);
        if (!repl_num_str.empty()) {
            addKeyVal(&dataObjInp->condInput, REPL_NUM_KW, repl_num_str.c_str());
        }
        const auto trim_list = get_list_of_replicas_to_trim(*dataObjInp, repl_list);
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
        irods::log(LOG_NOTICE, e.what());
        return e.code();
    }
    return retVal;
}

