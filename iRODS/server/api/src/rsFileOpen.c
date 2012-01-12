/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsFileOpen.c - server routine that handles the fileOpen
 * API
 */

/* script generated code */
#include "fileOpen.h"
#include "fileOpr.h"
#include "miscServerFunct.h"

int
rsFileOpen (rsComm_t *rsComm, fileOpenInp_t *fileOpenInp)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int fileInx;

    remoteFlag = resolveHost (&fileOpenInp->addr, &rodsServerHost);

    if (remoteFlag < 0) {
	return (remoteFlag);
    } else {
	fileInx = rsFileOpenByHost (rsComm, fileOpenInp, rodsServerHost);
	return (fileInx);
    }
}

int 
rsFileOpenByHost (rsComm_t *rsComm, fileOpenInp_t *fileOpenInp,
rodsServerHost_t *rodsServerHost)
{
    int fileInx;
    int fd;
    int remoteFlag;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
         "rsFileOpenByHost: Input NULL rodsServerHost");
	return (SYS_INTERNAL_NULL_INPUT_ERR);
    }
	    
    remoteFlag = rodsServerHost->localFlag;
    
    if (remoteFlag == LOCAL_HOST) {
	fd = _rsFileOpen (rsComm, fileOpenInp);
    } else if (remoteFlag == REMOTE_HOST) {
        fd = remoteFileOpen (rsComm, fileOpenInp, rodsServerHost);
    } else {
	if (remoteFlag < 0) {
	    return (remoteFlag);
	} else {
	    rodsLog (LOG_NOTICE,
	      "rsFileOpenByHost: resolveHost returned unrecognized value %d",
	       remoteFlag);
	    return (SYS_UNRECOGNIZED_REMOTE_FLAG);
	}
    }

    if (fd < 0) {
	return (fd);
    }

    fileInx = allocAndFillFileDesc (rodsServerHost, fileOpenInp->fileName,
      fileOpenInp->fileType, fd, fileOpenInp->mode);

    return (fileInx);
}

int
remoteFileOpen (rsComm_t *rsComm, fileOpenInp_t *fileOpenInp, 
rodsServerHost_t *rodsServerHost)
{
    int fileInx;
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
	  "remoteFileOpen: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    fileInx = rcFileOpen (rodsServerHost->conn, fileOpenInp);

    if (fileInx < 0) { 
        rodsLog (LOG_NOTICE,
	 "remoteFileOpen: rcFileOpen failed for %s",
	  fileOpenInp->fileName);
    }

    return fileInx;
}

/* _rsFileOpen - this the local version of rsFileOpen.
 */

int
_rsFileOpen (rsComm_t *rsComm, fileOpenInp_t *fileOpenInp)
{
    int fd;

    /* XXXX need to check resource permission and vault permission
     * when RCAT is available 
     */

    if ((fileOpenInp->flags & O_WRONLY) && (fileOpenInp->flags & O_RDWR)) {
	/* both O_WRONLY and O_RDWR are on, can cause I/O to fail */
	fileOpenInp->flags &= ~(O_WRONLY);
    }
    fd = fileOpen (fileOpenInp->fileType, rsComm, fileOpenInp->fileName,
     fileOpenInp->flags, fileOpenInp->mode);

    if (fd < 0) {
	rodsLog (LOG_NOTICE, 
	  "_rsFileOpen: fileOpen for %s, status = %d",
	  fileOpenInp->fileName, fd);
        return (fd);
    }

    return (fd);
} 
 
