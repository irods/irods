/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See dataObjRepl.h for a description of this API call.*/

#include "dataObjRepl.h"
#include "dataObjOpr.h"
#include "dataObjCreate.h"
#include "dataObjOpen.h"
#include "dataObjPut.h"
#include "dataObjGet.h"
#include "rodsLog.h"
#include "objMetaOpr.h"
#include "physPath.h"
#include "specColl.h"
#include "resource.h"
#include "reGlobalsExtern.h"
#include "reDefines.h"
#include "reSysDataObjOpr.h"
#include "getRemoteZoneResc.h"
#include "l3FileGetSingleBuf.h"
#include "l3FilePutSingleBuf.h"
#include "fileSyncToArch.h"
#include "fileStageToCache.h"
#include "unbunAndRegPhyBunfile.h"
#include "dataObjTrim.h" // JMC - backport 4550
#include "dataObjLock.h" // JMC - backport 4609

#include "eirods_log.h"

int
rsDataObjRepl250 (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                  transStat_t **transStat)
{
    int status;
    transferStat_t *transferStat = NULL;

    status = rsDataObjRepl (rsComm, dataObjInp, &transferStat);

    if (transStat != NULL && status >= 0 && transferStat != NULL) {
        *transStat = (transStat_t *) malloc (sizeof (transStat_t));
        (*transStat)->numThreads = transferStat->numThreads;
        (*transStat)->bytesWritten = transferStat->bytesWritten;
        free (transferStat);
    }
    return status;
}

/* rsDataObjRepl - The Api handler of the rcDataObjRepl call - Replicate
 * a data object.
 * Input -
 *    rsComm_t *rsComm 
 *    dataObjInp_t *dataObjInp - The replication input
 *    transferStat_t **transStat - transfer stat output
 */

int
rsDataObjRepl (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
               transferStat_t **transStat)
{
    int status;
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;
    dataObjInfo_t *dataObjInfo = NULL;
    char* lockType = NULL; // JMC - backport 4609
    int   lockFd   = -1;   // JMC - backport 4609

    if (getValByKey (&dataObjInp->condInput, SU_CLIENT_USER_KW) != NULL) {
        /* To SU, cannot be called by normal user directly */ 
        if (rsComm->proxyUser.authInfo.authFlag < REMOTE_PRIV_USER_AUTH) {
            return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
        }
    }

    status = resolvePathInSpecColl (rsComm, dataObjInp->objPath,
                                    READ_COLL_PERM, 0, &dataObjInfo);

    if (status == DATA_OBJ_T) {
        if (dataObjInfo != NULL && dataObjInfo->specColl != NULL) {
            if (dataObjInfo->specColl->collClass == LINKED_COLL) {
                rstrcpy (dataObjInp->objPath, dataObjInfo->objPath,
                         MAX_NAME_LEN);
                freeAllDataObjInfo (dataObjInfo);
            } else {
                freeAllDataObjInfo (dataObjInfo);
                return (SYS_REG_OBJ_IN_SPEC_COLL);
            }
        }       
    }

    remoteFlag = getAndConnRemoteZone (rsComm, dataObjInp, &rodsServerHost,
                                       REMOTE_OPEN);

    if (remoteFlag < 0) {
        return (remoteFlag);
    } else if (remoteFlag == REMOTE_HOST) {
        status = _rcDataObjRepl (rodsServerHost->conn, dataObjInp,
                                 transStat);
        return status;
    }

    *transStat = (transferStat_t*)malloc (sizeof (transferStat_t));
    memset (*transStat, 0, sizeof (transferStat_t));
    // =-=-=-=-=-=-=-
    // JMC - backport 4609
    lockType = getValByKey (&dataObjInp->condInput, LOCK_TYPE_KW);
    if (lockType != NULL) {
        lockFd = rsDataObjLock (rsComm, dataObjInp);
        if (lockFd >= 0) {
            /* rm it so it won't be done again causing deadlock */
            rmKeyVal (&dataObjInp->condInput, LOCK_TYPE_KW);
        } else {
            rodsLogError (LOG_ERROR, lockFd,
                          "rsDataObjRepl: rsDataObjLock error for %s. lockType = %s",
                          dataObjInp->objPath, lockType);
            return lockFd;
        }
    }
    // =-=-=-=-=-=-=-
    status = _rsDataObjRepl (rsComm, dataObjInp, *transStat, NULL); 
     
    if (lockFd > 0) rsDataObjUnlock (rsComm, dataObjInp, lockFd); // JMC - backport 4609

    return (status);
}
    
