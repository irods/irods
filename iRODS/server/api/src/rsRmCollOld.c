/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* This is script-generated code (for the most part).  */
/* See collCreate.h for a description of this API call.*/

#include "rmCollOld.h"
#include "collCreate.h"
#include "icatHighLevelRoutines.h"
#include "rodsLog.h"
#include "icatDefines.h"
#include "dataObjRename.h"
#include "dataObjOpr.h"
#include "objMetaOpr.h"
#include "collection.h"
#include "specColl.h"
#include "fileRmdir.h"
#include "subStructFileRmdir.h"
#include "genQuery.h"
#include "dataObjUnlink.h"

int
rsRmCollOld (rsComm_t *rsComm, collInp_t *rmCollInp)
{
    int status;
    rodsServerHost_t *rodsServerHost = NULL;


    status = getAndConnRcatHost (rsComm, MASTER_RCAT, rmCollInp->collName,
                                &rodsServerHost);
    if (status < 0) {
       return(status);
    }

    if (rodsServerHost->localFlag == LOCAL_HOST) {
#ifdef RODS_CAT
        status = _rsRmCollOld (rsComm, rmCollInp);
#else
        status = SYS_NO_RCAT_SERVER_ERR;
#endif
    } else {
	status = rcRmCollOld (rodsServerHost->conn, rmCollInp);
    }

    return (status);
}

int
_rsRmCollOld (rsComm_t *rsComm, collInp_t *rmCollInp)
{
    int status;
    dataObjInfo_t *dataObjInfo = NULL;

#if 0
    dataObjInp_t dataObjInp;
    memset (&dataObjInp, 0, sizeof (dataObjInp));
    rstrcpy (dataObjInp.objPath, rmCollInp->collName, MAX_NAME_LEN);
#endif
    status = resolvePathInSpecColl (rsComm, rmCollInp->collName, 
      WRITE_COLL_PERM, 0, &dataObjInfo);


    if (status == COLL_OBJ_T && dataObjInfo->specColl != NULL) {
	status = svrRmSpecColl (rsComm, rmCollInp, dataObjInfo);
    } else { 
	status = svrRmCollOld (rsComm, rmCollInp);
    }

    return (0);
}

int
svrRmCollOld (rsComm_t *rsComm, collInp_t *rmCollInp)
{
    int status;
    collInfo_t collInfo;

    if (getValByKey (&rmCollInp->condInput, RECURSIVE_OPR__KW) != NULL) {
	status = _rsRmCollRecurOld (rsComm, rmCollInp);
	return (status);
    }

    memset (&collInfo, 0, sizeof (collInfo));

    rstrcpy (collInfo.collName, rmCollInp->collName, MAX_NAME_LEN);
#ifdef RODS_CAT
    if (getValByKey (&rmCollInp->condInput, IRODS_ADMIN_RMTRASH_KW) != NULL) {
        status = chlDelCollByAdmin (rsComm, &collInfo);
        if (status >= 0) {
            chlCommit(rsComm);
        }
    } else {
        status = chlDelColl (rsComm, &collInfo);
    }
    return (status);
#else
    return (SYS_NO_RCAT_SERVER_ERR);
#endif
}

int
_rsRmCollRecurOld (rsComm_t *rsComm, collInp_t *rmCollInp)
{
    int status;
    ruleExecInfo_t rei;
    int trashPolicy;

    if (getValByKey (&rmCollInp->condInput, FORCE_FLAG_KW) != NULL) { 
        status = rsPhyRmCollRecurOld (rsComm, rmCollInp);
    } else {
        initReiWithDataObjInp (&rei, rsComm, NULL);
        status = applyRule ("acTrashPolicy", NULL, &rei, NO_SAVE_REI);
        trashPolicy = rei.status;

        if (trashPolicy != NO_TRASH_CAN) { 
            status = rsMvCollToTrash (rsComm, rmCollInp);
	    return status;
        } else {
	    status = rsPhyRmCollRecurOld (rsComm, rmCollInp);
	}
    }

    return (status);
}

