/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsStructFileExtAndReg.c. See unbunAndRegPhyBunfile.h for a description of 
 * this API call.*/

#include "unbunAndRegPhyBunfile.h"
#include "apiHeaderAll.h"
#include "miscServerFunct.h"
#include "objMetaOpr.h"
#include "resource.h"
#include "dataObjOpr.h"
#include "physPath.h"
#include "rcGlobalExtern.h"
#include "reGlobalsExtern.h"

int
rsUnbunAndRegPhyBunfile (rsComm_t *rsComm, dataObjInp_t *dataObjInp)
{
    int status;
    char *rescName;
    rescGrpInfo_t *rescGrpInfo = NULL;

    if ((rescName = getValByKey (&dataObjInp->condInput, DEST_RESC_NAME_KW)) 
      == NULL) {
        return USER_NO_RESC_INPUT_ERR;
    }

    status = resolveAndQueResc (rescName, NULL, &rescGrpInfo);
    if (status < 0) {
        rodsLog (LOG_NOTICE,
          "rsUnbunAndRegPhyBunfile: resolveAndQueRescerror for %s, status = %d",
          rescName, status);
        return (status);
    }

    status = _rsUnbunAndRegPhyBunfile (rsComm, dataObjInp, 
      rescGrpInfo->rescInfo);

    return (status);
}

int
_rsUnbunAndRegPhyBunfile (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
rescInfo_t *rescInfo)
{
    int status;
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;
    char *bunFilePath;
    char phyBunDir[MAX_NAME_LEN];
    int rmBunCopyFlag;

    remoteFlag = resolveHostByRescInfo (rescInfo, &rodsServerHost);

    if (remoteFlag == REMOTE_HOST) {
        addKeyVal (&dataObjInp->condInput, DEST_RESC_NAME_KW, 
	  rescInfo->rescName);
	status = remoteUnbunAndRegPhyBunfile (rsComm, dataObjInp, 
	  rodsServerHost);
	return status;
    }
    /* process this locally */
    if ((bunFilePath = getValByKey (&dataObjInp->condInput, FILE_PATH_KW))
      == NULL) {
        rodsLog (LOG_ERROR,
          "_rsUnbunAndRegPhyBunfile: No filePath input for %s",
          dataObjInp->objPath);
        return (SYS_INVALID_FILE_PATH);
    }

    createPhyBundleDir (rsComm, bunFilePath, phyBunDir);

    status = unbunPhyBunFile (rsComm, dataObjInp, rescInfo, bunFilePath,
      phyBunDir);

    if (status < 0) {
        rodsLog (LOG_ERROR,
        "_rsUnbunAndRegPhyBunfile:unbunPhyBunFile err for %s to dir %s.stat=%d",
          bunFilePath, phyBunDir, status);
	return status;
    }

    if (getValByKey (&dataObjInp->condInput, RM_BUN_COPY_KW) == NULL) {
	rmBunCopyFlag = 0;
    } else {
	rmBunCopyFlag = 1;
    }

    status = regUnbunPhySubfiles (rsComm, rescInfo, phyBunDir, rmBunCopyFlag);

    if (status == CAT_NO_ROWS_FOUND) {
        /* some subfiles have been deleted. harmless */
        status = 0;
    } else if (status < 0) {
        rodsLog (LOG_ERROR,
          "_rsUnbunAndRegPhyBunfile: regUnbunPhySubfiles for dir %s. stat = %d",
          phyBunDir, status);
    }

    return status;
}

