/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See fileGetFsFreeSpace.h for a description of this API call.*/

#include "fileGetFsFreeSpace.h"
#include "miscServerFunct.h"

int
rsFileGetFsFreeSpace (rsComm_t *rsComm, 
fileGetFsFreeSpaceInp_t *fileGetFsFreeSpaceInp,
fileGetFsFreeSpaceOut_t **fileGetFsFreeSpaceOut)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    *fileGetFsFreeSpaceOut = NULL;

    remoteFlag = resolveHost (&fileGetFsFreeSpaceInp->addr, &rodsServerHost);
    if (remoteFlag == LOCAL_HOST) {
        status = _rsFileGetFsFreeSpace (rsComm, fileGetFsFreeSpaceInp,
	  fileGetFsFreeSpaceOut);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteFileGetFsFreeSpace (rsComm, fileGetFsFreeSpaceInp,
	  fileGetFsFreeSpaceOut, rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsFileGetFsFreeSpace: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

      /* Manually insert call-specific code here */

    return (status);
}

int
remoteFileGetFsFreeSpace (rsComm_t *rsComm, 
fileGetFsFreeSpaceInp_t *fileGetFsFreeSpaceInp,
fileGetFsFreeSpaceOut_t **fileGetFsFreeSpaceOut,
rodsServerHost_t *rodsServerHost)
{
    int status;

        if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteFileGetFsFreeSpace: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }


    status = rcFileGetFsFreeSpace (rodsServerHost->conn, fileGetFsFreeSpaceInp,
      fileGetFsFreeSpaceOut);

    if (status < 0) { 
        rodsLog (LOG_NOTICE,
         "remoteFileGetFsFreeSpace: rcFileGetFsFreeSpace failed for %s, status = %d",
          fileGetFsFreeSpaceInp->fileName, status);
    }

    return status;
}

int
_rsFileGetFsFreeSpace (rsComm_t *rsComm, 
fileGetFsFreeSpaceInp_t *fileGetFsFreeSpaceInp,
fileGetFsFreeSpaceOut_t **fileGetFsFreeSpaceOut)
{
    rodsLong_t status;

    status = fileGetFsFreeSpace (fileGetFsFreeSpaceInp->fileType, rsComm, 
     fileGetFsFreeSpaceInp->fileName, fileGetFsFreeSpaceInp->flag);

    if (status < 0) {
        rodsLog (LOG_NOTICE, 
          "_rsFileGetFsFreeSpace: fileGetFsFreeSpace for %s, status = %lld",
          fileGetFsFreeSpaceInp->fileName, status);
        return ((int) status);
    }

    *fileGetFsFreeSpaceOut = (fileGetFsFreeSpaceOut_t*)malloc (sizeof (fileGetFsFreeSpaceOut_t));
    (*fileGetFsFreeSpaceOut)->size = status;

    return (0);
} 
