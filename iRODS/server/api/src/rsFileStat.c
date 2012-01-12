/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See fileStat.h for a description of this API call.*/

#include "fileStat.h"
#include "miscServerFunct.h"

int
rsFileStat (rsComm_t *rsComm, fileStatInp_t *fileStatInp, 
rodsStat_t **fileStatOut)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    *fileStatOut = NULL;

    remoteFlag = resolveHost (&fileStatInp->addr, &rodsServerHost);

    if (remoteFlag < 0) {
        return (remoteFlag);
    } else {
        status = rsFileStatByHost (rsComm, fileStatInp, fileStatOut,
	  rodsServerHost);
        return (status);
    }
}

int
rsFileStatByHost (rsComm_t *rsComm, fileStatInp_t *fileStatInp,
rodsStat_t **fileStatOut, rodsServerHost_t *rodsServerHost)
{
    int remoteFlag;
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
         "rsFileStatByHost: Input NULL rodsServerHost");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    remoteFlag = rodsServerHost->localFlag;

    if (remoteFlag == LOCAL_HOST) {
        status = _rsFileStat (rsComm, fileStatInp, fileStatOut);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteFileStat (rsComm, fileStatInp, fileStatOut,
	 rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsFileStat: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    /* Manually insert call-specific code here */

    return (status);
}

int
remoteFileStat (rsComm_t *rsComm, fileStatInp_t *fileStatInp,
rodsStat_t **fileStatOut, rodsServerHost_t *rodsServerHost)
{   
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteFileStat: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }


    status = rcFileStat (rodsServerHost->conn, fileStatInp, fileStatOut);

    if (status < 0) { 
        rodsLog (LOG_DEBUG,
         "remoteFileStat: rcFileStat failed for %s",
          fileStatInp->fileName);
    }

    return status;
}

int
_rsFileStat (rsComm_t *rsComm, fileStatInp_t *fileStatInp,
rodsStat_t **fileStatOut)
{
    int status;
    struct stat myFileStat;

    status = fileStat (fileStatInp->fileType, rsComm, fileStatInp->fileName,
     &myFileStat);

    if (status < 0) {
        rodsLog (LOG_DEBUG, 
          "_rsFileStat: fileStat for %s, status = %d",
          fileStatInp->fileName, status);
        return (status);
    }

    *fileStatOut = (rodsStat_t*)malloc (sizeof (rodsStat_t));

    status = statToRodsStat (*fileStatOut, &myFileStat);

    if (status < 0) {
	free (*fileStatOut);
	*fileStatOut = NULL;
    }

    return (status);
} 
