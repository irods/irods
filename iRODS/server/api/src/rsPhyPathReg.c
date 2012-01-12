/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See phyPathReg.h for a description of this API call.*/

#include "phyPathReg.h"
#include "rodsLog.h"
#include "icatDefines.h"
#include "objMetaOpr.h"
#include "dataObjOpr.h"
#include "collection.h"
#include "specColl.h"
#include "resource.h"
#include "physPath.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"
#include "reGlobalsExtern.h"
#include "miscServerFunct.h"
#include "apiHeaderAll.h"

/* phyPathRegNoChkPerm - Wrapper internal function to allow phyPathReg with 
 * no checking for path Perm.
 */
int
phyPathRegNoChkPerm (rsComm_t *rsComm, dataObjInp_t *phyPathRegInp)
{
    int status;

    addKeyVal (&phyPathRegInp->condInput, NO_CHK_FILE_PERM_KW, "");

    status = irsPhyPathReg (rsComm, phyPathRegInp);
    return (status);
}

int
rsPhyPathReg (rsComm_t *rsComm, dataObjInp_t *phyPathRegInp)
{
    int status;

    if (getValByKey (&phyPathRegInp->condInput, NO_CHK_FILE_PERM_KW) != NULL &&
      rsComm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
	return SYS_NO_API_PRIV;
    }

    status = irsPhyPathReg (rsComm, phyPathRegInp);
    return (status);
}

