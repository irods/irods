/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsRmColl.c
 */

#include "rmCollOld.h"
#include "rmColl.h"
#include "objMetaOpr.h"
#include "specColl.h"
#include "icatHighLevelRoutines.h"
#include "openCollection.h"
#include "readCollection.h"
#include "closeCollection.h"
#include "dataObjUnlink.h"
#include "rsApiHandler.h"

int
rsRmColl (rsComm_t *rsComm, collInp_t *rmCollInp,
collOprStat_t **collOprStat)
{
    int status;
    ruleExecInfo_t rei;
    collInfo_t collInfo;
    rodsServerHost_t *rodsServerHost = NULL;
    specCollCache_t *specCollCache = NULL;

    resolveLinkedPath (rsComm, rmCollInp->collName, &specCollCache,
      &rmCollInp->condInput);
    status = getAndConnRcatHost (rsComm, MASTER_RCAT,
     rmCollInp->collName, &rodsServerHost);

    if (status < 0) {
        return (status);
    } else if (rodsServerHost->rcatEnabled == REMOTE_ICAT) {
	int retval;
        retval = _rcRmColl (rodsServerHost->conn, rmCollInp, collOprStat);
	status = svrSendZoneCollOprStat (rsComm, rodsServerHost->conn,
	  *collOprStat, retval);
        return status;
    }

    initReiWithCollInp (&rei, rsComm, rmCollInp, &collInfo);

    status = applyRule ("acPreprocForRmColl", NULL, &rei, NO_SAVE_REI);

    if (status < 0) {
        if (rei.status < 0) {
            status = rei.status;
        }
        rodsLog (LOG_ERROR,
         "rsRmColl:acPreprocForRmColl error for %s,stat=%d",
          rmCollInp->collName, status);
        return status;
    }

    if (collOprStat != NULL)
        *collOprStat = NULL;
    if (getValByKey (&rmCollInp->condInput, RECURSIVE_OPR__KW) == NULL) {
	status = _rsRmColl (rsComm, rmCollInp, collOprStat);
    } else {
	if (isTrashPath (rmCollInp->collName) == False &&
	  getValByKey (&rmCollInp->condInput, FORCE_FLAG_KW) != NULL) {
            rodsLog (LOG_NOTICE,
              "rsRmColl: Recursively removing %s.",
              rmCollInp->collName);
	}
        status = _rsRmCollRecur (rsComm, rmCollInp, collOprStat);
    }
    rei.status = status;
    rei.status = applyRule ("acPostProcForRmColl", NULL, &rei,
      NO_SAVE_REI);

    if (rei.status < 0) {
        rodsLog (LOG_ERROR,
         "rsRmColl:acPostProcForRmColl error for %s,stat=%d",
          rmCollInp->collName, status);
    }

    return status;
}

int
_rsRmColl (rsComm_t *rsComm, collInp_t *rmCollInp,
collOprStat_t **collOprStat)
{
    int status;
    dataObjInfo_t *dataObjInfo = NULL;

    if (getValByKey (&rmCollInp->condInput, UNREG_COLL_KW) != NULL) {
        status = svrUnregColl (rsComm, rmCollInp);
    } else {
	/* need to check if it is spec coll */
#if 0
        dataObjInp_t dataObjInp;
        memset (&dataObjInp, 0, sizeof (dataObjInp));
        rstrcpy (dataObjInp.objPath, rmCollInp->collName, MAX_NAME_LEN);
#endif
        status = resolvePathInSpecColl (rsComm, rmCollInp->collName, 
	  WRITE_COLL_PERM, 0, &dataObjInfo);
        if (status < 0 && status != CAT_NO_ROWS_FOUND) {
	    return status;
        } else if (status == COLL_OBJ_T && dataObjInfo->specColl != NULL) {
	    if (dataObjInfo->specColl->collClass == LINKED_COLL) {
                rstrcpy (rmCollInp->collName, dataObjInfo->objPath,
                  MAX_NAME_LEN);
                status = svrUnregColl (rsComm, rmCollInp);
	    } else {
                status = l3Rmdir (rsComm, dataObjInfo);
	    }
	    freeDataObjInfo (dataObjInfo);
        } else {
	    status = svrUnregColl (rsComm, rmCollInp);
        }
    }
    if (status >= 0 && collOprStat != NULL) {
        *collOprStat = (collOprStat_t*)malloc (sizeof (collOprStat_t));
        memset (*collOprStat, 0, sizeof (collOprStat_t));
	(*collOprStat)->filesCnt = 1; 
	(*collOprStat)->totalFileCnt = 1; 
	rstrcpy ((*collOprStat)->lastObjPath, rmCollInp->collName,
           MAX_NAME_LEN);
    }
    return (status);
}

