/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See fileClose.h for a description of this API call.*/

#include "fileClose.h"
#include "miscServerFunct.h"
#include "rsGlobalExtern.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_log.h"
#include "eirods_file_object.h"



int
rsFileClose (rsComm_t *rsComm, fileCloseInp_t *fileCloseInp)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int retVal;

    remoteFlag = getServerHostByFileInx (fileCloseInp->fileInx, 
      &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        retVal = _rsFileClose (rsComm, fileCloseInp);
    } else if (remoteFlag == REMOTE_HOST) {
        retVal = remoteFileClose (rsComm, fileCloseInp, rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsFileClose: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

   /* Manually insert call-specific code here */

    freeFileDesc (fileCloseInp->fileInx);

    return (retVal);
}

int
remoteFileClose (rsComm_t *rsComm, fileCloseInp_t *fileCloseInp,
rodsServerHost_t *rodsServerHost)
{    
    int status;
    fileCloseInp_t remFileCloseInp;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteFileClose: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    memset (&remFileCloseInp, 0, sizeof (remFileCloseInp));
    remFileCloseInp.fileInx = convL3descInx (fileCloseInp->fileInx);
    status = rcFileClose (rodsServerHost->conn, &remFileCloseInp);

    if (status < 0) { 
        rodsLog (LOG_NOTICE,
         "remoteFileClose: rcFileClose failed for %d, status = %d",
          remFileCloseInp.fileInx, status);
    }

    return status;
}

int
_rsFileClose (rsComm_t *rsComm, fileCloseInp_t *fileCloseInp)
{
	// =-=-=-=-=-=-=-
	// call the resource plugin close operation 
    int status = -1;
	eirods::file_object file_obj( rsComm,
                                  FileDesc[fileCloseInp->fileInx].fileName, 
                                  FileDesc[fileCloseInp->fileInx].fd,
								  0, 0 );

    eirods::error close_err = fileClose( file_obj );
    // =-=-=-=-=-=-=-
	// log an error, if any
    if( !close_err.ok() ) {
        std::stringstream msg;
		msg << "_rsFileClose: fileClose failed for ";
		msg << fileCloseInp->fileInx;
		msg << ", status = ";
		msg << close_err.code();
        eirods::error err = PASS( false, close_err.code(), msg.str(), close_err ); 
    }

    return close_err.code();

} // _rsFileClose 








