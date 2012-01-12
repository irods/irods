/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsStructFileBundle.c. See structFileBundle.h for a description of 
 * this API call.*/

#include "structFileBundle.h"
#include "apiHeaderAll.h"
#include "objMetaOpr.h"
#include "dataObjOpr.h"
#include "physPath.h"
#include "miscServerFunct.h"
#include "rcGlobalExtern.h"
#include "reGlobalsExtern.h"

int
rsStructFileBundle (rsComm_t *rsComm,
structFileExtAndRegInp_t *structFileBundleInp)
{
#if 0
    char *destRescName = NULL;
#endif
    int status;
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    rodsHostAddr_t rescAddr;
    dataObjInp_t dataObjInp;
    rescGrpInfo_t *rescGrpInfo = NULL;

    memset (&dataObjInp, 0, sizeof (dataObjInp));
    rstrcpy (dataObjInp.objPath, structFileBundleInp->objPath,
      MAX_NAME_LEN);

    remoteFlag = getAndConnRemoteZone (rsComm, &dataObjInp, &rodsServerHost,
      REMOTE_CREATE);

    if (remoteFlag < 0) {
        return (remoteFlag);
    } else if (remoteFlag == REMOTE_HOST) {
        status = rcStructFileBundle (rodsServerHost->conn,
          structFileBundleInp);
        return status;
    }

#if 0
    if ((destRescName = 
     getValByKey (&structFileBundleInp->condInput, DEST_RESC_NAME_KW)) == NULL 
     && (destRescName = 
     getValByKey (&structFileBundleInp->condInput, DEF_RESC_NAME_KW)) == NULL) {
        return USER_NO_RESC_INPUT_ERR;
    }

    status = _getRescInfo (rsComm, destRescName, &rescGrpInfo);
    if (status < 0) {
         rodsLog (LOG_ERROR,
          "rsStructFileBundle: _getRescInfo of %s error for %s. stat = %d",
          destRescName, structFileBundleInp->collection, status);
        return status;
    }
#else
    /* borrow conInput */
    dataObjInp.condInput = structFileBundleInp->condInput;
    status = getRescGrpForCreate (rsComm, &dataObjInp, &rescGrpInfo);
    if (status < 0) return status;
#endif

    bzero (&rescAddr, sizeof (rescAddr));
    rstrcpy (rescAddr.hostAddr, rescGrpInfo->rescInfo->rescLoc, NAME_LEN);
    remoteFlag = resolveHost (&rescAddr, &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        status = _rsStructFileBundle (rsComm, structFileBundleInp);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteStructFileBundle (rsComm, structFileBundleInp, 
	  rodsServerHost);
    } else if (remoteFlag < 0) {
            status = remoteFlag;
    }
    freeAllRescGrpInfo (rescGrpInfo);
    return status;
}

