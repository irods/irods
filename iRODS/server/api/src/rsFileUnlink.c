/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See fileUnlink.h for a description of this API call.*/

#include "fileUnlink.h"
#include "miscServerFunct.h"

int
rsFileUnlink (rsComm_t *rsComm, fileUnlinkInp_t *fileUnlinkInp)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&fileUnlinkInp->addr, &rodsServerHost);
    if (remoteFlag == LOCAL_HOST) {
        status = _rsFileUnlink (rsComm, fileUnlinkInp);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteFileUnlink (rsComm, fileUnlinkInp, rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsFileUnlink: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

      /* Manually insert call-specific code here */

    return (status);
}

int
remoteFileUnlink (rsComm_t *rsComm, fileUnlinkInp_t *fileUnlinkInp,
rodsServerHost_t *rodsServerHost)
{
    int status;

        if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteFileUnlink: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }


    status = rcFileUnlink (rodsServerHost->conn, fileUnlinkInp);

    if (status < 0) { 
        rodsLog (LOG_NOTICE,
         "remoteFileUnlink: rcFileUnlink failed for %s, status = %d",
          fileUnlinkInp->fileName, status);
    }

    return status;
}

int
_rsFileUnlink (rsComm_t *rsComm, fileUnlinkInp_t *fileUnlinkInp)
{
    int status;

    status = fileUnlink (fileUnlinkInp->fileType, rsComm, 
     fileUnlinkInp->fileName);

    if (status < 0) {
        rodsLog (LOG_NOTICE, 
          "_rsFileUnlink: fileUnlink for %s, status = %d",
          fileUnlinkInp->fileName, status);
        return (status);
    }

    return (status);
} 
