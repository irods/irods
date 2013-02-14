/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See fileGetFsFreeSpace.h for a description of this API call.*/

#include "fileGetFsFreeSpace.h"
#include "miscServerFunct.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_log.h"
#include "eirods_file_object.h"
#include "eirods_stacktrace.h"

int
rsFileGetFsFreeSpace (rsComm_t *rsComm, 
                      fileGetFsFreeSpaceInp_t *fileGetFsFreeSpaceInp,
                      fileGetFsFreeSpaceOut_t **fileGetFsFreeSpaceOut)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    *fileGetFsFreeSpaceOut = NULL;

    remoteFlag = resolveHost (&fileGetFsFreeSpaceInp->addr, &rodsServerHost);
    if (remoteFlag == LOCAL_HOST) {
        status = _rsFileGetFsFreeSpace (rsComm, fileGetFsFreeSpaceInp,
                                        fileGetFsFreeSpaceOut);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteFileGetFsFreeSpace (rsComm, fileGetFsFreeSpaceInp,
                                           fileGetFsFreeSpaceOut, rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
                     "rsFileGetFsFreeSpace: resolveHost returned unrecognized value %d",
                     remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    /* Manually insert call-specific code here */

    return (status);
}

int
remoteFileGetFsFreeSpace (rsComm_t *rsComm, 
                          fileGetFsFreeSpaceInp_t *fileGetFsFreeSpaceInp,
                          fileGetFsFreeSpaceOut_t **fileGetFsFreeSpaceOut,
                          rodsServerHost_t *rodsServerHost)
{
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
                 "remoteFileGetFsFreeSpace: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }


    status = rcFileGetFsFreeSpace (rodsServerHost->conn, fileGetFsFreeSpaceInp,
                                   fileGetFsFreeSpaceOut);

    if (status < 0) { 
        rodsLog (LOG_NOTICE,
                 "remoteFileGetFsFreeSpace: rcFileGetFsFreeSpace failed for %s, status = %d",
                 fileGetFsFreeSpaceInp->fileName, status);
    }

    return status;
}

// =-=-=-=-=-=-=-
// local function to make call to freespace via resource plugin
int _rsFileGetFsFreeSpace( rsComm_t *rsComm, fileGetFsFreeSpaceInp_t *fileGetFsFreeSpaceInp, 
                           fileGetFsFreeSpaceOut_t **fileGetFsFreeSpaceOut) {

    if(fileGetFsFreeSpaceInp->objPath[0] == '\0') {

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
    // make call to freespace via resource plugin
    eirods::file_object file_obj( rsComm, fileGetFsFreeSpaceInp->objPath, fileGetFsFreeSpaceInp->fileName, fileGetFsFreeSpaceInp->rescHier,
                                  0, 0, fileGetFsFreeSpaceInp->flag );
 
    eirods::error free_err = fileGetFsFreeSpace( rsComm, file_obj );
    // =-=-=-=-=-=-=-
    // handle errors if any
    if( !free_err.ok() ) {
        std::stringstream msg;
        msg << "_rsFileGetFsFreeSpace: fileGetFsFreeSpace for ";
        msg << fileGetFsFreeSpaceInp->fileName;
        msg << ", status = ";
        msg << free_err.code();
        eirods::error err = PASS( false, free_err.code(), msg.str(), free_err );
        eirods::log ( err );
        return ((int) free_err.code());
    }

    // =-=-=-=-=-=-=-
    // otherwise its a success, set size appropriately
    *fileGetFsFreeSpaceOut = (fileGetFsFreeSpaceOut_t*)malloc (sizeof (fileGetFsFreeSpaceOut_t));
    (*fileGetFsFreeSpaceOut)->size = free_err.code();

    return (0);

} // _rsFileGetFsFreeSpace







