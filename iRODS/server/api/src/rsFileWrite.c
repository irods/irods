/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsFileWrite.c - server routine that handles the fileWrite
 * API
 */

/* script generated code */
#include "fileWrite.h"
#include "miscServerFunct.h"
#include "rsGlobalExtern.h"

int
rsFileWrite (rsComm_t *rsComm, fileWriteInp_t *fileWriteInp,
bytesBuf_t *fileWriteInpBBuf)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int retVal;


    remoteFlag = getServerHostByFileInx (fileWriteInp->fileInx, 
      &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
	retVal = _rsFileWrite (rsComm, fileWriteInp, fileWriteInpBBuf);
    } else if (remoteFlag == REMOTE_HOST) {
        retVal = remoteFileWrite (rsComm, fileWriteInp, fileWriteInpBBuf,
          rodsServerHost);
    } else {
	if (remoteFlag < 0) {
	    return (remoteFlag);
	} else {
	    rodsLog (LOG_NOTICE,
	      "rsFileWrite: resolveHost returned unrecognized value %d",
	       remoteFlag);
	    return (SYS_UNRECOGNIZED_REMOTE_FLAG);
	}
    }

    if (retVal >= 0) {
	FileDesc[fileWriteInp->fileInx].writtenFlag = 1;
    }

    return (retVal);
}

int
remoteFileWrite (rsComm_t *rsComm, fileWriteInp_t *fileWriteInp, 
bytesBuf_t *fileWriteInpBBuf, rodsServerHost_t *rodsServerHost)
{
    int retVal;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
	  "remoteFileWrite: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((retVal = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return retVal;
    }

    fileWriteInp->fileInx = convL3descInx (fileWriteInp->fileInx);
    retVal = rcFileWrite (rodsServerHost->conn, fileWriteInp, 
      fileWriteInpBBuf);

    if (retVal < 0) { 
        rodsLog (LOG_NOTICE,
	 "remoteFileWrite: rcFileWrite failed for %s",
	  FileDesc[fileWriteInp->fileInx].fileName);
    }

    return retVal;
}

/* _rsFileWrite - this the local version of rsFileWrite.
 */

int
_rsFileWrite (rsComm_t *rsComm, fileWriteInp_t *fileWriteInp,
bytesBuf_t *fileWriteInpBBuf)
{
    int retVal;

    /* XXXX need to check resource permission and vault permission
     * when RCAT is available 
     */

    retVal = fileWrite (FileDesc[fileWriteInp->fileInx].fileType, rsComm, 
      FileDesc[fileWriteInp->fileInx].fd, fileWriteInpBBuf->buf,
      fileWriteInp->len);

    if (retVal < 0) {
	rodsLog (LOG_NOTICE, 
	  "_rsFileWrite: fileWrite for %s, status = %d",
	  FileDesc[fileWriteInp->fileInx].fileName, retVal);
        return (retVal);
    }

    return (retVal);
} 
 
