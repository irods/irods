/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "structFileDriver.h"
#include "subStructFileRename.h" 
#include "miscServerFunct.h"
#include "dataObjOpr.h"

int
rsSubStructFileRename (rsComm_t *rsComm, subStructFileRenameInp_t *subStructFileRenameInp)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&subStructFileRenameInp->subFile.addr, &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        status = _rsSubStructFileRename (rsComm, subStructFileRenameInp);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteSubStructFileRename (rsComm, subStructFileRenameInp, rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsSubStructFileRename: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    return (status);
}

int
remoteSubStructFileRename (rsComm_t *rsComm, subStructFileRenameInp_t *subStructFileRenameInp,
rodsServerHost_t *rodsServerHost)
{
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteSubStructFileRename: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    status = rcSubStructFileRename (rodsServerHost->conn, subStructFileRenameInp);

    if (status < 0) {
        rodsLog (LOG_NOTICE,
         "remoteSubStructFileRename: rcSubStructFileRename failed for %s, status = %d",
          subStructFileRenameInp->subFile.subFilePath, status);
    }

    return status;
}

int
_rsSubStructFileRename (rsComm_t *rsComm, subStructFileRenameInp_t *subStructFileRenameInp)
{
    int status;

    status = subStructFileRename (rsComm, &subStructFileRenameInp->subFile,
      subStructFileRenameInp->newSubFilePath);

    return (status);
}

