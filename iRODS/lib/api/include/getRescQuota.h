/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#ifndef GET_RESC_QUOTA_H__
#define GET_RESC_QUOTA_H__

/* This is a Object File I/O call */

#include "rcConnect.h"
#include "objInfo.h"
#include "rodsType.h"

/* definition for RescQuotaPolicy and GlobalQuotaLimit */
#define RESC_QUOTA_UNINIT	-1	/* RescQuotaPolicy not initialized */
#define RESC_QUOTA_OFF		0	/* RescQuotaPolicy disabled */
#define RESC_QUOTA_ON		1	/* RescQuotaPolicy enabled */

/* definition for flags in updatequotaOverrun */
#define GLB_QUOTA       0x1     /* update the global quota */
#define RESC_QUOTA      0x2     /* update the resource quota */
#define ALL_QUOTA       GLB_QUOTA|RESC_QUOTA

typedef struct getRescQuotaInp {
    char rescName[NAME_LEN];		/* resc Name */
    char userName[NAME_LEN];		/* userName or userName#zone */
    char zoneHint[MAX_NAME_LEN];
    int flags;				/* not used */
    int dummy;
    keyValPair_t condInput;   /* include chksum flag and value */
} getRescQuotaInp_t;

typedef struct rescQuota {
    char rescName[NAME_LEN];            /* resc Name */
    char userId[NAME_LEN];
    int flags;
    int dummy;
    rodsLong_t quotaLimit;
    rodsLong_t quotaOverrun;
    struct rescQuota *next;
} rescQuota_t;

/* definition for flag in rescQuota_t */
#define GLOBAL_QUOTA	0x1	/* the quota is global */

#define getRescQuotaInp_PI "str rescName[NAME_LEN]; str userName[NAME_LEN]; str zoneHint[MAX_NAME_LEN]; int flags; int dummy; struct KeyValPair_PI;"

#define rescQuota_PI "str rescName[NAME_LEN]; str userId[NAME_LEN]; int flags; int dummy; double quotaLimit; double quotaOverrun; struct *rescQuota_PI;"

#if defined(RODS_SERVER)
#define RS_GET_RESC_QUOTA rsGetRescQuota
#include "rodsGenQuery.h"
/* prototype for the server handler */
int
rsGetRescQuota( rsComm_t *rsComm, getRescQuotaInp_t *getRescQuotaInp,
                rescQuota_t **rescQuota );
int
_rsGetRescQuota( rsComm_t *rsComm, getRescQuotaInp_t *getRescQuotaInp,
                 rescQuota_t **rescQuota );
int
getQuotaByResc( rsComm_t *rsComm, char *userName, char *rescName,
                genQueryOut_t **genQueryOut );
int
queRescQuota( rescQuota_t **rescQuota, genQueryOut_t *genQueryOut );
int
fillRescQuotaStruct( rescQuota_t *rescQuota, char *tmpQuotaLimit,
                     char *tmpQuotaOver, char *tmpRescName, char *tmpQuotaRescId,
                     char *tmpQuotaUserId );
int
updatequotaOverrun( const char *_resc_hier, rodsLong_t dataSize, int flags );
int
chkRescQuotaPolicy( rsComm_t *rsComm );
#else
#define RS_GET_RESC_QUOTA NULL
#endif

/* prototype for the client call */
int
rcGetRescQuota( rcComm_t *conn, getRescQuotaInp_t *getRescQuotaInp,
                rescQuota_t **rescQuota );
int
freeAllRescQuota( rescQuota_t *rescQuotaHead );

#endif	// GET_RESC_QUOTA_H__