int
_rsDataObjRepl (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                transferStat_t *transStat, dataObjInfo_t *outDataObjInfo)
{
    int status;
    dataObjInfo_t *dataObjInfoHead = NULL;
    dataObjInfo_t *oldDataObjInfoHead = NULL;
    dataObjInfo_t *destDataObjInfo = NULL;
    rescGrpInfo_t *myRescGrpInfo = NULL;
    ruleExecInfo_t rei;
    int multiCopyFlag;
    char *accessPerm;
    int backupFlag;
    int allFlag;
    int savedStatus = 0;

    if (getValByKey (&dataObjInp->condInput, SU_CLIENT_USER_KW) != NULL) {
        accessPerm = NULL;
    } else if (getValByKey (&dataObjInp->condInput, IRODS_ADMIN_KW) != NULL) {
        if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
            return (CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
        }
        accessPerm = NULL;
    } else {
        accessPerm = ACCESS_READ_OBJECT;
    }

    initReiWithDataObjInp (&rei, rsComm, dataObjInp);
    status = applyRule ("acSetMultiReplPerResc", NULL, &rei, NO_SAVE_REI);
    if (strcmp (rei.statusStr, MULTI_COPIES_PER_RESC) == 0) {
        multiCopyFlag = 1;
    } else {
        multiCopyFlag = 0;
    }

    /* query rcat for dataObjInfo and sort it */

    if (multiCopyFlag) {
        status = getDataObjInfo (rsComm, dataObjInp, &dataObjInfoHead,
                                 accessPerm, 0);
    } else {
        /* No multiCopy allowed. ignoreCondInput - need to find all copies
         * to make sure no multiCopy in the same resource */
        status = getDataObjInfo (rsComm, dataObjInp, &dataObjInfoHead,
                                 accessPerm, 1);
    }

    if (status < 0) {
        rodsLog (LOG_NOTICE,
                 "rsDataObjRepl: getDataObjInfo for %s", dataObjInp->objPath);
        return (status);
    }
    
    if (getValByKey (&dataObjInp->condInput, UPDATE_REPL_KW) != NULL) {
        status = sortObjInfoForRepl (&dataObjInfoHead, &oldDataObjInfoHead, 0);
        if (status < 0) return status;

        /* update old repl to new repl */
        status = _rsDataObjReplUpdate (rsComm, dataObjInp, dataObjInfoHead,oldDataObjInfoHead, transStat, NULL);
          
        if (status >= 0 && outDataObjInfo != NULL) {
            *outDataObjInfo = *oldDataObjInfoHead;
            outDataObjInfo->next = NULL;
        }
        freeAllDataObjInfo (dataObjInfoHead);
        freeAllDataObjInfo (oldDataObjInfoHead);
        
        return status;
    }
    /* if multiCopy allowed, remove old so they won't be overwritten */
    status = sortObjInfoForRepl (&dataObjInfoHead, &oldDataObjInfoHead,multiCopyFlag);
      
    if (status < 0) 
        return status;

    if (getValByKey (&dataObjInp->condInput, BACKUP_RESC_NAME_KW) != NULL) {
        /* backup to the DEST_RESC if one does not exist */
        backupFlag = 1;
        multiCopyFlag = 0; // JMC - backport 4594
    } else {
        backupFlag = 0;
    }
    if (getValByKey (&dataObjInp->condInput, ALL_KW) != NULL) {
        allFlag = 1;
    } else {    
        allFlag = 0;
    }

    if( backupFlag == 0 && allFlag == 1                                 &&
        getValByKey (&dataObjInp->condInput, DEST_RESC_NAME_KW) == NULL &&
        dataObjInfoHead != NULL && dataObjInfoHead->rescGroupName[0] != '\0') {
        /* replicate to all resc in the rescGroup if DEST_RESC is not specified */
        addKeyVal (&dataObjInp->condInput, DEST_RESC_NAME_KW, 
                   dataObjInfoHead->rescGroupName);
    }

    /* query rcat for resource info and sort it */
    dataObjInp->oprType = REPLICATE_OPR; // JMC - backport 4660
    status = getRescGrpForCreate (rsComm, dataObjInp, &myRescGrpInfo);
    if (status < 0) return status;


    if (multiCopyFlag == 0 ) { // JMC - backport 4594
        /* if one copy per resource, see if a good copy already exist, 
         * If it does, the copy is returned in destDataObjInfo. 
         * Otherwise, Resources in &myRescGrpInfo are trimmed. Only those
         ( target resources remained are left in &myRescGrpInfo.
         * Also, the copies need to be overwritten is returned
         * in destDataObjInfo. */
        status = resolveSingleReplCopy (&dataObjInfoHead, &oldDataObjInfoHead,
                                        &myRescGrpInfo, &destDataObjInfo, &dataObjInp->condInput);
        if (status == HAVE_GOOD_COPY) {
            // =-=-=-=-=-=-=-
            // JMC - backport 4450
            dataObjInfo_t *cacheDataObjInfo = NULL;
            dataObjInfo_t *compDataObjInfo  = NULL; // JMC - backport 4594
            if( getValByKey (&dataObjInp->condInput, PURGE_CACHE_KW) != NULL &&

                // =-=-=-=-=-=-=-
                // JMC - backport 4494
                getDataObjByClass( dataObjInfoHead, COMPOUND_CL, &compDataObjInfo ) >= 0 &&
                strlen (compDataObjInfo->rescGroupName) > 0                              &&
                getCacheDataInfoForRepl (rsComm, dataObjInfoHead, NULL, compDataObjInfo, &cacheDataObjInfo) >= 0 ) {
                // =-=-=-=-=-=-=-
                /* purge the cache */
                int status1 = trimDataObjInfo (rsComm, cacheDataObjInfo);
                if (status1 < 0) {
                    rodsLog (LOG_NOTICE,
                             "_rsDataObjRepl: trimDataObjInfo for %s", 
                             dataObjInp->objPath);
                }
            }
            // =-=-=-=-=-=-=-
            if (outDataObjInfo != NULL && destDataObjInfo != NULL) {
                /* pass back the GOOD_COPY */
                *outDataObjInfo = *destDataObjInfo;
                outDataObjInfo->next = NULL;
            }
            // =-=-=-=-=-=-=-
            // JMC - backport 4494
            if( backupFlag == 0 && myRescGrpInfo != NULL        && 
                ( allFlag == 1 || myRescGrpInfo->next == NULL ) && 
                ( myRescGrpInfo->status < 0 ) ) {
                status = myRescGrpInfo->status;
                // =-=-=-=-=-=-=-
            } else {
                status = 0;
            }
            freeAllDataObjInfo (dataObjInfoHead);
            freeAllDataObjInfo (oldDataObjInfoHead);
            freeAllDataObjInfo (destDataObjInfo); // JMC - backport 4494
            freeAllRescGrpInfo (myRescGrpInfo);
            return status;
        } else if (status < 0) {
            freeAllDataObjInfo (dataObjInfoHead);
            freeAllDataObjInfo (oldDataObjInfoHead);
            freeAllDataObjInfo (destDataObjInfo); // JMC - backport 4494
            freeAllRescGrpInfo (myRescGrpInfo);
            return status;
        }
        /* NO_GOOD_COPY drop through here */
    }

    status = applyPreprocRuleForOpen (rsComm, dataObjInp, &dataObjInfoHead);
    if (status < 0) return status;

    /* If destDataObjInfo is not NULL, we will overwrite it. Otherwise
     * replicate to myRescGrpInfo */ 

    if (destDataObjInfo != NULL) {
        status = _rsDataObjReplUpdate( rsComm, dataObjInp, dataObjInfoHead,
                                       destDataObjInfo, transStat, oldDataObjInfoHead);
        if (status >= 0) {
            if (outDataObjInfo != NULL) {
                *outDataObjInfo = *destDataObjInfo;
                outDataObjInfo->next = NULL;
            }
            if (allFlag == 0) {
                freeAllDataObjInfo (dataObjInfoHead);
                freeAllDataObjInfo (oldDataObjInfoHead);
                freeAllDataObjInfo (destDataObjInfo); // JMC - backport 4494
                freeAllRescGrpInfo (myRescGrpInfo);
                return 0;
            } else { // JMC - backport 4494
                /* queue destDataObjInfo in &dataObjInfoHead so that stage to cache 
                 * can evaluate it */
                queDataObjInfo (&dataObjInfoHead, destDataObjInfo, 0, 1);
                destDataObjInfo = NULL;
            } 
        } else {
            savedStatus = status;
        }
    }

    if (myRescGrpInfo != NULL) {
        /* new kreplication to the resource group */
        status = _rsDataObjReplNewCopy( rsComm, dataObjInp, dataObjInfoHead,
                                        myRescGrpInfo, transStat, oldDataObjInfoHead, 
                                        outDataObjInfo);
        if (status < 0) savedStatus = status;
    }

    freeAllDataObjInfo (dataObjInfoHead);
    freeAllDataObjInfo (oldDataObjInfoHead);
    freeAllRescGrpInfo (myRescGrpInfo);

    return (savedStatus);
} 

/* _rsDataObjRepl - Update existing copies from srcDataObjInfoHead to
 *      destDataObjInfoHead.
 * Additinal input -
 *   dataObjInfo_t *srcDataObjInfoHead _ a link list of the src to be
 *     replicated. Only one will be picked.
 *   dataObjInfo_t *destDataObjInfoHead -  a link of copies to be updated.
 *     The dataSize in this struct will also be updated.
 *   dataObjInfo_t *oldDataObjInfo - this is for destDataObjInfo is a
 *       COMPOUND_CL resource. If it is, need to find an old copy of
 *       the resource in the same group so that it can be updated first.
 */

int
_rsDataObjReplUpdate (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                      dataObjInfo_t *srcDataObjInfoHead, dataObjInfo_t *destDataObjInfoHead,
                      transferStat_t *transStat, dataObjInfo_t *oldDataObjInfo)
{
    dataObjInfo_t *destDataObjInfo;
    dataObjInfo_t *srcDataObjInfo;
    int status;
    int allFlag;
    int savedStatus = 0;
    int replCnt = 0;

    if (getValByKey (&dataObjInp->condInput, ALL_KW) != NULL) {
        allFlag = 1;
    } else {
        allFlag = 0;
    }

    transStat->bytesWritten = srcDataObjInfoHead->dataSize;
    destDataObjInfo = destDataObjInfoHead;
    while (destDataObjInfo != NULL) {
        if (destDataObjInfo->dataId == 0) {
            destDataObjInfo = destDataObjInfo->next;
            continue;
        }

        /* destDataObj exists */
        if (getRescClass (destDataObjInfo->rescInfo) == COMPOUND_CL) {
            /* need to get a copy in cache first */
            if ((status = getCacheDataInfoOfCompObj (rsComm, dataObjInp,
                                                     srcDataObjInfoHead, destDataObjInfoHead, destDataObjInfo,
                                                     oldDataObjInfo, &srcDataObjInfo)) < 0) {
                return status;
            }
            status = _rsDataObjReplS (rsComm, dataObjInp,
                                      srcDataObjInfo, NULL, "", destDataObjInfo, 1);
        } else {
            srcDataObjInfo = srcDataObjInfoHead;
            while (srcDataObjInfo != NULL) {
                /* overwrite a specific destDataObjInfo */
                status = _rsDataObjReplS (rsComm, dataObjInp, srcDataObjInfo,
                                          NULL, "", destDataObjInfo, 1);
                if (status >= 0) {
                    break;
                }
                srcDataObjInfo = srcDataObjInfo->next;
            }
        }
        if (status >= 0) {
            transStat->numThreads = dataObjInp->numThreads;
            if (allFlag == 0) {
                return 0;
            }
        } else {
            savedStatus = status;
            replCnt ++;
        }
        destDataObjInfo = destDataObjInfo->next;
    }

    return savedStatus;
}

