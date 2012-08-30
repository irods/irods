/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See fileStage.h for a description of this API call.*/

#include "fileStage.h"
#include "miscServerFunct.h"
#include "eirods_log.h"

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
	// =-=-=-=-=-=-=-
    // make call to readdir via resource plugin
    int status = -1;
    eirods::error stage_err = fileStage( fileStageInp->fileName, fileStageInp->flag, status );

     // =-=-=-=-=-=-=-
	// handle errors, if necessary
    if( !stage_err.ok() ) {
		std::stringstream msg;
		msg << "_rsFileStage: fileStage for ";
		msg << fileStageInp->fileName;
		msg << ", status = ";
		msg << status;
		eirods::error err = PASS( false, status, msg.str(), stage_err );
		eirods::log ( err );
    }

    return (status);

} // _rsFileStage
