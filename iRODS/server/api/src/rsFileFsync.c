/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See fileFsync.h for a description of this API call.*/

#include "fileFsync.h"
#include "miscServerFunct.h"
#include "rsGlobalExtern.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_log.h"
#include "eirods_file_object.h"
#include "eirods_stacktrace.h"

int
rsFileFsync (rsComm_t *rsComm, fileFsyncInp_t *fileFsyncInp)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int retVal;

    remoteFlag = getServerHostByFileInx (fileFsyncInp->fileInx, 
                                         &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        retVal = _rsFileFsync (rsComm, fileFsyncInp);
    } else if (remoteFlag == REMOTE_HOST) {
        retVal = remoteFileFsync (rsComm, fileFsyncInp, rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
                     "rsFileFsync: resolveHost returned unrecognized value %d",
                     remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    /* Manually insert call-specific code here */

    return (retVal);
}

int
remoteFileFsync (rsComm_t *rsComm, fileFsyncInp_t *fileFsyncInp,
                 rodsServerHost_t *rodsServerHost)
{    
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
                 "remoteFileFsync: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    fileFsyncInp->fileInx = convL3descInx (fileFsyncInp->fileInx);
    status = rcFileFsync (rodsServerHost->conn, fileFsyncInp);

    if (status < 0) { 
        rodsLog (LOG_NOTICE,
                 "remoteFileFsync: rcFileFsync failed for %d, status = %d",
                 fileFsyncInp->fileInx, status);
    }

    return status;
}

// =-=-=-=-=-=-=-
// local function to handle call to fsync via resource plugin
int _rsFileFsync( 
    rsComm_t*       _comm, 
    fileFsyncInp_t* _fsync_inp ) {

    if(FileDesc[_fsync_inp->fileInx].objPath == NULL ||
       FileDesc[_fsync_inp->fileInx].objPath[0] == '\0') {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Empty logical path.";
        eirods::log(LOG_ERROR, msg.str());
        return -1;
    }
    
    // =-=-=-=-=-=-=-
    // make call to fsync via resource plugin
    eirods::file_object_ptr file_obj(  
                                new eirods::file_object( 
                                    _comm,
                                    FileDesc[_fsync_inp->fileInx].objPath,
                                    FileDesc[_fsync_inp->fileInx].fileName,
                                    FileDesc[_fsync_inp->fileInx].rescHier,
                                    FileDesc[_fsync_inp->fileInx].fd,
                                    0, 0 ) );
    eirods::error fsync_err = fileFsync( _comm, file_obj );

    // =-=-=-=-=-=-=-
    // log error if necessary
    if( !fsync_err.ok() ) {
        std::stringstream msg;
        msg << "fsync failed for [";
        msg << FileDesc[_fsync_inp->fileInx].fileName;
        msg << "]";
        eirods::error ret_err = PASSMSG( msg.str(), fsync_err );
        eirods::log( ret_err );
    }

    return (fsync_err.code());

} // _rsFileFsync