/* _rsDataObjReplNewCopy - Replicate new copies to destRescGrpInfo.
 * Additinal input -
 *   dataObjInfo_t *srcDataObjInfoHead _ a link list of the src to be
 *     replicated. Only one will be picked.
 *   rescGrpInfo_t *destRescGrpInfo - The dest resource info
 *   dataObjInfo_t *oldDataObjInfo - this is for destDataObjInfo is a
 *       COMPOUND_CL resource. If it is, need to find an old copy of
 *       the resource in the same group so that it can be updated first.
 *   dataObjInfo_t *outDataObjInfo - If it is not NULL, output the last
 *       dataObjInfo_t of the new copy.
 */

int
_rsDataObjReplNewCopy (rsComm_t *rsComm,
                       dataObjInp_t *dataObjInp,
                       dataObjInfo_t *srcDataObjInfoHead,
                       rescGrpInfo_t *destRescGrpInfo,
                       transferStat_t *transStat,
                       dataObjInfo_t *oldDataObjInfo,
                       dataObjInfo_t *outDataObjInfo)
{
    dataObjInfo_t *srcDataObjInfo;
    rescGrpInfo_t *tmpRescGrpInfo;
    rescInfo_t *tmpRescInfo;
    int status;
    int allFlag;
    int savedStatus = 0;
    rescInfo_t *compRescInfo = NULL; // JMC - backport 4593
    rescInfo_t *cacheRescInfo = NULL; // JMC - backport 4593

    if (getValByKey (&dataObjInp->condInput, ALL_KW) != NULL) {
        allFlag = 1;
    } else {
        allFlag = 0;
    }
    // =-=-=-=-=-=-=-
    // JMC - backport 4593
    /* If doing ALL, need to skip cacheRescInfo if the rescGrp has a COMPOUND_CL resc 
     * because getCacheDataInfoOfCompResc will stage it */
    if( allFlag == 1 && destRescGrpInfo != NULL    &&
        strlen(destRescGrpInfo->rescGroupName) > 0 &&
        getRescGrpClass (destRescGrpInfo, &compRescInfo) == COMPOUND_CL) {
        getCacheRescInGrp (rsComm, destRescGrpInfo->rescGroupName,compRescInfo, &cacheRescInfo);
    }
    // =-=-=-=-=-=-=-
    transStat->bytesWritten = srcDataObjInfoHead->dataSize;
    tmpRescGrpInfo = destRescGrpInfo;
    while (tmpRescGrpInfo != NULL) {
        tmpRescInfo = tmpRescGrpInfo->rescInfo;
        // =-=-=-=-=-=-=-
        // JMC - backport 4593
        if (tmpRescInfo == cacheRescInfo) {
            /* skip cacheResc of COMPOUND_CL because getCacheDataInfoOfCompResc will
             * stage to this cache */
            tmpRescGrpInfo = tmpRescGrpInfo->next;
            continue;
        }
        // =-=-=-=-=-=-=-
        if (getRescClass (tmpRescInfo) == COMPOUND_CL) {
            /* need to get a copy in cache first */
            if ((status = getCacheDataInfoOfCompResc (rsComm, dataObjInp,
                                                      srcDataObjInfoHead, NULL, tmpRescGrpInfo,
                                                      oldDataObjInfo, &srcDataObjInfo)) < 0) {
                return status;
            }

            status = _rsDataObjReplS (rsComm, dataObjInp, srcDataObjInfo,
                                      tmpRescInfo, tmpRescGrpInfo->rescGroupName, outDataObjInfo, 0);
        } else {
            srcDataObjInfo = srcDataObjInfoHead;
            while (srcDataObjInfo != NULL) {
                status = _rsDataObjReplS( rsComm, dataObjInp, srcDataObjInfo,
                                          tmpRescInfo, tmpRescGrpInfo->rescGroupName,
                                          outDataObjInfo, 0);
                if (status >= 0) {
                    break;
                } else {
                    savedStatus = status;
                }
                srcDataObjInfo = srcDataObjInfo->next;
            }
        }
        if (status >= 0) {
            transStat->numThreads = dataObjInp->numThreads;
            if (allFlag == 0) {
                return 0;
            }
        } else {
            savedStatus = status;
        }
        tmpRescGrpInfo = tmpRescGrpInfo->next;
    }

    if (savedStatus == 0 && destRescGrpInfo->status < 0) {
        /* resource down or quoto overrun */
        return destRescGrpInfo->status;
    } else { 
        return (savedStatus);
    }
}

/* _rsDataObjReplS - replicate a single obj 
 *   dataObjInfo_t *srcDataObjInfo - the src to be replicated. 
 *   rescInfo_t *destRescInfo - The dest resource info
 *   rescGroupName - only meaningful if the destDataObj does not exist. 
 *   dataObjInfo_t *destDataObjInfo - This can be both input and output.
 *      If destDataObjInfo == NULL, dest is new and no output is required.
 *      If destDataObjInfo != NULL:
 *          If updateFlag == 0, Output only.  Output the dataObjInfo
 *          of the replicated copy. This is needed by msiSysReplDataObj and
 *          msiStageDataObj which need a copy of destDataObjInfo.
 *          If updateFlag > 0, the dest repl exists. Need to
 *          update it.
 */
int
_rsDataObjReplS (rsComm_t *rsComm,
                 dataObjInp_t *dataObjInp,
                 dataObjInfo_t *srcDataObjInfo,
                 rescInfo_t *destRescInfo, 
                 char *rescGroupName,
                 dataObjInfo_t *destDataObjInfo,
                 int updateFlag)
{
    int status, status1;
    int l1descInx;
    openedDataObjInp_t dataObjCloseInp;
    dataObjInfo_t *myDestDataObjInfo;

    l1descInx = dataObjOpenForRepl (rsComm, dataObjInp, srcDataObjInfo,
                                    destRescInfo, rescGroupName, destDataObjInfo, updateFlag);

    if (l1descInx < 0) {
        return (l1descInx);
    }

    if (L1desc[l1descInx].stageFlag != NO_STAGING) {
        status = l3DataStageSync (rsComm, l1descInx);
    } else if (L1desc[l1descInx].dataObjInp->numThreads == 0 && 
               L1desc[l1descInx].dataObjInfo->dataSize <= MAX_SZ_FOR_SINGLE_BUF) {
        status = l3DataCopySingleBuf (rsComm, l1descInx);
    } else {
        status = dataObjCopy (rsComm, l1descInx);
    }

    memset (&dataObjCloseInp, 0, sizeof (dataObjCloseInp));

    dataObjCloseInp.l1descInx = l1descInx;
    /* myDestDataObjInfo = L1desc[l1descInx].dataObjInfo; */
    L1desc[l1descInx].oprStatus = status;
    if (status >= 0) {
        L1desc[l1descInx].bytesWritten =  L1desc[l1descInx].dataObjInfo->dataSize;
    }

    status1 = irsDataObjClose (rsComm, &dataObjCloseInp, &myDestDataObjInfo);

    if (destDataObjInfo != NULL) {
        if (destDataObjInfo->dataId <= 0 && myDestDataObjInfo != NULL) {
            destDataObjInfo->dataId = myDestDataObjInfo->dataId;
            destDataObjInfo->replNum = myDestDataObjInfo->replNum;
        } else {
            /* the size could change */
            destDataObjInfo->dataSize = myDestDataObjInfo->dataSize;
        }
    }
    freeDataObjInfo (myDestDataObjInfo);

    if (status < 0) {
        return status;
    } else if (status1 < 0) {
        return status1;
    } else {
        return (status);
    }
}

/* dataObjOpenForRepl - Create/open the dest and open the src
 */

