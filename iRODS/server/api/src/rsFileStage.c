/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See fileStage.h for a description of this API call.*/

#include "fileStage.h"
#include "miscServerFunct.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_log.h"
#include "eirods_file_object.h"
#include "eirods_stacktrace.h"

int
rsFileStage (rsComm_t *rsComm, fileStageInp_t *fileStageInp)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&fileStageInp->addr, &rodsServerHost);
    if (remoteFlag == LOCAL_HOST) {
        status = _rsFileStage (rsComm, fileStageInp);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteFileStage (rsComm, fileStageInp, rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
                     "rsFileStage: resolveHost returned unrecognized value %d",
                     remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    /* Manually insert call-specific code here */

    return (status);
}

int
remoteFileStage (rsComm_t *rsComm, fileStageInp_t *fileStageInp,
                 rodsServerHost_t *rodsServerHost)
{
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
                 "remoteFileStage: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }


    status = rcFileStage (rodsServerHost->conn, fileStageInp);

    if (status < 0) { 
        rodsLog (LOG_NOTICE,
                 "remoteFileStage: rcFileStage failed for %s, status = %d",
                 fileStageInp->fileName, status);
    }

    return status;
}

// =-=-=-=-=-=-=-
// local function for calling stage via resource plugin
int _rsFileStage( rsComm_t *rsComm, fileStageInp_t *fileStageInp ) {

    if(fileStageInp->objPath[0] == '\0') {

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
    // make call to readdir via resource plugin
    eirods::file_object file_obj( rsComm, fileStageInp->objPath, fileStageInp->fileName, fileStageInp->rescHier, 0, 0, fileStageInp->flag );
    eirods::error stage_err = fileStage( rsComm, file_obj );

    // =-=-=-=-=-=-=-
    // handle errors, if necessary
    if( !stage_err.ok() ) {
        std::stringstream msg;
        msg << "_rsFileStage: fileStage for ";
        msg << fileStageInp->fileName;
        msg << ", status = ";
        msg << stage_err.code();
        eirods::error err = PASS( false, stage_err.code(), msg.str(), stage_err );
        eirods::log ( err );
    }

    return stage_err.code();

} // _rsFileStage
