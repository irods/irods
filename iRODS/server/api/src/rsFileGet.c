/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See fileGet.h for a description of this API call.*/

#include "fileGet.h"
#include "miscServerFunct.h"
#include "eirods_log.h"

/* rsFileGet - Get the content of a small file into a single buffer
 * in fileGetOutBBuf->buf.
 * Return value - int - number of bytes read.
 */

int
rsFileGet (rsComm_t *rsComm, fileOpenInp_t *fileGetInp, 
bytesBuf_t *fileGetOutBBuf)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&fileGetInp->addr, &rodsServerHost);
    if (remoteFlag == LOCAL_HOST) {
        status = _rsFileGet (rsComm, fileGetInp, fileGetOutBBuf);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteFileGet (rsComm, fileGetInp, fileGetOutBBuf, 
          rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsFileGet: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    return (status);
}

int
remoteFileGet (rsComm_t *rsComm, fileOpenInp_t *fileGetInp,
bytesBuf_t *fileGetOutBBuf, rodsServerHost_t *rodsServerHost)
{    
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteFileGet: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }



    status = rcFileGet (rodsServerHost->conn, fileGetInp, fileGetOutBBuf);

    if (status < 0) { 
        rodsLog (LOG_NOTICE,
         "remoteFileGet: rcFileGet failed for %s",
          fileGetInp->fileName);
    }

    return status;
}

int _rsFileGet (rsComm_t *rsComm, fileOpenInp_t *fileGetInp, bytesBuf_t *fileGetOutBBuf ) {

    int status;
    int fd;
    int len;

    len = fileGetInp->dataSize;
    if (len <= 0)
	return (0);

    fd = _rsFileOpen (rsComm, fileGetInp);

    if (fd < 0) {
        rodsLog (LOG_NOTICE,
          "_rsFileGet: fileGet for %s, status = %d",
          fileGetInp->fileName, fd);
        return (fd);
    }

    if (fileGetOutBBuf->buf == NULL) {
        fileGetOutBBuf->buf = malloc (len);
    }

    eirods::error read_err = fileRead( fileGetInp->fileName, fd, 
	                                   fileGetOutBBuf->buf, len,
							           status );

    if (status != len) {
       if (status >= 0) {
#if 0	/* XXXXXXXX take this out for now so that on the fly convert works */
            rodsLog (LOG_NOTICE,
              "_rsFileGet: fileRead for %s, toread %d, read %d",
              fileGetInp->fileName, len, status);
            status = SYS_COPY_LEN_ERR;
#else
        fileGetOutBBuf->len = status;
#endif
        } else {
			std::stringstream msg;
			msg << "_rsFileGet: fileRead for ";
			msg << fileGetInp->fileName;
			msg << ", status = ";
			msg << status;
			eirods::error ret_err = PASS( false, status, msg.str(), read_err );
			eirods::log( ret_err );
        }
    } else {
        fileGetOutBBuf->len = status;
    }

    // =-=-=-=-=-=-=-
	// call resource plugin close 
    int close_stat;
    eirods::error close_err = fileClose( fileGetInp->fileName, fd, close_stat );
    if( !close_err.ok() ) {
		eirods::error err = PASS( false, close_stat, "_rsFileGet - error on close", close_err );
		eirods::log( err );
	}

    return (status);

} // _rsFileGet 







