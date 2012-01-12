/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See dataObjUnlink.h for a description of this API call.*/

#include "dataObjUnlink.h"
#include "rodsLog.h"
#include "icatDefines.h"
#include "fileUnlink.h"
#include "unregDataObj.h"
#include "objMetaOpr.h"
#include "dataObjOpr.h"
#include "specColl.h"
#include "resource.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"
#include "reGlobalsExtern.h"
#include "reDefines.h"
#include "rmCollOld.h"
#include "rmColl.h"
#include "dataObjRename.h"
#include "subStructFileUnlink.h"
#include "modDataObjMeta.h"
#include "phyBundleColl.h"
#include "dataObjRepl.h"
#include "regDataObj.h"
#include "physPath.h"

int
rsDataObjUnlink (rsComm_t *rsComm, dataObjInp_t *dataObjUnlinkInp)
{
    int status;
    ruleExecInfo_t rei;
    int trashPolicy;
    dataObjInfo_t *dataObjInfoHead = NULL;
    rodsServerHost_t *rodsServerHost = NULL;
    int rmTrashFlag = 0;
    specCollCache_t *specCollCache = NULL;

    resolveLinkedPath (rsComm, dataObjUnlinkInp->objPath, &specCollCache,
      &dataObjUnlinkInp->condInput);
    status = getAndConnRcatHost (rsComm, MASTER_RCAT,
     dataObjUnlinkInp->objPath, &rodsServerHost);

    if (status < 0) {
        return (status);
    } else if (rodsServerHost->rcatEnabled == REMOTE_ICAT) {
        int retval;
        retval = rcDataObjUnlink (rodsServerHost->conn, dataObjUnlinkInp);
        return status;
    }

    if (getValByKey (
      &dataObjUnlinkInp->condInput, IRODS_ADMIN_RMTRASH_KW) != NULL ||
      getValByKey (
      &dataObjUnlinkInp->condInput, IRODS_RMTRASH_KW) != NULL) {
        if (isTrashPath (dataObjUnlinkInp->objPath) == False) {
            return (SYS_INVALID_FILE_PATH);
        }
	rmTrashFlag = 1;
    }

    dataObjUnlinkInp->openFlags = O_WRONLY;  /* set the permission checking */
    status = getDataObjInfoIncSpecColl (rsComm, dataObjUnlinkInp, 
      &dataObjInfoHead);

    if (status < 0) return (status);

    if (dataObjUnlinkInp->oprType == UNREG_OPR ||
      getValByKey (&dataObjUnlinkInp->condInput, FORCE_FLAG_KW) != NULL ||
      getValByKey (&dataObjUnlinkInp->condInput, REPL_NUM_KW) != NULL ||
      dataObjInfoHead->specColl != NULL || rmTrashFlag == 1) {
        status = _rsDataObjUnlink (rsComm, dataObjUnlinkInp, &dataObjInfoHead);
    } else {
        initReiWithDataObjInp (&rei, rsComm, dataObjUnlinkInp);
        status = applyRule ("acTrashPolicy", NULL, &rei, NO_SAVE_REI);
        trashPolicy = rei.status;

        if (trashPolicy != NO_TRASH_CAN) {
            status = rsMvDataObjToTrash (rsComm, dataObjUnlinkInp, 
	      &dataObjInfoHead);
    	    freeAllDataObjInfo (dataObjInfoHead);
            return status;
        } else {
            status = _rsDataObjUnlink (rsComm, dataObjUnlinkInp, 
	      &dataObjInfoHead);
        }
    }

    initReiWithDataObjInp (&rei, rsComm, dataObjUnlinkInp);
    rei.doi = dataObjInfoHead;
    rei.status = status;
    rei.status = applyRule ("acPostProcForDelete", NULL, &rei, NO_SAVE_REI);

    if (rei.status < 0) {
        rodsLog (LOG_NOTICE,
          "rsDataObjUnlink: acPostProcForDelete error for %s. status = %d",
          dataObjUnlinkInp->objPath, rei.status);
    }

    /* dataObjInfoHead may be outdated */
    freeAllDataObjInfo (dataObjInfoHead);

    return (status);
}

