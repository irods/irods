#include "irods/get_logical_quota.h"

#include "irods/apiNumber.h"
#include "irods/rcMisc.h"
#include "irods/procApiRequest.h"

int rc_get_logical_quota( struct RcComm *_conn, getLogicalQuotaInp_t *_getLogicalQuotaInp, logicalQuotaList_t **_logicalQuotaList ) {
    return procApiRequest(_conn, GET_LOGICAL_QUOTA_AN,  _getLogicalQuotaInp, nullptr, reinterpret_cast<void**>(_logicalQuotaList), nullptr);
}

void clear_get_logical_quota_input(void* _get_logical_quota_input) {
    if(!_get_logical_quota_input) {
        return;
    }

    auto* getLogicalQuotaInp = static_cast<getLogicalQuotaInp_t*>(_get_logical_quota_input);
    clearKeyVal(&getLogicalQuotaInp->cond_input);
    std::free(getLogicalQuotaInp->coll_name);
    std::memset(getLogicalQuotaInp, 0, sizeof(getLogicalQuotaInp_t));
}

void clear_logical_quota_list(void* _logical_quota_list) {
    if(!_logical_quota_list) {
        return;
    }

    auto* logicalQuotaList = static_cast<logicalQuotaList_t*>(_logical_quota_list);
    for(int i = 0; i < logicalQuotaList->len; i++) {
        std::free(logicalQuotaList->list[i].coll_name);
    }
    std::free(logicalQuotaList->list);
    std::memset(logicalQuotaList, 0, sizeof(logicalQuotaList_t));
}
