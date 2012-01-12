/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsFileCreate.c - server routine that handles the fileCreate
 * API
 */

/* script generated code */
#include "fileCreate.h"
#include "fileOpr.h"
#include "miscServerFunct.h"
#include "dataObjOpr.h"
#include "physPath.h"

int
rsFileCreate (rsComm_t *rsComm, fileCreateInp_t *fileCreateInp)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int fileInx;
    int fd;

    remoteFlag = resolveHost (&fileCreateInp->addr, &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
	fd = _rsFileCreate (rsComm, fileCreateInp, rodsServerHost);
    } else if (remoteFlag == REMOTE_HOST) {
        fd = remoteFileCreate (rsComm, fileCreateInp, rodsServerHost);
    } else {
	if (remoteFlag < 0) {
	    return (remoteFlag);
	} else {
	    rodsLog (LOG_NOTICE,
	      "rsFileCreate: resolveHost returned unrecognized value %d",
	       remoteFlag);
	    return (SYS_UNRECOGNIZED_REMOTE_FLAG);
	}
    }

    if (fd < 0) {
	return (fd);
    }

    fileInx = allocAndFillFileDesc (rodsServerHost, fileCreateInp->fileName,
      fileCreateInp->fileType, fd, 
      fileCreateInp->mode);

    return (fileInx);
}

int
remoteFileCreate (rsComm_t *rsComm, fileCreateInp_t *fileCreateInp, 
rodsServerHost_t *rodsServerHost)
{
    int fileInx;
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
	  "remoteFileCreate: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    fileInx = rcFileCreate (rodsServerHost->conn, fileCreateInp);

    if (fileInx < 0) { 
        rodsLog (LOG_NOTICE,
	 "remoteFileCreate: rcFileCreate failed for %s",
	  fileCreateInp->fileName);
    }

    return fileInx;
}

/* _rsFileCreate - this the local version of rsFileCreate.
 */

int
_rsFileCreate (rsComm_t *rsComm, fileCreateInp_t *fileCreateInp,
rodsServerHost_t *rodsServerHost)
{
    int fd;
    int status;

    /* XXXX need to check resource permission and vault permission
     * when RCAT is available 
     */

    if (fileCreateInp->otherFlags & CHK_PERM_FLAG) {
	status = chkFilePathPerm (rsComm, fileCreateInp, 
	  rodsServerHost);
	if (status < 0) {
	    return (status);
	}
    }

    fd = fileCreate (fileCreateInp->fileType, rsComm, fileCreateInp->fileName,
     fileCreateInp->mode, fileCreateInp->dataSize);

    if (fd < 0) {
	if (getErrno (fd) == ENOENT) {
	    /* the directory does not exist */
	    mkDirForFilePath (fileCreateInp->fileType, rsComm, 
	      "/", fileCreateInp->fileName, getDefDirMode ()); 
	    fd = fileCreate (fileCreateInp->fileType, rsComm, 
	      fileCreateInp->fileName, fileCreateInp->mode, 
	      fileCreateInp->dataSize);
	    if (fd < 0) {
		if (getErrno (fd) == EEXIST) {
                    rodsLog (LOG_DEBUG1,
                    "_rsFileCreate: fileCreate for %s, status = %d",
                      fileCreateInp->fileName, fd);
	        } else {
        	    rodsLog (LOG_NOTICE,
          	    "_rsFileCreate: fileCreate for %s, status = %d",
        	      fileCreateInp->fileName, fd);
		}
	    }
	} else if (getErrno (fd) == EEXIST) {
	    /* an empty dir may be there */ 
	    fileRmdir (fileCreateInp->fileType, rsComm, 
	     fileCreateInp->fileName);
            fd = fileCreate (fileCreateInp->fileType, rsComm,
              fileCreateInp->fileName, fileCreateInp->mode,
              fileCreateInp->dataSize);
	} else {
            if (getErrno (fd) == EEXIST) {
                rodsLog (LOG_DEBUG1,
                "_rsFileCreate: fileCreate for %s, status = %d",
                  fileCreateInp->fileName, fd);
            } else {
                rodsLog (LOG_NOTICE,
                "_rsFileCreate: fileCreate for %s, status = %d",
                  fileCreateInp->fileName, fd);
            }
	}
    }
    return (fd);
} 
 
