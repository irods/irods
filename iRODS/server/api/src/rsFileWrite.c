/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsFileWrite.c - server routine that handles the fileWrite
 * API
 */

/* script generated code */
#include "fileWrite.h"
#include "miscServerFunct.h"
#include "rsGlobalExtern.h"
#include <sstream>

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_log.h"
#include "eirods_file_object.h"

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

// =-=-=-=-=-=-=-
// _rsFileWrite - this the local version of rsFileWrite.
int _rsFileWrite( rsComm_t *rsComm, fileWriteInp_t *fileWriteInp, bytesBuf_t *fileWriteInpBBuf ) {
    // =-=-=-=-=-=-=-
    // XXXX need to check resource permission and vault permission
    // when RCAT is available 

    // =-=-=-=-=-=-=-
    // make a call to the resource write
    eirods::file_object file_obj( rsComm,
                                  FileDesc[fileWriteInp->fileInx].fileName,
                                  FileDesc[fileWriteInp->fileInx].rescHier,
                                  FileDesc[fileWriteInp->fileInx].fd,
                                  0, 0 ); 
 
    eirods::error write_err = fileWrite( rsComm,
                                         file_obj,										 
                                         fileWriteInpBBuf->buf,
                                         fileWriteInp->len );
    // =-=-=-=-=-=-=-
    // log error if necessary
    if( !write_err.ok() ) {
        std::stringstream msg;
        msg << "_rsFileWrite: fileWrite for ";
        msg << file_obj.physical_path();
        msg << ", status = ";
        msg << write_err.code();
        eirods::error err = PASS( false, write_err.code(), msg.str(), write_err );
        eirods::log( err ); 
    }

    return write_err.code();

} // _rsFileWrite
 
