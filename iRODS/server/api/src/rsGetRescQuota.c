/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* See genQuery.h for a description of this API call.*/

#include "getRescQuota.h"
#include "miscUtil.h"
#include "genQuery.h"
#include "objMetaOpr.h"
#include "resource.h"

int
rsGetRescQuota (rsComm_t *rsComm, getRescQuotaInp_t *getRescQuotaInp,
rescQuota_t **rescQuota)
{
    rodsServerHost_t *rodsServerHost;
    int status = 0;

    status = getAndConnRcatHost (rsComm, SLAVE_RCAT, 
      getRescQuotaInp->zoneHint, &rodsServerHost);

    if (status < 0) return(status);

    if (rodsServerHost->localFlag == LOCAL_HOST) {
	status = _rsGetRescQuota (rsComm, getRescQuotaInp, rescQuota);
    } else {
	status = rcGetRescQuota (rodsServerHost->conn, getRescQuotaInp, 
	  rescQuota);
    }

    return status;
}

int
_rsGetRescQuota (rsComm_t *rsComm, getRescQuotaInp_t *getRescQuotaInp,
rescQuota_t **rescQuota)
{
    int status = 0;
    rescGrpInfo_t *tmpRescGrpInfo;
    rescGrpInfo_t *rescGrpInfo = NULL;
    genQueryOut_t *genQueryOut = NULL;

    if (rescQuota == NULL) return USER__NULL_INPUT_ERR;

    *rescQuota = NULL;

    /* assume it is a resource */

    status = getQuotaByResc (rsComm, getRescQuotaInp->userName,
      getRescQuotaInp->rescName, &genQueryOut);

    if (status >= 0) {
        queRescQuota (rescQuota, genQueryOut, NULL);
        freeGenQueryOut (&genQueryOut);
        return status;
    }

    /* not a resource. may be a resource group */
    status = _getRescInfo (rsComm, getRescQuotaInp->rescName, &rescGrpInfo);

   if (status < 0) {
         rodsLog (LOG_ERROR,
          "_rsGetRescQuota: _getRescInfo of %s error for %s. stat = %d",
          getRescQuotaInp->rescName, getRescQuotaInp->zoneHint, status);
        return status;
    }

    tmpRescGrpInfo = rescGrpInfo;
    while (tmpRescGrpInfo != NULL) {
        status = getQuotaByResc (rsComm, getRescQuotaInp->userName,
          tmpRescGrpInfo->rescInfo->rescName, &genQueryOut);

        if (status >= 0) 
	  queRescQuota (rescQuota, genQueryOut, tmpRescGrpInfo->rescGroupName);
	tmpRescGrpInfo = tmpRescGrpInfo->next;
    }
    freeGenQueryOut (&genQueryOut);
    freeAllRescGrpInfo (rescGrpInfo);

    return 0;
}

/* getQuotaByResc - get the quoto for an individual resource. The code is
 * mostly from doTest7 of iTestGenQuery.c
 */

int
getQuotaByResc (rsComm_t *rsComm, char *userName, char *rescName,
genQueryOut_t **genQueryOut) 
{
    int status;
    genQueryInp_t genQueryInp;
    char condition1[MAX_NAME_LEN];
    char condition2[MAX_NAME_LEN];

    if (genQueryOut == NULL) return USER__NULL_INPUT_ERR;

    *genQueryOut = NULL;
    memset (&genQueryInp, 0, sizeof (genQueryInp));

    genQueryInp.options = QUOTA_QUERY;

    snprintf (condition1, MAX_NAME_LEN, "%s",
             userName);
    addInxVal (&genQueryInp.sqlCondInp, COL_USER_NAME, condition1);

    if (rescName != NULL && strlen(rescName)>0) {
       snprintf (condition2, MAX_NAME_LEN, "%s",
                rescName);
       addInxVal (&genQueryInp.sqlCondInp, COL_R_RESC_NAME, condition2);
    }

    genQueryInp.maxRows = MAX_SQL_ROWS;

    status = rsGenQuery (rsComm, &genQueryInp, genQueryOut);

    clearGenQueryInp(&genQueryInp);

    return (status);
}

