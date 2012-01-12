/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See syncMountedColl.h for a description of this API call.*/

#include "syncMountedColl.h"
#include "rodsLog.h"
#include "icatDefines.h"
#include "objMetaOpr.h"
#include "collection.h"
#include "resource.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"
#include "reGlobalsExtern.h"
#include "miscServerFunct.h"
#include "apiHeaderAll.h"

int
rsSyncMountedColl (rsComm_t *rsComm, dataObjInp_t *syncMountedCollInp)
{
    int status;
    rodsObjStat_t *rodsObjStatOut = NULL;
    dataObjInp_t myDataObjInp;
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;

    status = collStat (rsComm, syncMountedCollInp, &rodsObjStatOut);
    if (status < 0) return status;

    if (rodsObjStatOut->specColl == NULL) {
        freeRodsObjStat (rodsObjStatOut);
        rodsLog (LOG_ERROR,
          "rsSyncMountedColl: %s not a mounted collection", 
	syncMountedCollInp->objPath);
        return (SYS_COLL_NOT_MOUNTED_ERR);
    }
	
    bzero (&myDataObjInp, sizeof (myDataObjInp));
    rstrcpy (myDataObjInp.objPath, rodsObjStatOut->specColl->objPath,
      MAX_NAME_LEN);
    remoteFlag = getAndConnRemoteZone (rsComm, &myDataObjInp, &rodsServerHost,
      REMOTE_OPEN);

    if (remoteFlag < 0) {
        return (remoteFlag);
    } else if (remoteFlag == REMOTE_HOST) {
	status = rcSyncMountedColl (rodsServerHost->conn, 
	  syncMountedCollInp); 
    } else {
        status = _rsSyncMountedColl (rsComm, rodsObjStatOut->specColl,
          syncMountedCollInp->oprType);
    }

    freeRodsObjStat (rodsObjStatOut);

    return (status);
}

int
_rsSyncMountedColl (rsComm_t *rsComm, specColl_t *specColl, int oprType)
{
    int status;

    if (getStructFileType (specColl) >= 0) { 	/* a struct file */
	structFileOprInp_t structFileOprInp;
	rescInfo_t *rescInfo;

	if (strlen (specColl->resource) == 0) {
	    /* nothing to sync */
	    return (0);
	}

        memset (&structFileOprInp, 0, sizeof (structFileOprInp));
        status = resolveResc (specColl->resource, &rescInfo);

        if (status < 0) {
            rodsLog (LOG_NOTICE,
              "_rsSyncMountedColl: resolveResc error for %s, status = %d",
              specColl->resource, status);
            return (status);
        }
        rstrcpy (structFileOprInp.addr.hostAddr, rescInfo->rescLoc, NAME_LEN);
        structFileOprInp.oprType = oprType;
        structFileOprInp.specColl = specColl;
	status = rsStructFileSync (rsComm, &structFileOprInp);
    } else {			/* not a struct file */
	status = SYS_COLL_NOT_MOUNTED_ERR;
    }

    return (status);
}

