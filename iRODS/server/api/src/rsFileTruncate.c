/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See fileTruncate.h for a description of this API call.*/

#include "fileTruncate.h"
#include "miscServerFunct.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_log.h"
#include "eirods_file_object.h"

int
rsFileTruncate (rsComm_t *rsComm, fileOpenInp_t *fileTruncateInp)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&fileTruncateInp->addr, &rodsServerHost);
    if (remoteFlag == LOCAL_HOST) {
        status = _rsFileTruncate (rsComm, fileTruncateInp);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteFileTruncate (rsComm, fileTruncateInp, rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
                     "rsFileTruncate: resolveHost returned unrecognized value %d",
                     remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    /* Manually insert call-specific code here */

    return (status);
}

int
remoteFileTruncate (rsComm_t *rsComm, fileOpenInp_t *fileTruncateInp,
                    rodsServerHost_t *rodsServerHost)
{
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
                 "remoteFileTruncate: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }


    status = rcFileTruncate (rodsServerHost->conn, fileTruncateInp);

    if (status < 0) { 
        rodsLog (LOG_NOTICE,
                 "remoteFileTruncate: rcFileTruncate failed for %s, status = %d",
                 fileTruncateInp->fileName, status);
    }

    return status;
}

// =-=-=-=-=-=-=-
// local function which makes the call to truncate via the resource plugin
int _rsFileTruncate( rsComm_t *rsComm, fileOpenInp_t *fileTruncateInp ) {
    // =-=-=-=-=-=-=-
    // make the call to rename via the resource plugin
    eirods::file_object file_obj( rsComm, fileTruncateInp->fileName, fileTruncateInp->resc_hier_, 0, 0, 0 );
    file_obj.size( fileTruncateInp->dataSize );
    eirods::error trunc_err = fileTruncate( rsComm, file_obj );

    // =-=-=-=-=-=-=-
    // report errors if any
    if ( !trunc_err.ok() ) {
        std::stringstream msg;
        msg << "_rsFileTruncate: fileTruncate for ";
        msg << fileTruncateInp->fileName;
        msg << ", status = ";
        msg << trunc_err.code();
        eirods::error err = PASS( false, trunc_err.code(), msg.str(), trunc_err );
        eirods::log ( err );
    }

    return trunc_err.code();

} // _rsFileTruncate






