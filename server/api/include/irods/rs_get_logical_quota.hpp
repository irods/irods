#ifndef IRODS_RS_GET_LOGICAL_QUOTA_HPP
#define IRODS_RS_GET_LOGICAL_QUOTA_HPP

/// \file

#include "irods/get_logical_quota.h"

struct RsComm;

int rs_get_logical_quota( struct RsComm *rsComm, getLogicalQuotaInp_t *getLogicalQuotaInp, logicalQuotaList_t **logicalQuotaList );
int check_logical_quota_violation(struct RsComm *rsComm, const char* _coll_name);

#endif // IRODS_RS_GET_LOGICAL_QUOTA_HPP
