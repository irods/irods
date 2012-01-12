/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsBulkDataObjPut.c. See bulkDataObjReg.h for a description of
 * this API call.*/

#include "apiHeaderAll.h"
#include "objMetaOpr.h"
#include "resource.h"
#include "collection.h"
#include "specColl.h"
#include "dataObjOpr.h"
#include "physPath.h"
#include "miscServerFunct.h"
#include "rcGlobalExtern.h"
#include "reGlobalsExtern.h"

int
rsBulkDataObjPut (rsComm_t *rsComm, bulkOprInp_t *bulkOprInp,
bytesBuf_t *bulkOprInpBBuf)
{
    int status;
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;
    specCollCache_t *specCollCache = NULL;
    dataObjInp_t dataObjInp;

    resolveLinkedPath (rsComm, bulkOprInp->objPath, &specCollCache,
      &bulkOprInp->condInput);

    /* need to setup dataObjInp */
    initDataObjInpFromBulkOpr (&dataObjInp, bulkOprInp);

    remoteFlag = getAndConnRemoteZone (rsComm, &dataObjInp, &rodsServerHost,
      REMOTE_CREATE);

    if (remoteFlag < 0) {
        return (remoteFlag);
    } else if (remoteFlag == LOCAL_HOST) {
        status = _rsBulkDataObjPut (rsComm, bulkOprInp, bulkOprInpBBuf);
    } else {
        status = rcBulkDataObjPut (rodsServerHost->conn, bulkOprInp,
          bulkOprInpBBuf);
    }
    return status;
}

int
_rsBulkDataObjPut (rsComm_t *rsComm, bulkOprInp_t *bulkOprInp,
bytesBuf_t *bulkOprInpBBuf)
{
    int status;
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;
    rescInfo_t *rescInfo;
    char *inpRescGrpName;
    char phyBunDir[MAX_NAME_LEN];
    rescGrpInfo_t *myRescGrpInfo = NULL;
    int flags = 0;
    dataObjInp_t dataObjInp;
    fileDriverType_t fileType;
    rodsObjStat_t *myRodsObjStat = NULL;

    inpRescGrpName = getValByKey (&bulkOprInp->condInput, RESC_GROUP_NAME_KW);

    status = chkCollForExtAndReg (rsComm, bulkOprInp->objPath, &myRodsObjStat);
    if (status < 0) return status;

    /* query rcat for resource info and sort it */

    /* need to setup dataObjInp */
    initDataObjInpFromBulkOpr (&dataObjInp, bulkOprInp);

    if (myRodsObjStat->specColl != NULL) {
        status = resolveResc (myRodsObjStat->specColl->resource, &rescInfo);
        if (status < 0) {
            rodsLog (LOG_ERROR,
              "_rsBulkDataObjPut: resolveResc error for %s, status = %d",
             myRodsObjStat->specColl->resource, status);
	    freeRodsObjStat (myRodsObjStat);
            return (status);
	}
    } else {
        status = getRescGrpForCreate (rsComm, &dataObjInp, &myRescGrpInfo);
        if (status < 0) {
	    freeRodsObjStat (myRodsObjStat);
	    return status;
	}

        /* just take the top one */
        rescInfo = myRescGrpInfo->rescInfo;
    }
    fileType = (fileDriverType_t)getRescType (rescInfo);
    if (fileType != UNIX_FILE_TYPE && fileType != NT_FILE_TYPE) {
	freeRodsObjStat (myRodsObjStat);
	return SYS_INVALID_RESC_FOR_BULK_OPR;
    }

    remoteFlag = resolveHostByRescInfo (rescInfo, &rodsServerHost);

    if (remoteFlag == REMOTE_HOST) {
        addKeyVal (&bulkOprInp->condInput, DEST_RESC_NAME_KW,
          rescInfo->rescName);
	if (myRodsObjStat->specColl == NULL && inpRescGrpName == NULL && 
	  strlen (myRescGrpInfo->rescGroupName) > 0) {
            addKeyVal (&bulkOprInp->condInput, RESC_GROUP_NAME_KW,
              myRescGrpInfo->rescGroupName);
	}
	freeRodsObjStat (myRodsObjStat);
        if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
            return status;
        }
        status = rcBulkDataObjPut (rodsServerHost->conn, bulkOprInp, 
          bulkOprInpBBuf);
        freeAllRescGrpInfo (myRescGrpInfo);
	return status;
    }

    status = createBunDirForBulkPut (rsComm, &dataObjInp, rescInfo, 
      myRodsObjStat->specColl, phyBunDir);

    if (status < 0) {
	freeRodsObjStat (myRodsObjStat);
	return status;
    }

