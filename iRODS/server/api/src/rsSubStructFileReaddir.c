/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "structFileDriver.h"
#include "subStructFileReaddir.h" 
#include "miscServerFunct.h"
#include "dataObjOpr.h"

int
rsSubStructFileReaddir (rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileReaddirInp,
rodsDirent_t **rodsDirent)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&subStructFileReaddirInp->addr, &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        status = _rsSubStructFileReaddir (rsComm, subStructFileReaddirInp, rodsDirent);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteSubStructFileReaddir (rsComm, subStructFileReaddirInp, rodsDirent,
          rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsSubStructFileReaddir: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    return (status);
}

int
remoteSubStructFileReaddir (rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileReaddirInp,
rodsDirent_t **rodsDirent, rodsServerHost_t *rodsServerHost)
{
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteSubStructFileReaddir: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    status = rcSubStructFileReaddir (rodsServerHost->conn, subStructFileReaddirInp,
      rodsDirent);

    if (status < 0 && status != -1) {
        rodsLog (LOG_NOTICE,
         "remoteSubStructFileReaddir: rcFileReaddir failed for fd %d",
         subStructFileReaddirInp->fd);
    }

    return status;

}

int
_rsSubStructFileReaddir (rsComm_t *rsComm, 
subStructFileFdOprInp_t *subStructFileReaddirInp, rodsDirent_t **rodsDirent)
{
    int status;

    status = subStructFileReaddir (subStructFileReaddirInp->type, rsComm, 
      subStructFileReaddirInp->fd, rodsDirent);

    if (status < 0 && status != -1) {
        rodsLog (LOG_NOTICE,
         "_rsSubStructFileReaddir: subStructFileReaddir failed for fd %d",
         subStructFileReaddirInp->fd);
    }

    return (status);
}

