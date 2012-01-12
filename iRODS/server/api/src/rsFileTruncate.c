/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See fileTruncate.h for a description of this API call.*/

#include "fileTruncate.h"
#include "miscServerFunct.h"

int
rsFileTruncate (rsComm_t *rsComm, fileOpenInp_t *fileTruncateInp)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&fileTruncateInp->addr, &rodsServerHost);
    if (remoteFlag == LOCAL_HOST) {
        status = _rsFileTruncate (rsComm, fileTruncateInp);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteFileTruncate (rsComm, fileTruncateInp, rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsFileTruncate: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

      /* Manually insert call-specific code here */

    return (status);
}

int
remoteFileTruncate (rsComm_t *rsComm, fileOpenInp_t *fileTruncateInp,
rodsServerHost_t *rodsServerHost)
{
    int status;

        if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteFileTruncate: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }


    status = rcFileTruncate (rodsServerHost->conn, fileTruncateInp);

    if (status < 0) { 
        rodsLog (LOG_NOTICE,
         "remoteFileTruncate: rcFileTruncate failed for %s, status = %d",
          fileTruncateInp->fileName, status);
    }

    return status;
}

int
_rsFileTruncate (rsComm_t *rsComm, fileOpenInp_t *fileTruncateInp)
{
    int status;

    status = fileTruncate (fileTruncateInp->fileType, rsComm, 
     fileTruncateInp->fileName, fileTruncateInp->dataSize);

    if (status < 0) {
        rodsLog (LOG_NOTICE, 
          "_rsFileTruncate: fileTruncate for %s, status = %d",
          fileTruncateInp->fileName, status);
        return (status);
    }

    return (status);
} 
