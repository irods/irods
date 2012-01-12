/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See fileStage.h for a description of this API call.*/

#include "fileStage.h"
#include "miscServerFunct.h"

int
rsFileStage (rsComm_t *rsComm, fileStageInp_t *fileStageInp)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&fileStageInp->addr, &rodsServerHost);
    if (remoteFlag == LOCAL_HOST) {
        status = _rsFileStage (rsComm, fileStageInp);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteFileStage (rsComm, fileStageInp, rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsFileStage: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

      /* Manually insert call-specific code here */

    return (status);
}

int
remoteFileStage (rsComm_t *rsComm, fileStageInp_t *fileStageInp,
rodsServerHost_t *rodsServerHost)
{
    int status;

        if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteFileStage: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }


    status = rcFileStage (rodsServerHost->conn, fileStageInp);

    if (status < 0) { 
        rodsLog (LOG_NOTICE,
         "remoteFileStage: rcFileStage failed for %s, status = %d",
          fileStageInp->fileName, status);
    }

    return status;
}

int
_rsFileStage (rsComm_t *rsComm, fileStageInp_t *fileStageInp)
{
    int status;

    status = fileStage (fileStageInp->fileType, rsComm, 
     fileStageInp->fileName, fileStageInp->flag);

    if (status < 0) {
        rodsLog (LOG_NOTICE, 
          "_rsFileStage: fileStage for %s, status = %d",
          fileStageInp->fileName, status);
        return (status);
    }

    return (status);
} 