int
_rsRmCollRecur (rsComm_t *rsComm, collInp_t *rmCollInp,
collOprStat_t **collOprStat)
{
    int status;
    ruleExecInfo_t rei;
    int trashPolicy;
    dataObjInfo_t *dataObjInfo = NULL;
#if 0
    dataObjInp_t dataObjInp;

    memset (&dataObjInp, 0, sizeof (dataObjInp));
    rstrcpy (dataObjInp.objPath, rmCollInp->collName, MAX_NAME_LEN);
#endif
    /* check for specColl and permission */
    status = resolvePathInSpecColl (rsComm, rmCollInp->collName, 
      WRITE_COLL_PERM, 0, &dataObjInfo);

    if (status < 0 && status != CAT_NO_ROWS_FOUND) {
        return status;
    }
    if (status == COLL_OBJ_T && dataObjInfo->specColl != NULL &&
      dataObjInfo->specColl->collClass == LINKED_COLL) {
        rstrcpy (rmCollInp->collName, dataObjInfo->objPath,
          MAX_NAME_LEN);
	free (dataObjInfo->specColl);
	dataObjInfo->specColl = NULL;
    }
    if (status != COLL_OBJ_T || dataObjInfo->specColl == NULL) {
	/* a normal coll */
	if (rmCollInp->oprType != UNREG_OPR &&
	  getValByKey (&rmCollInp->condInput, FORCE_FLAG_KW) == NULL &&
	  getValByKey (&rmCollInp->condInput, IRODS_RMTRASH_KW) == NULL &&
	  getValByKey (&rmCollInp->condInput, IRODS_ADMIN_RMTRASH_KW) == NULL) {
            initReiWithDataObjInp (&rei, rsComm, NULL);
            status = applyRule ("acTrashPolicy", NULL, &rei, NO_SAVE_REI);
            trashPolicy = rei.status;
            if (trashPolicy != NO_TRASH_CAN) {
                status = rsMvCollToTrash (rsComm, rmCollInp);
                if (status >= 0 && collOprStat != NULL) {
		    if (*collOprStat == NULL) {
                        *collOprStat = (collOprStat_t*)malloc (sizeof (collOprStat_t));
                        memset (*collOprStat, 0, sizeof (collOprStat_t));
		    }
                    (*collOprStat)->filesCnt = 1;
                    (*collOprStat)->totalFileCnt = 1;
                    rstrcpy ((*collOprStat)->lastObjPath, rmCollInp->collName,
                       MAX_NAME_LEN);
		}
                return status;
	    }
	}
    }
    /* got here. will recursively phy delete the collection */
    status = _rsPhyRmColl (rsComm, rmCollInp, dataObjInfo, collOprStat);

    if (dataObjInfo != NULL) freeDataObjInfo (dataObjInfo);
    return (status);
}

