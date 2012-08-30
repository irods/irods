/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See fileClosedir.h for a description of this API call.*/

#include "fileClosedir.h"
#include "miscServerFunct.h"
#include "rsGlobalExtern.h"
#include "eirods_log.h"

int
rsFileClosedir (rsComm_t *rsComm, fileClosedirInp_t *fileClosedirInp)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int retVal;

    remoteFlag = getServerHostByFileInx (fileClosedirInp->fileInx, 
      &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        retVal = _rsFileClosedir (rsComm, fileClosedirInp);
    } else if (remoteFlag == REMOTE_HOST) {
        retVal = remoteFileClosedir (rsComm, fileClosedirInp, rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsFileClosedir: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

   /* Manually insert call-specific code here */

    freeFileDesc (fileClosedirInp->fileInx);

    return (retVal);
}

int
remoteFileClosedir (rsComm_t *rsComm, fileClosedirInp_t *fileClosedirInp,
rodsServerHost_t *rodsServerHost)
{    
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteFileClosedir: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }


    status = rcFileClosedir (rodsServerHost->conn, fileClosedirInp);

    if (status < 0) { 
        rodsLog (LOG_NOTICE,
         "remoteFileClosedir: rcFileClosedir failed for %d, status = %d",
          fileClosedirInp->fileInx, status);
    }

    return status;
}

// =-=-=-=-=-=-=-
// local function for handling call to closedir via resource plugin
int _rsFileClosedir( rsComm_t *rsComm, fileClosedirInp_t *fileClosedirInp ) {
	// =-=-=-=-=-=-=-
	// call closedir via resource plugin, handle errors
    int status = -1;
	eirods::error closedir_err = fileClosedir( FileDesc[fileClosedirInp->fileInx].fileName,
	                                           FileDesc[fileClosedirInp->fileInx].driverDep,
											   status );
	if( !closedir_err.ok() ) {
		std::stringstream msg;
		msg << "_rsFileRmdir: fileClosedir for ";
		msg << FileDesc[fileClosedirInp->fileInx].fileName;
		msg << ", status = ";
		msg << status;
		eirods::error log_err = PASS( false, status, msg.str(), closedir_err );
		eirods::log( log_err ); 
	}

    return (status);

}  // _rsFileClosedir
