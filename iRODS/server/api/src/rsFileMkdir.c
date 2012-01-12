/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See fileMkdir.h for a description of this API call.*/

#include "fileMkdir.h"
#include "miscServerFunct.h"

int
rsFileMkdir (rsComm_t *rsComm, fileMkdirInp_t *fileMkdirInp)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&fileMkdirInp->addr, &rodsServerHost);
    if (remoteFlag == LOCAL_HOST) {
        status = _rsFileMkdir (rsComm, fileMkdirInp);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteFileMkdir (rsComm, fileMkdirInp, rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsFileMkdir: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    /* Manually insert call-specific code here */

    return (status);
}

int
remoteFileMkdir (rsComm_t *rsComm, fileMkdirInp_t *fileMkdirInp,
rodsServerHost_t *rodsServerHost)
{    
    int status;

        if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteFileMkdir: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }


    status = rcFileMkdir (rodsServerHost->conn, fileMkdirInp);

    if (status < 0) { 
        rodsLog (LOG_NOTICE,
         "remoteFileOpen: rcFileMkdir failed for %s",
          fileMkdirInp->dirName);
    }

    return status;
}

int
_rsFileMkdir (rsComm_t *rsComm, fileMkdirInp_t *fileMkdirInp)
{
    int status;

    status = fileMkdir (fileMkdirInp->fileType, rsComm, fileMkdirInp->dirName,
     fileMkdirInp->mode);

    if (status < 0) {
	if (getErrno (status) != EEXIST)
            rodsLog (LOG_NOTICE, 
              "_rsFileMkdir: fileMkdir for %s, status = %d",
              fileMkdirInp->dirName, status);
        return (status);
    }

    return (status);
} 