int 
dataObjOpenForRepl (rsComm_t *rsComm,
                    dataObjInp_t *dataObjInp,
                    dataObjInfo_t *inpSrcDataObjInfo,
                    rescInfo_t *destRescInfo,
                    char *rescGroupName,
                    dataObjInfo_t *inpDestDataObjInfo,
                    int updateFlag)
{
    dataObjInfo_t *myDestDataObjInfo, *srcDataObjInfo = NULL;
    rescInfo_t *myDestRescInfo;
    int destL1descInx;
    int srcL1descInx;
    int status;
    int replStatus;
    int destRescClass;
    char *destRescName, *srcRescName;
    int srcRescClass = getRescClass (inpSrcDataObjInfo->rescInfo);
    dataObjInfo_t *cacheDataObjInfo = NULL;
    dataObjInp_t myDataObjInp, *l1DataObjInp;

    if (destRescInfo == NULL) {
        myDestRescInfo = inpDestDataObjInfo->rescInfo;
    } else {
        myDestRescInfo = destRescInfo;
    }

    if (inpSrcDataObjInfo->rescInfo->rescStatus == INT_RESC_STATUS_DOWN) {
        return SYS_RESC_IS_DOWN;
    }

    if (myDestRescInfo->rescStatus == INT_RESC_STATUS_DOWN) {
        return SYS_RESC_IS_DOWN;
    }

    destRescClass = getRescClass (myDestRescInfo);

    /* Setup the srcDataObjInfo. If inpSrcDataObjInfo is in COMPOUND_CL,
     * stage it */
    if (srcRescClass == COMPOUND_CL) {
        rescGrpInfo_t *myRescGrpInfo;
        if (destRescClass == CACHE_CL && isRescsInSameGrp (rsComm, 
                                                           myDestRescInfo->rescName, inpSrcDataObjInfo->rescInfo->rescName,
                                                           &myRescGrpInfo)) {
            /* src and dest in same resc group. no need to stage */
            if (strlen (inpSrcDataObjInfo->rescGroupName) == 0) {
                rstrcpy (inpSrcDataObjInfo->rescGroupName, 
                         myRescGrpInfo->rescGroupName, NAME_LEN);
            }
        } else if (getRescInGrp (rsComm, myDestRescInfo->rescName,
                                 inpSrcDataObjInfo->rescGroupName, NULL) < 0) {
            cacheDataObjInfo = (dataObjInfo_t*)calloc (1, sizeof (dataObjInfo_t));
            status = stageDataFromCompToCache (rsComm, inpSrcDataObjInfo,
                                               cacheDataObjInfo);
            if (status < 0) { free( cacheDataObjInfo ); return status; } // JMC cppcheck - leak
            /* srcRescClass is now CACHE_CL */
            srcRescClass = getRescClass (cacheDataObjInfo->rescInfo);
        }
    }

    if (cacheDataObjInfo == NULL) {
        srcDataObjInfo = (dataObjInfo_t*)calloc (1, sizeof (dataObjInfo_t));
        *srcDataObjInfo = *inpSrcDataObjInfo;
    } else {
        srcDataObjInfo = cacheDataObjInfo;
    }

    if( NULL == srcDataObjInfo ) { // JMC cppcheck - nullptr
        rodsLog( LOG_ERROR, "dataObjOpenForRepl - srcDataObjInfo is NULL" );
        return -1;      
    }
    /* open the dest */
    myDataObjInp = *dataObjInp;
    myDataObjInp.dataSize = inpSrcDataObjInfo->dataSize;

    destL1descInx = allocL1desc ();

    if (destL1descInx < 0) return destL1descInx;

    myDestDataObjInfo = (dataObjInfo_t*)calloc (1, sizeof (dataObjInfo_t));
    if (updateFlag > 0) {
        /* update an existing copy */
        if(inpDestDataObjInfo == NULL || inpDestDataObjInfo->dataId <= 0) {
            rodsLog (LOG_ERROR,
                     "dataObjOpenForRepl: dataId of %s copy to be updated not defined",
                     srcDataObjInfo->objPath);
            return (SYS_UPDATE_REPL_INFO_ERR);
        }
        /* inherit the replStatus of the src */
        inpDestDataObjInfo->replStatus = srcDataObjInfo->replStatus;
        *myDestDataObjInfo = *inpDestDataObjInfo;
        replStatus = srcDataObjInfo->replStatus | OPEN_EXISTING_COPY;
        addKeyVal (&myDataObjInp.condInput, FORCE_FLAG_KW, "");
        myDataObjInp.openFlags |= (O_TRUNC | O_WRONLY);
    } else {    /* a new copy */
        initDataObjInfoForRepl (rsComm, myDestDataObjInfo, srcDataObjInfo, 
                                destRescInfo, rescGroupName);
        replStatus = srcDataObjInfo->replStatus;
    }

    fillL1desc (destL1descInx, &myDataObjInp, myDestDataObjInfo, 
                replStatus, srcDataObjInfo->dataSize);
    l1DataObjInp = L1desc[destL1descInx].dataObjInp;
    if (l1DataObjInp->oprType == PHYMV_OPR) {
        L1desc[destL1descInx].oprType = PHYMV_DEST;
        myDestDataObjInfo->replNum = srcDataObjInfo->replNum;
        myDestDataObjInfo->dataId = srcDataObjInfo->dataId;
    } else {
        L1desc[destL1descInx].oprType = REPLICATE_DEST;
    }

    if (destRescClass == COMPOUND_CL) {
        L1desc[destL1descInx].stageFlag = SYNC_DEST;
    } else if (srcRescClass == COMPOUND_CL) {
        L1desc[destL1descInx].stageFlag = STAGE_SRC;
    }

    if (destRescInfo != NULL)
        destRescName = destRescInfo->rescName;
    else
        destRescName = NULL;

    if (srcDataObjInfo != NULL && srcDataObjInfo->rescInfo != NULL)
        srcRescName = srcDataObjInfo->rescInfo->rescName;
    else
        srcRescName = NULL;

    l1DataObjInp->numThreads = dataObjInp->numThreads =
        getNumThreads( rsComm, l1DataObjInp->dataSize, l1DataObjInp->numThreads, 
                       &dataObjInp->condInput, destRescName, srcRescName);

    if (l1DataObjInp->numThreads > 0 && 
        L1desc[destL1descInx].stageFlag == NO_STAGING) {
        if (updateFlag > 0) {
            status = dataOpen (rsComm, destL1descInx);
        } else {
            status = getFilePathName (rsComm, myDestDataObjInfo,
                                      L1desc[destL1descInx].dataObjInp);
            if (status >= 0) 
                status = dataCreate (rsComm, destL1descInx);
        }

        if (status < 0) {
            freeL1desc (destL1descInx);
            return (status);
        }
    } else {
        if (updateFlag == 0) {
            status = getFilePathName (rsComm, myDestDataObjInfo, 
                                      L1desc[destL1descInx].dataObjInp);
            if (status < 0) {
                freeL1desc (destL1descInx);
                return (status);
            }
        }
    }

    if (inpDestDataObjInfo != NULL && updateFlag == 0) {
        /* a new replica */
        *inpDestDataObjInfo = *myDestDataObjInfo;
        inpDestDataObjInfo->next = NULL;
    }

    /* open the src */

    srcL1descInx = allocL1desc ();
    if (srcL1descInx < 0) return srcL1descInx;
    fillL1desc (srcL1descInx, &myDataObjInp, srcDataObjInfo, 
                srcDataObjInfo->replStatus, srcDataObjInfo->dataSize);
    l1DataObjInp = L1desc[srcL1descInx].dataObjInp;
    l1DataObjInp->numThreads = dataObjInp->numThreads;
    if (l1DataObjInp->oprType == PHYMV_OPR) {
        L1desc[srcL1descInx].oprType = PHYMV_SRC;
    } else {
        L1desc[srcL1descInx].oprType = REPLICATE_SRC;
    }

    if( L1desc[destL1descInx].stageFlag == SYNC_DEST && // JMC - backport 4549
        getValByKey (&dataObjInp->condInput, PURGE_CACHE_KW) != NULL) {
        L1desc[srcL1descInx].purgeCacheFlag = 1;
    }


    if (l1DataObjInp->numThreads > 0 &&
        L1desc[destL1descInx].stageFlag == NO_STAGING) {
        openedDataObjInp_t dataObjCloseInp;

        l1DataObjInp->openFlags = O_RDONLY;
        status = dataOpen (rsComm, srcL1descInx);
        if (status < 0) {
            freeL1desc (srcL1descInx);
            memset (&dataObjCloseInp, 0, sizeof (dataObjCloseInp));
            dataObjCloseInp.l1descInx = destL1descInx;
            rsDataObjClose (rsComm, &dataObjCloseInp);
            return (status);
        }
    }
    L1desc[destL1descInx].srcL1descInx = srcL1descInx;

    return (destL1descInx);
}

