/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "structFileDriver.h"
#include "subStructFileFstat.h" 
#include "miscServerFunct.h"
#include "dataObjOpr.h"

int
rsSubStructFileFstat (rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileFstatInp,
rodsStat_t **subStructFileStatOut)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&subStructFileFstatInp->addr, &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        status = _rsSubStructFileFstat (rsComm, subStructFileFstatInp, subStructFileStatOut);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteSubStructFileFstat (rsComm, subStructFileFstatInp, subStructFileStatOut,
          rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsSubStructFileFstat: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    return (status);
}

int
remoteSubStructFileFstat (rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileFstatInp,
rodsStat_t **subStructFileStatOut, rodsServerHost_t *rodsServerHost)
{
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteSubStructFileFstat: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    status = rcSubStructFileFstat (rodsServerHost->conn, subStructFileFstatInp,
      subStructFileStatOut);

    if (status < 0) {
        rodsLog (LOG_NOTICE,
         "remoteSubStructFileFstat: rcFileFstat failed for fd %d",
         subStructFileFstatInp->fd);
    }

    return status;

}

int
_rsSubStructFileFstat (rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileFstatInp,
rodsStat_t **subStructFileStatOut)
{
    int status;

    status = subStructFileFstat (subStructFileFstatInp->type, rsComm, 
      subStructFileFstatInp->fd, subStructFileStatOut);

    return (status);
}

