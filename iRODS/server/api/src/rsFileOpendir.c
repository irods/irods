/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsFileOpendir.c - server routine that handles the fileOpendir
 * API
 */

/* script generated code */
#include "fileOpendir.h"
#include "miscServerFunct.h"
#include "rsGlobalExtern.h"


int
rsFileOpendir (rsComm_t *rsComm, fileOpendirInp_t *fileOpendirInp)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int fileInx;
    int status;
    void *dirPtr = NULL;

    remoteFlag = resolveHost (&fileOpendirInp->addr, &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
	status = _rsFileOpendir (rsComm, fileOpendirInp, &dirPtr);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteFileOpendir (rsComm, fileOpendirInp, rodsServerHost);
    } else {
	if (remoteFlag < 0) {
	    return (remoteFlag);
	} else {
	    rodsLog (LOG_NOTICE,
	      "rsFileOpendir: resolveHost returned unrecognized value %d",
	       remoteFlag);
	    return (SYS_UNRECOGNIZED_REMOTE_FLAG);
	}
    }

    if (status < 0) {
	return (status);
    }

    fileInx = allocAndFillFileDesc (rodsServerHost, fileOpendirInp->dirName,
      fileOpendirInp->fileType, status, 0);
    FileDesc[fileInx].driverDep = dirPtr;

    return (fileInx);
}

int
remoteFileOpendir (rsComm_t *rsComm, fileOpendirInp_t *fileOpendirInp, 
rodsServerHost_t *rodsServerHost)
{
    int fileInx;
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
	  "remoteFileOpendir: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    fileInx = rcFileOpendir (rodsServerHost->conn, fileOpendirInp);

    if (fileInx < 0) { 
        rodsLog (LOG_NOTICE,
	 "remoteFileOpendir: rcFileOpendir failed for %s",
	  fileOpendirInp->dirName);
    }

    return fileInx;
}

/* _rsFileOpendir - this the local version of rsFileOpendir.
 */

int
_rsFileOpendir (rsComm_t *rsComm, fileOpendirInp_t *fileOpendirInp,
void **dirPtr)
{
    int status;

    /* XXXX need to check resource permission and vault permission
     * when RCAT is available 
     */

    status = fileOpendir (fileOpendirInp->fileType, rsComm, 
      fileOpendirInp->dirName, dirPtr);

    if (status < 0) {
	rodsLog (LOG_NOTICE, 
	  "_rsFileOpendir: fileOpendir for %s, status = %d",
	  fileOpendirInp->dirName, status);
        return (status);
    }

    return (status);
} 
 
