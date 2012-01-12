/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "structFileDriver.h"
#include "subStructFileStat.h" 
#include "miscServerFunct.h"
#include "dataObjOpr.h"


int
rsSubStructFileStat (rsComm_t *rsComm, subFile_t *subFile, rodsStat_t **subStructFileStatOut)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&subFile->addr, &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        status = _rsSubStructFileStat (rsComm, subFile, subStructFileStatOut);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteSubStructFileStat (rsComm, subFile, subStructFileStatOut,
	  rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsSubStructFileStat: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    return (status);
}

int
remoteSubStructFileStat (rsComm_t *rsComm, subFile_t *subFile,
rodsStat_t **subStructFileStatOut, rodsServerHost_t *rodsServerHost)
{
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteSubStructFileStat: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    status = rcSubStructFileStat (rodsServerHost->conn, subFile, subStructFileStatOut);

    if (status < 0 && getErrno (status) != ENOENT) {
        rodsLog (LOG_NOTICE,
         "remoteSubStructFileStat: rcSubStructFileStat failed for %s, status = %d",
          subFile->subFilePath, status);
    }

    return status;
}

int
_rsSubStructFileStat (rsComm_t *rsComm, subFile_t *subFile, rodsStat_t **subStructFileStatOut)
{
    int status;

    status = subStructFileStat (rsComm, subFile, subStructFileStatOut);
    if (status < 0) {
	*subStructFileStatOut = NULL;
    } 

    return (status);
}