int 
regUnbunPhySubfiles (rsComm_t *rsComm, rescInfo_t *rescInfo, char *phyBunDir,
int rmBunCopyFlag)
{
#ifndef USE_BOOST_FS
    DIR *dirPtr;
    struct dirent *myDirent;
    struct stat statbuf;
#endif  /* USE_BOOST_FS */
    char subfilePath[MAX_NAME_LEN];
    dataObjInp_t dataObjInp;
    dataObjInp_t dataObjUnlinkInp;
    int status;
    int savedStatus = 0;

    dataObjInfo_t *dataObjInfoHead = NULL; 
    dataObjInfo_t *bunDataObjInfo= NULL; 	/* the copy in BUNDLE_RESC */
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
    if (rmBunCopyFlag > 0) {
        bzero (&dataObjUnlinkInp, sizeof (dataObjUnlinkInp));
        addKeyVal (&dataObjUnlinkInp.condInput, IRODS_ADMIN_KW, "");
    }

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
#if 0
        path p (subfilePath);
#endif
	if (!exists(p)) {
#else
        status = stat (subfilePath, &statbuf);
        if (status != 0) {
            closedir (dirPtr);
#endif
            rodsLog (LOG_ERROR,
              "regUnbunphySubfiles: stat error for %s, errno = %d",
              subfilePath, errno);
            return (UNIX_FILE_STAT_ERR - errno);
        }

#ifdef USE_BOOST_FS
	if (!is_regular_file(p)) continue;

        path childPath = p.filename();
        /* do the registration */
        addKeyVal (&dataObjInp.condInput, QUERY_BY_DATA_ID_KW,
          (char *) childPath.c_str ());
#else
        if ((statbuf.st_mode & S_IFREG) == 0) continue;

	/* do the registration */
	addKeyVal (&dataObjInp.condInput, QUERY_BY_DATA_ID_KW, 
	  myDirent->d_name);
#endif
	status = getDataObjInfo (rsComm, &dataObjInp, &dataObjInfoHead,
	  NULL, 1);
       if (status < 0) {
            rodsLog (LOG_DEBUG,
              "regUnbunphySubfiles: getDataObjInfo error for %s, status = %d",
              subfilePath, status);
	    /* don't terminate beause the data Obj may be deleted */
	    unlink (subfilePath);
	    continue;
        }
	requeDataObjInfoByResc (&dataObjInfoHead, BUNDLE_RESC, 1, 1);
	bunDataObjInfo = NULL;
	if (strcmp (dataObjInfoHead->rescName, BUNDLE_RESC) != 0) {
	    /* no match */
	    rodsLog (LOG_DEBUG,
              "regUnbunphySubfiles: No copy in BUNDLE_RESC for %s",
              dataObjInfoHead->objPath);
            /* don't terminate beause the copy may be deleted */
	    unlink (subfilePath);
            continue;
        } else {
	    bunDataObjInfo = dataObjInfoHead;
	}
	requeDataObjInfoByResc (&dataObjInfoHead, rescInfo->rescName, 1, 1);
	/* The copy in DEST_RESC_NAME_KW should be on top */
	if (strcmp (dataObjInfoHead->rescName, rescInfo->rescName) != 0) {
	    /* no copy. stage it */
	    status = regPhySubFile (rsComm, subfilePath, bunDataObjInfo, 
	      rescInfo);
            unlink (subfilePath);
            if (status < 0) {
                rodsLog (LOG_DEBUG,
                  "regUnbunphySubfiles: regPhySubFile err for %s, status = %d",
                  bunDataObjInfo->objPath, status);
	    }
	} else {
	    /* XXXXXX have a copy. don't do anything for now */
            unlink (subfilePath);
	}
	if (rmBunCopyFlag > 0) {
	    rstrcpy (dataObjUnlinkInp.objPath, bunDataObjInfo->objPath, 
	      MAX_NAME_LEN);
	    status = dataObjUnlinkS (rsComm, &dataObjUnlinkInp, bunDataObjInfo);
            if (status < 0) {
                rodsLog (LOG_ERROR,
                  "regUnbunphySubfiles: dataObjUnlinkS err for %s, status = %d",
                  bunDataObjInfo->objPath, status);
		savedStatus = status;
	    }
        }
	freeAllDataObjInfo (dataObjInfoHead);

    }
    clearKeyVal (&dataObjInp.condInput);
#ifndef USE_BOOST_FS
    closedir (dirPtr);
#endif
    if (status >= 0 && savedStatus < 0) {
	return savedStatus;
    } else {
        return status;
    }
}