int
rsPhyRmCollRecurOld (rsComm_t *rsComm, collInp_t *rmCollInp)
{
    int status, i;
    int savedStatus = 0;
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    dataObjInp_t dataObjInp;
    int collLen;
    int continueInx;
    collInfo_t collInfo;
    collInp_t tmpCollInp;
    int rmtrashFlag;

    memset (&dataObjInp, 0, sizeof (dataObjInp));
    memset (&tmpCollInp, 0, sizeof (tmpCollInp));
    addKeyVal (&tmpCollInp.condInput, FORCE_FLAG_KW, "");
    addKeyVal (&dataObjInp.condInput, FORCE_FLAG_KW, "");

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

    collLen = strlen (rmCollInp->collName);

    /* Now get all the files */

    memset (&genQueryInp, 0, sizeof (genQueryInp));

    status = rsQueryDataObjInCollReCur (rsComm, rmCollInp->collName, 
      &genQueryInp, &genQueryOut, NULL, 1);

    if (status < 0 && status != CAT_NO_ROWS_FOUND) {
	rodsLog (LOG_ERROR,
	  "rsPhyRmCollRecurOld: rsQueryDataObjInCollReCur error for %s, stat=%d",
	  rmCollInp->collName, status);
	return (status);
    }

    while (status >= 0) {
        sqlResult_t *subColl, *dataObj;

        if ((subColl = getSqlResultByInx (genQueryOut, COL_COLL_NAME))
          == NULL) {
            rodsLog (LOG_ERROR,
              "rsPhyRmCollRecurOld: getSqlResultByInx for COL_COLL_NAME failed");
            return (UNMATCHED_KEY_OR_INDEX);
        }

        if ((dataObj = getSqlResultByInx (genQueryOut, COL_DATA_NAME))
          == NULL) {
            rodsLog (LOG_ERROR,
              "rsPhyRmCollRecurOld: getSqlResultByInx for COL_DATA_NAME failed");
            return (UNMATCHED_KEY_OR_INDEX);
        }

        for (i = 0; i < genQueryOut->rowCnt; i++) {
            char *tmpSubColl, *tmpDataName;

            tmpSubColl = &subColl->value[subColl->len * i];
            tmpDataName = &dataObj->value[dataObj->len * i];

            snprintf (dataObjInp.objPath, MAX_NAME_LEN, "%s/%s",
              tmpSubColl, tmpDataName);
            status = rsDataObjUnlink (rsComm, &dataObjInp);
            if (status < 0) {
                rodsLog (LOG_ERROR,
                  "rsPhyRmCollRecurOld:rsDataObjUnlink failed for %s. stat = %d", 
		  dataObjInp.objPath, status);
                /* need to set global error here */
                savedStatus = status;
            }
        }

        continueInx = genQueryOut->continueInx;

        freeGenQueryOut (&genQueryOut);

        if (continueInx > 0) {
            /* More to come */
            genQueryInp.continueInx = continueInx;
            status =  rsGenQuery (rsComm, &genQueryInp, &genQueryOut);
        } else {
            break;
        }
    }

    clearKeyVal (&dataObjInp.condInput);

    /* query all sub collections in rmCollInp->collName and the mk the required
     * subdirectories */

    status = rsQueryCollInColl (rsComm, rmCollInp->collName, &genQueryInp,
      &genQueryOut);

    while (status >= 0) {
        sqlResult_t *subColl;

        if ((subColl = getSqlResultByInx (genQueryOut, COL_COLL_NAME))
          == NULL) {
            rodsLog (LOG_ERROR,
              "rsPhyRmCollRecurOld: getSqlResultByInx for COL_COLL_NAME failed");
            return (UNMATCHED_KEY_OR_INDEX);
        }

        for (i = 0; i < genQueryOut->rowCnt; i++) {
            char *tmpSubColl;

            tmpSubColl = &subColl->value[subColl->len * i];
            if ((int) strlen (tmpSubColl) < collLen)
                continue;
            /* recursively rm the collection */
	    rstrcpy (tmpCollInp.collName, tmpSubColl, MAX_NAME_LEN);
            status = rsPhyRmCollRecurOld (rsComm, &tmpCollInp);
            if (status < 0) {
                rodsLogError (LOG_ERROR, status,
                  "rsPhyRmCollRecurOld: rsPhyRmCollRecurOld of %s failed, status = %d",
                  tmpSubColl, status);
                savedStatus = status;
            }
        }
        continueInx = genQueryOut->continueInx;

        freeGenQueryOut (&genQueryOut);

        if (continueInx > 0) {
            /* More to come */
            genQueryInp.continueInx = continueInx;
            status =  rsGenQuery (rsComm, &genQueryInp, &genQueryOut);
        } else {
            break;
        }
    }
    clearKeyVal (&tmpCollInp.condInput);


    memset (&collInfo, 0, sizeof (collInfo));

    rstrcpy (collInfo.collName, rmCollInp->collName, MAX_NAME_LEN);
#ifdef RODS_CAT
    if (rmtrashFlag > 0 && isTrashHome (rmCollInp->collName) > 0) {
	/* don't rm user's home trash coll */
	status = 0;
    } else {
        memset (&collInfo, 0, sizeof (collInfo));
        rstrcpy (collInfo.collName, rmCollInp->collName, MAX_NAME_LEN);
	if (rmtrashFlag == 2) {
	    status = chlDelCollByAdmin (rsComm, &collInfo);
	    if (status >= 0) {
	        chlCommit(rsComm);
	    }
	} else {
            status = chlDelColl (rsComm, &collInfo);
	}
    }

    if (status < 0) {
       rodsLog (LOG_ERROR,
        "rsPhyRmCollRecurOld: chlDelColl of %s failed, status = %d",
        rmCollInp->collName, status);
    }

    clearGenQueryInp (&genQueryInp);

    if (savedStatus < 0) {
        return (savedStatus);
    } else if (status == CAT_NO_ROWS_FOUND) {
        return (0);
    } else {
        return (status);
    }
#else
    return (SYS_NO_RCAT_SERVER_ERR);
#endif
}

