#ifndef GET_LOGICAL_QUOTA_H__
#define GET_LOGICAL_QUOTA_H__

#include "irods/objInfo.h"
#include "irods/rcConnect.h"
#include "irods/rodsType.h"

typedef struct getLogicalQuotaInp {
    char* collName;
    keyValPair_t condInput;
} getLogicalQuotaInp_t;
#define getLogicalQuotaInp_PI "str *collName; struct KeyValPair_PI;"

typedef struct logicalQuota {
    char* collName;
    rodsLong_t maxBytes;
    rodsLong_t maxObjects;
    rodsLong_t overBytes;
    rodsLong_t overObjects;
} logicalQuota_t;
#define logicalQuota_PI "str *collName; double maxBytes; double maxObjects; double overBytes; double overObjects;"

typedef struct logicalQuotaList {
    int len;
    logicalQuota_t* list;
} logicalQuotaList_t;
#define logicalQuotaList_PI "int len; struct *logicalQuota_PI(len);"

int rcGetLogicalQuota( rcComm_t *conn, getLogicalQuotaInp_t *getLogicalQuotaInp, logicalQuotaList_t **logicalQuotaList );

#endif  // GET_LOGICAL_QUOTA_H__
