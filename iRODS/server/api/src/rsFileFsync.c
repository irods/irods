/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See fileFsync.h for a description of this API call.*/

#include "fileFsync.h"
#include "miscServerFunct.h"
#include "rsGlobalExtern.h"

int
rsFileFsync (rsComm_t *rsComm, fileFsyncInp_t *fileFsyncInp)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int retVal;

    remoteFlag = getServerHostByFileInx (fileFsyncInp->fileInx, 
      &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        retVal = _rsFileFsync (rsComm, fileFsyncInp);
    } else if (remoteFlag == REMOTE_HOST) {
        retVal = remoteFileFsync (rsComm, fileFsyncInp, rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsFileFsync: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

   /* Manually insert call-specific code here */

    return (retVal);
}

int
remoteFileFsync (rsComm_t *rsComm, fileFsyncInp_t *fileFsyncInp,
rodsServerHost_t *rodsServerHost)
{    
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteFileFsync: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    fileFsyncInp->fileInx = convL3descInx (fileFsyncInp->fileInx);
    status = rcFileFsync (rodsServerHost->conn, fileFsyncInp);

    if (status < 0) { 
        rodsLog (LOG_NOTICE,
         "remoteFileFsync: rcFileFsync failed for %d, status = %d",
          fileFsyncInp->fileInx, status);
    }

    return status;
}

int
_rsFileFsync (rsComm_t *rsComm, fileFsyncInp_t *fileFsyncInp)
{
    int status;

    status = fileFsync (FileDesc[fileFsyncInp->fileInx].fileType, 
     rsComm, FileDesc[fileFsyncInp->fileInx].fd);

    if (status < 0) {
        rodsLog (LOG_NOTICE, 
          "_rsFileFsync: fileFsync failed for %d, status = %d",
          fileFsyncInp->fileInx, status);
        return (status);
    }

    return (status);
} 