int
dataObjCopy (rsComm_t *rsComm, int l1descInx)
{
    int srcL1descInx, destL1descInx;
    int srcL3descInx, destL3descInx;
    int status;
    portalOprOut_t *portalOprOut = NULL;
    dataCopyInp_t dataCopyInp;
    dataOprInp_t *dataOprInp;
    int srcRemoteFlag, destRemoteFlag;

    bzero (&dataCopyInp, sizeof (dataCopyInp));
    dataOprInp = &dataCopyInp.dataOprInp;
    srcL1descInx = L1desc[l1descInx].srcL1descInx;
    destL1descInx = l1descInx;

    srcL3descInx = L1desc[srcL1descInx].l3descInx;
    destL3descInx = L1desc[destL1descInx].l3descInx;

    if (L1desc[srcL1descInx].remoteZoneHost != NULL) {
        srcRemoteFlag = REMOTE_ZONE_HOST;
    } else {
        srcRemoteFlag = FileDesc[srcL3descInx].rodsServerHost->localFlag;
    }
 
    if (L1desc[destL1descInx].remoteZoneHost != NULL) {
        destRemoteFlag = REMOTE_ZONE_HOST;
    } else {
        destRemoteFlag = FileDesc[destL3descInx].rodsServerHost->localFlag;
    }

    if (srcRemoteFlag != REMOTE_ZONE_HOST &&
        destRemoteFlag != REMOTE_ZONE_HOST &&
        FileDesc[srcL3descInx].rodsServerHost == 
        FileDesc[destL3descInx].rodsServerHost) {
        /* local zone same host copy */
        initDataOprInp (&dataCopyInp.dataOprInp, l1descInx, SAME_HOST_COPY_OPR);
        /* dataCopyInp.portalOprOut.numThreads is needed by rsDataCopy */
        dataCopyInp.portalOprOut.numThreads = 
            dataCopyInp.dataOprInp.numThreads;
        if (srcRemoteFlag == LOCAL_HOST) {
            addKeyVal (&dataOprInp->condInput, EXEC_LOCALLY_KW, "");
        }
            
    } else if ((srcRemoteFlag == LOCAL_HOST && destRemoteFlag != LOCAL_HOST) ||
               destRemoteFlag == REMOTE_ZONE_HOST) {
        initDataOprInp (&dataCopyInp.dataOprInp, l1descInx, COPY_TO_REM_OPR);
        /* do it locally if numThreads == 0 */
        if (L1desc[l1descInx].dataObjInp->numThreads > 0) {
            /* copy from local to remote */
            /* preProcParaPut to establish portalOprOut without data transfer */
            status = preProcParaPut (rsComm, destL1descInx, &portalOprOut);
            if (status < 0 || NULL == portalOprOut ) { // JMC cppcheck - nullptr
                rodsLog (LOG_NOTICE,
                         "dataObjCopy: preProcParaPut error for %s",
                         L1desc[srcL1descInx].dataObjInfo->objPath);
                return (status);
            }
            dataCopyInp.portalOprOut = *portalOprOut;
        } else {
            dataCopyInp.portalOprOut.l1descInx = destL1descInx;
        }
        if (srcRemoteFlag == LOCAL_HOST)
            addKeyVal (&dataOprInp->condInput, EXEC_LOCALLY_KW, "");
    } else if ((srcRemoteFlag != LOCAL_HOST && destRemoteFlag == LOCAL_HOST) ||
               srcRemoteFlag == REMOTE_ZONE_HOST) {
        /* copy from remote to local */
        initDataOprInp (&dataCopyInp.dataOprInp, l1descInx, COPY_TO_LOCAL_OPR);
        /* do it locally if numThreads == 0 */
        if (L1desc[l1descInx].dataObjInp->numThreads > 0) {
            /* preProcParaGet to establish portalOprOut without data transfer */
            status = preProcParaGet (rsComm, srcL1descInx, &portalOprOut);
            if (status < 0 || NULL == portalOprOut ) { // JMC cppcheck - null ptr
                rodsLog (LOG_NOTICE,
                         "dataObjCopy: preProcParaGet error for %s",
                         L1desc[srcL1descInx].dataObjInfo->objPath);
                return (status);
            }
            dataCopyInp.portalOprOut = *portalOprOut;
        } else {
            dataCopyInp.portalOprOut.l1descInx = srcL1descInx;
        }
        if (destRemoteFlag == LOCAL_HOST)
            addKeyVal (&dataOprInp->condInput, EXEC_LOCALLY_KW, "");
    } else {
        /* remote to remote */
        initDataOprInp (&dataCopyInp.dataOprInp, l1descInx, COPY_TO_LOCAL_OPR);
        /* preProcParaGet only establish &portalOprOut without data transfer */
        /* do it locally if numThreads == 0 */
        if (L1desc[l1descInx].dataObjInp->numThreads > 0) {
            status = preProcParaGet (rsComm, srcL1descInx, &portalOprOut);

            if (status < 0 || NULL == portalOprOut ) { // JMC cppcheck - null ptr
                rodsLog (LOG_NOTICE,
                         "dataObjCopy: preProcParaGet error for %s", 
                         L1desc[srcL1descInx].dataObjInfo->objPath);
                return (status);
            }
            dataCopyInp.portalOprOut = *portalOprOut;
        } else {
            dataCopyInp.portalOprOut.l1descInx = srcL1descInx;
        }
    }
    /* rsDataCopy - does the physical data transfer */
    if (getValByKey (&L1desc[l1descInx].dataObjInp->condInput, 
                     NO_CHK_COPY_LEN_KW) != NULL) {
        /* don't check the transfer len */
        addKeyVal (&dataOprInp->condInput, NO_CHK_COPY_LEN_KW, "");
        if (L1desc[l1descInx].dataObjInp->numThreads > 1) {
            L1desc[l1descInx].dataObjInp->numThreads = 
                L1desc[srcL1descInx].dataObjInp->numThreads =
                dataCopyInp.portalOprOut.numThreads = 1;
        }
    }
    status =  rsDataCopy (rsComm, &dataCopyInp);

    if (status >= 0 && portalOprOut != NULL && 
        L1desc[l1descInx].dataObjInp != NULL) {
        /* update numThreads since it could be chnages by remote server */ 
        L1desc[l1descInx].dataObjInp->numThreads = portalOprOut->numThreads;
    }
    if (portalOprOut != NULL) free (portalOprOut);
    clearKeyVal (&dataOprInp->condInput);
        
    return (status);
}

int
l3DataCopySingleBuf (rsComm_t *rsComm, int l1descInx)
{
    bytesBuf_t dataBBuf;
    int bytesRead, bytesWritten;
    int srcL1descInx;

    memset (&dataBBuf, 0, sizeof (bytesBuf_t));

    srcL1descInx = L1desc[l1descInx].srcL1descInx;
    if (L1desc[srcL1descInx].dataSize < 0) {
        rodsLog (LOG_ERROR,
                 "l3DataCopySingleBuf: dataSize %lld for %s is negative",
                 L1desc[srcL1descInx].dataSize, 
                 L1desc[srcL1descInx].dataObjInfo->objPath); 
        return (SYS_COPY_LEN_ERR);
    } else if (L1desc[srcL1descInx].dataSize == 0) {
        bytesRead = 0;
    } else {
        dataBBuf.buf = malloc (L1desc[srcL1descInx].dataSize);
        bytesRead = rsL3FileGetSingleBuf (rsComm, &srcL1descInx, &dataBBuf);
    }

    if (bytesRead < 0) {
        free( dataBBuf.buf ); // JMC cppcheck - leak
        return (bytesRead);
    } else if (getValByKey (&L1desc[l1descInx].dataObjInp->condInput,
                            NO_CHK_COPY_LEN_KW) != NULL) {
        /* need to update size */
        L1desc[l1descInx].dataSize = L1desc[l1descInx].dataObjInp->dataSize
            = bytesRead;
    }
    bytesWritten = rsL3FilePutSingleBuf (rsComm, &l1descInx, &dataBBuf);
 
    L1desc[l1descInx].bytesWritten = bytesWritten;

    if (dataBBuf.buf != NULL) {
        free (dataBBuf.buf);
        memset (&dataBBuf, 0, sizeof (bytesBuf_t));
    }

    if (bytesWritten != bytesRead) {
        if (bytesWritten >= 0) {
            rodsLog (LOG_NOTICE,
                     "l3DataCopySingleBuf: l3FilePut error, towrite %d, written %d",
                     bytesRead, bytesWritten);
            return (SYS_COPY_LEN_ERR);
        } else {
            return (bytesWritten);
        }
    }
        

    return (0); 
}