int
_rsDataObjUnlink (rsComm_t *rsComm, dataObjInp_t *dataObjUnlinkInp,
dataObjInfo_t **dataObjInfoHead)
{
    int status;
    int retVal = 0;
    dataObjInfo_t *tmpDataObjInfo, *myDataObjInfoHead;

    status = chkPreProcDeleteRule (rsComm, dataObjUnlinkInp, *dataObjInfoHead);
    if (status < 0) return status;

#if 0	/* done in chkPreProcDeleteRule */
    ruleExecInfo_t rei;
    initReiWithDataObjInp (&rei, rsComm, dataObjUnlinkInp);
    rei.doi = *dataObjInfoHead;

    status = applyRule ("acDataDeletePolicy", NULL, &rei, NO_SAVE_REI);

    if (status < 0 && status != NO_MORE_RULES_ERR &&
      status != SYS_DELETE_DISALLOWED) {
        rodsLog (LOG_NOTICE,
          "_rsDataObjUnlink: acDataDeletePolicy error for %s. status = %d",
          dataObjUnlinkInp->objPath, status);
        return (status);
    }

    if (rei.status == SYS_DELETE_DISALLOWED) {
        rodsLog (LOG_NOTICE,
        "_rsDataObjUnlink:disallowed for %s via acDataDeletePolicy,stat=%d",
          dataObjUnlinkInp->objPath, rei.status);
        return (rei.status);
    }
#endif

    myDataObjInfoHead = *dataObjInfoHead;
    if (strcmp (myDataObjInfoHead->dataType, TAR_BUNDLE_TYPE) == 0) {
        if (rsComm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
            return CAT_INSUFFICIENT_PRIVILEGE_LEVEL;
	}
	if (getValByKey (&dataObjUnlinkInp->condInput, REPL_NUM_KW) != NULL) {
	    return SYS_CANT_MV_BUNDLE_DATA_BY_COPY;
	}
	status = _unbunAndStageBunfileObj (rsComm, dataObjInfoHead, NULL, 1);
	if (status < 0) {
	    /* go ahead and unlink the obj if the phy file does not exist or
	     * have problem untaring it */
	    if (getErrno (status) != EEXIST && 
	      getIrodsErrno (status) != SYS_TAR_STRUCT_FILE_EXTRACT_ERR) {
                rodsLog (LOG_NOTICE,
                "_rsDataObjUnlink:_unbunAndStageBunfileObj err for %s,stat=%d",
                  myDataObjInfoHead->objPath, status);
                return (status);
	    }
        }
	/* dataObjInfoHead may be outdated */
	*dataObjInfoHead = NULL;
        status = getDataObjInfoIncSpecColl (rsComm, dataObjUnlinkInp,
          dataObjInfoHead);

        if (status < 0) return (status);
    }

    tmpDataObjInfo = *dataObjInfoHead;
    while (tmpDataObjInfo != NULL) {
	status = dataObjUnlinkS (rsComm, dataObjUnlinkInp, tmpDataObjInfo);
	if (status < 0) {
	    if (retVal == 0) {
	        retVal = status;
	    }
	}
        if (dataObjUnlinkInp->specColl != NULL) 	/* do only one */
	    break;
	tmpDataObjInfo = tmpDataObjInfo->next;
    }

    if ((*dataObjInfoHead)->specColl == NULL)
        resolveDataObjReplStatus (rsComm, dataObjUnlinkInp);

    return (retVal);
}

/* resolveDataObjReplStatus - a dirty copy may be deleted leaving no
 * dirty copy. In that case, pick the newest copy and mark it dirty
 */