#ifdef BULK_OPR_WITH_TAR
    status = untarBuf (phyBunDir, bulkOprInpBBuf);
    if (status < 0) {
        rodsLog (LOG_ERROR,
          "_rsBulkDataObjPut: untarBuf for dir %s. stat = %d",
          phyBunDir, status);
        return status;
    }
#else
    status = unbunBulkBuf (phyBunDir, bulkOprInp, bulkOprInpBBuf);
    if (status < 0) {
        rodsLog (LOG_ERROR,
          "_rsBulkDataObjPut: unbunBulkBuf for dir %s. stat = %d",
          phyBunDir, status);
        return status;
    }
#endif
    if (myRodsObjStat->specColl != NULL) {
	freeRodsObjStat (myRodsObjStat);
	return status;
    } else {
        freeRodsObjStat (myRodsObjStat);
    }

    if (strlen (myRescGrpInfo->rescGroupName) > 0) 
        inpRescGrpName = myRescGrpInfo->rescGroupName;

    if (getValByKey (&bulkOprInp->condInput, FORCE_FLAG_KW) != NULL) {
        flags = flags | FORCE_FLAG_FLAG;
    }

    if (getValByKey (&bulkOprInp->condInput, VERIFY_CHKSUM_KW) != NULL) {
        flags = flags | VERIFY_CHKSUM_FLAG;
    }

    status = regUnbunSubfiles (rsComm, rescInfo, inpRescGrpName,
      bulkOprInp->objPath, phyBunDir, flags, &bulkOprInp->attriArray);

    if (status == CAT_NO_ROWS_FOUND) {
        /* some subfiles have been deleted. harmless */
        status = 0;
    } else if (status < 0) {
        rodsLog (LOG_ERROR,
          "_rsBulkDataObjPut: regUnbunSubfiles for dir %s. stat = %d",
          phyBunDir, status);
    }
    freeAllRescGrpInfo (myRescGrpInfo);
    return status;
}

int
createBunDirForBulkPut (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
rescInfo_t *rescInfo, specColl_t *specColl, char *phyBunDir)
{
    dataObjInfo_t dataObjInfo;
#ifndef USE_BOOST_FS
    struct stat statbuf;
#endif
    int status;

    if (dataObjInp == NULL || rescInfo == NULL || phyBunDir == NULL) 
	return USER__NULL_INPUT_ERR;

    if (specColl != NULL) {
        status = getMountedSubPhyPath (specColl->collection,
          specColl->phyPath, dataObjInp->objPath, phyBunDir);
        if (status >= 0) {
            mkdirR ("/", phyBunDir, getDefDirMode ());
        }
	return status;
    }
    bzero (&dataObjInfo, sizeof (dataObjInfo));
    rstrcpy (dataObjInfo.objPath, dataObjInp->objPath, MAX_NAME_LEN);
    rstrcpy (dataObjInfo.rescName, rescInfo->rescName, NAME_LEN);
    dataObjInfo.rescInfo = rescInfo;

    status = getFilePathName (rsComm, &dataObjInfo, dataObjInp);
    if (status < 0) {
        rodsLog (LOG_ERROR,
          "createBunDirForBulkPut: getFilePathName err for %s. status = %d",
          dataObjInp->objPath, status);
        return (status);
    }
    do {
	snprintf (phyBunDir, MAX_NAME_LEN, "%s/%s.%d", dataObjInfo.filePath,
	  TMP_PHY_BUN_DIR, (int) random ());
#ifdef USE_BOOST_FS
        path p (phyBunDir);
	if (exists (p)) status = 0;
	else status = -1;
#else
	status =  stat (phyBunDir, &statbuf);
#endif
    } while (status == 0);

    mkdirR ("/", phyBunDir, getDefDirMode ());

    return 0;
}