int
_rsStructFileBundle (rsComm_t *rsComm,
structFileExtAndRegInp_t *structFileBundleInp)
{
    int status;
    dataObjInp_t dataObjInp;
    openedDataObjInp_t dataObjCloseInp;
    collInp_t collInp;
    collEnt_t *collEnt = NULL;
    int handleInx;
    int collLen;
    char phyBunDir[MAX_NAME_LEN];
    char tmpPath[MAX_NAME_LEN];
    chkObjPermAndStat_t chkObjPermAndStatInp;
    int l1descInx;
    int savedStatus = 0;

    /* open the structured file */
    memset (&dataObjInp, 0, sizeof (dataObjInp));
    rstrcpy (dataObjInp.objPath, structFileBundleInp->objPath, 
      MAX_NAME_LEN);
 
    /* replicate the condInput. may have resource input */
    replKeyVal (&structFileBundleInp->condInput, &dataObjInp.condInput);

    dataObjInp.openFlags = O_WRONLY;  

    l1descInx = rsDataObjCreate (rsComm, &dataObjInp);
    if (l1descInx < 0) {
        rodsLog (LOG_ERROR,
          "rsStructFileBundle: rsDataObjCreate of %s error. status = %d",
          dataObjInp.objPath, l1descInx);
        return (l1descInx);
    }
    l3Close (rsComm, l1descInx);
    L1desc[l1descInx].l3descInx = 0;

    memset (&chkObjPermAndStatInp, 0, sizeof (chkObjPermAndStatInp));
    rstrcpy (chkObjPermAndStatInp.objPath, 
      structFileBundleInp->collection, MAX_NAME_LEN); 
    chkObjPermAndStatInp.flags = CHK_COLL_FOR_BUNDLE_OPR;
    addKeyVal (&chkObjPermAndStatInp.condInput, RESC_NAME_KW,
      L1desc[l1descInx].dataObjInfo->rescName);

    status = rsChkObjPermAndStat (rsComm, &chkObjPermAndStatInp);

    clearKeyVal (&chkObjPermAndStatInp.condInput);

    if (status < 0) {
        rodsLog (LOG_ERROR,
          "rsStructFileBundle: rsChkObjPermAndStat of %s error. stat = %d",
          chkObjPermAndStatInp.objPath, status);
        dataObjCloseInp.l1descInx = l1descInx;
        rsDataObjClose (rsComm, &dataObjCloseInp);
        return (status);
    }

    createPhyBundleDir (rsComm, L1desc[l1descInx].dataObjInfo->filePath,
      phyBunDir);

    bzero (&collInp, sizeof (collInp));
    rstrcpy (collInp.collName, structFileBundleInp->collection, MAX_NAME_LEN);
    collInp.flags = RECUR_QUERY_FG | VERY_LONG_METADATA_FG |
      NO_TRIM_REPL_FG | INCLUDE_CONDINPUT_IN_QUERY;
    addKeyVal (&collInp.condInput, RESC_NAME_KW, 
      L1desc[l1descInx].dataObjInfo->rescName);
    handleInx = rsOpenCollection (rsComm, &collInp);
    if (handleInx < 0) {
        rodsLog (LOG_ERROR,
          "rsStructFileBundle: rsOpenCollection of %s error. status = %d",
          collInp.collName, handleInx);
        rmdir (phyBunDir);
        return (handleInx);
    }
    collLen = strlen (collInp.collName);

    while ((status = rsReadCollection (rsComm, &handleInx, &collEnt)) >= 0) {
        if (collEnt->objType == DATA_OBJ_T) {
            if (collEnt->collName[collLen] == '\0') {
                snprintf (tmpPath, MAX_NAME_LEN, "%s/%s",
                  phyBunDir, collEnt->dataName);
            } else {
                snprintf (tmpPath, MAX_NAME_LEN, "%s/%s/%s",
                 phyBunDir, collEnt->collName + collLen + 1, collEnt->dataName);               
	        mkDirForFilePath (UNIX_FILE_TYPE, rsComm, phyBunDir, 
	          tmpPath, getDefDirMode ());
            }
            /* add a link */
            status = link (collEnt->phyPath, tmpPath);
            if (status < 0) {
                rodsLog (LOG_ERROR,
                  "rsStructFileBundle: link error %s to %s. errno = %d",
                  collEnt->phyPath, tmpPath, errno);
                rmLinkedFilesInUnixDir (phyBunDir);
                rmdir (phyBunDir);
                return (UNIX_FILE_LINK_ERR - errno);
            }
        } else {        /* a collection */
	    if ((int) strlen (collEnt->collName) + 1 <= collLen) {
		free (collEnt);
		continue;
	    }
            snprintf (tmpPath, MAX_NAME_LEN, "%s/%s",
              phyBunDir, collEnt->collName + collLen);
            mkdirR (phyBunDir, tmpPath, getDefDirMode ());
	}
	if (collEnt != NULL) {
	    free (collEnt);
	    collEnt = NULL;
	}
    }
    clearKeyVal (&collInp.condInput);
    rsCloseCollection (rsComm, &handleInx);

    status = phyBundle (rsComm, L1desc[l1descInx].dataObjInfo, phyBunDir,
      collInp.collName);
    if (status < 0) {
        rodsLog (LOG_ERROR,
          "rsStructFileBundle: phyBundle of %s error. stat = %d",
          L1desc[l1descInx].dataObjInfo->objPath, status);
	L1desc[l1descInx].bytesWritten = 0;
	savedStatus = status;
    } else {
        /* mark it was written so the size would be adjusted */
        L1desc[l1descInx].bytesWritten = 1;
    }

    rmLinkedFilesInUnixDir (phyBunDir);
    rmdir (phyBunDir);

    dataObjCloseInp.l1descInx = l1descInx;
    status = rsDataObjClose (rsComm, &dataObjCloseInp);

    if (status >= 0)
	return savedStatus;
    else
        return (status);
}

int
remoteStructFileBundle (rsComm_t *rsComm,
structFileExtAndRegInp_t *structFileBundleInp, rodsServerHost_t *rodsServerHost)
{
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteStructFileBundle: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    status = rcStructFileBundle (rodsServerHost->conn, structFileBundleInp);
    return status;
}