int
l3DataStageSync (rsComm_t *rsComm, int l1descInx)
{
    bytesBuf_t dataBBuf;
    int srcL1descInx;
    int status = 0;

    memset (&dataBBuf, 0, sizeof (bytesBuf_t));

    srcL1descInx = L1desc[l1descInx].srcL1descInx;
    if (L1desc[srcL1descInx].dataSize < 0 && 
        L1desc[srcL1descInx].dataSize != UNKNOWN_FILE_SZ) {
        rodsLog (LOG_ERROR,
                 "l3DataStageSync: dataSize %lld for %s is negative",
                 L1desc[srcL1descInx].dataSize, 
                 L1desc[srcL1descInx].dataObjInfo->objPath); 
        return (SYS_COPY_LEN_ERR);
    } else if (L1desc[srcL1descInx].dataSize > 0 || 
               L1desc[srcL1descInx].dataSize == UNKNOWN_FILE_SZ) {
        if (L1desc[l1descInx].stageFlag == SYNC_DEST) {
            /* dest a DO_STAGE type, sync */
            status = l3FileSync (rsComm, srcL1descInx, l1descInx);
        } else {
            /* src a DO_STAGE type, stage */
            status = l3FileStage (rsComm, srcL1descInx, l1descInx);
        }
    }

    if (status < 0) {
        L1desc[l1descInx].bytesWritten = -1;
    } else {
        L1desc[l1descInx].bytesWritten = L1desc[l1descInx].dataSize =
            L1desc[srcL1descInx].dataSize;
    }

    return (status); 
}

int
l3FileSync (rsComm_t *rsComm, int srcL1descInx, int destL1descInx)
{
    dataObjInfo_t *srcDataObjInfo, *destDataObjInfo;
    int rescTypeInx, cacheRescTypeInx;
    fileStageSyncInp_t fileSyncToArchInp;
    dataObjInp_t *dataObjInp;
    int status;
    char *outFileName = NULL;   /* for fileSyncToArch */

    srcDataObjInfo = L1desc[srcL1descInx].dataObjInfo;
    destDataObjInfo = L1desc[destL1descInx].dataObjInfo;

    rescTypeInx = destDataObjInfo->rescInfo->rescTypeInx;
    cacheRescTypeInx = srcDataObjInfo->rescInfo->rescTypeInx;

    switch (RescTypeDef[rescTypeInx].rescCat) {
        dataObjInfo_t tmpDataObjInfo;
    case FILE_CAT:
        /* make sure the fileName is not already taken */
        if (RescTypeDef[rescTypeInx].createPathFlag == CREATE_PATH) {
            status = chkOrphanFile ( rsComm, destDataObjInfo->filePath, destDataObjInfo->rescName, &tmpDataObjInfo );
            if (status == 0 && tmpDataObjInfo.dataId != destDataObjInfo->dataId) {
                /* someone is using it */
                snprintf (destDataObjInfo->filePath, MAX_NAME_LEN, 
                          "%s.%-d", destDataObjInfo->filePath, (int) random());
            }
        }

        memset (&fileSyncToArchInp, 0, sizeof (fileSyncToArchInp));
        dataObjInp                      = L1desc[destL1descInx].dataObjInp;
        fileSyncToArchInp.dataSize      = srcDataObjInfo->dataSize;
        fileSyncToArchInp.fileType      = (fileDriverType_t)RescTypeDef[rescTypeInx].driverType;
        fileSyncToArchInp.cacheFileType = (fileDriverType_t)RescTypeDef[cacheRescTypeInx].driverType;
        
        rstrcpy( fileSyncToArchInp.addr.hostAddr,  srcDataObjInfo->rescInfo->rescLoc, NAME_LEN );

        /* use cache addr destDataObjInfo->rescInfo->rescLoc, NAME_LEN); */
        rstrcpy( fileSyncToArchInp.filename,      destDataObjInfo->filePath, MAX_NAME_LEN);
        rstrcpy( fileSyncToArchInp.cacheFilename, srcDataObjInfo->filePath,  MAX_NAME_LEN);

        fileSyncToArchInp.mode = getFileMode (dataObjInp);
        status = rsFileSyncToArch (rsComm, &fileSyncToArchInp, &outFileName);

        if (status >= 0 && 
            RescTypeDef[rescTypeInx].createPathFlag == NO_CREATE_PATH &&
            outFileName != NULL) {
            /* path name is created by the resource */
            rstrcpy (destDataObjInfo->filePath, outFileName, MAX_NAME_LEN);
            L1desc[destL1descInx].replStatus |= FILE_PATH_HAS_CHG;
            free (outFileName);
        }
        break;
    default:
        rodsLog (LOG_ERROR,
                 "l3FileSync: rescCat type %d is not recognized",
                 RescTypeDef[rescTypeInx].rescCat);
        status = SYS_INVALID_RESC_TYPE;
        break;
    }
    return (status);
}

int
l3FileStage (rsComm_t *rsComm, int srcL1descInx, int destL1descInx)
{
    dataObjInfo_t *srcDataObjInfo, *destDataObjInfo;
    int mode, status; // JMC - backport 4527

    srcDataObjInfo = L1desc[srcL1descInx].dataObjInfo;
    destDataObjInfo = L1desc[destL1descInx].dataObjInfo;

    mode = getFileMode (L1desc[destL1descInx].dataObjInp);

    status = _l3FileStage (rsComm, srcDataObjInfo, destDataObjInfo, mode);

    return status;
}

int
_l3FileStage (rsComm_t *rsComm, dataObjInfo_t *srcDataObjInfo, // JMC - backport 4527
              dataObjInfo_t *destDataObjInfo, int mode)
{
    int rescTypeInx, cacheRescTypeInx;
    fileStageSyncInp_t fileSyncToArchInp;
    int status;

    rescTypeInx = srcDataObjInfo->rescInfo->rescTypeInx;
    cacheRescTypeInx = destDataObjInfo->rescInfo->rescTypeInx;


    switch (RescTypeDef[rescTypeInx].rescCat) {
    case FILE_CAT:
        memset (&fileSyncToArchInp, 0, sizeof (fileSyncToArchInp));
        fileSyncToArchInp.dataSize = srcDataObjInfo->dataSize;
        fileSyncToArchInp.fileType = (fileDriverType_t)RescTypeDef[rescTypeInx].driverType;
        fileSyncToArchInp.cacheFileType = 
            (fileDriverType_t)RescTypeDef[cacheRescTypeInx].driverType;
        rstrcpy (fileSyncToArchInp.addr.hostAddr,  
                 destDataObjInfo->rescInfo->rescLoc, NAME_LEN);
        /* use the cache addr srcDataObjInfo->rescInfo->rescLoc, NAME_LEN);*/
        rstrcpy( fileSyncToArchInp.cacheFilename, destDataObjInfo->filePath, 
                 MAX_NAME_LEN);
        rstrcpy( fileSyncToArchInp.filename, srcDataObjInfo->filePath, 
                 MAX_NAME_LEN);
        fileSyncToArchInp.mode = mode;
        status = rsFileStageToCache (rsComm, &fileSyncToArchInp);
        break;
    default:
        rodsLog (LOG_ERROR,
                 "l3FileStage: rescCat type %d is not recognized",
                 RescTypeDef[rescTypeInx].rescCat);
        status = SYS_INVALID_RESC_TYPE;
        break;
    }
    return (status);
}

