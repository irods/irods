#include "irods/rs_get_logical_quota.hpp"

#include "irods/irods_at_scope_exit.hpp"
#include "irods/icatHighLevelRoutines.hpp"
#include "irods/rcMisc.h"
#include "irods/rs_get_grid_configuration_value.hpp"

#include <cstdlib>
#include <string>
#include <tuple>
#include <vector>

using log_api = irods::experimental::log::server;

int _rs_get_logical_quota(
    struct RsComm *_comm,
    getLogicalQuotaInp_t *_get_logical_quota_inp,
    logicalQuotaList_t **_logical_quota_list ) {

    std::vector<std::tuple<std::string, std::int64_t, std::int64_t, std::int64_t, std::int64_t>> quota_values;

    int status = chl_check_logical_quota(_comm, _get_logical_quota_inp->coll_name, &quota_values);
    if(status < 0) {
        log_api::error("{}: chl_check_logical_quota failed with ec=[{}] ", __func__, status);
        return status;
    }
    *_logical_quota_list = static_cast<logicalQuotaList_t*>(std::malloc(sizeof(logicalQuotaList_t)));
    (*_logical_quota_list)->len = static_cast<int>(quota_values.size());
    (*_logical_quota_list)->list = static_cast<logicalQuota_t*>(std::malloc(sizeof(logicalQuota_t)*(*_logical_quota_list)->len));


    for(int i = 0; i < (*_logical_quota_list)->len; i++) {
       (*_logical_quota_list)->list[i].coll_name = strdup(std::get<0>(quota_values[i]).c_str());
       (*_logical_quota_list)->list[i].max_bytes = std::get<1>(quota_values[i]);
       (*_logical_quota_list)->list[i].max_objects = std::get<2>(quota_values[i]);
       (*_logical_quota_list)->list[i].over_bytes = std::get<3>(quota_values[i]);
       (*_logical_quota_list)->list[i].over_objects = std::get<4>(quota_values[i]);
    }

    return status;
} // _rs_get_logical_quota

int
rs_get_logical_quota(struct RsComm *_comm, getLogicalQuotaInp_t *_get_logical_quota_inp,
                logicalQuotaList_t **_logical_quota_list ) {
    rodsServerHost_t *rodsServerHost{};
    int status = 0;

    status = getAndConnRcatHost(_comm, PRIMARY_RCAT, _get_logical_quota_inp->coll_name, &rodsServerHost);

    if ( status < 0 ) {
        return status;
    }

    if ( rodsServerHost->localFlag == LOCAL_HOST ) {
        status = _rs_get_logical_quota( _comm, _get_logical_quota_inp, _logical_quota_list );
    }
    else {
        status = rc_get_logical_quota( rodsServerHost->conn, _get_logical_quota_inp,
                                 _logical_quota_list );
    }

    return status;
} // rs_get_logical_quota

// Returns an indicator for which types of quotas were violated
// 0 - none
// 1 - byte count
// 2 - object count
// 3 - both
int check_logical_quota_violation(struct RsComm *_comm, const char* _coll_name) {
    gridConfigurationInp_t gcinp { {"logical_quotas"}, {"enforcement_enabled"}, { 0 } };
    gridConfigurationOut_t *gcout{};

    const auto free_gcout = irods::at_scope_exit{[&gcout] { std::free(gcout); }};

    int status = rs_get_grid_configuration_value(_comm, &gcinp, &gcout);
    if(status < 0) {
        log_api::warn("{}: Failed to get logical quota enforcement status. (ec=[{}]) Logical quotas will not be enforced.", __func__, status);
        return static_cast<int>(irods::LogicalQuotaViolation::NONE);
    }

    if(std::strncmp(gcout->option_value, "0", 2) == 0) {
        // No logical quota enforcement
        return static_cast<int>(irods::LogicalQuotaViolation::NONE);
    }

    if(std::strncmp(gcout->option_value, "1", 2) != 0) {
        // Strange value set, so log a message
        log_api::warn("{}: Received unknown value [{}] when fetching logical quota enforcement status. (Set to \"1\" to enable enforcement.) Logical quotas will not be enforced.", __func__, gcout->option_value);
        return static_cast<int>(irods::LogicalQuotaViolation::NONE);
    }

    getLogicalQuotaInp_t inp{};
    logicalQuotaList_t* out{};
    auto tmp = std::string(_coll_name);
    inp.coll_name = const_cast<char*>(tmp.c_str());
    status = rs_get_logical_quota(_comm, &inp, &out);
    if(status < 0) {
        log_api::error("{}: rs_get_logical_quota failed with ec=[{}] ", __func__, status);
        return status;
    }

    status = static_cast<int>(irods::LogicalQuotaViolation::NONE);
    for(int i = 0; i < out->len && status != static_cast<int>(irods::LogicalQuotaViolation::DUAL); i++) {
       if(out->list[i].over_bytes > 0) {
            status |= static_cast<int>(irods::LogicalQuotaViolation::BYTES);
       }
       if(out->list[i].over_objects > 0) {
            status |= static_cast<int>(irods::LogicalQuotaViolation::OBJECTS);
       }
    }
    clearLogicalQuotaList(out);
    return status;
} // check_logical_quota_violation