int
initDataObjInpFromBulkOpr (dataObjInp_t *dataObjInp, bulkOprInp_t *bulkOprInp)
{
    if (dataObjInp == NULL || bulkOprInp == NULL)
	return USER__NULL_INPUT_ERR;

    bzero (dataObjInp, sizeof (dataObjInp_t));
    rstrcpy (dataObjInp->objPath, bulkOprInp->objPath, MAX_NAME_LEN);
    dataObjInp->condInput = bulkOprInp->condInput;

    return (0);
}

int
bulkRegUnbunSubfiles (rsComm_t *rsComm, rescInfo_t *rescInfo, 
char *rescGroupName, char *collection, char *phyBunDir, int flags, 
genQueryOut_t *attriArray)
{
    genQueryOut_t bulkDataObjRegInp;
    renamedPhyFiles_t renamedPhyFiles;
    int status = 0;

    bzero (&renamedPhyFiles, sizeof (renamedPhyFiles));
    initBulkDataObjRegInp (&bulkDataObjRegInp);
    /* the continueInx is used for the matching of objPath */
    if (attriArray != NULL) attriArray->continueInx = 0;

    status = _bulkRegUnbunSubfiles (rsComm, rescInfo, rescGroupName, collection,
      phyBunDir, flags, &bulkDataObjRegInp, &renamedPhyFiles, attriArray);

    if (bulkDataObjRegInp.rowCnt > 0) {
        int status1;
        genQueryOut_t *bulkDataObjRegOut = NULL;
        status1 = rsBulkDataObjReg (rsComm, &bulkDataObjRegInp,
          &bulkDataObjRegOut);
        if (status1 < 0) {
            status = status1;
            rodsLog (LOG_ERROR,
              "regUnbunSubfiles: rsBulkDataObjReg error for %s. stat = %d",
              collection, status1);
            cleanupBulkRegFiles (rsComm, &bulkDataObjRegInp);
        }
        postProcRenamedPhyFiles (&renamedPhyFiles, status);
        postProcBulkPut (rsComm, &bulkDataObjRegInp, bulkDataObjRegOut);
        freeGenQueryOut (&bulkDataObjRegOut);
    }
    clearGenQueryOut (&bulkDataObjRegInp);
    return status;
}