int
rsMvCollToTrash (rsComm_t *rsComm, collInp_t *rmCollInp)
{
    int status;
    char trashPath[MAX_NAME_LEN];
    dataObjCopyInp_t dataObjRenameInp;
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    int continueInx;
    dataObjInfo_t dataObjInfo;

    /* check permission of files */

    memset (&genQueryInp, 0, sizeof (genQueryInp));
    status = rsQueryDataObjInCollReCur (rsComm, rmCollInp->collName,
      &genQueryInp, &genQueryOut, ACCESS_DELETE_OBJECT, 0);

    memset (&dataObjInfo, 0, sizeof (dataObjInfo));
    while (status >= 0) {
	sqlResult_t *subColl, *dataObj, *rescName;
	ruleExecInfo_t rei;

	/* check if allow to delete */

        if ((subColl = getSqlResultByInx (genQueryOut, COL_COLL_NAME))
          == NULL) {
            rodsLog (LOG_ERROR,
              "rsMvCollToTrash: getSqlResultByInx for COL_COLL_NAME failed");
            return (UNMATCHED_KEY_OR_INDEX);
        }

        if ((dataObj = getSqlResultByInx (genQueryOut, COL_DATA_NAME))
          == NULL) {
            rodsLog (LOG_ERROR,
              "rsMvCollToTrash: getSqlResultByInx for COL_DATA_NAME failed");
            return (UNMATCHED_KEY_OR_INDEX);
        }

        if ((rescName = getSqlResultByInx (genQueryOut, COL_D_RESC_NAME))
          == NULL) {
            rodsLog (LOG_ERROR,
              "rsMvCollToTrash: getSqlResultByInx for COL_D_RESC_NAME failed");
            return (UNMATCHED_KEY_OR_INDEX);
        }

	snprintf (dataObjInfo.objPath, MAX_NAME_LEN, "%s/%s", 
	  subColl->value, dataObj->value);
	rstrcpy (dataObjInfo.rescName, rescName->value, NAME_LEN);

        initReiWithDataObjInp (&rei, rsComm, NULL);
        rei.doi = &dataObjInfo;

        status = applyRule ("acDataDeletePolicy", NULL, &rei, NO_SAVE_REI);

        if (status < 0 && status != NO_MORE_RULES_ERR &&
          status != SYS_DELETE_DISALLOWED) {
            rodsLog (LOG_NOTICE,
              "rsMvCollToTrash: acDataDeletePolicy error for %s. status = %d",
              dataObjInfo.objPath, status);
            return (status);
        }

        if (rei.status == SYS_DELETE_DISALLOWED) {
            rodsLog (LOG_NOTICE,
            "rsMvCollToTrash:disallowed for %s via DataDeletePolicy,status=%d",
              dataObjInfo.objPath, rei.status);
            return (rei.status);
        }

        continueInx = genQueryOut->continueInx;

        freeGenQueryOut (&genQueryOut);

        if (continueInx > 0) {
            /* More to come */
            genQueryInp.continueInx = continueInx;
            status =  rsGenQuery (rsComm, &genQueryInp, &genQueryOut);
        } else {
            break;
        }
    }

    if (status < 0 && status != CAT_NO_ROWS_FOUND) {
        rodsLog (LOG_ERROR,
          "rsMvCollToTrash: rsQueryDataObjInCollReCur error for %s, stat=%d",
          rmCollInp->collName, status);
        return (status);
    }

    status = rsMkTrashPath (rsComm, rmCollInp->collName, trashPath);

    if (status < 0) {
        appendRandomToPath (trashPath);
        status = rsMkTrashPath (rsComm, rmCollInp->collName, trashPath);
        if (status < 0) {
            return (status);
        }
    }

    memset (&dataObjRenameInp, 0, sizeof (dataObjRenameInp));

    dataObjRenameInp.srcDataObjInp.oprType =
      dataObjRenameInp.destDataObjInp.oprType = RENAME_COLL;

    rstrcpy (dataObjRenameInp.destDataObjInp.objPath, trashPath, MAX_NAME_LEN);
    rstrcpy (dataObjRenameInp.srcDataObjInp.objPath, rmCollInp->collName,
      MAX_NAME_LEN);

    status = rsDataObjRename (rsComm, &dataObjRenameInp);

    while (status == CAT_NAME_EXISTS_AS_COLLECTION) {
        appendRandomToPath (dataObjRenameInp.destDataObjInp.objPath);
        status = rsDataObjRename (rsComm, &dataObjRenameInp);
#if 0
        if (status < 0) {
            rodsLog (LOG_ERROR,
              "mvCollToTrash: rcDataObjRename error for %s",
              dataObjRenameInp.destDataObjInp.objPath);
            return (status);
	}
#endif
    }

    if (status < 0) {
        rodsLog (LOG_ERROR,
          "mvCollToTrash: rcDataObjRename error for %s, status = %d",
          dataObjRenameInp.destDataObjInp.objPath, status);
        return (status);
    }

    return (status);
}