int
regPhySubFile (rsComm_t *rsComm, char *subfilePath, 
dataObjInfo_t *bunDataObjInfo, rescInfo_t *rescInfo)
{
    dataObjInfo_t stageDataObjInfo;
    dataObjInp_t dataObjInp;
#ifndef USE_BOOST_FS
    struct stat statbuf;
#endif
    int status;
    regReplica_t regReplicaInp;

    bzero (&dataObjInp, sizeof (dataObjInp));
    bzero (&stageDataObjInfo, sizeof (stageDataObjInfo));
    rstrcpy (dataObjInp.objPath, bunDataObjInfo->objPath, MAX_NAME_LEN);
    rstrcpy (stageDataObjInfo.objPath, bunDataObjInfo->objPath, MAX_NAME_LEN);
    rstrcpy (stageDataObjInfo.rescName, rescInfo->rescName, NAME_LEN);
    stageDataObjInfo.rescInfo = rescInfo;

    status = getFilePathName (rsComm, &stageDataObjInfo, &dataObjInp);
    if (status < 0) {
        rodsLog (LOG_ERROR,
          "regPhySubFile: getFilePathName err for %s. status = %d",
          dataObjInp.objPath, status);
        return (status);
    }

#ifdef USE_BOOST_FS
    path p (stageDataObjInfo.filePath);
    if (exists (p)) {
#else
    status = stat (stageDataObjInfo.filePath, &statbuf);
    if (status == 0 || errno != ENOENT) {
#endif
	status = resolveDupFilePath (rsComm, &stageDataObjInfo, &dataObjInp);
        if (status < 0) {
            rodsLog (LOG_ERROR,
              "regPhySubFile: resolveDupFilePath err for %s. status = %d",
              stageDataObjInfo.filePath, status);
            return (status);
	}
    }
    /* make the necessary dir */
    mkDirForFilePath (UNIX_FILE_TYPE, rsComm, "/", stageDataObjInfo.filePath,
      getDefDirMode ());
    /* add a link */
    status = link (subfilePath, stageDataObjInfo.filePath);
    if (status < 0) {
        rodsLog (LOG_ERROR,
          "regPhySubFile: link error %s to %s. errno = %d",
          subfilePath, stageDataObjInfo.filePath, errno);
        return (UNIX_FILE_LINK_ERR - errno);
    }

    bzero (&regReplicaInp, sizeof (regReplicaInp));
    regReplicaInp.srcDataObjInfo = bunDataObjInfo;
    regReplicaInp.destDataObjInfo = &stageDataObjInfo;
    addKeyVal (&regReplicaInp.condInput, SU_CLIENT_USER_KW, "");
    addKeyVal (&regReplicaInp.condInput, IRODS_ADMIN_KW, "");

    status = rsRegReplica (rsComm, &regReplicaInp);

    clearKeyVal (&regReplicaInp.condInput);
    if (status < 0) {
        rodsLog (LOG_ERROR,
          "regPhySubFile: rsRegReplica error for %s. status = %d",
          bunDataObjInfo->objPath, status);
        return status;
    }

    return status;
}

int
unbunPhyBunFile (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
rescInfo_t *rescInfo, char *bunFilePath, char *phyBunDir)
{
    int status;
    structFileOprInp_t structFileOprInp;

    /* untar the bunfile */
    memset (&structFileOprInp, 0, sizeof (structFileOprInp_t));
    structFileOprInp.specColl = (specColl_t*)malloc (sizeof (specColl_t));
    memset (structFileOprInp.specColl, 0, sizeof (specColl_t));
    structFileOprInp.specColl->type = TAR_STRUCT_FILE_T;
    snprintf (structFileOprInp.specColl->collection, MAX_NAME_LEN,
      "%s.dir", dataObjInp->objPath);
    rstrcpy (structFileOprInp.specColl->objPath,
      dataObjInp->objPath, MAX_NAME_LEN);
    structFileOprInp.specColl->collClass = STRUCT_FILE_COLL;
    rstrcpy (structFileOprInp.specColl->resource, rescInfo->rescName,
      NAME_LEN);
    rstrcpy (structFileOprInp.specColl->phyPath, bunFilePath, MAX_NAME_LEN);
    rstrcpy (structFileOprInp.addr.hostAddr, rescInfo->rescLoc,
      NAME_LEN);
    /* set the cacheDir */
    rstrcpy (structFileOprInp.specColl->cacheDir, phyBunDir, MAX_NAME_LEN);

    rmLinkedFilesInUnixDir (phyBunDir);
    status = rsStructFileExtract (rsComm, &structFileOprInp);
    if (status == SYS_DIR_IN_VAULT_NOT_EMPTY) {
	/* phyBunDir is not empty */
	if (chkOrphanDir (rsComm, phyBunDir, rescInfo->rescName) > 0) {
	    /* it is a orphan dir */
	    fileRenameInp_t fileRenameInp;
	    bzero (&fileRenameInp, sizeof (fileRenameInp));
            rstrcpy (fileRenameInp.oldFileName, phyBunDir, MAX_NAME_LEN);
            status = renameFilePathToNewDir (rsComm, ORPHAN_DIR, 
	      &fileRenameInp, rescInfo, 1);

            if (status >= 0) {
                rodsLog (LOG_NOTICE,
                  "unbunPhyBunFile: %s has been moved to ORPHAN_DIR.stat=%d",
                  phyBunDir, status);
		status = rsStructFileExtract (rsComm, &structFileOprInp);
            } else {
        	rodsLog (LOG_ERROR,
        	  "unbunPhyBunFile: renameFilePathToNewDir err for %s.stat=%d",
          	  phyBunDir, status);
		status = SYS_DIR_IN_VAULT_NOT_EMPTY;
            }
	}
    }
    if (status < 0) {
        rodsLog (LOG_ERROR,
          "unbunPhyBunFile: rsStructFileExtract err for %s. status = %d",
          dataObjInp->objPath, status);
    }
    free (structFileOprInp.specColl);

    return (status);
}

int
remoteUnbunAndRegPhyBunfile (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
rodsServerHost_t *rodsServerHost)
{
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteUnbunAndRegPhyBunfile: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    status = rcUnbunAndRegPhyBunfile (rodsServerHost->conn, dataObjInp);

    return status;
}

int
rmLinkedFilesInUnixDir (char *phyBunDir)
{
#ifndef USE_BOOST_FS
    DIR *dirPtr;
    struct dirent *myDirent;
    struct stat statbuf;
#endif
    char subfilePath[MAX_NAME_LEN];
    int status;
    int linkCnt;

#ifdef USE_BOOST_FS
    path srcDirPath (phyBunDir);
    if (!exists(srcDirPath) || !is_directory(srcDirPath)) return 0;
#else
    dirPtr = opendir (phyBunDir);
    if (dirPtr == NULL) return 0;
#endif

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
            continue;
        }

        if (is_regular_file(p)) {
            if ((linkCnt = hard_link_count (p)) >= 2) {
#else
        status = stat (subfilePath, &statbuf);

        if (status != 0) {
	    continue;
        }

        if ((statbuf.st_mode & S_IFREG) != 0) {
	    if ((linkCnt = statbuf.st_nlink) >= 2) {
#endif
		/* only unlink files with multi-links */
	        unlink (subfilePath);
	    } else {
        	rodsLog (LOG_ERROR,
        	  "rmLinkedFilesInUnixDir: st_nlink of %s is only %d",
        	  subfilePath, linkCnt);
	    }
	} else {	/* a directory */
	    status = rmLinkedFilesInUnixDir (subfilePath);
	    /* rm subfilePath but not phyBunDir */
            rmdir (subfilePath);
	}
    }
#ifndef USE_BOOST_FS
    closedir (dirPtr);
#endif
    return 0;
}

