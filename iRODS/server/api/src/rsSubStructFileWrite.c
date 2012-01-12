/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "structFileDriver.h"
#include "subStructFileWrite.h" 
#include "miscServerFunct.h"
#include "dataObjOpr.h"

int
rsSubStructFileWrite (rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileWriteInp,
bytesBuf_t *subStructFileWriteOutBBuf)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&subStructFileWriteInp->addr, &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        status = _rsSubStructFileWrite (rsComm, subStructFileWriteInp, subStructFileWriteOutBBuf);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteSubStructFileWrite (rsComm, subStructFileWriteInp, subStructFileWriteOutBBuf,
          rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsSubStructFileWrite: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    return (status);
}

int
remoteSubStructFileWrite (rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileWriteInp,
bytesBuf_t *subStructFileWriteOutBBuf, rodsServerHost_t *rodsServerHost)
{
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteSubStructFileWrite: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    status = rcSubStructFileWrite (rodsServerHost->conn, subStructFileWriteInp,
      subStructFileWriteOutBBuf);

    if (status < 0) {
        rodsLog (LOG_NOTICE,
         "remoteSubStructFileWrite: rcFileWrite failed for fd %d",  
         subStructFileWriteInp->fd);
    }

    return status;
}

int
_rsSubStructFileWrite (rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileWriteInp,
bytesBuf_t *subStructFileWriteOutBBuf)
{
    int status;

    status =  subStructFileWrite (subStructFileWriteInp->type, rsComm,
      subStructFileWriteInp->fd, subStructFileWriteOutBBuf->buf, subStructFileWriteInp->len);

    return (status);
}

