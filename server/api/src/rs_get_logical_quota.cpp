#include "irods/icatHighLevelRoutines.hpp"
#include "irods/get_logical_quota.h"
#include "irods/rcMisc.h"
#include "irods/rs_get_logical_quota.hpp"

#include <tuple>
#include <vector>

using log_api = irods::experimental::log::server;

int _rs_get_logical_quota(
    struct RsComm *rsComm,
    getLogicalQuotaInp_t *getLogicalQuotaInp,
    logicalQuotaList_t **logicalQuotaList ) {
    int status = 0;

    std::vector<std::tuple<std::string, std::int64_t, std::int64_t, std::int64_t, std::int64_t>> quota_values;

    status = chl_check_logical_quota(rsComm, getLogicalQuotaInp->coll_name, &quota_values);
    if(status < 0) {
        return status;
    }     
    (*logicalQuotaList) = static_cast<logicalQuotaList_t*>(malloc(sizeof(logicalQuotaList_t)));
    (*logicalQuotaList)->len = static_cast<int>(quota_values.size());
    (*logicalQuotaList)->list = static_cast<logicalQuota_t*>(malloc(sizeof(logicalQuota_t)*(*logicalQuotaList)->len));


    for(int i = 0; i < (*logicalQuotaList)->len; i++) {
       (*logicalQuotaList)->list[i].coll_name = strdup(std::get<0>(quota_values[i]).c_str());
       (*logicalQuotaList)->list[i].max_bytes = std::get<1>(quota_values[i]);
       (*logicalQuotaList)->list[i].max_objects = std::get<2>(quota_values[i]);
       (*logicalQuotaList)->list[i].over_bytes = std::get<3>(quota_values[i]);
       (*logicalQuotaList)->list[i].over_objects = std::get<4>(quota_values[i]);
    }

    return status;
}

int
rs_get_logical_quota( rsComm_t *rsComm, getLogicalQuotaInp_t *getLogicalQuotaInp,
                logicalQuotaList_t **logicalQuotaList ) {
    rodsServerHost_t *rodsServerHost;
    int status = 0;

    status = getAndConnRcatHost(rsComm, SECONDARY_RCAT, (const char*) getLogicalQuotaInp->coll_name, &rodsServerHost);

    if ( status < 0 ) {
        return status;
    }

    if ( rodsServerHost->localFlag == LOCAL_HOST ) {
        status = _rs_get_logical_quota( rsComm, getLogicalQuotaInp, logicalQuotaList );
    }
    else {
        status = rc_get_logical_quota( rodsServerHost->conn, getLogicalQuotaInp,
                                 logicalQuotaList );
    }

    return status;
}

// Returns an indicator for which types of quotas were violated
// 0 - none
// 1 - byte count
// 2 - object count
// 3 - both
int check_logical_quota_violation(struct RsComm *rsComm, const char* _coll_name) {
    getLogicalQuotaInp_t inp;
    logicalQuotaList_t* out;
    int status;
    char* tmp = strdup(_coll_name);
    inp.coll_name = tmp;
    status = rs_get_logical_quota(rsComm, &inp, &out);
    if(status < 0) {
        free(tmp);
        return status;
    }
    status = 0;
    for(int i = 0; i < out->len && status != 3; i++) {
       if(out->list[i].over_bytes > 0) {
            status |= 1;
       }
       if(out->list[i].over_objects > 0) {
            status |= 2;
       } 
    }
    clearLogicalQuotaList(out);
    free(tmp);
    return status;
}