int
_rsPhyRmColl (rsComm_t *rsComm, collInp_t *rmCollInp,
dataObjInfo_t *dataObjInfo, collOprStat_t **collOprStat)
{
    int status;
    collInp_t openCollInp;
    collEnt_t *collEnt;
    int handleInx;
    dataObjInp_t dataObjInp;
    collInp_t tmpCollInp;
    int rmtrashFlag =0;
    int savedStatus = 0;
    int fileCntPerStatOut = FILE_CNT_PER_STAT_OUT;
    int entCnt = 0;

    memset (&openCollInp, 0, sizeof (openCollInp));
    rstrcpy (openCollInp.collName, rmCollInp->collName, MAX_NAME_LEN);
    /* cannot query recur because collection is sorted in wrong order */
    openCollInp.flags = 0;
    handleInx = rsOpenCollection (rsComm, &openCollInp);
    if (handleInx < 0) {
        rodsLog (LOG_ERROR,
          "_rsPhyRmColl: rsOpenCollection of %s error. status = %d",
          openCollInp.collName, handleInx);
        return (handleInx);
    }

    if (collOprStat != NULL && *collOprStat == NULL) {
        *collOprStat = (collOprStat_t*)malloc (sizeof (collOprStat_t));
        memset (*collOprStat, 0, sizeof (collOprStat_t));
    }

    memset (&dataObjInp, 0, sizeof (dataObjInp));
    memset (&tmpCollInp, 0, sizeof (tmpCollInp));
    /* catch the UNREG_OPR */
    dataObjInp.oprType = tmpCollInp.oprType = rmCollInp->oprType;
    addKeyVal (&dataObjInp.condInput, FORCE_FLAG_KW, "");
    addKeyVal (&tmpCollInp.condInput, FORCE_FLAG_KW, "");
    if (getValByKey (&rmCollInp->condInput, IRODS_ADMIN_RMTRASH_KW) != NULL) {
        if (isTrashPath (rmCollInp->collName) == False) {
            return (SYS_INVALID_FILE_PATH);
        }
        if (rsComm->clientUser.authInfo.authFlag != LOCAL_PRIV_USER_AUTH) {
           return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
        }
        addKeyVal (&tmpCollInp.condInput, IRODS_ADMIN_RMTRASH_KW, "");
        addKeyVal (&dataObjInp.condInput, IRODS_ADMIN_RMTRASH_KW, "");
	rmtrashFlag = 2;
    } else if (getValByKey (&rmCollInp->condInput, IRODS_RMTRASH_KW) != NULL) {
        if (isTrashPath (rmCollInp->collName) == False) {
            return (SYS_INVALID_FILE_PATH);
        }
        addKeyVal (&tmpCollInp.condInput, IRODS_RMTRASH_KW, "");
        addKeyVal (&dataObjInp.condInput, IRODS_RMTRASH_KW, "");
	rmtrashFlag = 1;
    }

    while ((status = rsReadCollection (rsComm, &handleInx, &collEnt)) >= 0) {
	if (entCnt == 0) {
	    entCnt ++;
	    /* cannot rm non-empty home collection */
	    if (isHomeColl (rmCollInp->collName)) 
		return (CANT_RM_NON_EMPTY_HOME_COLL);
	} 
        if (collEnt->objType == DATA_OBJ_T) {
            snprintf (dataObjInp.objPath, MAX_NAME_LEN, "%s/%s",
              collEnt->collName, collEnt->dataName);

            status = rsDataObjUnlink (rsComm, &dataObjInp);
            if (status < 0) {
                rodsLog (LOG_ERROR,
                  "_rsPhyRmColl:rsDataObjUnlink failed for %s. stat = %d",
                  dataObjInp.objPath, status);
                /* need to set global error here */
                savedStatus = status;
            } else if (collOprStat != NULL) {
                (*collOprStat)->filesCnt ++;
                if ((*collOprStat)->filesCnt >= fileCntPerStatOut) {
                    rstrcpy ((*collOprStat)->lastObjPath, dataObjInp.objPath,
                      MAX_NAME_LEN);
                    status = svrSendCollOprStat (rsComm, *collOprStat);
                    if (status < 0) {
                        rodsLogError (LOG_ERROR, status,
                          "_rsPhyRmColl: svrSendCollOprStat failed for %s. status = %d",
                          rmCollInp->collName, status);
                        *collOprStat = NULL;
                        savedStatus = status;
                        break;
                    }
                     *collOprStat = (collOprStat_t*)malloc (sizeof (collOprStat_t));
                     memset (*collOprStat, 0, sizeof (collOprStat_t));
		}
            }
	} else if (collEnt->objType == COLL_OBJ_T) {
	    if (strcmp (collEnt->collName, rmCollInp->collName) == 0)
		continue;	/* duplicate */
            rstrcpy (tmpCollInp.collName, collEnt->collName, MAX_NAME_LEN);
	    if (collEnt->specColl.collClass != NO_SPEC_COLL) {
		if (strcmp (collEnt->collName, collEnt->specColl.collection)
		  == 0) continue;	/* no mount point */
	    }
	    status = _rsRmCollRecur (rsComm, &tmpCollInp, collOprStat);
	}
	if (status < 0) savedStatus = status;
	free (collEnt);     /* just free collEnt but not content */
    }
    rsCloseCollection (rsComm, &handleInx);

    if (rmtrashFlag > 0 && (isTrashHome (rmCollInp->collName) > 0 || 
      isOrphanPath (rmCollInp->collName) == is_ORPHAN_HOME)) {
        /* don't rm user's home trash coll or orphan collection */
        status = 0;
    } else {
        if (dataObjInfo != NULL && dataObjInfo->specColl != NULL) {
            if (dataObjInfo->specColl->collClass == LINKED_COLL) {
                rstrcpy (rmCollInp->collName, dataObjInfo->objPath,
                  MAX_NAME_LEN);
                status = svrUnregColl (rsComm, rmCollInp);
            } else {
                status = l3Rmdir (rsComm, dataObjInfo);
	    }
        } else {
            status = svrUnregColl (rsComm, rmCollInp);
	    if (status < 0) savedStatus = status;
	}
    }
    clearKeyVal (&tmpCollInp.condInput);
    clearKeyVal (&dataObjInp.condInput);

    return (savedStatus);
}

