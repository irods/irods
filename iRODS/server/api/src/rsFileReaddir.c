/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See fileReaddir.h for a description of this API call.*/

#include "fileReaddir.h"
#include "miscServerFunct.h"
#include "rsGlobalExtern.h"
#include "eirods_log.h"

int
rsFileReaddir (rsComm_t *rsComm, fileReaddirInp_t *fileReaddirInp, 
rodsDirent_t **fileReaddirOut)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    *fileReaddirOut = NULL;

    remoteFlag = getServerHostByFileInx (fileReaddirInp->fileInx,
      &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        status = _rsFileReaddir (rsComm, fileReaddirInp, fileReaddirOut);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteFileReaddir (rsComm, fileReaddirInp, fileReaddirOut,
	 rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsFileReaddir: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    /* Manually insert call-specific code here */

    return (status);
}

int
remoteFileReaddir (rsComm_t *rsComm, fileReaddirInp_t *fileReaddirInp,
rodsDirent_t **fileReaddirOut, rodsServerHost_t *rodsServerHost)
{   
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteFileReaddir: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    fileReaddirInp->fileInx = convL3descInx (fileReaddirInp->fileInx);
    status = rcFileReaddir (rodsServerHost->conn, fileReaddirInp, fileReaddirOut);

    if (status < 0) { 
        if (status != -1) {     /* eof */
            rodsLog (LOG_NOTICE,
             "remoteFileReaddir: rcFileReaddir failed for %s",
              FileDesc[fileReaddirInp->fileInx].fileName);
	}
    }

    return status;
}

// =-=-=-=-=-=-=-
// local function to call readdir via resource plugin
int _rsFileReaddir( rsComm_t *rsComm, fileReaddirInp_t *fileReaddirInp, rodsDirent_t **fileReaddirOut ) {

	// =-=-=-=-=-=-=-
    // have to do this for solaris for the odd d_name definition */
    #if defined(solaris_platform)
    char fileDirent[sizeof (struct dirent) + MAX_NAME_LEN];  
    struct dirent *myFileDirent = (struct dirent *) fileDirent;
    #else
    struct dirent fileDirent;
    struct dirent *myFileDirent = &fileDirent;
    #endif

	// =-=-=-=-=-=-=-
    // make call to readdir via resource plugin
    int status;
    eirods::error readdir_err = fileReaddir( FileDesc[fileReaddirInp->fileInx].fileName,
                                             FileDesc[fileReaddirInp->fileInx].driverDep, 
						                     myFileDirent,
						                     status );
    // =-=-=-=-=-=-=-
	// handle errors, if necessary
    if( !readdir_err.ok() ) {
		std::stringstream msg;
		msg << "_rsFileReaddir: fileReaddir for ";
		msg << FileDesc[fileReaddirInp->fileInx].fileName;
		msg << ", status = ";
		msg << status;
		eirods::error err = PASS( false, status, msg.str(), readdir_err );
		eirods::log ( err );

        return status;
    } else {
		// =-=-=-=-=-=-=-
		// case for handle end of file 
        if( -1 == status ) {
			return status;
		}
	}

    // =-=-=-=-=-=-=-
	// still in the beginning / middle of a read
    *fileReaddirOut = (rodsDirent_t*)malloc (sizeof (rodsDirent_t));
    status = direntToRodsDirent (*fileReaddirOut, myFileDirent);
    if( status < 0 ) {
		free (*fileReaddirOut);
		*fileReaddirOut = NULL;
    }

    return (status);

} // _rsFileReaddir 














