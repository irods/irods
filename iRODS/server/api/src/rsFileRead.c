/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsFileRead.c - server routine that handles the fileRead
 * API
 */

/* script generated code */
#include "fileRead.h"
#include "miscServerFunct.h"
#include "rsGlobalExtern.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_log.h"
#include "eirods_file_object.h"
#include "eirods_stacktrace.h"

int
rsFileRead (rsComm_t *rsComm, fileReadInp_t *fileReadInp,
            bytesBuf_t *fileReadOutBBuf)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int retVal;


    remoteFlag = getServerHostByFileInx (fileReadInp->fileInx, 
                                         &rodsServerHost);

    if (fileReadInp->len > 0) {
        if (fileReadOutBBuf->buf == NULL)
            fileReadOutBBuf->buf = malloc (fileReadInp->len);
    } else {
        return (0);
    }
 
    if (remoteFlag == LOCAL_HOST) {
        retVal = _rsFileRead (rsComm, fileReadInp, fileReadOutBBuf);
    } else if (remoteFlag == REMOTE_HOST) {
        retVal = remoteFileRead (rsComm, fileReadInp, fileReadOutBBuf,
                                 rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
                     "rsFileRead: resolveHost returned unrecognized value %d",
                     remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    return (retVal);
}

int
remoteFileRead (rsComm_t *rsComm, fileReadInp_t *fileReadInp, 
                bytesBuf_t *fileReadOutBBuf, rodsServerHost_t *rodsServerHost)
{
    int retVal;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
                 "remoteFileRead: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((retVal = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return retVal;
    }

    fileReadInp->fileInx = convL3descInx (fileReadInp->fileInx);
    retVal = rcFileRead (rodsServerHost->conn, fileReadInp, 
                         fileReadOutBBuf);

    if (retVal < 0) { 
        rodsLog (LOG_NOTICE,
                 "remoteFileRead: rcFileRead failed for %s",
                 FileDesc[fileReadInp->fileInx].fileName);
    }

    return retVal;
}

/* _rsFileRead - this the local version of rsFileRead.
 */

int _rsFileRead( rsComm_t *rsComm, fileReadInp_t *fileReadInp, bytesBuf_t *fileReadOutBBuf ) {
    /* XXXX need to check resource permission and vault permission
     * when RCAT is available 
     */

    if(FileDesc[fileReadInp->fileInx].objPath == NULL ||
       FileDesc[fileReadInp->fileInx].objPath[0] == '\0') {

        if(true) {
            eirods::stacktrace st;
            st.trace();
            st.dump();
        }

        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Empty logical path.";
        eirods::log(LOG_ERROR, msg.str());
        return -1;
    }
    
    // =-=-=-=-=-=-=-
    // call resource plugin for POSIX read
    eirods::file_object file_obj( rsComm,
                                  FileDesc[fileReadInp->fileInx].objPath,
                                  FileDesc[fileReadInp->fileInx].fileName,
                                  FileDesc[fileReadInp->fileInx].rescHier,
                                  FileDesc[fileReadInp->fileInx].fd,  
                                  0, 0 );
                                     
    eirods::error ret = fileRead( rsComm, 
                                  file_obj,  
                                  fileReadOutBBuf->buf, 
                                  fileReadInp->len );
    // =-=-=-=-=-=-=
    // log an error if the read failed, 
    // pass long read error
    if( !ret.ok() ) {
        std::stringstream msg;
        msg << "_rsFileRead: fileRead for ";
        msg << file_obj.physical_path();
        msg << ", status = ";
        msg << ret.code();
        eirods::error err = PASS( false, ret.code(), msg.str(), ret );
        eirods::log( err );
    } else {
        fileReadOutBBuf->len = ret.code();
    }

    return ret.code();

} // _rsFileRead
 
