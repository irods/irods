#ifndef IRODS_RS_GET_LOGICAL_QUOTA_HPP
#define IRODS_RS_GET_LOGICAL_QUOTA_HPP

/// \file

#include "irods/get_logical_quota.h"

struct RsComm;

namespace irods {
    enum class LogicalQuotaViolation : int {
        NONE = 0,
        BYTES,
        OBJECTS,
        DUAL = 3
    };
}
int rs_get_logical_quota( struct RsComm *_rsComm, getLogicalQuotaInp_t *_getLogicalQuotaInp, logicalQuotaList_t **_logicalQuotaList );
int check_logical_quota_violation(struct RsComm *_rsComm, const char* _coll_name);

#endif // IRODS_RS_GET_LOGICAL_QUOTA_HPP