int
queRescQuota (rescQuota_t **rescQuotaHead, genQueryOut_t *genQueryOut,
char *rescGroupName)
{
    sqlResult_t *quotaLimit, *quotaOver, *rescName, *quotaRescId, *quotaUserId;
    char *tmpQuotaLimit, *tmpQuotaOver, *tmpRescName, *tmpQuotaRescId,
      *tmpQuotaUserId;
    int i;
    rescQuota_t *tmpRescQuota;

    if ((quotaLimit = getSqlResultByInx (genQueryOut, COL_QUOTA_LIMIT)) == 
      NULL) {
        rodsLog (LOG_ERROR,
          "queRescQuota: getSqlResultByInx for COL_QUOTA_LIMIT failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((quotaOver = getSqlResultByInx (genQueryOut, COL_QUOTA_OVER))==NULL) {
        rodsLog (LOG_ERROR,
          "queRescQuota: getSqlResultByInx for COL_QUOTA_OVER failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((rescName = getSqlResultByInx (genQueryOut, COL_R_RESC_NAME))==NULL) {
        rodsLog (LOG_ERROR,
          "queRescQuota: getSqlResultByInx for COL_R_RESC_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((quotaRescId = getSqlResultByInx (genQueryOut, COL_QUOTA_RESC_ID)) ==
      NULL) {
        rodsLog (LOG_ERROR,
          "queRescQuota: getSqlResultByInx for COL_QUOTA_RESC_ID failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((quotaUserId = getSqlResultByInx (genQueryOut, COL_QUOTA_USER_ID)) ==
      NULL) {
        rodsLog (LOG_ERROR,
          "queRescQuota: getSqlResultByInx for COL_QUOTA_USER_ID failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    for (i = 0;i < genQueryOut->rowCnt; i++) {
	tmpQuotaLimit =  &quotaLimit->value[quotaLimit->len * i];
	tmpQuotaOver =  &quotaOver->value[quotaOver->len * i];
	tmpRescName =  &rescName->value[rescName->len * i];
	tmpQuotaRescId =  &quotaRescId->value[quotaRescId->len * i];
	tmpQuotaUserId =  &quotaUserId->value[quotaUserId->len * i];
	tmpRescQuota = (rescQuota_t*)malloc (sizeof (rescQuota_t));
	fillRescQuotaStruct (tmpRescQuota, tmpQuotaLimit, tmpQuotaOver,
	  tmpRescName, tmpQuotaRescId, tmpQuotaUserId, rescGroupName);
	tmpRescQuota->next = *rescQuotaHead;
	*rescQuotaHead = tmpRescQuota;
    }

    return 0;
}

int
fillRescQuotaStruct (rescQuota_t *rescQuota, char *tmpQuotaLimit, 
char *tmpQuotaOver, char *tmpRescName, char *tmpQuotaRescId,
char *tmpQuotaUserId, char *rescGroupName)
{
    bzero (rescQuota, sizeof (rescQuota_t));

    rescQuota->quotaLimit = strtoll (tmpQuotaLimit, 0, 0);
    rescQuota->quotaOverrun = strtoll (tmpQuotaOver, 0, 0);
    if (strtoll (tmpQuotaRescId, 0, 0) > 0) {
	/* quota by resource */
	rstrcpy (rescQuota->rescName, tmpRescName, NAME_LEN);
    } else {
	rescQuota->flags = GLOBAL_QUOTA;
    } 
    rstrcpy (rescQuota->userId, tmpQuotaUserId, NAME_LEN);
    if (rescGroupName != NULL)
        rstrcpy (rescQuota->rescGroupName, rescGroupName, NAME_LEN);

    return 0;
}

int
setRescQuota (rsComm_t *rsComm, char *zoneHint,
rescGrpInfo_t **rescGrpInfoHead, rodsLong_t dataSize)
{
    rescGrpInfo_t *tmpRescGrpInfo;
    getRescQuotaInp_t getRescQuotaInp;
    rescQuota_t *rescQuota = NULL;
    rescQuota_t *tmpRescQuota;
    int needInit = 0;
    int status = 0;

    status = chkRescQuotaPolicy (rsComm);
    if (status != RESC_QUOTA_ON) return 0;

    if (rescGrpInfoHead == NULL) return USER__NULL_INPUT_ERR;
    tmpRescGrpInfo = *rescGrpInfoHead;
    while (tmpRescGrpInfo != NULL) {
        if (tmpRescGrpInfo->rescInfo->quotaLimit == RESC_QUOTA_UNINIT) {
            needInit = 1;
            break;
        }
        tmpRescGrpInfo = tmpRescGrpInfo->next;
    }
    if (needInit == 0) {
        status = chkRescGrpInfoForQuota (rescGrpInfoHead, dataSize);
	return status;
    }
    bzero (&getRescQuotaInp, sizeof (getRescQuotaInp));
    tmpRescGrpInfo = *rescGrpInfoHead;
    rstrcpy (getRescQuotaInp.zoneHint, zoneHint, MAX_NAME_LEN);
    if (strlen (tmpRescGrpInfo->rescGroupName) > 0) {
        rstrcpy (getRescQuotaInp.rescName, tmpRescGrpInfo->rescGroupName, 
	  NAME_LEN);
    } else {
        rstrcpy (getRescQuotaInp.rescName, tmpRescGrpInfo->rescInfo->rescName, 
	  NAME_LEN);
    }
    snprintf (getRescQuotaInp.userName, NAME_LEN, "%s#%s",
      rsComm->clientUser.userName, rsComm->clientUser.rodsZone);
    status = rsGetRescQuota (rsComm, &getRescQuotaInp, &rescQuota);
    if (status < 0) return status;

    /* process the quota */
    tmpRescQuota = rescQuota;
    while (tmpRescQuota != NULL) {
	if ((tmpRescQuota->flags & GLOBAL_QUOTA) > 0) {
	    /* global limit */
	    /* get the worst. the higher the overrun, the worse */
	    if (GlobalQuotaLimit < 0 || 
	      GlobalQuotaOverrun < tmpRescQuota->quotaOverrun) {
	        GlobalQuotaLimit = tmpRescQuota->quotaLimit;
		GlobalQuotaOverrun = tmpRescQuota->quotaOverrun; 
	    }
	} else {
            tmpRescGrpInfo = *rescGrpInfoHead;
	    /* update the quota */
	    while (tmpRescGrpInfo != NULL) {
		rescInfo_t *rescInfo = tmpRescGrpInfo->rescInfo;
		if (strcmp (tmpRescQuota->rescName, rescInfo->rescName) == 0) {
		    if (rescInfo->quotaLimit == RESC_QUOTA_UNINIT || 
		      rescInfo->quotaOverrun < tmpRescQuota->quotaOverrun) {
			rescInfo->quotaLimit = tmpRescQuota->quotaLimit;
			rescInfo->quotaOverrun = tmpRescQuota->quotaOverrun;
		    }
		    break;
		}
		tmpRescGrpInfo = tmpRescGrpInfo->next;
	    }
	}
	tmpRescQuota = tmpRescQuota->next;
    }
    freeAllRescQuota (rescQuota);
    status = chkRescGrpInfoForQuota (rescGrpInfoHead, dataSize);

    return status;
}

int 
chkRescGrpInfoForQuota (rescGrpInfo_t **rescGrpInfoHead, rodsLong_t dataSize)
{
    rescGrpInfo_t *tmpRescGrpInfo, *prevRescGrpInfo;

    if (dataSize < 0) dataSize = 0;
    if (GlobalQuotaLimit == RESC_QUOTA_UNINIT) {
	/* indicate we have already query quota */
	GlobalQuotaLimit = 0;
    } else if (GlobalQuotaLimit > 0) {
	if (GlobalQuotaOverrun + dataSize >= 0) return SYS_RESC_QUOTA_EXCEEDED;
    }

    /* take out resc that has been overrun and indicate that the quota has
     * been initialized */
    prevRescGrpInfo = NULL;
    tmpRescGrpInfo = *rescGrpInfoHead;
    while (tmpRescGrpInfo != NULL) {
        rescGrpInfo_t *nextRescGrpInfo = tmpRescGrpInfo->next;
        if (tmpRescGrpInfo->rescInfo->quotaLimit > 0 &&
          tmpRescGrpInfo->rescInfo->quotaOverrun + dataSize >= 0) {
            /* overrun the quota. take the resc out */
            if (prevRescGrpInfo == NULL) {
                /* head */
                *rescGrpInfoHead = nextRescGrpInfo;
            } else {
                prevRescGrpInfo->next = nextRescGrpInfo;
            }
            if (*rescGrpInfoHead != NULL) {
                (*rescGrpInfoHead)->status = SYS_RESC_QUOTA_EXCEEDED;
            }
            free (tmpRescGrpInfo);
        } else {
            if (tmpRescGrpInfo->rescInfo->quotaLimit == RESC_QUOTA_UNINIT) {
	        /* indicate we have already query quota */
                tmpRescGrpInfo->rescInfo->quotaLimit = 0;
            }
            prevRescGrpInfo = tmpRescGrpInfo;
        }
        tmpRescGrpInfo = nextRescGrpInfo;
    }
    if (rescGrpInfoHead == NULL || *rescGrpInfoHead == NULL) 
	return SYS_RESC_QUOTA_EXCEEDED;
    else 
	return 0;
}

int
updatequotaOverrun (rescInfo_t *rescInfo, rodsLong_t dataSize, int flags)
{
    if ((flags & GLB_QUOTA) > 0 && GlobalQuotaLimit > 0) {
        GlobalQuotaOverrun += dataSize;
    }

    if (rescInfo == NULL) return USER__NULL_INPUT_ERR;
    if ((flags & RESC_QUOTA) > 0 && rescInfo->quotaLimit > 0) {
        rescInfo->quotaOverrun += dataSize;
    }
    return 0;
}

int
chkRescQuotaPolicy (rsComm_t *rsComm)
{
    int status;
    ruleExecInfo_t rei;

    if (RescQuotaPolicy == RESC_QUOTA_UNINIT) {
        initReiWithDataObjInp (&rei, rsComm, NULL);
        status = applyRule ("acRescQuotaPolicy", NULL, &rei, NO_SAVE_REI);
	if (status < 0) {
            rodsLog (LOG_ERROR,
              "queRescQuota: acRescQuotaPolicy error status = %d", status);
	    RescQuotaPolicy = RESC_QUOTA_OFF;
	}
    }
    return RescQuotaPolicy;
}

