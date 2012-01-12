/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "structFileDriver.h"
#include "subStructFileClose.h" 
#include "miscServerFunct.h"
#include "dataObjOpr.h"

int
rsSubStructFileClose (rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileCloseInp)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&subStructFileCloseInp->addr, &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        status = _rsSubStructFileClose (rsComm, subStructFileCloseInp);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteSubStructFileClose (rsComm, subStructFileCloseInp,
          rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsSubStructFileClose: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    return (status);
}

int
remoteSubStructFileClose (rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileCloseInp,
rodsServerHost_t *rodsServerHost)
{
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteSubStructFileClose: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    status = rcSubStructFileClose (rodsServerHost->conn, subStructFileCloseInp);

    if (status < 0) {
        rodsLog (LOG_NOTICE,
         "remoteSubStructFileClose: rcFileClose failed for fd %d",
         subStructFileCloseInp->fd);
    }

    return status;
}

int
_rsSubStructFileClose (rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileCloseInp)
{
    int status;

    status =  subStructFileClose (subStructFileCloseInp->type, rsComm,
      subStructFileCloseInp->fd);

    return (status);
}

