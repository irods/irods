/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "structFileDriver.h"
#include "subStructFileGet.h" 
#include "miscServerFunct.h"
#include "dataObjOpr.h"

int
rsSubStructFileGet (rsComm_t *rsComm, subFile_t *subFile,
bytesBuf_t *subFileGetOutBBuf)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&subFile->addr, &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        status = _rsSubStructFileGet (rsComm, subFile, subFileGetOutBBuf);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteSubStructFileGet (rsComm, subFile, subFileGetOutBBuf,
	  rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsSubStructFileGet: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    return (status);
}

int
remoteSubStructFileGet (rsComm_t *rsComm, subFile_t *subFile,
bytesBuf_t *subFileGetOutBBuf, rodsServerHost_t *rodsServerHost)
{
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteSubStructFileGet: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    status = rcSubStructFileGet (rodsServerHost->conn, subFile, 
      subFileGetOutBBuf);

    if (status < 0) {
        rodsLog (LOG_NOTICE,
         "remoteSubStructFileGet: rcSubStructFileGet failed for %s, status = %d",
          subFile->subFilePath, status);
    }

    return status;
}

int
_rsSubStructFileGet (rsComm_t *rsComm, subFile_t *subFile,
bytesBuf_t *subFileGetOutBBuf)
{
    int status;
    int fd;
    int len;

    len = subFile->offset;
    if (len <= 0)
        return (0);

    fd = subStructFileOpen (rsComm, subFile);

    if (fd < 0) {
        rodsLog (LOG_NOTICE,
          "_rsSubStructFileGet: subStructFileOpen error for %s, status = %d",
          subFile->subFilePath, fd);
        return (fd);
    }

    if (subFileGetOutBBuf->buf == NULL) {
        subFileGetOutBBuf->buf = malloc (len);
    }
    status = subStructFileRead (subFile->specColl->type, rsComm,
      fd, subFileGetOutBBuf->buf, len);

    if (status != len) {
       if (status >= 0) {
            rodsLog (LOG_NOTICE,
              "_rsSubStructFileGet:subStructFileRead for %s,toread %d,read %d",
              subFile->subFilePath, len, status);
            status = SYS_COPY_LEN_ERR;
        } else {
            rodsLog (LOG_NOTICE,
              "_rsSubStructFileGet: subStructFileRead for %s, status = %d",
              subFile->subFilePath, status);
        }
    } else {
        subFileGetOutBBuf->len = status;
    }

    subStructFileClose (subFile->specColl->type, rsComm, fd);

    return (status);
}