int
resolveDataObjReplStatus (rsComm_t *rsComm, dataObjInp_t *dataObjUnlinkInp)
{
    int status;
    dataObjInfo_t *dataObjInfoHead = NULL;
    dataObjInfo_t *newestDataObjInfo = NULL;
    dataObjInfo_t *tmpDataObjInfo;

    if (getValByKey (&dataObjUnlinkInp->condInput, RESC_NAME_KW) == NULL &&
      getValByKey (&dataObjUnlinkInp->condInput, REPL_NUM_KW) == NULL) {
	return 0;
    } 
    status = getDataObjInfo (rsComm, dataObjUnlinkInp,
      &dataObjInfoHead, ACCESS_DELETE_OBJECT, 1);

    if (status < 0) return status;

    tmpDataObjInfo = dataObjInfoHead;
    while (tmpDataObjInfo != NULL) {
	if (tmpDataObjInfo->replStatus == 0) {
	    if (newestDataObjInfo == NULL) {
		newestDataObjInfo = tmpDataObjInfo;
	    } else if (atoi (tmpDataObjInfo->dataModify) >
	      atoi (newestDataObjInfo->dataModify)) {
		newestDataObjInfo = tmpDataObjInfo;
	    }
	} else {
	    newestDataObjInfo = NULL;
	    break;
	}
        tmpDataObjInfo = tmpDataObjInfo->next;
    }

    /* modify the repl status */
    if (newestDataObjInfo != NULL) {
        keyValPair_t regParam;
        char tmpStr[MAX_NAME_LEN];
        modDataObjMeta_t modDataObjMetaInp;

	memset (&regParam, 0, sizeof (regParam));
	memset (&modDataObjMetaInp, 0, sizeof (modDataObjMetaInp));
        snprintf (tmpStr, MAX_NAME_LEN, "%d", NEWLY_CREATED_COPY);
        addKeyVal (&regParam, REPL_STATUS_KW, tmpStr);
        modDataObjMetaInp.dataObjInfo = newestDataObjInfo;
        modDataObjMetaInp.regParam = &regParam;

        status = rsModDataObjMeta (rsComm, &modDataObjMetaInp);

        clearKeyVal (&regParam);
    }
    freeAllDataObjInfo (dataObjInfoHead);
    return (status);
}

int
dataObjUnlinkS (rsComm_t *rsComm, dataObjInp_t *dataObjUnlinkInp,
dataObjInfo_t *dataObjInfo)
{
    int status;
    unregDataObj_t unregDataObjInp;

    if (dataObjInfo->specColl == NULL) {
	if (dataObjUnlinkInp->oprType == UNREG_OPR && 
	  rsComm->clientUser.authInfo.authFlag != LOCAL_PRIV_USER_AUTH) {
	    ruleExecInfo_t rei;

            initReiWithDataObjInp (&rei, rsComm, dataObjUnlinkInp);
            rei.doi = dataObjInfo;
            rei.status = CHK_PERM_FLAG;         /* default */
            applyRule ("acNoChkFilePathPerm", NULL, &rei, NO_SAVE_REI);
            if (rei.status == CHK_PERM_FLAG) {
                char *outVaultPath;
                rodsServerHost_t *rodsServerHost;
	        status = resolveHostByRescInfo (dataObjInfo->rescInfo, 
	          &rodsServerHost);
	        if (status < 0) return status;
	        /* unregistering but not an admin user */
	        status = matchVaultPath (rsComm, dataObjInfo->filePath, 
	          rodsServerHost, &outVaultPath);
	        if (status != 0) {
		    /* in the vault */
                    rodsLog (LOG_DEBUG,
                      "dataObjUnlinkS: unregistering in vault file %s",
                      dataObjInfo->filePath);
                    return CANT_UNREG_IN_VAULT_FILE;
		}
	    }
#if 0	/* don't need this since we are doing orphan */
	} else if (RescTypeDef[dataObjInfo->rescInfo->rescTypeInx].driverType 
	  == WOS_FILE_TYPE && dataObjUnlinkInp->oprType != UNREG_OPR) {
	    /* WOS_FILE_TYPE, unlink first before unreg because orphan files
	     * cannot be reclaimed */
            status = l3Unlink (rsComm, dataObjInfo);
            if (status < 0) {
                rodsLog (LOG_NOTICE,
                  "dataObjUnlinkS: l3Unlink error for WOS file %s. status = %d",
                  dataObjUnlinkInp->objPath, status);
		return status;
	    }
            unregDataObjInp.dataObjInfo = dataObjInfo;
            unregDataObjInp.condInput = &dataObjUnlinkInp->condInput;
            status = rsUnregDataObj (rsComm, &unregDataObjInp);
            if (status < 0) {
                rodsLog (LOG_NOTICE,
                  "dataObjUnlinkS: rsUnregDataObj error for %s. status = %d",
                  dataObjUnlinkInp->objPath, status);
            }
            return status;
#endif
	}
        unregDataObjInp.dataObjInfo = dataObjInfo;
        unregDataObjInp.condInput = &dataObjUnlinkInp->condInput;
        status = rsUnregDataObj (rsComm, &unregDataObjInp);

        if (status < 0) {
            rodsLog (LOG_NOTICE,
              "dataObjUnlinkS: rsUnregDataObj error for %s. status = %d",
              dataObjUnlinkInp->objPath, status);
	    return status;
        }
    }
    
    if (dataObjUnlinkInp->oprType != UNREG_OPR) {
        status = l3Unlink (rsComm, dataObjInfo);
        if (status < 0) {
	    int myError = getErrno (status);
            rodsLog (LOG_NOTICE,
              "dataObjUnlinkS: l3Unlink error for %s. status = %d",
              dataObjUnlinkInp->objPath, status);
	    /* allow ENOENT to go on and unregister */
	    if (myError != ENOENT && myError != EACCES) {
		char orphanPath[MAX_NAME_LEN];
		int status1 = 0;
                rodsLog (LOG_NOTICE,
                  "dataObjUnlinkS: orphan file %s", dataObjInfo->filePath);
		while (1) { 
		    if (isOrphanPath (dataObjUnlinkInp->objPath) == 
		      NOT_ORPHAN_PATH) {
			/* don't rename orphan path */
		        status1 = rsMkOrhpanPath (rsComm, dataObjInfo->objPath,
		          orphanPath);
		        if (status1 < 0) break;
		        /* reg the orphan path */
		        rstrcpy (dataObjInfo->objPath, orphanPath,MAX_NAME_LEN);
		    }
		    status1 = svrRegDataObj (rsComm, dataObjInfo);
		    if (status1 == CAT_NAME_EXISTS_AS_DATAOBJ ||
		      status1 == CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME) {
			continue;
		    } else if (status1 < 0) {
			rodsLogError (LOG_ERROR, status1,
			  "dataObjUnlinkS: svrRegDataObj of orphan %s error",
			  dataObjInfo->objPath);
		    }
		    break;
		}
	        return (status);
	    } else {
	        status = 0;
	    }
	}
    }

    return (status);
}

