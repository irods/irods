/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "structFileDriver.h"
#include "subStructFileRead.h" 
#include "miscServerFunct.h"
#include "dataObjOpr.h"

int
rsSubStructFileRead (rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileReadInp,
bytesBuf_t *subStructFileReadOutBBuf)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&subStructFileReadInp->addr, &rodsServerHost);

    if (subStructFileReadInp->len > 0) {
        if (subStructFileReadOutBBuf->buf == NULL)
            subStructFileReadOutBBuf->buf = malloc (subStructFileReadInp->len);
    } else {
        return (0);
    }

    if (remoteFlag == LOCAL_HOST) {
        status = _rsSubStructFileRead (rsComm, subStructFileReadInp, subStructFileReadOutBBuf);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteSubStructFileRead (rsComm, subStructFileReadInp, subStructFileReadOutBBuf, 
	  rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsSubStructFileRead: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    return (status);
}

int
remoteSubStructFileRead (rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileReadInp, 
bytesBuf_t *subStructFileReadOutBBuf, rodsServerHost_t *rodsServerHost)
{
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteSubStructFileRead: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    status = rcSubStructFileRead (rodsServerHost->conn, subStructFileReadInp,
      subStructFileReadOutBBuf);

    if (status < 0) {
        rodsLog (LOG_NOTICE,
         "remoteSubStructFileRead: rcFileRead failed for fd %d",  subStructFileReadInp->fd);
    }

    return status;

}

int
_rsSubStructFileRead (rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileReadInp,
bytesBuf_t *subStructFileReadOutBBuf)
{
    int status;

    status =  subStructFileRead (subStructFileReadInp->type, rsComm,
      subStructFileReadInp->fd, subStructFileReadOutBBuf->buf, subStructFileReadInp->len);

    if (status > 0) {
        subStructFileReadOutBBuf->len = status;
    } else {
	subStructFileReadOutBBuf->len = 0;
    }
    return (status);
}

