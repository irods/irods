/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsFileSyncToArch.c - server routine that handles the fileSyncToArch
 * API
 */

/* script generated code */
#include "fileSyncToArch.h"
#include "fileOpr.h"
#include "miscServerFunct.h"
#include "dataObjOpr.h"
#include "physPath.h"

int
rsFileSyncToArch (rsComm_t *rsComm, fileStageSyncInp_t *fileSyncToArchInp,
char **outFileName)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&fileSyncToArchInp->addr, &rodsServerHost);

    if (remoteFlag < 0) {
	return (remoteFlag);
    } else {
	status = rsFileSyncToArchByHost (rsComm, fileSyncToArchInp, 
	  outFileName, rodsServerHost);
	return (status);
    }
}

int 
rsFileSyncToArchByHost (rsComm_t *rsComm, 
fileStageSyncInp_t *fileSyncToArchInp, char **outFileName,
rodsServerHost_t *rodsServerHost)
{
    int status;
    int remoteFlag;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
         "rsFileSyncToArchByHost: Input NULL rodsServerHost");
	return (SYS_INTERNAL_NULL_INPUT_ERR);
    }
	    
    remoteFlag = rodsServerHost->localFlag;
    
    if (remoteFlag == LOCAL_HOST) {
	status = _rsFileSyncToArch (rsComm, fileSyncToArchInp, outFileName);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteFileSyncToArch (rsComm, fileSyncToArchInp, 
	  outFileName, rodsServerHost);
    } else {
	if (remoteFlag < 0) {
	    return (remoteFlag);
	} else {
	    rodsLog (LOG_NOTICE,
	      "rsFileSyncToArchByHost: resolveHost returned value %d",
	       remoteFlag);
	    return (SYS_UNRECOGNIZED_REMOTE_FLAG);
	}
    }

    return (status);
}

int
remoteFileSyncToArch (rsComm_t *rsComm, 
fileStageSyncInp_t *fileSyncToArchInp, char **outFileName,
rodsServerHost_t *rodsServerHost)
{
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
	  "remoteFileSyncToArch: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    status = rcFileSyncToArch (rodsServerHost->conn, fileSyncToArchInp,
      outFileName);

    if (status < 0) { 
        rodsLog (LOG_NOTICE,
	 "remoteFileSyncToArch: rcFileSyncToArch failed for %s",
	  fileSyncToArchInp->filename);
    }

    return status;
}

/* _rsFileSyncToArch - this the local version of rsFileSyncToArch.
 */

int
_rsFileSyncToArch (rsComm_t *rsComm, fileStageSyncInp_t *fileSyncToArchInp,
char **outFileName)
{
    int status;
    char myFileName[MAX_NAME_LEN];

    /* XXXX need to check resource permission and vault permission
     * when RCAT is available 
     */

    *outFileName = NULL;
    rstrcpy (myFileName, fileSyncToArchInp->filename, MAX_NAME_LEN);
    status = fileSyncToArch (fileSyncToArchInp->fileType, rsComm, 
      fileSyncToArchInp->cacheFileType, 
      fileSyncToArchInp->mode, fileSyncToArchInp->flags,
      myFileName, fileSyncToArchInp->cacheFilename, 
      fileSyncToArchInp->dataSize, &fileSyncToArchInp->condInput);

    if (status < 0) {
        if (getErrno (status) == ENOENT) {
            /* the directory does not exist */
            mkDirForFilePath (fileSyncToArchInp->fileType, rsComm,
              "/", fileSyncToArchInp->filename, getDefDirMode ());
        } else if (getErrno (status) == EEXIST) {
            /* an empty dir may be there */
            fileRmdir (fileSyncToArchInp->fileType, rsComm,
             fileSyncToArchInp->filename);
	} else {
	    rodsLog (LOG_NOTICE, 
	      "_rsFileSyncToArch: fileSyncToArch for %s, status = %d",
	      fileSyncToArchInp->filename, status);
            return (status);
	}
        status = fileSyncToArch (fileSyncToArchInp->fileType, rsComm,
          fileSyncToArchInp->cacheFileType,
          fileSyncToArchInp->mode, fileSyncToArchInp->flags,
          myFileName, fileSyncToArchInp->cacheFilename,
          fileSyncToArchInp->dataSize, &fileSyncToArchInp->condInput);
	if (status < 0) {
            rodsLog (LOG_NOTICE,
              "_rsFileSyncToArch: fileSyncToArch for %s, status = %d",
              fileSyncToArchInp->filename, status);
        }
    }
    if (strcmp (myFileName, fileSyncToArchInp->filename) != 0) {
	/* file name has changed */
	*outFileName = strdup (myFileName);
    }
    return (status);
} 
 