int
l3Unlink (rsComm_t *rsComm, dataObjInfo_t *dataObjInfo)
{
    int rescTypeInx;
    fileUnlinkInp_t fileUnlinkInp;
    int status;

    if (getRescClass (dataObjInfo->rescInfo) == BUNDLE_CL) return 0;

     if (dataObjInfo->rescInfo->rescStatus == INT_RESC_STATUS_DOWN) 
	return SYS_RESC_IS_DOWN;

    if (getStructFileType (dataObjInfo->specColl) >= 0) {
        subFile_t subFile;
        memset (&subFile, 0, sizeof (subFile));
        rstrcpy (subFile.subFilePath, dataObjInfo->subPath,
          MAX_NAME_LEN);
        rstrcpy (subFile.addr.hostAddr, dataObjInfo->rescInfo->rescLoc,
          NAME_LEN);
        subFile.specColl = dataObjInfo->specColl;
        status = rsSubStructFileUnlink (rsComm, &subFile);
    } else {
        rescTypeInx = dataObjInfo->rescInfo->rescTypeInx;


        switch (RescTypeDef[rescTypeInx].rescCat) {
          case FILE_CAT:
            memset (&fileUnlinkInp, 0, sizeof (fileUnlinkInp));
            fileUnlinkInp.fileType = (fileDriverType_t)RescTypeDef[rescTypeInx].driverType;
            rstrcpy (fileUnlinkInp.fileName, dataObjInfo->filePath, 
	      MAX_NAME_LEN);
            rstrcpy (fileUnlinkInp.addr.hostAddr, 
	      dataObjInfo->rescInfo->rescLoc, NAME_LEN);
            status = rsFileUnlink (rsComm, &fileUnlinkInp);
            break;

          default:
            rodsLog (LOG_NOTICE,
              "l3Unlink: rescCat type %d is not recognized",
              RescTypeDef[rescTypeInx].rescCat);
            status = SYS_INVALID_RESC_TYPE;
            break;
	}
    }
    return (status);
}

