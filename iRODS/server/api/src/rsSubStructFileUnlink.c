/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "structFileDriver.h"
#include "subStructFileUnlink.h" 
#include "miscServerFunct.h"
#include "dataObjOpr.h"

int
rsSubStructFileUnlink (rsComm_t *rsComm, subFile_t *subFile)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&subFile->addr, &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        status = _rsSubStructFileUnlink (rsComm, subFile);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteSubStructFileUnlink (rsComm, subFile, rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsSubStructFileUnlink: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    return (status);
}

int
remoteSubStructFileUnlink (rsComm_t *rsComm, subFile_t *subFile,
rodsServerHost_t *rodsServerHost)
{
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteSubStructFileUnlink: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    status = rcSubStructFileUnlink (rodsServerHost->conn, subFile);

    if (status < 0) {
        rodsLog (LOG_NOTICE,
         "remoteSubStructFileUnlink: rcSubStructFileUnlink failed for %s, status = %d",
          subFile->subFilePath, status);
    }

    return status;
}

int
_rsSubStructFileUnlink (rsComm_t *rsComm, subFile_t *subFile)
{
    int status;

    status = subStructFileUnlink (rsComm, subFile);

    return (status);
}

