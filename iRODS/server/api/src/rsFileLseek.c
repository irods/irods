/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See fileLseek.h for a description of this API call.*/

#include "fileLseek.h"
#include "miscServerFunct.h"
#include "rsGlobalExtern.h"

int
rsFileLseek (rsComm_t *rsComm, fileLseekInp_t *fileLseekInp, 
fileLseekOut_t **fileLseekOut)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int retVal;

    *fileLseekOut = NULL;

    remoteFlag = getServerHostByFileInx (fileLseekInp->fileInx, 
      &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        retVal = _rsFileLseek (rsComm, fileLseekInp, fileLseekOut);
    } else if (remoteFlag == REMOTE_HOST) {
        retVal = remoteFileLseek (rsComm, fileLseekInp, 
	  fileLseekOut, rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsFileLseek: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

   /* Manually insert call-specific code here */

    return (retVal);
}

int
remoteFileLseek (rsComm_t *rsComm, fileLseekInp_t *fileLseekInp, 
fileLseekOut_t **fileLseekOut, rodsServerHost_t *rodsServerHost)
{    
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteFileLseek: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    fileLseekInp->fileInx = convL3descInx (fileLseekInp->fileInx);
    status = rcFileLseek (rodsServerHost->conn, fileLseekInp, fileLseekOut);

    if (status < 0) { 
        rodsLog (LOG_NOTICE,
         "remoteFileLseek: rcFileLseek failed for %d, status = %d",
          fileLseekInp->fileInx, status);
    }

    return status;
}

int
_rsFileLseek (rsComm_t *rsComm, fileLseekInp_t *fileLseekInp, 
fileLseekOut_t **fileLseekOut)
{
    int status;
    rodsLong_t lStatus;

    lStatus = fileLseek (FileDesc[fileLseekInp->fileInx].fileType, 
     rsComm, FileDesc[fileLseekInp->fileInx].fd,
     fileLseekInp->offset, fileLseekInp->whence);

    if (lStatus < 0) {
	status = lStatus;
        rodsLog (LOG_NOTICE, 
          "_rsFileLseek: fileLseek failed for %d, status = %d",
          fileLseekInp->fileInx, status);
        return (status);
    } else {
        *fileLseekOut = (fileLseekOut_t*)malloc (sizeof (fileLseekOut_t));
        memset (*fileLseekOut, 0, sizeof (fileLseekOut_t));

	(*fileLseekOut)->offset = lStatus;
	status = 0;
    }

    return (status);
} 