int
rsMvDataObjToTrash (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
dataObjInfo_t **dataObjInfoHead)
{
    int status;
    char trashPath[MAX_NAME_LEN];
    dataObjCopyInp_t dataObjRenameInp;

    if (strcmp ((*dataObjInfoHead)->dataType, TAR_BUNDLE_TYPE) == 0) {
	return SYS_CANT_MV_BUNDLE_DATA_TO_TRASH;
    }

    if (getValByKey (&dataObjInp->condInput, DATA_ACCESS_KW) == NULL) {
        addKeyVal (&dataObjInp->condInput, DATA_ACCESS_KW,
          ACCESS_DELETE_OBJECT);
    }

    status = getDataObjInfo (rsComm, dataObjInp, dataObjInfoHead,
      ACCESS_DELETE_OBJECT, 0);

    if (status < 0) {
        rodsLog (LOG_NOTICE,
          "rsMvDataObjToTrash: getDataObjInfo error for %s. status = %d",
          dataObjInp->objPath, status);
        return (status);
    }

    status = chkPreProcDeleteRule (rsComm, dataObjInp, *dataObjInfoHead);
    if (status < 0) return status;

#if 0   /* done in chkPreProcDeleteRule */

    initReiWithDataObjInp (&rei, rsComm, dataObjInp);
    rei.doi = *dataObjInfoHead;

    status = applyRule ("acDataDeletePolicy", NULL, &rei, NO_SAVE_REI);

    if (status < 0 && status != NO_MORE_RULES_ERR &&
      status != SYS_DELETE_DISALLOWED) {
        rodsLog (LOG_NOTICE,
          "rsMvDataObjToTrash: acDataDeletePolicy error for %s. status = %d",
          dataObjInp->objPath, status);
        return (status);
    }

    if (rei.status == SYS_DELETE_DISALLOWED) {
        rodsLog (LOG_NOTICE,
        "rsMvDataObjToTrash:disallowed for %s via DataDeletePolicy,status=%d",
          dataObjInp->objPath, rei.status);
        return (rei.status);
    }
#endif

    status = rsMkTrashPath (rsComm, dataObjInp->objPath, trashPath);

    if (status < 0) {
        return (status);
    }

    memset (&dataObjRenameInp, 0, sizeof (dataObjRenameInp));

    dataObjRenameInp.srcDataObjInp.oprType =
      dataObjRenameInp.destDataObjInp.oprType = RENAME_DATA_OBJ;

    rstrcpy (dataObjRenameInp.destDataObjInp.objPath, trashPath, MAX_NAME_LEN);
    rstrcpy (dataObjRenameInp.srcDataObjInp.objPath, dataObjInp->objPath,
      MAX_NAME_LEN);

    status = rsDataObjRename (rsComm, &dataObjRenameInp);

    while (status == CAT_NAME_EXISTS_AS_DATAOBJ || 
      status == CAT_NAME_EXISTS_AS_COLLECTION || 
      status == SYS_PHY_PATH_INUSE || 
      getErrno (status) == EISDIR) {
        appendRandomToPath (dataObjRenameInp.destDataObjInp.objPath);
	status = rsDataObjRename (rsComm, &dataObjRenameInp);
#if 0
        if (status1 < 0) {
            rodsLog (LOG_ERROR,
              "rsMvDataObjToTrash: rsDataObjRename error for %s",
              dataObjRenameInp.destDataObjInp.objPath);
	} else {
	    status = 0;
	}
#endif
    }
    if (status < 0) {
        rodsLog (LOG_ERROR,
          "rsMvDataObjToTrash: rcDataObjRename error for %s, status = %d",
          dataObjRenameInp.destDataObjInp.objPath, status);
        return (status);
    }
    return (status);
}

int
chkPreProcDeleteRule (rsComm_t *rsComm, dataObjInp_t *dataObjUnlinkInp,
dataObjInfo_t *dataObjInfoHead)
{
    dataObjInfo_t *tmpDataObjInfo;
    ruleExecInfo_t rei;
    int status = 0;

    initReiWithDataObjInp (&rei, rsComm, dataObjUnlinkInp);
    tmpDataObjInfo = dataObjInfoHead;
    while (tmpDataObjInfo != NULL) {
        /* have to go through the loop to test each copy (resource). */
        rei.doi = tmpDataObjInfo;

        status = applyRule ("acDataDeletePolicy", NULL, &rei, NO_SAVE_REI);

        if (status < 0 && status != NO_MORE_RULES_ERR &&
          status != SYS_DELETE_DISALLOWED) {
            rodsLog (LOG_ERROR,
              "chkPreProcDeleteRule: acDataDeletePolicy err for %s. stat = %d",
              dataObjUnlinkInp->objPath, status);
            return (status);
        }

        if (rei.status == SYS_DELETE_DISALLOWED) {
            rodsLog (LOG_ERROR,
            "chkPreProcDeleteRule:acDataDeletePolicy disallowed delete of %s",
              dataObjUnlinkInp->objPath);
            return (rei.status);
        }
        tmpDataObjInfo = tmpDataObjInfo->next;
    }
    return status;
}
