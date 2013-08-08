/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See fileMkdir.h for a description of this API call.*/

#include "fileMkdir.h"
#include "miscServerFunct.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_log.h"
#include "eirods_collection_object.h"
#include "eirods_stacktrace.h"

int
rsFileMkdir (rsComm_t *rsComm, fileMkdirInp_t *fileMkdirInp)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&fileMkdirInp->addr, &rodsServerHost);
    if (remoteFlag == LOCAL_HOST) {
        status = _rsFileMkdir (rsComm, fileMkdirInp);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteFileMkdir (rsComm, fileMkdirInp, rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
                     "rsFileMkdir: resolveHost returned unrecognized value %d",
                     remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    /* Manually insert call-specific code here */

    return (status);
}

int
remoteFileMkdir (rsComm_t *rsComm, fileMkdirInp_t *fileMkdirInp,
                 rodsServerHost_t *rodsServerHost)
{    
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
                 "remoteFileMkdir: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }


    status = rcFileMkdir (rodsServerHost->conn, fileMkdirInp);

    if (status < 0) { 
        rodsLog (LOG_NOTICE,
                 "remoteFileOpen: rcFileMkdir failed for %s",
                 fileMkdirInp->dirName);
    }

    return status;
}

// =-=-=-=-=-=-=-
// local function to handle call to mkdir via resource plugin
int _rsFileMkdir( rsComm_t *rsComm, fileMkdirInp_t *fileMkdirInp ) {
    // =-=-=-=-=-=-=-
    // make call to mkdir via resource plugin

    eirods::collection_object coll_obj( fileMkdirInp->dirName, fileMkdirInp->rescHier, fileMkdirInp->mode, 0 );
    eirods::error mkdir_err = fileMkdir( rsComm, coll_obj );

    // =-=-=-=-=-=-=-
    // log error if necessary
    if( !mkdir_err.ok() ) {
        if( getErrno( mkdir_err.code() ) != EEXIST ) {
            std::stringstream msg;
            msg << "fileMkdir failed for ";
            msg << fileMkdirInp->dirName;
            msg << "]";
            eirods::error ret_err = PASSMSG( msg.str(), mkdir_err );
            eirods::log( ret_err );
        }
    }

    return (mkdir_err.code());

} // _rsFileMkdir


 