int
_bulkRegUnbunSubfiles (rsComm_t *rsComm, rescInfo_t *rescInfo, 
char *rescGroupName, char *collection, char *phyBunDir, int flags, 
genQueryOut_t *bulkDataObjRegInp, renamedPhyFiles_t *renamedPhyFiles, 
genQueryOut_t *attriArray)
{
#ifndef USE_BOOST_FS
    DIR *dirPtr;
    struct dirent *myDirent;
    struct stat statbuf;
#endif
    char subfilePath[MAX_NAME_LEN];
    char subObjPath[MAX_NAME_LEN];
    dataObjInp_t dataObjInp;
    int status;
    int savedStatus = 0;
    int st_mode;
    rodsLong_t st_size;

#ifdef USE_BOOST_FS
    path srcDirPath (phyBunDir);
    if (!exists(srcDirPath) || !is_directory(srcDirPath)) {
#else
    dirPtr = opendir (phyBunDir);
    if (dirPtr == NULL) {
#endif
        rodsLog (LOG_ERROR,
        "regUnbunphySubfiles: opendir error for %s, errno = %d",
         phyBunDir, errno);
        return (UNIX_FILE_OPENDIR_ERR - errno);
    }
    bzero (&dataObjInp, sizeof (dataObjInp));
#ifdef USE_BOOST_FS
    directory_iterator end_itr; // default construction yields past-the-end
    for (directory_iterator itr(srcDirPath); itr != end_itr;++itr) {
        path p = itr->path();
        snprintf (subfilePath, MAX_NAME_LEN, "%s",
          p.c_str ());
#else
    while ((myDirent = readdir (dirPtr)) != NULL) {
        if (strcmp (myDirent->d_name, ".") == 0 ||
          strcmp (myDirent->d_name, "..") == 0) {
            continue;
        }
        snprintf (subfilePath, MAX_NAME_LEN, "%s/%s",
          phyBunDir, myDirent->d_name);
#endif

#ifdef USE_BOOST_FS
	if (!exists (p)) {
#else
        status = stat (subfilePath, &statbuf);

        if (status != 0) {
#endif
            rodsLog (LOG_ERROR,
              "regUnbunphySubfiles: stat error for %s, errno = %d",
              subfilePath, errno);
            savedStatus = UNIX_FILE_STAT_ERR - errno;
            unlink (subfilePath);
            continue;
        }

#ifdef USE_BOOST_FS
        path childPath = p.filename();
        snprintf (subObjPath, MAX_NAME_LEN, "%s/%s",
          collection, childPath.c_str());
	if (is_directory (p)) {
#else
        snprintf (subObjPath, MAX_NAME_LEN, "%s/%s",
          collection, myDirent->d_name);
       if ((statbuf.st_mode & S_IFDIR) != 0) {
#endif
            status = rsMkCollR (rsComm, "/", subObjPath);
            if (status < 0) {
                rodsLog (LOG_ERROR,
                  "regUnbunSubfiles: rsMkCollR of %s error. status = %d",
                  subObjPath, status);
                savedStatus = status;
                continue;
            }
            status = _bulkRegUnbunSubfiles (rsComm, rescInfo, rescGroupName,
              subObjPath, subfilePath, flags, bulkDataObjRegInp,
              renamedPhyFiles, attriArray);
            if (status < 0) {
                rodsLog (LOG_ERROR,
                  "regUnbunSubfiles: regUnbunSubfiles of %s error. status=%d",
                  subObjPath, status);
                savedStatus = status;
                continue;
            }
#ifdef USE_BOOST_FS
	} else if (is_regular_file (p)) {
	    st_mode = getPathStMode (p);
	    st_size = file_size (p);
#else
        } else if ((statbuf.st_mode & S_IFREG) != 0) {
	    st_mode = statbuf.st_mode;
	    st_size = statbuf.st_size;
#endif
            status = bulkProcAndRegSubfile (rsComm, rescInfo, rescGroupName,
              subObjPath, subfilePath, st_size,
              st_mode & 0777, flags, bulkDataObjRegInp,
              renamedPhyFiles, attriArray);
            unlink (subfilePath);
            if (status < 0) {
                rodsLog (LOG_ERROR,
                 "regUnbunSubfiles:bulkProcAndRegSubfile of %s err.stat=%d",
                    subObjPath, status);
                savedStatus = status;
                continue;
            }
        }
    }
#ifndef USE_BOOST_FS
    closedir (dirPtr);
#endif
    rmdir (phyBunDir);
    return savedStatus;
}

int
bulkProcAndRegSubfile (rsComm_t *rsComm, rescInfo_t *rescInfo,
char *rescGroupName, char *subObjPath, char *subfilePath, rodsLong_t dataSize,
int dataMode, int flags, genQueryOut_t *bulkDataObjRegInp,
renamedPhyFiles_t *renamedPhyFiles, genQueryOut_t *attriArray)
{
    dataObjInfo_t dataObjInfo;
    dataObjInp_t dataObjInp;
#ifndef USE_BOOST_FS
    struct stat statbuf;
#endif
    int status;
    int modFlag = 0;
    char *myChksum = NULL;
    int myDataMode = dataMode;

    bzero (&dataObjInp, sizeof (dataObjInp));
    bzero (&dataObjInfo, sizeof (dataObjInfo));
    rstrcpy (dataObjInp.objPath, subObjPath, MAX_NAME_LEN);
    rstrcpy (dataObjInfo.objPath, subObjPath, MAX_NAME_LEN);
    rstrcpy (dataObjInfo.rescName, rescInfo->rescName, NAME_LEN);
    rstrcpy (dataObjInfo.dataType, "generic", NAME_LEN);
    dataObjInfo.rescInfo = rescInfo;
    dataObjInfo.dataSize = dataSize;

    status = getFilePathName (rsComm, &dataObjInfo, &dataObjInp);
    if (status < 0) {
        rodsLog (LOG_ERROR,
          "regSubFile: getFilePathName err for %s. status = %d",
          dataObjInp.objPath, status);
        return (status);
    }

#ifdef USE_BOOST_FS
    path p (dataObjInfo.filePath);
    if (exists (p)) {
	if (is_directory (p)) {
#else
    status = stat (dataObjInfo.filePath, &statbuf);
    if (status == 0 || errno != ENOENT) {
        if ((statbuf.st_mode & S_IFDIR) != 0) {
#endif
            return SYS_PATH_IS_NOT_A_FILE;
        }
        if (chkOrphanFile (rsComm, dataObjInfo.filePath, rescInfo->rescName,
          &dataObjInfo) <= 0) {
            /* not an orphan file */
            if ((flags & FORCE_FLAG_FLAG) != 0 && dataObjInfo.dataId > 0 &&
              strcmp (dataObjInfo.objPath, subObjPath) == 0) {
                /* overwrite the current file */
                modFlag = 1;
            } else {
                status = SYS_COPY_ALREADY_IN_RESC;
                rodsLog (LOG_ERROR,
                  "bulkProcAndRegSubfile: phypath %s is already in use. status = %d",
                  dataObjInfo.filePath, status);
                return (status);
            }
        }
        /* rename it to the orphan dir */
        fileRenameInp_t fileRenameInp;
        bzero (&fileRenameInp, sizeof (fileRenameInp));
        rstrcpy (fileRenameInp.oldFileName, dataObjInfo.filePath,
          MAX_NAME_LEN);
        status = renameFilePathToNewDir (rsComm, ORPHAN_DIR,
          &fileRenameInp, rescInfo, 1);
        if (status < 0) {
            rodsLog (LOG_ERROR,
              "bulkProcAndRegSubfile: renameFilePathToNewDir err for %s. status = %d",
              fileRenameInp.oldFileName, status);
            return (status);
        }
        if (modFlag > 0) {
            status = addRenamedPhyFile (subObjPath, fileRenameInp.oldFileName,
              fileRenameInp.newFileName, renamedPhyFiles);
            if (status < 0) return status;
        }
    } else {
        /* make the necessary dir */
        mkDirForFilePath (UNIX_FILE_TYPE, rsComm, "/", dataObjInfo.filePath,
          getDefDirMode ());
    }
    /* add a link */
#ifndef windows_platform
    status = link (subfilePath, dataObjInfo.filePath);
    if (status < 0) {
        rodsLog (LOG_ERROR,
          "bulkProcAndRegSubfile: link error %s to %s. errno = %d",
          subfilePath, dataObjInfo.filePath, errno);
        return (UNIX_FILE_LINK_ERR - errno);
    }
#endif

    if (attriArray != NULL) {
        /* dataMode in attriArray overwrites passed in value */
        status =getAttriInAttriArray (subObjPath, attriArray, &myDataMode,
          &myChksum);
        if (status < 0) {
            rodsLog (LOG_NOTICE,
              "bulkProcAndRegSubfile: matchObjPath error for %s, stat = %d",
              subObjPath, status);
        } else {
            if ((flags & VERIFY_CHKSUM_FLAG) != 0 && myChksum != NULL) {
                char chksumStr[NAME_LEN];
                /* verify the chksum */
                status = chksumLocFile (dataObjInfo.filePath, chksumStr);
                if (status < 0) {
                    rodsLog (LOG_ERROR,
                     "bulkProcAndRegSubfile: chksumLocFile error for %s ",
                      dataObjInfo.filePath);
                    return (status);
                }
                if (strcmp (myChksum, chksumStr) != 0) {
                    rodsLog (LOG_ERROR,
                      "bulkProcAndRegSubfile: chksum of %s %s != input %s",
                        dataObjInfo.filePath, chksumStr, myChksum);
                    return (USER_CHKSUM_MISMATCH);
                }
            }
        }
    }

    status = bulkRegSubfile (rsComm, rescInfo->rescName, rescGroupName,
      subObjPath, dataObjInfo.filePath, dataSize, myDataMode, modFlag,
      dataObjInfo.replNum, myChksum, bulkDataObjRegInp, renamedPhyFiles);

    return status;
}

int
bulkRegSubfile (rsComm_t *rsComm, char *rescName, char *rescGroupName,
char *subObjPath, char *subfilePath, rodsLong_t dataSize, int dataMode,
int modFlag, int replNum, char *chksum, genQueryOut_t *bulkDataObjRegInp,
renamedPhyFiles_t *renamedPhyFiles)
{
    int status;

    /* XXXXXXXX use NULL for chksum for now */
    status = fillBulkDataObjRegInp (rescName, rescGroupName, subObjPath,
      subfilePath, "generic", dataSize, dataMode, modFlag,
      replNum, chksum, bulkDataObjRegInp);
    if (status < 0) {
        rodsLog (LOG_ERROR,
          "bulkRegSubfile: fillBulkDataObjRegInp error for %s. status = %d",
          subfilePath, status);
        return status;
    }

    if (bulkDataObjRegInp->rowCnt >= MAX_NUM_BULK_OPR_FILES) {
        genQueryOut_t *bulkDataObjRegOut = NULL;
        status = rsBulkDataObjReg (rsComm, bulkDataObjRegInp,
          &bulkDataObjRegOut);
        if (status < 0) {
            rodsLog (LOG_ERROR,
              "bulkRegSubfile: rsBulkDataObjReg error for %s. status = %d",
              subfilePath, status);
            cleanupBulkRegFiles (rsComm, bulkDataObjRegInp);
        }
        postProcRenamedPhyFiles (renamedPhyFiles, status);
        postProcBulkPut (rsComm, bulkDataObjRegInp, bulkDataObjRegOut);
        freeGenQueryOut (&bulkDataObjRegOut);
        bulkDataObjRegInp->rowCnt = 0;
    }
    return status;
}

int
addRenamedPhyFile (char *subObjPath, char *oldFileName, char *newFileName,
renamedPhyFiles_t *renamedPhyFiles)
{
    if (subObjPath == NULL || oldFileName == NULL || newFileName == NULL ||
      renamedPhyFiles == NULL) return USER__NULL_INPUT_ERR;

    if (renamedPhyFiles->count >= MAX_NUM_BULK_OPR_FILES) {
        rodsLog (LOG_ERROR,
          "addRenamedPhyFile: count >= %d for %s", MAX_NUM_BULK_OPR_FILES,
          subObjPath);
        return (SYS_RENAME_STRUCT_COUNT_EXCEEDED);
    }
    rstrcpy (&renamedPhyFiles->objPath[renamedPhyFiles->count][0],
      subObjPath, MAX_NAME_LEN);
    rstrcpy (&renamedPhyFiles->origFilePath[renamedPhyFiles->count][0],
      oldFileName, MAX_NAME_LEN);
    rstrcpy (&renamedPhyFiles->newFilePath[renamedPhyFiles->count][0],
      newFileName, MAX_NAME_LEN);
    renamedPhyFiles->count++;
    return 0;
}

int
postProcRenamedPhyFiles (renamedPhyFiles_t *renamedPhyFiles, int regStatus)
{
    int i;
    int status = 0;
    int savedStatus = 0;

    if (renamedPhyFiles == NULL) return USER__NULL_INPUT_ERR;

    if (regStatus >= 0) {
        for (i = 0; i < renamedPhyFiles->count; i++) {
            unlink (&renamedPhyFiles->newFilePath[i][0]);
        }
    } else {
        /* restore the phy files */
        for (i = 0; i < renamedPhyFiles->count; i++) {
            status = rename (&renamedPhyFiles->newFilePath[i][0],
              &renamedPhyFiles->origFilePath[i][0]);
            savedStatus = UNIX_FILE_RENAME_ERR - errno;
            rodsLog (LOG_ERROR,
              "postProcRenamedPhyFiles: rename error from %s to %s, status=%d",
              &renamedPhyFiles->newFilePath[i][0],
              &renamedPhyFiles->origFilePath[i][0], savedStatus);
        }
    }
    bzero (renamedPhyFiles, sizeof (renamedPhyFiles_t));

    return savedStatus;
}

int
cleanupBulkRegFiles (rsComm_t *rsComm, genQueryOut_t *bulkDataObjRegInp)
{
    sqlResult_t *filePath, *rescName;
    char *tmpFilePath, *tmpRescName;
    int i;

    if (bulkDataObjRegInp == NULL) return USER__NULL_INPUT_ERR;

    if ((filePath =
      getSqlResultByInx (bulkDataObjRegInp, COL_D_DATA_PATH)) == NULL) {
        rodsLog (LOG_NOTICE,
          "cleanupBulkRegFiles: getSqlResultByInx for COL_D_DATA_PATH failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }
    if ((rescName =
      getSqlResultByInx (bulkDataObjRegInp, COL_D_RESC_NAME)) == NULL) {
        rodsLog (LOG_NOTICE,
          "rsBulkDataObjReg: getSqlResultByInx for COL_D_RESC_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    for (i = 0;i < bulkDataObjRegInp->rowCnt; i++) {
        tmpFilePath = &filePath->value[filePath->len * i];
        tmpRescName = &rescName->value[rescName->len * i];
        /* make sure it is an orphan file */
        if (chkOrphanFile (rsComm, tmpFilePath, tmpRescName, NULL) > 0) {
            unlink (tmpFilePath);
        }
    }

    return 0;
}

int
postProcBulkPut (rsComm_t *rsComm, genQueryOut_t *bulkDataObjRegInp,
genQueryOut_t *bulkDataObjRegOut)
{
    dataObjInfo_t dataObjInfo;
    sqlResult_t *objPath, *dataType, *dataSize, *rescName, *filePath,
      *dataMode, *oprType, *rescGroupName, *replNum, *chksum;
    char *tmpObjPath, *tmpDataType, *tmpDataSize, *tmpFilePath,
      *tmpDataMode, *tmpOprType, *tmpReplNum, *tmpChksum;
    sqlResult_t *objId;
    char *tmpObjId;
    int status, i;
    dataObjInp_t dataObjInp;
    ruleExecInfo_t rei;
    int savedStatus = 0;

    if (bulkDataObjRegInp == NULL || bulkDataObjRegOut == NULL)
        return USER__NULL_INPUT_ERR;

    initReiWithDataObjInp (&rei, rsComm, NULL);
    status = applyRule ("acBulkPutPostProcPolicy", NULL, &rei, NO_SAVE_REI);
    if (status < 0) {
        rodsLog (LOG_ERROR,
          "postProcBulkPut: acBulkPutPostProcPolicy error status = %d", status);
        return status;
    }

    if (rei.status == POLICY_OFF) return 0;

    if ((objPath =
      getSqlResultByInx (bulkDataObjRegInp, COL_DATA_NAME)) == NULL) {
        rodsLog (LOG_ERROR,
          "postProcBulkPut: getSqlResultByInx for COL_DATA_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((dataType =
      getSqlResultByInx (bulkDataObjRegInp, COL_DATA_TYPE_NAME)) == NULL) {
        rodsLog (LOG_ERROR,
          "postProcBulkPut: getSqlResultByInx for COL_DATA_TYPE_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }
    if ((dataSize =
      getSqlResultByInx (bulkDataObjRegInp, COL_DATA_SIZE)) == NULL) {
        rodsLog (LOG_ERROR,
          "postProcBulkPut: getSqlResultByInx for COL_DATA_SIZE failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }
    if ((rescName =
      getSqlResultByInx (bulkDataObjRegInp, COL_D_RESC_NAME)) == NULL) {
        rodsLog (LOG_ERROR,
          "postProcBulkPut: getSqlResultByInx for COL_D_RESC_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((filePath =
      getSqlResultByInx (bulkDataObjRegInp, COL_D_DATA_PATH)) == NULL) {
        rodsLog (LOG_ERROR,
          "postProcBulkPut: getSqlResultByInx for COL_D_DATA_PATH failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((dataMode =
      getSqlResultByInx (bulkDataObjRegInp, COL_DATA_MODE)) == NULL) {
        rodsLog (LOG_ERROR,
          "postProcBulkPut: getSqlResultByInx for COL_DATA_MODE failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((oprType =
      getSqlResultByInx (bulkDataObjRegInp, OPR_TYPE_INX)) == NULL) {
        rodsLog (LOG_ERROR,
          "postProcBulkPut: getSqlResultByInx for OPR_TYPE_INX failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((rescGroupName =
      getSqlResultByInx (bulkDataObjRegInp, COL_RESC_GROUP_NAME)) == NULL) {
        rodsLog (LOG_ERROR,
          "postProcBulkPut: getSqlResultByInx for COL_RESC_GROUP_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((replNum =
      getSqlResultByInx (bulkDataObjRegInp, COL_DATA_REPL_NUM)) == NULL) {
        rodsLog (LOG_ERROR,
          "postProcBulkPut: getSqlResultByInx for COL_DATA_REPL_NUM failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }
    chksum = getSqlResultByInx (bulkDataObjRegInp, COL_D_DATA_CHECKSUM);

   /* the output */
    if ((objId =
      getSqlResultByInx (bulkDataObjRegOut, COL_D_DATA_ID)) == NULL) {
        rodsLog (LOG_ERROR,
          "postProcBulkPut: getSqlResultByInx for COL_D_DATA_ID failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    /* create a template */
    bzero (&dataObjInfo, sizeof (dataObjInfo_t));
    rstrcpy (dataObjInfo.rescName, rescName->value, NAME_LEN);
    rstrcpy (dataObjInfo.rescGroupName, rescGroupName->value, NAME_LEN);
    dataObjInfo.replStatus = NEWLY_CREATED_COPY;
    status = resolveResc (rescName->value, &dataObjInfo.rescInfo);
    if (status < 0) {
        rodsLog (LOG_ERROR,
          "postProcBulkPut: resolveResc error for %s, status = %d",
          rescName->value, status);
        return (status);
    }
    bzero (&dataObjInp, sizeof (dataObjInp_t));
    dataObjInp.openFlags = O_WRONLY;

    for (i = 0;i < bulkDataObjRegInp->rowCnt; i++) {
        dataObjInfo_t *tmpDataObjInfo;

        tmpDataObjInfo = (dataObjInfo_t *)malloc (sizeof (dataObjInfo_t));
        if (tmpDataObjInfo == NULL) return SYS_MALLOC_ERR;

        *tmpDataObjInfo = dataObjInfo;

        tmpObjPath = &objPath->value[objPath->len * i];
        tmpDataType = &dataType->value[dataType->len * i];
        tmpDataSize = &dataSize->value[dataSize->len * i];
        tmpFilePath = &filePath->value[filePath->len * i];
        tmpDataMode = &dataMode->value[dataMode->len * i];
        tmpOprType = &oprType->value[oprType->len * i];
        tmpReplNum =  &replNum->value[replNum->len * i];
        tmpObjId = &objId->value[objId->len * i];

        rstrcpy (tmpDataObjInfo->objPath, tmpObjPath, MAX_NAME_LEN);
        rstrcpy (dataObjInp.objPath, tmpObjPath, MAX_NAME_LEN);
        rstrcpy (tmpDataObjInfo->dataType, tmpDataType, NAME_LEN);
        tmpDataObjInfo->dataSize = strtoll (tmpDataSize, 0, 0);
#if 0   /* not needed. from dataObjInfo ? */
        rstrcpy (tmpDataObjInfo->rescName, tmpRescName, NAME_LEN);
        rstrcpy (tmpDataObjInfo->rescGroupName, tmpRescGroupName, NAME_LEN);
#endif
        rstrcpy (tmpDataObjInfo->filePath, tmpFilePath, MAX_NAME_LEN);
        rstrcpy (tmpDataObjInfo->dataMode, tmpDataMode, NAME_LEN);
        tmpDataObjInfo->replNum = atoi (tmpReplNum);
        if (chksum != NULL) {
            tmpChksum = &chksum->value[chksum->len * i];
            if (strlen (tmpChksum) > 0) {
                rstrcpy (tmpDataObjInfo->chksum, tmpChksum, NAME_LEN);
            }
        }
        initReiWithDataObjInp (&rei, rsComm, &dataObjInp);
        rei.doi = tmpDataObjInfo;

        status = applyRule ("acPostProcForPut", NULL, &rei, NO_SAVE_REI);
        if (status < 0) savedStatus = status;

        freeAllDataObjInfo (rei.doi);
    }
    return savedStatus;
}

