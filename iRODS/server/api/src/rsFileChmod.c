/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See fileChmod.h for a description of this API call.*/

#include "fileChmod.h"
#include "miscServerFunct.h"
#include "eirods_log.h"

int
rsFileChmod (rsComm_t *rsComm, fileChmodInp_t *fileChmodInp)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&fileChmodInp->addr, &rodsServerHost);
    if (remoteFlag == LOCAL_HOST) {
        status = _rsFileChmod (rsComm, fileChmodInp);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteFileChmod (rsComm, fileChmodInp, rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsFileChmod: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    /* Manually insert call-specific code here */

    return (status);
}

int
remoteFileChmod (rsComm_t *rsComm, fileChmodInp_t *fileChmodInp,
rodsServerHost_t *rodsServerHost)
{    
    int status;

        if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteFileChmod: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }


    status = rcFileChmod (rodsServerHost->conn, fileChmodInp);

    if (status < 0) { 
        rodsLog (LOG_NOTICE,
         "remoteFileOpen: rcFileChmod failed for %s",
          fileChmodInp->fileName);
    }

    return status;
}

// =-=-=-=-=-=-=-
// call chmod throught the resource plugin for a given file
int _rsFileChmod( rsComm_t *rsComm, fileChmodInp_t *fileChmodInp ) {

    // =-=-=-=-=-=-=-
	// make the call to chmod via the resource plugin
    int status = -1;
	eirods::error chmod_err = fileChmod( fileChmodInp->fileName, fileChmodInp->mode, status );

    // =-=-=-=-=-=-=-
	// log an error, if any
    if (status < 0) {
		std::stringstream msg;
		msg << "_rsFileChmod: fileChmod for ";
		msg << fileChmodInp->fileName;
		msg << ", status = ";
		msg << status;
		eirods::error err = PASS( false, status, msg.str(), chmod_err );
		eirods::log ( err );
    }

    return status;

} // _rsFileChmod




 