int
irsPhyPathReg (rsComm_t *rsComm, dataObjInp_t *phyPathRegInp)
{
    int status;
    rescGrpInfo_t *rescGrpInfo = NULL;
    rodsServerHost_t *rodsServerHost = NULL;
    int remoteFlag;
    int rescCnt;
    rodsHostAddr_t addr;
    char *tmpStr = NULL;
    char *rescGroupName = NULL;
    rescInfo_t *tmpRescInfo = NULL;

    if ((tmpStr = getValByKey (&phyPathRegInp->condInput,
      COLLECTION_TYPE_KW)) != NULL && strcmp (tmpStr, UNMOUNT_STR) == 0) {
        status = unmountFileDir (rsComm, phyPathRegInp);
        return (status);
    } else if (tmpStr != NULL && (strcmp (tmpStr, HAAW_STRUCT_FILE_STR) == 0 ||
      strcmp (tmpStr, TAR_STRUCT_FILE_STR) == 0)) {
	status = structFileReg (rsComm, phyPathRegInp);
        return (status);
    } else if (tmpStr != NULL && strcmp (tmpStr, LINK_POINT_STR) == 0) {
        status = linkCollReg (rsComm, phyPathRegInp);
        return (status);
    }

    status = getRescInfo (rsComm, NULL, &phyPathRegInp->condInput,
      &rescGrpInfo);

    if (status < 0) {
	rodsLog (LOG_ERROR,
	  "rsPhyPathReg: getRescInfo error for %s, status = %d",
	  phyPathRegInp->objPath, status);
	return (status);
    }

    rescCnt = getRescCnt (rescGrpInfo);

    if (rescCnt != 1) {
        rodsLog (LOG_ERROR,
          "rsPhyPathReg: The input resource is not unique for %s",
          phyPathRegInp->objPath);
        return (SYS_INVALID_RESC_TYPE);
    }

    if ((rescGroupName = getValByKey (&phyPathRegInp->condInput,
      RESC_GROUP_NAME_KW)) != NULL) {
        status = getRescInGrp (rsComm, rescGrpInfo->rescInfo->rescName, 
	  rescGroupName, &tmpRescInfo);
	if (status < 0) {
            rodsLog (LOG_ERROR,
              "rsPhyPathReg: resc %s not in rescGrp %s for %s",
              rescGrpInfo->rescInfo->rescName, rescGroupName,
	      phyPathRegInp->objPath);
            return SYS_UNMATCHED_RESC_IN_RESC_GRP;
	}
    }

    memset (&addr, 0, sizeof (addr));
    
    rstrcpy (addr.hostAddr, rescGrpInfo->rescInfo->rescLoc, LONG_NAME_LEN);
    remoteFlag = resolveHost (&addr, &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        status = _rsPhyPathReg (rsComm, phyPathRegInp, rescGrpInfo, 
	  rodsServerHost);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remotePhyPathReg (rsComm, phyPathRegInp, rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_ERROR,
              "rsPhyPathReg: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    return (status);
}

int
remotePhyPathReg (rsComm_t *rsComm, dataObjInp_t *phyPathRegInp, 
rodsServerHost_t *rodsServerHost)
{
   int status;

   if (rodsServerHost == NULL) {
        rodsLog (LOG_ERROR,
          "remotePhyPathReg: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    status = rcPhyPathReg (rodsServerHost->conn, phyPathRegInp);

    if (status < 0) {
        rodsLog (LOG_ERROR,
         "remotePhyPathReg: rcPhyPathReg failed for %s",
          phyPathRegInp->objPath);
    }

    return (status);
}

int
_rsPhyPathReg (rsComm_t *rsComm, dataObjInp_t *phyPathRegInp, 
rescGrpInfo_t *rescGrpInfo, rodsServerHost_t *rodsServerHost)
{
    int status;
    fileOpenInp_t chkNVPathPermInp;
    int rescTypeInx;
    char *tmpFilePath;
    char filePath[MAX_NAME_LEN];
    dataObjInfo_t dataObjInfo;
    char *tmpStr = NULL;

    if ((tmpFilePath = getValByKey (&phyPathRegInp->condInput, FILE_PATH_KW))
      == NULL) {
        rodsLog (LOG_ERROR,
	  "_rsPhyPathReg: No filePath input for %s",
	  phyPathRegInp->objPath);
	return (SYS_INVALID_FILE_PATH);
    } else {
	/* have to do this since it will be over written later */
	rstrcpy (filePath, tmpFilePath, MAX_NAME_LEN);
    }

    /* check if we need to chk permission */

    memset (&dataObjInfo, 0, sizeof (dataObjInfo));
    rstrcpy (dataObjInfo.objPath, phyPathRegInp->objPath, MAX_NAME_LEN);
    rstrcpy (dataObjInfo.filePath, filePath, MAX_NAME_LEN);
    dataObjInfo.rescInfo = rescGrpInfo->rescInfo;
    rstrcpy (dataObjInfo.rescName, rescGrpInfo->rescInfo->rescName, 
      LONG_NAME_LEN);

 
    if (getValByKey (&phyPathRegInp->condInput, NO_CHK_FILE_PERM_KW) == NULL &&
      getchkPathPerm (rsComm, phyPathRegInp, &dataObjInfo)) { 
        memset (&chkNVPathPermInp, 0, sizeof (chkNVPathPermInp));

        rescTypeInx = rescGrpInfo->rescInfo->rescTypeInx;
        rstrcpy (chkNVPathPermInp.fileName, filePath, MAX_NAME_LEN);
        chkNVPathPermInp.fileType = (fileDriverType_t)RescTypeDef[rescTypeInx].driverType;
        rstrcpy (chkNVPathPermInp.addr.hostAddr,  
	  rescGrpInfo->rescInfo->rescLoc, NAME_LEN);

        status = chkFilePathPerm (rsComm, &chkNVPathPermInp, rodsServerHost);
    
        if (status < 0) {
            rodsLog (LOG_ERROR,
              "_rsPhyPathReg: chkFilePathPerm error for %s",
              phyPathRegInp->objPath);
            return (SYS_NO_PATH_PERMISSION);
        }
    } else {
	status = 0;
    }

    if (getValByKey (&phyPathRegInp->condInput, COLLECTION_KW) != NULL) {
	status = dirPathReg (rsComm, phyPathRegInp, filePath, 
	  rescGrpInfo->rescInfo); 
    } else if ((tmpStr = getValByKey (&phyPathRegInp->condInput, 
      COLLECTION_TYPE_KW)) != NULL && strcmp (tmpStr, MOUNT_POINT_STR) == 0) {
        status = mountFileDir (rsComm, phyPathRegInp, filePath,
          rescGrpInfo->rescInfo);
    } else {
        if (getValByKey (&phyPathRegInp->condInput, REG_REPL_KW) != NULL) {
	    status = filePathRegRepl (rsComm, phyPathRegInp, filePath,
	      rescGrpInfo->rescInfo); 
	} else {
	    status = filePathReg (rsComm, phyPathRegInp, filePath,
	      rescGrpInfo->rescInfo); 
	}
    }

    return (status);
}

int
filePathRegRepl (rsComm_t *rsComm, dataObjInp_t *phyPathRegInp, char *filePath,
rescInfo_t *rescInfo)
{
    dataObjInfo_t destDataObjInfo, *dataObjInfoHead = NULL;
    regReplica_t regReplicaInp;
    char *rescGroupName = NULL;
    int status;

    status = getDataObjInfo (rsComm, phyPathRegInp, &dataObjInfoHead,
      ACCESS_READ_OBJECT, 0);

    if (status < 0) {
        rodsLog (LOG_ERROR,
          "filePathRegRepl: getDataObjInfo for %s", phyPathRegInp->objPath);
        return (status);
    }
    status = sortObjInfoForOpen (rsComm, &dataObjInfoHead, NULL, 0);
    if (status < 0) return status;

    destDataObjInfo = *dataObjInfoHead;
    rstrcpy (destDataObjInfo.filePath, filePath, MAX_NAME_LEN);
    destDataObjInfo.rescInfo = rescInfo;
    rstrcpy (destDataObjInfo.rescName, rescInfo->rescName, NAME_LEN);
    if ((rescGroupName = getValByKey (&phyPathRegInp->condInput,
      RESC_GROUP_NAME_KW)) != NULL) {
        rstrcpy (destDataObjInfo.rescGroupName, rescGroupName, NAME_LEN);
    }
    memset (&regReplicaInp, 0, sizeof (regReplicaInp));
    regReplicaInp.srcDataObjInfo = dataObjInfoHead;
    regReplicaInp.destDataObjInfo = &destDataObjInfo;
    if (getValByKey (&phyPathRegInp->condInput, SU_CLIENT_USER_KW) != NULL) {
        addKeyVal (&regReplicaInp.condInput, SU_CLIENT_USER_KW, "");
        addKeyVal (&regReplicaInp.condInput, IRODS_ADMIN_KW, "");
    } else if (getValByKey (&phyPathRegInp->condInput,
      IRODS_ADMIN_KW) != NULL) {
        addKeyVal (&regReplicaInp.condInput, IRODS_ADMIN_KW, "");
    }
    status = rsRegReplica (rsComm, &regReplicaInp);
    clearKeyVal (&regReplicaInp.condInput);
    freeAllDataObjInfo (dataObjInfoHead);

    return status;
}

int
filePathReg (rsComm_t *rsComm, dataObjInp_t *phyPathRegInp, char *filePath, 
rescInfo_t *rescInfo)
{
    dataObjInfo_t dataObjInfo;
    int status;
    char *rescGroupName = NULL;
    char *chksum;

    initDataObjInfoWithInp (&dataObjInfo, phyPathRegInp);
    if ((rescGroupName = getValByKey (&phyPathRegInp->condInput, 
      RESC_GROUP_NAME_KW)) != NULL) {
	rstrcpy (dataObjInfo.rescGroupName, rescGroupName, NAME_LEN);
    }      
    dataObjInfo.replStatus = NEWLY_CREATED_COPY;
    dataObjInfo.rescInfo = rescInfo;
    rstrcpy (dataObjInfo.rescName, rescInfo->rescName, NAME_LEN);

    if (dataObjInfo.dataSize <= 0 && 
      (dataObjInfo.dataSize = getSizeInVault (rsComm, &dataObjInfo)) < 0 &&
      dataObjInfo.dataSize != UNKNOWN_FILE_SZ) {
	status = (int) dataObjInfo.dataSize;
        rodsLog (LOG_ERROR,
         "filePathReg: getSizeInVault for %s failed, status = %d",
          dataObjInfo.objPath, status);
	return (status);
    }

    if ((chksum = getValByKey (&phyPathRegInp->condInput, 
      REG_CHKSUM_KW)) != NULL) {
        rstrcpy (dataObjInfo.chksum, chksum, NAME_LEN);
    }
    else if ((chksum = getValByKey (&phyPathRegInp->condInput, 
           VERIFY_CHKSUM_KW)) != NULL) {
        status = _dataObjChksum (rsComm, &dataObjInfo, &chksum);
        if (status < 0) {
            rodsLog (LOG_ERROR, 
             "rodsPathReg: _dataObjChksum for %s failed, status = %d",
             dataObjInfo.objPath, status);
            return (status);
        }
        rstrcpy (dataObjInfo.chksum, chksum, NAME_LEN);
    }

    status = svrRegDataObj (rsComm, &dataObjInfo);
    if (status < 0) {
        rodsLog (LOG_ERROR,
         "filePathReg: rsRegDataObj for %s failed, status = %d",
          dataObjInfo.objPath, status);
    } else {
        ruleExecInfo_t rei;
        initReiWithDataObjInp (&rei, rsComm, phyPathRegInp);
	rei.doi = &dataObjInfo;
	rei.status = status;
        rei.status = applyRule ("acPostProcForFilePathReg", NULL, &rei,
          NO_SAVE_REI);
    }

    return (status);
}

int
dirPathReg (rsComm_t *rsComm, dataObjInp_t *phyPathRegInp, char *filePath,
rescInfo_t *rescInfo)
{
    collInp_t collCreateInp;
    fileOpendirInp_t fileOpendirInp;
    fileClosedirInp_t fileClosedirInp;
    int rescTypeInx;
    int status;
    int dirFd;
    dataObjInp_t subPhyPathRegInp;
    fileReaddirInp_t fileReaddirInp;
    rodsDirent_t *rodsDirent = NULL;
    rodsObjStat_t *rodsObjStatOut = NULL;
    int forceFlag;

    status = collStat (rsComm, phyPathRegInp, &rodsObjStatOut);
    if (status < 0) {
        memset (&collCreateInp, 0, sizeof (collCreateInp));
        rstrcpy (collCreateInp.collName, phyPathRegInp->objPath, 
	  MAX_NAME_LEN);
        /* create the coll just in case it does not exist */
        status = rsCollCreate (rsComm, &collCreateInp);
	if (status < 0) return status;
    } else if (rodsObjStatOut->specColl != NULL) {
        freeRodsObjStat (rodsObjStatOut);
        rodsLog (LOG_ERROR,
          "mountFileDir: %s already mounted", phyPathRegInp->objPath);
        return (SYS_MOUNT_MOUNTED_COLL_ERR);
    }
    freeRodsObjStat (rodsObjStatOut);

    memset (&fileOpendirInp, 0, sizeof (fileOpendirInp));

    rescTypeInx = rescInfo->rescTypeInx;
    rstrcpy (fileOpendirInp.dirName, filePath, MAX_NAME_LEN);
    fileOpendirInp.fileType = (fileDriverType_t)RescTypeDef[rescTypeInx].driverType;
    rstrcpy (fileOpendirInp.addr.hostAddr,  rescInfo->rescLoc, NAME_LEN);

    dirFd = rsFileOpendir (rsComm, &fileOpendirInp);
    if (dirFd < 0) {
       rodsLog (LOG_ERROR,
          "dirPathReg: rsFileOpendir for %s error, status = %d",
          filePath, dirFd);
        return (dirFd);
    }

    fileReaddirInp.fileInx = dirFd;

    if (getValByKey (&phyPathRegInp->condInput, FORCE_FLAG_KW) != NULL) {
	forceFlag = 1;
    } else {
	forceFlag = 0;
    }

    while ((status = rsFileReaddir (rsComm, &fileReaddirInp, &rodsDirent))
      >= 0) {
        fileStatInp_t fileStatInp;
	rodsStat_t *myStat = NULL;

        if (strcmp (rodsDirent->d_name, ".") == 0 ||
          strcmp (rodsDirent->d_name, "..") == 0) {
            continue;
        }

	memset (&fileStatInp, 0, sizeof (fileStatInp));

        snprintf (fileStatInp.fileName, MAX_NAME_LEN, "%s/%s",
          filePath, rodsDirent->d_name);

        fileStatInp.fileType = fileOpendirInp.fileType; 
	fileStatInp.addr = fileOpendirInp.addr;
        status = rsFileStat (rsComm, &fileStatInp, &myStat);

	if (status != 0) {
            rodsLog (LOG_ERROR,
	      "dirPathReg: rsFileStat failed for %s, status = %d",
	      fileStatInp.fileName, status);
	    return (status);
	}

	subPhyPathRegInp = *phyPathRegInp;
	snprintf (subPhyPathRegInp.objPath, MAX_NAME_LEN, "%s/%s",
	  phyPathRegInp->objPath, rodsDirent->d_name);

	if ((myStat->st_mode & S_IFREG) != 0) {     /* a file */
	    if (forceFlag > 0) {
		/* check if it already exists */
	        if (isData (rsComm, subPhyPathRegInp.objPath, NULL) >= 0) {
		    free (myStat);
		    continue;
		}
	    }
	    subPhyPathRegInp.dataSize = myStat->st_size;
	    if (getValByKey (&phyPathRegInp->condInput, REG_REPL_KW) != NULL) {
                status = filePathRegRepl (rsComm, &subPhyPathRegInp,
                  fileStatInp.fileName, rescInfo);
	    } else {
                addKeyVal (&subPhyPathRegInp.condInput, FILE_PATH_KW, 
	          fileStatInp.fileName);
	        status = filePathReg (rsComm, &subPhyPathRegInp, 
	          fileStatInp.fileName, rescInfo);
	    }
        } else if ((myStat->st_mode & S_IFDIR) != 0) {      /* a directory */
            status = dirPathReg (rsComm, &subPhyPathRegInp,
              fileStatInp.fileName, rescInfo);
	}
	free (myStat);
    }
    if (status == -1) {         /* just EOF */
        status = 0;
    }

    fileClosedirInp.fileInx = dirFd;
    rsFileClosedir (rsComm, &fileClosedirInp);

    return (status);
}

int
mountFileDir (rsComm_t *rsComm, dataObjInp_t *phyPathRegInp, char *filePath,
rescInfo_t *rescInfo)
{
    collInp_t collCreateInp;
    int rescTypeInx;
    int status;
    fileStatInp_t fileStatInp;
    rodsStat_t *myStat = NULL;
    rodsObjStat_t *rodsObjStatOut = NULL;

    status = collStat (rsComm, phyPathRegInp, &rodsObjStatOut);
    if (status < 0) return status;

    if (rodsObjStatOut->specColl != NULL) {
        freeRodsObjStat (rodsObjStatOut);
        rodsLog (LOG_ERROR,
          "mountFileDir: %s already mounted", phyPathRegInp->objPath);
        return (SYS_MOUNT_MOUNTED_COLL_ERR);
    }
    freeRodsObjStat (rodsObjStatOut);

    if (isCollEmpty (rsComm, phyPathRegInp->objPath) == False) {
        rodsLog (LOG_ERROR,
          "mountFileDir: collection %s not empty", phyPathRegInp->objPath);
        return (SYS_COLLECTION_NOT_EMPTY);
    }

    memset (&fileStatInp, 0, sizeof (fileStatInp));

    rstrcpy (fileStatInp.fileName, filePath, MAX_NAME_LEN);

    rescTypeInx = rescInfo->rescTypeInx;
    fileStatInp.fileType = (fileDriverType_t)RescTypeDef[rescTypeInx].driverType;
    rstrcpy (fileStatInp.addr.hostAddr,  rescInfo->rescLoc, NAME_LEN);
    status = rsFileStat (rsComm, &fileStatInp, &myStat);

    if (status < 0) {
	fileMkdirInp_t fileMkdirInp;

        rodsLog (LOG_NOTICE,
          "mountFileDir: rsFileStat failed for %s, status = %d, create it",
          fileStatInp.fileName, status);
	memset (&fileMkdirInp, 0, sizeof (fileMkdirInp));
	rstrcpy (fileMkdirInp.dirName, filePath, MAX_NAME_LEN);
        fileMkdirInp.fileType = (fileDriverType_t)RescTypeDef[rescTypeInx].driverType;
	fileMkdirInp.mode = getDefDirMode ();
        rstrcpy (fileMkdirInp.addr.hostAddr,  rescInfo->rescLoc, NAME_LEN);
	status = rsFileMkdir (rsComm, &fileMkdirInp);
	if (status < 0) {
            return (status);
	}
    } else if ((myStat->st_mode & S_IFDIR) == 0) {
        rodsLog (LOG_ERROR,
          "mountFileDir: phyPath %s is not a directory",
          fileStatInp.fileName);
	free (myStat);
        return (USER_FILE_DOES_NOT_EXIST);
    }

    free (myStat);
    /* mk the collection */

    memset (&collCreateInp, 0, sizeof (collCreateInp));
    rstrcpy (collCreateInp.collName, phyPathRegInp->objPath, MAX_NAME_LEN);
    addKeyVal (&collCreateInp.condInput, COLLECTION_TYPE_KW, MOUNT_POINT_STR);

    addKeyVal (&collCreateInp.condInput, COLLECTION_INFO1_KW, filePath);
    addKeyVal (&collCreateInp.condInput, COLLECTION_INFO2_KW, 
      rescInfo->rescName);

    /* try to mod the coll first */
    status = rsModColl (rsComm, &collCreateInp);

    if (status < 0) {	/* try to create it */
       status = rsRegColl (rsComm, &collCreateInp);
    }

    if (status >= 0) {
        char outLogPath[MAX_NAME_LEN];
	int status1;
	/* see if the phyPath is mapped into a real collection */
	if (getLogPathFromPhyPath (filePath, rescInfo, outLogPath) >= 0 &&
	  strcmp (outLogPath, phyPathRegInp->objPath) != 0) {
	    /* log path not the same as input objPath */
	    if (isColl (rsComm, outLogPath, NULL) >= 0) {
		modAccessControlInp_t modAccessControl;
		/* it is a real collection. better set the collection
		 * to read-only mode because any modification to files
		 * through this mounted collection can be trouble */
		bzero (&modAccessControl, sizeof (modAccessControl));
		modAccessControl.accessLevel = "read";
		modAccessControl.userName = rsComm->clientUser.userName;
		modAccessControl.zone = rsComm->clientUser.rodsZone;
		modAccessControl.path = phyPathRegInp->objPath;
                status1 = rsModAccessControl(rsComm, &modAccessControl);
                if (status1 < 0) {
                    rodsLog (LOG_NOTICE, 
		      "mountFileDir: rsModAccessControl err for %s, stat = %d",
                      phyPathRegInp->objPath, status1);
		}
	    }
	}
    }
    return (status);
}

int
unmountFileDir (rsComm_t *rsComm, dataObjInp_t *phyPathRegInp)
{
    int status;
    collInp_t modCollInp;
    rodsObjStat_t *rodsObjStatOut = NULL;

    status = collStat (rsComm, phyPathRegInp, &rodsObjStatOut);
    if (status < 0) {
        return status;
    } else if (rodsObjStatOut->specColl == NULL) {
        freeRodsObjStat (rodsObjStatOut);
        rodsLog (LOG_ERROR,
          "unmountFileDir: %s not mounted", phyPathRegInp->objPath);
        return (SYS_COLL_NOT_MOUNTED_ERR);
    }

    if (getStructFileType (rodsObjStatOut->specColl) >= 0) {    
	/* a struct file */
        status = _rsSyncMountedColl (rsComm, rodsObjStatOut->specColl,
          PURGE_STRUCT_FILE_CACHE);
#if 0
	if (status < 0) {
	    freeRodsObjStat (rodsObjStatOut);
	    return (status);
	}
#endif
    }

    freeRodsObjStat (rodsObjStatOut);

    memset (&modCollInp, 0, sizeof (modCollInp));
    rstrcpy (modCollInp.collName, phyPathRegInp->objPath, MAX_NAME_LEN);
    addKeyVal (&modCollInp.condInput, COLLECTION_TYPE_KW, 
      "NULL_SPECIAL_VALUE");
    addKeyVal (&modCollInp.condInput, COLLECTION_INFO1_KW, "NULL_SPECIAL_VALUE");
    addKeyVal (&modCollInp.condInput, COLLECTION_INFO2_KW, "NULL_SPECIAL_VALUE");

    status = rsModColl (rsComm, &modCollInp);

    return (status);
}

int
structFileReg (rsComm_t *rsComm, dataObjInp_t *phyPathRegInp)
{
    collInp_t collCreateInp;
    int status;
    dataObjInfo_t *dataObjInfo = NULL;
    char *structFilePath = NULL;
    dataObjInp_t dataObjInp;
    char *collType;
    int len;
    rodsObjStat_t *rodsObjStatOut = NULL;
    specCollCache_t *specCollCache = NULL;
    rescInfo_t *rescInfo = NULL;

    if ((structFilePath = getValByKey (&phyPathRegInp->condInput, FILE_PATH_KW))
      == NULL) {
        rodsLog (LOG_ERROR,
          "structFileReg: No structFilePath input for %s",
          phyPathRegInp->objPath);
        return (SYS_INVALID_FILE_PATH);
    }

    collType = getValByKey (&phyPathRegInp->condInput, COLLECTION_TYPE_KW);
    if (collType == NULL) {
        rodsLog (LOG_ERROR,
          "structFileReg: Bad COLLECTION_TYPE_KW for structFilePath %s",
              dataObjInp.objPath);
            return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    len = strlen (phyPathRegInp->objPath);
    if (strncmp (structFilePath, phyPathRegInp->objPath, len) == 0 &&
     (structFilePath[len] == '\0' || structFilePath[len] == '/')) {
        rodsLog (LOG_ERROR,
          "structFileReg: structFilePath %s inside collection %s",
          structFilePath, phyPathRegInp->objPath);
        return (SYS_STRUCT_FILE_INMOUNTED_COLL);
    }

    /* see if the struct file is in spec coll */

    if (getSpecCollCache (rsComm, structFilePath, 0,  &specCollCache) >= 0) {
        rodsLog (LOG_ERROR,
          "structFileReg: structFilePath %s is in a mounted path",
          structFilePath);
        return (SYS_STRUCT_FILE_INMOUNTED_COLL);
    }

    status = collStat (rsComm, phyPathRegInp, &rodsObjStatOut);
    if (status < 0) return status;
 
    if (rodsObjStatOut->specColl != NULL) {
	freeRodsObjStat (rodsObjStatOut);
        rodsLog (LOG_ERROR,
          "structFileReg: %s already mounted", phyPathRegInp->objPath);
	return (SYS_MOUNT_MOUNTED_COLL_ERR);
    }

    freeRodsObjStat (rodsObjStatOut);

    if (isCollEmpty (rsComm, phyPathRegInp->objPath) == False) {
        rodsLog (LOG_ERROR,
          "structFileReg: collection %s not empty", phyPathRegInp->objPath);
        return (SYS_COLLECTION_NOT_EMPTY);
    }

    memset (&dataObjInp, 0, sizeof (dataObjInp));
    rstrcpy (dataObjInp.objPath, structFilePath, sizeof (dataObjInp));
    /* user need to have write permission */
    dataObjInp.openFlags = O_WRONLY;
    status = getDataObjInfoIncSpecColl (rsComm, &dataObjInp, &dataObjInfo);
    if (status < 0) {
	int myStatus;
	/* try to make one */
	dataObjInp.condInput = phyPathRegInp->condInput;
	/* have to remove FILE_PATH_KW because getFullPathName will use it */
	rmKeyVal (&dataObjInp.condInput, FILE_PATH_KW);
	myStatus = rsDataObjCreate (rsComm, &dataObjInp);
	if (myStatus < 0) {
            rodsLog (LOG_ERROR,
              "structFileReg: Problem with open/create structFilePath %s, status = %d",
              dataObjInp.objPath, status);
            return (status);
	} else {
	    openedDataObjInp_t dataObjCloseInp;
	    bzero (&dataObjCloseInp, sizeof (dataObjCloseInp));
	    rescInfo = L1desc[myStatus].dataObjInfo->rescInfo;
	    dataObjCloseInp.l1descInx = myStatus;
	    rsDataObjClose (rsComm, &dataObjCloseInp);
	}
    } else {
	rescInfo = dataObjInfo->rescInfo;
    }

    if (!structFileSupport (rsComm, phyPathRegInp->objPath, 
      collType, rescInfo)) {
        rodsLog (LOG_ERROR,
          "structFileReg: structFileDriver type %s does not exist for %s",
          collType, dataObjInp.objPath);
        return (SYS_NOT_SUPPORTED);
    }

    /* mk the collection */

    memset (&collCreateInp, 0, sizeof (collCreateInp));
    rstrcpy (collCreateInp.collName, phyPathRegInp->objPath, MAX_NAME_LEN);
    addKeyVal (&collCreateInp.condInput, COLLECTION_TYPE_KW, collType);

    /* have to use dataObjInp.objPath because structFile path was removed */ 
    addKeyVal (&collCreateInp.condInput, COLLECTION_INFO1_KW, 
     dataObjInp.objPath);

    /* try to mod the coll first */
    status = rsModColl (rsComm, &collCreateInp);

    if (status < 0) {	/* try to create it */
       status = rsRegColl (rsComm, &collCreateInp);
    }

    return (status);
}

int
structFileSupport (rsComm_t *rsComm, char *collection, char *collType, 
rescInfo_t *rescInfo)
{
    rodsStat_t *myStat = NULL;
    int status;
    subFile_t subFile;
    specColl_t specColl;

    if (rsComm == NULL || collection == NULL || collType == NULL || 
      rescInfo == NULL) return 0;

    memset (&subFile, 0, sizeof (subFile));
    memset (&specColl, 0, sizeof (specColl));
    /* put in some fake path */
    subFile.specColl = &specColl;
    rstrcpy (specColl.collection, collection, MAX_NAME_LEN);
    specColl.collClass = STRUCT_FILE_COLL;
    if (strcmp (collType, HAAW_STRUCT_FILE_STR) == 0) { 
        specColl.type = HAAW_STRUCT_FILE_T;
    } else if (strcmp (collType, TAR_STRUCT_FILE_STR) == 0) {
	specColl.type = TAR_STRUCT_FILE_T;
    } else {
	return (0);
    }
    snprintf (specColl.objPath, MAX_NAME_LEN, "%s/myFakeFile",
      collection);
    rstrcpy (specColl.resource, rescInfo->rescName, NAME_LEN);
    rstrcpy (specColl.phyPath, "/fakeDir1/fakeDir2/myFakeStructFile",
      MAX_NAME_LEN);
    rstrcpy (subFile.subFilePath, "/fakeDir1/fakeDir2/myFakeFile",
      MAX_NAME_LEN);
    rstrcpy (subFile.addr.hostAddr, rescInfo->rescLoc, NAME_LEN);

    status = rsSubStructFileStat (rsComm, &subFile, &myStat);

    if (status == SYS_NOT_SUPPORTED)
	return (0);
    else
	return (1);
}

int
linkCollReg (rsComm_t *rsComm, dataObjInp_t *phyPathRegInp)
{
    collInp_t collCreateInp;
    int status;
    char *linkPath = NULL;
    char *collType;
    int len;
    rodsObjStat_t *rodsObjStatOut = NULL;
    specCollCache_t *specCollCache = NULL;

    if ((linkPath = getValByKey (&phyPathRegInp->condInput, FILE_PATH_KW))
      == NULL) {
        rodsLog (LOG_ERROR,
          "linkCollReg: No linkPath input for %s",
          phyPathRegInp->objPath);
        return (SYS_INVALID_FILE_PATH);
    }

    collType = getValByKey (&phyPathRegInp->condInput, COLLECTION_TYPE_KW);
    if (collType == NULL || strcmp (collType, LINK_POINT_STR) != 0) {
        rodsLog (LOG_ERROR,
          "linkCollReg: Bad COLLECTION_TYPE_KW for linkPath %s",
              phyPathRegInp->objPath);
            return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    if (phyPathRegInp->objPath[0] != '/' || linkPath[0] != '/') {
        rodsLog (LOG_ERROR,
          "linkCollReg: linkPath %s or collection %s not absolute path",
          linkPath, phyPathRegInp->objPath);
        return (SYS_COLL_LINK_PATH_ERR);
    }

    len = strlen (phyPathRegInp->objPath);
    if (strncmp (linkPath, phyPathRegInp->objPath, len) == 0 && 
      linkPath[len] == '/') { 
        rodsLog (LOG_ERROR,
          "linkCollReg: linkPath %s inside collection %s",
          linkPath, phyPathRegInp->objPath);
        return (SYS_COLL_LINK_PATH_ERR);
    }

    len = strlen (linkPath);
    if (strncmp (phyPathRegInp->objPath, linkPath, len) == 0 &&
      phyPathRegInp->objPath[len] == '/') {
        rodsLog (LOG_ERROR,
          "linkCollReg: collection %s inside linkPath %s",
          linkPath, phyPathRegInp->objPath);
        return (SYS_COLL_LINK_PATH_ERR);
    }

    if (getSpecCollCache (rsComm, linkPath, 0,  &specCollCache) >= 0 &&
     specCollCache->specColl.collClass != LINKED_COLL) {
        rodsLog (LOG_ERROR,
          "linkCollReg: linkPath %s is in a spec coll path",
          linkPath);
        return (SYS_COLL_LINK_PATH_ERR);
    }

    status = collStat (rsComm, phyPathRegInp, &rodsObjStatOut);
    if (status < 0) {
	/* does not exist. make one */
	collInp_t collCreateInp;
        memset (&collCreateInp, 0, sizeof (collCreateInp));
        rstrcpy (collCreateInp.collName, phyPathRegInp->objPath, MAX_NAME_LEN);
        status = rsRegColl (rsComm, &collCreateInp);
	if (status < 0) {
            rodsLog (LOG_ERROR,
               "linkCollReg: rsRegColl error for  %s, status = %d",
               collCreateInp.collName, status);
             return status;
        }
        status = collStat (rsComm, phyPathRegInp, &rodsObjStatOut);
	if (status < 0) return status;

    }

    if (rodsObjStatOut->specColl != NULL && 
      rodsObjStatOut->specColl->collClass != LINKED_COLL) {
        freeRodsObjStat (rodsObjStatOut);
        rodsLog (LOG_ERROR,
          "linkCollReg: link collection %s in a spec coll path", 
	  phyPathRegInp->objPath);
        return (SYS_COLL_LINK_PATH_ERR);
    }

    freeRodsObjStat (rodsObjStatOut);

    if (isCollEmpty (rsComm, phyPathRegInp->objPath) == False) {
        rodsLog (LOG_ERROR,
          "linkCollReg: collection %s not empty", phyPathRegInp->objPath);
        return (SYS_COLLECTION_NOT_EMPTY);
    }

    /* mk the collection */

    memset (&collCreateInp, 0, sizeof (collCreateInp));
    rstrcpy (collCreateInp.collName, phyPathRegInp->objPath, MAX_NAME_LEN);
    addKeyVal (&collCreateInp.condInput, COLLECTION_TYPE_KW, collType);

    /* have to use dataObjInp.objPath because structFile path was removed */
    addKeyVal (&collCreateInp.condInput, COLLECTION_INFO1_KW,
     linkPath);

    /* try to mod the coll first */
    status = rsModColl (rsComm, &collCreateInp);

    if (status < 0) {   /* try to create it */
       status = rsRegColl (rsComm, &collCreateInp);
    }

    return (status);
}

