#ifndef RS_GET_LOGICAL_QUOTA_HPP
#define RS_GET_LOGICAL_QUOTA_HPP

#include "irods/getLogicalQuota.h"
#include "irods/rcConnect.h"
#include "irods/rodsType.h"

int rsGetLogicalQuota( rsComm_t *rsComm, getLogicalQuotaInp_t *getLogicalQuotaInp, logicalQuotaList_t **logicalQuotaList );
int _rsGetLogicalQuota( rsComm_t *rsComm, getLogicalQuotaInp_t *getLogicalQuotaInp, logicalQuotaList_t **logicalQuotaList );
int checkLogicalQuotaViolation(rsComm_t *rsComm, const char* _coll_name);

#endif
