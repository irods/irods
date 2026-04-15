#include "irods/get_logical_quota.h"

#include "irods/apiNumber.h"
#include "irods/procApiRequest.h"

int
rc_get_logical_quota( struct RcComm *_conn, getLogicalQuotaInp_t *_getLogicalQuotaInp,
                logicalQuotaList_t **_logicalQuotaList ) {
    return procApiRequest(_conn, GET_LOGICAL_QUOTA_AN,  _getLogicalQuotaInp, nullptr, reinterpret_cast<void**>(_logicalQuotaList), nullptr);

}