int
rsMkTrashPath (rsComm_t *rsComm, char *objPath, char *trashPath)
{
    int status;
    char *tmpStr;
    char startTrashPath[MAX_NAME_LEN];
    char destTrashColl[MAX_NAME_LEN], myFile[MAX_NAME_LEN];
    char *trashPathPtr;

    trashPathPtr = trashPath;
    *trashPathPtr = '/';
    trashPathPtr++;
    tmpStr = objPath + 1;
    /* copy the zone */
    while (*tmpStr != '\0') {
	*trashPathPtr = *tmpStr;
	trashPathPtr ++;
	if (*tmpStr == '/') {
	    tmpStr ++;
	    break;
	}
	tmpStr ++;
    }

    if (*tmpStr == '\0') {
        rodsLog (LOG_ERROR,
          "rsMkTrashPath: input path %s too short", objPath);
	return (USER_INPUT_PATH_ERR);
    }
     
    /* skip "home/userName/"  or "home/userName#" */

    if (strncmp (tmpStr, "home/", 5) == 0) {
        int nameLen; 
        tmpStr += 5;
        nameLen = strlen (rsComm->clientUser.userName);
        if (strncmp (tmpStr, rsComm->clientUser.userName, nameLen) == 0 &&
	  (*(tmpStr + nameLen) == '/' || *(tmpStr + nameLen) == '#')) { 
            /* tmpStr += (nameLen + 1); */
	    tmpStr = strchr (tmpStr, '/') + 1;
        }
    }


    /* don't want to go back beyond /myZone/trash/home */
    *trashPathPtr = '\0';
    snprintf (startTrashPath, MAX_NAME_LEN, "%strash/home", trashPath);

    /* add home/userName/ */

    if (rsComm->clientUser.authInfo.authFlag == REMOTE_USER_AUTH || 
      rsComm->clientUser.authInfo.authFlag == REMOTE_PRIV_USER_AUTH) {
	/* remote user */
        snprintf (trashPathPtr, MAX_NAME_LEN, "trash/home/%s#%s/%s",  
          rsComm->clientUser.userName, rsComm->clientUser.rodsZone, tmpStr); 
    } else {
        snprintf (trashPathPtr, MAX_NAME_LEN, "trash/home/%s/%s",  
          rsComm->clientUser.userName, tmpStr); 
    }

    if ((status = splitPathByKey (trashPath, destTrashColl, myFile, '/')) < 0) {
        rodsLog (LOG_ERROR,
          "rsMkTrashPath: splitPathByKey error for %s ", trashPath);
        return (USER_INPUT_PATH_ERR);
    }

    status = rsMkCollR (rsComm, startTrashPath, destTrashColl);

    if (status < 0) {
        rodsLog (LOG_ERROR,
          "rsMkTrashPath: rsMkCollR error for startPath %s, destPath %s ",
          startTrashPath, destTrashColl);
    }

    return (status);
}