/* rsReplAndRequeDataObjInfo - This routine handle the msiSysReplDataObj
 * micro-service. It replicates from srcDataObjInfoHead to destRescName
 * and support options flags given in flagStr.
 * Flags supported are: ALL_KW, RBUDP_TRANSFER_KW, SU_CLIENT_USER_KW
 * and UPDATE_REPL_KW. Multiple flags can be input with % as separator.
 * The replicated DataObjInfoHead will be put on top of the queue.
 */
 
int
rsReplAndRequeDataObjInfo (rsComm_t *rsComm, 
                           dataObjInfo_t **srcDataObjInfoHead, char *destRescName, char *flagStr)
{
    dataObjInfo_t *dataObjInfoHead, *myDataObjInfo;
    transferStat_t transStat;
    dataObjInp_t dataObjInp;
    char tmpStr[NAME_LEN];
    int status;

    dataObjInfoHead = *srcDataObjInfoHead;
    myDataObjInfo = (dataObjInfo_t*)malloc (sizeof (dataObjInfo_t));
    memset (myDataObjInfo, 0, sizeof (dataObjInfo_t));
    memset (&dataObjInp, 0, sizeof (dataObjInp_t));
    memset (&transStat, 0, sizeof (transStat));

    if (flagStr != NULL) {
        if (strstr (flagStr, ALL_KW) != NULL) {
            addKeyVal (&dataObjInp.condInput, ALL_KW, "");
        }
        if (strstr (flagStr, RBUDP_TRANSFER_KW) != NULL) {
            addKeyVal (&dataObjInp.condInput, RBUDP_TRANSFER_KW, "");
        }
        if (strstr (flagStr, SU_CLIENT_USER_KW) != NULL) {
            addKeyVal (&dataObjInp.condInput, SU_CLIENT_USER_KW, "");
        }
        if (strstr (flagStr, UPDATE_REPL_KW) != NULL) {
            addKeyVal (&dataObjInp.condInput, UPDATE_REPL_KW, "");
        }
    }

    rstrcpy (dataObjInp.objPath, dataObjInfoHead->objPath, MAX_NAME_LEN);
    snprintf (tmpStr, NAME_LEN, "%d", dataObjInfoHead->replNum);
    addKeyVal (&dataObjInp.condInput, REPL_NUM_KW, tmpStr);
    addKeyVal (&dataObjInp.condInput, DEST_RESC_NAME_KW, destRescName);

    status = _rsDataObjRepl (rsComm, &dataObjInp, &transStat, 
                             myDataObjInfo);

    /* fix mem leak */
    clearKeyVal (&dataObjInp.condInput);
    if (status >= 0) {
        status = 1;
        /* que the cache copy at the top */
        queDataObjInfo (srcDataObjInfoHead, myDataObjInfo, 0, 1);
    } else {
        freeAllDataObjInfo (myDataObjInfo);
    }

    return status;
}

int
stageDataFromCompToCache (rsComm_t *rsComm, dataObjInfo_t *compObjInfo,
                          dataObjInfo_t *outCacheObjInfo)
{
    int status;
    rescInfo_t *cacheResc;
    transferStat_t transStat;
    dataObjInp_t dataObjInp;
    char tmpStr[NAME_LEN];

    if (getRescClass (compObjInfo->rescInfo) != COMPOUND_CL) return 0;

    status = getCacheRescInGrp (rsComm, compObjInfo->rescGroupName,
                                compObjInfo->rescInfo, &cacheResc);
    if (status < 0) {
        rodsLog (LOG_ERROR,
                 "stageDataFromCompToCache: getCacheRescInGrp %s failed for %s stat=%d",
                 compObjInfo->rescGroupName, compObjInfo->objPath, status);
        return status;
    }
    if (outCacheObjInfo != NULL)
        memset (outCacheObjInfo, 0, sizeof (dataObjInfo_t));
    memset (&dataObjInp, 0, sizeof (dataObjInp_t));
    memset (&transStat, 0, sizeof (transStat));

    rstrcpy (dataObjInp.objPath, compObjInfo->objPath, MAX_NAME_LEN);
    snprintf (tmpStr, NAME_LEN, "%d", compObjInfo->replNum);
    addKeyVal (&dataObjInp.condInput, REPL_NUM_KW, tmpStr);
    addKeyVal (&dataObjInp.condInput, DEST_RESC_NAME_KW, cacheResc->rescName);

    status = _rsDataObjRepl (rsComm, &dataObjInp, &transStat,
                             outCacheObjInfo);

    clearKeyVal (&dataObjInp.condInput);
    return status;
}

/* stageAndRequeDataToCache - stage the compund copy in compObjInfoHead
 * to a cache resource. Put the cache copy on to on the compObjInfoHead
 * queue
 */ 

int
stageAndRequeDataToCache (rsComm_t *rsComm, dataObjInfo_t **compObjInfoHead)
{
    int status;
    dataObjInfo_t *outCacheObjInfo;

    outCacheObjInfo = (dataObjInfo_t*)malloc (sizeof (dataObjInfo_t));
    bzero (outCacheObjInfo, sizeof (dataObjInfo_t));
    status = stageDataFromCompToCache (rsComm, *compObjInfoHead,
                                       outCacheObjInfo);

    if (status < 0) {
        /* if (status == SYS_COPY_ALREADY_IN_RESC) { */
        if (outCacheObjInfo->dataId > 0) {
            /* put the cache copy on top */
            if( requeDataObjInfoByReplNum (compObjInfoHead, 
                                           outCacheObjInfo->replNum) == 0) { // JMC - backport 4608
                /* sucessfully requeued */
                status = 0;
            }
        }
        free (outCacheObjInfo);
    } else {
        queDataObjInfo (compObjInfoHead, outCacheObjInfo, 0, 1);
    }
    return status;
}

int
replToCacheRescOfCompObj (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                          dataObjInfo_t *srcDataObjInfoHead, dataObjInfo_t *compObjInfo, 
                          dataObjInfo_t *oldDataObjInfo, dataObjInfo_t **outDestDataObjInfo)
{
    int status = 0;
    rescInfo_t *cacheResc;
    dataObjInfo_t *destDataObjInfo, *srcDataObjInfo;
    dataObjInfo_t *tmpDestDataObjInfo = NULL;
    int updateFlag;

    if ((status = getCacheDataInfoForRepl (rsComm, oldDataObjInfo, NULL, compObjInfo, &tmpDestDataObjInfo)) >= 0) {
        cacheResc = oldDataObjInfo->rescInfo;
        updateFlag = 1;
    } else {
        /* no old copy */
        status = getCacheRescInGrp (rsComm, compObjInfo->rescGroupName,
                                    compObjInfo->rescInfo, &cacheResc);
        if (status < 0) {
            rodsLog (LOG_ERROR,
                     "replToCacheRescOfCompObj:getCacheRescInGrp %s err for %s stat=%d",
                     compObjInfo->rescGroupName, compObjInfo->objPath, status);
            return status;
        }
        updateFlag = 0;
    }

    if (outDestDataObjInfo == NULL) {
        destDataObjInfo = tmpDestDataObjInfo;
    } else {
        destDataObjInfo = (dataObjInfo_t*)malloc (sizeof (dataObjInfo_t));
        if (tmpDestDataObjInfo == NULL) {
            bzero (destDataObjInfo, sizeof (dataObjInfo_t));
        } else {
            *destDataObjInfo = *tmpDestDataObjInfo;
        }
    }
    srcDataObjInfo = srcDataObjInfoHead;
    while (srcDataObjInfo != NULL) {
        status = _rsDataObjReplS (rsComm, dataObjInp, srcDataObjInfo,
                                  cacheResc, compObjInfo->rescGroupName, destDataObjInfo, updateFlag);
        if (status >= 0) {
            if (outDestDataObjInfo != NULL) 
                *outDestDataObjInfo = destDataObjInfo;
            break;
        }
        srcDataObjInfo = srcDataObjInfo->next;
    }

    return status;
}

