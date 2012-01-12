/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "structFileDriver.h"
#include "structFileSync.h" 
#include "miscServerFunct.h"
#include "dataObjOpr.h"

int
rsStructFileSync (rsComm_t *rsComm, structFileOprInp_t *structFileOprInp)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&structFileOprInp->addr, &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        status = _rsStructFileSync (rsComm, structFileOprInp);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteStructFileSync (rsComm, structFileOprInp, rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsStructFileSync: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    return (status);
}

int
remoteStructFileSync (rsComm_t *rsComm, structFileOprInp_t *structFileOprInp,
rodsServerHost_t *rodsServerHost)
{
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteStructFileSync: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    status = rcStructFileSync (rodsServerHost->conn, structFileOprInp);

    if (status < 0) {
        rodsLog (LOG_NOTICE,
         "remoteStructFileSync: rcStructFileSync failed for %s, status = %d",
          structFileOprInp->specColl->collection, status);
    }

    return status;
}

int
_rsStructFileSync (rsComm_t *rsComm, structFileOprInp_t *structFileOprInp)
{
    int status;

    status = structFileSync (rsComm, structFileOprInp);

    return (status);
}

