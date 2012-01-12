/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "structFileDriver.h"
#include "subStructFileLseek.h" 
#include "miscServerFunct.h"
#include "dataObjOpr.h"

int
rsSubStructFileLseek (rsComm_t *rsComm, subStructFileLseekInp_t *subStructFileLseekInp,
fileLseekOut_t **subStructFileLseekOut)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&subStructFileLseekInp->addr, &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        status = _rsSubStructFileLseek (rsComm, subStructFileLseekInp, subStructFileLseekOut);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteSubStructFileLseek (rsComm, subStructFileLseekInp, subStructFileLseekOut,
          rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsSubStructFileLseek: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    return (status);
}

int
remoteSubStructFileLseek (rsComm_t *rsComm, subStructFileLseekInp_t *subStructFileLseekInp,
fileLseekOut_t **subStructFileLseekOut, rodsServerHost_t *rodsServerHost)
{
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteSubStructFileLseek: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    status = rcSubStructFileLseek (rodsServerHost->conn, subStructFileLseekInp,
      subStructFileLseekOut);

    if (status < 0) {
        rodsLog (LOG_NOTICE,
         "remoteSubStructFileLseek: rcFileLseek failed for fd %d",
         subStructFileLseekInp->fd);
    }

    return status;
}

int
_rsSubStructFileLseek (rsComm_t *rsComm, subStructFileLseekInp_t *subStructFileLseekInp,
fileLseekOut_t **subStructFileLseekOut)
{
    rodsLong_t lStatus;
    int status;

    *subStructFileLseekOut = (fileLseekOut_t *) malloc (sizeof (fileLseekOut_t));
    memset (*subStructFileLseekOut, 0, sizeof (fileLseekOut_t));
    lStatus = subStructFileLseek (subStructFileLseekInp->type, rsComm, subStructFileLseekInp->fd,
      subStructFileLseekInp->offset, subStructFileLseekInp->whence);

    if (lStatus < 0) {
        status = lStatus;
        rodsLog (LOG_ERROR,
          "rsSubStructFileLseek: subStructFileLseek failed for %d, status = %d",
          subStructFileLseekInp->fd, status);
        return (status);
    } else {
        *subStructFileLseekOut = (fileLseekOut_t *) malloc (sizeof (fileLseekOut_t));
        memset (*subStructFileLseekOut, 0, sizeof (fileLseekOut_t));
        (*subStructFileLseekOut)->offset = lStatus;
        status = 0;
    }

    return (status);
}

