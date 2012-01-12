/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "structFileDriver.h"
#include "subStructFileOpendir.h" 
#include "miscServerFunct.h"
#include "dataObjOpr.h"

int
rsSubStructFileOpendir (rsComm_t *rsComm, subFile_t *subFile)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int fd;

    remoteFlag = resolveHost (&subFile->addr, &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        fd = _rsSubStructFileOpendir (rsComm, subFile);
    } else if (remoteFlag == REMOTE_HOST) {
        fd = remoteSubStructFileOpendir (rsComm, subFile, rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsSubStructFileOpendir: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    return (fd);
}

int
remoteSubStructFileOpendir (rsComm_t *rsComm, subFile_t *subFile,
rodsServerHost_t *rodsServerHost)
{
    int fd;
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteSubStructFileOpendir: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    fd = rcSubStructFileOpendir (rodsServerHost->conn, subFile);

    if (fd < 0) {
        rodsLog (LOG_NOTICE,
         "remoteSubStructFileOpendir: rcSubStructFileOpendir failed for %s, status = %d",
          subFile->subFilePath, fd);
    }

    return fd;
}

int
_rsSubStructFileOpendir (rsComm_t *rsComm, subFile_t *subFile)
{
    int fd;

    fd = subStructFileOpendir (rsComm, subFile);

    return (fd);
}

