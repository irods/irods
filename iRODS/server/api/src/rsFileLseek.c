/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See fileLseek.h for a description of this API call.*/

#include "fileLseek.h"
#include "miscServerFunct.h"
#include "rsGlobalExtern.h"


// =-=-=-=-=-=-=-
// eirods inludes
#include "eirods_log.h"
#include "eirods_file_object.h"

int
rsFileLseek (rsComm_t *rsComm, fileLseekInp_t *fileLseekInp, 
             fileLseekOut_t **fileLseekOut)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int retVal;

    *fileLseekOut = NULL;

    remoteFlag = getServerHostByFileInx (fileLseekInp->fileInx, 
                                         &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        retVal = _rsFileLseek (rsComm, fileLseekInp, fileLseekOut);
    } else if (remoteFlag == REMOTE_HOST) {
        retVal = remoteFileLseek (rsComm, fileLseekInp, 
                                  fileLseekOut, rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
                     "rsFileLseek: resolveHost returned unrecognized value %d",
                     remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    /* Manually insert call-specific code here */

    return (retVal);
}

int
remoteFileLseek (rsComm_t *rsComm, fileLseekInp_t *fileLseekInp, 
                 fileLseekOut_t **fileLseekOut, rodsServerHost_t *rodsServerHost)
{    
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
                 "remoteFileLseek: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    fileLseekInp->fileInx = convL3descInx (fileLseekInp->fileInx);
    status = rcFileLseek (rodsServerHost->conn, fileLseekInp, fileLseekOut);

    if (status < 0) { 
        rodsLog (LOG_NOTICE,
                 "remoteFileLseek: rcFileLseek failed for %d, status = %d",
                 fileLseekInp->fileInx, status);
    }

    return status;
}

// =-=-=-=-=-=-=-
// local function to handle call to stat via resource plugin
int _rsFileLseek (rsComm_t *rsComm, fileLseekInp_t *fileLseekInp, fileLseekOut_t **fileLseekOut) {
    // =-=-=-=-=-=-=-
    // make call to lseek via resource plugin
    eirods::file_object file_obj( rsComm,
                                  FileDesc[fileLseekInp->fileInx].fileName,
                                  FileDesc[fileLseekInp->fileInx].rescHier,
                                  FileDesc[fileLseekInp->fileInx].fd,
                                  0, 0 );

    eirods::error lseek_err = fileLseek( rsComm, 
                                         file_obj,
                                         fileLseekInp->offset, 
                                         fileLseekInp->whence );
    // =-=-=-=-=-=-=-
    // handle error conditions and log
    if( !lseek_err.ok() ) {
        std::stringstream msg;
        msg << "_rsFileLseek: lseek for ";
        msg << FileDesc[fileLseekInp->fileInx].fileName;
        msg << ", status = ";
        msg << lseek_err.code();
        eirods::error ret_err = PASS( false, lseek_err.code(), msg.str(), lseek_err );
        eirods::log( ret_err );
        return lseek_err.code();

    } else {
        *fileLseekOut = (fileLseekOut_t*)malloc (sizeof (fileLseekOut_t));
        memset (*fileLseekOut, 0, sizeof (fileLseekOut_t));
        (*fileLseekOut)->offset = lseek_err.code();
        return 0;
    }

} // _rsFileLseek


 