int 
svrUnregColl (rsComm_t *rsComm, collInp_t *rmCollInp)
{
    int status;
#ifdef RODS_CAT
    collInfo_t collInfo;
#endif
    rodsServerHost_t *rodsServerHost = NULL;

    status = getAndConnRcatHost (rsComm, MASTER_RCAT, rmCollInp->collName,
                                &rodsServerHost);
    if (status < 0) {
       return(status);
    }

    if (rodsServerHost->localFlag == LOCAL_HOST) {
#ifdef RODS_CAT
        memset (&collInfo, 0, sizeof (collInfo));
        rstrcpy (collInfo.collName, rmCollInp->collName, MAX_NAME_LEN);
        if (getValByKey (&rmCollInp->condInput, IRODS_ADMIN_RMTRASH_KW)
          != NULL) {
            status = chlDelCollByAdmin (rsComm, &collInfo);
            if (status >= 0) {
                chlCommit(rsComm);
            }
        } else {
            status = chlDelColl (rsComm, &collInfo);
        }

#else
        status = SYS_NO_RCAT_SERVER_ERR;
#endif
    } else {
        collOprStat_t *collOprStat = NULL;;
	addKeyVal (&rmCollInp->condInput, UNREG_COLL_KW, "");
        status = _rcRmColl (rodsServerHost->conn, rmCollInp, &collOprStat);
	if (collOprStat != NULL) free (collOprStat);
    }

    return status;
}

#ifdef COMPAT_201
int
rsRmColl201 (rsComm_t *rsComm, collInp201_t *rmCollInp,
collOprStat_t **collOprStat)
{
    collInp_t collInp;
    int status; 

    collInp201ToCollInp (rmCollInp, &collInp);

    status = rsRmColl (rsComm, &collInp, collOprStat);

    return status;
}
#endif
    