int
svrRmSpecColl (rsComm_t *rsComm, collInp_t *rmCollInp, 
dataObjInfo_t *dataObjInfo)
{
    int status;

    if (getValByKey (&rmCollInp->condInput, RECURSIVE_OPR__KW) != NULL) {
	/* XXXX need to take care of structFile */
        status = svrRmSpecCollRecur (rsComm, dataObjInfo);
    } else {
	/* XXXX need to take care of structFile */
	status = l3Rmdir (rsComm, dataObjInfo);
    }
    return (status);
}

int
l3Rmdir (rsComm_t *rsComm, dataObjInfo_t *dataObjInfo)
{
    int rescTypeInx;
    fileRmdirInp_t fileRmdirInp;
    int status;

    if (getStructFileType (dataObjInfo->specColl) >= 0) {
        subFile_t subFile;
        memset (&subFile, 0, sizeof (subFile));
        rstrcpy (subFile.subFilePath, dataObjInfo->subPath,
          MAX_NAME_LEN);
        rstrcpy (subFile.addr.hostAddr, dataObjInfo->rescInfo->rescLoc,
          NAME_LEN);
        subFile.specColl = dataObjInfo->specColl;
        status = rsSubStructFileRmdir (rsComm, &subFile);
    } else {
        rescTypeInx = dataObjInfo->rescInfo->rescTypeInx;

        switch (RescTypeDef[rescTypeInx].rescCat) {
          case FILE_CAT:
            memset (&fileRmdirInp, 0, sizeof (fileRmdirInp));
            fileRmdirInp.fileType = (fileDriverType_t)RescTypeDef[rescTypeInx].driverType;
            rstrcpy (fileRmdirInp.dirName, dataObjInfo->filePath,
              MAX_NAME_LEN);
            rstrcpy (fileRmdirInp.addr.hostAddr,
              dataObjInfo->rescInfo->rescLoc, NAME_LEN);
            status = rsFileRmdir (rsComm, &fileRmdirInp);
            break;

          default:
            rodsLog (LOG_NOTICE,
              "l3Rmdir: rescCat type %d is not recognized",
              RescTypeDef[rescTypeInx].rescCat);
            status = SYS_INVALID_RESC_TYPE;
            break;
        }
    }
    return (status);
}