int
stageBundledData (rsComm_t *rsComm, dataObjInfo_t **subfileObjInfoHead)
{
    int status;
    dataObjInfo_t *dataObjInfoHead = *subfileObjInfoHead;
    rescInfo_t *cacheResc;
    dataObjInp_t dataObjInp;
    dataObjInfo_t *cacheObjInfo;

    if (getRescClass (dataObjInfoHead->rescInfo) != BUNDLE_CL) return 0;

    status = unbunAndStageBunfileObj (rsComm, dataObjInfoHead->filePath,
                                      &cacheResc);

    if (status < 0) return status;

    /* query the bundle dataObj */
    bzero (&dataObjInp, sizeof (dataObjInp));
    rstrcpy (dataObjInp.objPath, dataObjInfoHead->objPath, MAX_NAME_LEN);
    addKeyVal (&dataObjInp.condInput, RESC_NAME_KW, cacheResc->rescName);
    status = getDataObjInfo (rsComm, &dataObjInp, &cacheObjInfo, NULL, 0);
    clearKeyVal (&dataObjInp.condInput);
    if (status < 0) {
        rodsLog (LOG_ERROR,
                 "unbunAndStageBunfileObj: getDataObjInfo of subfile %s failed.stat=%d",
                 dataObjInp.objPath, status);
        return status;
    }
    /* que the cache copy at the top */
    queDataObjInfo (subfileObjInfoHead, cacheObjInfo, 0, 1);


    return status;
}

int
unbunAndStageBunfileObj (rsComm_t *rsComm, char *bunfileObjPath, 
                         rescInfo_t **outCacheResc)
{
    dataObjInfo_t *bunfileObjInfoHead;
    dataObjInp_t dataObjInp;
    int status;

    /* query the bundle dataObj */
    bzero (&dataObjInp, sizeof (dataObjInp));
    rstrcpy (dataObjInp.objPath, bunfileObjPath, MAX_NAME_LEN);
    
    status = getDataObjInfo (rsComm, &dataObjInp, &bunfileObjInfoHead, NULL, 1);
    if (status < 0) {
        rodsLog (LOG_ERROR,
                 "unbunAndStageBunfileObj: getDataObjInfo of bunfile %s failed.stat=%d",
                 dataObjInp.objPath, status);
        return status;
    }
    status = _unbunAndStageBunfileObj (rsComm, &bunfileObjInfoHead, 
                                       outCacheResc, 0);

    freeAllDataObjInfo (bunfileObjInfoHead);
    
    return status;
}

int
_unbunAndStageBunfileObj (rsComm_t *rsComm, dataObjInfo_t **bunfileObjInfoHead,
                          rescInfo_t **outCacheResc, int rmBunCopyFlag)
{
    int status;
    rescInfo_t *cacheResc;
    dataObjInp_t dataObjInp;

    bzero (&dataObjInp, sizeof (dataObjInp));
    rstrcpy (dataObjInp.objPath, (*bunfileObjInfoHead)->objPath, MAX_NAME_LEN);
    status = sortObjInfoForOpen (rsComm, bunfileObjInfoHead, NULL, 0);
    if (status < 0) return status;

    if (getRescClass ((*bunfileObjInfoHead)->rescInfo) != CACHE_CL) {
        /* don't have a good copy on cache yet */
        status = getCacheRescInGrp (rsComm, 
                                    (*bunfileObjInfoHead)->rescGroupName,
                                    (*bunfileObjInfoHead)->rescInfo, &cacheResc);
        if (status < 0) {
            rodsLog (LOG_ERROR,
                     "unbunAndStageBunfileObj:getCacheRescInGrp %s err for %s stat=%d",
                     (*bunfileObjInfoHead)->rescGroupName, 
                     (*bunfileObjInfoHead)->objPath, status);
            return status;
        }
        if (outCacheResc != NULL)
            *outCacheResc = cacheResc;

        /* XXXXXX need to take care of permission */
        status = rsReplAndRequeDataObjInfo (rsComm, bunfileObjInfoHead,
                                            cacheResc->rescName, SU_CLIENT_USER_KW);
        if (status < 0) {
            rodsLog (LOG_ERROR,
                     "unbunAndStageBunfileObj:rsReplAndRequeDataObjInfo %s err stat=%d",
                     (*bunfileObjInfoHead)->objPath, status);
            return status;
        }
    } else {
        if (outCacheResc != NULL)
            *outCacheResc = (*bunfileObjInfoHead)->rescInfo;
    }
    addKeyVal (&dataObjInp.condInput, BUN_FILE_PATH_KW,  // JMC - backport 4768
               (*bunfileObjInfoHead)->filePath);
    if (rmBunCopyFlag > 0) {
        addKeyVal (&dataObjInp.condInput, RM_BUN_COPY_KW, "");
    }
    if (strlen ((*bunfileObjInfoHead)->dataType) > 0) { // JMC - backport 4664
        addKeyVal (&dataObjInp.condInput, DATA_TYPE_KW, 
                   (*bunfileObjInfoHead)->dataType);
    }
    status = _rsUnbunAndRegPhyBunfile (rsComm, &dataObjInp, 
                                       (*bunfileObjInfoHead)->rescInfo);

    return status;
}

/* getCacheDataInfoOfCompObj - get the Cache DataInfo belong to the 
 * same resource group as the compDataObjInfo. If one does not
 * exist, make one.
 * srcDataObjInfoHead - the source DataObjInfo where the Cache copy
 *    may have already existed.
 * destDataObjInfoHead - the list may be searched by getCacheDataInfoForRepl
 *    when searching for the existence of the cache copy.
 * oldDataObjInfo - the list may be searched by replToCacheRescOfCompObj
 *    as a target.
 * outDataObjInfo - points to the resulting cache dataObjInfo. No new
 *    dataObjInfo is allocated. The new dataObjInfo is in destDataObjInfoHead
 *    or destDataObjInfoHead.
 *
 */

int
getCacheDataInfoOfCompObj (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                           dataObjInfo_t *srcDataObjInfoHead, dataObjInfo_t *destDataObjInfoHead,  
                           dataObjInfo_t *compDataObjInfo, dataObjInfo_t *oldDataObjInfo,
                           dataObjInfo_t **outDataObjInfo)
{
    int status;

    if ((status = getCacheDataInfoForRepl (rsComm, srcDataObjInfoHead,
                                           destDataObjInfoHead, compDataObjInfo, outDataObjInfo)) < 0) {
        /* we don't have a good cache copy, make one */
        status = replToCacheRescOfCompObj (rsComm, dataObjInp,
                                           srcDataObjInfoHead, compDataObjInfo, oldDataObjInfo, 
                                           outDataObjInfo);
        if (status < 0) {
            rodsLog (LOG_ERROR,
                     "_rsDataObjRepl: replToCacheRescOfCompObj of %s error stat=%d",
                     srcDataObjInfoHead->objPath, status);
            return status;
        }
        /* save newly created. It is OK to use &srcDataObjInfoHead
         * because it will be queued at the end */
        queDataObjInfo (&srcDataObjInfoHead, *outDataObjInfo, 1, 0);
    }
    return status;
}

int
getCacheDataInfoOfCompResc (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                            dataObjInfo_t *srcDataObjInfoHead, dataObjInfo_t *destDataObjInfoHead,
                            rescGrpInfo_t *compRescGrpInfo, dataObjInfo_t *oldDataObjInfo,
                            dataObjInfo_t **outDataObjInfo)
{
    int status;
    dataObjInfo_t compDataObjInfo;

    /* craft a fake compDataObjInfo */
    bzero (&compDataObjInfo, sizeof (compDataObjInfo));
    rstrcpy (compDataObjInfo.rescGroupName, compRescGrpInfo->rescGroupName,
             NAME_LEN);
    compDataObjInfo.rescInfo = compRescGrpInfo->rescInfo;
    status = getCacheDataInfoOfCompObj (rsComm, dataObjInp,
                                        srcDataObjInfoHead, destDataObjInfoHead, &compDataObjInfo, 
                                        oldDataObjInfo, outDataObjInfo);

    return status;
}

