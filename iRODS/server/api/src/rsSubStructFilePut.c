/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "structFileDriver.h"
#include "subStructFilePut.h" 
#include "miscServerFunct.h"
#include "dataObjOpr.h"

int
rsSubStructFilePut (rsComm_t *rsComm, subFile_t *subFile,
bytesBuf_t *subFilePutOutBBuf)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&subFile->addr, &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        status = _rsSubStructFilePut (rsComm, subFile, subFilePutOutBBuf);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteSubStructFilePut (rsComm, subFile, subFilePutOutBBuf,
	  rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsSubStructFilePut: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    return (status);
}

int
remoteSubStructFilePut (rsComm_t *rsComm, subFile_t *subFile,
bytesBuf_t *subFilePutOutBBuf, rodsServerHost_t *rodsServerHost)
{
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteSubStructFilePut: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    status = rcSubStructFilePut (rodsServerHost->conn, subFile, 
      subFilePutOutBBuf);

    if (status < 0) {
        rodsLog (LOG_NOTICE,
         "remoteSubStructFilePut: rcSubStructFilePut failed for %s, status = %d",
          subFile->subFilePath, status);
    }

    return status;
}

int
_rsSubStructFilePut (rsComm_t *rsComm, subFile_t *subFile,
bytesBuf_t *subFilePutOutBBuf)
{
    int status;
    int fd;

    if (subFile->flags & FORCE_FLAG) {
        fd = subStructFileOpen (rsComm, subFile);
    } else {
        fd = subStructFileCreate (rsComm, subFile);
    }

    if (fd < 0) {
       if (getErrno (fd) == EEXIST) {
            rodsLog (LOG_DEBUG1,
              "_rsSubStructFilePut: filePut for %s, status = %d",
              subFile->subFilePath, fd);
        } else {
            rodsLog (LOG_NOTICE,
              "_rsSubStructFilePut: subStructFileOpen error for %s, stat=%d",
              subFile->subFilePath, fd);
	}
        return (fd);
    }

    status = subStructFileWrite (subFile->specColl->type, rsComm,
      fd, subFilePutOutBBuf->buf, subFilePutOutBBuf->len);

    if (status != subFilePutOutBBuf->len) {
       if (status >= 0) {
            rodsLog (LOG_NOTICE,
              "_rsSubStructFilePut:Write error for %s,towrite %d,read %d",
              subFile->subFilePath, subFilePutOutBBuf->len, status);
            status = SYS_COPY_LEN_ERR;
        } else {
            rodsLog (LOG_NOTICE,
              "_rsSubStructFilePut: Write error for %s, status = %d",
              subFile->subFilePath, status);
        }
    }

    subStructFileClose (subFile->specColl->type, rsComm, fd);

    return (status);
}