int
svrRmSpecCollRecur (rsComm_t *rsComm, dataObjInfo_t *dataObjInfo)
{
    int status, i;
    dataObjInfo_t myDataObjInfo;
    genQueryOut_t *genQueryOut = NULL;
    dataObjInp_t dataObjInp;
    int continueInx;
    int savedStatus = 0;

    myDataObjInfo = *dataObjInfo;
    memset (&dataObjInp, 0, sizeof (dataObjInp));
    rstrcpy (dataObjInp.objPath, dataObjInfo->objPath, MAX_NAME_LEN);
    status = rsQuerySpecColl (rsComm, &dataObjInp, &genQueryOut);

    while (status >= 0) {
        sqlResult_t *subColl, *dataObj;

        if ((subColl = getSqlResultByInx (genQueryOut, COL_COLL_NAME))
          == NULL) {
            rodsLog (LOG_ERROR,
              "svrRmSpecCollRecur:getSqlResultByInx COL_COLL_NAME failed");
            return (UNMATCHED_KEY_OR_INDEX);
        }

        if ((dataObj = getSqlResultByInx (genQueryOut, COL_DATA_NAME))
          == NULL) {
            rodsLog (LOG_ERROR,
              "svrRmSpecCollRecur: getSqlResultByInx COL_DATA_NAME failed");
            return (UNMATCHED_KEY_OR_INDEX);
        }

        for (i = 0; i < genQueryOut->rowCnt; i++) {
            char *tmpSubColl, *tmpDataName;

            tmpSubColl = &subColl->value[subColl->len * i];
            tmpDataName = &dataObj->value[dataObj->len * i];

            snprintf (myDataObjInfo.objPath, MAX_NAME_LEN, "%s/%s",
              tmpSubColl, tmpDataName);

            status = getMountedSubPhyPath (dataObjInfo->objPath,
              dataObjInfo->filePath, myDataObjInfo.objPath,
              myDataObjInfo.filePath);
            if (status < 0) {
                rodsLog (LOG_ERROR,
                  "svrRmSpecCollRecur:getMountedSubPhyPat err for %s,stat=%d",
                  myDataObjInfo.filePath, status);
		savedStatus = status;
            }

	    if (strlen (tmpDataName) > 0) {
	        status = l3Unlink (rsComm, &myDataObjInfo);
	    } else {
		status = svrRmSpecCollRecur (rsComm, &myDataObjInfo);
	    }

            if (status < 0) {
                rodsLog (LOG_NOTICE,
                  "svrRmSpecCollRecur: l3Unlink error for %s. status = %d",
                  myDataObjInfo.objPath, status);
		savedStatus = status;
	    }
        }
        continueInx = genQueryOut->continueInx;

        freeGenQueryOut (&genQueryOut);

        if (continueInx > 0) {
            /* More to come */
            dataObjInp.openFlags = continueInx;
            status = rsQuerySpecColl (rsComm, &dataObjInp, &genQueryOut);
        } else {
            break;
        }
    }

    status = l3Rmdir (rsComm, dataObjInfo);
    if (status < 0) savedStatus = status;

    return (savedStatus);
}

#ifdef COMPAT_201
int
rsRmCollOld201 (rsComm_t *rsComm, collInp201_t *rmCollInp)
{
    collInp_t collInp;
    int status; 

    collInp201ToCollInp (rmCollInp, &collInp);

    status = rsRmCollOld (rsComm, &collInp);

    return status;
}
#endif

