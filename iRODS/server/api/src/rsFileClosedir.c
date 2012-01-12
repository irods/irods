/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See fileClosedir.h for a description of this API call.*/

#include "fileClosedir.h"
#include "miscServerFunct.h"
#include "rsGlobalExtern.h"

int
rsFileClosedir (rsComm_t *rsComm, fileClosedirInp_t *fileClosedirInp)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int retVal;

    remoteFlag = getServerHostByFileInx (fileClosedirInp->fileInx, 
      &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        retVal = _rsFileClosedir (rsComm, fileClosedirInp);
    } else if (remoteFlag == REMOTE_HOST) {
        retVal = remoteFileClosedir (rsComm, fileClosedirInp, rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsFileClosedir: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

   /* Manually insert call-specific code here */

    freeFileDesc (fileClosedirInp->fileInx);

    return (retVal);
}

int
remoteFileClosedir (rsComm_t *rsComm, fileClosedirInp_t *fileClosedirInp,
rodsServerHost_t *rodsServerHost)
{    
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteFileClosedir: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }


    status = rcFileClosedir (rodsServerHost->conn, fileClosedirInp);

    if (status < 0) { 
        rodsLog (LOG_NOTICE,
         "remoteFileClosedir: rcFileClosedir failed for %d, status = %d",
          fileClosedirInp->fileInx, status);
    }

    return status;
}

int
_rsFileClosedir (rsComm_t *rsComm, fileClosedirInp_t *fileClosedirInp)
{
    int status;

    status = fileClosedir (FileDesc[fileClosedirInp->fileInx].fileType, 
     rsComm, FileDesc[fileClosedirInp->fileInx].driverDep);

    if (status < 0) {
        rodsLog (LOG_NOTICE, 
          "_rsFileClosedir: fileClosedir failed for %d, status = %d",
          fileClosedirInp->fileInx, status);
        return (status);
    }

    return (status);
} 
