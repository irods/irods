#ifndef IRODS_GET_LOGICAL_QUOTA_H
#define IRODS_GET_LOGICAL_QUOTA_H

#include "irods/objInfo.h"
#include "irods/rodsType.h"

struct RcComm;

typedef struct GetLogicalQuotaInput {
    char* coll_name;
    keyValPair_t cond_input;
} getLogicalQuotaInp_t;

#define getLogicalQuotaInp_PI "str *collName; struct KeyValPair_PI;"

typedef struct LogicalQuota {
    char* coll_name;
    rodsLong_t max_bytes;
    rodsLong_t max_objects;
    rodsLong_t over_bytes;
    rodsLong_t over_objects;
} logicalQuota_t;

#define logicalQuota_PI "str *collName; double maxBytes; double maxObjects; double overBytes; double overObjects;"

typedef struct LogicalQuotaList {
    int len;
    logicalQuota_t* list;
} logicalQuotaList_t;

#define logicalQuotaList_PI "int len; struct *logicalQuota_PI(len);"

int rc_get_logical_quota( struct RcComm *conn, getLogicalQuotaInp_t *getLogicalQuotaInp, logicalQuotaList_t **logicalQuotaList );

#endif  // IRODS_GET_LOGICAL_QUOTA_H
